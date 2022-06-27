#include "wsparser.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

// the base64-encoded [RFC4648] version minus
// any leading and trailing whitespace) and concatenate this with the
// Globally Unique Identifier (GUID, [RFC4122]) "258EAFA5-E914-47DA-
// 95CA-C5AB0DC85B11" in string form, which is unlikely to be used by
// network endpoints that do not understand the WebSocket Protocol.  A
// SHA-1 hash (160 bits) [FIPS.180-3], base64-encoded (see Section 4 of
//[RFC4648]), of this concatenation is then returned in the server's
// handshake.
static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
std::string CWSParser::GenSecWebSocketAccept(const std::string& swsk)
{
    std::string swsa = swsk;
    swsa += GUID;
    // openssl sha1
    swsa = CUtils::Sha1Raw(swsa);
    // openssl base64
    swsa = CUtils::Base64Encode(swsa);
    return swsa;
}

// picohttparser search header helper
static int bufis(const char* s, size_t l, const char* t)
{
    return strlen(t) == l && memcmp(s, t, l) == 0;
}

// picohttparser search header helper
static int bufisi(const char* s, size_t l, const char* t)
{
    return strlen(t) == l && std::equal(s, s + l, t, [](auto a, auto b) { return std::tolower(a) == std::tolower(b); });
}
// Base Framing Protocol
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+-+-+-+-+-------+-+-------------+-------------------------------+
//|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
//|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
//|N|V|V|V|       |S|             |   (if payload len==126/127)   |
//| |1|2|3|       |K|             |                               |
//+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
//|     Extended payload length continued, if payload len == 127  |
//+ - - - - - - - - - - - - - - - +-------------------------------+
//|                               |Masking-key, if MASK set to 1  |
//+-------------------------------+-------------------------------+
//| Masking-key (continued)       |          Payload Data         |
//+-------------------------------- - - - - - - - - - - - - - - - +
//:                     Payload Data continued ...                :
//+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
//|                     Payload Data continued ...                |
//+---------------------------------------------------------------+
static int getfin(unsigned char h)
{
    return h >> 7;
}

static void setfin(unsigned char& h)
{
    h |= 0x80;
}

static int getrsv1(unsigned char h)
{
    return (h & 0x7F) >> 6;
}

static int getrsv2(unsigned char h)
{
    return (h & 0x3F) >> 5;
}

static int getrsv3(unsigned char h)
{
    return (h & 0x1F) >> 4;
}

static int getopcode(unsigned char h)
{
    return h & 0xF;
}

static void setopcode(unsigned char& h, unsigned char co)
{
    h &= (~0x0F);
    co &= 0xF;
    h |= co;
}

static int getmask(unsigned char h)
{
    return h >> 7;
}

static int setmask(unsigned char& h)
{
    return h |= 0x80;
}

static int get_payload_len(unsigned char h)
{
    return h & 0x7F;
}

static void set_payload_len(unsigned char& h, unsigned char len)
{
    h &= (~0x7F);
    len &= 0x7F;
    h |= len;
}

static bool is_control_frame(unsigned char opcode)
{
    return (opcode & 0x8) == 0x8;
}

// Octet i of the transformed data ("transformed-octet-i") is the XOR of
// octet i of the original data ("original-octet-i") with octet at index
// i modulo 4 of the masking key ("masking-key-octet-j"):
//
//					j = i MOD 4
// transformed-octet-i = original-octet-i XOR masking-key-octet-j
static void masking(unsigned char* payload, unsigned int payload_len, unsigned int mask_key)
{
    for (unsigned int i = 0; i < payload_len; ++i) {
        int j = i % 4;
        payload[i] = payload[i] ^ ((mask_key >> (j * 8)) & 0xFF);
    }
}

static unsigned long long hton64(unsigned long long u64_host)
{
    unsigned long long u64_net;
    unsigned int u32_host_h, u32_host_l;
    u32_host_l = u64_host & 0xffffffff;
    u32_host_h = (u64_host >> 32) & 0xffffffff;

    u64_net = CUtils::Hton32(u32_host_l);
    u64_net = (u64_net << 32) | CUtils::Hton32(u32_host_h);
    return u64_net;
}

