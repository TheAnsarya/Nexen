# Debugger SleepUntilResume Guard Context Builder Audit (2026-05-24)

## Scope

Review `SleepUntilResume` entry guard wiring after phase/runtime/loop simplification batches.

## Findings

- Guard context setup remains a manual field-by-field assignment block in `Debugger::SleepUntilResume`.
- Field mapping is deterministic and independent of side effects.
- Manual wiring duplicates contract details now centralized elsewhere in dispatch helpers.

## Recommendation

- Introduce `BuildSleepUntilResumeGuardContext(...)` to compose guard context from runtime guard inputs.
- Replace direct assignment in `Debugger::SleepUntilResume` with helper invocation.
- Add deterministic tests for guard mapping and continuation boundary combination.

## Expected Impact

- Cleaner coordinator entry point.
- Reduced drift risk when guard schema changes.
- Explicit tested mapping contract for guard policy inputs.
