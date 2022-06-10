#include "httpserver.hpp"
#include "connection.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "worker.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

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
    CHTTPServer* self = (CHTTPServer*)arg;
    bufferevent* bv = nullptr;
    if (self->m_ishttps) {
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, CSSLContex::Instance().CreateOneSSL(), true);
    } else {
        bv = CWorker::MAIN_CONTEX->Bvsocket(-1, nullptr, nullptr, nullptr, nullptr, nullptr, true);
    }

    return bv;
}

std::optional<const char*> CHTTPServer::GetValueByKey(evkeyvalq* headers, const char* key)
{
    if (!headers || !key)
        return std::nullopt;
    auto val = evhttp_find_header(headers, key);
    return val ? std::optional(val) : std::nullopt;
}

static const std::map<std::string, std::string> SMime = {
    { "ai", "application/postscript; charset=UTF-8" },
    { "aif", "audio/x-aiff; charset=UTF-8" },
    { "aifc", "audio/x-aiff; charset=UTF-8" },
    { "aiff", "audio/x-aiff; charset=UTF-8" },
    { "arj", "application/x-arj-compressed; charset=UTF-8" },
    { "asc", "text/plain; charset=UTF-8" },
    { "asf", "video/x-ms-asf; charset=UTF-8" },
    { "asx", "video/x-ms-asx; charset=UTF-8" },
    { "au", "audio/ulaw; charset=UTF-8" },
    { "avi", "video/x-msvideo; charset=UTF-8" },
    { "bat", "application/x-msdos-program; charset=UTF-8" },
    { "bcpio", "application/x-bcpio; charset=UTF-8" },
    { "bin", "application/octet-stream; charset=UTF-8" },
    { "c", "text/plain; charset=UTF-8" },
    { "cc", "text/plain; charset=UTF-8" },
    { "ccad", "application/clariscad; charset=UTF-8" },
    { "cdf", "application/x-netcdf; charset=UTF-8" },
    { "class", "application/octet-stream; charset=UTF-8" },
    { "cod", "application/vnd.rim.cod; charset=UTF-8" },
    { "com", "application/x-msdos-program; charset=UTF-8" },
    { "cpio", "application/x-cpio; charset=UTF-8" },
    { "cpt", "application/mac-compactpro; charset=UTF-8" },
    { "csh", "application/x-csh; charset=UTF-8" },
    { "css", "text/css; charset=UTF-8" },
    { "dcr", "application/x-director; charset=UTF-8" },
    { "deb", "application/x-debian-package; charset=UTF-8" },
    { "dir", "application/x-director; charset=UTF-8" },
    { "dl", "video/dl; charset=UTF-8" },
    { "dms", "application/octet-stream; charset=UTF-8" },
    { "doc", "application/msword; charset=UTF-8" },
    { "drw", "application/drafting; charset=UTF-8" },
    { "dvi", "application/x-dvi; charset=UTF-8" },
    { "dwg", "application/acad; charset=UTF-8" },
    { "dxf", "application/dxf; charset=UTF-8" },
    { "dxr", "application/x-director; charset=UTF-8" },
    { "eps", "application/postscript; charset=UTF-8" },
    { "etx", "text/x-setext; charset=UTF-8" },
    { "exe", "application/octet-stream; charset=UTF-8" },
    { "ez", "application/andrew-inset; charset=UTF-8" },
    { "f", "text/plain; charset=UTF-8" },
    { "f90", "text/plain; charset=UTF-8" },
    { "fli", "video/fli; charset=UTF-8" },
    { "flv", "video/flv; charset=UTF-8" },
    { "gif", "image/gif; charset=UTF-8" },
    { "gl", "video/gl; charset=UTF-8" },
    { "gtar", "application/x-gtar; charset=UTF-8" },
    { "gz", "application/x-gzip; charset=UTF-8" },
    { "hdf", "application/x-hdf; charset=UTF-8" },
    { "hh", "text/plain; charset=UTF-8" },
    { "hqx", "application/mac-binhex40; charset=UTF-8" },
    { "h", "text/plain; charset=UTF-8" },
    { "htm", "text/html; charset=UTF-8" },
    { "html", "text/html; charset=UTF-8" },
    { "ice", "x-conference/x-cooltalk; charset=UTF-8" },
    { "ief", "image/ief; charset=UTF-8" },
    { "iges", "model/iges; charset=UTF-8" },
    { "igs", "model/iges; charset=UTF-8" },
    { "ips", "application/x-ipscript; charset=UTF-8" },
    { "ipx", "application/x-ipix; charset=UTF-8" },
    { "jad", "text/vnd.sun.j2me.app-descriptor; charset=UTF-8" },
    { "jar", "application/java-archive; charset=UTF-8" },
    { "jpeg", "image/jpeg; charset=UTF-8" },
    { "jpe", "image/jpeg; charset=UTF-8" },
    { "jpg", "image/jpeg; charset=UTF-8" },
    { "js", "text/x-javascript; charset=UTF-8" },
    /* application/javascript is commonly used for JS, but the
    ** HTML spec says text/javascript is correct:
    ** https://html.spec.whatwg.org/multipage/scripting.html
    ** #scriptingLanguages:javascript-mime-type */
    { "json", "application/json; charset=UTF-8" },
    { "kar", "audio/midi; charset=UTF-8" },
    { "latex", "application/x-latex; charset=UTF-8" },
    { "lha", "application/octet-stream; charset=UTF-8" },
    { "lsp", "application/x-lisp; charset=UTF-8" },
    { "lzh", "application/octet-stream; charset=UTF-8" },
    { "m", "text/plain; charset=UTF-8" },
    { "m3u", "audio/x-mpegurl; charset=UTF-8" },
    { "man", "application/x-troff-man; charset=UTF-8" },
    { "me", "application/x-troff-me; charset=UTF-8" },
    { "mesh", "model/mesh; charset=UTF-8" },
    { "mid", "audio/midi; charset=UTF-8" },
    { "midi", "audio/midi; charset=UTF-8" },
    { "mif", "application/x-mif; charset=UTF-8" },
    { "mime", "www/mime; charset=UTF-8" },
    { "mjs", "text/javascript" /*EM6 modules*/ },
    { "movie", "video/x-sgi-movie; charset=UTF-8" },
    { "mov", "video/quicktime; charset=UTF-8" },
    { "mp2", "audio/mpeg; charset=UTF-8" },
    { "mp2", "video/mpeg; charset=UTF-8" },
    { "mp3", "audio/mpeg; charset=UTF-8" },
    { "mpeg", "video/mpeg; charset=UTF-8" },
    { "mpe", "video/mpeg; charset=UTF-8" },
    { "mpga", "audio/mpeg; charset=UTF-8" },
    { "mpg", "video/mpeg; charset=UTF-8" },
    { "ms", "application/x-troff-ms; charset=UTF-8" },
    { "msh", "model/mesh; charset=UTF-8" },
    { "nc", "application/x-netcdf; charset=UTF-8" },
    { "oda", "application/oda; charset=UTF-8" },
    { "ogg", "application/ogg; charset=UTF-8" },
    { "ogm", "application/ogg; charset=UTF-8" },
    { "pbm", "image/x-portable-bitmap; charset=UTF-8" },
    { "pdb", "chemical/x-pdb; charset=UTF-8" },
    { "pdf", "application/pdf; charset=UTF-8" },
    { "pgm", "image/x-portable-graymap; charset=UTF-8" },
    { "pgn", "application/x-chess-pgn; charset=UTF-8" },
    { "pgp", "application/pgp; charset=UTF-8" },
    { "pl", "application/x-perl; charset=UTF-8" },
    { "pm", "application/x-perl; charset=UTF-8" },
    { "png", "image/png; charset=UTF-8" },
    { "pnm", "image/x-portable-anymap; charset=UTF-8" },
    { "pot", "application/mspowerpoint; charset=UTF-8" },
    { "ppm", "image/x-portable-pixmap; charset=UTF-8" },
    { "pps", "application/mspowerpoint; charset=UTF-8" },
    { "ppt", "application/mspowerpoint; charset=UTF-8" },
    { "ppz", "application/mspowerpoint; charset=UTF-8" },
    { "pre", "application/x-freelance; charset=UTF-8" },
    { "prt", "application/pro_eng; charset=UTF-8" },
    { "ps", "application/postscript; charset=UTF-8" },
    { "qt", "video/quicktime; charset=UTF-8" },
    { "ra", "audio/x-realaudio; charset=UTF-8" },
    { "ram", "audio/x-pn-realaudio; charset=UTF-8" },
    { "rar", "application/x-rar-compressed; charset=UTF-8" },
    { "ras", "image/cmu-raster; charset=UTF-8" },
    { "ras", "image/x-cmu-raster; charset=UTF-8" },
    { "rgb", "image/x-rgb; charset=UTF-8" },
    { "rm", "audio/x-pn-realaudio; charset=UTF-8" },
    { "roff", "application/x-troff; charset=UTF-8" },
    { "rpm", "audio/x-pn-realaudio-plugin; charset=UTF-8" },
    { "rtf", "application/rtf; charset=UTF-8" },
    { "rtf", "text/rtf; charset=UTF-8" },
    { "rtx", "text/richtext; charset=UTF-8" },
    { "scm", "application/x-lotusscreencam; charset=UTF-8" },
    { "set", "application/set; charset=UTF-8" },
    { "sgml", "text/sgml; charset=UTF-8" },
    { "sgm", "text/sgml; charset=UTF-8" },
    { "sh", "application/x-sh; charset=UTF-8" },
    { "shar", "application/x-shar; charset=UTF-8" },
    { "silo", "model/mesh; charset=UTF-8" },
    { "sit", "application/x-stuffit; charset=UTF-8" },
    { "skd", "application/x-koan; charset=UTF-8" },
    { "skm", "application/x-koan; charset=UTF-8" },
    { "skp", "application/x-koan; charset=UTF-8" },
    { "skt", "application/x-koan; charset=UTF-8" },
    { "smi", "application/smil; charset=UTF-8" },
    { "smil", "application/smil; charset=UTF-8" },
    { "snd", "audio/basic; charset=UTF-8" },
    { "sol", "application/solids; charset=UTF-8" },
    { "spl", "application/x-futuresplash; charset=UTF-8" },
    { "src", "application/x-wais-source; charset=UTF-8" },
    { "step", "application/STEP; charset=UTF-8" },
    { "stl", "application/SLA; charset=UTF-8" },
    { "stp", "application/STEP; charset=UTF-8" },
    { "sv4cpio", "application/x-sv4cpio; charset=UTF-8" },
    { "sv4crc", "application/x-sv4crc; charset=UTF-8" },
    { "svg", "image/svg+xml; charset=UTF-8" },
    { "swf", "application/x-shockwave-flash; charset=UTF-8" },
    { "t", "application/x-troff; charset=UTF-8" },
    { "tar", "application/x-tar; charset=UTF-8" },
    { "tcl", "application/x-tcl; charset=UTF-8" },
    { "tex", "application/x-tex; charset=UTF-8" },
    { "texi", "application/x-texinfo; charset=UTF-8" },
    { "texinfo", "application/x-texinfo; charset=UTF-8" },
    { "tgz", "application/x-tar-gz; charset=UTF-8" },
    { "tiff", "image/tiff; charset=UTF-8" },
    { "tif", "image/tiff; charset=UTF-8" },
    { "tr", "application/x-troff; charset=UTF-8" },
    { "tsi", "audio/TSP-audio; charset=UTF-8" },
    { "tsp", "application/dsptype; charset=UTF-8" },
    { "tsv", "text/tab-separated-values; charset=UTF-8" },
    { "txt", "text/plain; charset=UTF-8" },
    { "unv", "application/i-deas; charset=UTF-8" },
    { "ustar", "application/x-ustar; charset=UTF-8" },
    { "vcd", "application/x-cdlink; charset=UTF-8" },
    { "vda", "application/vda; charset=UTF-8" },
    { "viv", "video/vnd.vivo; charset=UTF-8" },
    { "vivo", "video/vnd.vivo; charset=UTF-8" },
    { "vrml", "model/vrml; charset=UTF-8" },
    { "vsix", "application/vsix; charset=UTF-8" },
    { "wasm", "application/wasm" },
    { "wav", "audio/x-wav; charset=UTF-8" },
    { "wax", "audio/x-ms-wax; charset=UTF-8" },
    { "wiki", "application/x-fossil-wiki; charset=UTF-8" },
    { "wma", "audio/x-ms-wma; charset=UTF-8" },
    { "wmv", "video/x-ms-wmv; charset=UTF-8" },
    { "wmx", "video/x-ms-wmx; charset=UTF-8" },
    { "wrl", "model/vrml; charset=UTF-8" },
    { "wvx", "video/x-ms-wvx; charset=UTF-8" },
    { "xbm", "image/x-xbitmap; charset=UTF-8" },
    { "xlc", "application/vnd.ms-excel; charset=UTF-8" },
    { "xll", "application/vnd.ms-excel; charset=UTF-8" },
    { "xlm", "application/vnd.ms-excel; charset=UTF-8" },
    { "xls", "application/vnd.ms-excel; charset=UTF-8" },
    { "xlw", "application/vnd.ms-excel; charset=UTF-8" },
    { "xml", "text/xml; charset=UTF-8" },
    { "xpm", "image/x-xpixmap; charset=UTF-8" },
    { "xwd", "image/x-xwindowdump; charset=UTF-8" },
    { "xyz", "chemical/x-pdb; charset=UTF-8" },
    { "zip", "application/zip; charset=UTF-8" }
};

