# Issue #2517 SleepUntilResume Runtime Side-Effect Application Context Builder

## Scope

Extract deterministic builder that maps runtime bundle side-effect outputs and runtime state into a side-effect application context.

## Planned Changes

- Add `BuildSleepUntilResumeRuntimeSideEffectApplicationContext(...)`.
- Include wait-for-break-resume, notification-sent, and runtime side-effect payload fields.

## Acceptance Criteria

- Builder centralizes runtime side-effect application input mapping.
- Coordinator no longer performs ad-hoc mapping for this stage.
