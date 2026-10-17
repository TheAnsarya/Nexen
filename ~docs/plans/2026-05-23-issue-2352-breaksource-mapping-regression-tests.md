# Issue #2352 BreakSource Mapping Regression Tests (2026-05-23)

## Scope

- Add direct unit-test coverage for BreakSource mapping utility behavior.
- Verify mapped sources read expected DebugConfig fields and unmapped sources preserve default-true behavior.

## Approach

1. Extend `Core.Tests/Debugger/DebuggerDispatchUtilsTests.cpp`.
2. Add assertions for representative mapped sources across GB, NES, GBA, and SNES options.
3. Add assertions for unmapped sources (for example user/internal break sources) returning true by default.

## Acceptance Criteria

- Tests catch BreakSource-to-config mapping regressions immediately.
- Existing expected defaults are explicitly documented by assertions.
- Targeted debugger dispatch utility tests pass in CI/local runs.
