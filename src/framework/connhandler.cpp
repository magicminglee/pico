#include "connhandler.hpp"
#include "argument.hpp"
#include "connection.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "msgparser.hpp"
#include "protocol.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "utils.hpp"
#include "xlog.hpp"

#include "picohttpparser/picohttpparser.h"

NAMESPACE_FRAMEWORK_BEGIN

bool CConnectionHandler::init(const int32_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, SSL* ssl, bool accept)
{
    CConnection* c = (CConnection*)(Connection());
    CheckCondition(c, false);
    c->m_bev = CWorker::MAIN_CONTEX->Bvsocket(fd, rcb, wcb, ecb, this, ssl, accept);
    CheckCondition(c->IsValide(), false);

    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    if (fd > 0 && 0 == ::getpeername(fd, (struct sockaddr*)&addr, &len)) {
        char addrbuf[64] = { 0 };
        if (addr.ss_family == AF_INET) { // IPV4
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            c->m_peer_port = ntohs(s->sin_port);
            evutil_inet_ntop(AF_INET, &s->sin_addr, addrbuf, sizeof(addrbuf));
        } else { // IPV6
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            c->m_peer_port = ntohs(s->sin6_port);
            evutil_inet_ntop(AF_INET6, &s->sin6_addr, addrbuf, sizeof(addrbuf));
        }
        c->m_peer_ip = addrbuf;
    }
    if (fd < 0)
        c->SetId(CUtils::LinkId(++MYARGS.InitInternConnId == 0 ? ++MYARGS.InitInternConnId : MYARGS.InitInternConnId, 0));
    else
        c->SetId(CUtils::LinkId(0, fd));
    c->m_handler = this;
    SetId(fd);
    return true;
}

bool CConnectionHandler::Connect(std::string host, std::optional<bool> ipv6)
{
    auto [schema, hostname, port] = CConnection::SplitUri(host);
    SSL* ssl = nullptr;
    if (schema == "https" || schema == "wss") {
        ssl = CSSLContex::Instance().CreateOneSSL();
        if (ssl)
            SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (void*)host.data());
    }
    CheckCondition(Init(-1, ssl), false);
    if (ssl)
        bufferevent_openssl_set_allow_dirty_shutdown(m_conn->m_bev, 1);
    m_conn->m_host = host;
    m_conn->SetStreamTypeBySchema(schema);
    return m_conn->Connnect(hostname, std::stoi(port), ipv6 ? false : true);
}

void CConnectionHandler::OnConnected()
{
    if (m_connected_callback) {
        m_connected_callback(this);
    }
}

CConnectionHandler::CConnectionHandler(CReadCBFunc readcb,
    CCloseCBFunc closecb,
    CConnectedCBFunc connectedcb,
    std::unordered_map<std::string, std::string> headers)
{
    m_read_callback = std::move(readcb);
    m_close_callback = std::move(closecb);
    m_connected_callback = std::move(connectedcb);
    m_http_headers = std::move(headers);
}

bool CConnectionHandler::Init(const int32_t fd, SSL* ssl)
{
    m_first_package = true;
    m_conn.reset(CNEW CConnection());
    return CConnectionHandler::init(fd, OnRead, OnWrite, OnError, ssl, fd > 0);
}

bool CConnectionHandler::SendXGameMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data)
{
    return m_conn->SendXGameMsg(maincmd, subcmd, data);
}

bool CConnectionHandler::ForwardMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data)
{
    return m_conn->ForwardMsg(maincmd, subcmd, data);
}

