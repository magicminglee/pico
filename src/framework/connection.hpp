#pragma once

#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"
#include "utils.hpp"
#include "websocket.hpp"
#include "worker.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CConnectionHandler;
class CConnection : public CObject {
    friend class CConnectionMgr;
    friend class CConnectionHandler;
    friend class CConnectionHandler;
    friend class CInternConnectionHandler;

public:
    enum ConnectionFlags {
        ConnectionFlags_Destroy = 1,
        ConnectionFlags_HeartBeatLost = 2,
    };
    enum class StreamType {
        StreamType_Raw = 1,
        StreamType_HAProxy = 2,
        StreamType_WS = 3,
        StreamType_HTTP = 4,
        StreamType_Tcp = 5,
        StreamType_Udp = 6,
        StreamType_Unix = 7,
        StreamType_WSS = 8,
        StreamType_HTTPS = 9
    };

    CConnection();
    ~CConnection();
    virtual bool Destroy();
    virtual bool Connnect(const std::string host, const uint16_t port, bool ipv4_or_ipv6 /*ipv4 is true or ipv6*/);
    virtual bool SendCmd(const void* data, const uint32_t dlen, std::optional<uint8_t> opcode = std::nullopt, std::optional<uint32_t> maskkey = std::nullopt);
    void SetFlag(const int32_t flag) { CUtils::BitSet::Set(m_flags, flag); }
    bool IsFlag(const int32_t flag) { return CUtils::BitSet::Is(m_flags, flag); }
    void ClearFlag(const int32_t flag) { CUtils::BitSet::Clear(m_flags, flag); }
    void OnConnected(const std::unordered_map<std::string, std::string>& headers);

    void SetSockType(const StreamType type) { m_st = type; }
    constexpr bool IsValide() { return nullptr != m_bev; }
    bool IsStreamType(const StreamType type) { return m_st == type; }
    void SetStreamTypeBySchema(const std::string& schema);
    constexpr CConnectionHandler* Handler() { return m_handler; }
    const char* PeerIp() { return m_peer_ip.c_str(); }
    const uint16_t PeerPort() { return m_peer_port; }
    const std::string& Host() { return m_host; }
    const std::string& Schema() { return m_schema; }
    CWebSocket& GetWebSocket() { return m_ws; }
    static auto SplitUri(const std::string& uri) -> std::tuple<std::string, std::string, std::string>;
    static std::string GetRealSchema(const std::string& schema);
    void ShellProxy();

    bool SendXGameMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data);
    bool ForwardMsg(const uint16_t maincmd, const uint16_t subcmd, const std::string_view data, const int64_t linkid = 0);

protected:
    bool sendcmd(const void* data, const uint32_t dlen);
    bufferevent* m_bev = { nullptr };
    std::string m_host;
    std::string m_peer_ip;
    uint16_t m_peer_port = { 0 };
    StreamType m_st = { StreamType::StreamType_Raw };
    CWebSocket m_ws;
    CConnectionHandler* m_handler = { nullptr };
    uint8_t m_flags[1] = { 0 };
    std::string m_schema;
};

class CConnectionMgr : public CObject {
public:
    CConnectionMgr() = default;
    ~CConnectionMgr();

    bool Add(std::optional<CConnection*> t);
    void Del(std::optional<CConnection*> t);
    std::optional<CConnection*> GetTaskById(const uint64_t id);
    std::optional<CConnection*> GetTaskByType(const uint16_t type, const uint64_t key = 0);
    std::vector<CConnection*> GetAllTaskByType(const uint16_t type);
    std::vector<uint64_t> GetAllKeys();
    void ForEach(std::function<bool(CConnection*)> cb);
    uint32_t Size();

private:
    using ContainerT = std::unordered_map<uint64_t, CConnection*>;
    ContainerT m_mgr;
};

NAMESPACE_FRAMEWORK_END