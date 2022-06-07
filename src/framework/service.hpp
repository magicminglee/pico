#pragma once
#include "common.hpp"
#include "contex.hpp"
#include "object.hpp"
#include "signal.hpp"
#include "worker.hpp"

NAMESPACE_FRAMEWORK_BEGIN

//Process Entrance On Main Thread
class CService : public CObject {
public:
    CService() = default;
    ~CService() = default;
    bool Loop();

public:
    virtual bool Init();
    virtual void Start();
    virtual void Destroy();
    virtual void Reload();

private:
    void InitSignal();
    void Stopping();
    void initWorker();

private:
    std::vector<std::thread> m_threads;
    CSignal m_signal;

    DISABLE_CLASS_COPYABLE(CService);
};

NAMESPACE_FRAMEWORK_END