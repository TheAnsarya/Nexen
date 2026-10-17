# Issue #2474 SleepUntilResume Pre-Break Action Plan Regression Tests

## Scope

Add direct unit coverage for pre-break action plan helper composition and outcome behavior.

## Planned Tests

- Context builder maps emitted flow fields from phase outcome.
- Context builder disables fields for non-emitted flow.
- Resolver disables action calls when sequence does not run.
- Resolver propagates action calls and flags when sequence runs.

## Acceptance Criteria

- New tests run in `DebuggerDispatchUtilsTests`.
- Emitted/non-emitted scenarios are both covered.
- No regressions in existing SleepUntilResume helper tests.
