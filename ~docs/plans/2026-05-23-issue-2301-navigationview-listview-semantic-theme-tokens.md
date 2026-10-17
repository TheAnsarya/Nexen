# Issue #2301 NavigationView and ListView Semantic Theme Tokens (2026-05-23)

## Scope

Add dedicated semantic theme tokens for NavigationView and ListView hover/selected states, and wire them through profile defaults, runtime overrides, settings customization, localization, and regression tests.

## Token Targets

- NavigationView hover background
- NavigationView selected background
- NavigationView selected foreground
- ListView hover background
- ListView selected background
- ListView selected foreground

## Acceptance Criteria

1. `ThemeProfile` contains new NavigationView/ListView semantic token fields with dark/light defaults.
2. Runtime theme application (`NexenThemeManager`) applies new tokens to resource keys.
3. Preferences config lifecycle (save/import/export/reset/preset/divergence/customized) includes new tokens.
4. Theme customization UI exposes new pickers and preview chips with localized labels.
5. Focused tests pass and Release x64 build succeeds.
