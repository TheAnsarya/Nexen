# Issue 2291 Control Semantic Theme Tokens (2026-05-23)

## Context

Issue [#2291](https://github.com/TheAnsarya/Nexen/issues/2291) continues epic [#2289](https://github.com/TheAnsarya/Nexen/issues/2289) by centralizing remaining control-interaction chrome tokens used by button/combo/repeat families.

## Scope

- Add semantic control token fields to `ThemeProfile`.
- Apply those token values to button/combo/repeat pointer and pressed resource keys at runtime.
- Expose key semantic control token pickers in Preferences theme settings.
- Preserve import/export compatibility and validation.
- Update tests and docs.

## Semantic Tokens

- `ControlPointerOverBackgroundColor`
- `ControlPressedBackgroundColor`
- `ControlPointerOverBorderColor`
- `ControlPressedBorderColor`

## Implementation Plan

1. Extend `ThemeProfile` defaults and `ThemeProfileFile.IsValid` color checks.
2. Propagate new fields through `PreferencesConfig` profile lifecycle operations.
3. Update `NexenThemeManager` to override these keys:
	- `ButtonBackgroundPointerOver`, `ButtonBackgroundPressed`
	- `ButtonBorderBrushPointerOver`, `ButtonBorderBrushPressed`
	- `ComboBoxBackgroundPointerOver`, `ComboBoxBorderBrushPointerOver`
	- `RepeatButtonBackgroundPointerOver`, `RepeatButtonBackgroundPressed`
	- `RepeatButtonBorderBrushPointerOver`, `RepeatButtonBorderBrushPressed`
4. Add Preferences UI pickers and previews for the new semantic tokens.
5. Add localization labels (en/es/ja).
6. Update tests and user docs.

## Acceptance Criteria

- Active profile updates control hover/pressed semantics consistently across button/combo/repeat families.
- New token values are import/export roundtrip-safe.
- Invalid imported color values for new fields are rejected.
- Preferences includes picker controls for all new semantic tokens.
- Build and focused tests pass.

## Validation Plan

- Build Release x64.
- Run focused tests:
	- `Nexen.Tests.Config.ThemeProfileTests`
	- `Nexen.Tests.UI.UiScrollabilityMarkupTests`
- Execute manual checklist in `docs/manual-testing/theme-profile-chrome-customization-manual-test.md`.
