#include "contex.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

CEvent::CEvent()
{
}

CEvent::~CEvent()
{
    if (m_event) {
        event_del(m_event);
        event_free(m_event);
    }
}

void CEvent::callback(evutil_socket_t fd, short which, void* arg)
{
    CEvent* event = (CEvent*)arg;
    if (event && event->m_evfunc) {
        event->m_evfunc();
    }
}

CEvent* CEvent::Create(event_base* base, evutil_socket_t fd, short flags, std::function<void()> func)
{
    auto event = CNEW CEvent();
    if (auto ev = event_new(base, fd, flags, CEvent::callback, event); ev) {
        event->m_evfunc = std::move(func);
        event->m_event = ev;
        return event;
    } else {
        CDEL(event);
    }
    return nullptr;
}
// End CEvent

CContex::CContex()
{
    m_base = event_base_new();
    assert(nullptr != m_base);
}

CContex::~CContex()
{
    if (m_base) {
        event_base_free(m_base);
        m_base = nullptr;
    }
}

event_base* CContex::Base()
{
    return m_base;
}

void CContex::Loop()
{
    assert(0 == event_base_dispatch(m_base));
}

void CContex::Exit(const int32_t t)
{
    timeval tv;
    tv.tv_sec = t;
    tv.tv_usec = 0;
    event_base_loopexit(m_base, &tv);
}

CEvent* CContex::Register(evutil_socket_t fd, short flags, const timeval* tv, std::function<void()> cb)
{
    assert(nullptr != m_base);
    auto ev = CEvent::Create(m_base, fd, flags, cb);

    if (event_add(ev->Event(), tv) == -1) {
        CDEL(ev);
        return nullptr;
    }
    return ev;
}

void CContex::UnRegister(CEvent* ev)
{
    if (ev) {
        CDEL(ev);
    }
}

struct bufferevent* CContex::Bvsocket(evutil_socket_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, void* arg, SSL* ssl, bool accept)
{
    assert(nullptr != m_base);
    struct bufferevent* bv = nullptr;
    if (ssl) {
        bv = bufferevent_openssl_socket_new(m_base, fd, ssl, accept ? BUFFEREVENT_SSL_ACCEPTING : BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    } else {
        bv = bufferevent_socket_new(m_base, fd, BEV_OPT_CLOSE_ON_FREE);
    }
    if (!bv) {
        return nullptr;
    }
    bufferevent_setcb(bv, rcb, wcb, ecb, arg);
    if (-1 == bufferevent_enable(bv, EV_READ)) {
        bufferevent_free(bv);
        return nullptr;
    }

    bufferevent_setwatermark(bv, EV_READ, 0, MAX_WATERMARK_SIZE * 4);
    return bv;
}

bool CContex::AddEvent(const uint32_t id, const uint32_t t, std::function<void()> cb)
{
    auto it = m_em.find(id);
    if (it != m_em.end()) {
        UnRegister(it->second);
    }

    timeval tv;
    tv.tv_sec = t / 1000;
    tv.tv_usec = (t - tv.tv_sec * 1000) * 1000;
    auto ev = Register(-1, 0, &tv, cb);
    if (nullptr == ev)
        return false;

    m_em[id] = ev;
    return true;
}

bool CContex::AddPersistEvent(const uint32_t id, const uint32_t t, std::function<void()> cb)
{
    auto it = m_em.find(id);
    if (it != m_em.end()) {
        UnRegister(it->second);
    }

    timeval tv;
    tv.tv_sec = t / 1000;
    tv.tv_usec = (t - tv.tv_sec * 1000) * 1000;
    auto ev = Register(-1, EV_PERSIST, &tv, cb);
    if (nullptr == ev)
        return false;

    m_em[id] = ev;
    return true;
}

void CContex::DelEvent(const uint32_t id)
{
    auto it = m_em.find(id);
    if (it != m_em.end()) {
        UnRegister(it->second);
        m_em.erase(it);
    }
}

bool CContex::HasEvent(const uint32_t id)
{
    return m_em.find(id) == m_em.end() ? false : true;
}

void CContex::Destroy()
{
    for (auto& v : m_em) {
        UnRegister(v.second);
    }
    m_em.clear();
}

evhttp_connection* CContex::CreateHttpConnection(bufferevent* bev, const char* address, unsigned short port)
{
    return evhttp_connection_base_bufferevent_new(m_base, nullptr, bev, address, port);
}

NAMESPACE_FRAMEWORK_END