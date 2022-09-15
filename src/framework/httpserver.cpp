#include "httpserver.hpp"
#include "argument.hpp"
#include "connection.hpp"
#include "decoder.hpp"
#include "encoder.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include "fmt/core.h"

namespace fs = std::filesystem;
NAMESPACE_FRAMEWORK_BEGIN

/////////////////////////////////////////////////////////////////////////CWebSocket/////////////////////////////////////////////////////////////

namespace {
int htp_msg_begin(llhttp_t* htp)
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    auto req = session->GetStream(-1).value()->GetRequest();
    auto rsp = session->GetStream(-1).value()->GetResponse();
    if (session->IsPassive()) {
        req->Reset();
        rsp->Reset();
    } else {
        rsp->Reset();
    }
    return 0;
}

int htp_uricb(llhttp_t* htp, const char* data, size_t len)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    session->GetStream(-1).value()->GetRequest()->SetPath(std::string(data, len));
    SPDLOG_DEBUG("{} {}", __FUNCTION__, std::string(data, len));
    return 0;
}

// HTTP response status code
int htp_statuscb(llhttp_t* htp, const char* at, size_t length)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    if (htp->status_code / 100 == 1) {
        return 0;
    }

    session->GetStream(-1).value()->GetResponse()->SetStatus((ghttp::HttpStatusCode)htp->status_code);

    SPDLOG_DEBUG("{} {}", __FUNCTION__, htp->status_code);

    return 0;
}

int htp_hdr_keycb(llhttp_t* htp, const char* data, size_t len)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    if (session->IsPassive()) {
        session->GetStream(-1).value()->GetRequest()->AddHeader(std::string(data, len), std::string(""));
    } else {
        session->GetStream(-1).value()->GetResponse()->AddHeader(std::string(data, len), std::string(""));
    }
    SPDLOG_DEBUG("{} {}", __FUNCTION__, std::string(data, len));
    return 0;
}

int htp_hdr_valcb(llhttp_t* htp, const char* data, size_t len)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    if (session->IsPassive()) {
        session->GetStream(-1).value()->GetRequest()->AddHeader(std::string(""), std::string(data, len));
    } else {
        session->GetStream(-1).value()->GetResponse()->AddHeader(std::string(""), std::string(data, len));
    }
    SPDLOG_DEBUG("{} {}", __FUNCTION__, std::string(data, len));
    return 0;
}

int htp_hdrs_completecb(llhttp_t* htp)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    if (session->IsPassive()) {
        session->GetStream(-1).value()->GetRequest()->SetMethod((ghttp::HttpMethod)(1 << htp->method));
        session->GetStream(-1).value()->GetRequest()->SetMajor(htp->http_major);
        session->GetStream(-1).value()->GetRequest()->SetMinor(htp->http_minor);
    } else {
        session->GetStream(-1).value()->GetResponse()->SetMajor(htp->http_major);
        session->GetStream(-1).value()->GetResponse()->SetMinor(htp->http_minor);
        if (ghttp::HttpMethod::CONNECT == session->GetStream(-1).value()->GetRequest()->GetMethod()) {
            SPDLOG_INFO("proxy connected");
            session->Response(-1);
        }
    }
    SPDLOG_DEBUG("{}", __FUNCTION__);

    return 0;
}

int htp_bodycb(llhttp_t* htp, const char* data, size_t len)
{
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    if (session->IsPassive()) {
        session->GetStream(-1).value()->GetRequest()->AppendBody(std::string_view(data, len));
    } else {
        session->GetStream(-1).value()->GetResponse()->AppendBody(std::string_view(data, len));
    }
    SPDLOG_DEBUG("{} {}", __FUNCTION__, std::string(data, len));
    return 0;
}

int htp_msg_completecb(llhttp_t* htp)
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    CHTTPClient* session = static_cast<CHTTPClient*>(htp->data);
    auto req = session->GetStream(-1).value()->GetRequest();
    auto rsp = session->GetStream(-1).value()->GetResponse();
    if (session->IsPassive()) {
        auto filter = session->GetHttpServer()->GetFilter(req->GetPath());
        auto cmd = (uint32_t)req->GetMethod();
        if (filter) {
            if (0 == ((uint32_t)(filter.value()->cmd) & cmd)) {
                rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "" });
            } else {
                auto r = session->GetHttpServer()->EmitEvent("start", req, rsp);
                if (r) {
                    rsp->Response({ r.value().first, ghttp::HttpReason(r.value().first).value_or("") });
                } else {
                    auto res = filter.value()->cb(req, rsp);
                    if (res) {
                        session->GetHttpServer()->EmitEvent("finish", req, rsp);
                    }
                }
            }
        } else {
            rsp->SendFile(req->GetPath());
        }
    } else {
        session->Response(-1);
    }
    return 0;
}

int htp_status_complete(llhttp_t* htp)
{
    SPDLOG_DEBUG("{}", __FUNCTION__);
    return 0;
}

