# Issue #2317 Plan - ThemeProfile Validation Regression for NavigationView/ListView Semantic Colors

## Scope

Add regression coverage to ensure `ThemeProfileFile.IsValid` rejects invalid color values in NavigationView/ListView semantic token fields.

## Acceptance Criteria

- Add focused tests that inject invalid color literals into NavigationView semantic token properties.
- Add focused tests that inject invalid color literals into ListView semantic token properties.
- Confirm `ThemeProfileFile.IsValid()` returns `false` for both cases.
- Keep existing valid-profile behavior unchanged.
