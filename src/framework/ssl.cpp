#include "ssl.hpp"
#include "argument.hpp"
#include "xlog.hpp"

#include "nghttp2/nghttp2.h"

NAMESPACE_FRAMEWORK_BEGIN

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

static int select_next_proto_cb(SSL* ssl, unsigned char** out,
    unsigned char* outlen, const unsigned char* in,
    unsigned int inlen, void* arg)
{
    (void)ssl;
    (void)arg;

    if (nghttp2_select_next_protocol(out, outlen, in, inlen) <= 0) {
        CERROR("Server did not advertise " NGHTTP2_PROTO_VERSION_ID);
    }
    return SSL_TLSEXT_ERR_OK;
}

bool CSSLContex::LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file)
{
    if (!m_server_ssl_ctx)
        return false;

    SSL_CTX_set_ecdh_auto(m_server_ssl_ctx, 1);
    if (!certificate_chain_file.empty() && SSL_CTX_use_certificate_chain_file(m_server_ssl_ctx, certificate_chain_file.c_str()) <= 0) {
        CERROR("load certificate chain file %s failed!!!", certificate_chain_file.c_str());
        return false;
    }

    if (!privatekey_file.empty() && SSL_CTX_use_PrivateKey_file(m_server_ssl_ctx, privatekey_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        CERROR("load private key file %s failed!!!", privatekey_file.c_str());
        return false;
    }

    if (!SSL_CTX_check_private_key(m_server_ssl_ctx)) {
        CERROR("check private key %s not match!!!", privatekey_file.c_str());
        return false;
    }

    SSL_CTX_set_alpn_select_cb(m_server_ssl_ctx, alpn_select_proto_cb, nullptr);
    CINFO("%s success %s %s", __FUNCTION__, certificate_chain_file.c_str(), privatekey_file.c_str());
    return true;
}

SSL* CSSLContex::CreateOneSSL(bool isserver)
{
    return isserver ? (SSL*)SSL_new(m_server_ssl_ctx) : (SSL*)SSL_new(m_client_ssl_ctx);
}

bool CSSLContex::Init()
{
    if (!m_server_ssl_ctx) {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        m_server_ssl_ctx = SSL_CTX_new(TLS_server_method());
        if (!m_server_ssl_ctx) {
            CERROR("Could not create SSL/TLS context: %s", ERR_error_string(ERR_get_error(), NULL));
        }
    }
    if (!m_client_ssl_ctx) {
        m_client_ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!m_client_ssl_ctx) {
            CERROR("Could not create SSL/TLS context: %s", ERR_error_string(ERR_get_error(), NULL));
        }
        SSL_CTX_set_options(m_client_ssl_ctx,
            SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
        SSL_CTX_set_next_proto_select_cb(m_client_ssl_ctx, select_next_proto_cb, NULL);
        SSL_CTX_set_alpn_protos(m_client_ssl_ctx, (const unsigned char*)"\x02h2", 3);
    }

    return m_server_ssl_ctx != nullptr || m_client_ssl_ctx != nullptr;
}

void CSSLContex::Destroy()
{
    if (m_server_ssl_ctx)
        SSL_CTX_free(m_server_ssl_ctx);
    m_server_ssl_ctx = nullptr;

    if (m_client_ssl_ctx)
        SSL_CTX_free(m_client_ssl_ctx);
    m_client_ssl_ctx = nullptr;
}

NAMESPACE_FRAMEWORK_END