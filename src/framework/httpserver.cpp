#include "httpserver.hpp"
#include "argument.hpp"
#include "connection.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include <filesystem>

NAMESPACE_FRAMEWORK_BEGIN
namespace fs = std::filesystem;

static const std::map<std::string, std::string> MIME = {
    { ".ai", "application/postscript; charset=utf-8" },
    { ".aif", "audio/x-aiff; charset=utf-8" },
    { ".aifc", "audio/x-aiff; charset=utf-8" },
    { ".aiff", "audio/x-aiff; charset=utf-8" },
    { ".arj", "application/x-arj-compressed; charset=utf-8" },
    { ".asc", "text/plain; charset=utf-8" },
    { ".asf", "video/x-ms-asf; charset=utf-8" },
    { ".asx", "video/x-ms-asx; charset=utf-8" },
    { ".au", "audio/ulaw; charset=utf-8" },
    { ".avi", "video/x-msvideo; charset=utf-8" },
    { ".bat", "application/x-msdos-program; charset=utf-8" },
    { ".bcpio", "application/x-bcpio; charset=utf-8" },
    { ".bin", "application/octet-stream; charset=utf-8" },
    { ".c", "text/plain; charset=utf-8" },
    { ".cc", "text/plain; charset=utf-8" },
    { ".ccad", "application/clariscad; charset=utf-8" },
    { ".cdf", "application/x-netcdf; charset=utf-8" },
    { ".class", "application/octet-stream; charset=utf-8" },
    { ".cod", "application/vnd.rim.cod; charset=utf-8" },
    { ".com", "application/x-msdos-program; charset=utf-8" },
    { ".cpio", "application/x-cpio; charset=utf-8" },
    { ".cpt", "application/mac-compactpro; charset=utf-8" },
    { ".csh", "application/x-csh; charset=utf-8" },
    { ".css", "text/css; charset=utf-8" },
    { ".dcr", "application/x-director; charset=utf-8" },
    { ".deb", "application/x-debian-package; charset=utf-8" },
    { ".dir", "application/x-director; charset=utf-8" },
    { ".dl", "video/dl; charset=utf-8" },
    { ".dms", "application/octet-stream; charset=utf-8" },
    { ".doc", "application/msword; charset=utf-8" },
    { ".drw", "application/drafting; charset=utf-8" },
    { ".dvi", "application/x-dvi; charset=utf-8" },
    { ".dwg", "application/acad; charset=utf-8" },
    { ".dxf", "application/dxf; charset=utf-8" },
    { ".dxr", "application/x-director; charset=utf-8" },
    { ".eps", "application/postscript; charset=utf-8" },
    { ".etx", "text/x-setext; charset=utf-8" },
    { ".exe", "application/octet-stream; charset=utf-8" },
    { ".ez", "application/andrew-inset; charset=utf-8" },
    { ".f", "text/plain; charset=utf-8" },
    { ".f90", "text/plain; charset=utf-8" },
    { ".fli", "video/fli; charset=utf-8" },
    { ".flv", "video/flv; charset=utf-8" },
    { ".gif", "image/gif; charset=utf-8" },
    { ".gl", "video/gl; charset=utf-8" },
    { ".gtar", "application/x-gtar; charset=utf-8" },
    { ".gz", "application/x-gzip; charset=utf-8" },
    { ".hdf", "application/x-hdf; charset=utf-8" },
    { ".hh", "text/plain; charset=utf-8" },
    { ".hqx", "application/mac-binhex40; charset=utf-8" },
    { ".h", "text/plain; charset=utf-8" },
    { ".htm", "text/html; charset=utf-8" },
    { ".html", "text/html; charset=utf-8" },
    { ".ice", "x-conference/x-cooltalk; charset=utf-8" },
    { ".ief", "image/ief; charset=utf-8" },
    { ".iges", "model/iges; charset=utf-8" },
    { ".igs", "model/iges; charset=utf-8" },
    { ".ips", "application/x-ipscript; charset=utf-8" },
    { ".ipx", "application/x-ipix; charset=utf-8" },
    { ".jad", "text/vnd.sun.j2me.app-descriptor; charset=utf-8" },
    { ".jar", "application/java-archive; charset=utf-8" },
    { ".jpeg", "image/jpeg; charset=utf-8" },
    { ".jpe", "image/jpeg; charset=utf-8" },
    { ".jpg", "image/jpeg; charset=utf-8" },
    { ".js", "text/x-javascript; charset=utf-8" },
    { ".json", "application/json; charset=utf-8" },
    { ".kar", "audio/midi; charset=utf-8" },
    { ".latex", "application/x-latex; charset=utf-8" },
    { ".lha", "application/octet-stream; charset=utf-8" },
    { ".lsp", "application/x-lisp; charset=utf-8" },
    { ".lzh", "application/octet-stream; charset=utf-8" },
    { ".m", "text/plain; charset=utf-8" },
    { ".m3u", "audio/x-mpegurl; charset=utf-8" },
    { ".man", "application/x-troff-man; charset=utf-8" },
    { ".me", "application/x-troff-me; charset=utf-8" },
    { ".mesh", "model/mesh; charset=utf-8" },
    { ".mid", "audio/midi; charset=utf-8" },
    { ".midi", "audio/midi; charset=utf-8" },
    { ".mif", "application/x-mif; charset=utf-8" },
    { ".mime", "www/mime; charset=utf-8" },
    { ".mjs", "text/javascript" },
    { ".movie", "video/x-sgi-movie; charset=utf-8" },
    { ".mov", "video/quicktime; charset=utf-8" },
    { ".mp2", "audio/mpeg; charset=utf-8" },
    { ".mp2", "video/mpeg; charset=utf-8" },
    { ".mp3", "audio/mpeg; charset=utf-8" },
    { ".mpeg", "video/mpeg; charset=utf-8" },
    { ".mpe", "video/mpeg; charset=utf-8" },
    { ".mpga", "audio/mpeg; charset=utf-8" },
    { ".mpg", "video/mpeg; charset=utf-8" },
    { ".ms", "application/x-troff-ms; charset=utf-8" },
    { ".msh", "model/mesh; charset=utf-8" },
    { ".nc", "application/x-netcdf; charset=utf-8" },
    { ".oda", "application/oda; charset=utf-8" },
    { ".ogg", "application/ogg; charset=utf-8" },
    { ".ogm", "application/ogg; charset=utf-8" },
    { ".pbm", "image/x-portable-bitmap; charset=utf-8" },
    { ".pdb", "chemical/x-pdb; charset=utf-8" },
    { ".pdf", "application/pdf; charset=utf-8" },
    { ".pgm", "image/x-portable-graymap; charset=utf-8" },
    { ".pgn", "application/x-chess-pgn; charset=utf-8" },
    { ".pgp", "application/pgp; charset=utf-8" },
    { ".pl", "application/x-perl; charset=utf-8" },
    { ".pm", "application/x-perl; charset=utf-8" },
    { ".png", "image/png; charset=utf-8" },
    { ".pnm", "image/x-portable-anymap; charset=utf-8" },
    { ".pot", "application/mspowerpoint; charset=utf-8" },
    { ".ppm", "image/x-portable-pixmap; charset=utf-8" },
    { ".pps", "application/mspowerpoint; charset=utf-8" },
    { ".ppt", "application/mspowerpoint; charset=utf-8" },
    { ".ppz", "application/mspowerpoint; charset=utf-8" },
    { ".pre", "application/x-freelance; charset=utf-8" },
    { ".prt", "application/pro_eng; charset=utf-8" },
    { ".ps", "application/postscript; charset=utf-8" },
    { ".qt", "video/quicktime; charset=utf-8" },
    { ".ra", "audio/x-realaudio; charset=utf-8" },
    { ".ram", "audio/x-pn-realaudio; charset=utf-8" },
    { ".rar", "application/x-rar-compressed; charset=utf-8" },
    { ".ras", "image/cmu-raster; charset=utf-8" },
    { ".ras", "image/x-cmu-raster; charset=utf-8" },
    { ".rgb", "image/x-rgb; charset=utf-8" },
    { ".rm", "audio/x-pn-realaudio; charset=utf-8" },
    { ".roff", "application/x-troff; charset=utf-8" },
    { ".rpm", "audio/x-pn-realaudio-plugin; charset=utf-8" },
    { ".rtf", "application/rtf; charset=utf-8" },
    { ".rtf", "text/rtf; charset=utf-8" },
    { ".rtx", "text/richtext; charset=utf-8" },
    { ".scm", "application/x-lotusscreencam; charset=utf-8" },
    { ".set", "application/set; charset=utf-8" },
    { ".sgml", "text/sgml; charset=utf-8" },
    { ".sgm", "text/sgml; charset=utf-8" },
    { ".sh", "application/x-sh; charset=utf-8" },
    { ".shar", "application/x-shar; charset=utf-8" },
    { ".silo", "model/mesh; charset=utf-8" },
    { ".sit", "application/x-stuffit; charset=utf-8" },
    { ".skd", "application/x-koan; charset=utf-8" },
    { ".skm", "application/x-koan; charset=utf-8" },
    { ".skp", "application/x-koan; charset=utf-8" },
    { ".skt", "application/x-koan; charset=utf-8" },
    { ".smi", "application/smil; charset=utf-8" },
    { ".smil", "application/smil; charset=utf-8" },
    { ".snd", "audio/basic; charset=utf-8" },
    { ".sol", "application/solids; charset=utf-8" },
    { ".spl", "application/x-futuresplash; charset=utf-8" },
    { ".src", "application/x-wais-source; charset=utf-8" },
    { ".step", "application/STEP; charset=utf-8" },
    { ".stl", "application/SLA; charset=utf-8" },
    { ".stp", "application/STEP; charset=utf-8" },
    { ".sv4cpio", "application/x-sv4cpio; charset=utf-8" },
    { ".sv4crc", "application/x-sv4crc; charset=utf-8" },
    { ".svg", "image/svg+xml; charset=utf-8" },
    { ".swf", "application/x-shockwave-flash; charset=utf-8" },
    { ".t", "application/x-troff; charset=utf-8" },
    { ".tar", "application/x-tar; charset=utf-8" },
    { ".tcl", "application/x-tcl; charset=utf-8" },
    { ".tex", "application/x-tex; charset=utf-8" },
    { ".texi", "application/x-texinfo; charset=utf-8" },
    { ".texinfo", "application/x-texinfo; charset=utf-8" },
    { ".tgz", "application/x-tar-gz; charset=utf-8" },
    { ".tiff", "image/tiff; charset=utf-8" },
    { ".tif", "image/tiff; charset=utf-8" },
    { ".tr", "application/x-troff; charset=utf-8" },
    { ".tsi", "audio/TSP-audio; charset=utf-8" },
    { ".tsp", "application/dsptype; charset=utf-8" },
    { ".tsv", "text/tab-separated-values; charset=utf-8" },
    { ".txt", "text/plain; charset=utf-8" },
    { ".unv", "application/i-deas; charset=utf-8" },
    { ".ustar", "application/x-ustar; charset=utf-8" },
    { ".vcd", "application/x-cdlink; charset=utf-8" },
    { ".vda", "application/vda; charset=utf-8" },
    { ".viv", "video/vnd.vivo; charset=utf-8" },
    { ".vivo", "video/vnd.vivo; charset=utf-8" },
    { ".vrml", "model/vrml; charset=utf-8" },
    { ".vsix", "application/vsix; charset=utf-8" },
    { ".wasm", "application/wasm" },
    { ".wav", "audio/x-wav; charset=utf-8" },
    { ".wax", "audio/x-ms-wax; charset=utf-8" },
    { ".wiki", "application/x-fossil-wiki; charset=utf-8" },
    { ".wma", "audio/x-ms-wma; charset=utf-8" },
    { ".wmv", "video/x-ms-wmv; charset=utf-8" },
    { ".wmx", "video/x-ms-wmx; charset=utf-8" },
    { ".wrl", "model/vrml; charset=utf-8" },
    { ".wvx", "video/x-ms-wvx; charset=utf-8" },
    { ".xbm", "image/x-xbitmap; charset=utf-8" },
    { ".xlc", "application/vnd.ms-excel; charset=utf-8" },
    { ".xll", "application/vnd.ms-excel; charset=utf-8" },
    { ".xlm", "application/vnd.ms-excel; charset=utf-8" },
    { ".xls", "application/vnd.ms-excel; charset=utf-8" },
    { ".xlw", "application/vnd.ms-excel; charset=utf-8" },
    { ".xml", "text/xml; charset=utf-8" },
    { ".xpm", "image/x-xpixmap; charset=utf-8" },
    { ".xwd", "image/x-xwindowdump; charset=utf-8" },
    { ".xyz", "chemical/x-pdb; charset=utf-8" },
    { ".zip", "application/zip; charset=utf-8" }
};

