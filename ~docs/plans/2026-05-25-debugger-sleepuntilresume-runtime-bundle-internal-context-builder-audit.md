# Debugger SleepUntilResume Runtime-Bundle Internal Context Builder Audit (2026-05-25)

## Context

`ResolveSleepUntilResumeRuntimeBundleOutcome` still manually assembled nested dispatch and side-effect contexts inline. This wiring is deterministic and should be represented with explicit builder helpers.

## Risk

Inline field mapping duplicates orchestration wiring and increases coordinator maintenance cost when payload fields change.

## Slice Trio

1. #2476 extract runtime-bundle dispatch context builder helper.
2. #2477 add regression tests for runtime-bundle internal context builders.
3. #2478 refactor runtime-bundle resolver to use extracted builders.

## Acceptance

- Runtime-bundle resolver no longer manually wires nested context fields inline.
- Builder tests cover payload and notification-mark propagation.
- Existing runtime-bundle behavior remains unchanged.
