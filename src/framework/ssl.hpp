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
    bool Init(const int64_t max_queue_count = 500);
    void Destroy();
    bool LoadCertificateAndPrivateKey(const std::string certificate_chain_file, const std::string privatekey_file);
    SSL* CreateOneSSL();
    void IncsQueueCnt();
    void DescQueueCnt();
    bool IsQueueOver();
    SSL_CTX* Ctx();

private:
    int64_t m_max_queue_count = { 0 };
    int64_t m_in_queue_count = { 0 };
    SSL_CTX* m_ssl_ctx = { nullptr };
};
NAMESPACE_FRAMEWORK_END