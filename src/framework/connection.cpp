#include "connection.hpp"
#include "connhandler.hpp"
#include "contex.hpp"
#include "encoder.hpp"
#include "ssl.hpp"
#include "utils.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

auto CConnection::SplitUri(const std::string& uri)
    -> std::tuple<std::string, std::string, std::string, std::string>
{
    evhttp_uri* evuri = evhttp_uri_parse_with_flags(uri.c_str(), 0);
    if (!evuri) {
        SPDLOG_ERROR("invalid URI: no scheme {}", uri);
        return std::make_tuple("", "", "", "");
    }
    std::string host = evhttp_uri_get_host(evuri);
    // std::transform(host.begin(), host.end(), host.begin(), ::tolower);
    std::string path = evhttp_uri_get_path(evuri);
    // std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    int32_t tmpport = evhttp_uri_get_port(evuri);
    std::string scheme = evhttp_uri_get_scheme(evuri);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::tolower);
    evhttp_uri_free(evuri);

    // Set the default port.
    std::string port;
    if (tmpport == -1) {
        if (scheme == "https" || scheme == "wss")
            port = "443";
        else
            port = "80";
    } else {
        port = std::to_string(tmpport);
    }

    return std::make_tuple(scheme, host, port, path);
}

void CConnection::SetStreamTypeBySchema(const std::string& schema)
{
    static std::map<const std::string, StreamType> STREAMMAP = {
        { "http", StreamType::StreamType_Http },
        { "https", StreamType::StreamType_Http },
        { "ws", StreamType::StreamType_Http },
        { "wss", StreamType::StreamType_Http },
        { "haproxy", StreamType::StreamType_HAProxy },
        { "tcp", StreamType::StreamType_Tcp },
        { "udp", StreamType::StreamType_Udp },
        { "unix", StreamType::StreamType_Unix },
    };
    m_schema = schema;
    m_st = STREAMMAP[m_schema];
}

CConnection::CConnection()
{
    SPDLOG_DEBUG("new cconnection {}", fmt::ptr(this));
}

CConnection::~CConnection()
{
    if (m_bev) {
        bufferevent_free(m_bev);
        m_bev = nullptr;
    }
    SPDLOG_DEBUG("free cconnection {}", fmt::ptr(this));
}

bool CConnection::IsClosing()
{
    return IsFlag(ConnectionFlags_Closing);
}

bool CConnection::Connnect(const std::string host, const uint16_t port, bool ipv4_or_ipv6)
{
    CheckCondition(m_bev, false);
    return 0 == bufferevent_socket_connect_hostname(m_bev, nullptr, ipv4_or_ipv6 ? AF_INET : AF_INET6, host.c_str(), port);
}

bool CConnection::SendCmd(const void* data, const uint32_t dlen)
{
    CheckCondition(m_bev, false);
    struct evbuffer* output = bufferevent_get_output(m_bev);
    CheckCondition(output, false);
    return -1 != evbuffer_add(output, data, dlen);
}

bool CConnection::SendFile(const int32_t fd)
{
    CheckCondition(m_bev, false);
    struct evbuffer* output = bufferevent_get_output(m_bev);
    CheckCondition(output, false);
    return -1 != evbuffer_add_file(output, fd, 0, -1);
}

///////////////////////////////////////////////////////////////////End CConnection////////////////////////////////////////////////////////////////

CConnectionMgr::~CConnectionMgr()
{
    for (auto&& [_, second] : m_mgr) {
        Del(second);
    }
    m_mgr.clear();
}

bool CConnectionMgr::Add(std::optional<CConnection*> t)
{
    CheckCondition(t, false);
    auto e = m_mgr.find(t.value()->Id());
    return e == m_mgr.end() ? m_mgr.insert(std::make_pair(t.value()->Id(), t.value())).second : false;
}

void CConnectionMgr::Del(std::optional<CConnection*> t)
{
    CheckConditionVoid(t);
    m_mgr.erase(t.value()->Id());
}

uint32_t CConnectionMgr::Size()
{
    return m_mgr.size();
}

void CConnectionMgr::ForEach(std::function<bool(CConnection*)> cb)
{
    for (auto&& [_, second] : m_mgr) {
        if (!cb(second))
            break;
    }
}

std::optional<CConnection*> CConnectionMgr::GetTaskById(const uint64_t id)
{
    ContainerT::iterator e = m_mgr.find(id);
    return e != m_mgr.end() ? std::optional(e->second) : std::nullopt;
}

std::optional<CConnection*> CConnectionMgr::GetTaskByType(const uint16_t type, const uint64_t key)
{
    std::vector<std::pair<uint64_t, CConnection*>> vec;
    std::copy_if(m_mgr.begin(), m_mgr.end(), std::back_inserter(vec), [type](const std::pair<uint64_t, CConnection*>& e) {
        return (e.second && (CUtils::ServerType(e.first) == type));
    });

    static thread_local uint32_t ROUND_ROBIN = 0;
    size_t idx = key > 0 ? key : ROUND_ROBIN++;
    return vec.empty() ? std::nullopt : std::optional(vec[idx % vec.size()].second);
}

std::vector<CConnection*> CConnectionMgr::GetAllTaskByType(const uint16_t type)
{
    std::vector<CConnection*> vec;
    for (auto&& [first, second] : m_mgr) {
        if (second && (CUtils::ServerType(first) == type)) {
            vec.push_back(second);
        }
    }
    return vec;
}

std::vector<uint64_t> CConnectionMgr::GetAllKeys()
{
    std::vector<uint64_t> vec;
    for (auto&& [first, _] : m_mgr) {
        vec.push_back(first);
    }
    return vec;
}

NAMESPACE_FRAMEWORK_END