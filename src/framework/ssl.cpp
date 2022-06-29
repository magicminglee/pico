#include "ssl.hpp"
#include "argument.hpp"
#include "xlog.hpp"

#include "nghttp2/nghttp2.h"

NAMESPACE_FRAMEWORK_BEGIN

SSL_CTX* CSSLContex::Ctx()
{
    return m_ssl_ctx;
}

static int alpn_select_proto_cb(SSL* ssl, const unsigned char** out,
    unsigned char* outlen, const unsigned char* in,
    unsigned int inlen, void* arg)
{
    if (!MYARGS.Http2Able || (MYARGS.Http2Able && !MYARGS.Http2Able.value()))
        return SSL_TLSEXT_ERR_NOACK;
    int rv;
    (void)ssl;
    (void)arg;

    rv = nghttp2_select_next_protocol((unsigned char**)out, outlen, in, inlen);

    if (rv != 1) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    return SSL_TLSEXT_ERR_OK;
}

bool CSSLContex::LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file)
{
    if (!m_ssl_ctx)
        return false;

    SSL_CTX_set_ecdh_auto(m_ssl_ctx, 1);
    if (!certificate_chain_file.empty() && SSL_CTX_use_certificate_chain_file(m_ssl_ctx, certificate_chain_file.c_str()) <= 0) {
        CERROR("load certificate chain file %s failed!!!", certificate_chain_file.c_str());
        return false;
    }

    if (!privatekey_file.empty() && SSL_CTX_use_PrivateKey_file(m_ssl_ctx, privatekey_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        CERROR("load private key file %s failed!!!", privatekey_file.c_str());
        return false;
    }

    if (!SSL_CTX_check_private_key(m_ssl_ctx)) {
        CERROR("check private key %s not match!!!", privatekey_file.c_str());
        return false;
    }

    SSL_CTX_set_alpn_select_cb(m_ssl_ctx, alpn_select_proto_cb, nullptr);
    CINFO("%s success %s %s", __FUNCTION__, certificate_chain_file.c_str(), privatekey_file.c_str());
    return true;
}

SSL* CSSLContex::CreateOneSSL()
{
    return (SSL*)SSL_new(m_ssl_ctx);
}

bool CSSLContex::Init()
{
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