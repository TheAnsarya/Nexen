# Lua Package Migration Track

Issue: #2187
Parent: #2185

## Goal

Remove vendored `Lua/*` source from Nexen and consume Lua through package-managed dependencies while preserving Nexen-specific behavior.

## Constraints

- Preserve all existing scripting behavior and debugger integrations.
- Preserve Nexen watchdog and safety semantics currently embedded in vendored Lua.
- Keep C++ tests passing and avoid runtime behavior regressions.

## Current State

- Vendored Lua remains the largest tracked third-party source footprint.
- Inventory baseline (from [../../docs/THIRD-PARTY-SOURCE-INVENTORY.md](../../docs/THIRD-PARTY-SOURCE-INVENTORY.md)):
	- Lua files: 103
	- Lua lines: 31818
	- Lua bytes: 1113623

## Phase 1 Delta Inventory (completed)

Nexen-specific Lua deltas explicitly marked in vendored sources:

- `Lua/lua.h`
	- Added `lua_WatchDogHook` callback typedef.
	- Added `lua_setwatchdogtimer(lua_State*, lua_WatchDogHook, int)` API declaration.
- `Lua/lstate.h`
	- Added per-thread watchdog state fields (`watchdogtimer`, `watchdoghook`).
- `Lua/lstate.c`
	- Initialized watchdog state in `preinit_thread()`.
- `Lua/ldebug.c`
	- Implemented `lua_setwatchdogtimer()` API.
- `Lua/lvm.c`
	- Added watchdog countdown check in `vmfetch()` instruction dispatch macro.

## Prototype Build Toggles (implemented)

- `NexenUsePackagedLua` (MSBuild property, default `false`)
	- When `true`, solution projects can bypass direct `Lua/Lua.vcxproj` linkage in:
		- `Core.Tests/Core.Tests.vcxproj`
		- `Core.Benchmarks/Core.Benchmarks.vcxproj`
		- `InteropDLL/InteropDLL.vcxproj`
- `NEXEN_USE_PACKAGED_LUA` (compile-time define in `Core/Core.vcxproj`)
	- Added interop adapter: `Core/Debugger/LuaInterop.h`
	- Added compatibility shim: `Core/Debugger/LuaWatchdogCompat.cpp`
	- Current shim maps watchdog behavior onto `lua_sethook(..., LUA_MASKCOUNT, ...)` for prototype validation.

## Plan

1. Identify all Nexen-specific Lua deltas versus upstream/package Lua.
2. Implement a patch layer isolated from vendor sources (wrapper hooks/adapter headers).
3. Switch build targets from `Lua/Lua.vcxproj` source compilation to package-linked Lua.
4. Validate scripting, debugger callbacks, and serialization behavior.
5. Remove vendored Lua source and update third-party inventory/policy baselines.

## Validation

- Build: Release x64
- Tests:
	- `Core.Tests.exe --gtest_filter=Lua*`
	- `Core.Tests.exe --gtest_filter=Debugger*`
	- Existing Genesis/targeted suites unaffected

## Deliverables

- Build-system switch to package dependency.
- Patch-layer docs + code.
- Updated inventory showing Lua footprint reduction.
