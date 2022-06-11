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

            if (!CApp::Init()) {
                CERROR("CTX:%s CApp::Init fail", MYARGS.CTXID.c_str());
                return false;
            }
            if (!CMongo::Instance().GetOne()) {
                CERROR("CTX:%s CMongo::GetOne fail", MYARGS.CTXID.c_str());
                return false;
            }
            if(!CRedisMgr::Instance().Init()){
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

    static bool Init();
    static void Register(std::shared_ptr<CHTTPServer> hs);
};

NAMESPACE_FRAMEWORK_END