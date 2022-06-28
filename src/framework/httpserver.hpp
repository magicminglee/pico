#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "global.hpp"
#include "object.hpp"
#include "wsparser.hpp"

NAMESPACE_FRAMEWORK_BEGIN

namespace ghttp {
enum class HttpStatusCode : int32_t {
    SWITCH = 101, // Switching Protocols
    OK = 200, // request completed ok
    NOCONTENT = 204, // request does not have content
    MOVEPERM = 301, // the uri moved permanently
    MOVETEMP = 302, // the uri moved temporarily
    NOTMODIFIED = 304, // page was not modified from last
    BADREQUEST = 400, // invalid http request was made
    UNAUTHORIZED = 401, // Unauthorized
    FORBIDDEN = 403, // Forbidden
    NOTFOUND = 404, // could not find content for uri
    BADMETHOD = 405, // method not allowed for this uri
    ENTITYTOOLARGE = 413, // Payload Too Large
    EXPECTATIONFAILED = 417, // we can't handle this expectation
    INTERNAL = 500, // internal error
    NOTIMPLEMENTED = 501, // not implemented
    BADGATEWAY = 502, // Bad Gateway
    SERVUNAVAIL = 503, // the server is not available
};
enum class HttpMethod : uint32_t {
    GET = 1 << 0,
    POST = 1 << 1,
    HEAD = 1 << 2,
    PUT = 1 << 3,
    DELETE = 1 << 4,
    OPTIONS = 1 << 5,
    TRACE = 1 << 6,
    CONNECT = 1 << 7,
    PATCH = 1 << 8
};
std::optional<const char*> HttpReason(HttpStatusCode code);
class CRequest {
public:
    void SetUid(const int64_t uid);
    std::optional<int64_t> GetUid() const;
    std::optional<std::string_view> GetQueryByKey(const char* key) const;
    std::optional<std::string_view> GetHeaderByKey(const char* key) const;
    std::optional<std::string_view> GetRequestPath() const;
    std::optional<std::string_view> GetRequestBody() const;

public:
    std::optional<evkeyvalq*> qheaders;
    std::optional<evkeyvalq*> headers;
    std::optional<std::string_view> path;
    std::optional<std::string_view> data;
    std::optional<int64_t> uid;
    struct evhttp_request* req = { nullptr };
};

class CResponse {
public:
    bool Response(const std::pair<ghttp::HttpStatusCode, std::string>& res) const;
    void AddHeader(const std::string& key, const std::string& val);
    evhttp_connection* Conn() { return conn; }
    std::optional<std::string_view> GetQueryByKey(const char* key) const;
    std::optional<std::string_view> GetHeaderByKey(const char* key) const;
    std::optional<ghttp::HttpStatusCode> GetResponseStatus() const;
    std::optional<std::string_view> GetResponseBody() const;

public:
    std::optional<evkeyvalq*> qheaders;
    std::optional<evkeyvalq*> headers;
    std::optional<ghttp::HttpStatusCode> rspstatus;
    std::optional<std::string_view> rspdata;
    struct evhttp_connection* conn = { nullptr };
    struct evhttp_request* req = { nullptr };
};

using HttpReqRspCallback = std::function<bool(CRequest*, CResponse*)>;
using HttpRspCallback = std::function<bool(CResponse*)>;
}

class CWebSocket {
public:
    using Callback = std::function<bool(CWebSocket*, std::string_view)>;
    static CWebSocket* Upgrade(ghttp::CResponse* rsp, Callback rdfunc);
    bool SendCmd(const CWSParser::WS_OPCODE opcode, std::string_view data);
    static bool Connect(const std::string& url, Callback cb);

private:
    CWebSocket(evhttp_connection* evconn);
    ~CWebSocket();
    static void onRead(struct bufferevent* bev, void* arg);
    static void onWrite(struct bufferevent* bev, void* arg);
    static void onError(struct bufferevent* bev, short which, void* arg);

private:
    struct evhttp_connection* m_evconn = { nullptr };
    CWSParser m_parser;
    Callback m_rdfunc = { nullptr };
};

class CHTTPServer : public CObject {
    typedef std::optional<std::pair<ghttp::HttpStatusCode, std::string>> HttpEventType(ghttp::CRequest*, ghttp::CResponse*);

public:
    struct FilterData {
        ghttp::HttpMethod cmd;
        ghttp::HttpReqRspCallback cb;
        std::unordered_map<std::string, std::string> h;
        CHTTPServer* self;
    };

public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool Init(std::string host);
    void ServeWs(const std::string path, CWebSocket::Callback cb);
    bool Register(const std::string path, ghttp::HttpMethod cmd, ghttp::HttpReqRspCallback cb);
    bool Register(const std::string path, FilterData rd);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);

private:
    std::optional<std::pair<ghttp::HttpStatusCode, std::string>> EmitEvent(std::string ename, ghttp::CRequest* req, ghttp::CResponse* rsp);
    static bufferevent* onConnected(struct event_base*, void*);
    bool setOption(const int32_t fd);
    void destroy();
    static void onBindCallback(evhttp_request* req, void* arg);

private:
    evhttp* m_http = nullptr;
    bool m_ishttps = false;
    std::unordered_map<std::string, FilterData> m_callbacks;
    std::unordered_map<std::string, std::function<HttpEventType>> m_ev_callbacks;
};

class CHTTPClient {
    typedef void (*HttpHandleCallbackFunc)(struct evhttp_request* req, void* arg);

public:
    static bool Get(std::string_view url, ghttp::HttpRspCallback cb);
    static bool Post(std::string_view url, ghttp::HttpRspCallback cb, std::optional<std::string_view> data);

    static std::optional<CHTTPClient*> Emit(std::string_view url,
        ghttp::HttpRspCallback cb,
        ghttp::HttpMethod cmd = ghttp::HttpMethod::GET,
        std::optional<std::string_view> data = std::nullopt,
        std::map<std::string, std::string> headers = {});

private:
    bool request(const char* url, const uint32_t cmd, std::optional<std::string_view> data, std::optional<std::map<std::string, std::string>> headers, HttpHandleCallbackFunc cb);
    static void delegateCallback(struct evhttp_request* req, void* arg);
    ghttp::HttpRspCallback m_callback = { nullptr };
    struct evhttp_connection* m_evconn = { nullptr };
};

NAMESPACE_FRAMEWORK_END