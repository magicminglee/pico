#pragma once

#include "common.hpp"

#include "nlohmann/json.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CArgument {
public:
    struct Worker {
        std::vector<std::string> Host;
        std::string InHost;
        std::vector<std::string> SubHost;
    };
    std::optional<std::string> TimeZone;
    std::optional<uint32_t> Sid;
    std::optional<uint16_t> Tid;
    std::optional<bool> Daemonize;
    std::optional<std::string> LogDir;
    std::optional<std::string> ModuleName;
    std::optional<std::string> LogLevel;
    std::optional<std::string> ConfDir;
    std::optional<bool> IsVerbose;
    std::optional<bool> IsIsolate;
    std::optional<bool> IsForceHttps;
    std::optional<uint16_t> RedirectStatus;
    std::optional<std::string> RedirectUrl;
    std::optional<std::string> WebRootDir;
    std::optional<std::string> CertificateFile;
    std::optional<std::string> PrivateKeyFile;
    std::optional<bool> IsAllowOrigin;
    std::optional<std::string> ConfHost;
    std::vector<Worker> Workers;
    std::vector<std::string> RouteConf;
    std::optional<uint32_t> Interval;
    std::vector<std::string> RedisUrl;
    std::optional<std::string> MongoUrl;
    std::vector<std::string> DbName;
    std::vector<std::vector<std::string>> DbColl;
    std::optional<std::string> ApiKey;
    std::optional<uint32_t> MaxFrameSize;

    std::string CTXID;
    uint32_t InitInternConnId = { 0 };

    bool ParseArg(const int argc, char** argv, const std::string pgname, const std::string pgdesc);
    bool ParseYaml();

private:
    std::optional<std::string> m_conf;
    std::optional<std::string> m_pgname;
    std::optional<std::string> m_pgdesc;
};

inline thread_local CArgument MYARGS;

NAMESPACE_FRAMEWORK_END