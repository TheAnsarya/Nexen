# Channel F Ordered Prompt Pack

This file provides an ordered sequence of prompts for multi-session execution. Run prompts in order.

## Prompt 01: Research Closure and Contracts

```text
[Nexen][Channel F] Complete issue #971: finalize source-backed architecture contract for Channel F and F8. Build comparison table from MAME/FreeChaF/docs, list unknowns, decide assumptions, and post an issue comment with accepted contracts and open risks.
```

## Prompt 02: Core Skeleton

```text
[Nexen][Channel F] Start #972: add non-invasive Channel F core skeleton files under Core/ChannelF with compile-only stubs and clear interfaces for CPU/memory/video/audio/input/cart. No behavior yet; compile and add tests that validate construction paths.
```

## Prompt 03: F8 Decode Scaffold

```text
[Nexen][Channel F] Work #973: create F8 opcode metadata table scaffold and decode harness tests. Implement enough structure to validate opcode mapping completeness without full execution semantics.
```

## Prompt 04: External Interface Semantics

```text
[Nexen][Channel F] Work #974: model F8 external interface behavior contracts (program/data counters, bus reads/writes) with deterministic tests and documentation comments.
```

## Prompt 05: Memory/Bios Bring-Up

```text
[Nexen][Channel F] Work #975 and #976: implement BIOS loading and memory map skeleton, plus cartridge abstraction hooks for standard and RAM-backed boards. Add hash validation and failure diagnostics.
```

## Prompt 06: Video Timing Bring-Up

```text
[Nexen][Channel F] Work #977 and #978: implement scanline/frame timing model and minimal renderer path, then add frame hash tests and reference capture comparison harness.
```

## Prompt 07: Audio + Input

```text
[Nexen][Channel F] Work #979 and #980: add audio tone generation timing and full controller/front-panel input semantics (twist/pull/push/TIME/HOLD/MODE/START) with unit tests.
```

## Prompt 08: Save States + Accuracy Harness

```text
[Nexen][Channel F] Work #981 and #982: serialize all Channel F mutable state and add deterministic replay/frame hash differential accuracy tests.
```

## Prompt 09: Core Performance Gates

```text
[Nexen][Channel F] Work #983: add Channel F core benchmarks, memory profile counters, and issue comments with baseline metrics. No optimization without before/after evidence.
```

## Prompt 10: Runtime + UI Registration

```text
[Nexen][Channel F] Work #997 and #998: register Channel F in runtime factory and add UI ROM/BIOS selection flow with user-facing diagnostics.
```

## Prompt 11: Debugger Bring-Up

```text
[Nexen][Channel F] Work #999, #1000, #1001, #1002: add F8 debugger stepping, register panes, disassembler output, memory/code/data views, and trace timeline support.
```

## Prompt 12: Pansy + Lua + Persistence

```text
[Nexen][Channel F] Work #1003, #1004, #1005, #1006, #1008: integrate Pansy metadata export/import, Lua APIs, SRAM persistence, and symbol workflows.
```

## Prompt 13: TAS/Movie Integration

```text
[Nexen][Channel F] Work #1009-#1014: implement movie/TAS support including input encoding for Channel F gestures, TAS UI widgets, and mapping presets.
```

## Prompt 14: QA, Bench, Docs, CI

```text
[Nexen][Channel F] Work #1015-#1024: finish test matrix, ROM corpus, differential harness, memory/perf budgets, docs pack, release checklist, and CI lanes.
```

## Prompt 15: First Playable Milestone

```text
[Nexen][Channel F] Produce first playable milestone branch state: BIOS boot + at least one cartridge playable with save/load and basic debugger stepping. Publish evidence in linked issues and session log.
```

## Prompt 16: Release Candidate Hardening

```text
[Nexen][Channel F] Run full correctness/performance/memory regression sweep, fix blockers, close ready issues, and prepare release-readiness signoff for #1023.
```

## Delivery Checklist (Issue #1022)

Use this checklist at the end of each prompt execution session:

1. Issue state updated with concrete evidence links (build/test logs, docs, or benchmarks).
2. Session log updated with completed steps, blockers, and next prompt number.
3. New docs linked from indexes (`~docs/README.md`, `~docs/testing/README.md`, or `docs/manual-testing/README.md`).
4. Build and targeted test commands executed and summarized in issue comments.
5. Any deferred work split into follow-up issues with parent linkage.
