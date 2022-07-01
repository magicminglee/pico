#pragma once

#include "common.hpp"
#include "connection.hpp"
#include "contex.hpp"

NAMESPACE_FRAMEWORK_BEGIN

enum class EnumConnEventType : uint16_t {
    EnumConnEventType_Connected = 1,
    EnumConnEventType_Closed = 2,
};
using CReadCBFunc = std::function<size_t(std::string_view)>;
using CEventCBFunc = std::function<void(const EnumConnEventType)>;
using CWriteCBFunc = std::function<void()>;

class CConnectionHandler : public CObject {
private:
    CReadCBFunc m_read_callback = { nullptr };
    CWriteCBFunc m_write_callback = { nullptr };
    CEventCBFunc m_event_callback = { nullptr };
    std::unique_ptr<CConnection> m_conn = { nullptr };

    bool init(const int32_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, SSL* ssl, bool accept);

public:
    CConnectionHandler() = default;
    ~CConnectionHandler() = default;
    void Register(CReadCBFunc readcb, CWriteCBFunc closecb, CEventCBFunc connectcb);
    bool Init(const int32_t fd, const std::string host);
    bool Connect(std::string host, std::optional<bool> ipv6 = std::nullopt);
    CConnection* Connection() { return m_conn.get(); }
    bool SendXGameMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data);
    bool ForwardMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data);

    static void onRead(struct bufferevent* bev, void* arg);
    static void onWrite(struct bufferevent* bev, void* arg);
    static void onError(struct bufferevent* bev, short which, void* arg);

    // debug
    void* H2Session = { nullptr };
};

NAMESPACE_FRAMEWORK_END