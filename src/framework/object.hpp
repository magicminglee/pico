#pragma once
#include "common.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CObject {
public:
    CObject() = default;
    CObject(CObject&& rhs)
        : m_id(rhs.m_id)
        , m_name(std::move(rhs.m_name))
    {
    }
    virtual ~CObject() = default;
    constexpr auto Id() { return m_id; }
    constexpr const auto& Name() { return m_name; }
    constexpr void SetId(const int64_t id) { m_id = id; }
    void SetName(const std::string& name) { m_name = name; }

protected:
    int64_t m_id = { 0 };
    std::string m_name;
};

NAMESPACE_FRAMEWORK_END