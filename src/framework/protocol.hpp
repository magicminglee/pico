#pragma once

#ifndef ALL_SIZE_FUNC
#define ALL_SIZE_FUNC(num, data)                        \
    unsigned int AllSize() const                        \
    {                                                   \
        return sizeof(*this) + (num) * sizeof(data[0]); \
    }
#endif

#ifndef DATA_SIZE_FUNC
#define DATA_SIZE_FUNC(num, data)       \
    unsigned int DataSize() const       \
    {                                   \
        return (num) * sizeof(data[0]); \
    }
#endif

#pragma pack(1)
struct XGAMEExternalHeader {
    unsigned int cmdid;
    unsigned int bodylen;
    unsigned int seq;
    char data[0];
    unsigned int DataSize() const
    {
        return bodylen > sizeof(*this) ? bodylen - sizeof(*this) : 0;
    }
    unsigned int AllSize() const
    {
        return bodylen;
    }
};
struct XGAMEInternalHeader {
    unsigned short cmdtype;
    unsigned int maincmd;
    unsigned int subcmd;
    unsigned long long linkid;
    unsigned long varhrlen;
    unsigned int datalen;
    char data[0];
    ALL_SIZE_FUNC(datalen, data);
    DATA_SIZE_FUNC(datalen, data);
};
const char xgame_haproxy_v2sig[12] = { '\x0D', '\x0A', '\x0D', '\x0A', '\x00', '\x0D', '\x0A', '\x51', '\x55', '\x49', '\x54', '\x0A' };
struct XGAMEHAProxyHeader {
    int family; /*AF_INET OR AF_INET6*/
    union {
        struct {
            char line[108];
        } v1;
        struct {
            unsigned char sig[12];
            unsigned char ver_cmd;
            unsigned char fam;
            unsigned short len;
            union {
                struct { /* for TCP/UDP over IPv4, len = 12 */
                    unsigned int src_addr;
                    unsigned int dst_addr;
                    unsigned short src_port;
                    unsigned short dst_port;
                } ip4;
                struct { /* for TCP/UDP over IPv6, len = 36 */
                    unsigned char src_addr[16];
                    unsigned char dst_addr[16];
                    unsigned short src_port;
                    unsigned short dst_port;
                } ip6;
                struct { /* for AF_UNIX sockets, len = 216 */
                    unsigned char src_addr[108];
                    unsigned char dst_addr[108];
                } unx;
            } addr;
        } v2;
    } hahdr;
};

struct GRPCMessageHeader {
    unsigned char flags;
    unsigned int datalen;
    char data[0];
    ALL_SIZE_FUNC(datalen, data);
    DATA_SIZE_FUNC(datalen, data);
};
#pragma pack()

static const unsigned int XGAME_PACKET_HEADER_LENGTH = sizeof(XGAMEExternalHeader);
static const unsigned int XGAME_INTERPACKET_HEADER_LENGTH = sizeof(XGAMEInternalHeader);
static const unsigned int GRPC_MESSAGE_HEADER_LENGTH = sizeof(GRPCMessageHeader);