static const std::map<ghttp::HttpStatusCode, std::string> RESPSTR = {
    { ghttp::HttpStatusCode::SWITCH, "SWITCHING PROTOCOLS" },
    { ghttp::HttpStatusCode::OK, "OK" },
    { ghttp::HttpStatusCode::NOCONTENT, "NO CONTENT" },
    { ghttp::HttpStatusCode::MOVEPERM, "MOVED PERMANENTLY" },
    { ghttp::HttpStatusCode::MOVETEMP, "MOVED TEMPORARILY" },
    { ghttp::HttpStatusCode::NOTMODIFIED, "NOT MODIFIED" },
    { ghttp::HttpStatusCode::BADREQUEST, "INVALID REQUEST" },
    { ghttp::HttpStatusCode::UNAUTHORIZED, "UNAUTHORIZED" },
    { ghttp::HttpStatusCode::FORBIDDEN, "FORBIDDEN" },
    { ghttp::HttpStatusCode::NOTFOUND, "NOT FOUND" },
    { ghttp::HttpStatusCode::BADMETHOD, "METHOD NOT ALLOWED" },
    { ghttp::HttpStatusCode::ENTITYTOOLARGE, "ENTITYTOOLARGE" },
    { ghttp::HttpStatusCode::EXPECTATIONFAILED, "EXPECTATIONFAILED" },
    { ghttp::HttpStatusCode::INTERNAL, "INTERNAL ERROR" },
    { ghttp::HttpStatusCode::SERVUNAVAIL, "NOT AVAILABLE" },
    { ghttp::HttpStatusCode::BADGATEWAY, "BADGATEWAY" },
    { ghttp::HttpStatusCode::UNAUTHORIZED, "UNAUTHORIZED" },
};

