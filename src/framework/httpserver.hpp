#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

#define HTTP_UNAUTHORIZED 401

namespace ghttp {
std::optional<const char*> HttpReason(const uint32_t code);
class CGlobalData {
public:
    CGlobalData() = default;
    void SetUid(const int64_t uid);
    std::optional<int64_t> GetUid() const;
    std::optional<std::string> GetQueryByKey(const char* key) const;
    std::optional<std::string> GetHeaderByKey(const char* key) const;
    std::optional<std::string> GetRequestPath() const;
    std::optional<std::string> GetRequestBody() const;

public:
    std::optional<evkeyvalq*> qheaders;
    std::optional<evkeyvalq*> headers;
    std::optional<std::string> path;
    std::optional<std::string> data;
    std::optional<int64_t> uid;
};
}

class CHTTPServer : public CObject {
    typedef std::pair<uint32_t, std::string> HttpCallbackType(const ghttp::CGlobalData*);
    typedef std::optional<std::pair<uint32_t, std::string>> HttpEventType(ghttp::CGlobalData*);

public:
    struct FilterData {
        uint32_t cmd;
        std::function<HttpCallbackType> cb;
        std::unordered_map<std::string, std::string> h;
        CHTTPServer* self;
    };

public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool Init(std::string host);
    bool Register(const std::string path, const uint32_t cmd, HttpCallbackType cb);
    bool Register(const std::string path, FilterData rd);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);
    std::optional<std::pair<uint32_t, std::string>> EmitEvent(std::string ename, ghttp::CGlobalData* g);

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
    // DISABLE_CLASS_COPYABLE(CHTTPServer);
};

NAMESPACE_FRAMEWORK_END