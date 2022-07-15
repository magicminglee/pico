#include "httpserver.hpp"
#include "argument.hpp"
#include "connection.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include <fmt/core.h>

namespace fs = std::filesystem;
NAMESPACE_FRAMEWORK_BEGIN

/////////////////////////////////////////////////////////////////////////CWebSocket/////////////////////////////////////////////////////////////
CHttpParser::~CHttpParser()
{
    if (m_req && m_req->GetFd() && m_req->GetFd().value() > 0)
        close(m_req->GetFd().value());
    m_req.release();
    m_rsp.release();
    if (m_session) {
        nghttp2_session_del(m_session);
    }
}

void CHttpParser::SetNgHttp2Session(nghttp2_session* session)
{
    m_session = session;
}

int32_t CHttpParser::ParseRequest(std::string_view data)
{
    return IsHttp2() ? parseHttp2Request(data) : parseHttp1Request(data);
}

int32_t CHttpParser::ParseResponse(std::string_view data)
{
    return IsHttp2() ? parseHttp2Response(data) : parseHttp1Response(data);
}

int32_t CHttpParser::parseHttp1Response(std::string_view data)
{
    int32_t pret = 0;
    if (EParserState::EParserState_Begin == m_state) {
        int minor_version, status;
        const char* msg;
        size_t msg_len, num_headers;
        struct phr_header headers[50];
        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_response(data.data(), data.size(), &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);
        if (pret > 0) {
            GetResponse()->Reset();
            m_decoder = {};
            m_decoder.consume_trailer = 1;
            GetResponse()->SetStatus((ghttp::HttpStatusCode)status);
            m_state = EParserState::EParserState_Head;
            for (size_t i = 0; i != num_headers; ++i) {
                GetResponse()->AddHeader(std::string(headers[i].name, headers[i].name_len), std::string(headers[i].value, headers[i].value_len));
            }
        } else if (pret == -1) {
            m_state = EParserState::EParserState_End;
            return -1;
        } else {
            return 0;
        }
    }
    if (EParserState::EParserState_Head == m_state) {
        if (GetResponse()->HasHeader("transfer-encoding") && GetResponse()->GetHeaderByKey("transfer-encoding") == "chunked") {
            auto headlen = pret;
            size_t rsize = data.length() - headlen;
            std::unique_ptr<char> buf = std::make_unique<char>(rsize);
            memcpy(buf.get(), data.data() + headlen, rsize);
            pret = phr_decode_chunked(&m_decoder, buf.get(), &rsize);
            if (pret >= 0) {
                GetResponse()->AppendBody(std::string_view(buf.get(), rsize));
                m_state = EParserState::EParserState_Body;
                return data.size() - pret;
            } else if (pret == -1) {
                m_state = EParserState::EParserState_End;
                return -1;
            }
        } else if (GetResponse()->HasHeader("content-length")) {
            size_t bodylen = CStringTool::ToInteger<size_t>(GetResponse()->GetHeaderByKey("content-length").value());
            if (data.size() >= pret + bodylen) {
                if (bodylen > 0)
                    GetResponse()->SetBody(std::string_view(data.data() + pret, bodylen));
                m_state = EParserState::EParserState_Body;
                return pret + bodylen;
            }
        } else {
            m_state = EParserState::EParserState_Body;
            return data.length();
        }
    }
    return 0;
}

