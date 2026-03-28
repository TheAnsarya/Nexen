# UI Settings Completeness Plan (Epic #1040)

## Scope

Complete and validate UI settings surfaces for all supported systems, with immediate focus on Atari 2600, Lynx, and Channel F, then expand to cross-system parity for input, movie/TAS, and savestate UX.

## Linked Issues

1. Epic: #1040
2. Survey matrix: #1041
3. Channel F UX/localization pass: #1042
4. Atari 2600 + Lynx UX parity: #1043
5. Global Input/Movie/Savestate UX: #1044
6. Regression tests: #1045
7. Responsiveness benchmarks: #1046

## Immediate Deliverables (Session)

1. Resolve startup SVG loading failures and remove duplicate package warnings.
2. Add missing localization/form resources for Channel F and Lynx config views.
3. Add descriptive helper text for Atari 2600, Lynx, and Channel F settings pages.
4. Correct obvious binding typo in controller configuration UI.
5. Build and validate Release x64.

## Phase Plan

### Phase A: Coverage and UX Baseline

1. Produce per-platform matrix of:
- Tab coverage (General, Audio, Emulation, Input, Video)
- Text quality (labels, helper text, tooltips)
- Functional integrity (bindings, apply behavior, setup dialogs)

2. Identify placeholder/missing localization:
- Missing Form ID mappings in resources.en.xml
- Missing enum label values in localization enum blocks

### Phase B: Functional Integrity

1. Validate Config.Clone and apply behavior per platform viewmodel.
2. Validate command enablement and setup button interactions.
3. Validate platform-specific input combos and enum text rendering.

### Phase C: Input/Movie/Savestate Cohesion

1. Review global Input config for discoverability and platform-specific context.
2. Review Movie record/playback dialogs for validation clarity and metadata flow.
3. Review Greenzone/savestate dialogs for consistency and localization debt.

### Phase D: Tests and Benchmarks

1. Add targeted tests in UI-facing viewmodel/config layers.
2. Add startup/settings navigation timing checkpoints.
3. Track benchmark deltas and preserve correctness first.

## Test and Validation Commands

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe" Nexen.sln /p:Configuration=Release /p:Platform=x64 /t:Build /m /nologo /v:m
```

```powershell
.\bin\win-x64\Release\Core.Tests.exe --gtest_filter=NotificationManagerTests.* --gtest_brief=1
```

```powershell
dotnet test --no-build -c Release
```

## Risk Notes

1. Missing localization mappings degrade usability and create placeholder text artifacts.
2. Inconsistent platform-tab UX increases regression risk and user confusion.
3. UX improvements must not alter emulator correctness paths.

## Done Criteria

1. All targeted platform settings pages show complete descriptive text with no placeholder tokens.
2. Input/movie/savestate workflows are coherent and validated.
3. Build and tests pass.
4. Follow-up issues are either completed or explicitly scoped with acceptance criteria.
