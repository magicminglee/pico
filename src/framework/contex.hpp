#pragma once
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_ssl.h"
#include "event2/event.h"
#include "event2/http.h"
#include "event2/util.h"
#include "event2/keyvalq_struct.h"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

#include "common.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CEvent : public CObject {
public:
    CEvent();
    ~CEvent();
    struct event* Event() { return m_event; }
    static CEvent* Create(event_base* base, evutil_socket_t fd, short flags, std::function<void()> func);

private:
    static void callback(evutil_socket_t, short, void*);

    struct event* m_event = { nullptr };
    std::function<void()> m_evfunc;
    DISABLE_CLASS_COPYABLE(CEvent);
};
class CContex : public CObject {
public:
    CContex();
    ~CContex();
    event_base* Base();
    void Loop();
    void Exit(const int32_t t);
    CEvent* Register(evutil_socket_t fd, short flags, const timeval* tv, std::function<void()> cb);
    void UnRegister(CEvent* ev);
    bufferevent* Bvsocket(evutil_socket_t fd, bufferevent_data_cb rcb, bufferevent_data_cb wcb, bufferevent_event_cb ecb, void* arg, SSL* ssl, bool accept);
    bool AddEvent(const uint32_t id, const uint32_t t, std::function<void()> cb);
    bool AddPersistEvent(const uint32_t id, const uint32_t t, std::function<void()> cb);
    void DelEvent(const uint32_t id);
    bool HasEvent(const uint32_t id);
    void Destroy();
    evhttp_connection* CreateHttpConnection(bufferevent* bev, const char* address, unsigned short port);

private:
    event_base* m_base;
    using EventMap = std::map<const uint32_t, CEvent*>;
    EventMap m_em;

    DISABLE_CLASS_COPYABLE(CContex);
};

NAMESPACE_FRAMEWORK_END