constexpr llhttp_settings_t htp_hooks = {
    htp_msg_begin, // llhttp_cb on_message_begin;
    htp_uricb, // llhttp_data_cb on_url;
    htp_statuscb, // llhttp_data_cb on_status;
    htp_hdr_keycb, // llhttp_data_cb on_header_field;
    htp_hdr_valcb, // llhttp_data_cb on_header_value;
    htp_hdrs_completecb, // llhttp_cb on_headers_complete;
    htp_bodycb, // llhttp_data_cb on_body;
    htp_msg_completecb, // llhttp_cb on_message_complete;
    nullptr, // llhttp_cb on_chunk_header;
    nullptr, // llhttp_cb on_chunk_complete;
    nullptr, // llhttp_cb on_url_complete;
    htp_status_complete, // llhttp_cb on_status_complete;
    nullptr, // llhttp_cb on_header_field_complete;
    nullptr // llhttp_cb on_header_value_complete;
};
} // namespace

/////////////////////////////////////////////////////////////////////////CWebSocket/////////////////////////////////////////////////////////////
CWebSocket* CWebSocket::Upgrade(ghttp::CResponse* rsp, Callback rdfunc)
{
    auto evconn = rsp->Conn()->GetConnection();
    if (!evconn)
        return nullptr;
    auto ws = CNEW CWebSocket(evconn);
    if (auto bev = evconn->GetBufEvent(); bev) {
        bufferevent_setcb(bev, onRead, onWrite, onError, ws);
    }
    ws->m_rdfunc = std::move(rdfunc);
    ws->m_parser.SetStatus(CWSParser::WS_STATUS::WS_CONNECTED);
    SPDLOG_DEBUG("CWebSocket::Upgrade");
    return ws;
}

CWebSocket::CWebSocket(CConnection* evconn)
    : m_evcon(evconn)
{
}

CWebSocket::~CWebSocket()
{
    if (m_evcon) {
        auto h = m_evcon->Handler();
        CDEL(h);
    }
}

bool CWebSocket::Connect(const std::string& url, Callback cb)
{
    auto [_, host, port, path] = CConnection::SplitUri(url);
    auto key = CWSParser::GenSecWebSocketAccept(MYARGS.ConfMap["websocketkey"]);
    return (bool)CHTTPClient::Emit(
        url,
        [cb](ghttp::CResponse* rsp) {
            rsp->Conn()->SetWSCallback(cb);
            return true;
        },
        ghttp::HttpMethod::GET,
        std::nullopt,
        {
            { "Connection", "upgrade" },
            { "Host", host },
            { "Upgrade", "websocket" },
            { "Sec-WebSocket-Version", "13" },
            { "Sec-WebSocket-Key", key },
        });
}

bool CWebSocket::SendCmd(const CWSParser::WS_OPCODE opcode, std::string_view data)
{
    if (data.empty())
        return false;
    if (auto res = m_parser.Frame(data.data(), data.length(), opcode, m_parser.GetMaskingKey()); res) {
        for (auto&& v : res.value()) {
            return m_evcon->SendCmd(v.data(), v.length());
        }
    }
    return false;
}

void CWebSocket::onRead(struct bufferevent* bev, void* arg)
{
    CWebSocket* ws = (CWebSocket*)arg;
    auto ibuf = bufferevent_get_input(bev);
    auto obuf = bufferevent_get_output(bev);
    auto dlen = evbuffer_get_length(ibuf);
    while (dlen > 0) {
        auto data = evbuffer_pullup(ibuf, -1);
        auto ret = ws->m_parser.Process((const char*)data, dlen);
        switch (ret) {
        case 0:
            break;
        case -1:
        case -2:
            CDEL(ws);
            break;
        default: {
            if (ws->m_parser.IsFin()) {
                if (ws->m_parser.IsPong()) {
                } else if (ws->m_parser.IsPing()) {
                    auto cf = ws->m_parser.ControlFrame();
                    ws->SendCmd(CWSParser::WS_OPCODE_PONG, cf);
                } else if (ws->m_parser.IsClose()) {
                    auto cf = ws->m_parser.ControlFrame();
                    ws->SendCmd(CWSParser::WS_OPCODE_CLOSE, cf);
                } else {
                    if (ws->m_rdfunc)
                        ws->m_rdfunc(ws, std::string_view(ws->m_parser.Rcvbuf(), ws->m_parser.FrameLen()));
                }
            }
            evbuffer_drain(ibuf, ret);
        } break;
        }
        dlen = evbuffer_get_length(ibuf);
    }
}

void CWebSocket::onWrite(struct bufferevent* bev, void* arg)
{
    CWebSocket* self = (CWebSocket*)arg;
    if (self->m_parser.IsClose()) {
        SPDLOG_DEBUG("{} is closed", __FUNCTION__);
        CDEL(self);
    }
}

void CWebSocket::onError(struct bufferevent* bev, short which, void* arg)
{
    CWebSocket* self = (CWebSocket*)arg;
    SPDLOG_INFO("CTX:{} {} {},{},0x{:x},{}", MYARGS.CTXID, __FUNCTION__, fmt::ptr(self->m_evcon), self->m_evcon->Id(), which, fmt::ptr(bev));
    CDEL(self);
}

