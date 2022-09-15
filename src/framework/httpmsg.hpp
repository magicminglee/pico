
#pragma once
#include "common.hpp"
#include "connection.hpp"
#include "connhandler.hpp"

#include "llhttp.h"

#include "nghttp2/nghttp2.h"

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
    DISABLE = 0,
    DELETE = 1 << 0,
    GET = 1 << 1,
    HEAD = 1 << 2,
    POST = 1 << 3,
    PUT = 1 << 4,
    CONNECT = 1 << 5,
    OPTIONS = 1 << 6,
    TRACE = 1 << 7,
    GETPOST = GET | POST,
};
std::optional<const char*> HttpReason(HttpStatusCode code);
std::optional<const char*> HttpMethodStr(HttpMethod m);
std::optional<HttpMethod> HttpStrMethod(const std::string& m);
std::optional<std::string> GetMiMe(const std::string& m);
std::string GetHttpDate();
std::string GetHttpServer();

class CRequest {
public:
    CRequest(CHTTPClient* con);
    std::string DebugStr();
    void Reset();
    CRequest* Clone();
    CHTTPClient* Conn() { return con; }
    void SetUid(const int64_t);
    std::optional<int64_t> GetUid() const { return uid; }
    const std::string& GetQueryByKey(const std::string&) const;
    const std::string& GetHeaderByKey(const std::string&) const;
    void SetMethod(HttpMethod);
    HttpMethod GetMethod() const { return method; }
    void ClearHeader();
    void AddHeader(const std::string&, const std::string&);
    bool HasHeader(const std::string&) const;
    void SetBody(const std::string&);
    void SetBodyByView(std::string_view);
    const std::string& GetBody() const { return body; }
    void AppendBody(std::string_view);
    void SetPath(const std::string&);
    const std::string& GetPath() const { return path; }
    void SetStreamId(const int32_t);
    std::optional<int32_t> GetStreamId() const { return streamid; }
    void SetFd(const int32_t);
    std::optional<int32_t> GetFd() const { return fd; }
    void SetScheme(const std::string&);
    const std::string& GetScheme() const { return scheme; }
    void SetPort(const std::string&);
    const std::string& GetPort() const { return port; }
    void SetHost(const std::string&);
    const std::string& GetHost() const { return host; }
    void GetAllHeaders(std::map<std::string, std::string>&);
    void SetMajor(uint8_t major);
    uint8_t GetMajor() const { return major; }
    void SetMinor(uint8_t minor);
    uint8_t GetMinor() const { return minor; }
    const std::string& GetConnectionHeader();
    bool IsGRPC();

private:
    CHTTPClient* con = { nullptr };
    uint8_t major;
    uint8_t minor;
    std::map<std::string, std::string> qheaders;
    std::map<std::string, std::string> headers;
    std::string path;
    std::string body;
    std::string scheme;
    std::string host;
    std::string port;
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
    const std::string& GetQueryByKey(const std::string&) const;
    const std::string& GetHeaderByKey(const std::string&) const;
    std::optional<HttpStatusCode> GetStatus() const { return status; }
    std::optional<std::string_view> GetBody() const { return body; }
    void SetStreamId(const int32_t);
    std::optional<int32_t> GetStreamId() const { return streamid; }
    bool SendFile(const std::string&);
    bool HasHeader(const std::string&) const;
    void AppendBody(std::string_view);
    void SetMajor(uint8_t major);
    uint8_t GetMajor() const { return major; }
    void SetMinor(uint8_t minor);
    uint8_t GetMinor() const { return minor; }
    bool IsGRPC();

private:
    CHTTPClient* con = { nullptr };
    uint8_t major;
    uint8_t minor;
    std::map<std::string, std::string> qheaders;
    std::map<std::string, std::string> headers;
    std::optional<HttpStatusCode> status;
    std::optional<std::string> body;
    std::optional<int32_t> streamid;
};

class CStream {
public:
    CStream(const int32_t streamid, CHTTPClient* conn)
        : m_stream_id(streamid)
    {
        m_req.reset(CNEW ghttp::CRequest(conn));
        m_rsp.reset(CNEW ghttp::CResponse(conn));
        if (streamid > 0) {
            m_req->SetStreamId(streamid);
            m_rsp->SetStreamId(streamid);
        }
    }
    ~CStream()
    {
        m_req.release();
        m_req.release();
    }
    ghttp::CRequest* GetRequest() { return m_req.get(); }
    ghttp::CResponse* GetResponse() { return m_rsp.get(); }
    void SetRequest(ghttp::CRequest* req) { m_req.reset(req); }
    void SetResponse(ghttp::CResponse* rsp) { m_rsp.reset(rsp); }
    int32_t GetStreamId() { return m_stream_id; }

private:
    std::unique_ptr<ghttp::CRequest> m_req = { nullptr };
    std::unique_ptr<ghttp::CResponse> m_rsp = { nullptr };
    int32_t m_stream_id = { -1 };
};

class CHttpParser {
public:
    CHttpParser() = default;
    ~CHttpParser() = default;
    int32_t ParseHttpMsg(CHTTPClient* session, std::string_view data);
    llhttp_t& GetParser() { return m_parser; }
    void Reset();

private:
    int32_t parseHttp1Msg(CHTTPClient* session, std::string_view data);
    int32_t parseHttp2Msg(CHTTPClient* session, std::string_view data);
    llhttp_t m_parser;
};

using HttpReqRspCallback = std::function<bool(CRequest*, CResponse*)>;
using HttpRspCallback = std::function<bool(CResponse*)>;
}
NAMESPACE_FRAMEWORK_END