
#pragma once
#include "common.hpp"
#include "connection.hpp"
#include "connhandler.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CHTTPClient;
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
std::optional<const char*> HttpMethodStr(HttpMethod m);
std::optional<HttpMethod> HttpStrMethod(const std::string& m);
std::optional<std::string> GetMiMe(const std::string& m);

class CRequest {
public:
    std::string DebugStr();
    void Reset();
    CRequest* Clone();
    void SetUid(const int64_t);
    std::optional<int64_t> GetUid() const { return uid; }
    std::optional<std::string> GetQueryByKey(const std::string&) const;
    std::optional<std::string> GetHeaderByKey(const std::string&) const;
    void SetMethod(HttpMethod);
    HttpMethod GetMethod() const { return method; }
    void AddHeader(const std::string&, const std::string&);
    bool HasHeader(const std::string&) const;
    void SetBody(const std::string&);
    std::optional<std::string> GetBody() const { return body; }
    void AppendBody(std::string_view);
    void SetPath(const std::string&);
    std::optional<std::string> GetPath() const { return path; }
    void SetStreamId(const int32_t);
    std::optional<int32_t> GetStreamId() const { return streamid; }
    void SetFd(const int32_t);
    std::optional<int32_t> GetFd() const { return fd; }
    void SetScheme(const std::string&);
    std::optional<std::string> GetScheme() const { return scheme; }
    void SetPort(const std::string&);
    std::optional<std::string> GetPort() const { return port; }
    void SetHost(const std::string&);
    std::optional<std::string> GetHost() const { return host; }
    void GetAllHeaders(std::map<std::string, std::string>&);

private:
    std::map<std::string, std::string> qheaders;
    std::map<std::string, std::string> headers;
    std::optional<std::string> path;
    std::optional<std::string> body;
    std::optional<std::string> scheme;
    std::optional<std::string> host;
    std::optional<std::string> port;
    std::optional<int64_t> uid;
    HttpMethod method;
    std::optional<int32_t> streamid;
    std::optional<int32_t> fd;
};

class CResponse {
public:
    CResponse(CHTTPClient* con);
    std::string DebugStr();
    void Reset();
    CResponse* Clone();
    bool Response(const std::pair<HttpStatusCode, std::string>& res);
    void AddHeader(const std::string&, const std::string&);
    void SetStatus(std::optional<HttpStatusCode>);
    void SetBody(std::string_view);
    CHTTPClient* Conn() { return con; }
    std::optional<std::string> GetQueryByKey(const std::string&) const;
    std::optional<std::string> GetHeaderByKey(const std::string&) const;
    std::optional<HttpStatusCode> GetStatus() const { return status; }
    std::optional<std::string_view> GetBody() const { return body; }
    bool SendFile(const std::string&);
    bool HasHeader(const std::string&) const;
    void AppendBody(std::string_view);

private:
    CHTTPClient* con = { nullptr };
    std::map<std::string, std::string> qheaders;
    std::map<std::string, std::string> headers;
    std::optional<HttpStatusCode> status;
    std::optional<std::string_view> body;
};

using HttpReqRspCallback = std::function<bool(CRequest*, CResponse*)>;
using HttpRspCallback = std::function<bool(CResponse*)>;
}
NAMESPACE_FRAMEWORK_END