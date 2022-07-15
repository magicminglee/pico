#include "httpmsg.hpp"
#include "argument.hpp"
#include "httpserver.hpp"
#include "stringtool.hpp"

#include <fmt/core.h>

namespace fs = std::filesystem;
NAMESPACE_FRAMEWORK_BEGIN

namespace ghttp {
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

static const std::map<HttpStatusCode, std::string> RESPSTR = {
    { HttpStatusCode::SWITCH, "SWITCHING PROTOCOLS" },
    { HttpStatusCode::OK, "OK" },
    { HttpStatusCode::NOCONTENT, "NO CONTENT" },
    { HttpStatusCode::MOVEPERM, "MOVED PERMANENTLY" },
    { HttpStatusCode::MOVETEMP, "MOVED TEMPORARILY" },
    { HttpStatusCode::NOTMODIFIED, "NOT MODIFIED" },
    { HttpStatusCode::BADREQUEST, "INVALID REQUEST" },
    { HttpStatusCode::UNAUTHORIZED, "UNAUTHORIZED" },
    { HttpStatusCode::FORBIDDEN, "FORBIDDEN" },
    { HttpStatusCode::NOTFOUND, "NOT FOUND" },
    { HttpStatusCode::BADMETHOD, "METHOD NOT ALLOWED" },
    { HttpStatusCode::ENTITYTOOLARGE, "ENTITYTOOLARGE" },
    { HttpStatusCode::EXPECTATIONFAILED, "EXPECTATIONFAILED" },
    { HttpStatusCode::INTERNAL, "INTERNAL ERROR" },
    { HttpStatusCode::SERVUNAVAIL, "NOT AVAILABLE" },
    { HttpStatusCode::BADGATEWAY, "BADGATEWAY" },
};

static const std::map<HttpMethod, std::string> HTTPMETHOD = {
    { HttpMethod::GET, "GET" },
    { HttpMethod::POST, "POST" },
    { HttpMethod::HEAD, "HEAD" },
    { HttpMethod::PUT, "PUT" },
    { HttpMethod::DELETE, "DELETE" },
    { HttpMethod::OPTIONS, "OPTIONS" },
    { HttpMethod::TRACE, "TRACE" },
    { HttpMethod::CONNECT, "CONNECT" },
    { HttpMethod::PATCH, "PATCH" },
};

static const std::map<std::string, HttpMethod> HTTP_STR_METHOD = {
    { "GET", HttpMethod::GET },
    { "POST", HttpMethod::POST },
    { "HEAD", HttpMethod::HEAD },
    { "PUT", HttpMethod::PUT },
    { "DELETE", HttpMethod::DELETE },
    { "OPTIONS", HttpMethod::OPTIONS },
    { "TRACE", HttpMethod::TRACE },
    { "CONNECT", HttpMethod::CONNECT },
    { "PATCH", HttpMethod::PATCH },
};

std::optional<const char*> HttpReason(HttpStatusCode code)
{
    if (auto it = RESPSTR.find(code); it != std::end(RESPSTR))
        return std::optional<const char*>(it->second.c_str());

    return std::nullopt;
}
std::optional<const char*> HttpMethodStr(HttpMethod m)
{
    if (auto it = HTTPMETHOD.find(m); it != std::end(HTTPMETHOD))
        return std::optional<const char*>(it->second.c_str());

    return std::nullopt;
}
std::optional<HttpMethod> HttpStrMethod(const std::string& m)
{
    if (auto it = HTTP_STR_METHOD.find(CStringTool::ToUpper(m)); it != std::end(HTTP_STR_METHOD))
        return std::optional<HttpMethod>(it->second);

    return std::nullopt;
}
std::optional<std::string> GetMiMe(const std::string& m)
{
    auto it = MIME.find(m);
    if (it != std::end(MIME)) {
        return std::optional(it->second);
    }
    return std::nullopt;
}

///////////////////////////////////////////////////////////////CRequest////////////////////////////////////////////////////////
void CRequest::Reset()
{
    qheaders.clear();
    headers.clear();
    path = std::nullopt;
    body = std::nullopt;
    uid = std::nullopt;
    method = HttpMethod::HEAD;
}

CRequest* CRequest::Clone()
{
    CRequest* self = CNEW CRequest();
    *self = *this;
    return self;
}

void CRequest::SetUid(const int64_t uid)
{
    this->uid = uid;
}

std::optional<std::string> CRequest::GetQueryByKey(const std::string& key) const
{
    auto it = qheaders.find(key);
    return it != qheaders.end() ? std::make_optional(it->second) : std::nullopt;
}

std::optional<std::string> CRequest::GetHeaderByKey(const std::string& key) const
{
    auto it = headers.find(key);
    return it != headers.end() ? std::make_optional(it->second) : std::nullopt;
}

void CRequest::AddHeader(const std::string& key, const std::string& val)
{
    headers[CStringTool::ToLower(key)] = val;
}

bool CRequest::HasHeader(const std::string& key) const
{
    auto it = headers.find(key);
    return it != headers.end();
}

void CRequest::AppendBody(std::string_view body)
{
}

void CRequest::SetBody(const std::string& body)
{
    this->body = body;
}

void CRequest::SetPath(const std::string& path)
{
    this->path = path;
}

void CRequest::SetMethod(HttpMethod method)
{
    this->method = method;
}

void CRequest::SetHost(const std::string& host)
{
    this->host = host;
}

void CRequest::SetPort(const std::string& port)
{
    this->port = port;
}

void CRequest::SetScheme(const std::string& scheme)
{
    this->scheme = scheme;
}

void CRequest::GetAllHeaders(std::map<std::string, std::string>& header)
{
    for (auto& [k, v] : this->headers) {
        header[k] = v;
    }
}

void CRequest::SetStreamId(const int32_t streamid)
{
    this->streamid = streamid;
}

void CRequest::SetFd(const int32_t fd)
{
    this->fd = fd;
}

std::string CRequest::DebugStr()
{
    std::string h, debugstr;
    for (auto& [k, v] : headers) {
        h += fmt::format("{}: {} ", k, v);
    }
    return fmt::format("method:{} path:{} header:{} body:{}",
        HttpMethodStr(method).value_or(""),
        path.value_or(""), h, body.value_or(""));
}

///////////////////////////////////////////////////////////////CResponse////////////////////////////////////////////////////////
CResponse::CResponse(CHTTPClient* con)
    : con(con)
{
}

void CResponse::Reset()
{
    qheaders.clear();
    headers.clear();
    status = std::nullopt;
    body = std::nullopt;
}

CResponse* CResponse::Clone()
{
    CResponse* self = CNEW CResponse(nullptr);
    *self = *this;
    return self;
}

std::optional<std::string> CResponse::GetQueryByKey(const std::string& key) const
{
    auto it = qheaders.find(key);
    return it != qheaders.end() ? std::make_optional(it->second) : std::nullopt;
}

std::optional<std::string> CResponse::GetHeaderByKey(const std::string& key) const
{
    auto it = headers.find(key);
    return it != headers.end() ? std::make_optional(it->second) : std::nullopt;
}

bool CResponse::HasHeader(const std::string& key) const
{
    auto it = headers.find(key);
    return it != headers.end() ? true : false;
}

void CResponse::SetBody(std::string_view body)
{
    this->body = body;
}

void CResponse::SetStatus(std::optional<HttpStatusCode> status)
{
    this->status = status;
}

void CResponse::AppendBody(std::string_view)
{
}

bool CResponse::Response(const std::pair<HttpStatusCode, std::string>& res)
{
    if (!Conn()->IsHttp2()) {
        std::string httprsp = fmt::format("HTTP/1.1 {} {}\r\n", (int32_t)res.first, HttpReason(res.first).value_or(""));
        for (auto& [k, v] : headers) {
            httprsp += fmt::format("{}: {}\r\n", k, v);
        }
        httprsp += fmt::format("Content-Length: {}\r\n", res.second.size());
        httprsp += fmt::format("\r\n");
        if (!res.second.empty())
            httprsp.append(res.second);
        return Conn()->GetConnection()->SendCmd(httprsp.data(), httprsp.size());
    } else {
        return Conn()->SendResponse(Conn()->GetParser().GetRequest(), res.first, {}, res.second);
    }
}

bool CResponse::SendFile(const std::string& filename)
{
    fs::path f = MYARGS.WebRootDir.value_or(fs::current_path()) + filename;
    if (!fs::exists(f)) {
        return Response({ HttpStatusCode::NOTFOUND, HttpReason(HttpStatusCode::NOTFOUND).value_or("") });
    } else {
        auto it = MIME.find(f.extension());
        if (it == std::end(MIME)) {
            return Response({ HttpStatusCode::NOTFOUND, HttpReason(HttpStatusCode::NOTFOUND).value_or("") });
        } else {
            auto hfd = fopen(f.c_str(), "r");
            if (hfd) {
                std::string httprsp = fmt::format("HTTP/1.1 {} {}\r\n", (int32_t)HttpStatusCode::OK, HttpReason(HttpStatusCode::OK).value_or(""));
                for (auto& [k, v] : headers) {
                    httprsp += fmt::format("{}: {}\r\n", k, v);
                }
                httprsp += fmt::format("Content-Length: {}\r\n", fs::file_size(f));
                httprsp += fmt::format("\r\n");
                Conn()->GetConnection()->SendCmd(httprsp.data(), httprsp.size());
                return Conn()->GetConnection()->SendFile(fileno(hfd));
            } else {
                return Response({ HttpStatusCode::NOTFOUND, HttpReason(HttpStatusCode::NOTFOUND).value_or("") });
            }
        }
    }
    return Response({ HttpStatusCode::NOTFOUND, HttpReason(HttpStatusCode::NOTFOUND).value_or("") });
}

void CResponse::AddHeader(const std::string& key, const std::string& val)
{
    headers[CStringTool::ToLower(key)] = val;
}

std::string CResponse::DebugStr()
{
    std::string h, debugstr;
    for (auto& [k, v] : headers) {
        h += fmt::format("{}: {} ", k, v);
    }
    return fmt::format("status:{} header:{} body:{}",
        HttpReason(status.value_or(HttpStatusCode::INTERNAL)).value_or(""),
        h, body.value_or(""));
}

} // end namespace ghttp

NAMESPACE_FRAMEWORK_END