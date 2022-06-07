#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CUDPServer : public CObject {
public:
    CUDPServer() = default;
    CUDPServer(CUDPServer&&);
    ~CUDPServer();
    bool Init(std::string host, std::function<void(const std::string_view)> cb);

private:
    bool setOption();
    void destroy();

private:
    const int32_t UDP_SOCKET_RCEIVEBUF_SIZE = 8 * 1024 * 1024;
    evutil_socket_t m_listenfd = { -1 };
    CEvent* m_ev = { nullptr };
    std::function<void(const std::string_view)> m_read_callback = { nullptr };
    // DISABLE_CLASS_COPYABLE(CUDPServer);
};

NAMESPACE_FRAMEWORK_END