# Issue #2461 Plan - SleepUntilResume Coordinator Policy Entry Helper

## Goal

Extract helper that composes and resolves the `SleepUntilResume` entry policy model.

## Scope

- Add coordinator entry context/outcome model to dispatch helpers.
- Add resolver that returns phase context plus phase outcome from entry inputs.
- Preserve existing decision semantics.

## Acceptance Criteria

- Entry helper supports both initial and post-config entry passes.
- Phase context + outcome are produced consistently.
- No behavioral regressions.
