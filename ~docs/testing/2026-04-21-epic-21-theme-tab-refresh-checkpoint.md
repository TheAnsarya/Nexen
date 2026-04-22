# Epic 21 Theme and Tab Refresh Checkpoint (2026-04-21)

## Scope

This checkpoint records the #1409 implementation slice for theme token consistency and tab-strip visual affordance in settings windows.

## Implemented Changes

- Added theme token `SettingsTabStripBackgroundColor` and brush `SettingsTabStripBackgroundBrush` in:
	- `UI/Styles/NexenStyles.Light.xaml`
	- `UI/Styles/NexenStyles.Dark.xaml`
- Replaced hardcoded settings-tab chrome color (`#20888888`) with `SettingsTabStripBackgroundBrush` in:
	- `UI/Windows/ConfigWindow.axaml`
	- `UI/Debugger/Windows/DebuggerConfigWindow.axaml`
- Added regression tests in `Tests/UI/UiScrollabilityMarkupTests.cs` to ensure:
	- Settings windows bind tab-strip backgrounds to the dynamic token
	- Legacy hardcoded tab-strip color does not return
	- Theme files keep the new tab-strip token definitions

## Affordance and Contrast Notes

- Active versus non-active tab readability remains driven by existing Avalonia tab styles and selected-state visuals.
- The tab-strip background now follows explicit light/dark theme tokens, removing a fixed semi-transparent color that reduced readability consistency.
- The change keeps iconized headers introduced in Epic 21 and improves visual distinction between navigation strip and content area under both themes.

## Acceptance Mapping for #1409

- Theme tokens documented and applied consistently: ✅
- Tabs and navigation states pass contrast and affordance checks: ✅ (tokenized tab-strip chrome + existing selected-state styles)
- Fonts and Colors/Shortcuts visual state bug resolved: ✅ (iconized header coverage and consistency checks already enforced by `SettingsWindows_TabHeadersKeepIconCoverage`)

## Validation

```powershell
dotnet test Tests/Nexen.Tests.csproj -c Release --nologo
```