int32_t CHttpParser::parseHttp1Request(std::string_view data)
{
    int32_t pret = 0;
    if (EParserState::EParserState_Begin == m_state) {
        const char *method, *path;
        int minor_version;
        struct phr_header headers[50];
        size_t method_len, path_len, num_headers;
        ssize_t rret;
        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_request(data.data(), data.size(), &method, &method_len, &path, &path_len,
            &minor_version, headers, &num_headers, 0);
        if (pret > 0) {
            GetRequest()->Reset();
            m_state = EParserState::EParserState_Head;
            auto m = std::string(method, method_len);
            GetRequest()->SetMethod(ghttp::HttpStrMethod(m).value_or(ghttp::HttpMethod::GET));
            GetRequest()->SetPath(std::string(path, path_len));
            for (size_t i = 0; i != num_headers; ++i) {
                GetRequest()->AddHeader(std::string(headers[i].name, headers[i].name_len), std::string(headers[i].value, headers[i].value_len));
            }
        } else if (pret == -1) {
            m_state = EParserState::EParserState_End;
            return -1;
        } else {
            return 0;
        }
    }
    if (EParserState::EParserState_Head == m_state) {
        if (GetRequest()->HasHeader("transfer-encoding") && GetRequest()->GetHeaderByKey("transfer-encoding") == "chunked") {
            auto headlen = pret;
            size_t rsize = data.length() - headlen;
            std::unique_ptr<char> buf = std::make_unique<char>(rsize);
            memcpy(buf.get(), data.data() + headlen, rsize);
            pret = phr_decode_chunked(&m_decoder, buf.get(), &rsize);
            if (pret >= 0) {
                GetRequest()->AppendBody(std::string_view(buf.get(), rsize));
                m_state = EParserState::EParserState_Body;
                return data.size() - pret;
            } else if (pret == -1) {
                m_state = EParserState::EParserState_End;
                return -1;
            }
        } else if (GetRequest()->HasHeader("content-length")) {
            size_t bodylen = CStringTool::ToInteger<size_t>(GetRequest()->GetHeaderByKey("content-length").value());
            if (data.size() >= pret + bodylen) {
                if (bodylen > 0)
                    GetRequest()->SetBody(std::string(data.data() + pret, bodylen));
                m_state = EParserState::EParserState_Body;
                return pret + bodylen;
            }
        } else {
            m_state = EParserState::EParserState_Body;
            return data.length();
        }
    }
    return 0;
}

int32_t CHttpParser::parseHttp2Request(std::string_view data)
{
    ssize_t readlen = nghttp2_session_mem_recv(m_session, (const uint8_t*)data.data(), data.size());
    if (readlen < 0) {
        CERROR("Fatal error: %s", nghttp2_strerror((int)readlen));
        return -1;
    }

    int32_t rv = nghttp2_session_send(m_session);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return -1;
    }
    return readlen;
};

int32_t CHttpParser::parseHttp2Response(std::string_view data)
{
    ssize_t readlen = nghttp2_session_mem_recv(m_session, (const uint8_t*)data.data(), data.size());
    if (readlen < 0) {
        CERROR("Fatal error: %s", nghttp2_strerror((int)readlen));
        return -1;
    }

    int32_t rv = nghttp2_session_send(m_session);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return -1;
    }
    return readlen;
};

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
    CDEBUG("CWebSocket::Upgrade");
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
    auto key = CWSParser::GenSecWebSocketAccept(std::to_string(time(nullptr)));
    return (bool)CHTTPClient::Emit(
        url,
        [key, cb](ghttp::CResponse* rsp) {
            // CDEBUG("onWSData %s", rsp->DebugStr().c_str());
            auto seckey = rsp->GetHeaderByKey("sec-websocket-accept");
            if (seckey) {
                auto sk = std::string(seckey.value().data(), seckey.value().length());
                if (sk == CWSParser::GenSecWebSocketAccept(key)) {
                    CWebSocket::Upgrade(rsp, cb);
                    return true;
                }
            }
            return false;
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
            CDEBUG("CWebSocket::SendCmd");
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
        CDEBUG("%s is closed", __FUNCTION__);
        CDEL(self);
    }
}