void CHTTPClient::OnWebsocket()
{
    auto req = GetStream(-1).value()->GetRequest();
    auto rsp = GetStream(-1).value()->GetResponse();
    if (IsPassive()) {
        const auto& seckey = req->GetHeaderByKey("sec-websocket-key");
        if (!seckey.empty()) {
            auto&& swsk = CWSParser::GenSecWebSocketAccept(seckey);
            rsp->AddHeader("Connection", "upgrade");
            rsp->AddHeader("Sec-WebSocket-Accept", swsk);
            rsp->AddHeader("Upgrade", "websocket");
            rsp->Response({ ghttp::HttpStatusCode::SWITCH, "" });
            CWebSocket::Upgrade(rsp, m_wsfunc);
            return;
        }
    } else {
        const auto& seckey = rsp->GetHeaderByKey("sec-websocket-accept");
        if (!seckey.empty()) {
            const auto&& key = CWSParser::GenSecWebSocketAccept(MYARGS.ConfMap["websocketkey"]);
            if (seckey == CWSParser::GenSecWebSocketAccept(key)) {
                CWebSocket::Upgrade(rsp, m_wsfunc);
                return;
            }
        }
    }
    rsp->Response({ ghttp::HttpStatusCode::FORBIDDEN, "" });
}
///////////////////////////////////////////////////////////////CHTTPServer////////////////////////////////////////////////////////
CHTTPServer::~CHTTPServer()
{
    destroy();
}

CHTTPServer::CHTTPServer(CHTTPServer&& rhs)
{
}

void CHTTPServer::destroy()
{
}

bool CHTTPServer::OnConnected(std::string host, const int32_t fd)
{
    CheckCondition(!host.empty(), true);

    CHTTPClient* hclient = CNEW CHTTPClient();
    CConnectionHandler* h = CNEW CConnectionHandler();
    h->Register(
        [hclient](std::string_view data) {
            auto result = hclient->GetParser().ParseHttpMsg(hclient, data);
            if (result < 0) {
                SPDLOG_ERROR("CHTTPServer::OnConnected ParseRequest {} {}", hclient->GetConnection()->GetPeerIp(), data);
            }
            return result;
        },
        [hclient]() {
            CHTTPClient::OnWrite(hclient);
        },
        [hclient](const EnumConnEventType e) {
            if (e == EnumConnEventType::EnumConnEventType_Connected) {
            } else if (e == EnumConnEventType::EnumConnEventType_Closed) {
                delete hclient;
            }
        });
    if (h->Init(fd, host)) {
        hclient->Init(h->Connection(), this);
        struct timeval rwtv = { MYARGS.HttpTimeout.value_or(60), 0 };
        bufferevent_set_timeouts(hclient->GetConnection()->GetBufEvent(), &rwtv, &rwtv);
        return true;
    }
    CDEL(hclient);
    CDEL(h);
    return false;
}

void CHTTPServer::ServeWs(const std::string path, CWebSocket::Callback cb)
{
    Register(
        path,
        ghttp::HttpMethod::GET,
        [cb](const ghttp::CRequest* req, ghttp::CResponse* rsp) {
            const_cast<ghttp::CRequest*>(req)->Conn()->SetWSCallback(cb);
            return true;
        });
}

bool CHTTPServer::Register(const std::string path, FilterData filter)
{
    m_callbacks[path] = filter;
    return true;
}

bool CHTTPServer::Register(const std::string path, ghttp::HttpMethod cmd, ghttp::HttpReqRspCallback cb)
{
    FilterData filter = { .cmd = cmd, .cb = cb };
    return Register(path, filter);
}

bool CHTTPServer::Emit(const std::string& path, ghttp::CRequest* req, ghttp::CResponse* rsp)
{
    if (auto it = m_callbacks.find(path); it != m_callbacks.end()) {
        return it->second.cb(req, rsp);
    }
    return false;
}

bool CHTTPServer::RegEvent(std::string ename, std::function<HttpEventType> cb)
{
    m_ev_callbacks[ename] = std::move(cb);
    return true;
}

std::optional<std::pair<ghttp::HttpStatusCode, std::string>> CHTTPServer::EmitEvent(std::string ename, ghttp::CRequest* req, ghttp::CResponse* rsp)
{
    if (auto it = m_ev_callbacks.find(ename); it != std::end(m_ev_callbacks)) {
        return (it->second)(req, rsp);
    }
    return std::nullopt;
}

std::optional<CHTTPServer::FilterData*> CHTTPServer::GetFilter(const std::string& path)
{
    auto it = m_callbacks.find(path);
    if (it != std::end(m_callbacks)) {
        return std::optional<FilterData*>(&it->second);
    }
    return std::nullopt;
}

///////////////////////////////////////////////////////////////CHTTPClient/////////////////////////////////////////////////////////////////////////
CHTTPClient::CHTTPClient()
{
    llhttp_init(&m_parser.GetParser(), HTTP_BOTH, &htp_hooks);
    m_parser.GetParser().data = this;
}

CHTTPClient::~CHTTPClient()
{
    if (m_session)
        nghttp2_session_del(m_session);
}

void CHTTPClient::Init(CConnection* conn, CHTTPServer* server)
{
    SetConnection(conn);
    SetHttpServer(server);
    m_base_stream.reset(CNEW ghttp::CStream(-1, this));
}