static void onDefault(struct evhttp_request* req, void* arg)
{
    evhttp_send_error(req, HTTP_BADREQUEST, 0);
}

void CHTTPServer::onJsonBindCallback(evhttp_request* req, void* arg)
{
    auto filters = (FilterData*)arg;
    if (req && filters && filters->data_cb) {
        auto cmd = evhttp_request_get_command(req);
        // filter
        if (0 == (filters->filter & cmd)) {
            evhttp_send_error(req, HTTP_BADMETHOD, 0);
            return;
        }
        const char* uri = evhttp_request_get_uri(req);
        if (!uri) {
            evhttp_send_error(req, HTTP_INTERNAL, 0);
            return;
        }
        auto deuri = evhttp_uri_parse_with_flags(uri, 0);
        if (!deuri) {
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
        // decode headers
        auto headers = evhttp_request_get_input_headers(req);
        // decode the payload
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        char* data = (char*)evbuffer_pullup(buf, -1);
        int32_t dlen = evbuffer_get_length(buf);

        auto r = filters->data_cb(&qheaders, headers, std::string(data, dlen));

        auto evb = evbuffer_new();
        evbuffer_add_printf(evb, "%s", r.data());
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", "application/json; charset=UTF-8");
        evhttp_send_reply(req, HTTP_OK, "OK", evb);
        if (evb)
            evbuffer_free(evb);
        if (deuri)
            evhttp_uri_free(deuri);
        return;
    }
    evhttp_send_error(req, HTTP_INTERNAL, 0);
}

bool CHTTPServer::JsonRegister(const std::string path, const uint32_t methods, std::function<HttpCallbackType> cb)
{
    m_callbacks[path] = FilterData { .data_cb = cb, .filter = (evhttp_cmd_type)methods };
    evhttp_set_cb(m_http, path.c_str(), onJsonBindCallback, &m_callbacks[path]);
    return true;
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

    freeaddrinfo(result);

    if (::bind(listenfd, rp->ai_addr, rp->ai_addrlen) < 0) {
        destroy();
        return false;
    }

    if (::listen(listenfd, 512) < 0) {
        destroy();
        return false;
    }

    auto handle = evhttp_accept_socket_with_handle(m_http, listenfd);
    if (!handle) {
        destroy();
        return false;
    }
    evhttp_set_bevcb(m_http, onConnected, this);
    evhttp_set_gencb(m_http, onDefault, this);

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