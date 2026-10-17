# Issue #2327 Plan - NotificationManager Lifecycle Simplification

## Scope

Simplify listener lifecycle handling and reduce cleanup overhead in `NotificationManager` while keeping semantics unchanged.

## Acceptance Criteria

- Define current listener lifecycle pain points and target model.
- Implement container/cleanup simplification with equivalent observable behavior.
- Add or update tests for registration, dispatch, and cleanup behavior.
- Validate no regressions in touched tests and Release x64 build.
