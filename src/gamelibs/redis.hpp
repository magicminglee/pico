#pragma once
#include "framework/argument.hpp"
#include "framework/contex.hpp"
#include "framework/singleton.hpp"
#include "framework/utils.hpp"

#include "sw/redis++/redis++.h"

#include "hiredis/adapters/libevent.h"
#include "hiredis/async.h"
#include "hiredis/hiredis.h"

USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace redis {
    class CRedisMgr : public CTLSingleton<CRedisMgr> {
    private:
        struct RedisPrivateData {
            std::function<void(redisReply& r)> callback;
        };

    public:
        CRedisMgr() = default;
        ~CRedisMgr() = default;
        bool Init();
        sw::redis::Redis* Handle(const uint32_t idx);
        redisAsyncContext* AsynHandle(const uint32_t idx);
        bool AsynCommand(const uint32_t id, std::function<void(redisReply& r)> callback, const char* format, ...);
        void SubMessage(const uint32_t id, const std::string channel,
            std::function<void(const std::string& type, const std::string& channel, const std::string& msg)> onmsg,
            std::function<void(const std::string& type, const std::string& channel, const long long num)> onmeta = nullptr);

    private:
        static void replyCallback(redisAsyncContext* c, void* r, void* privdata);
        static void connectCallback(const redisAsyncContext* c, int32_t status);
        static void disconnectCallback(const redisAsyncContext* c, int32_t status);
        static void privdataFree(void* privdata);
        bool addAsyncOne(std::string connurl);

    private:
        std::vector<sw::redis::Redis> m_handlemgr;
        std::vector<redisAsyncContext*> m_async_handlemgr;
    };
}
} //end namespace redis