# Epic 21 Visual Baselines

This folder stores screenshot baselines and candidate captures used by the Epic 21 visual comparison workflow.

## Structure

- `baseline/` - approved baseline captures
- `candidate/` - captures from the candidate build

## Required Capture Set for #1408

Capture these windows at default size and at minimum size:

- Setup Wizard (`UI/Windows/SetupWizardWindow.axaml`)
- Config Window (`UI/Windows/ConfigWindow.axaml`)
- Debugger Config Window (`UI/Debugger/Windows/DebuggerConfigWindow.axaml`)

Recommended filename pattern:

- `<window>-default.png`
- `<window>-minimum.png`

Examples:

- `setup-wizard-default.png`
- `setup-wizard-minimum.png`
- `config-window-default.png`
- `config-window-minimum.png`
- `debugger-config-default.png`
- `debugger-config-minimum.png`

## Compare Command

```powershell
.\~scripts\compare-epic21-ui-visual-baselines.ps1 `
	-BaselineDir "~docs/testing/visual-baselines/epic-21/baseline" `
	-CandidateDir "~docs/testing/visual-baselines/epic-21/candidate"
```

## Output Report

- `~docs/testing/data/epic-21-visual-regression-report.md`
