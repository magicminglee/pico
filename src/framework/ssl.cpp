#include "ssl.hpp"
#include "argument.hpp"
#include "xlog.hpp"

#include <regex>

#include "nghttp2/nghttp2.h"

NAMESPACE_FRAMEWORK_BEGIN

static int alpn_select_proto_cb(SSL* ssl, const unsigned char** out,
    unsigned char* outlen, const unsigned char* in,
    unsigned int inlen, void* arg)
{
    if (MYARGS.Http2Able.value_or(false)) {
    } else {
        return SSL_TLSEXT_ERR_NOACK;
    }
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
        SPDLOG_ERROR("Server did not advertise " NGHTTP2_PROTO_VERSION_ID);
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}

int CSSLContex::tlsext_servername_cb(SSL* s, int* al, void* arg)
{
    CSSLContex* ctx = (CSSLContex*)arg;
    auto sn = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);
    if (sn) {
        if (auto it = ctx->m_server_ssl_ctx.find(sn); it != std::end(ctx->m_server_ssl_ctx)) {
            SPDLOG_INFO("sni {} match servername {}", sn, it->first);
            SSL_set_SSL_CTX(s, it->second);
            return SSL_TLSEXT_ERR_OK;
        }
        for (auto& [k, v] : ctx->m_server_ssl_ctx) {
            try {
                const std::regex exp_regex(k);
                if (std::regex_match(sn, exp_regex)) {
                    SPDLOG_INFO("sni {} match servername {}", sn, k);
                    SSL_set_SSL_CTX(s, v);
                    return SSL_TLSEXT_ERR_OK;
                }
            } catch (std::exception& e) {
                SPDLOG_ERROR("{} {} {} {}", __FUNCTION__, e.what(), k, sn);
            }
        }
        SPDLOG_ERROR("{} servername not found", sn);
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}

SSL_CTX* CSSLContex::LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file)
{
    auto ctx = SSL_CTX_new(TLS_server_method());
    if (!certificate_chain_file.empty() && SSL_CTX_use_certificate_chain_file(ctx, certificate_chain_file.c_str()) != 1) {
        SPDLOG_ERROR("load certificate chain file {} error {}", certificate_chain_file, ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (!privatekey_file.empty() && SSL_CTX_use_PrivateKey_file(ctx, privatekey_file.c_str(), SSL_FILETYPE_PEM) != 1) {
        SPDLOG_ERROR("load private key file {} error {}", privatekey_file, ERR_error_string(ERR_get_error(), NULL));
        SSL_CTX_free(ctx);
        return nullptr;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        SPDLOG_ERROR("check private key {} not match!!!", privatekey_file);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    SSL_CTX_set_alpn_select_cb(ctx, alpn_select_proto_cb, nullptr);
    SPDLOG_INFO("{} success {} {}", __FUNCTION__, certificate_chain_file, privatekey_file);
    return ctx;
}

SSL* CSSLContex::CreateOneSSL(const bool isserver, const std::string& hostname)
{
    SSL* ssl = nullptr;
    if (isserver) {
        if (!m_server_ssl_ctx.empty())
            ssl = (SSL*)SSL_new(m_server_ssl_ctx.begin()->second);
    } else {
        if (m_client_ssl_ctx) {
            ssl = (SSL*)SSL_new(m_client_ssl_ctx);
            SSL_set_tlsext_host_name(ssl, hostname.c_str());
        }
    }
    return ssl;
}

bool CSSLContex::Init()
{
    if (!m_is_init) {
        SSL_library_init();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        m_is_init = true;
    }

    Destroy();

    for (const auto& v : MYARGS.SslConf) {
        m_server_ssl_ctx[v.servername] = LoadCertificateAndPrivateKey(v.cert, v.prikey);
        SSL_CTX_set_tlsext_servername_callback(m_server_ssl_ctx[v.servername], tlsext_servername_cb);
        SSL_CTX_set_tlsext_servername_arg(m_server_ssl_ctx[v.servername], this);
    }

    {
        m_client_ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!m_client_ssl_ctx) {
            SPDLOG_ERROR("Could not create SSL/TLS context: {}", ERR_error_string(ERR_get_error(), NULL));
        }
        SSL_CTX_set_options(m_client_ssl_ctx,
            SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
        SSL_CTX_set_next_proto_select_cb(m_client_ssl_ctx, select_next_proto_cb, NULL);
        if (MYARGS.Http2Able.value_or(false))
            SSL_CTX_set_alpn_protos(m_client_ssl_ctx, (const unsigned char*)"\x02h2", 3);
    }

    return m_is_init;
}

void CSSLContex::Destroy()
{
    for (auto& [k, v] : m_server_ssl_ctx) {
        SSL_CTX_free(v);
    }
    m_server_ssl_ctx.clear();

    if (m_client_ssl_ctx)
        SSL_CTX_free(m_client_ssl_ctx);
    m_client_ssl_ctx = nullptr;
}

NAMESPACE_FRAMEWORK_END