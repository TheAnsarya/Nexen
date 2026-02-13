# Lua Library Replacement Analysis

## Overview

This document analyzes whether the vendored Lua C source code in `Lua/` can be replaced with a prebuilt library (NuGet, vcpkg, etc.) or if custom modifications require us to compile it ourselves.

## Current Lua Implementation

**Location:** `Lua/` (~74 .c files, ~52 core Lua + ~22 LuaSocket)

**Version:** Lua 5.4 (based on API function signatures)

### Components

| Component | Files | Purpose |
|-----------|-------|---------|
| Lua Core | `lapi.c`, `lcode.c`, `ldebug.c`, `ldo.c`, `lgc.c`, `llex.c`, `lvm.c`, etc. | Standard Lua interpreter |
| Lua Standard Libraries | `lbaselib.c`, `liolib.c`, `lmathlib.c`, `loslib.c`, `lstrlib.c`, `ltablib.c` | Standard Lua libraries |
| LuaSocket | `luasocket.c`, `tcp.c`, `udp.c`, `inet.c`, `socket.h`, etc. | Network socket support |

## Custom Modifications (Nexen-Specific)

The Lua code has **three key modifications** clearly marked as `// ##### NEXEN MODIFICATION #####`:

### 1. Watchdog Timer (`lua_setwatchdogtimer`)

**Purpose:** Prevents infinite loops in Lua scripts by calling a hook function after N bytecode instructions.

**Files Modified:**
- `lua.h` — Added `lua_WatchDogHook` typedef and `lua_setwatchdogtimer()` function declaration
- `lstate.h` — Added `watchdogtimer` and `watchdoghook` fields to `lua_State` struct
- `lstate.c` — Initialize watchdog fields to NULL/0
- `ldebug.c` — Implementation of `lua_setwatchdogtimer()`
- `lvm.c` — Check and call watchdog hook inside main VM execute loop

**Code Example:**
```c
// lua.h
typedef void (*lua_WatchDogHook)(lua_State* L);
LUA_API void lua_setwatchdogtimer(lua_State* L, lua_WatchDogHook func, int count);

// lvm.c (in main execution loop)
if (L->watchdogtimer && !--L->watchdogtimer) {
    (*L->watchdoghook)(L);
}
```

**Why It Exists:** The emulator needs to prevent runaway Lua scripts from freezing the UI. When a script runs too long, the watchdog hook is triggered and can abort execution.

### 2. Sandbox File Loading Flag

**Purpose:** Allows blocking `loadfile()` and `dofile()` for security sandboxing.

**Files Modified:**
- `lauxlib.h` — Declaration of `SANDBOX_ALLOW_LOADFILE` global
- `lauxlib.c` — Check `SANDBOX_ALLOW_LOADFILE` before loading files

**Usage:**
```c
extern int SANDBOX_ALLOW_LOADFILE;  // lauxlib.h
// In lauxlib.c
if (!SANDBOX_ALLOW_LOADFILE) {
    return luaL_error(L, "file operations disabled in sandbox mode");
}
```

### 3. LuaSocket Integration

LuaSocket is bundled as part of the Lua build to provide network socket access for Lua scripts. This is a standard library but is compiled together with Lua for simplicity.

## Emulator Integration (LuaApi.cpp)

The `LuaApi` class in `Core/Debugger/LuaApi.cpp` exposes ~40+ emulator functions to Lua:

| Category | Functions |
|----------|-----------|
| Memory | `read`, `write`, `read16`, `write16`, `read32`, `write32`, `getMemorySize`, `convertAddress` |
| Callbacks | `addMemoryCallback`, `removeMemoryCallback`, `addEventCallback`, `removeEventCallback` |
| Drawing | `drawString`, `drawPixel`, `drawLine`, `drawRectangle`, `clearScreen`, `getScreenBuffer`, `setScreenBuffer` |
| Emulation Control | `reset`, `stop`, `breakExecution`, `resume`, `step`, `rewind` |
| Save States | `createSavestate`, `loadSavestate`, `getState`, `setState` |
| Input | `getInput`, `setInput`, `isKeyPressed`, `getMouseState` |
| Cheats | `addCheat`, `clearCheats` |
| Debugging | `getAccessCounters`, `getCdlData`, `getLabelAddress`, `getLogWindowLog` |

These are standard C functions registered via `luaL_newlib()` — **no modifications to Lua core required**.

## Analysis: Can We Use a Prebuilt Lua Library?

### Option A: Standard Lua Library (vcpkg/NuGet)

**Problem:** The watchdog timer modification is embedded in the Lua VM's bytecode execution loop (`lvm.c`). A prebuilt Lua library would not have this modification.

**Impact:** Without the watchdog timer:
- Infinite loops in Lua scripts would freeze the emulator
- No way to interrupt long-running scripts
- Users could accidentally write `while true do end` and hang the application

