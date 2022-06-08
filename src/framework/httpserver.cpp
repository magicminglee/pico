#include "httpserver.hpp"
#include "connection.hpp"
#include "http.hpp"
#include "stringtool.hpp"
#include "worker.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

static const int32_t BACKLOG_SIZE = 512;

CHTTPServer::~CHTTPServer()
{
    destroy();
}

CHTTPServer::CHTTPServer(CHTTPServer&& rhs)
    : m_listenfd(-1)
    , m_connected_callback(std::move(rhs.m_connected_callback))
{
}

void CHTTPServer::destroy()
{
    if (m_http) {
        evhttp_free(m_http);
        m_http = nullptr;
    }
    if (m_listenfd > 0) {
        evutil_closesocket(m_listenfd);
        m_listenfd = -1;
    }
}

struct bufferevent* CHTTPServer::onConnected(struct event_base* base, void* arg)
{
    CHTTPServer* self = (CHTTPServer*)arg;
    SSL* ssl = nullptr;
    if (self->m_ishttps)
        ssl = CHttpContex::Instance().CreateOneSSL();
    auto bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, ssl, true);
    auto c = new CConnection();
    c->SetBufferEvent(bv);
    if (self->m_ishttps)
        c->SetSockType(CConnection::StreamType::StreamType_HTTPS);
    else
        c->SetSockType(CConnection::StreamType::StreamType_HTTP);

    if (self->m_connected_callback)
        self->m_connected_callback(c);
    return bv;
}

void CHTTPServer::onFrame(struct evhttp_request* req, void* arg)
{
    CHTTPServer* self = (CHTTPServer*)arg;
    const char* uri = evhttp_request_get_uri(req);
    CINFO("%s %s", __FUNCTION__, uri);
    evhttp_send_reply(req, 200, "OK", nullptr);
    // evhttp_send_error(req, HTTP_BADREQUEST, 0);
}

bool CHTTPServer::Init(std::string host, std::function<void(CConnection*)> cb)
{
    CheckCondition(!host.empty(), true);
    if (-1 != m_listenfd)
        return true;

    if (!m_http)
        m_http = evhttp_new(CWorker::MAIN_CONTEX->Base());

    auto [type, hostname, port] = CConnection::SplitUri(host);
    struct addrinfo hints;
    struct addrinfo *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */

    if (type == "https")
        m_ishttps = true;
    int32_t ret = -1;
    if (hostname == "*") {
        ret = evutil_getaddrinfo(nullptr, port.c_str(), &hints, &result);
    } else {
        ret = evutil_getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    }
    if (ret != 0) {
        return false;
    }
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        m_listenfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (m_listenfd == -1)
            continue;

        if (!setOption()) {
            destroy();
            continue;
        }
        break;
    }
    if (nullptr == rp) {
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);

    m_connected_callback = std::move(cb);
    evhttp_set_bevcb(m_http, onConnected, this);
    evhttp_set_gencb(m_http, onFrame, this);

    if (evhttp_bind_socket(m_http, "0.0.0.0", CStringTool::ToInteger<uint16_t>(port)) < 0) {
        destroy();
        return false;
    }

    return true;
}

bool CHTTPServer::setOption()
{
    if (evutil_make_listen_socket_reuseable(m_listenfd) < 0)
        return false;
    if (evutil_make_listen_socket_reuseable_port(m_listenfd) < 0)
        return false;
    if (evutil_make_socket_nonblocking(m_listenfd) < 0)
        return false;
#ifdef LINUX_PLATFORMOS
    int32_t flags = 1;
    int32_t error = ::setsockopt(m_listenfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));
    if (0 != error)
        return false;
#endif
    return true;
}

NAMESPACE_FRAMEWORK_END