# Issue #2307 Semantic Token Divergence and Customization Regression for NavigationView/ListView (2026-05-23)

## Scope

Add regression coverage that validates `PreferencesConfig` divergence counting and customized-token reporting include NavigationView and ListView semantic token families.

## Acceptance Criteria

1. Tests prove divergence count increments when NavigationView/ListView semantic tokens differ from defaults.
2. Tests prove customized token names include affected NavigationView/ListView semantic token property names.
3. Focused tests and Release x64 build pass.
