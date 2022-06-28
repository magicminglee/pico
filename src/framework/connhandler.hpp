#pragma once

#include "common.hpp"
#include "connection.hpp"
#include "contex.hpp"

NAMESPACE_FRAMEWORK_BEGIN

using CReadCBFunc = std::function<bool(const int64_t, CConnectionHandler*, std::string_view)>;
using CConnectedCBFunc = std::function<void(CConnectionHandler*)>;
using CCloseCBFunc = std::function<void(CConnectionHandler*)>;

class CConnectionHandler : public CObject {
private:
    bool m_first_package = { true };
    std::unordered_map<std::string, std::string> m_http_headers;
    CConnectedCBFunc m_connected_callback = { nullptr };
    CReadCBFunc m_read_callback = { nullptr };
    CCloseCBFunc m_close_callback = { nullptr };
    std::unique_ptr<CConnection> m_conn = { nullptr };

    int32_t handleHeadPacket(const std::string_view data);
    bool init(const int32_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, SSL* ssl, bool accept);

public:
    CConnectionHandler(CReadCBFunc readcb, CCloseCBFunc closecb);
    ~CConnectionHandler() = default;
    bool Init(const int32_t fd, SSL* ssl);
    bool Connect(std::string host, std::optional<bool> ipv6 = std::nullopt);
    CConnection* Connection() { return m_conn.get(); }
    bool SendXGameMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data);
    bool ForwardMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data);

    static void onRead(struct bufferevent* bev, void* arg);
    static void onWrite(struct bufferevent* bev, void* arg);
    static void onError(struct bufferevent* bev, short which, void* arg);
};

NAMESPACE_FRAMEWORK_END