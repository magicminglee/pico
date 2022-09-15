#include "argument.hpp"
#include "cxxopts.hpp"
#include "utils.hpp"
#include "worker.hpp"
#include "xlog.hpp"

#include "yaml-cpp/yaml.h"

#include <filesystem>

NAMESPACE_FRAMEWORK_BEGIN
namespace fs = std::filesystem;

bool CArgument::ParseArg(int argc, char** argv, const std::string pgname, const std::string pgdesc)
{
    cxxopts::Options options(pgname, pgdesc);

    options.add_options()("f,conf", "Specify a configuration file", cxxopts::value<std::string>())("h,help", "Usage");
    auto result = options.parse(argc, argv);

    if (result.count("conf")) {
        m_conf = result["conf"].as<std::string>();
    }

    if (!m_conf || result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    m_pgname = pgname;
    m_pgdesc = pgdesc;

    return ParseYaml();
}

bool CArgument::ParseYaml()
{
    YAML::Node config = YAML::LoadFile(m_conf.value());
    if (!config) {
        std::cout << "load config file failed:" << m_conf.value() << std::endl;
        return false;
    }

    if (config["main"] && config["main"].IsMap()) {
        if (config["main"]["type"] && config["main"]["id"]) {
            Sid = CUtils::HashServerId(config["main"]["type"].as<std::uint16_t>(), config["main"]["id"].as<std::uint16_t>());
            Tid = 0;
        }
        if (config["main"]["daemonize"]) {
            Daemonize = config["main"]["daemonize"].as<bool>();
        }
        if (config["main"]["timezone"]) {
            TimeZone = config["main"]["timezone"].as<std::string>();
        }
        if (config["main"]["scriptdir"]) {
            ScriptDir = config["main"]["scriptdir"].as<std::string>();
        } else {
            ScriptDir = fs::current_path().parent_path() / "luascripts";
        }

        if (config["main"]["hosts"] && config["main"]["hosts"].IsSequence()) {
            auto n = 1;
            if (config["main"] && config["main"].IsMap() && config["main"]["workers"]) {
                n = config["main"]["workers"].as<int32_t>();
            }
            if (Workers.empty()) {
                for (auto i = 0; i < n; ++i) {
                    auto& w = Workers.emplace_back();
                    for (auto&& v : config["main"]["hosts"]) {
                        w.Host.push_back(v.as<std::string>());
                    }
                }
            }
        }
    }

    if (config["main"] && config["main"].IsMap() && config["main"]["log"] && config["main"]["log"].IsMap()) {
        if (config["main"]["log"]["logfile"]) {
            LogFile = config["main"]["log"]["logfile"].as<std::string>();
        } else {
            LogFile = std::string(fs::current_path()) + std::string("log.log");
        }
        if (config["main"]["log"]["rotatebytes"]) {
            RotateBytes = config["main"]["log"]["rotatebytes"].as<uint64_t>();
        }
        if (config["main"]["log"]["rotatenums"]) {
            RotateNums = config["main"]["log"]["rotatenums"].as<uint64_t>();
        }
        if (config["main"]["log"]["loglevel"]) {
            LogLevel = config["main"]["log"]["loglevel"].as<std::string>();
        }
        if (config["main"]["log"]["flushlevel"]) {
            FlushLevel = config["main"]["log"]["flushlevel"].as<std::string>();
        }
        if (config["main"]["log"]["verbose"]) {
            IsVerbose = config["main"]["log"]["verbose"].as<bool>();
        }
        if (config["main"]["log"]["isolate"]) {
            IsIsolate = config["main"]["log"]["isolate"].as<bool>();
        }
        if (config["main"]["log"]["modulename"]) {
            ModuleName = config["main"]["log"]["modulename"].as<std::string>();
        }
    }

    if (config["main"] && config["main"].IsMap() && config["main"]["web"] && config["main"]["web"].IsMap()) {
        if (config["main"]["web"]["forcehttps"]) {
            IsForceHttps = config["main"]["web"]["forcehttps"].as<bool>();
        }
        if (config["main"]["web"]["redirecturl"]) {
            RedirectUrl = config["main"]["web"]["redirecturl"].as<std::string>();
        }
        if (config["main"]["web"]["redirectstatus"]) {
            RedirectStatus = config["main"]["web"]["redirectstatus"].as<std::uint16_t>();
        }
        if (config["main"]["web"]["webroot"]) {
            WebRootDir = config["main"]["web"]["webroot"].as<std::string>();
        } else {
            WebRootDir = fs::current_path();
        }
        if (config["main"]["web"]["alloworigin"]) {
            IsAllowOrigin = config["main"]["web"]["alloworigin"].as<bool>();
        }
        if (config["main"]["web"]["http2able"]) {
            Http2Able = config["main"]["web"]["http2able"].as<bool>();
        }
        if (config["main"]["web"]["interval"]) {
            Interval = config["main"]["web"]["interval"].as<std::uint32_t>() * 1000;
        }
        if (config["main"]["web"]["maxframesize"]) {
            MaxFrameSize = config["main"]["web"]["maxframesize"].as<std::uint32_t>();
        }
        if (config["main"]["web"]["apikey"]) {
            ApiKey = config["main"]["web"]["apikey"].as<std::string>();
        }
        if (config["main"]["web"]["tokenexpire"]) {
            TokenExpire = config["main"]["web"]["tokenexpire"].as<uint32_t>();
        } else {
            TokenExpire = 10;
        }
        if (config["main"]["web"]["httptimeout"]) {
            HttpTimeout = config["main"]["web"]["httptimeout"].as<uint32_t>();
        }
    }

    if (config["main"] && config["main"].IsMap() && config["main"]["ssl"] && config["main"]["ssl"].IsSequence()) {
        SslConf.clear();
        for (auto&& v : config["main"]["ssl"]) {
            SSLConf conf;
            if (v.IsMap() && v["servername"]) {
                conf.servername = v["servername"].as<std::string>();
            }
            if (v.IsMap() && v["certificate"]) {
                conf.cert = v["certificate"].as<std::string>();
            }
            if (v.IsMap() && v["privatekey"]) {
                conf.prikey = v["privatekey"].as<std::string>();
            }
            SslConf.push_back(std::move(conf));
        }
    }

    if (config["main"] && config["main"].IsMap() && config["main"]["db"] && config["main"]["db"].IsMap()) {
        if (config["main"]["db"]["redisurl"] && config["main"]["db"]["redisurl"].IsSequence()) {
            RedisUrl.clear();
            for (auto&& v : config["main"]["db"]["redisurl"]) {
                if (v.IsScalar()) {
                    RedisUrl.push_back(v.as<std::string>());
                }
            }
        }
        if (config["main"]["db"]["redisttl"]) {
            RedisTTL = config["main"]["db"]["redisttl"].as<std::uint64_t>();
        } else {
            RedisTTL = 300;
        }
        if (config["main"]["db"]["mongourl"]) {
            MongoUrl = config["main"]["db"]["mongourl"].as<std::string>();
        }
        if (config["main"]["db"]["dbconf"] && config["main"]["db"]["dbconf"].IsSequence()) {
            DbName.clear();
            DbColl.clear();
            for (auto&& v : config["main"]["db"]["dbconf"]) {
                if (v.IsMap() && v["dbname"] && v["collection"] && v["collection"].IsSequence()) {
                    DbName.push_back(v["dbname"].as<std::string>());
                    std::vector<std::string> vec;
                    for (auto&& vv : v["collection"]) {
                        vec.push_back(vv.as<std::string>());
                    }
                    DbColl.push_back(std::move(vec));
                }
            }
        }
        if (config["main"]["db"]["clickhouseurl"]) {
            ClickhouseUrl = config["main"]["db"]["clickhouseurl"].as<std::string>();
        }
    }

    if (config["main"] && config["main"].IsMap() && config["main"]["confmap"] && config["main"]["confmap"].IsMap()) {
        ConfMap.clear();
        for (auto&& v : config["main"]["confmap"]) {
            ConfMap[v.first.as<std::string>()] = v.second.as<std::string>();
        }
    }

    nlohmann::json j;
    j["cmd"] = "reload";
    if (LogLevel)
        j["loglevel"] = LogLevel.value();
    if (IsAllowOrigin)
        j["alloworigin"] = IsAllowOrigin.value();
    if (IsForceHttps)
        j["forcehttps"] = IsForceHttps.value();
    if (RedirectStatus)
        j["redirectstatus"] = RedirectStatus.value();
    if (RedirectUrl)
        j["redirecturl"] = RedirectUrl.value();
    if (ApiKey)
        j["apikey"] = ApiKey.value();
    if (TokenExpire)
        j["tokenexpire"] = TokenExpire.value();
    if (HttpTimeout)
        j["httptimeout"] = HttpTimeout.value();
    if (RedisTTL)
        j["redisttl"] = RedisTTL.value();
    if (Http2Able)
        j["http2able"] = Http2Able.value();
    auto a = nlohmann::json::array();
    for (auto&& v : Workers) {
        std::vector<std::string> hosts;
        for (auto&& vv : v.Host)
            hosts.push_back(vv);
        nlohmann::json w;
        w["hosts"] = hosts;
        a.push_back(w);
    }
    j["wokers"] = a;
    a.clear();
    for (auto&& v : SslConf) {
        nlohmann::json w;
        w["servername"] = v.servername;
        w["certificate"] = v.cert;
        w["privatekey"] = v.prikey;
        a.push_back(w);
    }
    j["ssl"] = a;

    nlohmann::json confmap;
    for (auto& [k, v] : ConfMap) {
        confmap[k] = v;
    }
    j["confmap"] = confmap;
    CWorkerMgr::Instance().SendMsgToAllWorkers(CChannel::MsgType::Json, j.dump());
    return true;
}

NAMESPACE_FRAMEWORK_END