namespace ghttp {
std::optional<const char*> HttpReason(ghttp::HttpStatusCode code)
{
    if (auto it = RESPSTR.find(code); it != std::end(RESPSTR))
        return std::optional<const char*>(it->second.c_str());

    return std::nullopt;
}

void CRequest::SetUid(const int64_t uid)
{
    this->uid = uid;
}

///////////////////////////////////////////////////////////////CRequest////////////////////////////////////////////////////////
std::optional<int64_t> CRequest::GetUid() const
{
    return uid;
}

std::optional<std::string_view> CRequest::GetQueryByKey(const char* key) const
{
    if (!key || !qheaders)
        return std::nullopt;
    auto val = evhttp_find_header(qheaders.value(), key);
    return val ? std::optional(val) : std::nullopt;
}

std::optional<std::string_view> CRequest::GetHeaderByKey(const char* key) const
{
    if (!key || !headers)
        return std::nullopt;
    auto val = evhttp_find_header(headers.value(), key);
    return val ? std::optional(val) : std::nullopt;
}

std::optional<std::string_view> CRequest::GetRequestPath() const
{
    return path;
}

std::optional<std::string_view> CRequest::GetRequestBody() const
{
    return data;
}

///////////////////////////////////////////////////////////////CResponse////////////////////////////////////////////////////////
std::optional<std::string_view> CResponse::GetQueryByKey(const char* key) const
{
    if (!key || !qheaders)
        return std::nullopt;
    auto val = evhttp_find_header(qheaders.value(), key);
    return val ? std::optional(val) : std::nullopt;
}

std::optional<std::string_view> CResponse::GetHeaderByKey(const char* key) const
{
    if (!key || !headers)
        return std::nullopt;
    auto val = evhttp_find_header(headers.value(), key);
    return val ? std::optional(val) : std::nullopt;
}

std::optional<ghttp::HttpStatusCode> CResponse::GetResponseStatus() const
{
    return rspstatus;
}

std::optional<std::string_view> CResponse::GetResponseBody() const
{
    return rspdata;
}

bool CResponse::Response(const std::pair<ghttp::HttpStatusCode, std::string>& res) const
{
    auto evb = evbuffer_new();
    evbuffer_add(evb, res.second.data(), res.second.length());

    evhttp_send_reply(req, (int32_t)res.first, ghttp::HttpReason(res.first).value_or(""), evb);

    if (evb)
        evbuffer_free(evb);
    return true;
}

void CResponse::AddHeader(const std::string& key, const std::string& val)
{
    evhttp_add_header(evhttp_request_get_output_headers(req), key.c_str(), val.c_str());
}

}

