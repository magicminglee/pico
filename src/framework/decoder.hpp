#pragma once

#include "common.hpp"
#include "object.hpp"
#include "protocol.hpp"
#include "stringtool.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

template <typename T>
class CDecoder : public CObject {
    DISABLE_CLASS_COPYABLE(CDecoder);
};

template <>
class CDecoder<XGAMEExternalHeader> : public CObject {
public:
    CDecoder(const char* data, const uint32_t dlen)
        : m_data(data)
        , m_dlen(dlen)
    {
    }
    std::tuple<bool, uint32_t, const char*, uint32_t> Decode()
    {
        XGAMEExternalHeader h;
        uint32_t offset = 0;
        h.cmdid = CUtils::Ntoh32(*(int32_t*)(m_data + offset));
        offset += sizeof(int32_t);
        CheckCondition(offset < m_dlen, std::make_tuple(false, 0, nullptr, 0));
        h.bodylen = CUtils::Ntoh32(*(int32_t*)(m_data + offset));
        offset += sizeof(int32_t);
        CheckCondition(offset < m_dlen, std::make_tuple(false, 0, nullptr, 0));
        h.seq = CUtils::Ntoh32(*(int32_t*)(m_data + offset));
        offset += sizeof(int32_t);
        CheckCondition(offset <= m_dlen, std::make_tuple(false, 0, nullptr, 0));
        return std::make_tuple(true, h.cmdid, m_data + XGAME_PACKET_HEADER_LENGTH, h.DataSize());
    }

private:
    const char* m_data;
    uint32_t m_dlen;
    DISABLE_CLASS_COPYABLE(CDecoder);
};

template <>
class CDecoder<XGAMEInternalHeader> : public CObject {
public:
    CDecoder(const char* data, const uint32_t dlen)
        : m_data(data)
    {
    }
    std::tuple<bool, uint32_t, const char*, uint32_t, int64_t> Decode()
    {
        XGAMEInternalHeader* h = (XGAMEInternalHeader*)m_data;
        return std::make_tuple(true, CUtils::HashCmdId(h->maincmd, h->subcmd), &h->data[0], h->DataSize(), h->linkid);
    }

private:
    const char* m_data;
    DISABLE_CLASS_COPYABLE(CDecoder);
};

template <>
class CDecoder<XGAMEHAProxyHeader> : public CObject {
public:
    CDecoder(const char* data, const uint32_t dlen)
        : m_data(data)
        , m_dlen(dlen)
    {
    }
    int32_t Decode(XGAMEHAProxyHeader& hdr)
    {
        memcpy(&hdr.hahdr, m_data, m_dlen);
        int32_t size = 0;
        if (m_dlen >= 16 && memcmp(&hdr.hahdr.v2, xgame_haproxy_v2sig, 12) == 0 && (hdr.hahdr.v2.ver_cmd & 0xF0) == 0x20) {
            size = 16 + CUtils::Hton16(hdr.hahdr.v2.len);
            if ((int32_t)m_dlen < size)
                return 0; /* truncated or too large header */

            switch (hdr.hahdr.v2.ver_cmd & 0xF) {
            case 0x01: /* PROXY command */
                switch (hdr.hahdr.v2.fam) {
                case 0x11: /* TCPv4 */
                    hdr.family = AF_INET;
                    hdr.hahdr.v2.addr.ip4.src_addr = CUtils::Ntoh32(hdr.hahdr.v2.addr.ip4.src_addr);
                    hdr.hahdr.v2.addr.ip4.src_port = CUtils::Hton16(hdr.hahdr.v2.addr.ip4.src_port);
                    hdr.hahdr.v2.addr.ip4.dst_addr = CUtils::Ntoh32(hdr.hahdr.v2.addr.ip4.dst_addr);
                    hdr.hahdr.v2.addr.ip4.dst_port = CUtils::Hton16(hdr.hahdr.v2.addr.ip4.dst_port);
                    goto done;
                case 0x21: /* TCPv6 */
                    hdr.family = AF_INET6;
                    hdr.hahdr.v2.addr.ip6.src_port = CUtils::Hton16(hdr.hahdr.v2.addr.ip6.src_port);
                    hdr.hahdr.v2.addr.ip6.dst_port = CUtils::Hton16(hdr.hahdr.v2.addr.ip6.dst_port);
                    goto done;
                }
                /* unsupported protocol, keep local connection address */
                break;
            case 0x00: /* LOCAL command */
                /* keep local connection address for LOCAL */
                break;
            default:
                break; /* not a supported command */
            }
        } else if (m_dlen >= 8 && memcmp(hdr.hahdr.v1.line, "PROXY", 5) == 0) {
            char* end = (char*)memchr(hdr.hahdr.v1.line, '\r', m_dlen - 1);
            if (!end || end[1] != '\n')
                return 0; /* partial or invalid header */
            *end = '\0'; /* terminate the string to ease parsing */
            size = end + 2 - hdr.hahdr.v1.line; /* skip header + CRLF */
            /* parse the V1 header using favorite address parsers like inet_pton.
             * return -1 upon error, or simply fall through to accept.
             */
            std::string_view line(hdr.hahdr.v1.line, size);
            auto indexes = CStringTool::Split(line, " ");
            if (6 != indexes.size())
                return size;

            if (std::string_view(indexes[1].data(), indexes[1].size()) == "TCP4") {
                hdr.family = AF_INET;
            } else if (std::string_view(indexes[1].data(), indexes[1].size()) == "TCP6") {
                hdr.family = AF_INET6;
            }
            std::from_chars(indexes[2].data(), indexes[2].data() + indexes[2].size(), hdr.hahdr.v2.addr.ip4.src_addr);
            std::from_chars(indexes[3].data(), indexes[3].data() + indexes[3].size(), hdr.hahdr.v2.addr.ip4.dst_addr);
            std::from_chars(indexes[4].data(), indexes[4].data() + indexes[4].size(), hdr.hahdr.v2.addr.ip4.src_port);
            std::from_chars(indexes[5].data(), indexes[5].data() + indexes[5].size(), hdr.hahdr.v2.addr.ip4.dst_port);
            goto done;
        } else {
            /* Wrong protocol */
            return -1;
        }

    done:
        return size;
    }

private:
    const char* m_data;
    uint32_t m_dlen;
    DISABLE_CLASS_COPYABLE(CDecoder);
};

NAMESPACE_FRAMEWORK_END