#pragma once
#include "common.hpp"
#include "object.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

#pragma pack(1)
//Data Websocket Frame EXCLUDE Extension Data
struct WSFrame {
    unsigned char fin_opcode;
    unsigned char mask_payloadlen;
    unsigned char data[0];
};
struct WSFrameMask {
    unsigned int mask_key;
    unsigned char data[0];
};
struct WSFrame16NoMask {
    unsigned short payload_len;
    unsigned char data[0];
};
struct WSFrame16Mask {
    unsigned short payload_len;
    unsigned int mask_key;
    unsigned char data[0];
};
struct WSFrame64NoMask {
    unsigned long long payload_len;
    unsigned char data[0];
};
struct WSFrame64Mask {
    unsigned long long payload_len;
    unsigned int mask_key;
    unsigned char data[0];
};
#pragma pack()

class CWebSocket {
public:
    enum WS_STATUS {
        WS_UNCONNECT = 0,
        WS_PASSIVE_OPENING = 1,
        WS_ACTIVE_OPENING = 2,
        WS_CONNECTED = 3,
        WS_CLOSED = 4
    };

    enum WS_OPCODE {
        WS_OPCODE_CONTINUE = 0x0,
        WS_OPCODE_TXTFRAME = 0x1,
        WS_OPCODE_BINFRAME = 0x2,
        WS_OPCODE_CLOSE = 0x8,
        WS_OPCODE_PING = 0x9,
        WS_OPCODE_PONG = 0xA,
    };

public:
    CWebSocket();
    ~CWebSocket();
    int Process(const char* data, const unsigned int dlen);
    int Handshake(const char* data, const unsigned int dlen, std::string& upgrade);
    std::optional<std::string> OpenHandFrame(const std::string& uri, const std::optional<std::unordered_map<std::string, std::string>> headers);
    constexpr char* Rcvbuf() { return m_rcv_buffer; }
    std::optional<std::vector<std::string_view>> Frame(const char* data, const unsigned long long dlen, const unsigned char opcode, const unsigned int mask_key);
    constexpr bool IsFin() { return m_is_fin; }
    constexpr bool IsPong() { return WS_OPCODE_PONG == m_opcode; }
    constexpr bool IsPing() { return WS_OPCODE_PING == m_opcode; }
    constexpr bool IsClose() { return WS_OPCODE_CLOSE == m_opcode; }
    constexpr unsigned long long FrameLen() { return m_frame_len; }
    void SetFrameType(const unsigned char type) { m_frame_type = type; }
    unsigned char GetFrameType() { return m_frame_type; }
    void SetMaskingKey(const unsigned int maskingkey) { m_masking_key = maskingkey; }
    const unsigned GetMaskingKey() { return m_masking_key; }
    auto ControlFrame() { return std::move(m_control_frame); }

private:
    void onControlFrame(char* payload, unsigned int payload_len, unsigned int mask_key);
    void onPingFrame(char* payload, unsigned int payload_len, unsigned int mask_key);
    void onPongFrame(char* payload, unsigned int payload_len, unsigned int mask_key);
    void onCloseFrame(char* payload, unsigned int payload_len, unsigned int mask_key);
    void onDataFrame(unsigned int mask_key);
    void onTxtDataFrame(unsigned int mask_key);
    void onBinDataFrame(unsigned int mask_key);

private:
    WS_STATUS m_status;
    //receive buffer
    char* m_rcv_buffer = nullptr;
    unsigned long long m_rcv_len;
    //this frame payload length
    unsigned long long m_frame_len;
    //record fragmented opcode
    int m_opcode;
    //websocket fin frame means a complete frame,that means be handled by upper layer.
    //when true means to submit upper layer, false discard.
    bool m_is_fin;
    std::string m_swsk;
    //frame data frame type
    unsigned char m_frame_type = { WS_OPCODE::WS_OPCODE_BINFRAME };
    //masking key
    unsigned int m_masking_key = { 0 };
    //control frame
    std::string_view m_control_frame;
};

NAMESPACE_FRAMEWORK_END