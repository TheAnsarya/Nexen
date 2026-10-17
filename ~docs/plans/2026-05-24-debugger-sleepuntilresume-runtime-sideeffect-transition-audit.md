# Debugger SleepUntilResume Runtime Side-Effect Transition Audit (2026-05-24)

## Scope

Architecture audit of runtime side-effect transitions in `SleepUntilResume` for:

1. Wait-for-break resume arming
2. Screensaver enable side effect
3. Notification-sent state transition

## Findings

1. Runtime side-effect transitions were deterministic but manually wired inline in coordinator code.
2. Notification-sent promotion logic was coupled to dispatch policy checks at call sites.
3. Coordinator maintained repetitive local transition wiring that reduced readability.

## Simplification Strategy

1. Introduce a runtime side-effect transition helper with explicit context/outcome types.
2. Centralize wait-arm, screensaver-enable, and notification-sent promotion logic into one pure helper.
3. Refactor coordinator side-effect block to consume the composed transition outcome.
4. Add deterministic tests for emitted and non-emitted transition paths.

## Implemented Slice

This audit maps to:

1. #2424 runtime side-effect transition helper extraction
2. #2425 runtime side-effect transition regression tests
3. #2426 runtime side-effect transition refactor

## Next Candidate Slice

1. Compose pre-loop sequence entry and runtime dispatch/side-effect transitions into one higher-order runtime bundle.
2. Add integration-level path coverage for full emitted vs non-emitted SleepUntilResume execution flow.
