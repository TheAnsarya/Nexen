# Debugger SleepUntilResume Loop/Post Context Builder Audit (2026-05-24)

## Scope

Review remaining coordinator-level context wiring in `Debugger::SleepUntilResume` loop and post-loop stages.

## Findings

- `SleepUntilResumeLoopPostBundleContext` is still manually composed at two call sites.
- Both mappings are deterministic, differ only by input values, and are policy contract plumbing.
- Manual assignments duplicate schema knowledge and increase drift risk for future field changes.

## Recommendation

- Introduce `BuildSleepUntilResumeLoopPostBundleContext(...)` in dispatch helpers.
- Replace both manual call-site mappings in `Debugger::SleepUntilResume`.
- Add regression tests for builder field mapping and idle/non-emitted composition behavior.

## Expected Impact

- Cleaner coordinator loop and post-loop sections.
- Reduced field-mapping duplication.
- Explicit tested context-construction contract.
