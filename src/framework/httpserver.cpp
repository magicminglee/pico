#include "httpserver.hpp"
#include "connection.hpp"
#include "http.hpp"
#include "stringtool.hpp"
#include "worker.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

CHTTPServer::~CHTTPServer()
{
    destroy();
}

CHTTPServer::CHTTPServer(CHTTPServer&& rhs)
    : m_connected_callback(std::move(rhs.m_connected_callback))
{
}

void CHTTPServer::destroy()
{
    if (m_http) {
        evhttp_free(m_http);
        m_http = nullptr;
    }
}

struct bufferevent* CHTTPServer::onConnected(struct event_base* base, void* arg)
{
    CHTTPServer* self = (CHTTPServer*)arg;
    bufferevent* bv = nullptr;
    auto c = new CConnection();
    if (self->m_ishttps) {
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, CHttpContex::Instance().CreateOneSSL(), true);
        c->SetSockType(CConnection::StreamType::StreamType_HTTPS);
    } else {
        c->SetSockType(CConnection::StreamType::StreamType_HTTP);
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, nullptr, true);
    }
    c->SetBufferEvent(bv);

    if (self->m_connected_callback)
        self->m_connected_callback(c);
    return bv;
}

static void onDefault(struct evhttp_request* req, void* arg)
{
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
}

bool CHTTPServer::Register(const std::string_view path, std::function<void()> cb)
{
    // evhttp_set_cb(m_http, path.data(), );
    return true;
}

bool CHTTPServer::Init(std::string host, std::function<void(CConnection*)> cb)
{
    CheckCondition(!host.empty(), true);
    CheckCondition(!m_http, true);

    m_http = evhttp_new(CWorker::MAIN_CONTEX->Base());

    auto [type, hostname, port] = CConnection::SplitUri(host);
    if (type == "https")
        m_ishttps = true;

    m_connected_callback = std::move(cb);

    auto handle = evhttp_bind_socket_with_handle(m_http, "0.0.0.0", CStringTool::ToInteger<uint16_t>(port));
    if (!handle) {
        destroy();
        return false;
    }
    setOption(evhttp_bound_socket_get_fd(handle));
    evhttp_set_bevcb(m_http, onConnected, this);
    evhttp_set_gencb(m_http, onDefault, this);

    return true;
}

bool CHTTPServer::setOption(const int32_t fd)
{
    if (evutil_make_listen_socket_reuseable(fd) < 0)
        return false;
    if (evutil_make_listen_socket_reuseable_port(fd) < 0)
        return false;
    if (evutil_make_socket_nonblocking(fd) < 0)
        return false;
#ifdef LINUX_PLATFORMOS
    int32_t flags = 1;
    int32_t error = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));
    if (0 != error)
        return false;
#endif
    return true;
}

NAMESPACE_FRAMEWORK_END