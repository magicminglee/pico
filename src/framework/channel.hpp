#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

//Entrance On Worker Thread
class CChannel : public CObject {
public:
    enum class MsgType : std::uint8_t {
        Text = 0x01,
        Json = 0x02,
        Binary = 0x03
    };

private:
#pragma pack(1)
    struct MsgData {
        std::uint32_t size;
        MsgType type;
        char data[0];
    };
#pragma pack()

public:
    CChannel()
    {
        memset(m_fds, 0, sizeof(m_fds));
        memset(m_bvs, 0, sizeof(m_bvs));
    }

    ~CChannel()
    {
        if (m_bvs[0])
            bufferevent_free(m_bvs[0]);
        if (m_bvs[1])
            bufferevent_free(m_bvs[1]);
        memset(m_fds, 0, sizeof(m_fds));
        memset(m_bvs, 0, sizeof(m_bvs));
    }

    bool Create()
    {
        if (0 != evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, m_fds))
            return false;

        return true;
    }

    bool CreateBvL(CContex& ctx, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, void* arg)
    {
        m_bvs[0] = ctx.Bvsocket(m_fds[0], rcb, wcb, ecb, arg, nullptr, false);
        return m_bvs[0] != nullptr;
    }

    bool CreateBvR(CContex& ctx, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, void* arg)
    {
        m_bvs[1] = ctx.Bvsocket(m_fds[1], rcb, wcb, ecb, arg, nullptr, false);
        return m_bvs[1] != nullptr;
    }

    bool IsValid()
    {
        return m_fds[0] > 0 && m_fds[1] > 0 && m_bvs[0] && m_bvs[1];
    }

    bool WriteR(const MsgType type, std::string_view data)
    {
        CheckCondition(IsValid(), false);
        return write(false, type, std::move(data));
    }

    bool WriteL(const MsgType type, std::string_view data)
    {
        CheckCondition(IsValid(), false);
        return write(true, type, std::move(data));
    }

    std::tuple<std::optional<MsgType>, std::optional<std::string_view>> ReadR()
    {
        CheckCondition(IsValid(), std::make_tuple(std::nullopt, std::nullopt));
        return read(false);
    }

    std::tuple<std::optional<MsgType>, std::optional<std::string_view>> ReadL()
    {
        CheckCondition(IsValid(), std::make_tuple(std::nullopt, std::nullopt));
        return read(true);
    }

private:
    evutil_socket_t m_fds[2];
    bufferevent* m_bvs[2];
    static const uint16_t CHANNEL_MAX_PACKET_LENGTH = 64 * 1024 - 1;

    std::tuple<std::optional<MsgType>, std::optional<std::string_view>> read(bool left_or_right)
    {
        static thread_local char* srcpackbuf = { nullptr };
        if (!srcpackbuf)
            srcpackbuf = CNEW char[CHANNEL_MAX_PACKET_LENGTH];
        struct evbuffer* input = bufferevent_get_input(left_or_right ? m_bvs[0] : m_bvs[1]);
        size_t len = evbuffer_get_length(input);
        if (len >= sizeof(MsgData)) {
            MsgData* msg = (MsgData*)srcpackbuf;
            evbuffer_copyout(input, msg, sizeof(MsgData));
            CheckCondition(len >= sizeof(MsgData) + msg->size, std::make_tuple(std::nullopt, std::nullopt));
            CheckCondition(CHANNEL_MAX_PACKET_LENGTH >= msg->size + sizeof(MsgData), std::make_tuple(std::nullopt, std::nullopt));

            evbuffer_remove(input, msg, msg->size + sizeof(MsgData));
            return std::make_tuple(msg->type, std::string_view(msg->data, msg->size));
        }
        return std::make_tuple(std::nullopt, std::nullopt);
    }

    bool write(bool left_or_right, const MsgType type, std::string_view data)
    {
        static thread_local char* srcbuffer = { nullptr };
        if (!srcbuffer)
            srcbuffer = CNEW char[CHANNEL_MAX_PACKET_LENGTH];
        CheckCondition(data.size() + sizeof(MsgData) < CHANNEL_MAX_PACKET_LENGTH, false);
        struct evbuffer* output = bufferevent_get_output(left_or_right ? m_bvs[0] : m_bvs[1]);
        CheckCondition(output, false);
        MsgData* msg = (MsgData*)srcbuffer;
        msg->size = data.size();
        msg->type = type;
        data.copy(msg->data, data.size());
        return -1 != evbuffer_add(output, msg, msg->size + sizeof(MsgData));
    }

    DISABLE_CLASS_COPYABLE(CChannel);
};

NAMESPACE_FRAMEWORK_END