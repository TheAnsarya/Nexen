# Issue 2290 UI Theme Chrome Token Centralization (2026-05-22)

## Context

Issue [#2290](https://github.com/TheAnsarya/Nexen/issues/2290) extends the existing theme profile system so centralized palette tokens also drive persistent UI chrome (menus/settings/accent), not only startup/setup surfaces.

Parent epic: [#2289](https://github.com/TheAnsarya/Nexen/issues/2289).

## Scope

- Extend `ThemeProfile` with UI chrome color tokens.
- Apply those tokens at runtime through `NexenThemeManager` resource overrides.
- Expose key menu/accent token pickers in Preferences theme settings.
- Preserve profile import/export compatibility and validation.
- Add/update tests and docs.

## Token Coverage

- Menu background
- Menu highlight
- Menu border highlight
- Menu separator background
- Settings tab strip background
- Accent color
- Highlight color

## Implementation Steps

1. Add token properties and canonical light/dark defaults in `ThemeProfile`.
2. Update `ThemeProfileFile.IsValid()` to validate the new colors.
3. Update profile lifecycle methods in `PreferencesConfig`:
	- upsert, duplicate, reset-to-defaults, preset apply, divergence count, customized-token listing.
4. Update `NexenThemeManager.ApplyTheme(...)` to override color resources for menu/chrome/accent and derive accent variants.
5. Add Preferences UI controls for menu background/highlight/accent color pickers.
6. Wire view-model and view event handlers for new picker actions.
7. Update localization strings for en/es/ja.
8. Update docs and add manual validation checklist.

## Acceptance Criteria

- Active theme profile can change menu/chrome/accent colors at runtime.
- New theme profile fields serialize and deserialize through import/export files.
- Invalid imported colors for new fields are rejected by validation.
- Preferences Settings includes picker-based customization for key chrome tokens.
- Tests and docs are updated and passing.

## Validation Plan

- Build `Release x64`.
- Run focused tests:
	- `Tests/Config/ThemeProfileTests.cs`
	- `Tests/UI/UiScrollabilityMarkupTests.cs`
	- localization/runtime refresh subset used in recent language/theme work.
- Manual pass:
	- import/export a profile,
	- apply token edits in Settings,
	- verify menu and settings strip update.
