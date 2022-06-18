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
    bool LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file);
    SSL* CreateOneSSL();
    SSL_CTX* Ctx();

private:
    SSL_CTX* m_ssl_ctx = { nullptr };
};
NAMESPACE_FRAMEWORK_END