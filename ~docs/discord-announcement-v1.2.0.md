# Discord Announcement — Nexen v1.2.0

> Copy the content below into Discord. Discord uses a subset of Markdown.

---

## Announcement Post

```
# 🎮 Nexen v1.2.0 Released!

**The biggest Nexen release yet** — 192 commits, 352 files changed, 1,495 tests passing.

📥 **Download:** <https://github.com/TheAnsarya/Nexen/releases/tag/v1.2.0>

---

## 🐱 What is Nexen?

Nexen is a **multi-system emulator** forked from [Mesen2](https://github.com/SourMesen/Mesen2) — one of the most accurate emulators ever made. We forked because we wanted things Mesen2 doesn't have:

- **TAS Editor** — Full piano roll, greenzone, branches, rerecording
- **New Systems** — Atari Lynx (and more coming)
- **🌼 Pansy Export** — Universal disassembly metadata format
- **Performance** — Hundreds of optimizations to the hot path
- **Upstream Bug Fixes** — We evaluate and apply all open Mesen2 PRs
- **Modern C++23** — constexpr, [[likely]], std::bit_cast, ranges
- **Testing** — 1,495 unit/integration tests (Mesen2 has 0)

Nexen emulates **NES, SNES, Game Boy, GBA, Master System, PC Engine, WonderSwan, and now Atari Lynx**.

---

## 🆕 What's New in v1.2.0

### 🕹️ Atari Lynx — Complete New Emulation Core!
> **Not a Handy fork** — written from scratch using hardware documentation

- **65C02 CPU** with full instruction set and hardware bug emulation
- **Mikey** — Timer system, interrupts, display DMA, palette, 4-channel LFSR audio
- **Suzy** — Sprite engine (1-4bpp, quadrant rendering, stretch/tilt), hardware math, collision
- **UART/ComLynx** serial port for multiplayer
- **EEPROM** support (93C46/66/86, auto-detected from LNX header)
- **RSA bootloader** decryption
- **Full debugger** — disassembler, trace logger, event manager, sprite viewer
- **TAS ready** — piano roll, movie converter, Lua API
- **~80 game ROM database** from No-Intro
- 67 performance benchmarks + 84 unit tests

### 🔧 8 Upstream Mesen2 Bug Fixes
We evaluated all 24 open Mesen2 PRs and applied 8 accuracy fixes:

- **SNES** — DMA overflow (#87), CX4 timing (#86), hi-res blend (#80), ExLoRom mapping (#74)
- **NES** — Open bus value (#82), NTSC dot crawl + Bisqwit matrix + RGB PPU emphasis (#31)
- **Debugger** — Lua non-string error crash (#76), Linux FontConfig perf (#85)

### ⚡ 13 Phases of Performance Optimization
- FastBinary positional serializer (eliminates string overhead in run-ahead)
- Rewind system: audio ring buffer, O(1) memory tracking, direct-write compression
- Move semantics for RenderedFrame/VideoDecoder pipeline
- ExpressionEvaluator const ref — **9x string copy elimination**
- DebugHud flat buffer — replaces unordered_map
- GetPortStates buffer reuse — eliminates per-frame allocation
- Lynx-specific: NZ lookup table, branchless flags, timer mask optimization

### 🧪 1,495 Tests (was 659)
- Exhaustive 65C02 CPU instruction tests
- Lynx hardware reference tests (63), math unit (16), sprite (50+), fuzz (17)
- Upstream Mesen2 fix verification
- TAS integration and movie converter tests

---

## 📊 Since Forking from Mesen2

| | Mesen2 | Nexen v1.2.0 |
| --- | --- | --- |
| **Emulated systems** | 7 | **8** (+Atari Lynx) |
| **Test suite** | 0 tests | **1,495 tests** |
| **Benchmarks** | 0 | **67+ benchmarks** |
| **TAS Editor** | ❌ None | ✅ Full (piano roll, greenzone, branches) |
| **Pansy Export** | ❌ None | ✅ Universal metadata format |
| **Open PR fixes applied** | 0 of 24 | **8 of 24** |
| **C++ standard** | C++17 | **C++23** |
| **Performance passes** | — | **13 phases, 192 commits** |

---

## 🔗 Links

- 📥 **Download:** <https://github.com/TheAnsarya/Nexen/releases/tag/v1.2.0>
- 📋 **Changelog:** <https://github.com/TheAnsarya/Nexen/blob/master/CHANGELOG.md>
- 🐙 **Source:** <https://github.com/TheAnsarya/Nexen>
- 🌼 **Pansy format:** <https://github.com/TheAnsarya/pansy>

**Nexen is free and open source** (Unlicense + GPL-3.0).
```

---

## Formatting Notes

- Discord supports `#` headings, `**bold**`, `> quotes`, `- lists`, and code blocks
- Tables render as code blocks in Discord — they'll look monospaced but readable
- Links auto-embed when wrapped in `< >`
- Emoji shortcodes (`:video_game:`) can replace Unicode emoji if preferred
