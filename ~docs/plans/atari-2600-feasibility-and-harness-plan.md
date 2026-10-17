# Atari 2600 Feasibility and Test Harness Plan

Parent issues: [#673](https://github.com/TheAnsarya/Nexen/issues/673), [#695](https://github.com/TheAnsarya/Nexen/issues/695)

## Objectives

1. Define an implementation-ready spike strategy for Atari 2600 emulation in Nexen.
2. Preserve emulator correctness by default, with measurable validation gates.
3. Decompose the work into issue-scoped increments that can be delivered independently.

## Feasibility Summary

Overall feasibility: Medium.

Reasons:

- CPU path can leverage existing 6502 design knowledge in Nexen.
- RIOT scope is small enough for an early deterministic model.
- Main technical risk is TIA timing correctness and scanline behavior.
- Mapper variety is manageable if sequenced by practical ROM prevalence.

## Spike Architecture Boundaries

### CPU Strategy (6507)

- Reuse 6502 execution model concepts with explicit 13-bit address constraints.
- Add a platform-specific memory map adapter that enforces Atari 2600 bus rules.
- Keep opcode behavior correctness equivalent to 6502 semantics.

### RIOT Scope

- Implement timer behavior needed for deterministic frame stepping.
- Implement minimal controller input path required for smoke ROM execution.
- Expose state checkpoints for regression harness assertions.

### TIA Scope (Spike)

- Implement cycle-accounted scanline stepping model.
- Provide initial visibility checkpoints for player/missile/ball rendering state.
- Defer advanced visual accuracy until baseline timing checks are stable.

### Mapper/Bankswitch Scope

- Sequence support in dependency order: F8, F6, F4, FE, E0, 3F.
- Add mapper-specific regression slots as each mapper is introduced.
- Keep mapper implementation isolated from core timing logic.

## Test Harness Plan

### Harness Goals

- Detect regressions in CPU step behavior, timer state, scanline timing, and mapper switching.
- Provide deterministic pass/fail outputs suitable for CI and local validation.

### Harness Components

1. Smoke ROM suite runner
2. Deterministic timing checkpoint verifier
3. Mapper transition verifier
4. Baseline output comparator for key test ROMs

### Measurable Milestones

| Milestone | Target | Evidence |
|---|---|---|
| M1 CPU/RIOT bring-up | Smoke ROM boots and steps for fixed frame count without crash | Deterministic frame-step log from harness |
| M2 TIA timing spike | Scanline and cycle checkpoints match expected baseline for selected ROMs | Checkpoint assertion report |
| M3 Initial mapper set | F8/F6/F4 mappings pass mapper transition tests | Mapper regression matrix report |
| M4 Expanded mapper set | FE/E0/3F mapped ROMs pass smoke and transition tests | Extended regression report |

### Baseline Validation Command Targets

- Build: `Build Nexen Release x64` task in VS Code.
- C++ tests: `bin/win-x64/Release/Core.Tests.exe --gtest_brief=1`.
- Focused harness tests: to be added during issue [#697](https://github.com/TheAnsarya/Nexen/issues/697) implementation.

## Risk Register

| Risk | Impact | Mitigation |
|---|---|---|
| TIA cycle drift | High | Add cycle-level checkpoints and fail fast on drift |
| Incorrect CPU bus mirroring | High | Isolate mapping layer and test address alias behavior |
| Mapper behavior regressions | Medium | Keep mapper tests separate and incremental by family |
| Input/timer nondeterminism | Medium | Snapshot RIOT state and assert deterministic replay |

## Issue Decomposition

| Issue | Purpose | Dependency |
|---|---|---|
| [#697](https://github.com/TheAnsarya/Nexen/issues/697) | CPU/RIOT bring-up skeleton | None |
| [#698](https://github.com/TheAnsarya/Nexen/issues/698) | TIA timing spike and smoke harness | Starts after #697 scaffolding |
| [#699](https://github.com/TheAnsarya/Nexen/issues/699) | Mapper order and regression matrix | Starts after #697, expands after #698 |

## Completion Criteria for Parent Issue #695

- Scope and risk are documented in this plan.
- Test harness milestones are measurable and explicitly sequenced.
- Follow-up implementation issues exist and are linked.

## 2026-03-20 Integration Wiring Sprint Delta

Active epic: [#830](https://github.com/TheAnsarya/Nexen/issues/830)

### New Issue Batch

- [#831](https://github.com/TheAnsarya/Nexen/issues/831) - Fix Atari lightweight CDL PRG-ROM memory resolution.
- [#832](https://github.com/TheAnsarya/Nexen/issues/832) - Implement Atari absolute-address mapping for debugger and CDL.
- [#833](https://github.com/TheAnsarya/Nexen/issues/833) - Add deterministic tests for Atari CDL/address-translation integration.

### Integration Gaps Addressed

- CPU-derived PRG-ROM memory type fallback for lightweight CDL startup now uses console PC absolute memory type when primary lookup has no registered memory.
- Atari absolute-address translation now returns typed Atari memory regions and mapper-backed ROM offsets.
- Atari PC absolute-address reporting now returns bounded ROM offsets suitable for CDL indexing.

### Current Verification Gates

- Build: `Nexen.sln` Release x64 via MSBuild.
- Focused integration tests: `Core.Tests.exe --gtest_filter=Atari2600IntegrationWiringTests.* --gtest_brief=1`.
- Focused Atari regression: `Core.Tests.exe --gtest_filter=Atari2600* --gtest_brief=1`.
- Full C++ regression: `Core.Tests.exe --gtest_brief=1`.
- Full .NET regression: `dotnet test --no-build -c Release`.
