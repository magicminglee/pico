#pragma once

#include "common.hpp"
#include "object.hpp"
#include "utils.hpp"

NAMESPACE_FRAMEWORK_BEGIN

class CConnectionHandler;
class CMsgParser : public CObject {
public:
    using Callback = std::function<bool(const int64_t, CConnectionHandler*, const std::string_view msg)>;

protected:
    CMsgParser() = default;
    ~CMsgParser() = default;
    void Register(uint16_t maincmd, uint16_t subcmd, Callback f)
    {
        m_mgr[CUtils::HashCmdId(maincmd, subcmd)] = std::move(f);
    }
    const uint32_t GetSize() const { return m_mgr.size(); }

public:
    virtual bool Init() { return true; }
    virtual bool Dispatch(const int64_t linkid, CConnectionHandler* h, const uint32_t cmdid, const std::string_view msg)
    {
        auto it = m_mgr.find(cmdid);
        return it == m_mgr.end() ? false : (it->second)(linkid, h, msg);
    }

private:
    using MsgMap = std::map<uint32_t, Callback>;
    MsgMap m_mgr;
};

NAMESPACE_FRAMEWORK_END