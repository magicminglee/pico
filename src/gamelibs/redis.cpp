#include "redis.hpp"

#include "framework/argument.hpp"
#include "framework/utils.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace redis {
    void CRedisMgr::replyCallback(redisAsyncContext* c, void* r, void* privdata)
    {
        redisReply* reply = (redisReply*)r;
        if (!reply) {
            if (c->errstr) {
                CERROR("%s: %s", __FUNCTION__, c->errstr);
            }
            return;
        }
        RedisPrivateData* prd = (RedisPrivateData*)privdata;
        prd->callback(*reply);
    }

    bool CRedisMgr::Init()
    {
        m_handlemgr.clear();
        m_async_handlemgr.clear();
        for (auto&& v : MYARGS.RedisUrl) {
            m_handlemgr.push_back(sw::redis::Redis(v));
            addAsyncOne(v);
        }
        return true;
    }

    sw::redis::Redis* CRedisMgr::Handle(const uint32_t idx)
    {
        CheckCondition(idx < m_handlemgr.size(), nullptr);
        return &m_handlemgr[idx];
    }

    redisAsyncContext* CRedisMgr::AsynHandle(const uint32_t idx)
    {
        CheckCondition(idx < m_async_handlemgr.size(), nullptr);
        return m_async_handlemgr[idx];
    }

    bool CRedisMgr::AsynCommand(const uint32_t id, std::function<void(redisReply& r)> callback, const char* format, ...)
    {
        int32_t status = 0;
        va_list ap;
        va_start(ap, format);
        if (callback) {
            RedisPrivateData* rpd = CNEW RedisPrivateData { callback : std::move(callback) };
            status = redisvAsyncCommand(AsynHandle(id), replyCallback, rpd, format, ap);
            if (status != REDIS_OK && rpd) {
                CERROR("%s: %d", __FUNCTION__, status);
                CDEL(rpd);
            }
        } else {
            status = redisvAsyncCommand(AsynHandle(id), nullptr, nullptr, format, ap);
        }

        return status == REDIS_OK;
    }

    void CRedisMgr::SubMessage(const uint32_t id, const std::string channel,
        std::function<void(const std::string& type, const std::string& channel, const std::string& msg)> onmsg,
        std::function<void(const std::string& type, const std::string& channel, const long long num)> onmeta)
    {
        AsynCommand(
            id, [onmsg, onmeta](redisReply& reply) {
                static const std::unordered_map<std::string, sw::redis::Subscriber::MsgType> msg_type_index = {
                    { "message", sw::redis::Subscriber::MsgType::MESSAGE },
                    { "pmessage", sw::redis::Subscriber::MsgType::PMESSAGE },
                    { "subscribe", sw::redis::Subscriber::MsgType::SUBSCRIBE },
                    { "unsubscribe", sw::redis::Subscriber::MsgType::UNSUBSCRIBE },
                    { "psubscribe", sw::redis::Subscriber::MsgType::PSUBSCRIBE },
                    { "punsubscribe", sw::redis::Subscriber::MsgType::PUNSUBSCRIBE }
                };
                try {
                    if (!sw::redis::reply::is_array(reply) || reply.elements < 1 || reply.element == nullptr) {
                        throw sw::redis::ProtoError("Invalid subscribe message");
                    }
                    auto type = sw::redis::reply::parse<std::string>(*(reply.element[0]));
                    auto iter = msg_type_index.find(type);
                    if (iter == msg_type_index.end()) {
                        throw sw::redis::ProtoError("Invalid message type.");
                    }
                    switch (iter->second) {
                    case sw::redis::Subscriber::MsgType::MESSAGE: {
                        if (reply.elements != 3) {
                            throw sw::redis::ProtoError("Expect 3 sub replies");
                        }

                        assert(reply.element != nullptr);

                        auto* channel_reply = reply.element[1];
                        if (channel_reply == nullptr) {
                            throw sw::redis::ProtoError("Null channel reply");
                        }
                        auto channel = sw::redis::reply::parse<std::string>(*channel_reply);

                        auto* msg_reply = reply.element[2];
                        if (msg_reply == nullptr) {
                            throw sw::redis::ProtoError("Null message reply");
                        }
                        auto msg = sw::redis::reply::parse<std::string>(*msg_reply);
                        //CINFO("message %s %s", channel.c_str(), msg.c_str());
                        if (onmsg)
                            onmsg(type, channel, msg);
                    } break;
                    case sw::redis::Subscriber::MsgType::PMESSAGE: {
                        if (reply.elements != 4) {
                            throw sw::redis::ProtoError("Expect 4 sub replies");
                        }

                        assert(reply.element != nullptr);

                        auto* pattern_reply = reply.element[1];
                        if (pattern_reply == nullptr) {
                            throw sw::redis::ProtoError("Null pattern reply");
                        }
                        auto pattern = sw::redis::reply::parse<std::string>(*pattern_reply);

                        auto* channel_reply = reply.element[2];
                        if (channel_reply == nullptr) {
                            throw sw::redis::ProtoError("Null channel reply");
                        }
                        auto channel = sw::redis::reply::parse<std::string>(*channel_reply);

                        auto* msg_reply = reply.element[3];
                        if (msg_reply == nullptr) {
                            throw sw::redis::ProtoError("Null message reply");
                        }
                        auto msg = sw::redis::reply::parse<std::string>(*msg_reply);
                        //CINFO("pmessage %s %s %s", pattern.c_str(), channel.c_str(), msg.c_str());
                        if (onmsg)
                            onmsg(type, channel, msg);
                    } break;
                    case sw::redis::Subscriber::MsgType::SUBSCRIBE:
                    case sw::redis::Subscriber::MsgType::UNSUBSCRIBE:
                    case sw::redis::Subscriber::MsgType::PSUBSCRIBE:
                    case sw::redis::Subscriber::MsgType::PUNSUBSCRIBE: {
                        if (reply.elements != 3) {
                            throw sw::redis::ProtoError("Expect 3 sub replies");
                        }

                        assert(reply.element != nullptr);

                        auto* channel_reply = reply.element[1];
                        if (channel_reply == nullptr) {
                            throw sw::redis::ProtoError("Null channel reply");
                        }
                        auto channel = sw::redis::reply::parse<sw::redis::OptionalString>(*channel_reply);

                        auto* num_reply = reply.element[2];
                        if (num_reply == nullptr) {
                            throw sw::redis::ProtoError("Null num reply");
                        }
                        auto num = sw::redis::reply::parse<long long>(*num_reply);
                        CINFO("meta %s %s %lld", type.c_str(), channel.value_or("").c_str(), num);
                        if (onmeta)
                            onmeta(type, channel.value_or(""), num);
                    } break;
                    default:
                        assert(false);
                    }
                } catch (const sw::redis::ProtoError& e) {
                    CERROR("%s %s", __FUNCTION__, e.what());
                }
            },
            "SUBSCRIBE %s", channel.c_str());
    }

    void CRedisMgr::connectCallback(const redisAsyncContext* c, int32_t status)
    {
        if (status != REDIS_OK) {
            CERROR("Error: %s", c->errstr);
            return;
        }
        CINFO("Connected...");
    }

    void CRedisMgr::disconnectCallback(const redisAsyncContext* c, int32_t status)
    {
        if (status != REDIS_OK) {
            CERROR("Error: %s", c->errstr);
            return;
        }
        CINFO("Disconnected...");
    }

    void CRedisMgr::privdataFree(void* privdata)
    {
        sw::redis::ConnectionOptions* priv = (sw::redis::ConnectionOptions*)privdata;
        CDEL(priv);
    }

    bool CRedisMgr::addAsyncOne(std::string connurl)
    {
        redisOptions options = { 0 };
        sw::redis::ConnectionOptions* privdata = CNEW sw::redis::ConnectionOptions(connurl);
        if (privdata->type == sw::redis::ConnectionType::TCP) {
            REDIS_OPTIONS_SET_TCP(&options, privdata->host.c_str(), privdata->port);
        } else if (privdata->type == sw::redis::ConnectionType::UNIX) {
            REDIS_OPTIONS_SET_UNIX(&options, privdata->path.c_str());
        }
        REDIS_OPTIONS_SET_PRIVDATA(&options, privdata, privdataFree);
        struct timeval tv = { 0 };
        tv.tv_sec = 1;
        options.connect_timeout = &tv;
        redisAsyncContext* c = redisAsyncConnectWithOptions(&options);
        if (c->err)
            return false;

        redisLibeventAttach(c, CWorker::MAIN_CONTEX->Base());
        redisAsyncSetConnectCallback(c, connectCallback);
        redisAsyncSetDisconnectCallback(c, disconnectCallback);
        if (!privdata->password.empty())
            redisAsyncCommand(c, nullptr, nullptr, "AUTH %s", privdata->password.c_str());
        if (privdata->db > 0)
            redisAsyncCommand(c, nullptr, nullptr, "SELECT %d", privdata->db);

        m_async_handlemgr.push_back(c);
        return true;
    }
}
} //end namespace redis