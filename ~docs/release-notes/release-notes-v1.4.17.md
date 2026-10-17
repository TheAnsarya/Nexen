# Nexen v1.4.17 — Channel F Complete Emulation & Lynx Improvements

> **20+ commits** | **Channel F core complete** | **Lynx ComLynx wiring** | **WonderSwan & CDL test coverage**

Nexen v1.4.17 delivers the complete Channel F emulation core with scanline-based rendering, accurate audio, F8 CPU fixes, and TAS editor integration. Also includes Lynx ComLynx multiplayer wiring, WonderSwan Flash/RTC test expansion, and CDL/BitUtilities benchmarks.

---

## Added

- **Channel F complete emulation core** -- Scanline-based frame execution with per-line rendering, correct VBLANK timing, 8-color palette with per-row selection, accurate 4-tone audio model, F8 interrupt delivery with ICB/VBLANK/SMI vector ports (#977, #979, #976, #1125)
- **Channel F SMI timer and IRQ** -- PSU/SMI/DMI timer state with ports 0x20/0x21 (#974)
- **Channel F TAS editor integration** -- Full TAS editor layout tests for Channel F (#1011)
- **Channel F DisUtils test coverage** -- 159 opcode disassembly tests (#1118)
- **Lynx ComLynx multiplayer wiring** -- Shared ComLynx cable lifecycle and runtime wiring tests (#955)
- **Lynx bank-addressing validation tests** -- Commercial ROM validation matrix (#1105)
- **WonderSwan Flash/RTC test coverage** -- Expanded test coverage and benchmarks (#1116)
- **LightweightCdlRecorder + BitUtilities tests** -- Additional test coverage and benchmarks (#1117)
- **TestRunner diagnostic improvements** -- Named error constants, stderr diagnostics, resource cleanup (#1105)

## Fixed

- **Channel F F8 CPU cycle timing** -- Return clock cycles not byte lengths (#1123)
- **Channel F INC opcode** -- Now correctly increments accumulator, not DC0 (#1124)
- **Channel F F8 flag bit layout** -- Complementary sign and CI/CM carry polarity (#1128)
- **Channel F DisUtils opcode tables** -- Corrected $88-$8e memory ALU and $8f BR7 (#1119)
- **Channel F battery save removal** -- Console has no battery-backed SRAM
- **Lynx boot-smoke Lua tooling** -- Fixed emu.stop() to emu.exit() for proper test-runner integration

---

## Downloads

### Windows

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-Windows-x64-v1.4.17.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Windows-x64-v1.4.17.exe) | Single-file, recommended |
| **Native AOT** | [Nexen-Windows-x64-AoT-v1.4.17.exe](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Windows-x64-AoT-v1.4.17.exe) | Faster startup, larger binary |

### Linux

| Build | Download | Notes |
|-------|----------|-------|
| **AppImage x64** | [Nexen-Linux-x64-v1.4.17.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-x64-v1.4.17.AppImage) | Recommended for most users |
| **AppImage ARM64** | [Nexen-Linux-ARM64-v1.4.17.AppImage](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-ARM64-v1.4.17.AppImage) | Raspberry Pi 4/5, etc. |
| Binary x64 (clang) | [Nexen-Linux-x64-v1.4.17.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-x64-v1.4.17.tar.gz) | Requires SDL2 |
| Binary x64 (gcc) | [Nexen-Linux-x64-gcc-v1.4.17.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-x64-gcc-v1.4.17.tar.gz) | Requires SDL2 |
| Binary ARM64 (clang) | [Nexen-Linux-ARM64-v1.4.17.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-ARM64-v1.4.17.tar.gz) | Requires SDL2 |
| Binary ARM64 (gcc) | [Nexen-Linux-ARM64-gcc-v1.4.17.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-ARM64-gcc-v1.4.17.tar.gz) | Requires SDL2 |
| Native AOT x64 | [Nexen-Linux-x64-AoT-v1.4.17.tar.gz](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-Linux-x64-AoT-v1.4.17.tar.gz) | Faster startup |

### macOS (Apple Silicon)

| Build | Download | Notes |
|-------|----------|-------|
| **Standard** | [Nexen-macOS-ARM64-v1.4.17.zip](https://github.com/TheAnsarya/Nexen/releases/download/v1.4.17/Nexen-macOS-ARM64-v1.4.17.zip) | App bundle |
| ~~Native AOT~~ | *Temporarily unavailable* | .NET 10 ILC compiler bug |

---

**Full Changelog:** https://github.com/TheAnsarya/Nexen/compare/v1.4.16...v1.4.17
