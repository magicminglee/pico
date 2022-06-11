#include "argument.hpp"
#include "cxxopts.hpp"
#include "utils.hpp"
#include "worker.hpp"

#include "yaml-cpp/yaml.h"

NAMESPACE_FRAMEWORK_BEGIN

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

    if (config["main"] && config["main"].IsMap() && config["main"]["type"] && config["main"]["id"]) {
        Sid = CUtils::HashServerId(config["main"]["type"].as<std::uint16_t>(), config["main"]["id"].as<std::uint16_t>());
        Tid = 0;
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["daemonize"]) {
        Daemonize = config["main"]["daemonize"].as<bool>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["timezone"]) {
        TimeZone = config["main"]["timezone"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["logdir"]) {
        LogDir = config["main"]["logdir"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["confdir"]) {
        ConfDir = config["main"]["confdir"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["loglevel"]) {
        LogLevel = config["main"]["loglevel"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["verbose"]) {
        IsVerbose = config["main"]["verbose"].as<bool>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["isolate"]) {
        IsIsolate = config["main"]["isolate"].as<bool>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["webroot"]) {
        WebRootDir = config["main"]["webroot"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["certificatefile"]) {
        CertificateFile = config["main"]["certificatefile"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["privatekeyfile"]) {
        PrivateKeyFile = config["main"]["privatekeyfile"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["alloworigin"]) {
        IsAllowOrigin = config["main"]["alloworigin"].as<bool>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["modulename"]) {
        ModuleName = config["main"]["modulename"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["confhost"]) {
        ConfHost = config["main"]["confhost"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["interval"]) {
        Interval = config["main"]["interval"].as<std::uint32_t>() * 1000;
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["maxframesize"]) {
        MaxFrameSize = config["main"]["maxframesize"].as<std::uint32_t>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["redisurl"] && config["main"]["redisurl"].IsSequence()) {
        RedisUrl.clear();
        for (auto&& v : config["main"]["redisurl"]) {
            if (v.IsScalar()) {
                RedisUrl.push_back(v.as<std::string>());
            }
        }
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["mongourl"]) {
        MongoUrl = config["main"]["mongourl"].as<std::string>();
    }
    if (config["main"] && config["main"].IsMap() && config["main"]["dbconf"] && config["main"]["dbconf"].IsSequence()) {
        DbName.clear();
        DbColl.clear();
        for (auto&& v : config["main"]["dbconf"]) {
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
    if (config["main"] && config["main"].IsMap() && config["main"]["apikey"]) {
        ApiKey = config["main"]["apikey"].as<std::string>();
    }

    if (config["workers"] && config["workers"].IsSequence()) {
        Workers.clear();
        for (auto&& v : config["workers"]) {
            if (v.IsMap()) {
                auto& w = Workers.emplace_back();
                if (v["hosts"] && v["hosts"].IsSequence()) {
                    for (auto&& vv : v["hosts"])
                        w.Host.push_back(vv.as<std::string>());
                }
                if (v["inhosts"])
                    w.InHost = v["inhosts"].as<std::string>();
                if (v["subhosts"] && v["subhosts"].IsSequence()) {
                    for (auto&& vv : v["subhosts"])
                        w.SubHost.push_back(vv.as<std::string>());
                }
            }
        }
    }

    nlohmann::json j;
    j["cmd"] = "reload";
    j["loglevel"] = LogLevel.value();
    j["certificatefile"] = CertificateFile.value();
    j["privatekeyfile"] = PrivateKeyFile.value();
    j["alloworigin"] = IsAllowOrigin.value();
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
    CWorkerMgr::Instance().SendMsgToAllWorkers(CChannel::MsgType::Json, j.dump());
    return true;
}

NAMESPACE_FRAMEWORK_END