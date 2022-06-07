#include "utils.hpp"

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/hmac.h"
#include "openssl/md5.h"
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
NAMESPACE_FRAMEWORK_END