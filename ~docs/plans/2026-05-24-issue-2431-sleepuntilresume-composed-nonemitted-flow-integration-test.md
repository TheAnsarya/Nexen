# Issue #2431 Plan: SleepUntilResume Composed Non-Emitted-Flow Integration Policy Test

## Objective

Add deterministic composed-flow coverage for non-emitted `SleepUntilResume` runtime behavior.

## Planned Changes

1. Build non-emitted phase context (unspecified source + break request).
2. Assert runtime bundle disables dispatch and side-effect transitions.
3. Assert loop and post-loop outcomes remain behavior-preserving for non-emitted state.

## Acceptance Criteria

1. Non-emitted composed-flow behavior is explicitly covered.
2. Dispatch/sent-state/screen side-effect suppression is asserted.
3. Test guards against composition regressions in non-emitted flow.
