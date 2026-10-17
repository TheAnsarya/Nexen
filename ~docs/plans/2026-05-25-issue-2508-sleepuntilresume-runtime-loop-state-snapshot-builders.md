# Issue #2508 SleepUntilResume Runtime Loop-State Snapshot Builders

## Scope

Extract deterministic helpers for runtime loop/post-loop bundle context snapshots.

## Planned Changes

- Add `BuildSleepUntilResumeLoopPostBundleRuntimeContext(bool waitForBreakResume, int32_t suspendRequestCount, int32_t breakRequestCount)`.
- Add `BuildSleepUntilResumePostLoopBundleRuntimeContext(bool notificationSent)`.

## Acceptance Criteria

- Helpers centralize runtime-to-context mapping for SleepUntilResume loop flow.
- Coordinator callsite no longer performs direct count-to-boolean mapping.
