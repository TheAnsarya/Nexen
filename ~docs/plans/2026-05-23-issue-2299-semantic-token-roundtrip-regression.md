# Issue #2299 Semantic Token Roundtrip Regression (2026-05-23)

## Scope

Add a focused regression test to ensure semantic theme tokens persist exactly through profile export/import and active-profile save cycle.

## Coverage Targets

- Combo/DataGrid semantic tokens from #2297:
	- ComboBoxDropDownBackgroundColor
	- ComboBoxDropDownBorderColor
	- DataGridHeaderBackgroundColor
	- DataGridHeaderForegroundColor
	- DataGridSelectedRowForegroundColor
- Extended semantic tokens now covered:
	- ListBoxItemHoverBackgroundColor
	- ListBoxItemSelectedBackgroundColor
	- ListBoxItemSelectedForegroundColor
	- TreeViewItemHoverBackgroundColor
	- TreeViewItemSelectedBackgroundColor
	- TreeViewItemSelectedForegroundColor

## Acceptance Criteria

1. A test serializes ThemeProfileFile to JSON and deserializes it back.
2. Imported profile is upserted and SaveCurrentToProfile is invoked.
3. All targeted semantic token fields remain exactly unchanged.
4. Focused tests pass and Release x64 build succeeds.
