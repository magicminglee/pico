#pragma once
#include "framework/argument.hpp"
#include "framework/chrono.hpp"
#include "framework/common.hpp"
#include "framework/connhandler.hpp"
#include "framework/global.hpp"
#include "framework/httpserver.hpp"
#include "framework/localstorage.hpp"
#include "framework/random.hpp"
#include "framework/service.hpp"
#include "framework/signal.hpp"
#include "framework/singleton.hpp"
#include "framework/ssl.hpp"
#include "framework/stringtool.hpp"
#include "framework/tcpserver.hpp"
#include "framework/udpserver.hpp"
#include "framework/utils.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

#include "mongo.hpp"
#include "redis.hpp"

#include "nlohmann/json.hpp"

NAMESPACE_FRAMEWORK_BEGIN

using namespace gamelibs::redis;
using namespace gamelibs::mongo;

class CApp {
    class AppWorker final : public CWorker {
        std::vector<std::shared_ptr<CHTTPServer>> m_http_server;
        std::vector<std::shared_ptr<CTCPServer>> m_tcp_server;

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
                            if ((j.contains("certificatefile") && j["certificatefile"].is_string()
                                    && j.contains("privatekeyfile") && j["privatekeyfile"].is_string())) {
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
                            if (j.contains("redisttl") && j["redisttl"].is_number_unsigned())
                                MYARGS.RedisTTL = j["redisttl"].get<uint64_t>();
                            if (j.contains("http2able") && j["http2able"].is_boolean())
                                MYARGS.Http2Able = j["http2able"].get<bool>();
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
                auto [schema, url] = CConnection::GetRealUri(v);
                if (schema == "http" || schema == "https") {
                    m_http_server.push_back(std::make_shared<CHTTPServer>());
                    auto ref = m_http_server.back();
                    if (!ref->Init(v)) {
                        CERROR("CTX:%s HttpServer::Init %s", MYARGS.CTXID.c_str(), v.c_str());
                        continue;
                    }
                    CApp::WebRegister(ref);
                } else if (schema == "tcp") {
                    m_tcp_server.push_back(std::make_shared<CTCPServer>());
                    auto ref = m_tcp_server.back();
                    CApp::TcpRegister(url, ref);
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
    static void WebRegister(std::shared_ptr<CHTTPServer> srv);
    static void TcpRegister(const std::string host, std::shared_ptr<CTCPServer> srv);
};

NAMESPACE_FRAMEWORK_END