CHTTPServer::~CHTTPServer()
{
    destroy();
}

CHTTPServer::CHTTPServer(CHTTPServer&& rhs)
{
}

void CHTTPServer::destroy()
{
    if (m_http) {
        evhttp_free(m_http);
        m_http = nullptr;
    }
}

struct bufferevent* CHTTPServer::onConnected(struct event_base* base, void* arg)
{
    return CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, *((bool*)arg) ? CSSLContex::Instance().CreateOneSSL() : nullptr, true);
}

static void onDefault(struct evhttp_request* req, void* arg)
{
    if (!req)
        return;

    // decode http or https
    auto is_https = false;
    auto evconn = evhttp_request_get_connection(req);
    if (evconn) {
        if (auto bv = evhttp_connection_get_bufferevent(evconn); bv && bufferevent_openssl_get_ssl(bv)) {
            is_https = true;
        }
    }
    auto uri = evhttp_request_get_uri(req);
    if (uri && !is_https && MYARGS.IsForceHttps && MYARGS.IsForceHttps.value() && MYARGS.RedirectUrl && !MYARGS.RedirectUrl.value().empty()) {
        auto loc = CStringTool::Format("%s%s", MYARGS.RedirectUrl.value().c_str(), uri);
        evhttp_add_header(evhttp_request_get_output_headers(req), "Location", loc.c_str());
        auto code = MYARGS.RedirectStatus.value_or((uint16_t)ghttp::HttpStatusCode::MOVEPERM);
        evhttp_send_reply(req, code, ghttp::HttpReason(ghttp::HttpStatusCode(code)).value_or(""), nullptr);
        return;
    }

    auto deuri = (evhttp_uri*)evhttp_request_get_evhttp_uri(req);
    if (!deuri) {
        evhttp_send_error(req, (int32_t)(ghttp::HttpStatusCode::BADREQUEST), 0);
        return;
    }
    auto pathuri = evhttp_uri_get_path(deuri);
    if (!pathuri) {
        evhttp_send_error(req, (int32_t)(ghttp::HttpStatusCode::BADREQUEST), 0);
        return;
    }
    fs::path f = MYARGS.WebRootDir.value_or(fs::current_path()) + pathuri;
    if (!fs::exists(f)) {
        evhttp_send_error(req, (int32_t)(ghttp::HttpStatusCode::NOTFOUND), 0);
        return;
    }
    auto it = MIME.find(f.extension());
    if (it == std::end(MIME)) {
        evhttp_send_error(req, (int32_t)(ghttp::HttpStatusCode::NOTFOUND), 0);
        return;
    }
    auto hfd = fopen(f.c_str(), "r");
    if (hfd) {
        auto fd = fileno(hfd);
        auto& contentype = it->second;
        evhttp_add_header(evhttp_request_get_output_headers(req), "content-type", contentype.c_str());
        if (MYARGS.IsAllowOrigin && MYARGS.IsAllowOrigin.value())
            evhttp_add_header(evhttp_request_get_output_headers(req), "access-control-allow-origin", "*");
        auto evb = evhttp_request_get_output_buffer(req);
        evbuffer_add_file(evb, fd, 0, fs::file_size(f));
        evhttp_send_reply(req, (int32_t)(ghttp::HttpStatusCode::OK), ghttp::HttpReason(ghttp::HttpStatusCode::OK).value_or(""), evb);
        return;
    }
    evhttp_send_error(req, (int32_t)(ghttp::HttpStatusCode::BADREQUEST), 0);
}

