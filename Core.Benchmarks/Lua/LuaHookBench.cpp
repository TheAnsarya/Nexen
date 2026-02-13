#include "pch.h"
#include <benchmark/benchmark.h>
#include "Lua/lua.hpp"

// =============================================================================
// Lua Hook Performance Benchmarks
// =============================================================================
// Compare standard lua_sethook (LUA_MASKCOUNT) vs custom watchdog timer.
// This is relevant for issue #239: Replace Vendored Lua with Library Dependency.

namespace {

// Counter for hook invocations
thread_local int64_t g_hookCallCount = 0;

// Standard Lua hook signature
void StandardHook(lua_State* L, lua_Debug* ar) {
	(void)L;
	(void)ar;
	++g_hookCallCount;
}

// Custom watchdog hook signature (Nexen modification)
void WatchdogHook(lua_State* L) {
	(void)L;
	++g_hookCallCount;
	// Re-arm the watchdog (as done in ScriptingContext.cpp)
	lua_setwatchdogtimer(L, WatchdogHook, 1000);
}

// Simple Lua script that runs many iterations
constexpr const char* kBenchmarkScript = R"lua(
local sum = 0
for i = 1, 100000 do
    sum = sum + i
end
return sum
)lua";

// Busy loop script (more instruction-dense)
constexpr const char* kBusyLoopScript = R"lua(
local x = 0
for i = 1, 50000 do
    x = x + 1
    x = x - 1
    x = x + 1
end
return x
)lua";

// -----------------------------------------------------------------------------
// Benchmark: Standard Lua Hook (LUA_MASKCOUNT)
// -----------------------------------------------------------------------------
static void BM_LuaHook_Standard_Count1000(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_hookCallCount = 0;

		// Set standard count hook (fire every 1000 instructions)
		lua_sethook(L, StandardHook, LUA_MASKCOUNT, 1000);

		// Load and run the benchmark script
		if (luaL_dostring(L, kBenchmarkScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_hookCallCount);
		lua_close(L);
	}
	state.counters["HookCalls"] = benchmark::Counter(
	    static_cast<double>(g_hookCallCount),
	    benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_LuaHook_Standard_Count1000);

// -----------------------------------------------------------------------------
// Benchmark: Custom Watchdog Timer (Nexen Modification)
// -----------------------------------------------------------------------------
static void BM_LuaHook_Watchdog_Count1000(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_hookCallCount = 0;

		// Set custom watchdog timer (fire every 1000 instructions)
		lua_setwatchdogtimer(L, WatchdogHook, 1000);

		// Load and run the benchmark script
		if (luaL_dostring(L, kBenchmarkScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_hookCallCount);
		lua_close(L);
	}
	state.counters["HookCalls"] = benchmark::Counter(
	    static_cast<double>(g_hookCallCount),
	    benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_LuaHook_Watchdog_Count1000);

// -----------------------------------------------------------------------------
// Benchmark: No Hook (Baseline)
// -----------------------------------------------------------------------------
static void BM_LuaHook_NoHook(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);

		// No hook set - baseline performance
		if (luaL_dostring(L, kBenchmarkScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_NoHook);

// -----------------------------------------------------------------------------
// Benchmark: Busy Loop with Standard Hook
// -----------------------------------------------------------------------------
static void BM_LuaHook_BusyLoop_Standard(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_hookCallCount = 0;

		lua_sethook(L, StandardHook, LUA_MASKCOUNT, 1000);

		if (luaL_dostring(L, kBusyLoopScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_hookCallCount);
		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_BusyLoop_Standard);

// -----------------------------------------------------------------------------
// Benchmark: Busy Loop with Watchdog
// -----------------------------------------------------------------------------
static void BM_LuaHook_BusyLoop_Watchdog(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_hookCallCount = 0;

		lua_setwatchdogtimer(L, WatchdogHook, 1000);

		if (luaL_dostring(L, kBusyLoopScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_hookCallCount);
		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_BusyLoop_Watchdog);

// -----------------------------------------------------------------------------
// Benchmark: Busy Loop No Hook (Baseline)
// -----------------------------------------------------------------------------
static void BM_LuaHook_BusyLoop_NoHook(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);

		if (luaL_dostring(L, kBusyLoopScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_BusyLoop_NoHook);

}  // namespace
