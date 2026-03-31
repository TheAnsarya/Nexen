# WonderSwan Test & Feature Plan

## Overview

This plan tracks WonderSwan (WS/WSC/SwanCrystal) test coverage expansion, feature gaps, and improvement roadmap.

## Current Status (2026-03-31)

| Area | Test Coverage | Files | Notes |
|------|--------------|-------|-------|
| PPU (rendering) | 43 tests | WsPpuTests.cpp | Pixel extraction, palettes, windows, frame timing |
| State (mixed) | 29 tests | WsStateTests.cpp | Default state, controller, EEPROM, CPU flags, APU volume |
| CPU (V30MZ) | 33 tests | WsCpuTests.cpp | Flags, addressing, arithmetic, parity, constants |
| Timer + DMA | 25 tests | WsTimerDmaTests.cpp | Timer ticks, auto-reload, DMA control flags |
| APU (audio) | 26 tests | WsApuTests.cpp | Channels 1-4, HyperVoice, volume, LFSR, sweep, wavetable |
| EEPROM + Serial | 30 tests | WsEepromTests.cpp | Command decode, address masking, timing, cart/serial state |
| **Total** | **186 tests** | **6 files** | Up from 72 |

## Source Files Inventory (Core/WS/)

### Root (13 files)

- WsConsole.cpp/h — Console initialization, frame loop
- WsCpu.cpp/h — V30MZ CPU (2670+ lines, full instruction set)
- WsCpuPrefetch.cpp/h — Instruction prefetch queue
- WsDmaController.cpp/h — GDMA and SDMA
- WsEeprom.cpp/h — EEPROM read/write/commands
- WsMemoryManager.cpp/h — I/O ports, memory map, IRQ management
- WsPpu.cpp/h — PPU rendering
- WsSerial.cpp/h — Link cable serial port
- WsTimer.cpp/h — H/VBlank timer countdown
- WsTypes.h — All type definitions (500+ lines)

### APU/ (7 files)

- WsApu.cpp/h — 4-channel wavetable + mixer
- WsApuCh1.h — Channel 1 (basic wavetable)
- WsApuCh2.h — Channel 2 (wavetable + PCM voice)
- WsApuCh3.h — Channel 3 (wavetable + sweep)
- WsApuCh4.h — Channel 4 (wavetable + noise/LFSR)
- WsHyperVoice.h — HyperVoice DMA PCM audio

### Carts/ (2 files)

- WsCart.cpp/h — Single cart class (no RTC/Flash subclasses)

### Debugger/ (13 files)

- Full debugger infrastructure (13 files)

## Feature Gaps

### Cart Board Types (#1092, #1093)

WsCart.h has two `TODOWS` markers:

- **Line 10:** `// TODOWS RTC` — Real-time clock cart support missing
- **Line 11:** `// TODOWS Flash` — Flash memory cart support missing

Currently a single `WsCart` class with no subclasses for different board types.

**Proposed approach:**

1. Research WS cart board types (standard, RTC, Flash, SRAM variants)
2. Add `WsCartRtc` subclass with RTC register emulation
3. Add `WsCartFlash` subclass with flash command/sector erase
4. Cart database to identify board type from ROM header/hash

### Test Coverage Gaps (#1096)

Areas still needing tests:

- **CPU instruction tests** — Actual V30MZ opcode execution (requires integration with WsCpu)
- **Memory port R/W** — Port read/write behavior for $00-$FF range
- **Banking behavior** — ROM bank switching (4 bank slots)
- **PPU scanline rendering** — Full scanline rendering pipeline
- **APU mixing** — Multi-channel audio mixing and output
- **Serial communication** — Link cable data exchange
- **Integration tests** — Full frame execution with ROM

### Benchmarks

Current: 1 benchmark file (WsPpuBench.cpp, 12+ benchmarks)

Proposed additions:

- WsCpuBench.cpp — V30MZ instruction throughput
- WsMemoryBench.cpp — Memory access patterns
- WsApuBench.cpp — Audio processing throughput

## Architecture Notes

### V30MZ CPU

- NEC V30MZ (8086-compatible, not 80186)
- 20-bit physical addressing: (segment << 4) + offset
- 10 flags: C, P, AC, Z, S, T, I, D, O, M (Mode is unique to V30MZ)
- Base flags value: 0x7002 (bits 1, 12, 13, 14 always set)
- Halted state exits on IRQ
- PowerOff is terminal

### Memory Map

- $00000-$0FFFF: RAM (64KB)
- $10000-$1FFFF: SRAM (optional, up to 64KB)
- $20000-$FFFFF: Cart ROM (banked, 4 slots)
- I/O ports: $00-$FF

### Timers

- Two independent countdown timers (HBlank, VBlank)
- Each has: counter, reload value, enable, auto-reload
- Fire IRQ on underflow

### DMA

- GDMA: General DMA (source → VRAM)
- SDMA: Sound DMA (source → APU, with repeat/hold/decrement options)
- SDMA can target HyperVoice for streaming audio

### APU

- 4 wavetable channels (32 × 4-bit samples each)
- Channel 2: Optional PCM voice mode
- Channel 3: Optional frequency sweep
- Channel 4: Optional noise (15-bit LFSR)
- HyperVoice: DMA-driven 8-bit PCM playback (WSC/SC only)
- Stereo output with per-channel L/R volume

## Related Issues

- #1092 — RTC cart support
- #1093 — Flash cart support
- #1096 — WS test suite coverage gaps
