# Issue #2363 ProcessEvent Routing Helper Regression Tests (2026-05-23)

## Scope

- Add regression coverage for newly extracted ProcessEvent routing helpers.
- Validate both requested-CPU routing and fallback decision behavior.

## Approach

1. Extend `DebuggerDispatchUtilsTests` with direct helper assertions.
2. Cover available and unavailable requested CPU scenarios.
3. Cover input-debugger fallback decision combinations.

## Acceptance Criteria

- Helper routing behavior is explicitly locked by unit tests.
- Tests are deterministic and runtime-isolated.
- Targeted utility test filter passes.
