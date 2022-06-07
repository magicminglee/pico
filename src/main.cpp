#include "framework/argument.hpp"
#include "framework/chrono.hpp"
#include "framework/common.hpp"
#include "framework/connection.hpp"
#include "framework/connhandler.hpp"
#include "framework/decoder.hpp"
#include "framework/encoder.hpp"
#include "framework/fixed_thread_pool.hpp"
#include "framework/global.hpp"
#include "framework/http.hpp"
#include "framework/msgparser.hpp"
#include "framework/protocol.hpp"
#include "framework/random.hpp"
#include "framework/service.hpp"
#include "framework/signal.hpp"
#include "framework/singleton.hpp"
#include "framework/stringtool.hpp"
#include "framework/tcpserver.hpp"
#include "framework/udpserver.hpp"
#include "framework/utils.hpp"
#include "framework/websocket.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

#include "gamelibs/httpcli.hpp"
#include "gamelibs/mongo.hpp"
#include "gamelibs/redis.hpp"

#include "nlohmann/json.hpp"

USE_NAMESPACE_FRAMEWORK

using namespace gamelibs::redis;
using namespace gamelibs::httpcli;
using namespace gamelibs::mongo;

class ExternalTCPConnectionMgr : public CConnectionMgr,
                                 public CTLSingleton<ExternalTCPConnectionMgr> {
};

class InternalTCPConnectionMgr : public CConnectionMgr,
                                 public CTLSingleton<InternalTCPConnectionMgr> {
};

class SubscribeTCPConnectionMgr : public CConnectionMgr,
                                  public CTLSingleton<SubscribeTCPConnectionMgr> {
};

class ExternalMsgParser : public CMsgParser,
                          public CTLSingleton<ExternalMsgParser> {
public:
    virtual bool Init() final
    {
        CheckCondition(!GetSize(), true);
        return true;
    }
};

class InternalMsgParser : public CMsgParser,
                          public CTLSingleton<InternalMsgParser> {
public:
    virtual bool Init() final
    {
        CheckCondition(!GetSize(), true);

        return true;
    }
};

class TXWorker final : public CWorker {
public:
    virtual bool Init() final
    {
        CheckCondition(CWorker::Init(), false);
        XLOG::LogInit(MYARGS.LogLevel, MYARGS.ModuleName, MYARGS.LogDir,
            MYARGS.IsVerbose, MYARGS.IsIsolate);

        nlohmann::json j(MYARGS.Workers[Id()].Host);
        CINFO("CTX:%s id %ld init worker listenning on hosts %s logdir %s",
            MYARGS.CTXID.c_str(), Id(), j.dump().c_str(),
            MYARGS.LogDir.value().c_str());

        CheckCondition(CMongo::Instance().GetOne(), false);
        CheckCondition(ExternalMsgParser::Instance().Init(), false);
        CheckCondition(InternalMsgParser::Instance().Init(), false);
        CheckCondition(CRedisMgr::Instance().Init(), false);
        CheckCondition(CHttpContex::Instance().Init(), false);
        if (MYARGS.CertificateFile && MYARGS.PrivateKeyFile)
            CheckCondition(CHttpContex::Instance().LoadCertificateAndPrivateKey(MYARGS.CertificateFile.value(), MYARGS.PrivateKeyFile.value()), false);

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
                    }
                } catch (const nlohmann::json::exception& e) {
                    CERROR("CTX:%s %s json exception %s", MYARGS.CTXID.c_str(), __FUNCTION__, e.what());
                }
            },
            nullptr);

        auto v = (*MongoCtx)->list_database_names();
        for (auto& vv : v) {
            CINFO("DB:%s", vv.c_str());
        }
        return true;
    }
};
class TXService final : public CService {

public:
    virtual bool Init() final
    {
        CheckCondition(CService::Init(), false);
        CWorkerMgr::Instance().Register(
            []() {
                return CNEW TXWorker();
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

int main(int argc, char** argv)
{
    CheckCondition(MYARGS.ParseArg(argc, argv, "TX", "TX Program"), EXIT_FAILURE);

    CGlobal::InitGlobal();

#ifdef LINUX_PLATFORMOS
    if (MYARGS.Daemonize && MYARGS.Daemonize.value() && -1 == daemon(1, 0)) {
        std::cout << "errmsg(" << errno << "):" << strerror(errno) << std::endl;
        return EXIT_FAILURE;
    }
#endif
    XLOG::LogInit(MYARGS.LogLevel, MYARGS.ModuleName, MYARGS.LogDir,
        MYARGS.IsVerbose, MYARGS.IsIsolate);

    TXService s;
    s.Loop();

    return EXIT_SUCCESS;
}
