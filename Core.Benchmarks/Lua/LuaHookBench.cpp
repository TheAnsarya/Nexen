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

// =============================================================================
// Per-Instruction Hook Dispatch Overhead
// =============================================================================
// The real bottleneck when Lua scripts are loaded is the per-instruction hook
// dispatch cost. For emulator-registered hooks (memory read/write/exec), the
// dispatch fires on EVERY CPU instruction or memory access.
//
// At 1.79MHz NES speed: ~1,790,000 instructions/second.
// If each hook dispatch costs 50ns, that's ~90ms/second just for dispatching.
//
// These benchmarks measure:
//   1. The cost of lua_sethook with count=1 (fires on every instruction)
//   2. The overhead of a no-op hook vs a hook that calls back into C++
//   3. The cost difference between "script loaded" and "no script" paths
// =============================================================================

// Hook that increments a counter for memory access simulation
thread_local int64_t g_accessHookCount = 0;

void MemAccessHook(lua_State* L, lua_Debug* ar) {
	(void)L;
	(void)ar;
	++g_accessHookCount;
}

// Script that simulates a script which reads memory on every iteration
// (like a typical NES trainer/cheater that reads RAM each frame)
constexpr const char* kMemoryReadScript = R"lua(
local sum = 0
for i = 1, 10000 do
    sum = sum + i
    if sum > 1000000 then sum = 0 end
end
return sum
)lua";

// Benchmark: Per-instruction hook (count=1) — fires on EVERY instruction
// This is the worst case: a Lua debug hook on every bytecode instruction.
static void BM_LuaHook_PerInstruction_Count1(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_accessHookCount = 0;

		// Set count hook to fire every 1 instruction — maximum overhead
		lua_sethook(L, MemAccessHook, LUA_MASKCOUNT, 1);

		if (luaL_dostring(L, kMemoryReadScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_accessHookCount);
		lua_close(L);
	}
	state.counters["HookCalls/iter"] = benchmark::Counter(
		static_cast<double>(g_accessHookCount),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_LuaHook_PerInstruction_Count1);

// Benchmark: Per-instruction hook (count=10) — fires every 10 instructions
// Represents a throttled hook (less accurate but lower overhead)
static void BM_LuaHook_PerInstruction_Count10(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_accessHookCount = 0;

		lua_sethook(L, MemAccessHook, LUA_MASKCOUNT, 10);

		if (luaL_dostring(L, kMemoryReadScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_accessHookCount);
		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_PerInstruction_Count10);

// Benchmark: Per-instruction hook (count=100)
static void BM_LuaHook_PerInstruction_Count100(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);
		g_accessHookCount = 0;

		lua_sethook(L, MemAccessHook, LUA_MASKCOUNT, 100);

		if (luaL_dostring(L, kMemoryReadScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		benchmark::DoNotOptimize(g_accessHookCount);
		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_PerInstruction_Count100);

// Benchmark: No-hook baseline — same script, no hook registered
// Compare with Count1 to determine hook overhead per instruction
static void BM_LuaHook_NoHook_Baseline(benchmark::State& state) {
	for (auto _ : state) {
		lua_State* L = luaL_newstate();
		luaL_openlibs(L);

		// No hook — pure script execution cost
		if (luaL_dostring(L, kMemoryReadScript) != LUA_OK) {
			state.SkipWithError("Script execution failed");
			lua_close(L);
			continue;
		}

		lua_close(L);
	}
}
BENCHMARK(BM_LuaHook_NoHook_Baseline);

// Benchmark: Hook dispatch with "has hooks" check overhead
// In Nexen's scripting system, every memory access first checks
// "if any Lua scripts have registered hooks for this address".
// Measure the cost of this boolean guard check when no scripts are loaded.
static void BM_LuaHook_EmulatorGuard_NoScript(benchmark::State& state) {
	// Simulate: hasMemReadHooks = false (no script loaded)
	// The emulator calls: if (hasMemReadHooks) { DispatchHook(...); }
	bool hasMemReadHooks  = false;
	bool hasMemWriteHooks = false;
	bool hasExecHooks     = false;
	uint32_t addr = 0;
	uint8_t  value = 0;

	for (auto _ : state) {
		// Simulate one NES instruction tick with 3 memory accesses
		// (1 opcode fetch + up to 2 operand/data reads)
		if (hasExecHooks) [[unlikely]] {
			benchmark::DoNotOptimize(addr); // hook call would go here
		}
		if (hasMemReadHooks) [[unlikely]] {
			benchmark::DoNotOptimize(value);
		}
		if (hasMemReadHooks) [[unlikely]] {
			benchmark::DoNotOptimize(value);
		}
		addr = (addr + 1) & 0xFFFF;
		value = static_cast<uint8_t>(addr & 0xFF);
		benchmark::DoNotOptimize(addr);
	}
	state.SetItemsProcessed(state.iterations() * 3); // 3 memory accesses per iteration
}
BENCHMARK(BM_LuaHook_EmulatorGuard_NoScript);

// Benchmark: Hook dispatch check with script loaded (hooks=true)
// When a script is loaded, every memory access must dispatch.
static void BM_LuaHook_EmulatorGuard_WithScript(benchmark::State& state) {
	bool hasMemReadHooks  = true;
	bool hasMemWriteHooks = true;
	bool hasExecHooks     = true;

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	g_accessHookCount = 0;

	// Leave hook unregistered (we're just measuring the dispatch path cost,
	// not full Lua execution) — this measures the C++ overhead before Lua
	uint32_t addr = 0;
	uint8_t  value = 0;

	for (auto _ : state) {
		if (hasExecHooks) {
			g_accessHookCount++;
		}
		if (hasMemReadHooks) {
			g_accessHookCount++;
		}
		if (hasMemWriteHooks) {
			g_accessHookCount++;
		}
		addr = (addr + 1) & 0xFFFF;
		value = static_cast<uint8_t>(addr & 0xFF);
		benchmark::DoNotOptimize(addr);
		benchmark::DoNotOptimize(value);
		benchmark::DoNotOptimize(g_accessHookCount);
	}
	state.SetItemsProcessed(state.iterations() * 3);
	lua_close(L);
}
BENCHMARK(BM_LuaHook_EmulatorGuard_WithScript);

}  // namespace
