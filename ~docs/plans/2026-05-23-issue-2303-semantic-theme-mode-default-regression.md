# Issue #2303 Semantic Theme-Mode Default Regression (2026-05-23)

## Scope

Add regression coverage that validates semantic token family defaults remain valid and theme-mode-specific across dark and light presets.

## Semantic Families

- ComboBox dropdown
- DataGrid header/selected-row
- ListBox hover/selected
- TreeView hover/selected
- NavigationView hover/selected
- ListView hover/selected

## Acceptance Criteria

1. A regression test validates all semantic token colors are syntactically valid (`#argb`/`#rrggbb` forms accepted by `ThemeProfile.IsValidColor`).
2. Dark and light defaults differ for representative semantic family anchors.
3. Test suite passes with no behavioral regressions.