void CHTTPServer::onBindCallback(evhttp_request* req, void* arg)
{
    if (!req) {
        return;
    }

    auto errcode = ghttp::HttpStatusCode::BADREQUEST;
    auto filters = (FilterData*)arg;
    if (filters && filters->cb) {
        uint32_t cmd = evhttp_request_get_command(req);
        // filter
        if (0 == ((uint32_t)(filters->cmd) & cmd)) {
            errcode = ghttp::HttpStatusCode::BADMETHOD;
            goto err;
        }
        // decode headers
        auto headers = evhttp_request_get_input_headers(req);
        auto deuri = (evhttp_uri*)evhttp_request_get_evhttp_uri(req);
        if (!deuri) {
            errcode = ghttp::HttpStatusCode::BADREQUEST;
            goto err;
        }
        auto path = evhttp_uri_get_path(deuri);
        if (!path) {
            errcode = ghttp::HttpStatusCode::BADREQUEST;
            goto err;
        }
        // decode query string to headers
        evkeyvalq qheaders = { 0 };
        if (auto qstr = evhttp_uri_get_query(deuri); qstr) {
            if (evhttp_parse_query_str(qstr, &qheaders) < 0) {
                errcode = ghttp::HttpStatusCode::BADREQUEST;
                goto err;
            }
        }
        // decode the payload
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        char* data = (char*)evbuffer_pullup(buf, -1);
        int32_t dlen = evbuffer_get_length(buf);

        ghttp::CRequest request;
        request.req = req;
        request.qheaders = &qheaders;
        request.headers = headers;
        request.path = std::string_view(path);
        request.data = std::string_view(data, dlen);
        ghttp::CResponse rsp;
        rsp.req = req;
        rsp.conn = evhttp_request_get_connection(req);
        auto r = filters->self->EmitEvent("start", &request, &rsp);
        if (r) {
            evhttp_send_reply(req, (int32_t)r.value().first, ghttp::HttpReason(r.value().first).value_or(""), nullptr);
            return;
        }
        // decode http or https
        auto is_https = false;
        auto evconn = evhttp_request_get_connection(req);
        if (evconn) {
            if (auto bv = evhttp_connection_get_bufferevent(evconn); bv && bufferevent_openssl_get_ssl(bv)) {
                is_https = true;
            }
        }
        auto uri = evhttp_request_get_uri(req);
        if (uri && !is_https && MYARGS.IsForceHttps && MYARGS.IsForceHttps.value() && MYARGS.RedirectUrl && !MYARGS.RedirectUrl.value().empty()) {
            auto loc = CStringTool::Format("%s%s", MYARGS.RedirectUrl.value().c_str(), uri);
            evhttp_add_header(evhttp_request_get_output_headers(req), "Location", loc.c_str());
            auto code = MYARGS.RedirectStatus.value_or((uint16_t)ghttp::HttpStatusCode::MOVEPERM);
            evhttp_send_reply(req, code, ghttp::HttpReason(ghttp::HttpStatusCode(code)).value_or(""), nullptr);
            return;
        }

        if (MYARGS.IsAllowOrigin && MYARGS.IsAllowOrigin.value())
            evhttp_add_header(evhttp_request_get_output_headers(req), "access-control-allow-origin", "*");
        for (auto& [k, v] : filters->h) {
            evhttp_add_header(evhttp_request_get_output_headers(req), k.c_str(), v.c_str());
        }

        auto res = filters->cb(&request, &rsp);

        if (res) {
            filters->self->EmitEvent("finish", &request, &rsp);
        }

        return;
    }
err:
    evhttp_send_error(req, (int32_t)errcode, 0);
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
    filter.self = this;
    m_callbacks[path] = filter;
    evhttp_set_cb(m_http, path.c_str(), onBindCallback, &m_callbacks[path]);
    return true;
}

bool CHTTPServer::Register(const std::string path, ghttp::HttpMethod cmd, ghttp::HttpReqRspCallback cb)
{
    FilterData filter = { .cmd = cmd, .cb = cb, .self = this };
    return Register(path, filter);
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

bool CHTTPServer::ListenAndServe(std::string host)
{
    CheckCondition(!host.empty(), true);
    CheckCondition(!m_http, true);

    m_http = evhttp_new(CWorker::MAIN_CONTEX->Base());

    auto [type, hostname, port] = CConnection::SplitUri(host);
    if (type == "https")
        m_ishttps = true;

    struct addrinfo hints;
    struct addrinfo *result = nullptr, *rp = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int32_t ret = -1;
    if (hostname == "*") {
        ret = evutil_getaddrinfo(nullptr, port.c_str(), &hints, &result);
    } else {
        ret = evutil_getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);
    }
    if (ret != 0) {
        return false;
    }
    int32_t listenfd = -1;
    for (rp = result; rp != nullptr; rp = rp->ai_next) {
        listenfd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listenfd == -1)
            continue;

        if (!setOption(listenfd)) {
            destroy();
            continue;
        }
        break;
    }
    if (nullptr == rp) {
        freeaddrinfo(result);
        return false;
    }

    if (::bind(listenfd, rp->ai_addr, rp->ai_addrlen) < 0) {
        destroy();
        CERROR("CTX:%s bind:%s", MYARGS.CTXID.c_str(), strerror(errno));
        return false;
    }

    if (::listen(listenfd, BACKLOG_SIZE) < 0) {
        destroy();
        return false;
    }

    auto handle = evhttp_accept_socket_with_handle(m_http, listenfd);
    if (!handle) {
        destroy();
        return false;
    }
    evhttp_set_bevcb(m_http, onConnected, &m_ishttps);
    evhttp_set_gencb(m_http, onDefault, this);
    evhttp_set_default_content_type(m_http, "application/json; charset=utf-8");
    freeaddrinfo(result);

    return true;
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
bool CHTTPClient::Get(std::string_view url, ghttp::HttpRspCallback cb)
{
    return (bool)Emit(url, cb, ghttp::HttpMethod::GET, std::nullopt);
}

bool CHTTPClient::Post(std::string_view url, ghttp::HttpRspCallback cb, std::optional<std::string_view> data)
{
    return (bool)Emit(url, cb, ghttp::HttpMethod::POST, data);
}

