#include "connection.hpp"
#include "connhandler.hpp"
#include "encoder.hpp"
#include "http.hpp"
#include "utils.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

static const uint16_t SERVERCMDTYPE_SHARED = 0xFF00;
static const uint16_t SERVERCMDTYPE_FORWARD = 0xFE00;
static const uint16_t SERVERCMDTYPE_BROADCAST = 0xFD00;
static const uint16_t SERVERCMDTYPE_CLIENT = 0xFC00;

static const uint8_t BROADCASTTYPE_EXCEPTUSERS = 1;
static const uint8_t BROADCASTTYPE_SPECIFIEDUSERS = 2;

auto CConnection::SplitUri(const std::string& uri)
    -> std::tuple<std::string, std::string, std::string>
{
    auto pos = uri.find("://");
    if (pos == std::string::npos) {
        CERROR("invalid URI: no scheme %s", uri.c_str());
    }

    auto type = uri.substr(0, pos);
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);

    auto start = pos + 3;
    pos = uri.find(":", start);
    if (pos == std::string::npos) {
        // Set the default port.
        std::string port = "80";
        if (type == "wss")
            port = "443";
        pos = uri.find("/", start);
        return std::make_tuple(type, uri.substr(start, pos - start), port);
    }

    auto hostname = uri.substr(start, pos - start);

    return std::make_tuple(type, hostname, uri.substr(pos + 1));
}

std::string CConnection::GetRealSchema(const std::string& schema)
{
    auto rs = schema;
    if (auto pos = schema.find("->"); pos != std::string::npos) {
        rs = schema.substr(pos + 2, schema.size() - (pos + 2));
    }
    return rs;
}

void CConnection::SetStreamTypeBySchema(const std::string& schema)
{
    static std::map<const std::string, StreamType> STREAMMAP = {
        { "raw", StreamType::StreamType_Raw },
        { "haproxy", StreamType::StreamType_HAProxy },
        { "ws", StreamType::StreamType_WS },
        { "http", StreamType::StreamType_HTTP },
        { "tcp", StreamType::StreamType_Tcp },
        { "udp", StreamType::StreamType_Udp },
        { "unix", StreamType::StreamType_Unix },
        { "wss", StreamType::StreamType_WSS },
        { "https", StreamType::StreamType_HTTPS }
    };
    if (auto pos = schema.find("->"); pos != std::string::npos)
        m_schema = schema.substr(0, pos);
    else
        m_schema = schema;
    m_st = STREAMMAP[m_schema];
}

void CConnection::ShellProxy()
{
    auto [schema, hostname, port] = CConnection::SplitUri(m_host);
    if (auto pos = schema.find("->"); pos != std::string::npos) {
        schema = m_host.substr(pos + 2, schema.size() - (pos + 2));
        SetStreamTypeBySchema(schema);
    }
}

CConnection::CConnection()
{
    CINFO("new cconnection %p", this);
}

CConnection::~CConnection()
{
    if (m_bev) {
        bufferevent_free(m_bev);
        m_bev = nullptr;
    }
    CINFO("free cconnection %p", this);
}

bool CConnection::Destroy()
{
    return IsFlag(ConnectionFlags_Destroy);
}

void CConnection::OnConnected(const std::unordered_map<std::string, std::string>& headers)
{
    if (IsStreamType(StreamType::StreamType_WS) || IsStreamType(StreamType::StreamType_WSS)) {
        if (auto data = m_ws.OpenHandFrame(m_host, headers); data) {
            CINFO("opening handshake %p %s", this, data.value().c_str());
            sendcmd(data.value().data(), data.value().size());
        }
    }
}

bool CConnection::Connnect(const std::string host, const uint16_t port, bool ipv4_or_ipv6)
{
    CheckCondition(m_bev, false);
    return 0 == bufferevent_socket_connect_hostname(m_bev, nullptr, ipv4_or_ipv6 ? AF_INET : AF_INET6, host.c_str(), port);
}

bool CConnection::sendcmd(const void* data, const uint32_t dlen)
{
    CheckCondition(m_bev, false);
    struct evbuffer* output = bufferevent_get_output(m_bev);
    CheckCondition(output, false);
    return -1 != evbuffer_add(output, data, dlen);
}

bool CConnection::SendCmd(const void* data, const uint32_t dlen, std::optional<uint8_t> opcode, std::optional<uint32_t> maskkey)
{
    if (IsStreamType(StreamType::StreamType_WS) || IsStreamType(StreamType::StreamType_WSS)) {
        if (auto res = m_ws.Frame((char*)data, dlen, opcode.value_or(m_ws.GetFrameType()), maskkey.value_or(m_ws.GetMaskingKey())); res) {
            for (auto&& v : res.value()) {
                return sendcmd(v.data(), v.size());
            }
        }
    } else if (IsStreamType(StreamType::StreamType_HTTP) || IsStreamType(StreamType::StreamType_HTTPS)) {
        auto fr = CHttp::Frame(data, dlen);
        return sendcmd(fr.data(), fr.size());
    }
    return sendcmd(data, dlen);
}

bool CConnection::SendXGameMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data)
{
    CEncoder<XGAMEExternalHeader> enc;
    auto endata = enc.Encode(maincmd, subcmd, data.data(), data.size(), 0);
    return endata ? SendCmd(endata.value().data(), endata.value().size()) : false;
}

bool CConnection::ForwardMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data, const int64_t linkid)
{
    CEncoder<XGAMEInternalHeader> enc;
    auto endata = enc.Encode(0, maincmd, subcmd, linkid, 0, data.data(), data.size());
    return endata ? SendCmd(endata.value().data(), endata.value().size()) : false;
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
    return e != m_mgr.end() ? std::move(std::optional(e->second)) : std::nullopt;
}

std::optional<CConnection*> CConnectionMgr::GetTaskByType(const uint16_t type, const uint64_t key)
{
    std::vector<std::pair<uint64_t, CConnection*>> vec;
    std::copy_if(m_mgr.begin(), m_mgr.end(), std::back_inserter(vec), [type](const std::pair<uint64_t, CConnection*>& e) {
        return (e.second && (CUtils::ServerType(e.first) == type));
    });

    static thread_local uint32_t ROUND_ROBIN = 0;
    size_t idx = key > 0 ? key : ROUND_ROBIN++;
    return vec.empty() ? std::nullopt : std::move(std::optional(vec[idx % vec.size()].second));
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