int32_t CConnectionHandler::handleHeadPacket(const std::string_view data)
{
    int32_t res = -1;
    if (m_conn->IsStreamType(CConnection::StreamType::StreamType_HAProxy)) { // HAProxy
        XGAMEHAProxyHeader hdr;
        memset(&hdr, 0, sizeof(hdr));
        uint32_t size = data.size() > sizeof(hdr) ? sizeof(hdr) : data.size();
        CDecoder<XGAMEHAProxyHeader> hadec(data.data(), size);
        res = hadec.Decode(hdr);
        if (res > 0) {
            char peerIp[64] = { 0 };
            snprintf(peerIp, sizeof(peerIp) - 1, "%u.%u.%u.%u",
                hdr.hahdr.v2.addr.ip4.src_addr >> 24,
                (hdr.hahdr.v2.addr.ip4.src_addr >> 16) & 0x00FF,
                (hdr.hahdr.v2.addr.ip4.src_addr & 0xFF00) >> 8,
                hdr.hahdr.v2.addr.ip4.src_addr & 0xFF);

            m_conn->m_peer_ip = peerIp;
            m_conn->m_peer_port = hdr.hahdr.v2.addr.ip4.src_port;

            CDEBUG(
                "CTX:%s HAProxy client addrinfo:%p,%ld,srcip:%u.%u.%u.%u,dstip:%u.%u.%u.%u,srcport:%u,dstport:%u",
                MYARGS.CTXID.c_str(),
                this, Id(), hdr.hahdr.v2.addr.ip4.src_addr >> 24,
                (hdr.hahdr.v2.addr.ip4.src_addr >> 16) & 0x00FF,
                (hdr.hahdr.v2.addr.ip4.src_addr & 0xFF00) >> 8,
                hdr.hahdr.v2.addr.ip4.src_addr & 0xFF,
                hdr.hahdr.v2.addr.ip4.dst_addr >> 24,
                (hdr.hahdr.v2.addr.ip4.dst_addr >> 16) & 0x00FF,
                (hdr.hahdr.v2.addr.ip4.dst_addr & 0xFF00) >> 8,
                hdr.hahdr.v2.addr.ip4.dst_addr & 0xFF,
                hdr.hahdr.v2.addr.ip4.src_port, hdr.hahdr.v2.addr.ip4.dst_port);

            return res;
        } else if (0 == res) {
            CDEBUG("CTX:%s HAProxy to be continue:%p,%ld", MYARGS.CTXID.c_str(), this, Id());
        }
    } else if (m_conn->IsStreamType(CConnection::StreamType::StreamType_WS) || m_conn->IsStreamType(CConnection::StreamType::StreamType_WSS)) { // WS/WSS
        std::string upgrade;
        res = m_conn->m_ws.Handshake(data.data(), data.size(), upgrade);
        if (res == -2) {
            res = 0;
            CDEBUG("CTX:%s websocket handshake to be continue:%p,%ld", MYARGS.CTXID.c_str(), this, Id());
        } else if (res > 0) {
            CDEBUG("CTX:%s websocket handshake to be OK:%p,%ld,%s", MYARGS.CTXID.c_str(), this, Id(), upgrade.c_str());
            if (!upgrade.empty())
                m_conn->sendcmd(upgrade.data(), upgrade.size());

            if (m_connected_callback)
                m_connected_callback(this);
            return res;
        }
    }
    return res;
}

