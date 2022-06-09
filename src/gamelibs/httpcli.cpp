#include "httpcli.hpp"

#include "framework/xlog.hpp"
#include "framework/ssl.hpp"
#include "framework/worker.hpp"

#include "openssl/md5.h"

namespace gamelibs {
namespace httpcli {

    CHttpCli::CHttpCli()
    {
        CSSLContex::Instance().IncsQueueCnt();
    }

    CHttpCli::~CHttpCli()
    {
        CSSLContex::Instance().DescQueueCnt();
    }

    bool CHttpCli::Emit(std::string_view url,
        std::optional<std::string_view> data,
        CallbackFuncType cb,
        std::map<std::string, std::string> headers)
    {
        CHttpCli* self = CNEW CHttpCli();
        self->m_callback = std::move(cb);
        if (!self->request(url.data(), std::move(data), headers, CHttpCli::delegateCallback)) {
            CDEL(self);
            return false;
        }
        return true;
    }

    void CHttpCli::Recovery(CHttpCli* self)
    {
        evhttp_connection_free(self->m_evcon);
        CDEL(self);
    }

    void CHttpCli::delegateCallback(struct evhttp_request* req, void* arg)
    {
        static thread_local char* recv_buffer = { nullptr };
        if (!recv_buffer)
            recv_buffer = CNEW char[MAX_WATERMARK_SIZE];
        constexpr uint32_t MAX_HEADER_SIZE = 32;
        std::pair<int32_t, std::string> result;
        CHttpCli* self = (CHttpCli*)arg;
        if (req) {
            int32_t retcode = evhttp_request_get_response_code(req);
            struct evbuffer* input = evhttp_request_get_input_buffer(req);
            size_t len = evbuffer_get_length(input);
            char buffer[64 * 1024];
            if (len > MAX_WATERMARK_SIZE) {
                result = { 501, "too large, buffer is overflow" };
                goto ERRLABLE;
            }
            len = evbuffer_copyout(input, recv_buffer, MAX_WATERMARK_SIZE - 1);
            if (len <= 0) {
                result = { 502, "can't drain the buffer" };
                goto ERRLABLE;
            }

            self->m_evheader = evhttp_request_get_input_headers(req);
            result = { evhttp_request_get_response_code(req), std::string(recv_buffer, len) };
        } else {
            result = { 503, "request null" };
        }

    ERRLABLE:
        if (self->m_callback)
            self->m_callback(self, result.first, result.second);
        CHttpCli::Recovery(self);
    }

    std::optional<std::string_view> CHttpCli::QueryHeader(const char* key)
    {
        if (key && m_evheader) {
            return evhttp_find_header(m_evheader, key);
        }
        return std::nullopt;
    }

    bool CHttpCli::request(const char* url, std::optional<std::string_view> data, std::optional<std::map<std::string, std::string>> headers, HttpHandleCallbackFunc cb)
    {
        bool ret = true;
        const char *scheme = nullptr, *host = nullptr, *path = nullptr, *query = nullptr;
        int port = -1, r = -1;
        bool is_https = false;
        SSL* ssl = nullptr;
        struct evkeyvalq* output_headers = nullptr;
        char uri[2048];
        size_t uri_len = 0;
        struct evhttp_uri* http_uri = nullptr;
        struct evhttp_request* req = nullptr;
        struct bufferevent* bev = nullptr;

        if (CSSLContex::Instance().IsQueueOver()) {
            CDEBUG("It's over in http queue max size");
            goto error;
        }
        http_uri = evhttp_uri_parse(url);
        if (!http_uri) {
            CDEBUG("parse error %s", url);
            goto error;
        }

        scheme = evhttp_uri_get_scheme(http_uri);
        if (!scheme || (strcasecmp(scheme, "https") != 0 && strcasecmp(scheme, "http") != 0)) {
            CDEBUG("must http or https");
            goto error;
        }

        host = evhttp_uri_get_host(http_uri);
        if (host == NULL) {
            CDEBUG("url must have a host");
            goto error;
        }

        port = evhttp_uri_get_port(http_uri);
        if (port == -1) {
            port = (strcasecmp(scheme, "http") == 0) ? 80 : 443;
        }

        path = evhttp_uri_get_path(http_uri);
        if (strlen(path) == 0) {
            path = "/";
        }

        query = evhttp_uri_get_query(http_uri);
        if (query == NULL) {
            uri_len = snprintf(uri, sizeof(uri) - 1, "%s", path);
        } else {
            uri_len = snprintf(uri, sizeof(uri) - 1, "%s?%s", path, query);
        }
        uri[uri_len] = '\0';

        if (strcasecmp(scheme, "http") == 0) {
            bev = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, this, nullptr, false);
        } else if (strcasecmp(scheme, "https") == 0) {
            ssl = CSSLContex::Instance().CreateOneSSL();
            if (!ssl) {
                CDEBUG("SSL_new fail");
                goto error;
            }

            SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (void*)host);

            bev = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, this, ssl, false);
            bufferevent_openssl_set_allow_dirty_shutdown(bev, 1);
            is_https = true;
        }

        if (!bev) {
            CDEBUG("bufferevent_openssl_socket_new() failed");
            goto error;
        }

        m_evcon = CWorker::MAIN_CONTEX->CreateHttpConnection(bev, host, port);
        if (!m_evcon) {
            CDEBUG("evhttp_connection_base_bufferevent_new() failed");
            goto error;
        }
        evhttp_connection_set_retries(m_evcon, 3);
        evhttp_connection_set_timeout(m_evcon, 30);
        req = evhttp_request_new(cb, this);
        if (!req) {
            CDEBUG("evhttp_request_new() failed");
            goto error;
        }
        output_headers = evhttp_request_get_output_headers(req);
        evhttp_add_header(output_headers, "Host", host);
        evhttp_add_header(output_headers, "Connection", "keep-alive");
        //evhttp_add_header(output_headers, "Content-Type", "application/json; charset=utf-8");
        if (headers) {
            for_each(headers.value().begin(),
                headers.value().end(),
                [output_headers](const std::pair<std::string, std::string> val) { evhttp_add_header(output_headers, val.first.c_str(), val.second.c_str()); });
        }

        if (data) {
            struct evbuffer* evout = evhttp_request_get_output_buffer(req);
            evbuffer_add(evout, data.value().data(), data.value().size());
        }

        r = evhttp_make_request(m_evcon, req, data ? EVHTTP_REQ_POST : EVHTTP_REQ_GET, uri);
        if (r != 0) {
            CDEBUG("evhttp_make_request() failed");
            goto error;
        }
        goto cleanup;

    error:
        ret = false;
        if (req)
            evhttp_request_free(req);
        if (m_evcon)
            evhttp_connection_free(m_evcon);
    cleanup:
        if (http_uri)
            evhttp_uri_free(http_uri);
        if (!is_https && ssl)
            SSL_free(ssl);

        return ret;
    }
}
}