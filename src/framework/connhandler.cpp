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

NAMESPACE_FRAMEWORK_BEGIN

bool CConnectionHandler::init(const int32_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, SSL* ssl, bool accept)
{
    CConnection* c = (CConnection*)(Connection());
    CheckCondition(c, false);
    c->m_bev = CContex::MAIN_CONTEX->Bvsocket(fd, rcb, wcb, ecb, this, ssl, accept);
    CheckCondition(c->IsValide(), false);

    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    if (fd > 0 && 0 == ::getpeername(fd, (struct sockaddr*)&addr, &len)) {
        char addrbuf[64] = { 0 };
        if (addr.ss_family == AF_INET) { // IPV4
            struct sockaddr_in* s = (struct sockaddr_in*)&addr;
            c->m_peer_port = CUtils::Ntoh16(s->sin_port);
            evutil_inet_ntop(AF_INET, &s->sin_addr, addrbuf, sizeof(addrbuf));
        } else { // IPV6
            struct sockaddr_in6* s = (struct sockaddr_in6*)&addr;
            c->m_peer_port = CUtils::Ntoh16(s->sin6_port);
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
    auto [schema, hostname, port, path] = CConnection::SplitUri(host);
    CheckCondition(Init(-1, host), false);
    m_conn->m_host = host;
    m_conn->SetStreamTypeBySchema(schema);
    return m_conn->Connnect(hostname, std::stoi(port), ipv6 ? false : true);
}

void CConnectionHandler::Register(CReadCBFunc readcb, CWriteCBFunc writecb, CEventCBFunc eventcb)
{
    m_read_callback = readcb;
    m_write_callback = writecb;
    m_event_callback = eventcb;
}

bool CConnectionHandler::InitProxy(const std::string& hostname)
{
    auto bv = bufferevent_openssl_filter_new(CContex::MAIN_CONTEX->Base(),
        Connection()->GetBufEvent(), CSSLContex::Instance().CreateOneSSL(false, hostname),
        BUFFEREVENT_SSL_CONNECTING,
        BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bv, onRead, onWrite, onError, this);
    Connection()->SetBufferEvent(bv);
    return true;
}

bool CConnectionHandler::EnableProxy()
{
    if (-1 == bufferevent_enable(Connection()->GetBufEvent(), EV_READ | EV_WRITE)) {
        SPDLOG_ERROR("{} bufferevent_enable", __FUNCTION__);
        return false;
    }
    bufferevent_setwatermark(Connection()->GetBufEvent(), EV_READ | EV_WRITE, 0, MAX_WATERMARK_SIZE * 4);
    return true;
}

bool CConnectionHandler::Init(const int32_t fd, const std::string host)
{
    auto [schema, hostname, port, path] = CConnection::SplitUri(host);
    SSL* ssl = nullptr;
    if (schema == "https" || schema == "wss") {
        ssl = CSSLContex::Instance().CreateOneSSL(fd > 0, hostname);
    }
    m_conn.reset(CNEW CConnection());
    m_conn->SetStreamTypeBySchema(schema);
    return CConnectionHandler::init(fd, onRead, onWrite, onError, ssl, fd > 0);
}

void CConnectionHandler::onRead(struct bufferevent* bev, void* arg)
{
    CConnectionHandler* self = (CConnectionHandler*)arg;
    struct evbuffer* input = bufferevent_get_input(bev);
    if (self->m_read_callback) {
        auto data = (char*)evbuffer_pullup(input, -1);
        auto dlen = evbuffer_get_length(input);
        std::string_view sv(data, dlen);
        auto readlen = self->m_read_callback(sv);
        if (readlen > 0) {
            evbuffer_drain(input, readlen);
        } else if (readlen == 0) {
        } else {
            if (self->m_event_callback) {
                self->m_event_callback(EnumConnEventType::EnumConnEventType_Closed);
                self->m_event_callback = nullptr;
            }
            CDEL(self);
        }
    }
}

void CConnectionHandler::onWrite(struct bufferevent* bev, void* arg)
{
    CConnectionHandler* self = (CConnectionHandler*)arg;
    if (self->m_conn->IsClosing()) {
        SPDLOG_INFO("CTX:{} {} need to recycle {}, {}, {}", MYARGS.CTXID, __FUNCTION__, fmt::ptr(self->m_conn.get()), self->m_conn->Id(), fmt::ptr(bev));
        if (self->m_event_callback) {
            self->m_event_callback(EnumConnEventType::EnumConnEventType_Closed);
            self->m_event_callback = nullptr;
        }
        CDEL(self);
    } else {
        if (self->m_write_callback) {
            self->m_write_callback();
        }
    }
}

void CConnectionHandler::onError(struct bufferevent* bev, short which, void* arg)
{
    CConnectionHandler* self = (CConnectionHandler*)arg;
    if (which & BEV_EVENT_CONNECTED) {
        if (self->m_event_callback) {
            if (bufferevent_openssl_get_ssl(bev)) {
                bufferevent_enable(bev, EV_READ | EV_WRITE);
                bufferevent_setwatermark(bev, EV_READ | EV_WRITE, 0, MAX_WATERMARK_SIZE * 4);
            }
            self->m_event_callback(EnumConnEventType::EnumConnEventType_Connected);
        }
        return;
    } else if (which & BEV_EVENT_EOF) {
    } else if (which & BEV_EVENT_ERROR) {
    } else if (which & BEV_EVENT_TIMEOUT) {
    }
    SPDLOG_INFO("CTX:{} {} {},{},0x{:x},{}", MYARGS.CTXID, __FUNCTION__, fmt::ptr(self->m_conn.get()), self->m_conn->Id(), which, fmt::ptr(bev));
    if (self->m_event_callback) {
        self->m_event_callback(EnumConnEventType::EnumConnEventType_Closed);
        self->m_event_callback = nullptr;
    }
    CDEL(self);
}

NAMESPACE_FRAMEWORK_END