#pragma once
#include "common.hpp"
#include "object.hpp"
#include "singleton.hpp"
#include "utils.hpp"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

NAMESPACE_FRAMEWORK_BEGIN

class CHttp {
public:
    CHttp() = default;
    ~CHttp() = default;
    int32_t Process(const char* data, const unsigned int dlen);
    std::string_view Data() { return m_data.data(); }
    const char* Method() { return m_method.data(); }
    const char* Path() { return m_path.data(); }
    static std::string_view Frame(const void* data, const uint32_t dlen);
    const std::optional<std::string_view> Header(const std::string& key);

private:
    using HeaderMap = std::unordered_map<std::string, std::string>;
    std::string m_method;
    std::string m_data;
    std::string m_path;
    int32_t m_minor_version;
    HeaderMap m_headers;
    HeaderMap m_querys;
};

class CHttpContex : public CTLSingleton<CHttpContex> {
public:
    CHttpContex() = default;
    ~CHttpContex() = default;
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