bool CHTTPClient::Request(ghttp::HttpMethod method, std::string_view body, std::map<std::string, std::string> header)
{
    ghttp::CStream h2stream(-1, this);
    ghttp::CStream* stream = &h2stream;
    if (!IsHttp2()) {
        stream = m_base_stream.get();
    }
    auto [scheme, hostname, port, path] = CConnection::SplitUri(GetURL());
    if (m_is_useproxy && !m_proxy_connected) {
        stream->GetRequest()->SetMethod(ghttp::HttpMethod::CONNECT);
        stream->GetRequest()->SetScheme(scheme);
        stream->GetRequest()->SetHost(fmt::format("{}:{}", hostname, port));
        stream->GetRequest()->SetPort(port);
        stream->GetRequest()->SetPath(fmt::format("{}:{}", hostname, port));
        stream->GetRequest()->AddHeader("Proxy-Connection", "keep-alive");
        stream->GetRequest()->AddHeader("User-Agent", "PICO-v1.0");
        stream->GetRequest()->AddHeader("Host", fmt::format("{}:{}", hostname, port));
    } else {
        for (auto& [k, v] : header) {
            stream->GetRequest()->AddHeader(k, v);
        }
        stream->GetRequest()->AddHeader("Host", fmt::format("{}:{}", hostname, port));
        stream->GetRequest()->SetMethod(method);
        stream->GetRequest()->SetScheme(scheme);
        stream->GetRequest()->SetHost(hostname);
        stream->GetRequest()->SetPort(port);
        if (scheme == "http" && m_is_useproxy) {
            stream->GetRequest()->SetPath(GetURL());
        } else {
            stream->GetRequest()->SetPath(path);
        }
        if (stream->GetRequest()->IsGRPC()) {
            CEncoder<GRPCMessageHeader> enc;
            if (!body.empty()) {
                auto res = enc.Encode(0, body.data(), body.length());
                if (res)
                    stream->GetRequest()->AppendBody(res.value());
            }
        } else {
            if (!body.empty())
                stream->GetRequest()->SetBodyByView(body);
        }
    }

    if (IsHttp2()) {
        return submitRequest(stream);
    } else {
        std::map<std::string, std::string> headers;
        stream->GetRequest()->GetAllHeaders(headers);
        std::string httprsp = fmt::format("{} {} HTTP/1.1\r\n", ghttp::HttpMethodStr(stream->GetRequest()->GetMethod()).value_or(""), stream->GetRequest()->GetPath());
        for (auto& [k, v] : headers) {
            if (k == "sec-websocket-key") {
                // compatible with case sensitive
                httprsp += fmt::format("{}: {}\r\n", "Sec-WebSocket-Key", v);
            } else {
                httprsp += fmt::format("{}: {}\r\n", k, v);
            }
        }
        httprsp += fmt::format("Content-Length: {}\r\n", stream->GetRequest()->GetBody().size());
        httprsp += fmt::format("\r\n");
        if (!stream->GetRequest()->GetBody().empty())
            httprsp.append(stream->GetRequest()->GetBody());
        SPDLOG_DEBUG("{} {}", __FUNCTION__, httprsp);
        GetConnection()->SendCmd(httprsp.data(), httprsp.size());
    }
    return true;
}

std::optional<CHTTPClient*> CHTTPClient::Emit(std::string_view url,
    ghttp::HttpRspCallback cb,
    ghttp::HttpMethod method,
    std::optional<std::string> body,
    std::map<std::string, std::string> header)
{
    CConnectionHandler* h = CNEW CConnectionHandler();
    CHTTPClient* hclient = CNEW CHTTPClient();
    hclient->m_callback = cb;
    hclient->m_url = std::string(url.data(), url.size());
    std::string conurl = hclient->GetURL();

    if (auto proxy = MYARGS.ConfMap.find("httpproxy"); proxy != std::end(MYARGS.ConfMap)) {
        conurl = proxy->second;
        hclient->m_is_useproxy = true;
        auto [scheme, hostname, port, path] = CConnection::SplitUri(hclient->GetURL());
        auto ns = hostname;
        auto sch = scheme;
        hclient->m_proxy_callback = [hclient, h, ns, sch, header, body, method](ghttp::CResponse* response) {
            if (response->GetStatus() == ghttp::HttpStatusCode::OK) {
                // proxy connected
                SPDLOG_DEBUG("Proxy Connected {} {}", hclient->m_need_connect_proxy, hclient->m_proxy_connected);
                hclient->m_proxy_connected = true;
                if (auto it = header.find("content-type"); hclient->CheckHttp2() || (it != std::end(header) && 0 == it->second.find("application/grpc"))) {
                    if (!hclient->GetNGHttp2Session()) {
                        hclient->InitNghttp2SessionData();
                    }
                }
                if (sch == "https" || sch == "wss") {
                    h->InitProxy(ns);
                } else {
                    // non tls sending request directly
                    hclient->Request(method, body.value_or(""), header);
                }
            }
            return true;
        };
    }
    h->Register(
        [hclient](std::string_view data) {
            auto result = hclient->GetParser().ParseHttpMsg(hclient, data);
            if (result < 0) {
                SPDLOG_ERROR("CHTTPClient::Emit ParseResponse {} {}", hclient->GetConnection()->GetPeerIp(), data);
            }
            return result;
        },
        [hclient]() {
            CHTTPClient::OnWrite(hclient);
        },
        [hclient, method, body, header, h](const EnumConnEventType e) {
            if (e == EnumConnEventType::EnumConnEventType_Connected) {
                if (hclient->m_proxy_connected) {
                    hclient->GetParser().Reset();
                    h->EnableProxy();
                }
                // check h2 if tls connected.
                if (auto it = header.find("content-type"); hclient->CheckHttp2() || (!hclient->m_is_useproxy && it != std::end(header) && 0 == it->second.find("application/grpc"))) {
                    if (!hclient->GetNGHttp2Session()) {
                        hclient->InitNghttp2SessionData();
                    }
                }
                SPDLOG_DEBUG("Client Connected {}", hclient->m_proxy_connected);
                hclient->Request(method, body.value_or(""), header);
            } else if (e == EnumConnEventType::EnumConnEventType_Closed) {
                delete hclient;
            }
        });
    if (h->Connect(conurl)) {
        hclient->Init(h->Connection(), nullptr);
        struct timeval rwtv = { MYARGS.HttpTimeout.value_or(60), 0 };
        bufferevent_set_timeouts(hclient->GetConnection()->GetBufEvent(), &rwtv, &rwtv);
    }

    return hclient;
}

