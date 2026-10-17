# Manual Step Priority Ranking

This document ranks manual test steps for release sign-off across all platforms.

## Priority Levels

| Priority | Meaning | Release Impact |
|---|---|---|
| Critical | Core correctness, determinism, data integrity, and debugger truth | Release blocker |
| High | Mainline user workflows and TAS tooling behavior | Release blocker |
| Medium | Productivity and UX quality improvements | Fix before next milestone |
| Low | Nice-to-have polish | Track as follow-up issue |

## Atari 2600 Ranked Step List

| ID | Area | Manual Step | Priority |
|---|---|---|---|
| A26-DBG-01 | Debugger UI | Disassembly updates correctly while stepping frame-by-frame | Critical |
| A26-DBG-02 | Debugger UI | Breakpoints hit exact expected address and pause reliably | Critical |
| A26-DBG-03 | Debugger UI | Memory viewer reflects TIA and RIOT register writes live | Critical |
| A26-DBG-04 | Debugger UI | Trace logger includes expected Atari 2600 tags and ordering | High |
| A26-DBG-05 | Debugger UI | Event viewer state remains stable when running and pausing repeatedly | High |
| A26-TAS-01 | TAS UI | Atari 2600 controller subtype button set matches selected port type | Critical |
| A26-TAS-02 | TAS UI | Paddle position editor appears only for paddle ports and writes values | Critical |
| A26-TAS-03 | TAS UI | Console SELECT and RESET toggles apply frame commands and undo correctly | Critical |
| A26-TAS-04 | TAS UI | Multi-port edit mode updates only the selected port input | High |
| A26-TAS-05 | TAS UI | Input preview matches frame data after edit, undo, and redo | High |
| A26-MOV-01 | Movie I/O | Nexen movie roundtrip preserves paddle position and console switches | Critical |
| A26-MOV-02 | Movie I/O | BK2 roundtrip preserves joystick, paddle position, and console switches | Critical |
| A26-MOV-03 | Movie I/O | Import and export mismatch paths produce clear validation errors | High |
| A26-RUN-01 | Runtime | Save state load resumes with identical controller and frame state | Critical |
| A26-RUN-02 | Runtime | Rewind and frame advance behavior stay deterministic over repeated cycles | High |

## Atari Lynx Ranked Step List

| ID | Area | Manual Step | Priority |
|---|---|---|---|
| LNX-ROM-01 | ROM Loading | LNX header ROM loads and runs without errors | Critical |
| LNX-ROM-02 | ROM Loading | Headerless ROM loads with game database auto-detection | Critical |
| LNX-ROM-03 | ROM Loading | HLE boot starts game correctly without boot ROM | Critical |
| LNX-ROM-04 | ROM Loading | Boot ROM plays FUJI animation and hands off to game | High |
| LNX-DSP-01 | Display | Landscape game renders at 160x102 with correct colors | Critical |
| LNX-DSP-02 | Display | Rotated game renders at 102x160 with correct orientation | Critical |
| LNX-AUD-01 | Audio | Music and sound effects play without crackling | High |
| LNX-AUD-02 | Audio | Stereo separation is correct with headphones | Medium |
| LNX-INP-01 | Input | All 9 buttons (UDLRabOoP) respond correctly in-game | Critical |
| LNX-INP-02 | Input | Input display overlay shows live button state | Medium |
| LNX-SAV-01 | Save Data | EEPROM save persists across sessions | Critical |
| LNX-SAV-02 | Save Data | Save states capture and restore complete system state | Critical |
| LNX-SAV-03 | Save Data | Cross-session save state persistence works | High |
| LNX-DBG-01 | Debugger UI | Disassembly shows valid 65C02 opcodes and steps correctly | Critical |
| LNX-DBG-02 | Debugger UI | Breakpoints hit exact address and pause reliably | Critical |
| LNX-DBG-03 | Debugger UI | Memory viewer shows WorkRam, PrgRom, BootRom correctly | Critical |
| LNX-DBG-04 | Debugger UI | Register viewer panels show live Mikey, Suzy, APU, Cart, EEPROM data | High |
| LNX-DBG-05 | Debugger UI | Trace logger and event viewer are stable | High |
| LNX-TAS-01 | TAS UI | Movie recording captures all 9 buttons per frame at 75 fps | Critical |
| LNX-TAS-02 | TAS UI | Movie playback replays deterministically | Critical |
| LNX-TAS-03 | TAS UI | BK2 roundtrip preserves all Lynx input data | Critical |
| LNX-TAS-04 | TAS UI | Frame advance and rewind work correctly | High |
| LNX-RUN-01 | Runtime | Rapid ROM switching has no crash or state corruption | High |
| LNX-RUN-02 | Runtime | Fast-forward mode works without crashes | Medium |

## Execution Order

1. Run all Critical steps first.
2. Run all High steps next.
3. Run Medium and Low only after blocker levels are clean.

## Failure Handling

1. Open or update a GitHub issue immediately.
2. Mark severity using this ranking table.
3. Attach reproduction steps and evidence reference.
