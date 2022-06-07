#include "udpserver.hpp"
#include "connection.hpp"
#include "stringtool.hpp"
#include "worker.hpp"

NAMESPACE_FRAMEWORK_BEGIN

CUDPServer::~CUDPServer()
{
    destroy();
}

CUDPServer::CUDPServer(CUDPServer&& rhs)
    : m_listenfd(rhs.m_listenfd)
    , m_ev(rhs.m_ev)
    , m_read_callback(std::move(rhs.m_read_callback))
{
}

void CUDPServer::destroy()
{
    if (nullptr != m_ev) {
        CWorker::MAIN_CONTEX->UnRegister(m_ev);
        m_ev = nullptr;
    }
    if (m_listenfd > 0) {
        evutil_closesocket(m_listenfd);
        m_listenfd = -1;
    }
}

bool CUDPServer::Init(std::string host, std::function<void(const std::string_view)> cb)
{
    CheckCondition(!host.empty(), true);
    if (-1 != m_listenfd)
        return true;

    auto [schema, hostname, port] = CConnection::SplitUri(host);
    struct addrinfo hints;
    struct addrinfo *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
    int32_t ret = -1;
    if (hostname == "*") {
        ret = evutil_getaddrinfo(nullptr, port.c_str(), &hints, &result);
    } else {
        ret = evutil_getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    }
    if (ret != 0) {
        return false;
    }
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        m_listenfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (m_listenfd == -1)
            continue;

        if (!setOption()) {
            destroy();
            continue;
        }
        break;
    }
    if (nullptr == rp) {
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);

    if (::bind(m_listenfd, rp->ai_addr, rp->ai_addrlen) < 0) {
        destroy();
        return false;
    }

    m_read_callback = std::move(cb);
    if (m_ev = CWorker::MAIN_CONTEX->Register(
            m_listenfd,
            EV_READ | EV_PERSIST,
            nullptr,
            [this]() {
                const int32_t MAX_UDP_PACKET_SIZE = 64 * 1024;
                char buf[MAX_UDP_PACKET_SIZE];
                struct sockaddr_in fromaddr;
                uint32_t fromaddr_len = 0;
                int32_t len = ::recvfrom(this->m_listenfd, buf, MAX_UDP_PACKET_SIZE, 0, (struct sockaddr*)(&fromaddr), (socklen_t*)(&fromaddr_len));
                if (len > 0 && len < MAX_UDP_PACKET_SIZE) {
                    this->m_read_callback(std::move(std::string_view(buf, len)));
                }
            });
        !m_ev) {
        destroy();
        return false;
    }

    return true;
}

bool CUDPServer::setOption()
{
    int flags = 1;
    int error = -1;
    if (evutil_make_listen_socket_reuseable(m_listenfd) < 0)
        return false;
    if (evutil_make_socket_nonblocking(m_listenfd) < 0)
        return false;
    if (evutil_make_listen_socket_reuseable_port(m_listenfd) < 0)
        return false;
    error = ::setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, (void*)&flags, sizeof(flags));
    CheckCondition(0 == error, false);
    struct linger ling = { 0, 0 };
    error = ::setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, (void*)&ling, sizeof(ling));
    CheckCondition(0 == error, false);
#ifdef LINUX_PLATFORMOS
    error = ::setsockopt(m_listenfd, IPPROTO_UDP, TCP_NODELAY, (void*)&flags, sizeof(flags));
    CheckCondition(0 == error, false);
#endif
    error = ::setsockopt(m_listenfd, SOL_SOCKET, SO_RCVBUF, &UDP_SOCKET_RCEIVEBUF_SIZE, sizeof(UDP_SOCKET_RCEIVEBUF_SIZE));
    CheckCondition(0 == error, false);
    int32_t revbuf = 0;
    socklen_t revbuf_len = sizeof(revbuf);
    error = ::getsockopt(m_listenfd, SOL_SOCKET, SO_RCVBUF, &revbuf, &revbuf_len);
    CheckCondition(0 == error, false);
    return true;
}

NAMESPACE_FRAMEWORK_END