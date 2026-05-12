#include "pch.h"
#include <mutex>
#include <unordered_map>
#include "Debugger/LuaInterop.h"

#if NEXEN_USE_PACKAGED_LUA

#pragma comment(lib, "lua.lib")

namespace {
	struct LuaWatchdogState {
		lua_WatchDogHook Hook = nullptr;
		int Count = 0;
	};

	std::mutex g_luaWatchdogLock;
	std::unordered_map<lua_State*, LuaWatchdogState> g_luaWatchdogs;

	void LuaWatchdogDispatch(lua_State* lua, lua_Debug* ar) {
		(void)ar;

		lua_WatchDogHook hook = nullptr;
		{
			std::lock_guard<std::mutex> lock(g_luaWatchdogLock);
			auto it = g_luaWatchdogs.find(lua);
			if (it != g_luaWatchdogs.end()) {
				hook = it->second.Hook;
			}
		}

		if (hook) {
			hook(lua);
		}
	}
}

extern "C" int luaopen_socket_core(lua_State* lua) {
	return luaL_error(lua, "socket.core is unavailable in package-managed Lua mode.");
}

extern "C" int luaopen_mime_core(lua_State* lua) {
	return luaL_error(lua, "mime.core is unavailable in package-managed Lua mode.");
}

extern "C" void lua_setwatchdogtimer(lua_State* lua, lua_WatchDogHook func, int count) {
	if (!lua) {
		return;
	}

	if (func && count > 0) {
		{
			std::lock_guard<std::mutex> lock(g_luaWatchdogLock);
			g_luaWatchdogs[lua] = LuaWatchdogState{ func, count };
		}

		lua_sethook(lua, LuaWatchdogDispatch, LUA_MASKCOUNT, count);
	} else {
		{
			std::lock_guard<std::mutex> lock(g_luaWatchdogLock);
			g_luaWatchdogs.erase(lua);
		}

		lua_sethook(lua, nullptr, 0, 0);
	}
}

#endif
