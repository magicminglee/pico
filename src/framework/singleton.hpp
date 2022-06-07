#pragma once

#include "common.hpp"
#include <mutex>

NAMESPACE_FRAMEWORK_BEGIN

template <class T>
class CSingleton {
private:
    static T* NewInstance()
    {
        assert(nullptr == m_instance);
        m_instance = CNEW T();
        return m_instance;
    }

public:
    CSingleton() = default;
    ~CSingleton() = default;
    static T& Instance()
    {
        std::call_once(m_once_flag, NewInstance);
        assert(nullptr != m_instance);
        return *m_instance;
    }

private:
    static T* m_instance;
    static std::once_flag m_once_flag;
    DISABLE_CLASS_COPYABLE(CSingleton);
};

template <class T>
T* CSingleton<T>::m_instance = nullptr;

template <class T>
std::once_flag CSingleton<T>::m_once_flag;

//Thread Local Singleton
template <class T>
class CTLSingleton {
private:
    static T* NewInstance()
    {
        assert(nullptr == m_instance);
        m_instance = CNEW T();
        return m_instance;
    }

public:
    CTLSingleton() = default;
    ~CTLSingleton() = default;
    static T& Instance()
    {
        if (nullptr == m_instance)
            NewInstance();
        assert(nullptr != m_instance);
        return *m_instance;
    }

private:
    static thread_local T* m_instance;
    DISABLE_CLASS_COPYABLE(CTLSingleton);
};

template <class T>
thread_local T* CTLSingleton<T>::m_instance = nullptr;

NAMESPACE_FRAMEWORK_END
