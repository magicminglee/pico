#include "http.hpp"
#include "argument.hpp"
#include "contex.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include "picohttpparser/picohttpparser.h"

NAMESPACE_FRAMEWORK_BEGIN

const static int32_t MAX_HEADER_NUM = 32;
// picohttparser search header helper
static int bufis(const char* s, size_t l, const char* t)
{
    return strlen(t) == l && memcmp(s, t, l) == 0;
}

int32_t CHttp::Process(const char* data, const unsigned int dlen)
{
    const char* method;
    size_t method_len;
    const char* path;
    size_t path_len;
    struct phr_header headers[MAX_HEADER_NUM];
    size_t num_headers = MAX_HEADER_NUM;
    int32_t ret = phr_parse_request(data, dlen, &method, &method_len, &path, &path_len, &m_minor_version, headers, &num_headers, 0);
    if (ret > 0) {
        m_method = std::string(method, method_len);

        m_headers.clear();
        for (size_t i = 0; i < num_headers; ++i) {
            m_headers.insert(std::make_pair(CStringTool::ToLower(std::string(headers[i].name, headers[i].name_len)),
                std::string(headers[i].value, headers[i].value_len)));
        }

        if (auto v = m_headers.find("content-length"); v != m_headers.end()) {
            size_t cl = CStringTool::ToInteger<size_t>(v->second);
            if (dlen < cl + ret) {
                return -2; // partial data
            } else {
                m_data = std::string(data + ret, cl);
                ret += cl; // complete data
            }
        } else {
            return -1;
        }
        auto vec = CStringTool::Split(std::string_view(path, path_len), "?");
        m_path = vec[0];
        if (vec.size() == 2) {
            std::string str(vec[1]);
            size_t size;
            char* decodestr = evhttp_uridecode(str.data(), 1, &size);
            auto vec1 = CStringTool::Split(decodestr, "&");
            m_querys.clear();
            for (auto& v : vec1) {
                auto vec2 = CStringTool::Split(v, "=");
                if (vec2.size() == 2) {
                    m_querys[std::string(vec2[0].data(), vec2[0].size())] = std::string(vec2[1].data(), vec2[1].size());
                }
            }
            free(decodestr);
        }
    }
    return ret;
}

std::string_view CHttp::Frame(const void* data, const uint32_t dlen)
{
    static thread_local char* buffer = { nullptr };
    if (!buffer)
        buffer = CNEW char[MAX_WATERMARK_SIZE];
    uint32_t offset = 0;
    if (MYARGS.IsAllowOrigin) {
        const char* header = "HTTP/1.1 200 OK\r\nServer: TxServer/1.31.4\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept\r\nContent-Length: %u\r\n\r\n";
        offset = snprintf(buffer, MAX_WATERMARK_SIZE, header, dlen);
    } else {
        const char* header = "HTTP/1.1 200 OK\r\nServer: TxServer/1.31.4\r\nContent-Type: application/json\r\nContent-Length: %u\r\n\r\n";
        offset = snprintf(buffer, MAX_WATERMARK_SIZE, header, dlen);
    }
    CheckCondition(offset + dlen < MAX_WATERMARK_SIZE, std::string_view());
    memcpy(buffer + offset, data, dlen);
    return std::string_view(buffer, dlen + offset);
}

const std::optional<std::string_view> CHttp::Header(const std::string& key)
{
    if (auto v = m_headers.find(key); v != m_headers.end()) {
        return v->second;
    }
    return std::nullopt;
}

void CHttpContex::IncsQueueCnt()
{
    ++m_in_queue_count;
}

void CHttpContex::DescQueueCnt()
{
    --m_in_queue_count;
}

bool CHttpContex::IsQueueOver()
{
    return m_in_queue_count > m_max_queue_count;
}

SSL_CTX* CHttpContex::Ctx()
{
    return m_ssl_ctx;
}

bool CHttpContex::LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file)
{
    if (!m_ssl_ctx)
        return false;

    (void)SSL_CTX_set_ecdh_auto(m_ssl_ctx, 1);

    if (SSL_CTX_use_certificate_chain_file(m_ssl_ctx, certificate_chain_file.c_str()) <= 0) {
        CERROR("load certificate chain file %s failed!!!", certificate_chain_file.c_str());
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, privatekey_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        CERROR("load private key file %s failed!!!", privatekey_file.c_str());
        return false;
    }

    if (!SSL_CTX_check_private_key(m_ssl_ctx)) {
        CERROR("check private key %s not match!!!", privatekey_file.c_str());
        return false;
    }
    CINFO("%s success %s %s", __FUNCTION__, certificate_chain_file.c_str(), privatekey_file.c_str());
    return true;
}

SSL* CHttpContex::CreateOneSSL()
{
    return (SSL*)SSL_new(m_ssl_ctx);
}

bool CHttpContex::Init(const int64_t max_queue_count)
{
    m_max_queue_count = max_queue_count;
    if (!m_ssl_ctx) {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        m_ssl_ctx = SSL_CTX_new(SSLv23_method());
    }
    return m_ssl_ctx != nullptr;
}

void CHttpContex::Destroy()
{
    if (m_ssl_ctx)
        SSL_CTX_free(m_ssl_ctx);
    m_ssl_ctx = nullptr;
}

NAMESPACE_FRAMEWORK_END