#pragma once

#include "common.hpp"
#include "object.hpp"
#include "protocol.hpp"

NAMESPACE_FRAMEWORK_BEGIN

template <typename T>
class CEncoder : public CObject {
    DISABLE_CLASS_COPYABLE(CEncoder);
};

template <>
class CEncoder<XGAMEExternalHeader> : public CObject {
public:
    CEncoder() = default;
    std::optional<std::string_view> Encode(const uint16_t maincmd, const uint16_t subcmd, const void* data, const uint32_t dlen, const uint32_t seq)
    {
        static thread_local char* tls_data = { nullptr };
        if (!tls_data)
            tls_data = CNEW char[MAX_WATERMARK_SIZE];
        XGAMEExternalHeader* h = (XGAMEExternalHeader*)tls_data;
        uint32_t cmdid = (maincmd << 16) | subcmd;
        h->cmdid = CUtils::Hton32(cmdid);
        h->bodylen = CUtils::Hton32(dlen + XGAME_PACKET_HEADER_LENGTH);
        h->seq = CUtils::Hton32(seq);
        CheckCondition(MAX_WATERMARK_SIZE > sizeof(*h) + dlen, std::nullopt);
        if (dlen > 0)
            memcpy(h->data, data, dlen);
        m_data = std::move(std::string_view((char*)tls_data, sizeof(*h) + dlen));
        return m_data;
    }
    void* Data()
    {
        return (void*)m_data.data();
    }

    uint32_t Length()
    {
        return m_data.size();
    }

private:
    std::string_view m_data;
    DISABLE_CLASS_COPYABLE(CEncoder);
};

template <>
class CEncoder<XGAMEInternalHeader> : public CObject {
public:
    CEncoder() = default;
    std::optional<std::string_view> Encode(const uint16_t cmdtype,
        const uint32_t maincmd,
        const uint32_t subcmd,
        const uint64_t linkid,
        const uint64_t varhrlen,
        const void* data,
        const uint32_t dlen)
    {
        static thread_local char* tls_data = { nullptr };
        if (!tls_data)
            tls_data = CNEW char[MAX_WATERMARK_SIZE];
        XGAMEInternalHeader* h = (XGAMEInternalHeader*)tls_data;
        h->cmdtype = cmdtype;
        h->maincmd = maincmd;
        h->subcmd = subcmd;
        h->linkid = linkid;
        h->varhrlen = varhrlen;
        h->datalen = dlen;
        CheckCondition(MAX_WATERMARK_SIZE > sizeof(*h) + dlen, std::nullopt);
        memcpy(h->data, data, dlen);
        m_data = std::move(std::string_view((char*)tls_data, sizeof(*h) + h->datalen));
        return m_data;
    }

    void* Data()
    {
        return (void*)m_data.data();
    }

    uint32_t Length()
    {
        return m_data.size();
    }

private:
    std::string_view m_data;
    DISABLE_CLASS_COPYABLE(CEncoder);
};

template <>
class CEncoder<GRPCMessageHeader> : public CObject {
public:
    CEncoder() = default;
    std::optional<std::string_view> Encode(const uint8_t flags,
        const void* data,
        const uint32_t dlen)
    {
        static thread_local char* tls_data = { nullptr };
        if (!tls_data)
            tls_data = CNEW char[MAX_WATERMARK_SIZE];
        GRPCMessageHeader* h = (GRPCMessageHeader*)tls_data;
        h->flags = flags;
        h->datalen = CUtils::Hton32(dlen);
        CheckCondition(MAX_WATERMARK_SIZE > sizeof(*h) + dlen, std::nullopt);
        memcpy(h->data, data, dlen);
        m_data = std::string_view((char*)tls_data, sizeof(*h) + dlen);
        return m_data;
    }

    void* Data()
    {
        return (void*)m_data.data();
    }

    uint32_t Length()
    {
        return m_data.size();
    }

private:
    std::string_view m_data;
    DISABLE_CLASS_COPYABLE(CEncoder);
};
NAMESPACE_FRAMEWORK_END