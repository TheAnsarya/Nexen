# Issue #2305 NexenThemeManager Semantic Override Mapping Regression (2026-05-23)

## Scope

Add automated regression checks for semantic token override mappings in `NexenThemeManager` across current semantic families.

## Semantic Families

- ComboBox dropdown
- DataGrid header/selected-row
- ListBox
- TreeView
- NavigationView
- ListView

## Acceptance Criteria

1. Tests assert expected `ApplyColorOverride` and `ApplyBrushOverride` semantic mappings exist.
2. Regression fails if a semantic family mapping is removed or renamed unintentionally.
3. Focused tests and Release x64 build succeed.
