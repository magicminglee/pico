#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CConnection;
class CHTTPServer : public CObject {
public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool Init(std::string host, std::function<void(CConnection*)> cb);

private:
    static struct bufferevent* onConnected(struct event_base*, void*);
    static void onFrame(struct evhttp_request*, void*);
    bool setOption();
    void destroy();

private:
    evhttp* m_http = nullptr;
    evutil_socket_t m_listenfd = -1;
    std::function<void(CConnection*)> m_connected_callback = { nullptr };
    bool m_ishttps = false;
    // DISABLE_CLASS_COPYABLE(CHTTPServer);
};

NAMESPACE_FRAMEWORK_END