std::optional<CHTTPClient*> CHTTPClient::Emit(std::string_view url,
    ghttp::HttpRspCallback cb,
    ghttp::HttpMethod cmd,
    std::optional<std::string_view> data,
    std::map<std::string, std::string> headers)
{
    CHTTPClient* self = CNEW CHTTPClient();
    self->m_callback = cb;
    if (!self->request(url.data(), (uint32_t)cmd, std::move(data), headers, CHTTPClient::delegateCallback)) {
        auto dv = std::string(url.data(), url.length());
        CERROR("request %s", dv.c_str());
        CDEL(self);
        return std::nullopt;
    }
    return self;
}

void CHTTPClient::delegateCallback(struct evhttp_request* req, void* arg)
{
    CHTTPClient* self = (CHTTPClient*)arg;
    ghttp::CResponse rsp;
    rsp.req = req;
    rsp.conn = self->m_evconn;
    rsp.rspstatus = ghttp::HttpStatusCode::BADREQUEST;
    rsp.rspdata = ghttp::HttpReason(ghttp::HttpStatusCode::BADREQUEST).value_or("");
    while (req) {
        // decode headers
        auto headers = evhttp_request_get_input_headers(req);
        // decode the payload
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        char* data = (char*)evbuffer_pullup(buf, -1);
        auto dlen = evbuffer_get_length(buf);

        rsp.headers = headers;
        rsp.rspstatus = ghttp::HttpStatusCode(evhttp_request_get_response_code(req));
        if (data)
            rsp.rspdata = std::string_view(data, dlen);
        break;
    }

    if (self->m_callback)
        self->m_callback(&rsp);
    CDEL(self);
}

bool CHTTPClient::request(const char* url,
    const uint32_t method,
    std::optional<std::string_view> body,
    std::optional<std::map<std::string, std::string>> headers,
    HttpHandleCallbackFunc cb)
{
    bool ret = true;
    const char *scheme = nullptr, *host = nullptr, *path = nullptr, *query = nullptr;
    char uri[2048];
    int port = -1, r = -1;
    bool is_https = false;
    SSL* ssl = nullptr;
    struct evkeyvalq* output_headers = nullptr;
    size_t uri_len = 0;
    struct evhttp_uri* http_uri = nullptr;
    struct evhttp_request* req = nullptr;
    struct bufferevent* bev = nullptr;

    // parse a valid uri from url
    http_uri = evhttp_uri_parse(url);
    if (!http_uri) {
        CERROR("evhttp_uri_parse %s", url);
        goto ERRLABLE;
    }

    scheme = evhttp_uri_get_scheme(http_uri);
    if (!scheme || (strcasecmp(scheme, "https") != 0 && strcasecmp(scheme, "http") != 0 && strcasecmp(scheme, "ws") != 0 && strcasecmp(scheme, "wss") != 0)) {
        CERROR("evhttp_uri_get_scheme");
        goto ERRLABLE;
    }

    host = evhttp_uri_get_host(http_uri);
    if (!host) {
        CERROR("evhttp_uri_get_host");
        goto ERRLABLE;
    }

    port = evhttp_uri_get_port(http_uri);
    if (port == -1) {
        port = (strcasecmp(scheme, "http") == 0 || strcasecmp(scheme, "ws") == 0) ? 80 : 443;
    }

    path = evhttp_uri_get_path(http_uri);
    if (strlen(path) == 0) {
        path = "/";
    }

    query = evhttp_uri_get_query(http_uri);
    if (query == NULL) {
        snprintf(uri, sizeof(uri) - 1, "%s", path);
    } else {
        snprintf(uri, sizeof(uri) - 1, "%s?%s", path, query);
    }
    uri[sizeof(uri) - 1] = '\0';

    // construct a request
    req = evhttp_request_new(cb, this);
    if (!req) {
        CERROR("evhttp_request_new");
        goto ERRLABLE;
    }
    output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Connection", "keep-alive");
    if (headers) {
        for_each(headers.value().begin(),
            headers.value().end(),
            [output_headers](const std::pair<std::string, std::string> val) { evhttp_add_header(output_headers, val.first.c_str(), val.second.c_str()); });
    }
    if (body) {
        struct evbuffer* evout = evhttp_request_get_output_buffer(req);
        evbuffer_add(evout, body.value().data(), body.value().size());
    }

    // create a connection by scheme
    if (strcasecmp(scheme, "http") == 0 || strcasecmp(scheme, "ws") == 0) {
        bev = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, this, nullptr, false);
    } else if (strcasecmp(scheme, "https") == 0 || strcasecmp(scheme, "wss") == 0) {
        ssl = CSSLContex::Instance().CreateOneSSL();
        if (!ssl) {
            CERROR("CreateOneSSL");
            goto ERRLABLE;
        }

        SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (void*)host);

        bev = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, this, ssl, false);
        bufferevent_openssl_set_allow_dirty_shutdown(bev, 1);
        is_https = true;
    }

    if (!bev) {
        CERROR("Bvsocket");
        goto ERRLABLE;
    }

    if (!m_evconn) {
        m_evconn = CWorker::MAIN_CONTEX->CreateHttpConnection(bev, host, port);
        if (!m_evconn) {
            CDEBUG("CreateHttpConnection");
            goto ERRLABLE;
        }
        evhttp_connection_set_retries(m_evconn, 3);
        evhttp_connection_set_timeout(m_evconn, 30);
        evhttp_connection_free_on_completion(m_evconn);
    }

    // make a http request
    r = evhttp_make_request(m_evconn, req, (evhttp_cmd_type)method, uri);
    if (r != 0) {
        CERROR("evhttp_make_request");
        goto ERRLABLE;
    }
    goto CLEANUPLABLE;

