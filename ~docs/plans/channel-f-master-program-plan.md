# Channel F Master Program Plan

## Program Identity

- Program: Fairchild Channel F first-class support in Nexen
- Branch: `features-channel-f`
- Parent epics: #968, #969, #970
- Program principle: accuracy and timing correctness first, then performance

## Outcomes

- Add production-quality Channel F emulation core to Nexen.
- Add full platform integration across UI, debugger, Lua, Pansy, save states, and TAS/movie.
- Support Channel F System I, System II, and documented variant behavior where feasible.
- Ship with comprehensive test/benchmark evidence and full documentation.

## Local ROM Inputs

- BIOS path: C:\~reference-roms\channel f\Channel F BIOS (1976) (Fairchild, Luxor)\
- ROM root path: C:\~reference-roms\channel f\ (use subfolders for per-title matrices)
- Use these as canonical local sources for #971, #975, #976, #982, and #1016.

## Workstreams

## 1. Core Emulation

- CPU and bus model: #972, #973, #974
- Memory/BIOS/cart: #975, #976
- Video/audio/input: #977, #978, #979, #980
- Save states/accuracy/perf gates: #981, #982, #983

## 2. Platform Integration

- Runtime registration/UI flows: #997, #998
- Debugger/disassembler/tracing: #999, #1000, #1001, #1002
- Pansy integration: #1003, #1004, #1008
- Lua and persistence metadata: #1005, #1006, #1007

## 3. TAS, QA, and Delivery

- Movie/TAS support: #1009, #1010, #1011, #1012
- Input UX and accessibility: #1013, #1014
- Validation and benchmark program: #1015, #1016, #1017, #1018, #1019
- Docs, release, CI: #1020, #1021, #1022, #1023, #1024

## Session Cadence

- Session A: Research closure + architecture contracts (#971, #972 skeleton)
- Session B: CPU decode/execute and bus semantics (#973, #974)
- Session C: Memory map, BIOS/cart bring-up (#975, #976)
- Session D: Video timing and renderer verification (#977, #978)
- Session E: Audio and input semantics (#979, #980)
- Session F: Save states and differential harness (#981, #982)
- Session G: Performance baselines and guardrails (#983)
- Session H: UI runtime integration (#997, #998)
- Session I: Debugger/disassembler/trace (#999, #1000, #1001, #1002)
- Session J: Pansy/Lua/persistence flows (#1003, #1004, #1005, #1006, #1008)
- Session K: TAS/movie/input UX (#1009-#1014)
- Session L: QA/docs/release/CI closure (#1015-#1024)

## Risk Register

| Risk | Impact | Mitigation |
|---|---|---|
| F8 timing ambiguities in references | High | Differential tests vs multiple references and documented assumptions |
| BIOS/cart variant behavior mismatch | High | Hash-based BIOS catalog and per-cart exception matrix |
| Debugger integration lag | Medium | Parallel disassembler metadata milestones (#1000) |
| TAS determinism drift | High | Determinism tests and replay hash checks per frame |
| Scope creep | High | Strict issue-gated execution and session checklists |

## Quality Gates

- Core correctness gate: CPU + memory + video + input deterministic tests pass.
- Accuracy gate: Differential harness reports accepted deltas only.
- Performance gate: No regression against baseline budgets.
- Tooling gate: Debugger/TAS/movie workflows operational.
- Documentation gate: Architecture + user + troubleshooting docs complete.

## Definition of Done

- All issues #968-#1024 closed with evidence comments.
- Channel F appears as first-class system option in Nexen UI and docs.
- CI lanes for Channel F tests/benchmarks are green.
- Release readiness checklist #1023 signed off.

## 2026-03-26 Progress Checkpoint

- Completed a cross-repo Channel F metadata slice tied to #1030 and #1031.
- Nexen now exports Channel F `.pansy` files using platform id `0x1f` (Fairchild Channel F).
- Channel F system memory regions are now emitted in export output:
	- Cartridge ROM (`$0000-$17ff`)
	- System RAM (`$2800-$2fff`)
	- Video RAM (`$3000-$37ff`)
	- I/O registers (`$3800-$38ff`)
- Validation evidence captured this session:
	- `Core.Tests.exe --gtest_filter=ChannelF* --gtest_brief=1` (15/15 passed)
	- `Core.Benchmarks.exe --benchmark_filter=BM_ChannelFCore_* --benchmark_repetitions=3`
	- Release x64 solution build successful

## 2026-03-26 Progress Checkpoint (Session 15)

- Closed #1033: Exhaustive BIOS database tests (all 4 entries + unknown hashes + IsKnownLuxorSystemII).
- Closed #1033: Opcode table tests (defined count, metadata consistency, all-opcode scan).
- Added 3 new benchmarks: SerializeRoundtrip (~23µs), Reset (~3ns), OpcodeTableFullScan (~150ns).
- Channel F test count: 32 gtest tests (up from 16), 8 benchmarks (up from 5).
- Cross-repo: pansy #90 closed (CDL/cross-ref/symbol/full-file roundtrip, 543 tests pass).
- Cross-repo: poppy #197 closed (12 alias theory tests, 1957 tests pass).
- All builds clean, all tests green across Nexen/pansy/poppy.

## 2026-03-27 Progress Checkpoint (Session 16)

- Cleanup: Closed Nexen #1028 (microbenchmark harness — covered by existing 8 benchmarks), pansy #88/#89 (stale duplicates).
- Closed Nexen #1034: 6 new scaffold behavior tests (input divergence, multi-reset idempotency, all 4 BIOS variant detection, unknown hash, zero-input safety, mid-run input change divergence).
- Channel F test count: 38 gtest tests (up from 32), 8 benchmarks clean.
- Cross-repo: pansy #91 closed (compressed roundtrip + metadata section tests, 546 tests pass).
- Cross-repo: poppy #198 closed (Channel F include file with full F8 hardware definitions, 1957 tests pass).
- All builds clean, all tests green across Nexen/pansy/poppy.

## 2026-03-27 Progress Checkpoint (Session 17)

- Closed Nexen #1035: 3 new serialize edge case tests (frame-0 roundtrip, max-inputs roundtrip, variant preservation).
- Channel F test count: 41 gtest tests (up from 38), 8 benchmarks clean.
- Cross-repo: pansy #92 closed (Channel F bookmark + CPU state roundtrip tests, 549 tests pass).
- Cross-repo: poppy #199 closed (Channel F hello-world example project template, 1957 tests pass).
- All builds clean, all tests green across Nexen/pansy/poppy.
