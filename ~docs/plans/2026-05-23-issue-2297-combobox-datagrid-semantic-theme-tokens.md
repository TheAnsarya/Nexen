# Issue #2297 ComboBox Dropdown and DataGrid Semantic Theme Tokens (2026-05-23)

## Scope

Extend centralized theme profiles with semantic tokens for ComboBox dropdown and DataGrid presentation states using verified stable resource keys.

## Resource-Key Targets

- ComboBox dropdown:
	- ComboBoxDropDownBackground
	- ComboBoxDropDownBorderBrush
- DataGrid semantic brushes:
	- DataGridColumnHeaderBackgroundBrush
	- DataGridColumnHeaderForegroundBrush
	- DataGridRowSelectedForegroundBrush

## Implementation Plan

1. Add token fields/defaults/validation in ThemeProfile and ThemeProfileFile.
2. Propagate tokens through PreferencesConfig lifecycle:
	- save-current capture (brush extraction for DataGrid brushes)
	- upsert/duplicate/reset/preset
	- divergence/customized token reporting
3. Apply runtime resource overrides in NexenThemeManager.
4. Ensure Dark/Light style dictionaries define fallback keys for ComboBox dropdown colors.
5. Add Preferences view model properties/preview brushes and mutation method.
6. Add Settings UI picker rows and code-behind handlers.
7. Add localization entries (en/es/ja).
8. Update focused tests and docs/manual-test coverage.

## Acceptance Criteria

- Theme profile import/export includes and validates all new semantic tokens.
- Settings can edit all new tokens with live preview chips.
- Runtime profile activation updates ComboBox dropdown and DataGrid header/selected-row visuals.
- Focused tests pass and Release x64 build succeeds.
