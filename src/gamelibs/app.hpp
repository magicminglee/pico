#pragma once
#include "framework/argument.hpp"
#include "framework/chrono.hpp"
#include "framework/common.hpp"
#include "framework/connhandler.hpp"
#include "framework/global.hpp"
#include "framework/httpserver.hpp"
#include "framework/random.hpp"
#include "framework/service.hpp"
#include "framework/signal.hpp"
#include "framework/singleton.hpp"
#include "framework/ssl.hpp"
#include "framework/stringtool.hpp"
#include "framework/tcpserver.hpp"
#include "framework/udpserver.hpp"
#include "framework/utils.hpp"
#include "framework/websocket.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

#include "httpcli.hpp"
#include "mongo.hpp"
#include "redis.hpp"

#include "nlohmann/json.hpp"

NAMESPACE_FRAMEWORK_BEGIN

using namespace gamelibs::redis;
using namespace gamelibs::httpcli;
using namespace gamelibs::mongo;

class CApp {
    class AppWorker final : public CWorker {
        std::vector<std::shared_ptr<CHTTPServer>> m_http_server;

    public:
        virtual bool Init() final
        {
            CheckCondition(CWorker::Init(), false);

            OnLeftEvent(
                [](std::string_view data) {
                    if (data == "stop") {
                        CWorker::MAIN_CONTEX->Exit(0);
                    }
                },
                [](std::string_view data) {
                    try {
                        auto j = nlohmann::json::parse(std::move(data));
                        if (j["cmd"].is_string() && j["cmd"].get_ref<const std::string&>() == "reload") {
                            if (j.contains("loglevel") && j["loglevel"].is_string()) {
                                MYARGS.LogLevel = j["loglevel"].get<std::string>();
                                XLOG::WARN_W.UpdateLogLevel(XLOG::LogWriter::LogStrToLogLevel(MYARGS.LogLevel.value()));
                                XLOG::INFO_W.UpdateLogLevel(XLOG::LogWriter::LogStrToLogLevel(MYARGS.LogLevel.value()));
                                XLOG::DBLOG_W.UpdateLogLevel(XLOG::LogWriter::LogStrToLogLevel(MYARGS.LogLevel.value()));
                            }
                            if (j.contains("certificatefile") && j["certificatefile"].is_string()
                                && j.contains("privatekeyfile") && j["privatekeyfile"].is_string()) {
                                MYARGS.CertificateFile = j["certificatefile"].get<std::string>();
                                MYARGS.PrivateKeyFile = j["privatekeyfile"].get<std::string>();
                                CSSLContex::Instance().LoadCertificateAndPrivateKey(MYARGS.CertificateFile.value(), MYARGS.PrivateKeyFile.value());
                            }
                            if (j.contains("alloworigin") && j["alloworigin"].is_boolean())
                                MYARGS.IsAllowOrigin = j["alloworigin"].get<bool>();
                            if (j.contains("forcehttps") && j["forcehttps"].is_boolean())
                                MYARGS.IsForceHttps = j["forcehttps"].get<bool>();
                            if (j.contains("redirectstatus") && j["redirectstatus"].is_number_unsigned())
                                MYARGS.RedirectStatus = j["redirectstatus"].get<uint16_t>();
                            if (j.contains("redirecturl") && j["redirecturl"].is_string())
                                MYARGS.RedirectUrl = j["redirecturl"].get<std::string>();
                            if (j.contains("apikey") && j["apikey"].is_string())
                                MYARGS.ApiKey = j["apikey"].get<std::string>();
                            if (j.contains("tokenexpire") && j["tokenexpire"].is_number_unsigned())
                                MYARGS.TokenExpire = j["tokenexpire"].get<uint32_t>();
                        }
                        CINFO("CTX:%s reload config %s", MYARGS.CTXID.c_str(), j.dump().c_str());
                    } catch (const nlohmann::json::exception& e) {
                        CERROR("CTX:%s %s json exception %s", MYARGS.CTXID.c_str(), __FUNCTION__, e.what());
                    }
                },
                nullptr);

            if (!CApp::Init()) {
                CERROR("CTX:%s CApp::Init fail", MYARGS.CTXID.c_str());
                return false;
            }
            if (!CRedisMgr::Instance().Init()) {
                CERROR("CTX:%s CRedisMgr::Init fail", MYARGS.CTXID.c_str());
                return false;
            }

            for (auto& v : MYARGS.Workers[Id()].Host) {
                auto [schema, hostname, port] = CConnection::SplitUri(v);
                auto schem = CConnection::GetRealSchema(schema);
                if (schem == "http" || schem == "https") {
                    m_http_server.push_back(std::make_shared<CHTTPServer>());
                    auto ref = m_http_server.back();
                    if (!ref->Init(v)) {
                        CERROR("CTX:%s HttpServer::Init %s fail", MYARGS.CTXID.c_str(), v.c_str());
                        continue;
                    }
                    CApp::Register(ref);
                }
            }

            return true;
        }
    };

    class AppService final : public CService {

    public:
        virtual bool Init() final
        {
            CheckCondition(CService::Init(), false);
            CWorkerMgr::Instance().Register(
                []() {
                    return CNEW AppWorker();
                },
                nullptr,
                nullptr,
                nullptr);

            CheckCondition(CMongo::Instance().Init(MYARGS.MongoUrl.value()), false);
#ifdef PLATFORMOS
            CINFO("Service has been initiated on platfrom %s", PLATFORMOS);
#endif
            return true;
        }
        virtual void Start() final
        {
        }
    };

public:
    static bool Start(int argc, char** argv)
    {
        CheckCondition(MYARGS.ParseArg(argc, argv, "Pico", "Pico Program"), false);
        CGlobal::InitGlobal();

        XLOG::LogInit(MYARGS.LogLevel, MYARGS.ModuleName, MYARGS.LogDir,
            MYARGS.IsVerbose, MYARGS.IsIsolate);

        AppService s;
        s.Loop();
        return true;
    }

    static bool Init() { return true; }
    static void Register(std::shared_ptr<CHTTPServer> hs);
};

NAMESPACE_FRAMEWORK_END