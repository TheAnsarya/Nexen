# Issue #2486 SleepUntilResume Pre-Loop Bundle Internal Context Builders

## Scope

Extract helper builders for internal context construction in pre-loop bundle resolver.

## Planned Changes

- Add `BuildSleepUntilResumePreBreakContext(const SleepUntilResumePreLoopBundleContext&)`.
- Add `BuildSleepUntilResumePreLoopContext(const SleepUntilResumePreLoopBundleContext&)`.
- Add `BuildSleepUntilResumeDispatchContext(const SleepUntilResumePreLoopOutcome&)`.

## Acceptance Criteria

- Helper builders compile and are used by resolver.
- Mappings remain deterministic and behavior-preserving.
