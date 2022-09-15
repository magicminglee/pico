#pragma once
#include "singleton.hpp"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

NAMESPACE_FRAMEWORK_BEGIN
class CSSLContex : public CTLSingleton<CSSLContex> {
public:
    CSSLContex() = default;
    ~CSSLContex() = default;
    bool Init();
    void Destroy();
    SSL_CTX* LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file);
    SSL* CreateOneSSL(const bool isserver, const std::string& hostname);

private:
    static int tlsext_servername_cb(SSL* s, int* al, void* arg);

private:
    bool m_is_init = { false };
    std::map<std::string, SSL_CTX*> m_server_ssl_ctx;
    SSL_CTX* m_client_ssl_ctx;
};
NAMESPACE_FRAMEWORK_END