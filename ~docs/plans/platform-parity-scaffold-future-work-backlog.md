# Platform Parity Scaffold Future-Work Backlog

Parent epic: [#673](https://github.com/TheAnsarya/Nexen/issues/673)

Status: Future Work only. Do not start implementation from this backlog until explicitly scheduled, except for explicitly promoted items listed below.

Promoted and completed from backlog:

- [#719](https://github.com/TheAnsarya/Nexen/issues/719): Cycle-accurate 6507 execution semantics (completed 2026-03-17).
- [#720](https://github.com/TheAnsarya/Nexen/issues/720): RIOT timer and I/O edge semantics (completed 2026-03-17).
- [#721](https://github.com/TheAnsarya/Nexen/issues/721): TIA scanline timing and WSYNC/HMOVE edge semantics (completed 2026-03-17).
- [#722](https://github.com/TheAnsarya/Nexen/issues/722): TIA playfield and object rendering pipeline (completed 2026-03-17).
- [#723](https://github.com/TheAnsarya/Nexen/issues/723): TIA audio channels and deterministic mixer path (completed 2026-03-17).
- [#724](https://github.com/TheAnsarya/Nexen/issues/724): Behavior-accurate mapper follow-through (completed 2026-03-17).
- [#725](https://github.com/TheAnsarya/Nexen/issues/725): ROM corpus compatibility matrix/checkpoint harness expansion (completed 2026-03-17).
- [#726](https://github.com/TheAnsarya/Nexen/issues/726): Save-state serialization and restore determinism gates (completed 2026-03-17).
- [#727](https://github.com/TheAnsarya/Nexen/issues/727): Performance gate execution after correctness stabilization (completed 2026-03-17).
- [#728](https://github.com/TheAnsarya/Nexen/issues/728): M68000 core semantics and exception/interrupt flow (completed 2026-03-17).
- [#729](https://github.com/TheAnsarya/Nexen/issues/729): Genesis memory map decode and bus ownership rules (completed 2026-03-17).
- [#730](https://github.com/TheAnsarya/Nexen/issues/730): VDP register file semantics and status behavior (completed 2026-03-17).
- [#731](https://github.com/TheAnsarya/Nexen/issues/731): VDP background/sprite rendering pipeline scaffold (completed 2026-03-17).
- [#732](https://github.com/TheAnsarya/Nexen/issues/732): VDP DMA modes and contention timing scaffold (completed 2026-03-17).
- [#733](https://github.com/TheAnsarya/Nexen/issues/733): H/V interrupt scheduling and frame timing scaffold (completed 2026-03-17).
- [#734](https://github.com/TheAnsarya/Nexen/issues/734): Z80 bootstrap and shared-bus handoff scaffold (completed 2026-03-17).
- [#735](https://github.com/TheAnsarya/Nexen/issues/735): YM2612 synthesis and timing scaffold (completed 2026-03-17).
- [#736](https://github.com/TheAnsarya/Nexen/issues/736): SN76489 PSG and mixed-output scaffold (completed 2026-03-17).
- [#737](https://github.com/TheAnsarya/Nexen/issues/737): Sonic/Jurassic compatibility harness expansion (completed 2026-03-17).
- [#738](https://github.com/TheAnsarya/Nexen/issues/738): Save-state serialization and replay determinism gates (completed 2026-03-17).
- [#739](https://github.com/TheAnsarya/Nexen/issues/739): Performance gate execution after correctness stabilization (completed 2026-03-17).

## Purpose

Backfill the completed scaffold phases with a deferred execution tree so Atari 2600 and Genesis scaffold work can be converted into production-ready implementations later without losing scope, dependencies, or validation expectations.

## Sub-Epics

| Sub-Epic | Scope | Status |
|---|---|---|
| [#717](https://github.com/TheAnsarya/Nexen/issues/717) | Atari 2600 scaffold-to-production implementation program | Future Work |
| [#718](https://github.com/TheAnsarya/Nexen/issues/718) | Genesis scaffold-to-production implementation program | Future Work |

## Atari 2600 Future-Work Tree

Scaffold lineage:

- [#697](https://github.com/TheAnsarya/Nexen/issues/697)
- [#698](https://github.com/TheAnsarya/Nexen/issues/698)
- [#699](https://github.com/TheAnsarya/Nexen/issues/699)
- [#711](https://github.com/TheAnsarya/Nexen/issues/711)
- [#712](https://github.com/TheAnsarya/Nexen/issues/712)
- [#713](https://github.com/TheAnsarya/Nexen/issues/713)

Deferred child issues:

| Issue | Focus |
|---|---|
| [#719](https://github.com/TheAnsarya/Nexen/issues/719) | Cycle-accurate 6507 execution semantics (completed; promoted from deferred backlog) |
| [#720](https://github.com/TheAnsarya/Nexen/issues/720) | RIOT timer and I/O edge behavior (completed; promoted from deferred backlog) |
| [#721](https://github.com/TheAnsarya/Nexen/issues/721) | TIA timing and WSYNC/HMOVE edge cases (completed; promoted from deferred backlog) |
| [#722](https://github.com/TheAnsarya/Nexen/issues/722) | TIA playfield and object rendering pipeline (completed; promoted from deferred backlog) |
| [#723](https://github.com/TheAnsarya/Nexen/issues/723) | TIA audio channels and deterministic mixer integration (completed; promoted from deferred backlog) |
| [#724](https://github.com/TheAnsarya/Nexen/issues/724) | Behavior-accurate mapper implementations (completed; promoted from deferred backlog) |
| [#725](https://github.com/TheAnsarya/Nexen/issues/725) | ROM corpus compatibility harness and checkpoint reports (completed; promoted from deferred backlog) |
| [#726](https://github.com/TheAnsarya/Nexen/issues/726) | Save-state serialization and determinism gates (completed; promoted from deferred backlog) |
| [#727](https://github.com/TheAnsarya/Nexen/issues/727) | Performance gate execution after correctness stabilization (completed; promoted from deferred backlog) |

## Genesis Future-Work Tree

Scaffold lineage:

- [#700](https://github.com/TheAnsarya/Nexen/issues/700)
- [#701](https://github.com/TheAnsarya/Nexen/issues/701)
- [#702](https://github.com/TheAnsarya/Nexen/issues/702)
- [#703](https://github.com/TheAnsarya/Nexen/issues/703)
- [#715](https://github.com/TheAnsarya/Nexen/issues/715)
- [#716](https://github.com/TheAnsarya/Nexen/issues/716)

Deferred child issues:

| Issue | Focus |
|---|---|
| [#728](https://github.com/TheAnsarya/Nexen/issues/728) | M68000 core semantics and interrupt/exception flow (completed; promoted from deferred backlog) |
| [#729](https://github.com/TheAnsarya/Nexen/issues/729) | Full memory-map decode and bus ownership rules (completed; promoted from deferred backlog) |
| [#730](https://github.com/TheAnsarya/Nexen/issues/730) | VDP register and status semantics (completed; promoted from deferred backlog) |
| [#731](https://github.com/TheAnsarya/Nexen/issues/731) | VDP background and sprite rendering pipeline (completed; promoted from deferred backlog) |
| [#732](https://github.com/TheAnsarya/Nexen/issues/732) | VDP DMA modes and contention timing behavior (completed; promoted from deferred backlog) |
| [#733](https://github.com/TheAnsarya/Nexen/issues/733) | H/V interrupt scheduling and frame event timing (completed; promoted from deferred backlog) |
| [#734](https://github.com/TheAnsarya/Nexen/issues/734) | Z80 subsystem bootstrap and shared-bus handoff (completed; promoted from deferred backlog) |
| [#735](https://github.com/TheAnsarya/Nexen/issues/735) | YM2612 synthesis and timing synchronization (completed; promoted from deferred backlog) |
| [#736](https://github.com/TheAnsarya/Nexen/issues/736) | SN76489 PSG path and mixed output integration (completed; promoted from deferred backlog) |
| [#737](https://github.com/TheAnsarya/Nexen/issues/737) | Sonic 1 and Jurassic Park compatibility harness execution (completed; promoted from deferred backlog) |
| [#738](https://github.com/TheAnsarya/Nexen/issues/738) | Save-state serialization and replay determinism gates (completed; promoted from deferred backlog) |
| [#739](https://github.com/TheAnsarya/Nexen/issues/739) | Performance gate execution after correctness stabilization (completed; promoted from deferred backlog) |

## Scheduling Rule

All issues in this backlog remain parked as future work unless one of the following occurs:

1. They are explicitly pulled into an active milestone by the maintainer.
2. A dedicated execution sprint is declared for Atari or Genesis parity completion.
3. A compatibility blocker requires selective promotion of one future-work item.

## References

- [Q3 Platform Parity Research Program](platform-parity-research-program-2026-q3.md)
- [Nexen Future Work Index](../../docs/FUTURE-WORK.md)

## 2026-03-21 Active Atari 2600 Queue Refresh

The following Atari 2600 issues were promoted/created for immediate production hardening work:

- [#873](https://github.com/TheAnsarya/Nexen/issues/873): TAS editor controller subtype detection and paddle-coordinate gap tracking.
- [#874](https://github.com/TheAnsarya/Nexen/issues/874): TAS editor paddle coordinate editing support.
- [#875](https://github.com/TheAnsarya/Nexen/issues/875): 6507 opcode and addressing conformance corpus expansion.
- [#876](https://github.com/TheAnsarya/Nexen/issues/876): Production-readiness validation matrix and checklist documentation.
- [#877](https://github.com/TheAnsarya/Nexen/issues/877): Atari2600Console monolith refactor into split implementation units.
- [#878](https://github.com/TheAnsarya/Nexen/issues/878): Atari 2600 Pansy metadata export parity.
