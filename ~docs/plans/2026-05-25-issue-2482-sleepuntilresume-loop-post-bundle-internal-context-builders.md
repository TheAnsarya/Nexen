# Issue #2482 SleepUntilResume Loop-Post Bundle Internal Context Builders

## Scope

Add helper builders for internal context construction in loop-post bundle resolver.

## Planned Changes

- Add `BuildSleepUntilResumeLoopContext(const SleepUntilResumeLoopPostBundleContext&)`.
- Add `BuildSleepUntilResumePostLoopContext(const SleepUntilResumeLoopPostBundleContext&)`.

## Acceptance Criteria

- Builder helpers compile and map fields deterministically.
- Resolver can consume helper outputs without behavior changes.
