#include <limits>

#include "framework/argument.hpp"
#include "framework/chrono.hpp"
#include "framework/common.hpp"
#include "framework/contex.hpp"
#include "framework/random.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

#include "lstate.hpp"

#include "fmt/format.h"

USE_NAMESPACE_FRAMEWORK

using namespace gamelibs::lstate;

///////////////////////////////////// random api ///////////////////////////////////////////
static int c_rand(lua_State* L)
{
    lua_pushinteger(L, RANDOM.Rand(1, std::numeric_limits<int>::max()));
    return 1;
}

static int c_rand_between(lua_State* L)
{
    int l = static_cast<int>(luaL_checkinteger(L, 1));
    int h = static_cast<int>(luaL_checkinteger(L, 2));
    int t = static_cast<int>(lua_tonumber(L, 3));
    if (l <= h) {
        lua_pushinteger(L, RANDOM.Rand(l, h));
    } else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

static int c_seed(lua_State* L)
{
    srand(time(0));
    return 0;
}

///////////////////////////////////// utils api ///////////////////////////////////////////
static int c_dir(lua_State* L)
{
    std::vector<std::string> vec;
    const char* d = luaL_checkstring(L, 1);
    const std::filesystem::path p(d);
    try {
        for (auto& e : std::filesystem::directory_iterator { p }) {
            const std::string v = e.path().c_str();
            vec.push_back(v);
        }
    } catch (std::exception& e) {
        SPDLOG_ERROR("dir api {}", e.what());
        return 0;
    }
    lua_newtable(L);
    auto i = 0;
    for (auto& v : vec) {
        lua_pushstring(L, v.c_str());
        lua_rawseti(L, -2, ++i);
    }

    return 1;
}

///////////////////////////////////// logger api ///////////////////////////////////////////
int c_debug(lua_State* L)
{
    if (lua_isstring(L, 1)) {
        SPDLOG_INFO("{}", fmt::format("{}", lua_tostring(L, 1)).c_str());
    } else {
        SPDLOG_ERROR("{} argument is not a string", __FUNCTION__);
    }
    return 0;
}

int c_info(lua_State* L)
{
    if (lua_isstring(L, 1)) {
        SPDLOG_INFO("{}", lua_tostring(L, 1));
    } else {
        SPDLOG_ERROR("{} argument is not a string", __FUNCTION__);
    }
    return 0;
}

int c_error(lua_State* L)
{
    if (lua_isstring(L, 1)) {
        SPDLOG_ERROR("{}", lua_tostring(L, 1));
    } else {
        SPDLOG_ERROR("{} argument is not a string", __FUNCTION__);
    }
    return 0;
}

///////////////////////////////////// timer api ///////////////////////////////////////////
struct TimerArgs {
    lua_State* l;
    int func_r;
    int func_arg_r;
};
using TimerArgsInterMap = std::map<int, TimerArgs*>;
using TimerArgsInterMapIter = TimerArgsInterMap::iterator;
using TimerArgsMap = std::map<int, TimerArgsInterMap>;
using TimerArgsMapIter = TimerArgsMap::iterator;
static thread_local TimerArgsMap _s_timer_arguments_map;

static int c_create(lua_State* L)
{
    CContex* p = CNEW CContex(nullptr);
    lua_pushlightuserdata(L, p);
    int r = luaL_ref(L, LUA_REGISTRYINDEX);
    SPDLOG_DEBUG("new timer:{},{}", fmt::ptr(p), r);
    lua_pushinteger(L, r);
    return 1;
}

static int c_destroy(lua_State* L)
{
    int r = luaL_checkinteger(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);
    CContex* p = (CContex*)lua_touserdata(L, -1);
    if (p) {
        TimerArgsMapIter it = _s_timer_arguments_map.find(r);
        if (it != _s_timer_arguments_map.end()) {
            for (TimerArgsInterMapIter iit(it->second.begin()), iend(it->second.end()); iit != iend; ++iit) {
                TimerArgs* args = iit->second;
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_r);
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_arg_r);
                CDEL(args);
            }
            SPDLOG_DEBUG("delete timer: {} {}", fmt::ptr(p), r);
            luaL_unref(L, LUA_REGISTRYINDEX, r);
            _s_timer_arguments_map.erase(it);
            CDEL(p);
        } else {
            SPDLOG_DEBUG("no tick timer");
            luaL_unref(L, LUA_REGISTRYINDEX, r);
            CDEL(p);
        }
    } else {
        SPDLOG_ERROR("no timer:{}", r);
    }
    lua_pop(L, 1);
    return 0;
}