static unsigned long long ntoh64(unsigned long long u64_net)
{
    unsigned long long u64_host;
    unsigned int u32_net_h, u32_net_l;
    u32_net_l = u64_net & 0xffffffff;
    u32_net_h = (u64_net >> 32) & 0xffffffff;

    u64_host = CUtils::Ntoh32(u32_net_l);
    u64_host = (u64_host << 32) | CUtils::Ntoh32(u32_net_h);
    return u64_host;
}

CWSParser::CWSParser()
    : m_status(WS_UNCONNECT)
    , m_rcv_len(0)
{
    m_rcv_buffer = CNEWARR(char, MAX_WATERMARK_SIZE);
    (void)getrsv1;
    (void)getrsv2;
    (void)getrsv3;
    // memset(m_snd_buffer, 0, sizeof(m_snd_buffer));
    // m_snd_len = 0;
    m_rcv_len = 0;
    m_frame_len = 0;
}

CWSParser::~CWSParser()
{
    if (m_rcv_buffer) {
        CDELARR(m_rcv_buffer);
    }
}

int CWSParser::Process(const char* data, const unsigned int dlen)
{
    if (m_status != WS_CONNECTED)
        return -1; // to be close connection

    CheckCondition(sizeof(WSFrame) <= dlen, 0);
    m_is_fin = false;
    // ws frame include frame header
    unsigned int size = 0;
    WSFrame* fr = (WSFrame*)data;
    // fin field
    char fin = getfin(fr->fin_opcode);
    // opcode field
    int opcode = getopcode(fr->fin_opcode);
    // mask field
    char mask = getmask(fr->mask_payloadlen);
    // payload data EXCLUDE extension data
    unsigned char* payload_data = nullptr;
    // payload length field
    unsigned long long payload_len = get_payload_len(fr->mask_payloadlen);
    // masking key if exist
    unsigned int mask_key = 0;

    if (payload_len < 126) {
        if (mask) {
            CheckCondition(sizeof(WSFrameMask) <= dlen, 0);
            WSFrameMask* payload = (WSFrameMask*)fr->data;
            mask_key = payload->mask_key;
            payload_data = payload->data;
            size = sizeof(*fr) + sizeof(*payload) + payload_len;
        } else {
            size = sizeof(*fr) + payload_len;
            payload_data = fr->data;
        }
    } else if (payload_len == 126) {
        if (mask) {
            CheckCondition(sizeof(WSFrame16Mask) <= dlen, 0);
            WSFrame16Mask* payload = (WSFrame16Mask*)fr->data;
            mask_key = payload->mask_key;
            payload_data = payload->data;
            payload_len = CUtils::Ntoh16(payload->payload_len);
            size = sizeof(*fr) + sizeof(*payload) + payload_len;
        } else {
            CheckCondition(sizeof(WSFrame16NoMask) <= dlen, 0);
            WSFrame16NoMask* payload = (WSFrame16NoMask*)fr->data;
            payload_len = CUtils::Ntoh16(payload->payload_len);
            payload_data = payload->data;
            size = sizeof(*fr) + sizeof(*payload) + payload_len;
        }
    } else if (payload_len == 127) {
        if (mask) {
            CheckCondition(sizeof(WSFrame64Mask) <= dlen, 0);
            WSFrame64Mask* payload = (WSFrame64Mask*)fr->data;
            mask_key = payload->mask_key;
            payload_data = payload->data;
            payload_len = ntoh64(payload->payload_len);
            size = sizeof(*fr) + sizeof(*payload) + payload_len;
        } else {
            CheckCondition(sizeof(WSFrame64NoMask) <= dlen, 0);
            WSFrame64NoMask* payload = (WSFrame64NoMask*)fr->data;
            payload_data = payload->data;
            payload_len = ntoh64(payload->payload_len);
            size = sizeof(*fr) + sizeof(*payload) + payload_len;
        }
    }
    // CINFO("fin=%d,rsv1=%d,rsv2=%d,rsv3=%d,opcode=%d,mask=%d,payload_len=%d,expayload_len=%llu,dlen=%u,size=%u,mask_key=%u,rcvlen=%llu",
    //     fin,
    //     getrsv1(fr->fin_opcode),
    //     getrsv2(fr->fin_opcode),
    //     getrsv3(fr->fin_opcode),
    //     opcode,
    //     mask,
    //     get_payload_len(fr->mask_payloadlen),
    //     payload_len,
    //     dlen,
    //     size,
    //     mask_key,
    //     m_rcv_len);

    // single frame limitation
    if (size >= MAX_WATERMARK_SIZE) {
        return -2;
    }
    // not enough frame data, obtain more data from socket
    if (dlen < size)
        return 0;
    // complete frame
    if (mask) {
        masking(payload_data, payload_len, mask_key);
        // std::string str((char*)payload_data,payload_len);
        // CINFO("%s", str.c_str());
    }
    // record opcode for fragmented frame
    if (0 == m_rcv_len)
        m_opcode = opcode;

    if (fin) { // fin frame to be handled
        if (is_control_frame(m_opcode)) { // handling control frames in the middle of a fragmented message
            onControlFrame((char*)payload_data, payload_len, mask_key);
        } else {
            memcpy(m_rcv_buffer + m_rcv_len, payload_data, payload_len);
            m_rcv_len += payload_len;
            // NOTE:mask_key is the last frame masking key if it's an fragmented message
            onDataFrame(mask_key);
            m_frame_len = m_rcv_len;
            m_rcv_len = 0;
        }
    } else { // fragmented frame, keep it until fin frame to come
        if (opcode > 0)
            m_opcode = opcode;
        memcpy(m_rcv_buffer + m_rcv_len, payload_data, payload_len);
        m_rcv_len += payload_len;
    }
    // drain size data from tcp connection buffer
    return size;
}

