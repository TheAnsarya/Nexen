# Debugger ProcessEvent Architecture Audit (2026-05-23)

## Objective

Identify high-leverage simplification opportunities in debugger event handling that reduce redundancy while preserving emulator behavior.

## Current Hotspot

`Core/Debugger/Debugger.cpp` contains `Debugger::ProcessEvent`, which mixes three concerns:

1. CPU routing decisions (requested CPU vs fallback to main CPU).
2. Script dispatch ownership checks.
3. Event-specific side effects (`InputPolled`, `StartFrame`, `Reset`, `StateLoaded`).

This coupling increases maintenance overhead and makes behavior regression-prone when adding new CPU types or event kinds.

## Redundancy Findings

- CPU routing decisions repeated in local branches rather than centralized helper semantics.
- Input debugger fallback behavior encoded inline in `InputPolled` branch.
- Shared dispatch utility layer already exists for adjacent mapping concerns (pause scanline, state layout, PPU backend, break-source flags), but ProcessEvent routing was not yet aligned.

## Selected Cleanup Slice

Use the existing `DebuggerDispatchUtils` pattern to centralize ProcessEvent routing decisions and make the behavior directly unit-testable.

### Why This Slice

- Low risk: behavior-preserving mapping extraction.
- High maintainability gain: removes local decision duplication and clarifies intent.
- Testable without full emulator runtime: pure utility helpers can be validated via focused gtests.

## Issue Decomposition

- #2361: Extract ProcessEvent CPU routing decisions into shared helper utilities.
- #2363: Add regression tests for routing helper behavior.
- #2364: Refactor `Debugger::ProcessEvent` to consume shared helpers.

## Validation Strategy

1. Targeted utility tests:
	- `Core.Tests.exe --gtest_filter=DebuggerDispatchUtilsTests.* --gtest_brief=1`
2. Release build verification:
	- `MSBuild Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build`

## Follow-Up Candidates

1. Extract script ownership dispatch decision into dedicated helper.
2. Normalize event-side-effect handlers into private member methods per event type.
3. Continue switch-reduction passes in debugger coordinator with utility-first mapping strategy.
