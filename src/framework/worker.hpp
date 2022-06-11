#pragma once
#include "channel.hpp"
#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"
#include "singleton.hpp"

NAMESPACE_FRAMEWORK_BEGIN

// Entrance On Worker Thread
class CWorker : public CObject {
    std::map<CChannel::MsgType, std::function<void(std::string_view)>> m_left_callbacks;
    std::map<CChannel::MsgType, std::function<void(std::string_view)>> m_right_callbacks;

public:
    friend class CWorkerMgr;
    CWorker() = default;
    ~CWorker() = default;
    virtual bool Loop();

    CChannel& Channel() { return m_main_and_work_chan; }
    bool SendMsgToMain(const CChannel::MsgType type, std::string_view data);

    static void WaitForAllWorkers(const int32_t total);

    static thread_local std::shared_ptr<CContex> MAIN_CONTEX;

protected:
    virtual bool Init();
    virtual void Destroy() { }
    void OnLeftEvent(
        std::function<void(std::string_view)> textcb,
        std::function<void(std::string_view)> jsoncb,
        std::function<void(std::string_view)> binarycb);
    void OnRightEvent(
        std::function<void(std::string_view)> textcb,
        std::function<void(std::string_view)> jsoncb,
        std::function<void(std::string_view)> binarycb);

private:
    static void readOnLeft(bufferevent* bev, void* ctx);
    static void readOnRight(bufferevent* bev, void* ctx);
    static void initOk();

    static std::mutex m_mutex;
    static std::condition_variable m_cond;
    static int32_t m_init_threads;

    // L(Woker)<=========>R(Main)
    CChannel m_main_and_work_chan;

    DISABLE_CLASS_COPYABLE(CWorker);
};

class CWorkerMgr : public CObject, public CTLSingleton<CWorkerMgr> {
public:
    friend class CTLSingleton<CWorkerMgr>;
    CWorkerMgr() = default;
    ~CWorkerMgr() = default;
    bool Register(
        std::function<CWorker*()> createcb,
        std::function<void(std::string_view)> textcb,
        std::function<void(std::string_view)> jsoncb,
        std::function<void(std::string_view)> binarycb);

    CWorker* Create(const uint64_t total);
    bool SendMsgToAllWorkers(const CChannel::MsgType type, std::string_view data);

private:
    using WT = std::vector<std::unique_ptr<CWorker>>;
    WT m_mgr;
    std::function<CWorker*()> m_create_func = { nullptr };

    DISABLE_CLASS_COPYABLE(CWorkerMgr);
};

NAMESPACE_FRAMEWORK_END