std::optional<std::vector<std::string_view>> CWSParser::Frame(const char* data, const unsigned long long dlen, const unsigned char opcode, const unsigned int mask_key)
{
    if (m_status != WS_CONNECTED)
        return std::nullopt;

    if (opcode == WS_OPCODE_CLOSE)
        m_status = WS_CLOSED;

    static thread_local char* buffer = { nullptr };
    if (!buffer)
        buffer = CNEW char[MAX_WATERMARK_SIZE];

    unsigned int m_snd_len = 0;
    WSFrame* fr = (WSFrame*)buffer;
    memset(fr, 0, sizeof(*fr));
    setfin(fr->fin_opcode);
    setopcode(fr->fin_opcode, opcode);
    set_payload_len(fr->mask_payloadlen, dlen);
    unsigned char* payload_data = fr->data;
    unsigned int payload_len = dlen;
    if (dlen < 126) {
        if (mask_key > 0) {
            WSFrameMask* payload = (WSFrameMask*)fr->data;
            payload_data = payload->data;
            payload->mask_key = mask_key;
            memcpy(payload_data, data, dlen);
            m_snd_len = sizeof(*fr) + sizeof(*payload) + dlen;
        } else {
            set_payload_len(fr->mask_payloadlen, dlen);
            memcpy(payload_data, data, dlen);
            m_snd_len = sizeof(*fr) + dlen;
        }
    } else if (dlen <= (unsigned short)-1) {
        set_payload_len(fr->mask_payloadlen, 126);
        if (mask_key > 0) {
            WSFrame16Mask* payload = (WSFrame16Mask*)fr->data;
            payload_data = payload->data;
            memcpy(payload_data, data, dlen);
            payload->payload_len = CUtils::Hton16(dlen);
            payload->mask_key = mask_key;
            m_snd_len = sizeof(*fr) + sizeof(*payload) + dlen;
        } else {
            WSFrame16NoMask* payload = (WSFrame16NoMask*)fr->data;
            payload_data = payload->data;
            memcpy(payload_data, data, dlen);
            payload->payload_len = CUtils::Hton16(dlen);
            m_snd_len = sizeof(*fr) + sizeof(*payload) + dlen;
        }
    } else if (dlen + 10 < MAX_WATERMARK_SIZE) {
        set_payload_len(fr->mask_payloadlen, 127);
        if (mask_key > 0) {
            WSFrame64Mask* payload = (WSFrame64Mask*)fr->data;
            payload_data = payload->data;
            memcpy(payload_data, data, dlen);
            payload->payload_len = hton64(dlen);
            payload->mask_key = mask_key;
            m_snd_len = sizeof(*fr) + sizeof(*payload) + dlen;
        } else {
            WSFrame64NoMask* payload = (WSFrame64NoMask*)fr->data;
            payload_data = payload->data;
            memcpy(payload_data, data, dlen);
            payload->payload_len = hton64(dlen);
            m_snd_len = sizeof(*fr) + sizeof(*payload) + dlen;
        }
    } else {
        // CINFO("payload is too long!");
        return std::nullopt;
    }
    if (mask_key > 0) {
        setmask(fr->mask_payloadlen);
        masking(payload_data, payload_len, mask_key);
    }

    // TODO:fragmented by some bytes, otherwise client can not process, but it's no need to do that on game client.
    std::vector<std::string_view> res;
    unsigned int offset = 0, leftlen = 0;
    while (offset < m_snd_len) {
        leftlen = m_snd_len - offset;
        leftlen = leftlen > MAX_WATERMARK_SIZE ? MAX_WATERMARK_SIZE : leftlen;
        res.push_back(std::string_view(buffer + offset, leftlen));
        offset += leftlen;
    }
    return std::move(res);
}

