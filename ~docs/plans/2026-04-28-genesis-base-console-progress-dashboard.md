# Genesis Base Console Progress Dashboard (2026-04-28)

## Goal

Ship a correctly working and performant base Genesis/Mega Drive emulator path with UI readiness to play target games (Sonic, Jurassic Park) by end of week.

## Scope Boundary

- In scope: base Genesis/Mega Drive hardware and UI workflow
- Out of scope for week target: Sega CD, 32X, Power Base Converter feature completion
- Rule: addon work must not displace base-console correctness, performance, and UX closure

## Subsystem Completion Overview

Percentages are execution estimates based on current test coverage, implemented scaffolding, and known remaining work.

| Subsystem | Completion | Done | Remaining |
| ---------- | ---------- | ---------- | ---------- |
| 68000 CPU execution baseline | 79% | Boundary/scaffold harnesses, deterministic compatibility digest path, checkpoint infrastructure, synthesized base-core compatibility gate | ISA edge-case correctness expansion, cycle-sensitive corner validation against hardware-focused corpus |
| Z80 handoff + bus arbitration (base path) | 72% | Handshake/read-write parity tests and transcript determinism gates | Additional real-game transition coverage and timing-sensitive conflict validation |
| Memory map + mapper edge handling | 77% | Mapper edge checkpoints and ownership gates in compatibility harness, base-core compatibility checkpoint synthesis | Mapper-specific behavioral deltas for target-game bank behaviors |
| VDP rendering + DMA core path | 69% | VDP/DMA scaffolding and regression test packs exist, startup VDP timing stability plus startup interrupt cadence, interval-tolerance, and VDP status-transition checkpoints added for Sonic/Jurassic startup paths | Pixel-accurate behavior and game-facing timing correctness for Sonic/Jurassic scenes |
| Audio (YM2612 + SN76489 base path) | 58% | Audio timing scaffold and mixed-audio test lanes available | Game-facing audio correctness and latency/perf tuning on base console path |
| Controller/input core path | 70% | Input/tooling checkpoints and controller matrix decomposition work available | Base-console UX and per-device mapping closure for week target |
| Save state determinism (base path) | 68% | Deterministic replay checkpoints across Genesis test scaffolds | Real gameplay save/load reliability checks for target games |
| Debugger/readout reliability (base path) | 75% | Transcript lane parity and debug checkpoint determinism packs, TMSS status exposed in Genesis debugger viewer | Register/event confidence pass for game-debug workflows |
| UI game-playability workflow | 58% | Existing settings/input/TAS UI foundations and parity checklists, Genesis region menu wiring/apply path and port2 default input initialization implemented | End-to-end base-console launch/config/play loop polish for Sonic/Jurassic |
| Performance headroom (base path) | 52% | Performance gate scaffolds present in Genesis tests | Hot-path profiling and optimization for reliable full-speed gameplay on target hardware |

## Week Target Definition

By end of week, all items below should be true for base Genesis path:

- Sonic and Jurassic Park boot and run through stable gameplay loops
- No blocker-class regressions in base-console compatibility checkpoints
- Playability-critical UI flow works end-to-end:
	- Open ROM
	- Configure input
	- Play without major audio/video/gameplay breakage
	- Save/load state reliably
- Performance remains playable on representative user hardware

## Priority Work Queue (Base Console First)

1. Compatibility and correctness
	- Expand base-console checkpoint enforcement for Sonic/Jurassic corpus
	- Resolve any failing deterministic or ownership gates on base path first

2. Rendering and timing
	- Tighten VDP timing/behavior impacting gameplay correctness
	- Validate DMA and scanline-sensitive behaviors against target-game scenarios

3. Audio and input usability
	- Remove playability-impacting audio glitches and timing drift
	- Close controller mapping and input UX blockers for base path

4. Performance stabilization
	- Profile base-console hotspots (CPU, VDP, memory handlers)
	- Apply correctness-preserving optimizations only

5. UX readiness
	- Ensure minimal friction from launch to playable state in UI
	- Track and burn down week-blocking UX defects first

## Tracking Issues

- Current base Genesis focus umbrella: #1706
- Base-core compatibility checkpoint slice: #1707
- VDP startup timing stability checkpoint slice: #1708
- Startup interrupt cadence checkpoint slice: #1709
- Startup VBlank/HBlank interval tolerance checkpoint slice: #1710
- Startup VDP status-transition checkpoint slice: #1711
- Genesis region override emulator+UI wiring slice: #1712
- TMSS runtime gating + debugger status slice: #1713
- SMD deinterleave payload-tail fix slice: #1714
- Genesis port2 default mapping initialization slice: #1715
- Immediate implementation backlog buckets:
	- #1696, #1697, #1700, #1701, #1702

## Progress Update Rule

Update this dashboard at the end of each implementation session with:

- changed completion percentages
- completed/added checkpoint evidence
- newly discovered blockers and owner issues