void CHTTPClient::Response(const int32_t streamid)
{
    if (m_proxy_callback) {
        GetStream(streamid).value()->GetRequest()->Reset();
        m_proxy_callback(GetStream(-1).value()->GetResponse());
        m_proxy_callback = nullptr;
    } else if (m_callback) {
        m_callback(GetStream(streamid).value()->GetResponse());
        m_callback = nullptr;
    }
}

void CHTTPClient::OnWrite(CHTTPClient* hclient)
{
    if (hclient->m_session) {
        if (hclient->IsPassive()) {
            if (nghttp2_session_want_read(hclient->m_session) == 0 && nghttp2_session_want_write(hclient->m_session) == 0) {
                CDEL(hclient);
                return;
            }
            if (!hclient->sessionSend()) {
                CDEL(hclient);
                return;
            }
        } else {
            // if (nghttp2_session_want_read(hclient->m_session) == 0
            //     && nghttp2_session_want_write(hclient->m_session) == 0
            //     && evbuffer_get_length(bufferevent_get_output(hclient->GetConnection()->GetBufEvent())) == 0) {
            //     CDEL(hclient);
            // }
        }
    }
}

bool CHTTPClient::CheckHttp2()
{
    auto isserver = m_evcon->IsPassive();
    auto bev = m_evcon->GetBufEvent();
    const unsigned char* alpn = nullptr;
    unsigned int alpnlen = 0;
    auto ssl = bufferevent_openssl_get_ssl(bev);
    if (!ssl) {
        return false;
    }
    if (!alpn) {
        SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);
    }
    if (!alpn || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) {
        return false;
    }
    SPDLOG_DEBUG("{} {} http2 is enabled and h2 is negotiated success {}", MYARGS.CTXID, __FUNCTION__, isserver);

    return true;
}

ssize_t CHTTPClient::sendCallback(nghttp2_session* session, const uint8_t* data, size_t length, int flags, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    struct bufferevent* bev = session_data->m_evcon->GetBufEvent();
    (void)session;
    (void)flags;

    /* Avoid excessive buffering in server side. */
    if (evbuffer_get_length(bufferevent_get_output(bev)) >= MAX_WATERMARK_SIZE) {
        return NGHTTP2_ERR_WOULDBLOCK;
    }
    session_data->GetConnection()->SendCmd(data, length);
    return (ssize_t)length;
}

int CHTTPClient::onRequestRecv(ghttp::CStream* stream)
{
    auto req = stream->GetRequest();
    auto rsp = stream->GetResponse();
    if (!req->GetPath().empty()) {
        SPDLOG_DEBUG("{} {} {} {}", MYARGS.CTXID, __FUNCTION__, req->GetStreamId().value(), req->GetPath());
        if (!req->IsGRPC()) {
            auto r = m_httpserver->EmitEvent("start", req, rsp);
            if (r) {
                H2Response(req, r.value().first, {}, r.value().second);
                return 0;
            }
        } else {
            CDecoder<GRPCMessageHeader> dec(req->GetBody().data(), req->GetBody().length());
            auto [ret, flag, msg, msglen] = dec.Decode();
            req->SetBodyByView(std::string_view((char*)msg, msglen));
        }

        if (!m_httpserver->Emit(req->GetPath(), req, rsp)) {
            fs::path f = MYARGS.WebRootDir.value_or(fs::current_path()) + req->GetPath();
            if (!fs::exists(f)) {
                H2Response(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
                return 0;
            }
            auto mime = ghttp::GetMiMe(f.extension());
            if (!mime) {
                H2Response(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
                return 0;
            }
            auto hfd = fopen(f.c_str(), "r");
            if (hfd) {
                std::unordered_map<std::string, std::string> header;
                header["content-type"] = mime.value();
                if (MYARGS.IsAllowOrigin && MYARGS.IsAllowOrigin.value())
                    header["access-control-allow-origin"] = "*";
                sendFile(req, ghttp::HttpStatusCode::OK, header, fileno(hfd));
                return 0;
            }
            H2Response(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
            return 0;
        }
        m_httpserver->EmitEvent("finish", req, rsp);
    } else {
        req->Reset();
    }
    return 0;
}

int CHTTPClient::onFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    if (session_data->m_httpserver) {
        switch (frame->hd.type) {
        case NGHTTP2_DATA:
        case NGHTTP2_HEADERS:
            /* Check that the client request has finished */
            if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
                ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
                /* For DATA and HEADERS frame, this callback may be called after
                   onstreamclosecallback. Check that stream still alive. */
                if (!stream_data) {
                    return 0;
                }
                return session_data->onRequestRecv(stream_data);
            }
            break;
        default:
            break;
        }
    } else {
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS: {
            ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && stream_data->GetStreamId() == frame->hd.stream_id) {
                SPDLOG_DEBUG("All headers received");
            }
        } break;
        default:
            break;
        }
    }
    return 0;
}

