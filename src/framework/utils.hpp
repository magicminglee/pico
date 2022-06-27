#pragma once
#include "common.hpp"
#include "object.hpp"

NAMESPACE_FRAMEWORK_BEGIN

#ifndef CheckCondition
#define CheckCondition(c, ret) \
    do {                       \
        if (!(c))              \
            return ret;        \
    } while (0)
#endif

#ifndef CheckConditionVoid
#define CheckConditionVoid(c) \
    do {                      \
        if (!(c))             \
            return;           \
    } while (0)
#endif

static const uint32_t MAX_INTERFACE_LENGTH = 128;
static const uint32_t MAX_NAMESIZE = 64;
static const uint32_t CHUNK_ALIGN_BYTES = 8;
static const uint32_t MAX_ENDPOIN_LENGTH = 64;
static const uint32_t MAX_SOCKET_RCVBUF_LENGTH = 128 * 1024;
static const uint32_t MAX_SOCKET_SNDBUF_LENGTH = 128 * 1024;
static const uint32_t MAX_HTTP_TIMEOUT = 10;
static const uint32_t MAX_HTTP_REREYS = 3;
static const uint32_t MAX_HTTP_BODY_SIZE = 1024 * 64;
static const uint32_t MAX_HTTP_HEAD_SIZE = 1024 * 64;
static uint32_t MAX_WATERMARK_SIZE = 1024 * 64;
static const uint32_t BACKLOG_SIZE = 512;

class CUtils {
public:
    static constexpr auto HashServerId(const uint16_t stype, const uint16_t sid)
    {
        return (stype << 16) | sid;
    }

    static constexpr auto ServerType(const uint64_t id)
    {
        return (id >> 16) & 0XFFFF;
    }

    static constexpr auto ServerId(const uint64_t id)
    {
        return id & 0XFFFF;
    }

    static constexpr uint64_t LinkId(const uint32_t srcid, const uint32_t dstid)
    {
        return ((uint64_t)srcid << 32) | dstid;
    }

    static constexpr uint32_t SrcId(const uint64_t linkid)
    {
        return (linkid >> 32) & 0xFFFFFFFF;
    }

    static constexpr uint32_t DstId(const uint64_t linkid)
    {
        return linkid & 0xFFFFFFFF;
    }

    static constexpr uint32_t HashCmdId(const uint16_t maincmd, const uint16_t subcmd)
    {
        return (maincmd << 16) | subcmd;
    }

    static constexpr uint16_t MainCmd(const uint32_t cmdid)
    {
        return (cmdid >> 16) & 0xFFFF;
    }
    static constexpr uint16_t SubCmd(const uint32_t cmdid)
    {
        return cmdid & 0xFFFF;
    }

    static constexpr uint32_t HashWorkerId(const uint32_t sid, const uint16_t tid)
    {
        // stype(16) | sid(8) | tid(8)
        return (sid & 0xFFFF0000) | ((sid & 0xFFFF) << 8) | tid;
    }
    static constexpr uint32_t WorkerSrvId(const uint32_t hashworkerid)
    {
        return (hashworkerid & 0xFFFF0000) | ((hashworkerid & 0xFF00) >> 8);
    }
    static uint32_t Hton32(const uint32_t v)
    {
        return htonl(v);
    }
    static uint32_t Ntoh32(const uint32_t v)
    {
        return ntohl(v);
    }
    static uint32_t Hton16(const uint16_t v)
    {
        return htons(v);
    }
    static uint32_t Ntoh16(const uint32_t v)
    {
        return ntohs(v);
    }

    static std::string Base64Encode(const std::string& in);
    static std::string Base64Decode(const std::string& in);
    static std::string Sha1(const std::string& in);
    static std::string Sha1Raw(const std::string& in);
    static std::string Sha256(const std::string& in);
    static std::string Sha256Raw(const std::string& in);
    static std::string HmacSha256(const std::string& in, const std::string& key);
    static std::string Md5(const std::string& in);

    class BitSet {
    public:
        static constexpr bool Is(unsigned char* s, unsigned long long b)
        {
            return s[b / 8] & (1 << (b % 8));
        }

        static constexpr void Set(unsigned char* s, unsigned long long b)
        {
            s[b / 8] |= (1 << (b % 8));
        }

        static constexpr void Clear(unsigned char* s, unsigned long long b)
        {
            s[b / 8] &= (~(1 << (b % 8)));
        }
    };
};

NAMESPACE_FRAMEWORK_END