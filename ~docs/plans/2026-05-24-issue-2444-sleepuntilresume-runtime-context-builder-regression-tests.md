# Issue #2444 Plan - Runtime Context Builder Regression Tests

## Goal

Add deterministic tests validating runtime context builder composition.

## Scope

- Add emitted-flow test asserting pre-loop derived flags map to context fields.
- Add non-emitted-flow test asserting pre-loop flags remain disabled while runtime payload passthrough is preserved.
- Reuse existing phase fixture helper for stable setup.

## Acceptance Criteria

- New tests cover emitted and non-emitted phase outcomes.
- Assertions include CPU/source/breakpoint/operation and notification state passthrough.
- Targeted dispatch utils test suite passes.
