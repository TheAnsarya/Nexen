# Issue #2340 CPU State-Size Mapping Regression Tests (2026-05-23)

## Scope

- Add direct regression coverage for CPU state-layout mapping used by debugger state-copy dispatch.
- Lock in mappings for all currently supported CPU types.

## Approach

1. Extend `Core.Tests/Debugger/DebuggerDispatchUtilsTests.cpp`.
2. Add assertions validating `GetCpuStateLayout(CpuType)` output for each CPU family.
3. Keep tests deterministic and isolated from emulator runtime setup.

## Acceptance Criteria

- New tests cover all CPU families currently mapped by debugger state-copy dispatch.
- Tests pass under `DebuggerDispatchUtilsTests.*` targeted filter.
- Mapping regressions are immediately visible via unit-test failure.
