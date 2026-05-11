#pragma once

#if NEXEN_USE_PACKAGED_LUA
extern "C" {
#include <lua.hpp>
}

extern "C" int luaopen_socket_core(lua_State* lua);
extern "C" int luaopen_mime_core(lua_State* lua);

typedef void (*lua_WatchDogHook)(lua_State* lua);
extern "C" void lua_setwatchdogtimer(lua_State* lua, lua_WatchDogHook func, int count);
#else
#include "Lua/lua.hpp"
#include "Lua/luasocket.hpp"
#endif