int CHTTPClient::onDataChunkRecvCallback(nghttp2_session* session,
    uint8_t flags, int32_t stream_id, const uint8_t* data, size_t len, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    if (session_data->m_httpserver) {
        ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (!stream_data) {
            return 0;
        }
        stream_data->GetRequest()->AppendBody(std::string_view((char*)data, len));
    } else {
        ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (stream_data->GetRequest()->GetStreamId().value_or(-1) == stream_id) {
            stream_data->GetResponse()->AppendBody(std::string_view((const char*)data, len));
            if (stream_data->GetResponse()->IsGRPC()) {
                CDecoder<GRPCMessageHeader> dec(stream_data->GetResponse()->GetBody().value().data(),
                    stream_data->GetResponse()->GetBody().value().length());
                auto [ret, flag, msg, msglen] = dec.Decode();
                SPDLOG_DEBUG("All data received {} {} {} {}", fmt::ptr(stream_data->GetRequest()), flag, len, msglen);
                if (msglen + GRPC_MESSAGE_HEADER_LENGTH > stream_data->GetResponse()->GetBody().value_or("").length())
                    return 0;
                stream_data->GetResponse()->SetBody(std::string_view((const char*)msg, msglen));
            } else {
                auto content_length = CStringTool::ToInteger<size_t>(stream_data->GetResponse()->GetHeaderByKey("content-length"));
                if (content_length > stream_data->GetResponse()->GetBody().value_or("").length())
                    return 0;
            }
            SPDLOG_DEBUG("All data received {} {} {}", flags, stream_id, stream_data->GetResponse()->GetBody().value_or("").length());
            session_data->Response(stream_id);
        }
    }
    return 0;
}

int CHTTPClient::onStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    (void)error_code;

    if (session_data->m_httpserver) {
        ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (!stream_data) {
            return 0;
        }
        session_data->DelStream(stream_id);
        SPDLOG_DEBUG("{} ServerStream {} closed with error_code={} errstr {}", MYARGS.CTXID.c_str(), stream_id, error_code, nghttp2_strerror(error_code));
        return 0;
    } else {
        ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (stream_data->GetRequest()->GetStreamId().value_or(-1) == stream_id) {
            // SPDLOG_DEBUG("{} ClientStream {} closed with error_code={} errstr {}", MYARGS.CTXID, stream_id, error_code, nghttp2_strerror(error_code));
            //  auto rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
            //  if (rv != 0) {
            //      return NGHTTP2_ERR_CALLBACK_FAILURE;
            //  }
        }
        session_data->DelStream(stream_id);
        return 0;
    }
}

/* nghttp2_on_header_callback: Called when nghttp2 library emits
   single header name/value pair. */
int CHTTPClient::onHeaderCallback(nghttp2_session* session,
    const nghttp2_frame* frame, const uint8_t* name,
    size_t namelen, const uint8_t* value,
    size_t valuelen, uint8_t flags, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    (void)flags;

    if (session_data->m_httpserver) {
        const char PATH[] = ":path";
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
                break;
            }
            ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (!stream_data) {
                break;
            }
            stream_data->GetRequest()->AddHeader(std::string((char*)name, namelen), std::string((char*)value, valuelen));
            SPDLOG_DEBUG("HTTP2 Server HEADER {} {}", name, value);
            if (namelen == sizeof(PATH) - 1 && memcmp(PATH, name, namelen) == 0) {
                size_t j;
                for (j = 0; j < valuelen && value[j] != '?'; ++j)
                    ;
                stream_data->GetRequest()->SetPath(std::string((char*)value, j));
            }
            break;
        }
    } else {
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && stream_data->GetRequest()->GetStreamId().value_or(-1) == frame->hd.stream_id) {
                stream_data->GetResponse()->AddHeader(std::string((char*)name, namelen), std::string((char*)value, valuelen));
                SPDLOG_DEBUG("HTTP2 Client HEADER {} {}", name, value);
                if (stream_data->GetResponse()->HasHeader(":status"))
                    stream_data->GetResponse()->SetStatus((ghttp::HttpStatusCode)CStringTool::ToInteger<int32_t>(stream_data->GetResponse()->GetHeaderByKey(":status")));
                break;
            }
        }
    }
    return 0;
}

