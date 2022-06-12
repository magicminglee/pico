#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CHTTPServer : public CObject {
    typedef std::string HttpCallbackType(evkeyvalq*, evkeyvalq*, std::string);

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
    static std::optional<const char*> GetValueByKey(evkeyvalq* headers, const char* key);

private:
    static bufferevent* onConnected(struct event_base*, void*);
    bool setOption(const int32_t fd);
    void destroy();
    static void onBindCallback(evhttp_request* req, void* arg);

private:
    evhttp* m_http = nullptr;
    bool m_ishttps = false;
    std::unordered_map<std::string, FilterData> m_callbacks;
    // DISABLE_CLASS_COPYABLE(CHTTPServer);
};

NAMESPACE_FRAMEWORK_END