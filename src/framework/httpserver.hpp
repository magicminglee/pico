#pragma once

#include "common.hpp"
#include "connection.hpp"
#include "connhandler.hpp"
#include "contex.hpp"
#include "global.hpp"
#include "httpmsg.hpp"
#include "object.hpp"
#include "tcpserver.hpp"
#include "wsparser.hpp"

#include "nghttp2/nghttp2.h"

#include "picohttpparser/picohttpparser.h"

NAMESPACE_FRAMEWORK_BEGIN

class CHttpParser {
    enum class EParserState : uint32_t {
        EParserState_Begin = 0,
        EParserState_Head = 1,
        EParserState_Body = 2,
        EParserState_End = 3,
    };

public:
    ~CHttpParser();
    bool IsHttp2() { return nullptr != m_session; }
    int32_t ParseRequest(std::string_view data);
    int32_t ParseResponse(std::string_view data);
    ghttp::CRequest* GetRequest() { return m_req.get(); }
    void SetRequest(ghttp::CRequest* req) { m_req.reset(req); }
    ghttp::CResponse* GetResponse() { return m_rsp.get(); }
    void SetResponse(ghttp::CResponse* rsp) { m_rsp.reset(rsp); }
    void SetNgHttp2Session(nghttp2_session* session);

private:
    int32_t parseHttp1Request(std::string_view data);
    int32_t parseHttp1Response(std::string_view data);
    int32_t parseHttp2Request(std::string_view data);
    int32_t parseHttp2Response(std::string_view data);
    std::unique_ptr<ghttp::CRequest> m_req = { nullptr };
    std::unique_ptr<ghttp::CResponse> m_rsp = { nullptr };
    struct phr_chunked_decoder m_decoder = {};
    EParserState m_state = { EParserState::EParserState_Begin };
    nghttp2_session* m_session = { nullptr };
};

class CWebSocket {
public:
    using Callback = std::function<bool(CWebSocket*, std::string_view)>;
    static CWebSocket* Upgrade(ghttp::CResponse* rsp, Callback rdfunc);
    bool SendCmd(const CWSParser::WS_OPCODE opcode, std::string_view data);
    static bool Connect(const std::string& url, Callback cb);

private:
    CWebSocket(CConnection* evconn);
    ~CWebSocket();
    static void onRead(struct bufferevent* bev, void* arg);
    static void onWrite(struct bufferevent* bev, void* arg);
    static void onError(struct bufferevent* bev, short which, void* arg);

private:
    CConnection* m_evcon = { nullptr };
    CWSParser m_parser;
    Callback m_rdfunc = { nullptr };
};

class CHTTPServer : public CObject {
    typedef std::optional<std::pair<ghttp::HttpStatusCode, std::string>> HttpEventType(ghttp::CRequest*, ghttp::CResponse*);

public:
    struct FilterData {
        ghttp::HttpMethod cmd;
        ghttp::HttpReqRspCallback cb;
        std::unordered_map<std::string, std::string> header;
    };

public:
    CHTTPServer() = default;
    CHTTPServer(CHTTPServer&&);
    ~CHTTPServer();
    bool ListenAndServe(std::string host);
    void ServeWs(const std::string path, CWebSocket::Callback cb);
    bool Register(const std::string path, ghttp::HttpMethod cmd, ghttp::HttpReqRspCallback cb);
    bool Register(const std::string path, FilterData rd);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);
    bool Emit(const std::string path, ghttp::CRequest* req, ghttp::CResponse* rsp);
    std::optional<std::pair<ghttp::HttpStatusCode, std::string>> EmitEvent(std::string ename, ghttp::CRequest* req, ghttp::CResponse* rsp);
    std::optional<FilterData*> GetFilter(const std::string& path);

private:
    bool setOption(const int32_t fd);
    void destroy();

private:
    CTCPServer m_tcpserver;
    std::unordered_map<std::string, FilterData> m_callbacks;
    std::unordered_map<std::string, std::function<HttpEventType>> m_ev_callbacks;
};

class CHTTPClient {
public:
    ~CHTTPClient();
    static std::optional<CHTTPClient*> Emit(std::string_view url,
        ghttp::HttpRspCallback cb,
        ghttp::HttpMethod method = ghttp::HttpMethod::GET,
        std::optional<std::string> body = std::nullopt,
        std::map<std::string, std::string> headers = {});

public:
    bool Request();
    void Response();
    void Init(CConnection* conn, CHTTPServer* server);
    CHttpParser& GetParser() { return m_parser; }
    void SetConnection(CConnection* conn) { m_evcon = conn; }
    CConnection* GetConnection() { return m_evcon; }
    void SetHttpServer(CHTTPServer* server) { m_httpserver = server; };
    bool IsHttp2() { return nullptr != m_session; }
    const std::string& GetURL() { return m_url; }

    // HTTP2 Api
    bool CheckHttp2();
    bool InitNghttp2SessionData();
    bool SendResponse(ghttp::CRequest* req, const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, std::optional<std::string_view> data);
    static void OnWrite(CHTTPClient* hclient);
    nghttp2_session* GetNGHttp2Session() { return m_session; }

private:
    static ssize_t onDataSourceReadCallback(nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length, uint32_t* data_flags, nghttp2_data_source* source, void* user_data);
    static ssize_t sendCallback(nghttp2_session* session, const uint8_t* data, size_t length, int flags, void* user_data);
    static int onFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);
    static int onStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data);
    static int onHeaderCallback(nghttp2_session* session,
        const nghttp2_frame* frame, const uint8_t* name,
        size_t namelen, const uint8_t* value,
        size_t valuelen, uint8_t flags, void* user_data);
    static int onDataChunkRecvCallback(nghttp2_session* session,
        uint8_t flags, int32_t stream_id, const uint8_t* data, size_t len, void* user_data);
    static int onBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);
    static int onBeforeFrameSendCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data);
    int onRequestRecv(ghttp::CRequest* req);
    void removeStreamData(ghttp::CRequest* s);
    bool sendFile(ghttp::CRequest* stream_data,
        const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, const int32_t fd);
    bool sessionSend();
    bool sendConnectionHeader();
    bool submitRequest();
    bool isPassive() { return nullptr == m_callback; }
    std::map<std::string, std::string> getHttp2Header();

private:
    ghttp::HttpRspCallback m_callback = { nullptr };
    CConnection* m_evcon = { nullptr };
    nghttp2_session* m_session = { nullptr };
    CHttpParser m_parser;
    std::string m_url;
    CHTTPServer* m_httpserver = { nullptr };
};

NAMESPACE_FRAMEWORK_END