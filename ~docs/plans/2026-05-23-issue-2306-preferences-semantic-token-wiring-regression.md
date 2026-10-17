# Issue #2306 Preferences Semantic Token Wiring Regression (2026-05-23)

## Scope

Add regression coverage that validates Preferences semantic token wiring for NavigationView/ListView across viewmodel state, XAML bindings, and picker handlers.

## Acceptance Criteria

1. Tests assert viewmodel mutation method and semantic properties exist.
2. Tests assert XAML contains NavigationView/ListView semantic bindings.
3. Tests assert code-behind contains corresponding picker handlers.
4. Focused tests and Release x64 build succeed.
