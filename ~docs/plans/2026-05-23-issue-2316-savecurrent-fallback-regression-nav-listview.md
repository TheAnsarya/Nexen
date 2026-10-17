# Issue #2316 Plan - SaveCurrent Fallback Regression for NavigationView/ListView

## Scope

Add regression coverage for the no-Application-resources path in `PreferencesConfig.SaveCurrentToProfile` to ensure NavigationView/ListView semantic token values are preserved when resource lookups are unavailable.

## Acceptance Criteria

- Add a failing-then-passing test that exercises `SaveCurrentToProfile` with no `Application.Current.Resources` path.
- Verify profile-level NavigationView/ListView semantic tokens are preserved (not overwritten) in fallback mode.
- Verify theme and font fields still sync into the profile during the same save operation.
- Include the test in the standard `ThemeProfileTests` suite.
