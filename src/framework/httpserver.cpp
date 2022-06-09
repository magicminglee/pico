#include "httpserver.hpp"
#include "connection.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "worker.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

CHTTPServer::~CHTTPServer()
{
    destroy();
}

CHTTPServer::CHTTPServer(CHTTPServer&& rhs)
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
    if (self->m_ishttps) {
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, CSSLContex::Instance().CreateOneSSL(), true);
    } else {
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, nullptr, true);
    }

    return bv;
}

std::optional<const char*> CHTTPServer::GetValueByKey(evkeyvalq* headers, const char* key)
{
    if (!headers || !key)
        return std::nullopt;
    auto val = evhttp_find_header(headers, key);
    return val ? std::optional(val) : std::nullopt;
}

static void onDefault(struct evhttp_request* req, void* arg)
{
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
}

void CHTTPServer::onJsonBindCallback(evhttp_request* req, void* arg)
{
    auto filters = (FilterData*)arg;
    if (req && filters && filters->data_cb) {
        auto cmd = evhttp_request_get_command(req);
        if (0 == (filters->filter & cmd)) {
            evhttp_send_error(req, HTTP_BADMETHOD, 0);
            return;
        }
        const char* uri = evhttp_request_get_uri(req);
        if (!uri) {
            evhttp_send_error(req, HTTP_INTERNAL, 0);
            return;
        }
        auto deuri = evhttp_uri_parse_with_flags(uri, 0);
        if (!deuri) {
            evhttp_send_error(req, HTTP_BADREQUEST, 0);
            return;
        }
        // decode query string to headers
        evkeyvalq qheaders;
        if (auto qstr = evhttp_uri_get_query(deuri); qstr) {
            if (evhttp_parse_query_str(qstr, &qheaders) < 0) {
                evhttp_send_error(req, HTTP_BADREQUEST, 0);
                return;
            }
        }
        // decode headers
        auto headers = evhttp_request_get_input_headers(req);
        // decode the payload
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        char* data = (char*)evbuffer_pullup(buf, -1);
        int32_t dlen = evbuffer_get_length(buf);

        auto r = filters->data_cb(&qheaders, headers, std::string(data, dlen));

        auto evb = evbuffer_new();
        evbuffer_add_printf(evb, "%s", r.data());
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json; charset=UTF-8");
        evhttp_send_reply(req, HTTP_OK, "OK", evb);
        if (evb)
            evbuffer_free(evb);
        evhttp_uri_free(deuri);
        return;
    }
    evhttp_send_error(req, HTTP_INTERNAL, 0);
}

bool CHTTPServer::JsonRegister(const std::string path, const uint32_t methods, std::function<HttpCallbackType> cb)
{
    m_callbacks[path] = FilterData { data_cb : cb, filter : (evhttp_cmd_type)methods };
    evhttp_set_cb(m_http, path.c_str(), onJsonBindCallback, &m_callbacks[path]);
    return true;
}

bool CHTTPServer::Init(std::string host)
{
    CheckCondition(!host.empty(), true);
    CheckCondition(!m_http, true);

    m_http = evhttp_new(CWorker::MAIN_CONTEX->Base());

    auto [type, hostname, port] = CConnection::SplitUri(host);
    if (type == "https")
        m_ishttps = true;

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