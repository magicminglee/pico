#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "global.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

namespace ghttp {
enum class HttpStatusCode : int32_t {
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
class CGlobalData {
public:
    CGlobalData() = default;
    void SetUid(const int64_t uid);
    std::optional<int64_t> GetUid() const;
    std::optional<std::string_view> GetQueryByKey(const char* key) const;
    std::optional<std::string_view> GetHeaderByKey(const char* key) const;
    std::optional<std::string_view> GetRequestPath() const;
    std::optional<std::string_view> GetRequestBody() const;
    std::optional<ghttp::HttpStatusCode> GetResponseStatus() const;
    std::optional<std::string_view> GetResponseBody() const;

public:
    std::optional<evkeyvalq*> qheaders;
    std::optional<evkeyvalq*> headers;
    std::optional<std::string_view> path;
    std::optional<std::string_view> data;
    std::optional<ghttp::HttpStatusCode> rspstatus;
    std::optional<std::string_view> rspdata;
    std::optional<int64_t> uid;
};
}

class CHTTPServer : public CObject {
    typedef std::optional<std::pair<ghttp::HttpStatusCode, std::string>> HttpCallbackType(const ghttp::CGlobalData*, evhttp_request*);
    typedef std::optional<std::pair<ghttp::HttpStatusCode, std::string>> HttpEventType(ghttp::CGlobalData*);

public:
    struct FilterData {
        ghttp::HttpMethod cmd;
        std::function<HttpCallbackType> cb;
        std::unordered_map<std::string, std::string> h;
        CHTTPServer* self;
    };

public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool Init(std::string host);
    bool Register(const std::string path, ghttp::HttpMethod cmd, std::function<HttpCallbackType> cb);
    bool Register(const std::string path, FilterData rd);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);
    std::optional<std::pair<ghttp::HttpStatusCode, std::string>> EmitEvent(std::string ename, ghttp::CGlobalData* g);
    static void Response(evhttp_request* req, const std::pair<ghttp::HttpStatusCode, std::string>& res);

private:
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

class CHTTPProxy {
    typedef void (*HttpHandleCallbackFunc)(struct evhttp_request* req, void* arg);
    using CallbackFuncType = std::function<void(const ghttp::CGlobalData*, ghttp::HttpStatusCode, std::optional<std::string_view>)>;

public:
    bool Get(std::string_view url, CallbackFuncType cb);
    bool Post(std::string_view url, CallbackFuncType cb, std::optional<std::string_view> data);

    static bool Emit(std::string_view url,
        CallbackFuncType cb,
        ghttp::HttpMethod cmd = ghttp::HttpMethod::GET,
        std::optional<std::string_view> data = std::nullopt,
        std::map<std::string, std::string> headers = {});

    bool request(const char* url, const uint32_t cmd, std::optional<std::string_view> data, std::optional<std::map<std::string, std::string>> headers, HttpHandleCallbackFunc cb);
    static void delegateCallback(struct evhttp_request* req, void* arg);
    CallbackFuncType m_callback = { nullptr };
};
NAMESPACE_FRAMEWORK_END