#include "utils.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"
#include "openssl/ssl.h"

NAMESPACE_FRAMEWORK_BEGIN

std::string CUtils::Base64Encode(const std::string& in)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);

    char out[512];
    int outlen = sizeof(out);
    int ret = BIO_write(b64, in.c_str(), in.size());
    (void)BIO_flush(b64);
    if (ret > 0) {
        ret = BIO_read(bio, out, outlen);
    }
    BIO_free_all(b64);
    if (!ret)
        return "";

    return std::string(out, ret);
}

std::string CUtils::Base64Decode(const std::string& in)
{
    int ret = 0;
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);

    char out[512];
    int outlen = sizeof(out);
    ret = BIO_write(bio, in.c_str(), in.size());
    (void)BIO_flush(bio);
    if (ret) {
        ret = BIO_read(b64, out, outlen);
    }
    BIO_free_all(b64);
    if (!ret)
        return "";
    return std::string(out, ret);
}

std::string CUtils::Sha1(const std::string& in)
{
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char*)in.c_str(), in.size(), (unsigned char*)&digest);
    char mdString[SHA_DIGEST_LENGTH * 2 + 1];
    memset(mdString, 0, sizeof(mdString));
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
    }
    return std::string(mdString);
}

std::string CUtils::Sha1Raw(const std::string& in)
{
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char*)in.c_str(), in.size(), (unsigned char*)&digest);
    return std::string((char*)digest, sizeof(digest));
}

std::string CUtils::Sha256Raw(const std::string& in)
{
    unsigned char outputBuffer[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, in.c_str(), in.length());
    SHA256_Final((unsigned char*)outputBuffer, &ctx);
    return std::string((char*)outputBuffer, SHA256_DIGEST_LENGTH);
}

std::string CUtils::Sha256(const std::string& in)
{
    unsigned char outputBuffer[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, in.c_str(), in.length());
    SHA256_Final(outputBuffer, &ctx);
    char hash[SHA256_DIGEST_LENGTH];
    memset(hash, 0, sizeof(hash));
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&hash[i * 2], "%02x", (unsigned int)outputBuffer[i]);
    }
    return std::string(hash);
}

std::string CUtils::HmacSha256(const std::string& in, const std::string& key)
{
    char outputBuffer[SHA256_DIGEST_LENGTH];
    unsigned int len = SHA256_DIGEST_LENGTH;
    HMAC(EVP_sha256(), key.c_str(), key.length(), (const unsigned char*)in.c_str(), in.length(), (unsigned char*)outputBuffer, &len);
    return std::string(outputBuffer, SHA256_DIGEST_LENGTH);
}

std::string CUtils::Md5(const std::string& in)
{
    char token[33];
    std::uint8_t* md = MD5((const std::uint8_t*)in.data(), in.size(), nullptr);
    int len = 0;
    for (std::uint32_t i = 0; i < 16; ++i) {
        len += snprintf(token + len, sizeof(token), "%02x", md[i]);
    }
    return std::string(token, len);
}

static EVP_MD* openssl_get_evp_md_from_algo(CUtils::OPENSSL_ALGO algo)
{
    EVP_MD* mdtype;

    switch (algo) {
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA1:
        mdtype = (EVP_MD*)EVP_sha1();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_MD5:
        mdtype = (EVP_MD*)EVP_md5();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_MD4:
        mdtype = (EVP_MD*)EVP_md4();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA224:
        mdtype = (EVP_MD*)EVP_sha224();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA256:
        mdtype = (EVP_MD*)EVP_sha256();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA384:
        mdtype = (EVP_MD*)EVP_sha384();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA512:
        mdtype = (EVP_MD*)EVP_sha512();
        break;
    case CUtils::OPENSSL_ALGO::OPENSSL_ALGO_RMD160:
        mdtype = (EVP_MD*)EVP_ripemd160();
        break;
    default:
        return NULL;
        break;
    }
    return mdtype;
}
bool CUtils::RSAVerify(const OPENSSL_ALGO algo, const std::string& key, const std::string& digest, const std::string& sig)
{
    BIO* cert = BIO_new_mem_buf(key.data(), key.size());
    EVP_PKEY* pubkey = PEM_read_bio_PUBKEY(cert, nullptr, nullptr, nullptr);
    int32_t err = 0;

    EVP_MD* mdtype = openssl_get_evp_md_from_algo(algo);
    auto md_ctx = EVP_MD_CTX_create();
    if (md_ctx == NULL
        || !EVP_VerifyInit(md_ctx, mdtype)
        || !EVP_VerifyUpdate(md_ctx, digest.data(), digest.size())
        || (err = EVP_VerifyFinal(md_ctx, (unsigned char*)sig.data(), sig.size(), pubkey)) < 0) {
        SPDLOG_ERROR("{} {} {} {}", ERR_error_string(ERR_get_error(), nullptr), digest.size(), sig.size(), err);
    }
    EVP_MD_CTX_destroy(md_ctx);
    EVP_PKEY_free(pubkey);
    BIO_free(cert);
    return err == 1;
}

auto CUtils::ParseDBURL(const std::string& url) -> std::tuple<std::string, std::string, std::string, std::string, std::string, std::string>
{
    // std::string fname("scheme://username:password@www.address.com:port/default_database");
    std::string scheme, username, password, host, port, database;
    const std::regex base_regex("([\\w\\d]+)://([\\w\\d]+):([\\w\\d]+)@([A-Za-z0-9\\.]+):([\\d]+)/([\\w\\d]+)");
    std::smatch base_match;
    if (std::regex_match(url, base_match, base_regex)) {
        // The first sub_match is the whole string; the next
        // sub_match is the first parenthesized expression.
        if (base_match.size() == 7) {
            scheme = base_match[1].str();
            username = base_match[2].str();
            password = base_match[3].str();
            host = base_match[4].str();
            port = base_match[5].str();
            database = base_match[6].str();
        }
    }
    return { scheme, username, password, host, port, database };
}

std::optional<std::string_view> CUtils::GetEnv(const std::string& name)
{
    if (auto v = std::getenv(CStringTool::ToLower(name).c_str()))
        return v;
    if (auto v = std::getenv(CStringTool::ToUpper(name).c_str()))
        return v;
    return std::nullopt;
}

NAMESPACE_FRAMEWORK_END