# C++ Hash Consolidation Plan (#181)

## Current State

### Source File Inventory

| File | Lines | License | Origin |
|------|-------|---------|--------|
| `Utilities/md5.h` | 43 | Public domain | Alexander Peslyak (Solar Designer) |
| `Utilities/md5.cpp` | 309 | Public domain | Same (OpenSSL-compatible API) |
| `Utilities/sha1.h` | 42 | Public domain | Steve Reid → Volker Grabsch C++ |
| `Utilities/sha1.cpp` | 294 | Public domain | Same |
| `Utilities/CRC32.h` | 54 | zlib | stbrumme/crc32 |
| `Utilities/CRC32.cpp` | 4,274 | zlib | Same (mostly lookup tables) |

Total: ~5,016 lines (4,100+ are CRC32 lookup tables).

### vcpkg Dependencies
Currently: `gtest`, `benchmark` only. **No crypto library.**

---

## Call Site Catalog

### MD5 (2 active call sites)

| # | File | Context | Path Type |
|---|------|---------|-----------|
| 1 | `Core.Tests/RecordedRomTest.cpp:30` | Hash each PPU frame buffer for test recording | ⚠️ Per-frame (test-only) |
| 2 | `Core.Tests/RecordedRomTest.cpp:53` | Hash each PPU frame buffer for test validation | ⚠️ Per-frame (test-only) |
| 3 | `InteropDLL/DebugApiWrapper.cpp:8` | Dead include (no calls) | N/A |

**Note:** The per-frame MD5 usage is only active in automated test mode, not during normal emulation.

### SHA1 (4 direct + 11 indirect call sites)

All cold-path (ROM load, on-demand, connection):

| # | File | Context |
|---|------|---------|
| 1 | `Utilities/VirtualFile.cpp:151` | Hash loaded file data |
| 2 | `Shared/RomInfo.cpp:376` | GB-on-SNES ROM hash |
| 3 | `Shared/RomInfo.cpp:378` | SNES PRG ROM hash |
| 4 | `Netplay/GameServer.cpp:47` | Salted password hash for netplay auth |
| 5 | `NES/NesConsole.cpp:441` | NES PRG ROM hash for cheat lookup |
| 6 | `Shared/MessageManager.cpp:12` | Dead include (no calls) |

Indirect callers (via VirtualFile.GetSha1Hash or RomInfo): MovieRecorder, SaveStateManager, NesHdPack, RomLoader, LuaApi, EmuApi.

### CRC32 (20 direct + 11 indirect call sites)

All cold-path (ROM load, mapper init, debugger init):

Used for ROM identification, patch validation, mapper selection, debugger CDL initialization, netplay ROM matching. The CRC32 implementation uses slice-by-16 algorithm — well optimized.

---

## Analysis

### Risk Assessment

| Hash | Replace? | Risk | Reasoning |
|------|----------|------|-----------|
| **CRC32** | ❌ Keep | N/A | 20+ call sites, optimized slice-by-16, used everywhere. `System.IO.Hashing` available on C# side but C++ core needs it. |
| **SHA1** | ✅ Best candidate | Low | 4 direct call sites, all cold-path. Clean API. |
| **MD5** | ⚠️ Maybe | Low-Medium | Only 2 real call sites but they're per-frame in test mode. Test performance matters. |

### Replacement Options

| Option | Pros | Cons |
|--------|------|------|
| **Windows CNG (BCrypt)** | Zero deps, platform API, FIPS-certified | Windows-only, need `#ifdef` for Linux/Mac |
| **OpenSSL via vcpkg** | Cross-platform, well-known, fast | Heavy dependency (~10MB), overkill for 2 hash functions |
| **PicoSHA2** | Single-header, MIT, tiny | SHA-2 only (no MD5), would need separate MD5 lib |
| **mbed TLS (via vcpkg)** | Lightweight, cross-platform, both MD5+SHA1 | Extra dependency |
| **Keep as-is** | Zero change risk, zero deps, public domain | ~650 lines of unmaintained vendored code |

### Recommendation

**Phase 1: No dead includes found** (subagent report was incorrect — all includes have matching usages)

**Phase 2: Evaluate BCrypt on Windows** (low risk, deferred)
- SHA1 only, behind `#ifdef _WIN32`
- Keep vendored sha1.cpp as fallback for Linux/Mac
- Profile to confirm no regression in ROM load time

**Phase 3: Decide on MD5** (deferred)
- Only 2 call sites, both in test code
- Low priority — can stay as vendored forever

### Effort Estimate
- Phase 2 (if pursued): 2-3 hours (including cross-platform testing)
- Phase 3: Optional, 1-2 hours if pursued

---

## Decision

Given that:
1. All implementations are public domain / zlib licensed
2. Total active code is ~650 lines (excluding CRC32 tables)
3. All call sites are cold-path (except test-only MD5)
4. No dead includes found — all are genuinely used
5. Adding a crypto library dependency for 2 hash functions is overkill

**Recommended: Close as low-value.** The vendored implementations are small, well-tested, public domain, and zero-dependency. The modernization benefit of replacing them is minimal compared to the risk of introducing platform-specific `#ifdef` code or adding a heavy dependency.

If future work requires SHA-256/SHA-512 (e.g., for Pansy metadata), *that* would be the right time to evaluate a unified crypto library.
