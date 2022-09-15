#include "lstate.hpp"

#include "framework/common.hpp"
#include "framework/xlog.hpp"

namespace gamelibs {
namespace lstate {
    thread_local std::unique_ptr<CLuaState> CLuaState::MAIN_LUASTATE = nullptr;
    CLuaState::CLuaState()
        : m_l(nullptr)
    {
    }

    CLuaState::~CLuaState()
    {
        if (m_l)
            lua_close(m_l);
    }

    bool CLuaState::Init()
    {
        if (m_l)
            return true;
        m_l = luaL_newstate();
        luaL_openlibs(m_l);
        return true;
    }

    void CLuaState::PushLString(const char* str, const int strlen)
    {
        lua_pushlstring(m_l, str, strlen);
    }

    void CLuaState::PushString(const std::string& str)
    {
        PushLString(str.data(), str.length());
    }

    void CLuaState::PushInteger(unsigned int i)
    {
        lua_pushinteger(m_l, i);
    }

    void CLuaState::PushNumber(double i)
    {
        lua_pushnumber(m_l, i);
    }

    void CLuaState::PushFunc(const char* func)
    {
        lua_getglobal(m_l, func);
    }
}
}