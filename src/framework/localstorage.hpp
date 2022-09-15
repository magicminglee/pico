#pragma once
#include "singleton.hpp"

NAMESPACE_FRAMEWORK_BEGIN
template <class T = int64_t>
class CLocalStorage : public CTLSingleton<CLocalStorage<T>> {
    using VMAP = std::unordered_map<T, std::string>;
    using KVMAP = std::unordered_map<std::string, VMAP>;

public:
    CLocalStorage() = default;
    ~CLocalStorage() = default;
    void Set(const std::string& key, const T& field, const std::string& val)
    {
        m_tls_map[key][field] = val;
    }
    std::optional<const std::string*> Get(const std::string& key, const T& field)
    {
        auto it = m_tls_map.find(key);
        if (it != std::end(m_tls_map)) {
            auto it1 = it->second.find(field);
            if (it1 != std::end(it->second)) {
                return std::optional(&it1->second);
            }
        }
        return std::nullopt;
    }
    void Remove(const std::string& key, const T& field)
    {
        auto it = m_tls_map.find(key);
        if (it != std::end(m_tls_map)) {
            auto it1 = it->second.find(field);
            if (it1 != std::end(it->second)) {
                it->second.erase(it1);
                if (it1->second.empty())
                    m_tls_map.erase(it);
            }
        }
    }

    void RemoveAll(const std::string& key)
    {
        if (auto it = m_tls_map.find(key); it != std::end(m_tls_map)) {
            m_tls_map.erase(it);
        }
    }
    bool Foreach(const std::string& key, std::function<bool(const T&, const std::string&)> cb)
    {
        if (auto it = m_tls_map.find(key); it != std::end(m_tls_map) && cb) {
            for (auto& [k, v] : it->second) {
                if (!cb(k, v))
                    return false;
            }
        }
        return true;
    }

private:
    KVMAP m_tls_map;
};
NAMESPACE_FRAMEWORK_END