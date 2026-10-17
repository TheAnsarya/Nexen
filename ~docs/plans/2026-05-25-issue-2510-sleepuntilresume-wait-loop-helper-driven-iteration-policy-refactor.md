# Issue #2510 SleepUntilResume Wait-Loop Helper-Driven Iteration Policy Refactor

## Scope

Refactor SleepUntilResume wait-loop orchestration to consume runtime snapshot helper builders.

## Planned Refactor

1. Replace inline loop context construction with `BuildSleepUntilResumeLoopPostBundleRuntimeContext`.
2. Replace inline post-loop context construction with `BuildSleepUntilResumePostLoopBundleRuntimeContext`.
3. Preserve `ResolveSleepUntilResumeLoopPostBundleOutcome` behavior.

## Acceptance Criteria

- Wait-loop callsite uses helper-driven context snapshots.
- Existing wait delay and resume notification behavior remains unchanged.
