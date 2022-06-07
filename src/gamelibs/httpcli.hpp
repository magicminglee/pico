#pragma once
#include "framework/contex.hpp"

USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace httpcli {
    class CHttpCli {
        typedef void (*HttpHandleCallbackFunc)(struct evhttp_request* req, void* arg);
        using CallbackFuncType = std::function<void(CHttpCli*, const int32_t, const std::string_view)>;

    public:
        CHttpCli();
        ~CHttpCli();
        static bool Emit(std::string_view url, std::optional<std::string_view> data, CallbackFuncType cb, std::map<std::string, std::string> headers = {});
        static void Recovery(CHttpCli* self);
        std::optional<std::string_view> QueryHeader(const char* key);

    protected:
        bool request(const char* url, std::optional<std::string_view> data, std::optional<std::map<std::string, std::string>> headers, HttpHandleCallbackFunc cb);
        static void delegateCallback(struct evhttp_request* req, void* arg);
        struct evhttp_connection* m_evcon = { nullptr };
        struct evkeyvalq* m_evheader = { nullptr };
        uint64_t m_create_time = { 0 };
        CallbackFuncType m_callback = { nullptr };
    };
}
}