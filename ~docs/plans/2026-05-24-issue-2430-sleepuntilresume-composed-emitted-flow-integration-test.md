# Issue #2430 Plan: SleepUntilResume Composed Emitted-Flow Integration Policy Test

## Objective

Add deterministic composed-flow coverage for emitted `SleepUntilResume` runtime behavior.

## Planned Changes

1. Build emitted-phase context that enables runtime pre-break sequence.
2. Assert runtime bundle dispatch/payload and side-effect transitions.
3. Assert loop and post-loop outcomes from composed emitted state.

## Acceptance Criteria

1. Emitted composed-flow behavior is covered through phase/runtime/loop/post-loop transitions.
2. Assertions validate behavior-preserving cross-helper composition.
3. Test remains deterministic and isolated from runtime dependencies.