void CWSParser::onControlFrame(char* payload, unsigned int payload_len, unsigned int mask_key)
{
    m_control_frame = std::move(std::string_view(payload, payload_len));
    switch (m_opcode) {
    case WS_OPCODE_CLOSE:
        onCloseFrame(payload, payload_len, mask_key);
        break;
    case WS_OPCODE_PING:
        onPingFrame(payload, payload_len, mask_key);
        break;
    case WS_OPCODE_PONG:
        onPongFrame(payload, payload_len, mask_key);
        break;
    default:
        break;
    }
}

void CWSParser::onDataFrame(unsigned int mask_key)
{
    switch (m_opcode) {
    case WS_OPCODE_TXTFRAME:
        // masking cleartext tcp frame as security
        // turn off it, since client failed to process oversize frame that masked by server side.
        onTxtDataFrame(0);
        break;
    case WS_OPCODE_BINFRAME:
        // no masking binary frame as performance
        onBinDataFrame(0);
        break;
    default:
        break;
    }
}

void CWSParser::onPingFrame(char* payload, unsigned int payload_len, unsigned int mask_key)
{
    m_is_fin = true;
}

void CWSParser::onPongFrame(char* payload, unsigned int payload_len, unsigned int mask_key)
{
    m_is_fin = true;
}

void CWSParser::onCloseFrame(char* payload, unsigned int payload_len, unsigned int mask_key)
{
    m_is_fin = true;
    // If there is a body, the first two bytes of
    // the body MUST be a 2-byte unsigned integer (in network byte order)
    // Following the 2-byte integer, the body MAY contain UTF-8-encoded data with value reason
    if (payload_len >= 2) {
        unsigned int offset = 0;
        unsigned short status_code = CUtils::Ntoh16(*(unsigned short*)(payload + offset));
        offset += 2;
        if (offset < payload_len) {
            std::string reason(payload + offset, payload_len - offset);
            CINFO("%s by client: status=%u,reason=%s", __FUNCTION__, status_code, reason.c_str());
        } else {
            CINFO("%s by client: status=%u", __FUNCTION__, status_code);
        }
    }
}

void CWSParser::onTxtDataFrame(unsigned int mask_key)
{
    m_is_fin = true;
}

void CWSParser::onBinDataFrame(unsigned int mask_key)
{
    m_is_fin = true;
}

NAMESPACE_FRAMEWORK_END