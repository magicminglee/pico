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

CEvent* CEvent::Create(evutil_socket_t fd, short flags, std::function<void()> func)
{
    auto event = CNEW CEvent();
    if (auto ev = event_new(CContex::MAIN_CONTEX->Base(), fd, flags, CEvent::callback, event); ev) {
        event->m_evfunc = std::move(func);
        event->m_event = ev;
        return event;
    } else {
        CDEL(event);
    }
    return nullptr;
}
// End CEvent

thread_local std::shared_ptr<CContex> CContex::MAIN_CONTEX = nullptr;
CContex::CContex(event_base* base)
    : m_base(base)
{
}

CContex::~CContex()
{
    if (m_base) {
        event_base_free(m_base);
        m_base = nullptr;
    }
    Destroy();
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
    auto ev = CEvent::Create(fd, flags, cb);

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
        if (accept)
            bv = bufferevent_openssl_socket_new(m_base, fd, ssl, BUFFEREVENT_SSL_ACCEPTING, BEV_OPT_CLOSE_ON_FREE);
        else
            bv = bufferevent_openssl_socket_new(m_base, fd, ssl, BUFFEREVENT_SSL_CONNECTING, BEV_OPT_CLOSE_ON_FREE);
    } else {
        bv = bufferevent_socket_new(m_base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setwatermark(bv, EV_READ | EV_WRITE, 0, MAX_WATERMARK_SIZE * 4);
    }
    if (!bv) {
        return nullptr;
    }
    bufferevent_enable(bv, EV_READ | EV_WRITE);
    bufferevent_setcb(bv, rcb, wcb, ecb, arg);
    return bv;
}

evhttp_connection* CContex::CreateHttpConnection(bufferevent* bev, const char* address, unsigned short port)
{
    assert(nullptr != m_base);
    return evhttp_connection_base_bufferevent_new(m_base, nullptr, bev, address, port);
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

NAMESPACE_FRAMEWORK_END