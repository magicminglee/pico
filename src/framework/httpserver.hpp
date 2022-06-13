#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

#define HTTP_UNAUTHORIZED 401
class CHTTPServer : public CObject {
    typedef std::pair<uint32_t, std::string> HttpCallbackType(evkeyvalq*, evkeyvalq*, std::string);
    typedef std::pair<uint32_t, std::string> HttpEventType(evkeyvalq*, evkeyvalq*, std::string);

public:
    struct FilterData {
        uint32_t cmd;
        std::function<HttpCallbackType> cb;
        std::unordered_map<std::string, std::string> h;
    };

public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool Init(std::string host);
    bool Register(const std::string path, FilterData filter);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);
    static std::optional<const char*> GetValueByKey(evkeyvalq* headers, const char* key);
    static std::optional<const char*> HttpReason(const uint32_t code);

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