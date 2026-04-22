# Epic 21 Settings Visual Snapshot Baseline (2026-04-21)

## Scope

This checkpoint provides #1408 visual-regression snapshot structure and capture criteria for the settings-window usability updates.

## Capture Targets

- Setup Wizard default and minimum-size views
- Config Window default and minimum-size views
- Debugger Config Window default and minimum-size views

## Expected Baseline Files

- `setup-wizard-default.png`
- `setup-wizard-minimum.png`
- `config-window-default.png`
- `config-window-minimum.png`
- `debugger-config-default.png`
- `debugger-config-minimum.png`

## Storage Paths

- Baseline path: `~docs/testing/visual-baselines/epic-21/baseline`
- Candidate path: `~docs/testing/visual-baselines/epic-21/candidate`

## Comparison Workflow

```powershell
.\~scripts\compare-epic21-ui-visual-baselines.ps1 `
	-BaselineDir "~docs/testing/visual-baselines/epic-21/baseline" `
	-CandidateDir "~docs/testing/visual-baselines/epic-21/candidate"
```

Result file:

- `~docs/testing/data/epic-21-visual-regression-report.md`

## Acceptance Mapping for #1408

- Minimum hit targets and spacing guidelines enforced: covered by markup contract tests and explicit `MinHeight="36"` action controls in key settings surfaces.
- New default window size and resize persistence implemented: covered by existing Config + Debugger settings window bounds persistence and size contracts.
- Visual regression snapshots updated: this baseline specification and folder structure are now in-repo, with standardized capture set and comparison workflow.
