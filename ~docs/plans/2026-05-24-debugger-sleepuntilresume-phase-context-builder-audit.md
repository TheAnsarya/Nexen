# Debugger SleepUntilResume Phase Context Builder Audit (2026-05-24)

## Scope

Review remaining manual wiring in `Debugger::SleepUntilResume` after runtime bundle and loop/post-loop bundle extraction.

## Findings

- `SleepUntilResumeGuardContext` is built once, but `SleepUntilResumePhaseContext` is still manually wired in two places.
- The second phase wiring pass only differs by debug-config toggles, making it a deterministic mapping operation.
- Keeping direct field assignment in the coordinator duplicates contract knowledge and increases drift risk.

## Recommendation

- Introduce `BuildSleepUntilResumePhaseContext(...)` in `DebuggerDispatchUtils`.
- Use the helper for both pre-config and post-config phase outcome passes.
- Add deterministic tests for guard/source/break/config mapping.

## Expected Impact

- Thinner coordinator with less repetitive assignment logic.
- Better change resilience when phase context schema evolves.
- Clear mapping contract under unit-test coverage.
