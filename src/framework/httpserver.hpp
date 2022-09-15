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

NAMESPACE_FRAMEWORK_BEGIN

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

class CHTTPServer : public CTCPServer {
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
    bool OnConnected(std::string host, const int32_t fd);
    void ServeWs(const std::string path, CWebSocket::Callback cb);
    bool Register(const std::string path, ghttp::HttpMethod cmd, ghttp::HttpReqRspCallback cb);
    bool Register(const std::string path, FilterData rd);
    bool RegEvent(std::string ename, std::function<HttpEventType> cb);
    bool Emit(const std::string& path, ghttp::CRequest* req, ghttp::CResponse* rsp);
    std::optional<std::pair<ghttp::HttpStatusCode, std::string>> EmitEvent(std::string ename, ghttp::CRequest* req, ghttp::CResponse* rsp);
    std::optional<FilterData*> GetFilter(const std::string& path);

private:
    void destroy();

private:
    std::unordered_map<std::string, FilterData> m_callbacks;
    std::unordered_map<std::string, std::function<HttpEventType>> m_ev_callbacks;
};

class CHTTPClient {
public:
    CHTTPClient();
    ~CHTTPClient();
    static std::optional<CHTTPClient*> Emit(std::string_view url,
        ghttp::HttpRspCallback cb,
        ghttp::HttpMethod method = ghttp::HttpMethod::GET,
        std::optional<std::string> body = std::nullopt,
        std::map<std::string, std::string> headers = {});

public:
    bool Request(ghttp::HttpMethod method, std::string_view body, std::map<std::string, std::string> header);
    void Response(const int32_t streamid);
    void Init(CConnection* conn, CHTTPServer* server);
    ghttp::CHttpParser& GetParser() { return m_parser; }
    void SetConnection(CConnection* conn) { m_evcon = conn; }
    CConnection* GetConnection() { return m_evcon; }
    void SetHttpServer(CHTTPServer* server) { m_httpserver = server; };
    CHTTPServer* GetHttpServer() { return m_httpserver; };
    const std::string& GetURL() { return m_url; }
    bool IsPassive() { return nullptr == m_callback; }

    // Websocket Api
    void SetWSCallback(CWebSocket::Callback cb) { m_wsfunc = cb; }
    void OnWebsocket();

    // HTTP2 Api
    bool IsHttp2() { return nullptr != m_session; }
    bool CheckHttp2();
    bool InitNghttp2SessionData();
    bool H2Response(ghttp::CRequest* req, const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, std::optional<std::string_view> data);
    static void OnWrite(CHTTPClient* hclient);
    nghttp2_session* GetNGHttp2Session() { return m_session; }
    ghttp::CStream* CreateStream(const int32_t streamid);
    bool DelStream(const int32_t streamid);
    std::optional<ghttp::CStream*> GetStream(const int32_t streamid);

private:
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
    static int onSendDataCallback(nghttp2_session* session,
        nghttp2_frame* frame,
        const uint8_t* framehd, size_t length,
        nghttp2_data_source* source,
        void* user_data);

    int onRequestRecv(ghttp::CStream* req);
    void removeStreamData(ghttp::CRequest* s);
    bool sendFile(ghttp::CRequest* stream_data,
        const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, const int32_t fd);
    bool sessionSend();
    bool sendConnectionHeader();
    bool submitRequest(ghttp::CStream* stream);

private:
    ghttp::HttpRspCallback m_callback = { nullptr };
    ghttp::HttpRspCallback m_proxy_callback = { nullptr };
    CConnection* m_evcon = { nullptr };
    nghttp2_session* m_session = { nullptr };
    ghttp::CHttpParser m_parser;
    std::string m_url;
    CHTTPServer* m_httpserver = { nullptr };
    bool m_is_useproxy = { false };
    bool m_need_connect_proxy = { false };
    bool m_proxy_connected = { false };
    CWebSocket::Callback m_wsfunc = { nullptr };
    std::map<int32_t, std::unique_ptr<ghttp::CStream>> m_streams;
    std::unique_ptr<ghttp::CStream> m_base_stream;
};

NAMESPACE_FRAMEWORK_END