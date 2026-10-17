# Issue #2442 Plan: SleepUntilResume Loop/Post-Loop Bundle Refactor

## Objective

Refactor `SleepUntilResume` loop and tail post-loop paths to consume composed loop/post-loop bundle outcomes.

## Planned Changes

1. Replace loop phase context setup with loop/post-loop bundle context and outcome.
2. Replace tail post-loop phase context setup with loop/post-loop bundle context and outcome.
3. Preserve behavior and execution ordering.

## Acceptance Criteria

1. Loop/tail coordinator behavior is unchanged.
2. Coordinator-local setup is reduced through bundle usage.
3. Tests/build pass for touched areas.
