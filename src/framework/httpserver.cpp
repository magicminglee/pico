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

static const std::map<const uint32_t, std::string> RESPSTR = {
    { HTTP_OK, "OK" },
    { HTTP_NOCONTENT, "NO CONTENT" },
    { HTTP_MOVEPERM, "MOVED PERMANENTLY" },
    { HTTP_MOVETEMP, "MOVED TEMPORARILY" },
    { HTTP_NOTMODIFIED, "NOT MODIFIED" },
    { HTTP_BADREQUEST, "INVALID REQUEST" },
    { HTTP_NOTFOUND, "NOT FOUND" },
    { HTTP_BADMETHOD, "METHOD NOT ALLOWED" },
    { HTTP_INTERNAL, "INTERNAL ERROR" },
    { HTTP_SERVUNAVAIL, "NOT AVAILABLE" },
    { HTTP_UNAUTHORIZED, "UNAUTHORIZED" },
};

std::optional<const char*> CHTTPServer::HttpReason(const uint32_t code)
{
    std::optional<const char*> reason;
    if (auto it = RESPSTR.find(code); it != std::end(RESPSTR))
        reason = it->second.c_str();

    return reason;
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
    bool* ishttps = (bool*)arg;
    return CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, *ishttps ? CSSLContex::Instance().CreateOneSSL() : nullptr, true);
}

std::optional<const char*> CHTTPServer::GetValueByKey(evkeyvalq* headers, const char* key)
{
    if (!headers || !key)
        return std::nullopt;
    auto val = evhttp_find_header(headers, key);
    return val ? std::optional(val) : std::nullopt;
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
        auto code = MYARGS.RedirectStatus.value_or(HTTP_MOVEPERM);
        evhttp_send_reply(req, code, CHTTPServer::HttpReason(code).value_or(""), nullptr);
        return;
    }

    auto deuri = (evhttp_uri*)evhttp_request_get_evhttp_uri(req);
    if (!deuri) {
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }
    auto pathuri = evhttp_uri_get_path(deuri);
    if (!pathuri) {
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }
    fs::path f = MYARGS.WebRootDir.value_or(fs::current_path()) + pathuri;
    if (!fs::exists(f)) {
        evhttp_send_error(req, HTTP_NOTFOUND, 0);
        return;
    }
    auto it = MIME.find(f.extension());
    if (it == std::end(MIME)) {
        evhttp_send_error(req, HTTP_NOTFOUND, 0);
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
        evhttp_send_reply(req, HTTP_OK, CHTTPServer::HttpReason(HTTP_OK).value_or(""), evb);
        return;
    }
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
}

void CHTTPServer::onBindCallback(evhttp_request* req, void* arg)
{
    if (!req)
        return;

    auto filters = (FilterData*)arg;
    if (filters && filters->cb) {
        uint32_t cmd = evhttp_request_get_command(req);
        // filter
        if (0 == (filters->cmd & cmd)) {
            evhttp_send_error(req, HTTP_BADMETHOD, 0);
            return;
        }
        // decode headers
        auto headers = evhttp_request_get_input_headers(req);
        auto deuri = (evhttp_uri*)evhttp_request_get_evhttp_uri(req);
        if (!deuri) {
            evhttp_send_error(req, HTTP_BADREQUEST, 0);
            return;
        }
        auto path = evhttp_uri_get_path(deuri);
        if (!path) {
            evhttp_send_error(req, HTTP_BADREQUEST, 0);
            return;
        }
        // decode query string to headers
        evkeyvalq qheaders = { 0 };
        if (auto qstr = evhttp_uri_get_query(deuri); qstr) {
            if (evhttp_parse_query_str(qstr, &qheaders) < 0) {
                evhttp_send_error(req, HTTP_BADREQUEST, 0);
                return;
            }
        }
        // decode the payload
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        char* data = (char*)evbuffer_pullup(buf, -1);
        int32_t dlen = evbuffer_get_length(buf);

        RequestData rd = { .qheaders = &qheaders, .headers = headers, .path = path };
        auto r = filters->self->EmitEvent("start", rd);
        if (r) {
            evhttp_send_reply(req, r.value().first, HttpReason(r.value().first).value_or(""), nullptr);
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
            auto code = MYARGS.RedirectStatus.value_or(HTTP_MOVEPERM);
            evhttp_send_reply(req, code, HttpReason(code).value_or(""), nullptr);
            return;
        }

        auto res = filters->cb(&qheaders, headers, std::string(data, dlen));

        auto evb = evbuffer_new();
        evbuffer_add_printf(evb, "%s", res.second.data());
        for (auto& [k, v] : filters->h) {
            evhttp_add_header(evhttp_request_get_output_headers(req), k.c_str(), v.c_str());
        }
        if (MYARGS.IsAllowOrigin && MYARGS.IsAllowOrigin.value())
            evhttp_add_header(evhttp_request_get_output_headers(req), "access-control-allow-origin", "*");

        evhttp_send_reply(req, res.first, HttpReason(res.first).value_or(""), evb);

        if (evb)
            evbuffer_free(evb);
        return;
    }
    evhttp_send_error(req, HTTP_INTERNAL, 0);
}

bool CHTTPServer::Register(const std::string path, FilterData filter)
{
    filter.self = this;
    m_callbacks[path] = filter;
    evhttp_set_cb(m_http, path.c_str(), onBindCallback, &m_callbacks[path]);
    return true;
}

bool CHTTPServer::RegEvent(std::string ename, std::function<HttpEventType> cb)
{
    m_ev_callbacks[ename] = std::move(cb);
    return true;
}

std::optional<std::pair<uint32_t, std::string>> CHTTPServer::EmitEvent(std::string ename, const RequestData& rd)
{
    if (auto it = m_ev_callbacks.find(ename); it != std::end(m_ev_callbacks)) {
        return (it->second)(rd);
    }
    return std::nullopt;
}

bool CHTTPServer::Init(std::string host)
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

NAMESPACE_FRAMEWORK_END