void CConnectionHandler::OnRead(struct bufferevent* bev, void* arg)
{
    static thread_local char* buffer = { nullptr };
    if (!buffer)
        buffer = CNEW char[MAX_WATERMARK_SIZE];
    CConnectionHandler* self = (CConnectionHandler*)arg;
    struct evbuffer* input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);
    if (len <= 0 || len >= MAX_WATERMARK_SIZE) {
        CERROR("%p,%ld Buffer is overflow!", self, self->Connection()->Id());
        if (self->m_close_callback)
            self->m_close_callback(self);
        return;
    }
    while (len > 0 && len < MAX_WATERMARK_SIZE) {
        if (self->m_first_package) {
            uint32_t dlen = len > MAX_WATERMARK_SIZE ? MAX_WATERMARK_SIZE : len;
            evbuffer_copyout(input, buffer, dlen);
            int32_t ret = self->handleHeadPacket(std::string_view(buffer, dlen));
            if (ret > 0) {
                evbuffer_drain(input, ret);
                len = evbuffer_get_length(input);
                //<<<packet(HAProxy->Websocket/Http)
                if (self->m_conn->IsStreamType(CConnection::StreamType::StreamType_HAProxy)) {
                    self->m_conn->ShellProxy();
                    continue;
                }
            } else if (ret == 0) {
                break;
            }
            self->m_first_package = false;
            continue;
        }

        if (self->m_conn->IsStreamType(CConnection::StreamType::StreamType_WS) || self->m_conn->IsStreamType(CConnection::StreamType::StreamType_WSS)) {
            uint32_t dlen = len > MAX_WATERMARK_SIZE ? MAX_WATERMARK_SIZE : len;
            evbuffer_copyout(input, buffer, dlen);
            int32_t ret = self->m_conn->m_ws.Process(buffer, dlen);
            if (0 == ret) {
                break;
            } else if (ret < 0) {
                CERROR("CTX:%s %p too large websocket frame %u ret %d", MYARGS.CTXID.c_str(), self->m_conn.get(), dlen, ret);
                if (self->m_close_callback)
                    self->m_close_callback(self);
                break;
            }
            if (self->m_conn->m_ws.IsFin()) {
                if (self->m_conn->m_ws.IsPong()) {
                } else if (self->m_conn->m_ws.IsPing()) {
                    auto cf = self->m_conn->m_ws.ControlFrame();
                    if (!cf.empty()) {
                        self->m_conn->SendCmd(cf.data(), cf.size(), CWebSocket::WS_OPCODE_PONG);
                        // CINFO("CTX:%s %p control frame ping %lu", MYARGS.CTXID.c_str(), self->m_conn.get(), cf.size());
                    }
                } else if (self->m_conn->m_ws.IsClose()) {
                    auto cf = self->m_conn->m_ws.ControlFrame();
                    if (!cf.empty()) {
                        self->m_conn->SetFlag(CConnection::ConnectionFlags::ConnectionFlags_Destroy);
                        self->m_conn->SendCmd(cf.data(), cf.size(), CWebSocket::WS_OPCODE_CLOSE);
                        CINFO("CTX:%s %p control frame close %lu", MYARGS.CTXID.c_str(), self->m_conn.get(), cf.size());
                    }
                } else {
                    if (auto res = self->m_read_callback(self->m_conn->Id(), self, std::string_view(self->m_conn->m_ws.Rcvbuf(), self->m_conn->m_ws.FrameLen())); !res)
                        CERROR("CTX:%s %ld command is not register cmdlen 0x%llx", MYARGS.CTXID.c_str(), self->Id(), self->m_conn->m_ws.FrameLen());
                }
            }
            evbuffer_drain(input, ret);
            len = evbuffer_get_length(input);
            continue;
        } else {
            CERROR("CTX:%s %ld %lu not supported socket protocol type", MYARGS.CTXID.c_str(), self->Id(), len);
            if (self->m_close_callback)
                self->m_close_callback(self);
            break;
        }
    }
}

void CConnectionHandler::OnWrite(struct bufferevent* bev, void* arg)
{
    CConnectionHandler* self = (CConnectionHandler*)arg;
    if (self->m_conn->Destroy()) {
        CINFO("CTX:%s %s need to recycle %p,%ld,%p", MYARGS.CTXID.c_str(), __FUNCTION__, self->m_conn.get(), self->m_conn->Id(), bev);
        if (self->m_close_callback)
            self->m_close_callback(self);
    }
}

void CConnectionHandler::OnError(struct bufferevent* bev, short which, void* arg)
{
    CConnectionHandler* self = (CConnectionHandler*)arg;
    if (which & BEV_EVENT_CONNECTED) {
        CINFO("CTX:%s connection has connected:%p,%ld,0x%x,%p", MYARGS.CTXID.c_str(), self->m_conn.get(), self->m_conn->Id(), which, self->m_conn.get());
        self->m_conn->OnConnected(self->m_http_headers);
        if (self->m_connected_callback && self->m_conn->IsStreamType(CConnection::StreamType::StreamType_HTTPS))
            self->m_connected_callback(self);
        return;
    } else if (which & BEV_EVENT_EOF) {
    } else if (which & BEV_EVENT_ERROR) {
    } else if (which & BEV_EVENT_TIMEOUT) {
    }
    CINFO("CTX:%s %s %p,%ld,0x%x,%p", MYARGS.CTXID.c_str(), __FUNCTION__, self->m_conn.get(), self->m_conn->Id(), which, bev);
    if (self->m_close_callback)
        self->m_close_callback(self);
}

NAMESPACE_FRAMEWORK_END