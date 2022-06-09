#include "ssl.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

void CSSLContex::IncsQueueCnt()
{
    ++m_in_queue_count;
}

void CSSLContex::DescQueueCnt()
{
    --m_in_queue_count;
}

bool CSSLContex::IsQueueOver()
{
    return m_in_queue_count > m_max_queue_count;
}

SSL_CTX* CSSLContex::Ctx()
{
    return m_ssl_ctx;
}

bool CSSLContex::LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file)
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

SSL* CSSLContex::CreateOneSSL()
{
    return (SSL*)SSL_new(m_ssl_ctx);
}

bool CSSLContex::Init(const int64_t max_queue_count)
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

void CSSLContex::Destroy()
{
    if (m_ssl_ctx)
        SSL_CTX_free(m_ssl_ctx);
    m_ssl_ctx = nullptr;
}
NAMESPACE_FRAMEWORK_END