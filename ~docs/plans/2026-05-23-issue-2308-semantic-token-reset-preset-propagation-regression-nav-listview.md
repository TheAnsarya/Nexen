# Issue #2308 Semantic Token Reset/Preset Propagation Regression for NavigationView/ListView (2026-05-23)

## Scope

Add regression tests to validate reset-to-default and preset-apply pipelines correctly propagate NavigationView/ListView semantic token values.

## Acceptance Criteria

1. Reset-to-default restores canonical dark defaults for NavigationView/ListView semantic tokens.
2. Applying the light preset propagates light-default NavigationView/ListView semantic tokens.
3. Focused tests and Release x64 build pass.
