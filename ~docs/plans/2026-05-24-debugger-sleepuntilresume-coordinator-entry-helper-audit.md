# Debugger SleepUntilResume Coordinator Entry Helper Audit (2026-05-24)

## Scope

Assess entry-phase wiring in `Debugger::SleepUntilResume` after prior helper extraction slices.

## Findings

- Coordinator still directly handles entry-phase context construction and phase-outcome resolution in two passes.
- This repeated orchestration is deterministic and policy-focused, suitable for one reusable helper.
- Consolidating entry resolution reduces coordinator branching noise and centralizes policy-entry evolution.

## Recommendation

- Introduce a coordinator entry helper that resolves phase context + phase outcome from guard/source/break/config inputs.
- Use it for both initial decision pass and post-config pass.
- Add deterministic tests for continue/skip and emitted/non-emitted entry behavior.

## Expected Impact

- Cleaner `SleepUntilResume` entry section.
- Reduced duplication in phase entry orchestration.
- Better testability for top-level policy entry behavior.
