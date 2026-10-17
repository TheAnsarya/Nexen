# Issue #2476 SleepUntilResume Runtime-Bundle Dispatch Context Builder

## Scope

Extract helper that maps `SleepUntilResumeRuntimeBundleContext` into `SleepUntilResumeRuntimeDispatchContext`.

## Planned Changes

- Add `BuildSleepUntilResumeRuntimeDispatchContext(const SleepUntilResumeRuntimeBundleContext&)`.
- Keep mapping deterministic and field-for-field.
- Preserve runtime dispatch behavior.

## Acceptance Criteria

- Builder compiles and is used by runtime-bundle resolver flow.
- No behavior change in runtime dispatch outcomes.