**Workaround Possibility:**
- Use `lua_sethook()` with `LUA_MASKCOUNT` — This is the **standard Lua debug hook** that can call a function every N instructions
- However, `lua_sethook()` requires a debug hook function signature `void (*lua_Hook)(lua_State* L, lua_Debug* ar)` while our watchdog uses `void (*lua_WatchDogHook)(lua_State* L)`
- The standard hook is slightly more overhead but functionally equivalent

**Verdict:** ✅ **Potentially replaceable** if we refactor to use `lua_sethook()` instead of custom watchdog.

### Option B: LuaJIT (Just-In-Time Compiler)

**Problem:** LuaJIT has its own execution model and the standard debug hooks work differently.

**Verdict:** ❌ **Not recommended** — Would require significant testing and API changes.

### Option C: Keep Vendored Lua with Minimal Modifications

**Current approach.** Works, but adds ~74 C files to the build.

## Recommendation

### Benchmark Results (February 2026)

We prototyped replacing the custom watchdog with standard `lua_sethook()` and ran benchmarks comparing performance:

| Benchmark | Time (ns) | Relative to No-Hook |
|-----------|-----------|---------------------|
| No Hook (Baseline) | 781,074 | 1.0x |
| Custom Watchdog | 870,315 | 1.11x |
| Standard `lua_sethook` | 1,950,104 | 2.50x |

**Key Finding:** The standard `lua_sethook(LUA_MASKCOUNT)` is **~2.2x slower** than the custom watchdog.

**Why the Difference:**
- Custom watchdog: Simple `if (--timer == 0) hook();` in `vmfetch()` macro
- Standard hook: Full `luaG_traceexec()` function call with trap flag checking, line number tracking, and debug info handling

**Implications:**
- **Functional equivalence:** ✅ Standard hook provides identical timeout functionality
- **Performance trade-off:** ❌ Scripts run ~50% slower with standard hook
- **Decision:** Keep custom watchdog for performance, or accept slowdown for standard Lua library

### Phase 1: Investigate Standard Hook Migration

Before replacing Lua, test whether `lua_sethook(L, hook, LUA_MASKCOUNT, 1000)` provides equivalent functionality to our custom watchdog:

```c
// Current custom approach
lua_setwatchdogtimer(_lua, ScriptingContext::ExecutionCountHook, 1000);

// Standard Lua approach (if it works)
lua_sethook(_lua, ScriptingContext::StandardExecutionHook, LUA_MASKCOUNT, 1000);
```

### Phase 2: If Hook Works, Use vcpkg Lua

- Remove `Lua/` directory
- Add `lua` to `vcpkg.json` dependencies
- Update `Core.vcxproj` to link against lua library
- Update include paths

### Phase 3: Remove Sandbox Flag

The `SANDBOX_ALLOW_LOADFILE` flag can likely be replaced by:
1. Not registering `loadfile` and `dofile` in the first place (custom `linit.c` equivalent)
2. Using a restricted `package.loader` setup

## Effort Estimate

| Task | Effort |
|------|--------|
| Research standard hook equivalence | 2-4 hours |
| Refactor to use `lua_sethook()` | 4-8 hours |
| Test all Lua scripts work correctly | 4-8 hours |
| Migrate to vcpkg Lua | 4-8 hours |
| Remove LuaSocket or add as separate dependency | 2-4 hours |
| **Total** | **16-32 hours** |

## Conclusion

The Lua vendored code **cannot be easily replaced** without performance trade-offs:

1. ✅ Standard `lua_sethook()` provides equivalent watchdog functionality
2. ❌ Standard hook is **2.2x slower** than custom watchdog
3. For scripts that run every frame, this could impact emulation performance

**Updated Recommendation:** 

**Keep the vendored Lua with custom watchdog** for performance reasons. Instead focus on:

1. **Clean up sandbox flag:** Replace `SANDBOX_ALLOW_LOADFILE` global with selective function registration
2. **Document modifications:** Add clear comments explaining why vendored Lua is necessary
3. **Upstream investigation:** Check if Lua 5.5 or LuaJIT have more efficient hook mechanisms

### Alternative: Conditional Migration

If Lua scripting performance is not critical (scripts run infrequently):
1. Accept the ~2.2x slowdown
2. Migrate to vcpkg Lua
3. Remove ~74 C files from repo

This trade-off should be measured with real-world Lua scripts before deciding.

### Prototype Code Location

The `lua_sethook()` prototype was tested in:
- `Core/Debugger/ScriptingContext.cpp` — Changed to use `lua_sethook(L, hook, LUA_MASKCOUNT, 1000)`
- `Core/Debugger/ScriptingContext.h` — Hook signature changed to `void(lua_State*, lua_Debug*)`
- `Core.Benchmarks/Lua/LuaHookBench.cpp` — Benchmark comparing both approaches

Note: The prototype changes should be reverted if keeping the vendored Lua approach.
