# Issue #2478 SleepUntilResume Runtime-Bundle Internal Context Builder Refactor

## Scope

Refactor `ResolveSleepUntilResumeRuntimeBundleOutcome` to consume extracted internal context builders.

## Planned Refactor

1. Replace inline construction of runtime dispatch context with helper builder.
2. Replace inline construction of runtime side-effect context with helper builder.
3. Keep outcome resolution flow and semantics unchanged.

## Acceptance Criteria

- Resolver uses builder helpers for both nested context construction paths.
- Existing runtime-bundle regression and composed-flow tests remain green.