int CHTTPClient::onBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;

    if (session_data->m_httpserver) {
        if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
            return 0;
        }
        nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, session_data->CreateStream(frame->hd.stream_id));
    } else {
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS: {
            ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && stream_data->GetRequest()->GetStreamId().value_or(-1) == frame->hd.stream_id) {
                return 0;
            }
        } break;
        default:
            break;
        }
    }
    return 0;
}

int CHTTPClient::onBeforeFrameSendCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;

    if (session_data->m_httpserver) {
        SPDLOG_DEBUG("{} Server streamid {} type {}", __FUNCTION__, frame->hd.stream_id, frame->hd.type);
    } else {
        SPDLOG_DEBUG("{} Client {}", __FUNCTION__, frame->hd.type);
    }
    return 0;
}

bool CHTTPClient::sendConnectionHeader()
{
    std::vector<nghttp2_settings_entry> iv = { { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100 } };

    int32_t rv = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
    if (rv != 0) {
        SPDLOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
        return false;
    }
    SPDLOG_DEBUG("sendConnectionHeader: {}", nghttp2_strerror(rv));
    return true;
}

bool CHTTPClient::sessionSend()
{
    int32_t rv = nghttp2_session_send(m_session);
    if (rv != 0) {
        SPDLOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
        return false;
    }
    SPDLOG_DEBUG("sessionSend: {}", nghttp2_strerror(rv));
    return true;
}

int CHTTPClient::onSendDataCallback(nghttp2_session* session,
    nghttp2_frame* frame,
    const uint8_t* framehd, size_t length,
    nghttp2_data_source* source,
    void* user_data)
{
    std::string_view* data = (std::string_view*)source->ptr;

    CHTTPClient* session_data = (CHTTPClient*)user_data;
    ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (!session_data->GetConnection()->SendCmd(framehd, 9))
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    if (data && !session_data->GetConnection()->SendCmd(data->data(), data->length()))
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;

    delete data;
    stream_data->GetRequest()->Reset();

    return 0;
}

static ssize_t nocopy_read_callback(nghttp2_session* session,
    int32_t stream_id,
    uint8_t* buf, size_t length,
    uint32_t* data_flags,
    nghttp2_data_source* source,
    void* user_data)
{
    (void)session;
    (void)stream_id;
    (void)user_data;
    std::string_view* data = (std::string_view*)source->ptr;
    ssize_t r = data->length();

    CHTTPClient* session_data = (CHTTPClient*)user_data;
    ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);
    SPDLOG_DEBUG("nocopy_read_callback: {}", fmt::ptr(stream_data));

    *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    *data_flags |= NGHTTP2_DATA_FLAG_NO_COPY;

    if (stream_data->GetRequest()->IsGRPC()) {
        *data_flags |= NGHTTP2_DATA_FLAG_NO_END_STREAM;
        static const std::string GRPC_STATUS = "grpc-status";
        std::vector<nghttp2_nv> hdrs;
        std::string grpc_status_value = std::to_string(0);
        hdrs.push_back({ (uint8_t*)GRPC_STATUS.c_str(), (uint8_t*)grpc_status_value.c_str(), GRPC_STATUS.size(), grpc_status_value.size(), NGHTTP2_NV_FLAG_NONE });
        auto rv = nghttp2_submit_trailer(session, stream_id, &hdrs[0], hdrs.size());
        if (rv != 0) {
            SPDLOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
            stream_data->GetRequest()->Reset();
            return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
        }
    }

    return r;
}

static ssize_t file_read_callback(nghttp2_session* session, int32_t stream_id,
    uint8_t* buf, size_t length,
    uint32_t* data_flags,
    nghttp2_data_source* source,
    void* user_data)
{
    int fd = source->fd;
    ssize_t r;
    (void)session;
    (void)stream_id;
    (void)user_data;

    ghttp::CStream* stream_data = (ghttp::CStream*)nghttp2_session_get_stream_user_data(session, stream_id);

    while ((r = read(fd, buf, length)) == -1 && errno == EINTR)
        ;
    if (r == -1) {
        stream_data->GetRequest()->Reset();
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }
    if (r == 0) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }

    stream_data->GetRequest()->Reset();

    return r;
}