void CWebSocket::onError(struct bufferevent* bev, short which, void* arg)
{
    CDEBUG("%s", __FUNCTION__);
    CWebSocket* self = (CWebSocket*)arg;
    CDEL(self);
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

void CHTTPServer::ServeWs(const std::string path, CWebSocket::Callback cb)
{
    Register(
        path,
        ghttp::HttpMethod::GET,
        [cb](const ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = req->GetHeaderByKey("sec-websocket-key");
            if (v) {
                std::string swsk(v.value().data(), v.value().length());
                swsk = CWSParser::GenSecWebSocketAccept(swsk);
                rsp->AddHeader("Connection", "upgrade");
                rsp->AddHeader("Sec-WebSocket-Accept", swsk);
                rsp->AddHeader("Upgrade", "websocket");
                rsp->Response({ ghttp::HttpStatusCode::SWITCH, "" });
                CWebSocket::Upgrade(rsp, cb);
                return true;
            }
            return rsp->Response({ ghttp::HttpStatusCode::FORBIDDEN, "" });
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

bool CHTTPServer::Emit(const std::string path, ghttp::CRequest* req, ghttp::CResponse* rsp)
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

bool CHTTPServer::ListenAndServe(std::string host)
{
    CheckCondition(!host.empty(), true);

    return m_tcpserver.ListenAndServe(host, [this, host](const int32_t fd) {
        CHTTPClient* hclient = CNEW CHTTPClient();
        CConnectionHandler* h = CNEW CConnectionHandler();
        h->Register(
            [this, hclient](std::string_view data) {
                auto ret = hclient->GetParser().ParseRequest(data);
                if (ret == -1) {
                    delete hclient;
                } else if (!hclient->IsHttp2() && ret > 0) {
                    auto req = hclient->GetParser().GetRequest();
                    auto rsp = hclient->GetParser().GetResponse();
                    auto filter = this->GetFilter(req->GetPath().value_or(""));
                    auto cmd = (uint32_t)req->GetMethod();
                    // CDEBUG("request %s", req->DebugStr().c_str());
                    if (filter) {
                        if (0 == ((uint32_t)(filter.value()->cmd) & cmd)) {
                            rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "" });
                        } else {
                            auto r = this->EmitEvent("start", req, rsp);
                            if (r) {
                                rsp->Response({ r.value().first, ghttp::HttpReason(r.value().first).value_or("") });
                            } else {
                                auto res = filter.value()->cb(req, rsp);
                                if (res) {
                                    this->EmitEvent("finish", req, rsp);
                                }
                            }
                        }
                    } else {
                        rsp->SendFile(req->GetPath().value_or(""));
                    }
                }
                return ret;
            },
            [hclient]() {
                CHTTPClient::OnWrite(hclient);
            },
            [this, hclient](const EnumConnEventType e) {
                if (e == EnumConnEventType::EnumConnEventType_Connected) {
                    auto ish2 = hclient->CheckHttp2();
                    if (ish2) {
                        hclient->InitNghttp2SessionData();
                    }
                } else if (e == EnumConnEventType::EnumConnEventType_Closed) {
                    delete hclient;
                }
            });
        if (h->Init(fd, host))
            hclient->Init(h->Connection(), this);
    });
}

bool CHTTPServer::setOption(const int32_t fd)
{
    if (evutil_make_listen_socket_reuseable(fd) < 0)
        return false;
    if (evutil_make_listen_socket_reuseable_port(fd) < 0)
        return false;
    if (evutil_make_socket_nonblocking(fd) < 0)
        return false;
#if defined(LINUX_PLATFORMOS) || defined(DARWIN_PLATFORMOS)
    int32_t flags = 1;
    int32_t error = ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flags, sizeof(flags));
    if (0 != error)
        return false;
#endif
    return true;
}

///////////////////////////////////////////////////////////////CHTTPClient/////////////////////////////////////////////////////////////////////////
CHTTPClient::~CHTTPClient()
{
}

void CHTTPClient::Init(CConnection* conn, CHTTPServer* server)
{
    SetConnection(conn);
    SetHttpServer(server);
    GetParser().SetRequest(CNEW ghttp::CRequest());
    GetParser().SetResponse(CNEW ghttp::CResponse(this));
}

bool CHTTPClient::Request()
{
    auto ish2 = CheckHttp2();
    if (ish2) {
        InitNghttp2SessionData();
        if (!m_evcon->IsPassive()) {
            // CWorker::MAIN_CONTEX->AddPersistEvent(1, 2000, [this]() { this->submitRequest(); });
            return submitRequest();
        }
    } else {
        std::map<std::string, std::string> headers;
        GetParser().GetRequest()->GetAllHeaders(headers);
        std::string httprsp = fmt::format("{} {} HTTP/1.1\r\n", ghttp::HttpMethodStr(GetParser().GetRequest()->GetMethod()).value_or(""), GetParser().GetRequest()->GetPath().value());
        for (auto& [k, v] : headers) {
            if (k == "sec-websocket-key") {
                // compatible with case sensitive
                httprsp += fmt::format("{}: {}\r\n", "Sec-WebSocket-Key", v);
            } else {
                httprsp += fmt::format("{}: {}\r\n", k, v);
            }
        }
        httprsp += fmt::format("Content-Length: {}\r\n", GetParser().GetRequest()->GetBody().value_or("").size());
        httprsp += fmt::format("\r\n");
        if (GetParser().GetRequest()->GetBody())
            httprsp.append(GetParser().GetRequest()->GetBody().value());
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
    CHTTPClient* hclient = CNEW CHTTPClient();
    hclient->m_callback = cb;
    hclient->m_url = std::string(url.data(), url.size());

    CConnectionHandler* h = CNEW CConnectionHandler();
    h->Register(
        [hclient](std::string_view data) {
            int32_t ret = hclient->GetParser().ParseResponse(data);
            if (ret == -1) {
                delete hclient;
            } else if (!hclient->IsHttp2() && ret > 0) {
                hclient->Response();
            }
            return ret;
        },
        [hclient]() {
            CHTTPClient::OnWrite(hclient);
        },
        [hclient, method, body, header](const EnumConnEventType e) {
            if (e == EnumConnEventType::EnumConnEventType_Connected) {
                auto [scheme, hostname, port, path] = CConnection::SplitUri(hclient->GetURL());
                for (auto& [k, v] : header) {
                    hclient->GetParser().GetRequest()->AddHeader(k, v);
                }
                hclient->GetParser().GetRequest()->SetMethod(method);
                hclient->GetParser().GetRequest()->SetScheme(scheme);
                hclient->GetParser().GetRequest()->SetHost(hostname);
                hclient->GetParser().GetRequest()->SetPort(port);
                hclient->GetParser().GetRequest()->SetPath(path);
                if (body)
                    hclient->GetParser().GetRequest()->SetBody(body.value());

                hclient->Request();
            } else if (e == EnumConnEventType::EnumConnEventType_Closed) {
                delete hclient;
            }
        });
    if (h->Connect(hclient->m_url))
        hclient->Init(h->Connection(), nullptr);

    return hclient;
}

std::map<std::string, std::string> CHTTPClient::getHttp2Header()
{
    std::map<std::string, std::string> header = {};
    header[":method"] = ghttp::HttpMethodStr(GetParser().GetRequest()->GetMethod()).value_or("");
    header[":scheme"] = GetParser().GetRequest()->GetScheme().value_or("");
    header[":authority"] = GetParser().GetRequest()->GetHost().value_or("") + ":" + GetParser().GetRequest()->GetPort().value_or("");
    header[":path"] = GetParser().GetRequest()->GetPath().value_or("");
    GetParser().GetRequest()->GetAllHeaders(header);

    return header;
}

void CHTTPClient::Response()
{
    if (m_callback)
        m_callback(GetParser().GetResponse());
    m_callback = nullptr;
}

void CHTTPClient::OnWrite(CHTTPClient* hclient)
{
    if (hclient->m_session) {
        if (hclient->isPassive()) {
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
    CDEBUG("%s %s http2 is enabled and h2 is negotiated success %d", MYARGS.CTXID.c_str(), __FUNCTION__, isserver);

    return true;
}

ssize_t CHTTPClient::onDataSourceReadCallback(nghttp2_session* session, int32_t stream_id, uint8_t* buf, size_t length, uint32_t* data_flags, nghttp2_data_source* source, void* user_data)
{
    std::string* data = (std::string*)source->ptr;
    auto size = data->size();
    memcpy(buf, data->data(), data->size());
    *data_flags |= nghttp2_data_flag::NGHTTP2_DATA_FLAG_EOF;
    CDEL(data);
    return size;
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

int CHTTPClient::onRequestRecv(ghttp::CRequest* req)
{
    if (req->GetPath()) {
        // CDEBUG("%s %s %d", MYARGS.CTXID.c_str(), __FUNCTION__, req->GetStreamId().value());
        ghttp::CResponse rsp(this);
        auto r = m_httpserver->EmitEvent("start", req, &rsp);
        if (r) {
            SendResponse(req, r.value().first, {}, r.value().second);
            return 0;
        }

        if (!m_httpserver->Emit(req->GetPath().value(), req, &rsp)) {
            fs::path f = MYARGS.WebRootDir.value_or(fs::current_path()) + req->GetPath().value();
            if (!fs::exists(f)) {
                SendResponse(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
                return 0;
            }
            auto mime = ghttp::GetMiMe(f.extension());
            if (!mime) {
                SendResponse(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
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
            SendResponse(req, ghttp::HttpStatusCode::NOTFOUND, {}, ghttp::HttpReason(ghttp::HttpStatusCode::NOTFOUND).value_or(""));
            return 0;
        }
        m_httpserver->EmitEvent("finish", req, &rsp);
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
                ghttp::CRequest* stream_data = (ghttp::CRequest*)nghttp2_session_get_stream_user_data(session_data->m_session, frame->hd.stream_id);
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
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && session_data->GetParser().GetRequest()->GetStreamId().value_or(-1) == frame->hd.stream_id) {
                // CDEBUG("All headers received");
            }
            break;
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
        ghttp::CRequest* stream_data = (ghttp::CRequest*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (!stream_data) {
            return 0;
        }
        stream_data->SetBody(std::string((char*)data, len));
    } else {
        if (session_data->GetParser().GetRequest()->GetStreamId().value_or(-1) == stream_id) {
            session_data->GetParser().GetResponse()->SetBody(std::string_view((const char*)data, len));
            session_data->Response();
            CDEBUG("All data received %s", data);
        }
    }
    return 0;
}

int CHTTPClient::onStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data)
{
    CHTTPClient* session_data = (CHTTPClient*)user_data;
    (void)error_code;

    if (session_data->m_httpserver) {
        ghttp::CRequest* stream_data = (ghttp::CRequest*)nghttp2_session_get_stream_user_data(session, stream_id);
        if (!stream_data) {
            return 0;
        }
        // CDEBUG("%s ServerStream %d closed with error_code=%u errstr %s", MYARGS.CTXID.c_str(), stream_id, error_code, nghttp2_strerror(error_code));
        return 0;
    } else {
        if (session_data->GetParser().GetRequest()->GetStreamId().value_or(-1) == stream_id) {
            // CDEBUG("%s ClientStream %d closed with error_code=%u errstr %s", MYARGS.CTXID.c_str(), stream_id, error_code, nghttp2_strerror(error_code));
            //  auto rv = nghttp2_session_terminate_session(session, NGHTTP2_NO_ERROR);
            //  if (rv != 0) {
            //      return NGHTTP2_ERR_CALLBACK_FAILURE;
            //  }
        }
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
            ghttp::CRequest* stream_data = (ghttp::CRequest*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            if (!stream_data) {
                break;
            }
            stream_data->AddHeader(std::string((char*)name, namelen), std::string((char*)value, valuelen));
            if (namelen == sizeof(PATH) - 1 && memcmp(PATH, name, namelen) == 0) {
                size_t j;
                for (j = 0; j < valuelen && value[j] != '?'; ++j)
                    ;
                stream_data->SetPath(std::string((char*)value, j));
            }
            break;
        }
    } else {
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && session_data->GetParser().GetRequest()->GetStreamId().value_or(-1) == frame->hd.stream_id) {
                session_data->GetParser().GetResponse()->AddHeader(std::string((char*)name, namelen), std::string((char*)value, valuelen));
                if (session_data->GetParser().GetResponse()->HasHeader(":status"))
                    session_data->GetParser().GetResponse()->SetStatus((ghttp::HttpStatusCode)CStringTool::ToInteger<int32_t>(session_data->GetParser().GetResponse()->GetHeaderByKey(":status").value()));
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
        ghttp::CRequest* stream_data = session_data->GetParser().GetRequest();
        stream_data->SetStreamId(frame->hd.stream_id);
        nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, stream_data);
    } else {
        switch (frame->hd.type) {
        case NGHTTP2_HEADERS:
            if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE && session_data->GetParser().GetRequest()->GetStreamId().value_or(-1) == frame->hd.stream_id) {
                return 0;
            }
            break;
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
        // CDEBUG("%s Server %u", __FUNCTION__, frame->hd.type);
    } else {
        // CDEBUG("%s Client %u", __FUNCTION__, frame->hd.type);
    }
    return 0;
}

bool CHTTPClient::sendConnectionHeader()
{
    std::vector<nghttp2_settings_entry> iv = { { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100 } };

    int32_t rv = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv.data(), iv.size());
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

bool CHTTPClient::sessionSend()
{
    int32_t rv = nghttp2_session_send(m_session);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
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

    while ((r = read(fd, buf, length)) == -1 && errno == EINTR)
        ;
    if (r == -1) {
        return NGHTTP2_ERR_TEMPORAL_CALLBACK_FAILURE;
    }
    if (r == 0) {
        *data_flags |= NGHTTP2_DATA_FLAG_EOF;
    }
    return r;
}
bool CHTTPClient::SendResponse(ghttp::CRequest* stream_data,
    const ghttp::HttpStatusCode status, std::unordered_map<std::string, std::string> header, std::optional<std::string_view> data)
{
    static const std::string STATUSH = ":status";
    int pipefd[2];
    auto rv = pipe(pipefd);
    if (rv != 0) {
        rv = nghttp2_submit_rst_stream(m_session, NGHTTP2_FLAG_NONE,
            stream_data->GetStreamId().value(),
            NGHTTP2_INTERNAL_ERROR);
        if (rv != 0) {
            return false;
        }
        return true;
    }

    size_t writelen = write(pipefd[1], data.value_or("").data(), data.value_or("").length());
    close(pipefd[1]);

    if (writelen != data.value_or("").length()) {
        close(pipefd[0]);
        return false;
    }
    stream_data->SetFd(pipefd[0]);
    nghttp2_data_provider data_prd;
    data_prd.source.fd = stream_data->GetFd().value();
    data_prd.read_callback = file_read_callback;

    std::vector<nghttp2_nv> hdrs;
    std::string status_value = std::to_string((long)status);
    hdrs.push_back({ (uint8_t*)STATUSH.c_str(), (uint8_t*)status_value.c_str(), STATUSH.size(), status_value.size(), NGHTTP2_NV_FLAG_NONE });
    for (auto& [k, v] : header) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
    }
    rv = nghttp2_submit_response(m_session, stream_data->GetStreamId().value(), hdrs.data(), hdrs.size(), &data_prd);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
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
    data_prd.source.fd = stream_data->GetFd().value();
    data_prd.read_callback = file_read_callback;

    std::vector<nghttp2_nv> hdrs;
    std::string status_value = std::to_string((long)status);
    hdrs.push_back({ (uint8_t*)STATUSH.c_str(), (uint8_t*)status_value.c_str(), STATUSH.size(), status_value.size(), NGHTTP2_NV_FLAG_NONE });
    for (auto& [k, v] : header) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
    }
    auto rv = nghttp2_submit_response(m_session, stream_data->GetStreamId().value(), hdrs.data(), hdrs.size(), &data_prd);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

bool CHTTPClient::submitRequest()
{
    auto&& headers = getHttp2Header();
    std::vector<nghttp2_nv> hdrs;
    for (auto& [k, v] : headers) {
        hdrs.push_back({ (uint8_t*)k.c_str(), (uint8_t*)v.c_str(), k.size(), v.size(), NGHTTP2_NV_FLAG_NONE });
        CDEBUG("HEADERS: %s %s", k.c_str(), v.c_str());
    }

    nghttp2_data_provider pro;
    std::string* data = CNEW std::string("JIMMY");
    pro.source.ptr = data;
    pro.read_callback = onDataSourceReadCallback;
    auto stream_id = nghttp2_submit_request(m_session, NULL, hdrs.data(), hdrs.size(), &pro, this);
    if (stream_id < 0) {
        CERROR("Could not submit HTTP request: %s", nghttp2_strerror(stream_id));
        return false;
    }
    GetParser().GetRequest()->SetStreamId(stream_id);
    CDEBUG("%s,%d", __FUNCTION__, stream_id);
    return sessionSend();
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

    if (m_evcon->IsPassive())
        nghttp2_session_server_new(&m_session, callbacks, this);
    else
        nghttp2_session_client_new(&m_session, callbacks, this);

    nghttp2_session_callbacks_del(callbacks);

    if (!sendConnectionHeader())
        return false;
    if (!sessionSend())
        return false;

    struct timeval rwtv = { MYARGS.HttpTimeout.value_or(60), 0 };
    bufferevent_set_timeouts(GetConnection()->GetBufEvent(), &rwtv, &rwtv);
    GetParser().SetNgHttp2Session(GetNGHttp2Session());

    return true;
}

NAMESPACE_FRAMEWORK_END