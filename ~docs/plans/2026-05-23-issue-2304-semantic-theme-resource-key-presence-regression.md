# Issue #2304 Semantic Theme Resource Key Presence Regression (2026-05-23)

## Scope

Add automated regression coverage that asserts semantic list/tree/navigation/listview color keys exist in both light and dark theme dictionaries.

## Acceptance Criteria

1. Tests validate required semantic keys in `NexenStyles.Light.xaml` and `NexenStyles.Dark.xaml`.
2. Regression fails if any required semantic key is removed from either theme.
3. Focused tests and Release x64 build succeed.