bool CHTTPClient::H2Response(ghttp::CRequest* stream_data,
    const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, std::optional<std::string_view> body)
{
    static const std::string STATUS = ":status";
    if (stream_data->IsGRPC()) {
        header["grpc-accept-encoding"] = "identity";
        CEncoder<GRPCMessageHeader> enc;
        if (body)
            body = enc.Encode(0, body.value().data(), body.value().length());
    }

    nghttp2_data_provider data_prd;
    data_prd.source.ptr = body ? CNEW std::string_view(body.value()) : nullptr;
    data_prd.read_callback = nocopy_read_callback;

    header["date"] = ghttp::GetHttpDate();
    header["server"] = ghttp::GetHttpServer();
    header["content-type"] = stream_data->GetHeaderByKey("content-type").empty() ? "application/json; charset=utf-8" : stream_data->GetHeaderByKey("content-type");
    std::vector<nghttp2_nv> hdrs;
    std::string status_value = std::to_string((long)status);
    hdrs.push_back({ (uint8_t*)STATUS.c_str(), (uint8_t*)status_value.c_str(), STATUS.size(), status_value.size(), NGHTTP2_NV_FLAG_NONE });
    for (auto& [k, v] : header) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
    }
    auto rv = nghttp2_submit_response(m_session, stream_data->GetStreamId().value(), hdrs.data(), hdrs.size(), body ? &data_prd : nullptr);
    if (rv != 0) {
        SPDLOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
        stream_data->Reset();
        return false;
    }
    return true;
}

bool CHTTPClient::sendFile(ghttp::CRequest* stream_data,
    const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, const int32_t fd)
{
    static const std::string STATUSH = ":status";
    stream_data->SetFd(fd);
    nghttp2_data_provider data_prd;
    data_prd.source.fd = stream_data->GetFd().value_or(-1);
    data_prd.read_callback = file_read_callback;

    header["date"] = ghttp::GetHttpDate();
    header["server"] = ghttp::GetHttpServer();
    std::vector<nghttp2_nv> hdrs;
    std::string status_value = std::to_string((long)status);
    hdrs.push_back({ (uint8_t*)STATUSH.c_str(), (uint8_t*)status_value.c_str(), STATUSH.size(), status_value.size(), NGHTTP2_NV_FLAG_NONE });
    for (auto& [k, v] : header) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
    }
    auto rv = nghttp2_submit_response(m_session, stream_data->GetStreamId().value(), hdrs.data(), hdrs.size(), &data_prd);
    if (rv != 0) {
        SPDLOG_ERROR("Fatal error: {}", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

bool CHTTPClient::submitRequest(ghttp::CStream* stream)
{
    std::map<std::string, std::string> header = {};
    header[":method"] = ghttp::HttpMethodStr(stream->GetRequest()->GetMethod()).value_or("");
    header[":scheme"] = stream->GetRequest()->GetScheme();
    header[":authority"] = fmt::format("{}:{}", stream->GetRequest()->GetHost(), stream->GetRequest()->GetPort());
    header[":path"] = stream->GetRequest()->GetPath();
    stream->GetRequest()->GetAllHeaders(header);

    std::vector<nghttp2_nv> hdrs;
    for (auto& [k, v] : header) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
        SPDLOG_DEBUG("HEADERS: {} {}", k, v);
    }

    std::string_view body = stream->GetRequest()->GetBody();
    nghttp2_data_provider pro;
    pro.read_callback = nocopy_read_callback;
    pro.source.ptr = body.empty() ? nullptr : CNEW std::string_view(body);
    auto stream_id = nghttp2_submit_request(m_session, nullptr, hdrs.data(), hdrs.size(), body.empty() ? nullptr : &pro, this);
    if (stream_id < 0) {
        SPDLOG_ERROR("Could not submit HTTP request: {}", nghttp2_strerror(stream_id));
        return false;
    }
    nghttp2_session_set_stream_user_data(GetNGHttp2Session(), stream_id, CreateStream(stream_id));

    SPDLOG_DEBUG("{},{}", __FUNCTION__, stream_id);
    return sessionSend();
}

ghttp::CStream* CHTTPClient::CreateStream(const int32_t streamid)
{
    m_streams[streamid].reset(CNEW ghttp::CStream(streamid, this));
    return m_streams[streamid].get();
}

bool CHTTPClient::DelStream(const int32_t streamid)
{
    auto stream = GetStream(streamid);
    if (stream && stream.value()->GetRequest()->GetFd()) {
        close(stream.value()->GetRequest()->GetFd().value());
    }

    m_streams.erase(streamid);
    return true;
}

std::optional<ghttp::CStream*> CHTTPClient::GetStream(const int32_t streamid)
{
    if (IsHttp2()) {
        if (auto it = m_streams.find(streamid); it != std::end(m_streams))
            return it->second.get();
    } else {
        return m_base_stream.get();
    }
    return std::nullopt;
}

bool CHTTPClient::InitNghttp2SessionData()
{
    nghttp2_session_callbacks* callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, sendCallback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, onFrameRecvCallback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, onStreamCloseCallback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, onHeaderCallback);
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, onBeginHeadersCallback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, onDataChunkRecvCallback);
    nghttp2_session_callbacks_set_before_frame_send_callback(callbacks, onBeforeFrameSendCallback);
    nghttp2_session_callbacks_set_send_data_callback(callbacks, onSendDataCallback);

    if (m_evcon->IsPassive())
        nghttp2_session_server_new(&m_session, callbacks, this);
    else
        nghttp2_session_client_new(&m_session, callbacks, this);

    nghttp2_session_callbacks_del(callbacks);

    if (!sendConnectionHeader())
        return false;
    if (!sessionSend())
        return false;

    SPDLOG_DEBUG("{}", __FUNCTION__);
    return true;
}

NAMESPACE_FRAMEWORK_END