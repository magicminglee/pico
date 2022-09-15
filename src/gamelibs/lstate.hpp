#pragma once
#include <string>

#include "framework/common.hpp"

#include "lua.hpp"

namespace gamelibs {
namespace lstate {
    class CLuaState {
    public:
        static thread_local std::unique_ptr<CLuaState> MAIN_LUASTATE;
        CLuaState();
        ~CLuaState();
        lua_State* operator()()
        {
            return m_l;
        }
        lua_State* Handle()
        {
            return m_l;
        }
        bool Init();
        void PushLString(const char* str, const int strlen);
        void PushString(const std::string& str);
        void PushFunc(const char* func);
        void PushInteger(unsigned int i);
        void PushNumber(double i);
        template <class T>
        T Call(const int args, const int rets)
        {
            lua_call(m_l, args, rets);
            if constexpr (std::is_same_v<T, int64_t>) {
                return lua_tointeger(m_l, -1);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return lua_tostring(m_l, -1);
            }
        }

    private:
        lua_State* m_l;
    };
}
}