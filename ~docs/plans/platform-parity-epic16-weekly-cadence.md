# Epic 16 Weekly Cadence and Triage Checklist

Issue linkage: [#800](https://github.com/TheAnsarya/Nexen/issues/800)

Parent issue: [#776](https://github.com/TheAnsarya/Nexen/issues/776)

## Weekly Cadence

### Monday: Plan and Scope

1. Select 2-4 Epic 16 issues for the week.
2. Confirm issue acceptance criteria are explicit.
3. Confirm required test and benchmark commands.
4. Add short execution plan comment on each selected issue.

### Tuesday-Wednesday: Implement and Validate

1. Implement one issue at a time.
2. Run focused tests after each implementation slice.
3. Run focused benchmarks for performance-relevant changes.
4. Record command output summaries in issue comments.

### Thursday: Regression and Drift Checks

1. Run reference artifact pack:
   - `./scripts/reference-validation-pack.ps1 -SkipBuild`
2. Compare against baseline:
   - `powershell -File scripts/compare-reference-validation.ps1 -BaselineDir artifacts/reference-validation-baseline -CandidateDir artifacts/reference-validation`
3. Investigate any digest drift or benchmark deltas above 5%.

### Friday: Close and Report

1. Close issues with commit + evidence summary.
2. Update closure matrix:
   - [platform-parity-epic16-closure-matrix.md](platform-parity-epic16-closure-matrix.md)
3. Post an Epic 16 weekly status comment with:
   - Closed issues
   - Remaining blockers
   - Next-week priorities

## Triage Categories

Use these triage categories in issue comments:

- `correctness-blocker`: deterministic mismatch, digest drift, or regression.
- `perf-watch`: benchmark slowdown above threshold requiring analysis.
- `needs-rom-data`: requires additional local reference ROM evidence.
- `docs-followup`: implementation done, evidence/docs still pending.
- `ready-to-close`: acceptance criteria met and evidence attached.

## Evidence Expectations

Each issue should include:

1. Build confirmation (or explicit not-required for docs-only tasks).
2. Focused test command and pass summary.
3. Benchmark command and summary for performance-relevant changes.
4. Artifact path(s) for generated output.
5. Commit hash tied to issue number.