ERRLABLE:
    ret = false;
    if (req)
        evhttp_request_free(req);
    if (m_evconn) {
        evhttp_connection_free(m_evconn);
        m_evconn = nullptr;
    }
CLEANUPLABLE:
    if (http_uri)
        evhttp_uri_free(http_uri);
    if (!is_https && ssl)
        SSL_free(ssl);

    return ret;
}

/////////////////////////////////////////////////////////////////////////CWebSocket/////////////////////////////////////////////////////////////
CWebSocket* CWebSocket::Upgrade(ghttp::CResponse* rsp, Callback rdfunc)
{
    auto evconn = rsp->Conn();
    if (!evconn)
        return nullptr;
    auto ws = CNEW CWebSocket(evconn);
    if (auto bv = evhttp_connection_get_bufferevent(evconn); bv) {
        bufferevent_setcb(bv, onRead, onWrite, onError, ws);
    }
    ws->m_rdfunc = std::move(rdfunc);
    ws->m_parser.SetStatus(CWSParser::WS_STATUS::WS_CONNECTED);
    CINFO("CWebSocket::Upgrade");
    return ws;
}

CWebSocket::CWebSocket(evhttp_connection* evconn)
    : m_evconn(evconn)
{
}

CWebSocket::~CWebSocket()
{
    if (m_evconn) {
        evhttp_connection_free(m_evconn);
        m_evconn = nullptr;
    }
}

bool CWebSocket::Connect(const std::string& url, Callback cb)
{
    auto [_, host, port] = CConnection::SplitUri(url);
    auto key = CWSParser::GenSecWebSocketAccept(std::to_string(time(nullptr)));
    return (bool)CHTTPClient::Emit(
        url,
        [key, cb](ghttp::CResponse* rsp) {
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
            auto bv = evhttp_connection_get_bufferevent(m_evconn);
            struct evbuffer* output = bufferevent_get_output(bv);
            return -1 != evbuffer_add(output, v.data(), v.length());
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
        CINFO("%s is closed", __FUNCTION__);
        CDEL(self);
    }
}

void CWebSocket::onError(struct bufferevent* bev, short which, void* arg)
{
    CINFO("%s", __FUNCTION__);
    CWebSocket* self = (CWebSocket*)arg;
    CDEL(self);
}
/////////////////////////////////////////////////////////////////////////CHTTP2Client/////////////////////////////////////////////////////////////
#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

#define MAKE_NV(NAME, VALUE)                                                  \
    {                                                                         \
        (uint8_t*)NAME, (uint8_t*)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1, \
            NGHTTP2_NV_FLAG_NONE                                              \
    }

CHTTP2SessionData::CHTTP2SessionData(struct bufferevent* bev)
    : m_bev(bev)
{
    CINFO("%s", __FUNCTION__);
}

CHTTP2SessionData::~CHTTP2SessionData()
{
    CINFO("%s", __FUNCTION__);
    if (m_bev) {
        SSL* ssl = bufferevent_openssl_get_ssl(m_bev);
        if (ssl)
            SSL_shutdown(ssl);
        bufferevent_free(m_bev);
    }
    if (m_session)
        nghttp2_session_del(m_session);
    for (auto& v : m_streams) {
        CDEL(v);
    }
    m_streams.clear();
}

CHTTP2SessionData::CStreamData* CHTTP2SessionData::AddStreamData(const int32_t streamid)
{
    CINFO("%s", __FUNCTION__);
    auto s = CNEW CStreamData { .StreamId = streamid };
    m_streams.push_back(s);
    return m_streams.back();
}

void CHTTP2SessionData::RemoveStreamData(CStreamData* s)
{
    CINFO("%s", __FUNCTION__);
    for (auto it = std::begin(m_streams); it != std::end(m_streams); ++it) {
        if ((*it) == s) {
            m_streams.erase(it);
            break;
        }
    }
}

ssize_t CHTTP2SessionData::sendCallback(nghttp2_session* session, const uint8_t* data, size_t length, int flags, void* user_data)
{
    CHTTP2SessionData* session_data = (CHTTP2SessionData*)user_data;
    struct bufferevent* bev = session_data->m_bev;
    (void)session;
    (void)flags;

    /* Avoid excessive buffering in server side. */
    if (evbuffer_get_length(bufferevent_get_output(bev)) >= 0xFFFFF) {
        return NGHTTP2_ERR_WOULDBLOCK;
    }
    CINFO("%s,%s,%lu", __FUNCTION__, data, length);
    bufferevent_write(bev, data, length);
    return (ssize_t)length;
}

int CHTTP2SessionData::onRequestRecv(nghttp2_session* session, CHTTP2SessionData* session_data, CHTTP2SessionData::CStreamData* stream_data)
{
    CINFO("%s", __FUNCTION__);
    // int fd;
    // char* rel_path;

    // if (!stream_data->request_path) {
    //     if (error_reply(session, stream_data) != 0) {
    //         return NGHTTP2_ERR_CALLBACK_FAILURE;
    //     }
    //     return 0;
    // }
    // for (rel_path = stream_data->request_path; *rel_path == '/'; ++rel_path)
    //     ;
    // fd = open(rel_path, O_RDONLY);
    // if (fd == -1) {
    //     if (error_reply(session, stream_data) != 0) {
    //         return NGHTTP2_ERR_CALLBACK_FAILURE;
    //     }
    //     return 0;
    // }
    // stream_data->fd = fd;

    if (!session_data->SendResponse(stream_data, { { ":status", "200" } }, "OK")) {
        return NGHTTP2_ERR_CALLBACK_FAILURE;
    }
    return 0;
}

int CHTTP2SessionData::onFrameRecvCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data)
{
    CINFO("%s,%d,%u,0x%x", __FUNCTION__, frame->hd.stream_id, frame->hd.type, frame->hd.flags);
    CHTTP2SessionData* session_data = (CHTTP2SessionData*)user_data;
    switch (frame->hd.type) {
    case NGHTTP2_DATA:
    case NGHTTP2_HEADERS:
        /* Check that the client request has finished */
        if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) {
            CStreamData* stream_data = (CStreamData*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
            /* For DATA and HEADERS frame, this callback may be called after
               onstreamclosecallback. Check that stream still alive. */
            if (!stream_data) {
                return 0;
            }
            return onRequestRecv(session, session_data, stream_data);
        }
        break;
    default:
        break;
    }
    return 0;
}

