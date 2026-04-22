# Epic 21 UI Test Strategy Matrix (2026-04-21)

## Scope

This document tracks #1415 and defines functional plus visual regression coverage for the Epic 21 UX surfaces:

- Onboarding
- Settings IA and navigation
- Themes and visual mode behavior
- Input routing and active-system landing

## Functional Matrix

| Critical area | Baseline automated path | Owner task status |
|---|---|---|
| Onboarding | `Tests/ViewModels/SetupWizardViewModelTests.cs`, `Tests/Config/SetupWizardMetricsStoreTests.cs`, `Tests/UI/UiScrollabilityMarkupTests.cs` | ✅ Implemented |
| Settings IA and navigation | `Tests/ViewModels/ConfigViewModelTests.cs`, `Tests/UI/UiScrollabilityMarkupTests.cs` | ✅ Implemented |
| Themes | `Tests/UI/UiScrollabilityMarkupTests.cs` (`PreferencesView_ContainsThemeSelector`) | ✅ Implemented |
| Input routing | `Tests/ViewModels/ConfigViewModelTests.cs` (`ResolveInputLandingTab_UsesExpectedConfigTab`) | ✅ Implemented |

## High-Traffic Settings Smoke Commands

```powershell
dotnet test Tests/Nexen.Tests.csproj -c Release --filter "FullyQualifiedName~ConfigViewModelTests|FullyQualifiedName~SetupWizardViewModelTests|FullyQualifiedName~SetupWizardMetricsStoreTests|FullyQualifiedName~UiScrollabilityMarkupTests" --nologo
```

## Visual Regression Workflow (Runnable)

### Step 1: Capture baseline screenshots

Use the existing screenshot hotkey flow (F12) while running the same scripted navigation path for each Epic 21 area, then copy images to a baseline folder such as:

- `~docs/testing/visual-baselines/epic-21/baseline`

### Step 2: Capture candidate screenshots

Capture the same navigation path on the candidate build and copy images to:

- `~docs/testing/visual-baselines/epic-21/candidate`

### Step 3: Compare screenshots with the automation script

```powershell
.\~scripts\compare-epic21-ui-visual-baselines.ps1 `
	-BaselineDir "~docs/testing/visual-baselines/epic-21/baseline" `
	-CandidateDir "~docs/testing/visual-baselines/epic-21/candidate"
```

Output report:

- `~docs/testing/data/epic-21-visual-regression-report.md`

The script exits non-zero on detected differences unless `-AllowDifferences` is specified.

## Task List

- ✅ Define critical Epic 21 areas and map each to at least one automated path.
- ✅ Add smoke command for high-traffic settings paths.
- ✅ Add runnable visual comparison automation script.
- ✅ Document baseline/candidate capture and report workflow.
- 🔄 Add CI execution for visual comparison once stable screenshot fixtures are checked in.
