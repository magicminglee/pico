#include "worker.hpp"
#include "argument.hpp"
#include "ssl.hpp"
#include "stringtool.hpp"
#include "xlog.hpp"

#include "nlohmann/json.hpp"

NAMESPACE_FRAMEWORK_BEGIN

std::mutex CWorker::m_mutex;
std::condition_variable CWorker::m_cond;
int32_t CWorker::m_init_threads;
thread_local std::shared_ptr<CContex> CWorker::MAIN_CONTEX = nullptr;

bool CWorker::Loop()
{
    MAIN_CONTEX.reset(CNEW CContex());
    assert(nullptr != MAIN_CONTEX);

    if (!Init()) {
        initOk();
        return false;
    }

    initOk();

    MAIN_CONTEX->Loop();

    Destroy();

    return true;
}

void CWorker::readOnLeft(bufferevent* bev, void* ctx)
{
    CWorker* w = (CWorker*)ctx;
    std::optional<CChannel::MsgType> type;
    std::optional<std::string_view> data;
    std::tie(type, data) = w->m_main_and_work_chan.ReadL();
    if (type && data) {
        if (auto it = w->m_left_callbacks.find(type.value()); it != w->m_left_callbacks.end()) {
            it->second(std::move(data.value()));
        }
    }
}

void CWorker::readOnRight(bufferevent* bev, void* ctx)
{
    CWorker* w = (CWorker*)ctx;
    std::optional<CChannel::MsgType> type;
    std::optional<std::string_view> data;
    std::tie(type, data) = w->m_main_and_work_chan.ReadR();
    if (type && data) {
        if (auto it = w->m_right_callbacks.find(type.value()); it != w->m_right_callbacks.end()) {
            it->second(std::move(data.value()));
        }
    }
}

void CWorker::OnLeftEvent(
    std::function<void(std::string_view)> textcb,
    std::function<void(std::string_view)> jsoncb,
    std::function<void(std::string_view)> binarycb)
{
    m_left_callbacks[CChannel::MsgType::Text] = std::move(textcb);
    m_left_callbacks[CChannel::MsgType::Json] = std::move(jsoncb);
    m_left_callbacks[CChannel::MsgType::Binary] = std::move(binarycb);
}

void CWorker::OnRightEvent(
    std::function<void(std::string_view)> textcb,
    std::function<void(std::string_view)> jsoncb,
    std::function<void(std::string_view)> binarycb)
{
    m_right_callbacks[CChannel::MsgType::Text] = std::move(textcb);
    m_right_callbacks[CChannel::MsgType::Json] = std::move(jsoncb);
    m_right_callbacks[CChannel::MsgType::Binary] = std::move(binarycb);
}

bool CWorker::Init()
{
    if (!m_main_and_work_chan.CreateBvL(*MAIN_CONTEX, CWorker::readOnLeft, nullptr, nullptr, this)) {
        CERROR("CTX:%s create channel fail", MYARGS.CTXID.c_str());
        return false;
    }
    MYARGS.LogDir = MYARGS.LogDir.value() + "w" + Name();
    MYARGS.CTXID = Name();
    MYARGS.Tid = Id();

    XLOG::LogInit(MYARGS.LogLevel, MYARGS.ModuleName, MYARGS.LogDir, MYARGS.IsVerbose, MYARGS.IsIsolate);
    if (!CSSLContex::Instance().Init()) {
        CERROR("CTX:%s init ssl contex fail", MYARGS.CTXID.c_str());
        return false;
    }
    if (MYARGS.CertificateFile && MYARGS.PrivateKeyFile) {
        if (!CSSLContex::Instance().LoadCertificateAndPrivateKey(MYARGS.CertificateFile.value(), MYARGS.PrivateKeyFile.value())) {
            CERROR("CTX:%s load ssl certificate and private key fail", MYARGS.CTXID.c_str());
            return false;
        }
    }

    return true;
}

bool CWorker::SendMsgToMain(const CChannel::MsgType type, std::string_view data)
{
    return m_main_and_work_chan.WriteL(type, std::move(data));
}

void CWorker::initOk()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_init_threads++;
    }
    m_cond.notify_one();
}

CWorker* CWorkerMgr::Create(const uint64_t total)
{
    if (!m_create_func)
        return nullptr;
    // overload
    if (m_mgr.size() >= total) {
        return nullptr;
    }
    m_mgr.emplace_back().reset(m_create_func());

    CWorker* w = m_mgr.back().get();

    if (!w->m_main_and_work_chan.Create())
        return nullptr;

    if (!w->m_main_and_work_chan.CreateBvR(*CWorker::MAIN_CONTEX, CWorker::readOnRight, nullptr, nullptr, w))
        return nullptr;

    return w;
}

void CWorker::WaitForAllWorkers(const int32_t total)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    while (m_init_threads < total) {
        m_cond.wait(lk);
    }
}

bool CWorkerMgr::Register(
    std::function<CWorker*()> createcb,
    std::function<void(std::string_view)> textcb,
    std::function<void(std::string_view)> jsoncb,
    std::function<void(std::string_view)> binarycb)
{
    if (!m_create_func) {
        m_create_func = std::move(createcb);
        for (auto&& w : m_mgr) {
            w->OnRightEvent(std::move(textcb), std::move(jsoncb), std::move(binarycb));
        }
    }
    return true;
}

bool CWorkerMgr::SendMsgToAllWorkers(const CChannel::MsgType type, std::string_view data)
{
    for (auto& w : m_mgr) {
        w->m_main_and_work_chan.WriteR(type, std::move(data));
    }
    return true;
}

NAMESPACE_FRAMEWORK_END