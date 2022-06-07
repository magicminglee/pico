#include "service.hpp"
#include "argument.hpp"
#include "signal.hpp"
#include "stringtool.hpp"
#include "worker.hpp"
#include "xlog.hpp"

NAMESPACE_FRAMEWORK_BEGIN

bool CService::Loop()
{
    CWorker::MAIN_CONTEX.reset(CNEW CContex());
    assert(nullptr != CWorker::MAIN_CONTEX);

    InitSignal();

    if (!Init())
        return false;

    initWorker();

    Start();

    CWorker::MAIN_CONTEX->Loop();

    Destroy();

    return true;
}

void CService::InitSignal()
{
    CWorker::MAIN_CONTEX->Register(
        SIGINT, EV_SIGNAL | EV_PERSIST, nullptr, [this]() -> void {
            this->Stopping();
        });
    CWorker::MAIN_CONTEX->Register(
        SIGTERM, EV_SIGNAL | EV_PERSIST, nullptr, [this]() -> void {
            this->Stopping();
        });
#if defined(LINUX_PLATFORMOS) || defined(DARWIN_PLATFORMOS)
    CWorker::MAIN_CONTEX->Register(
        SIGHUP, EV_SIGNAL | EV_PERSIST, nullptr, []() -> void {});
    CWorker::MAIN_CONTEX->Register(
        SIGPIPE, EV_SIGNAL | EV_PERSIST, nullptr, []() -> void {});
    CWorker::MAIN_CONTEX->Register(
        SIGUSR1, EV_SIGNAL | EV_PERSIST, nullptr, [this]() -> void {
            this->Reload();
        });
#endif
}

void CService::Stopping()
{
    CWorkerMgr::Instance().SendMsgToAllWorkers(CChannel::MsgType::Text, "stop");
    CWorker::MAIN_CONTEX->Exit(1);
}

void CService::initWorker()
{
    const auto MAX_WORKER_NUM = MYARGS.Workers.size();
    for (size_t i = 0; i < MAX_WORKER_NUM; ++i) {
        CWorker* w = CWorkerMgr::Instance().Create(MAX_WORKER_NUM);
        w->SetId(i);
        w->SetName(std::to_string(MYARGS.Sid.value()) + std::string("_") + std::to_string(i));
        CArgument arg = MYARGS;
        if (w) {
            m_threads.push_back(std::thread([w, arg]() { MYARGS = arg; w->Loop(); }));
        }
    }
    CWorker::WaitForAllWorkers(MAX_WORKER_NUM);
}

bool CService::Init()
{
    return true;
}

void CService::Start()
{
}

void CService::Destroy()
{
    for (auto& v : m_threads)
        v.join();
}

void CService::Reload()
{
    MYARGS.ParseYaml();
    Init();
}

NAMESPACE_FRAMEWORK_END