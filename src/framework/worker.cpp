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
thread_local CWorker* CWorker::LOCAL_WORKER = nullptr;

bool CWorker::Loop()
{
    LOCAL_WORKER = this;
    CContex::MAIN_CONTEX.reset(CNEW CContex(event_base_new()));
    assert(nullptr != CContex::MAIN_CONTEX);

    if (!Init()) {
        initOk();
        return false;
    }

    initOk();

    CContex::MAIN_CONTEX->Loop();

    Destroy();

    return true;
}

void CWorker::readOnLeft(bufferevent* bev, void* ctx)
{
    CWorker* w = (CWorker*)ctx;
    auto res = w->m_main_and_work_chan.ReadL();
    for (auto v : res) {
        auto& [type, data] = v;
        if (type && data) {
            if (auto it = w->m_left_callbacks.find(type.value()); it != w->m_left_callbacks.end()) {
                it->second(data.value());
            }
        }
    }
}

void CWorker::readOnRight(bufferevent* bev, void* ctx)
{
    CWorker* w = (CWorker*)ctx;
    auto res = w->m_main_and_work_chan.ReadR();
    for (auto v : res) {
        auto& [type, data] = v;
        if (type && data) {
            if (auto it = w->m_right_callbacks.find(type.value()); it != w->m_right_callbacks.end()) {
                it->second(data.value());
            }
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
    if (!m_main_and_work_chan.CreateBvL(*CContex::MAIN_CONTEX, CWorker::readOnLeft, nullptr, nullptr, this)) {
        SPDLOG_ERROR("CTX:{} create channel fail", MYARGS.CTXID);
        return false;
    }
    MYARGS.CTXID = Name();
    MYARGS.Tid = Id();

    if (!CSSLContex::Instance().Init()) {
        SPDLOG_ERROR("CTX:{} init ssl contex", MYARGS.CTXID);
        return false;
    }

    return true;
}

bool CWorker::SendMsgToMain(const CChannel::MsgType type, std::string_view data)
{
    return m_main_and_work_chan.WriteL(type, data);
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
    w->OnRightEvent(m_textcb, m_jsoncb, m_binarycb);

    if (!w->m_main_and_work_chan.Create())
        return nullptr;

    if (!w->m_main_and_work_chan.CreateBvR(*CContex::MAIN_CONTEX, CWorker::readOnRight, nullptr, nullptr, w))
        return nullptr;

    return w;
}

bool CWorkerMgr::SendMsgToOneWorker(const CChannel::MsgType type, std::string_view data)
{
    if (m_mgr.empty())
        return false;
    m_round_index = (m_round_index + 1) % m_mgr.size();
    return m_mgr[m_round_index]->m_main_and_work_chan.WriteR(type, std::move(data));
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
        m_textcb = textcb;
        m_jsoncb = jsoncb;
        m_binarycb = binarycb;
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