int CHTTP2SessionData::onStreamCloseCallback(nghttp2_session* session, int32_t stream_id, uint32_t error_code, void* user_data)
{
    CINFO("%s", __FUNCTION__);
    CHTTP2SessionData* session_data = (CHTTP2SessionData*)user_data;
    (void)error_code;

    CStreamData* stream_data = (CStreamData*)nghttp2_session_get_stream_user_data(session, stream_id);
    if (!stream_data) {
        return 0;
    }
    session_data->RemoveStreamData(stream_data);
    return 0;
}

/* nghttp2_on_header_callback: Called when nghttp2 library emits
   single header name/value pair. */
int CHTTP2SessionData::onHeaderCallback(nghttp2_session* session,
    const nghttp2_frame* frame, const uint8_t* name,
    size_t namelen, const uint8_t* value,
    size_t valuelen, uint8_t flags, void* user_data)
{
    CINFO("%s,%d,%s,%s", __FUNCTION__, frame->headers.hd.stream_id, name, value);
    const char PATH[] = ":path";
    (void)flags;
    (void)user_data;

    switch (frame->hd.type) {
    case NGHTTP2_HEADERS:
        if (frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
            break;
        }
        CStreamData* stream_data = (CStreamData*)nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
        if (!stream_data) {
            break;
        }
        if (namelen == sizeof(PATH) - 1 && memcmp(PATH, name, namelen) == 0) {
            size_t j;
            for (j = 0; j < valuelen && value[j] != '?'; ++j)
                ;
            stream_data->RequestPath = std::string((char*)value, j);
        }
        break;
    }
    return 0;
}

int CHTTP2SessionData::onBeginHeadersCallback(nghttp2_session* session, const nghttp2_frame* frame, void* user_data)
{
    CINFO("%s,%d,%u", __FUNCTION__, frame->hd.stream_id, frame->hd.type);
    CHTTP2SessionData* session_data = (CHTTP2SessionData*)user_data;

    if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST) {
        return 0;
    }
    CStreamData* stream_data = session_data->AddStreamData(frame->hd.stream_id);
    nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, stream_data);
    return 0;
}

bool CHTTP2SessionData::sendServerConnectionHeader()
{
    nghttp2_settings_entry iv[1] = {
        { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100 }
    };

    int32_t rv = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, ARRLEN(iv));
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

bool CHTTP2SessionData::SessionSend()
{
    int32_t rv = nghttp2_session_send(m_session);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

bool CHTTP2SessionData::SendResponse(const CStreamData* stream, std::unordered_map<std::string, std::string> header, const std::string& data)
{
    int rv;
    nghttp2_data_provider data_prd;
    data_prd.source.ptr = (void*)data.data();

    std::vector<nghttp2_nv> hdrs;
    for (auto& [k, v] : header) {
        hdrs.push_back(MAKE_NV(k.c_str(), v.c_str()));
    }
    CINFO("%s,%d", __FUNCTION__, stream->StreamId);
    rv = nghttp2_submit_response(m_session, stream->StreamId, hdrs.data(), hdrs.size(), &data_prd);
    if (rv != 0) {
        CERROR("Fatal error: %s", nghttp2_strerror(rv));
        return false;
    }
    return true;
}

size_t CHTTP2SessionData::SessionReceive(std::string_view data)
{
    size_t readlen = nghttp2_session_mem_recv(m_session, (const uint8_t*)data.data(), data.length());
    return readlen;
}

CHTTP2SessionData* CHTTP2SessionData::InitNghttp2SessionData(struct bufferevent* bev)
{
    CINFO("%s", __FUNCTION__);
    CHTTP2SessionData* h2 = CNEW CHTTP2SessionData(bev);
    nghttp2_session_callbacks* callbacks;

    nghttp2_session_callbacks_new(&callbacks);

    nghttp2_session_callbacks_set_send_callback(callbacks, sendCallback);

    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, onFrameRecvCallback);

    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, onStreamCloseCallback);

    nghttp2_session_callbacks_set_on_header_callback(callbacks, onHeaderCallback);

    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, onBeginHeadersCallback);

    nghttp2_session_server_new(&h2->m_session, callbacks, h2);

    nghttp2_session_callbacks_del(callbacks);

    if (!h2->sendServerConnectionHeader() || !h2->SessionSend()) {
        CDEL(h2);
    }

    return h2;
}
NAMESPACE_FRAMEWORK_END