# Epic 26 - Genesis Family Living Plan (2026-04-27)

## Purpose

Living execution plan for Sega Genesis-family completion in Nexen, spanning Mega Drive baseline, Sega CD, 32X, Power Base Converter path, and UX/tooling parity.

## Program Status

Epic-level decomposition is active and tracked via child issues.
This document is the living index for scope, sequencing, and measurable gates.

## Active Workstreams

### Core Baseline and Hardware Matrix

- [#1460](https://github.com/TheAnsarya/Nexen/issues/1460) - Genesis baseline completion: timing, mappers, hardware edge-case matrix
- [#1461](https://github.com/TheAnsarya/Nexen/issues/1461) - SMS maturity for Power Base Converter foundation
- [#1462](https://github.com/TheAnsarya/Nexen/issues/1462) - Power Base Converter integration track

### Add-On Expansion

- [#1463](https://github.com/TheAnsarya/Nexen/issues/1463) - Sega CD integration (sub-CPU, CD pipeline, timing, save-state contracts)
- [#1464](https://github.com/TheAnsarya/Nexen/issues/1464) - 32X integration track
- [#1468](https://github.com/TheAnsarya/Nexen/issues/1468) - 32X core integration program
- [#1469](https://github.com/TheAnsarya/Nexen/issues/1469) - Sega CD integration program

### Controllers, Tooling, and UX

- [#1465](https://github.com/TheAnsarya/Nexen/issues/1465) - Genesis-family controller/peripheral matrix completion
- [#1466](https://github.com/TheAnsarya/Nexen/issues/1466) - Debugger/TAS/cheat/UX parity program
- [#1472](https://github.com/TheAnsarya/Nexen/issues/1472) - Genesis UI/TAS/Cheat/Debugger integration backlog

### Expansion Coordination

- [#1467](https://github.com/TheAnsarya/Nexen/issues/1467) - Epic 24 Genesis full-stack expansion
- [#1470](https://github.com/TheAnsarya/Nexen/issues/1470) - Power Base converter compliance program

## Measurable Correctness and Benchmark Gates

### Mega Drive Baseline (M68K/Z80/VDP/YM2612/PSG)

- Correctness gates:
  - Deterministic replay/save-state parity across Genesis focused suites
  - Timing and arbitration coverage for CPU/audio/VDP boundaries
  - Mapper and hardware-edge matrix growth without regression
- Benchmark gates:
  - Focused benchmark suite remains at or below baseline budget for Genesis hot paths

### Sega CD and 32X

- Correctness gates:
  - Sub-CPU and bus arbitration determinism checks
  - Add-on timing and synchronization contract tests
  - Save-state and replay parity with add-on enabled
- Benchmark gates:
  - No regression in core Genesis baseline benchmarks when add-ons are disabled
  - Add-on enabled performance budgets tracked separately

### UX, Debugger, TAS, and Tooling

- Correctness gates:
  - Controller profile matrix deterministic behavior
  - TAS playback and save-state replay parity
  - Debugger register/event surfaces stable across frame replay
- Benchmark gates:
  - UI/tooling interactions remain within responsiveness baselines

## Update Protocol (Living Document)

- Update this plan whenever:
  - a new Genesis-family child issue is created,
  - subsystem scope changes,
  - new correctness/benchmark gate is added,
  - discovered parity gaps require decomposition.
- Keep linked issue list and gate definitions current before closing any parent umbrella issue.
