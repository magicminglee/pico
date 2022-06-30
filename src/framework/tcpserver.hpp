#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CTCPServer : public CObject {
public:
    CTCPServer() = default;
    CTCPServer(CTCPServer&&);
    ~CTCPServer();
    bool ListenAndServe(std::string host, std::function<void(const int32_t)> cb);

private:
    bool setOption();
    void destroy();
    const int32_t getAcceptFd();

private:
    evutil_socket_t m_listenfd = { -1 };
    CEvent* m_ev = { nullptr };
    std::function<void(const int32_t)> m_connected_callback = { nullptr };
    // DISABLE_CLASS_COPYABLE(CTCPServer);
};

NAMESPACE_FRAMEWORK_END