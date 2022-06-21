#pragma once
#include "singleton.hpp"

NAMESPACE_FRAMEWORK_BEGIN
template <class T>
class CLocalStoreage : public CTLSingleton<CLocalStoreage<T>> {
public:
    CLocalStoreage() = default;
    ~CLocalStoreage() = default;
    void Set(const T& key, const std::string& val)
    {
        m_tls_map[key] = val;
    }
    std::optional<const std::string*> Get(const T& key)
    {
        auto it = m_tls_map.find(key);
        return it == std::end(m_tls_map) ? std::nullopt : std::optional(&it->second);
    }
    void Remove(const T& key)
    {
        if (auto it = m_tls_map.find(key); it != std::end(m_tls_map))
            m_tls_map.erase(it);
    }

private:
    std::unordered_map<T, std::string> m_tls_map;
};
NAMESPACE_FRAMEWORK_END