/*call(ref, id, elapse, func, func_args)*/
static int c_tick(lua_State* L)
{
    int r = luaL_checkinteger(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);

    CContex* p = (CContex*)lua_touserdata(L, -1);
    if (p) {
        int id = luaL_checkinteger(L, 2);
        int time = luaL_checkinteger(L, 3);
        TimerArgsMapIter it = _s_timer_arguments_map.find(r);
        if (it != _s_timer_arguments_map.end()) {
            TimerArgsInterMap& im = it->second;
            TimerArgsInterMapIter iit = im.find(id);
            if (iit != im.end()) {
                TimerArgs* args = iit->second;
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_r);
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_arg_r);
                p->DelEvent(id);
                SPDLOG_DEBUG("timer has been started:{},{},{} {} {}", r, id, fmt::ptr(args), args->func_r, args->func_arg_r);
                CDEL(args);
                im.erase(id);
            }
        }

        TimerArgs* args = CNEW TimerArgs();
        args->l = L;
        int use_mainthread = lua_isnil(L, 6);
        if (!use_mainthread) {
            args->l = CLuaState::MAIN_LUASTATE->Handle();
        }
        lua_pushvalue(L, 4);
        args->func_r = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_pushvalue(L, 5);
        args->func_arg_r = luaL_ref(L, LUA_REGISTRYINDEX);
        _s_timer_arguments_map[r][id] = args;
        p->AddPersistEvent(id, time, [args]() {
            TimerArgs* ta = (TimerArgs*)args;
            if (LUA_OK != lua_status(ta->l)) {
                SPDLOG_ERROR("lua status {}", lua_status(ta->l));
                return;
            }
            lua_rawgeti(ta->l, LUA_REGISTRYINDEX, ta->func_r);
            lua_rawgeti(ta->l, LUA_REGISTRYINDEX, ta->func_arg_r);
            lua_call(ta->l, 1, 0);
        });

        SPDLOG_DEBUG("start timer:{},{},{},{},{},{}", r, id, time, fmt::ptr(args), args->func_r, args->func_arg_r);
    } else {
        SPDLOG_ERROR("no timer:{}", r);
    }
    lua_pop(L, 1);
    return 0;
}

/*call(ref, id)*/
static int c_cancel(lua_State* L)
{
    int r = luaL_checkinteger(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, r);
    CContex* p = (CContex*)lua_touserdata(L, -1);
    if (p) {
        int id = luaL_checkinteger(L, 2);
        TimerArgsMapIter it = _s_timer_arguments_map.find(r);
        if (it != _s_timer_arguments_map.end()) {
            TimerArgsInterMap& im = it->second;
            TimerArgsInterMapIter iit = im.find(id);
            if (iit != im.end()) {
                TimerArgs* args = iit->second;
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_r);
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_arg_r);
                p->DelEvent(id);
                // SPDLOG_DEBUG("cancel timer:{},{},{} {}", r, id, args->func_r, args->func_arg_r);
                CDEL(args);
                im.erase(id);
            } else {
                // SPDLOG_DEBUG("timer:{},{},{} has been canceled", fmt::ptr(p), r, id);
            }
        } else {
            SPDLOG_DEBUG("{} cancel find no timer:{},{}", fmt::ptr(p), r, id);
        }

    } else {
        SPDLOG_ERROR("no timer:{}", r);
    }
    lua_pop(L, 1);

    return 0;
}

static int c_timer_clean(lua_State* L)
{
    for (TimerArgsMapIter it(_s_timer_arguments_map.begin()), end(_s_timer_arguments_map.end()); it != end; ++it) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->first);
        CContex* p = (CContex*)lua_touserdata(L, -1);
        if (p) {
            TimerArgsInterMap& im = it->second;
            for (TimerArgsInterMapIter iter(im.begin()), ender(im.end()); iter != ender; ++iter) {
                TimerArgs* args = iter->second;
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_r);
                luaL_unref(L, LUA_REGISTRYINDEX, args->func_arg_r);
                p->DelEvent(iter->first);
                CDEL(args);
            }
            im.clear();
            luaL_unref(L, LUA_REGISTRYINDEX, it->first);
            CDEL(p);
        }
        lua_pop(L, 1);
    }
    SPDLOG_DEBUG("clean timer");
    _s_timer_arguments_map.clear();
    return 0;
}

extern "C" {
static int c_myargs(lua_State* L)
{
    lua_pushstring(L, "ARGS");

    lua_newtable(L);
    lua_pushstring(L, "scriptdir");
    lua_pushstring(L, MYARGS.ScriptDir.value_or("").c_str());
    lua_settable(L, -3);

    lua_pushstring(L, "sid");
    lua_pushinteger(L, MYARGS.Sid.value_or(0));
    lua_settable(L, -3);

    lua_pushstring(L, "tid");
    lua_pushinteger(L, MYARGS.Tid.value_or(0));
    lua_settable(L, -3);

    lua_settable(L, -3);

    return 0;
}

int luaopen_luabind(lua_State* L)
{
#if LUA_VERSION_NUM <= 501
#error No defined Constant Symbol CONST_NAME
#else
    luaL_checkversion(L);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "CAPI");

    {
        luaL_Reg reg[] = {
            { "rand", c_rand },
            { "rand_between", c_rand_between },
            { "seed", c_seed },
            { NULL, NULL },
        };
        lua_pushstring(L, "RAND");
        luaL_newlib(L, reg);
        lua_settable(L, -3);
    }
    {
        luaL_Reg reg[] = {
            { "debug", c_debug },
            { "info", c_info },
            { "error", c_error },
            { NULL, NULL },
        };
        lua_pushstring(L, "LOG");
        luaL_newlib(L, reg);
        lua_settable(L, -3);
    }
    {
        luaL_Reg reg[] = {
            { "dir", c_dir },
            { NULL, NULL },
        };
        lua_pushstring(L, "UTILS");
        luaL_newlib(L, reg);
        lua_settable(L, -3);
    }
    {
        luaL_Reg reg[] = {
            { "create", c_create },
            { "destroy", c_destroy },
            { "tick", c_tick },
            { "cancel", c_cancel },
            { "timer_clean", c_timer_clean },
            { NULL, NULL },
        };
        lua_pushstring(L, "TIMER");
        luaL_newlib(L, reg);
        lua_settable(L, -3);
    }

    c_myargs(L);
    lua_pop(L, 1);
#endif
    return 0;
}
}
