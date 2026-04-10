## Fixed

- **Linux/macOS build failure** - `std::chrono::clock_cast` is a C++23 feature unavailable in clang + libstdc++ on Ubuntu 22.04 / macOS 14; all Linux (clang, gcc, AoT, AppImage) and macOS builds were failing; replaced with portable `time_point_cast` arithmetic in `SaveStateManager.cpp`. (#1219)
- **clang `[[nodiscard]]` placement error in `LynxMemoryManager`** - `[[nodiscard]]` placed after `__forceinline` was parsed by clang as an attribute on the return type rather than on the function, causing all Linux/clang CI jobs to fail with `'nodiscard' attribute cannot be applied to types`; moved `[[nodiscard]]` before `__forceinline`. (#1220)
- **Accuracy test CI always failing** - `accuracy-tests.yml` builds the full `Nexen.sln` including `UI.csproj` which references `Pansy.Core`; the pansy repo was never cloned in CI causing `CS0246` on every run; added pansy checkout to both `smoke-tests` and `accuracy-tests` jobs. (#1219)
- **C++ tests CI missing pansy checkout** - Same pansy reference issue affected `tests.yml`; added checkout there as well. (#1219)
- **Accuracy tests running on every push** - Removed `push`/`pull_request` triggers from `accuracy-tests.yml`; workflow now only runs on `workflow_dispatch` and the nightly schedule. (#1219)
- **Avalonia 12 menu icon loading crash** - `ImageUtilities` threw an unhandled `ArgumentException` on Avalonia 12 when SVG assets failed to decode; hardened the icon load path with SVG -> PNG fallback and a non-throwing empty-image fallback to prevent UI-thread crashes. (#1212)
- **ReactiveUI Avalonia runtime initialization** - Startup crash regression introduced when `UseReactiveUI()` was removed during Avalonia 12 migration; restored correct `ReactiveUI.Avalonia` runtime initialization. (#1205)
- **Avalonia SVG image loading pipeline** - Avalonia 12 changed the SVG loading API; replaced the old load path with the `Svg.Controls.Skia.Avalonia` pipeline so SVG assets render correctly without PNG fallback masking. (#1214)

## Performance

- **CDL recording page cache - ~75-80% CDL overhead reduction** - Eliminated all virtual calls from the `RecordRead` hot path. Every memory read previously triggered 2 virtual calls + a switch per read (~10-15 ns x 3-4M reads/sec). Replaced with a flat 4 KB page cache mapping CPU-space addresses directly to absolute ROM offsets; `RebuildPageCache` keeps the cache coherent for consoles with dynamic mapping. (#1207)
- **Avalonia rendering pipeline lock reduction** - Consolidated `DrawOperation` bitmap locks from 3 to 2 per frame; lowered `DispatcherPriority` from `MaxValue` to `Render`. (#1207)
- **SNES memory handler devirtualization** - Every SNES CPU memory access (~3.58M/sec) went through virtual dispatch; for the common case (`RamHandler`/`RomHandler`) this resolved to a trivial array access. Added direct-access pointer fields (`_directReadPtr`, `_directWritePtr`, `_directMask`) to `IMemoryHandler` so `SnesMemoryManager` bypasses virtual dispatch with an inline array access for the dominant code paths. (#1208)
- **NES VS Dual per-frame 240 KB allocation eliminated** - `NesPpu::SendFrameVsDualSystem()` was allocating 240 KB on every frame; moved to lazy member allocation. (#1208)
- **NES internal RAM fast path** - NES CPU reads to internal RAM (~30% of all CPU reads) now bypass virtual dispatch; combined with the mapper fast path, ~90% of all NES CPU memory accesses are devirtualized. (#1209)
- **Atari 2600 `std::function` elimination** - Replaced `std::function` read/write bus callbacks with a direct `Atari2600Bus*` pointer, eliminating type-erasure overhead on every CPU memory access. (#1209)
- **Multi-system memory dispatch optimization** - Targeted SMS, Game Boy, PCE, Genesis, and Lynx with cached `CheatManager*` pointers, `[[likely]]`/`[[unlikely]]` branch annotations, and a Lynx fast-path/slow-path split (inline header fast path for direct RAM, `.cpp` slow path for overlay dispatch). (#1211)
- **ChannelF callback removal** - Finalized removal of callback-based dispatch in the ChannelF CPU/memory-manager path. (#1215)
- **WonderSwan flash dirty-state caching** - Added dirty-flag caching to the WonderSwan flash save-state path to avoid unnecessary I/O on unmodified flash banks. (#1215)
- **GBA/NES hotpath benchmarks - branch hints discarded** - Real emulator-path microbenchmarks for GBA `ProcessDma`, `ProcessInternalCycle` (0%/5%/50% pending-update distributions), and NES `BaseMapper::ReadVram`; measured and **discarded** `[[likely]]`/`[[unlikely]]` annotations that caused a +14.7% regression. (#1217, #1218)
- **Repeatable benchmark script** - Added `~scripts/run-memory-hotpath-benchmarks.ps1` for consistent hotpath perf audits with labeled, timestamped JSON output. (#1218)

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.25.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Windows-x64-v1.4.25.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.25.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Windows-x64-AoT-v1.4.25.exe) | Faster startup |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.25.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-x64-v1.4.25.AppImage) | Recommended |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.25.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-ARM64-v1.4.25.AppImage) | Raspberry Pi, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.25.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-x64-v1.4.25.tar.gz) | Tarball, requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.25.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-x64-gcc-v1.4.25.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.25.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-ARM64-v1.4.25.tar.gz) | Tarball, requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.25.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-ARM64-gcc-v1.4.25.tar.gz) | Tarball, requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.25.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-Linux-x64-AoT-v1.4.25.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.25.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.25/Nexen-macOS-ARM64-v1.4.25.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

> **Notes:**
> - Linux requires SDL2 (`sudo apt install libsdl2-2.0-0`)
> - macOS: Right-click -> Open on first launch to bypass Gatekeeper
