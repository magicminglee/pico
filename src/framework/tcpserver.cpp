#include "tcpserver.hpp"
#include "connection.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

CTCPServer::~CTCPServer()
{
    destroy();
}

CTCPServer::CTCPServer(CTCPServer&& rhs)
    : m_listenfd(rhs.m_listenfd)
    , m_ev(rhs.m_ev)
    , m_connected_callback(std::move(rhs.m_connected_callback))
{
}

void CTCPServer::destroy()
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

bool CTCPServer::Init(std::string host, std::function<void(const int32_t)> cb)
{
    CheckCondition(!host.empty(), true);
    if (-1 != m_listenfd)
        return true;

    auto [_, hostname, port] = CConnection::SplitUri(host);
    struct addrinfo hints;
    struct addrinfo *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
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

    if (::listen(m_listenfd, BACKLOG_SIZE) < 0) {
        destroy();
        return false;
    }

    m_connected_callback = std::move(cb);
    if (m_ev = CWorker::MAIN_CONTEX->Register(
            m_listenfd,
            EV_READ | EV_PERSIST,
            nullptr,
            [this]() {
                if (auto sfd = this->getAcceptFd(this->m_listenfd); sfd > 0)
                    this->m_connected_callback(sfd);
            });
        !m_ev) {
        destroy();
        return false;
    }

    return true;
}

int32_t CTCPServer::getAcceptFd(int32_t fd)
{
    if (fd != m_listenfd)
        return -1;

    struct sockaddr_storage addr;
    socklen_t addrlen = 0;
    memset(&addr, 0, sizeof(addr));
    int32_t sfd = ::accept(m_listenfd, (struct sockaddr*)&addr, &addrlen);
    if (evutil_make_socket_nonblocking(sfd) < 0)
        return -1;
    return sfd;
}

bool CTCPServer::setOption()
{
    if (evutil_make_listen_socket_reuseable(m_listenfd) < 0)
        return false;
    if (evutil_make_listen_socket_reuseable_port(m_listenfd) < 0)
        return false;
    if (evutil_make_socket_nonblocking(m_listenfd) < 0)
        return false;
#if defined(LINUX_PLATFORMOS) || defined(DARWIN_PLATFORMOS)
    int32_t flags = 1;
    int32_t error = ::setsockopt(m_listenfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));
    if (0 != error)
        return false;
#endif
    return true;
}

NAMESPACE_FRAMEWORK_END