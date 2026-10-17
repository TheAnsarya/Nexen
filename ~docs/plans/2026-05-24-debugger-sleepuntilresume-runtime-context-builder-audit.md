# Debugger SleepUntilResume Runtime Context Builder Audit (2026-05-24)

## Scope

Assess remaining coordinator wiring in `Debugger::SleepUntilResume` after phase, runtime bundle, and loop/post-loop bundle extraction.

## Findings

- The runtime bundle call site still manually maps phase pre-loop flags and runtime payload fields into `SleepUntilResumeRuntimeBundleContext`.
- This mapping duplicates policy glue that is deterministic and testable in isolation.
- Keeping this mapping in the coordinator increases surface area for future field drift when context schema changes.

## Recommendation

- Introduce a dedicated context-builder helper in `DebuggerDispatchUtils` that composes `SleepUntilResumeRuntimeBundleContext` from:
- `SleepUntilResumePhaseOutcome`
- runtime payload (`sourceCpu`, `source`, `breakpointId`, `operation`, `notificationSent`)

## Expected Impact

- Further coordinator thinning with no behavior change.
- Centralized mapping contract covered by deterministic tests.
- Lower regression risk for future context field additions.
