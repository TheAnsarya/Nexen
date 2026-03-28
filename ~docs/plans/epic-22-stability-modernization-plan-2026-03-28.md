# Epic 22 Stability and Modernization Plan (2026-03-28)

## Scope

This plan tracks execution for [Epic 22] Stability, Runtime Compatibility, and Modernization Program:

1. Linux crash/segfault triage and mitigation tracking.
2. Runtime dependency validation in CI and packaging flows.
3. Warning-baseline capture and low-risk warning elimination.
4. Safe modernization of dependency versions and practices.
5. Regression and benchmark evidence capture for every pass.

Issue mapping:

1. #1048 Epic 22 parent.
2. #1049 Linux crash and segfault triage matrix.
3. #1050 Modern library/version baseline and upgrade policy.
4. #1051 Warning baseline and compiler profile hardening.
5. #1054 CI runtime dependency validation expansion.
6. #1052 Regression and benchmark evidence pack.
7. #1053 Documentation and continuity pack.

## Current Execution Snapshot

1. Linux ICU runtime guardrails are now enforced in CI (`build.yml`) for Linux and AppImage jobs.
2. Runtime config is validated for `System.Globalization.AppLocalIcu = 72.1.0.3` and required ICU native files.
3. Warning pass identified and reduced MSB3277 conflict hotspots.
4. Dependency modernization pass completed for low-risk packages (`Microsoft.Extensions.Logging*`, `StreamHash`).

## Phased Approach

### Phase A: Crash and Library Stability

1. Maintain Linux startup crash signatures and repro metadata (distros, runtime versions, stack traces).
2. Keep runtime dependency assertions in CI to prevent regression of missing native assets.
3. Track known risk areas:
- App-local ICU loading and version token drift.
- Native graphics stack compatibility (Skia/HarfBuzz/system libs).

### Phase B: Warning Budget Hardening

1. Capture warning baseline from Release build logs.
2. Prioritize warning fixes that are:
- Low-risk.
- Deterministic.
- Non-behavioral.
3. Keep warning deltas in testing docs and issue comments for traceability.

### Phase C: Modernization Cadence

1. Apply modernization in compatibility-preserving increments.
2. Validate after each bump:
- Full Release build.
- Focused native tests.
- .NET tests.
- Benchmark checkpoint.
3. Defer risky transitive jumps to dedicated issues with rollback notes.

## Acceptance Criteria

1. No known critical Linux startup crash regressions caused by missing ICU artifacts.
2. CI fails fast when Linux runtime assets are missing or misconfigured.
3. Warning baseline is improved or clearly documented with blockers.
4. Regression tests and benchmark checkpoints are recorded each pass.
5. Documentation, chat logs, and session logs are linked from [~docs/README.md](../README.md).

## Next Actions

1. Expand runtime validation beyond ICU where practical (Skia/HarfBuzz presence checks).
2. Add lightweight warning report artifact publication in CI.
3. Continue dependency modernization with compatibility checks per batch.
