# Nexen v1.3.0 — Discord Release Announcement

> Copy the content below into Discord. Discord supports Markdown formatting.

---

## 🎮 Nexen v1.3.0 Released

**Nexen** is a multi-system emulator for **NES, SNES, Game Boy, GBA, SMS, PC Engine, WonderSwan, and Atari Lynx** — forked from Mesen2 and taken in a new direction.

📥 **Download:** <https://github.com/TheAnsarya/Nexen/releases/tag/v1.3.0>

---

## 🔥 What's New in v1.3.0

This release focuses on **performance optimization** — 20 commits of benchmark-proven improvements across the entire emulation pipeline.

### ⚡ Performance Highlights

- **8.8x faster** Base64 signature checks (constexpr decode LUT)
- **66.3x faster** file extension lookups (string_view + const ref)
- **3.8x faster** stats/timestamp formatting (stringstream → std::format)
- **Branch prediction hints** (`[[likely]]`/`[[unlikely]]`) across all 8 CPU/PPU/memory hot paths
- **Zero-copy** parameter passing sweep across core interfaces
- **String allocation elimination** — reserve, append, move instead of temporary copies

### 📊 Optimization Phases (16.6–16.10)

| Phase | Focus | Key Changes |
|-------|-------|-------------|
| 16.6 | Temp string elimination | TraceLogger, DebugStats, AudioPlayerHud |
| 16.7 | Branch hints | 9 CPU/PPU/memory files across all systems |
| 16.8 | Constexpr LUT + copy elim | Base64, NTSC filter, FDS, VS System, VirtualFile |
| 16.9 | String alloc elimination | Serializer, FolderUtilities, PcmReader |
| 16.10 | Stringstream elimination | MD5, SHA1, SaveStateManager, log joins |

> 💡 **Discovery:** MSVC `std::format` is **3.4x slower** than `operator+` for short strings. We only use it to replace `stringstream` (where it's 3.8x faster).

---

## 🌟 Why Nexen Exists — Full Diff from Mesen2

Nexen was forked from [Mesen2](https://github.com/SourMesen/Mesen2) because development had stalled, leaving unmerged bug fixes and no path forward for the features we wanted. **616 commits** and **+154,000 lines** later, Nexen is its own project.

### 🆕 New Systems

- **Atari Lynx** — Complete from-scratch emulation core (not a Handy fork)
	- 65C02 CPU, Mikey/Suzy hardware, sprite engine, 4-channel stereo audio
	- EEPROM, RSA bootloader, ComLynx serial, MAPCTL overlays
	- Full debugger integration + 84 unit tests + 67 benchmarks

### 🎬 TAS Editor

- **Piano Roll** — visual frame-by-frame input editing
- **Greenzone** — automatic savestates for instant seeking
- **Movie Branches** — save/compare alternate routes
- **Multi-format import/export** — BK2, FM2, SMV, LSMV, VBM, GMV, NMV
- **Lua scripting API** — InsertFrames, DeleteFrames, automation

### 💾 Save State System

- **Infinite saves** — no more 10-slot limit
- **Visual Picker** (Shift+F1) — grid view with screenshots + timestamps
- **Quick Save** (F1) + Designated Slot (F4/Shift+F4)
- **Auto-save** — configurable periodic saves (5-minute recent + 20-minute archive)

### 🌼 Pansy Metadata Export

- Universal disassembly metadata format integration
- Background CDL recording without debugger
- Auto-export every 5 minutes + ROM CRC32 verification
- Bidirectional sync with external label files

### 🐛 8 Upstream Mesen2 Bug Fixes
Fixes from unmerged Mesen2 PRs that Nexen integrates:

- SNES DMA overflow, CX4 cache/timing, hi-res blend, ExLoRom mapping
- NES open bus, NTSC dot crawl, Lua crash fix
- Linux FontConfig typeface caching

### 🚀 Performance — 192+ Optimization Commits

- **Branchless CPU flags** across NES, SNES, GB, PCE, SMS
- **constexpr LUTs** for brightness, hex, Base64, flag lookups
- **Move semantics** for frame data, HUD strings, expression evaluator
- **Per-frame allocation elimination** in hot paths
- **FastBinary serializer** for run-ahead (no string key overhead)
- **Ring buffers** for rewind audio, profiler callstacks, reverb
- **SNES PPU**: constexpr brightness (33% faster), ConvertToHiRes caching (38% faster)
- **GB PPU**: cached config (160x allocation reduction)

### 🔧 Modernization

- **C++23** with `/std:c++latest` and VS 2026 toolset
- **.NET 10** + latest Avalonia UI
- **`[[nodiscard]]`, `constexpr`, `string_view`, `emplace_back`** throughout
- **K&R brace style** standardized across entire codebase
- **StreamHash** for unified ROM hashing (CRC32/MD5/SHA-1/SHA-256)

### 🧪 Testing

- **1,495 C++ tests** + **152 .NET tests** (up from 421 at v1.0.0)
- Exhaustive 256×256 CPU flag comparison tests
- Lynx hardware reference tests, fuzz tests, sprite tests
- Google Benchmark suite with statistical validation

### 📦 CI/CD

- GitHub Actions builds for **Windows, Linux (x64/ARM64), macOS (ARM64)**
- AppImage packaging, Native AOT builds, versioned release assets

---

## 🔗 Links

- **Release:** <https://github.com/TheAnsarya/Nexen/releases/tag/v1.3.0>
- **Full Changelog:** <https://github.com/TheAnsarya/Nexen/compare/v1.2.0...v1.3.0>
- **All Changes vs Mesen2:** <https://github.com/SourMesen/Mesen2/compare/master...TheAnsarya:Nexen:master>
- **GitHub:** <https://github.com/TheAnsarya/Nexen>

---

*Nexen supports: NES, Famicom, SNES, Super Famicom, Game Boy, Game Boy Color, Game Boy Advance, Master System, Game Gear, PC Engine, TurboGrafx-16, WonderSwan, WonderSwan Color, and Atari Lynx.*
