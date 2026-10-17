# Issue #2520 SleepUntilResume Runtime Dispatch Execution Policy Context Builder

## Scope

Add deterministic builder that maps runtime bundle dispatch output into runtime dispatch execution policy context.

## Planned Changes

- Add `BuildSleepUntilResumeRuntimeDispatchExecutionContext(const SleepUntilResumeRuntimeBundleOutcome&)`.
- Centralize runtime dispatch execution input payload mapping.

## Acceptance Criteria

- Coordinator no longer consumes runtime bundle dispatch payload directly for execution wiring.
- Builder-driven context mapping is deterministic.
