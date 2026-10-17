# Issue #2295 Checkbox/Radio/Slider Semantic Theme Tokens (2026-05-23)

## Scope

Extend centralized theme profiles with semantic tokens for checkbox, radio button, and slider interaction states where stable resource keys are available.

## Resource-Key Targets

- Checkbox hover border:
	- CheckBoxCheckBackgroundStrokeUncheckedPointerOver
	- CheckBoxCheckBackgroundStrokeCheckedPointerOver
	- CheckBoxBorderBrushCheckedPointerOver
- Checkbox pressed background:
	- CheckBoxCheckBackgroundFillCheckedPressed
	- CheckBoxCheckBackgroundFillUncheckedPressed
	- CheckBoxBackgroundCheckedPressed
- Checkbox pressed border:
	- CheckBoxCheckBackgroundStrokeCheckedPressed
	- CheckBoxCheckBackgroundStrokeUncheckedPressed
	- CheckBoxBorderBrushCheckedPressed
- Radio pointer-over and pressed states:
	- RadioButtonOuterEllipseStrokePointerOver
	- RadioButtonOuterEllipseFillPressed
	- RadioButtonOuterEllipseStrokePressed
- Slider pointer-over and pressed track:
	- SliderTrackFillPointerOver
	- SliderTrackFillPressed

## Implementation Plan

1. Add token fields/defaults/validation in ThemeProfile and ThemeProfileFile.
2. Propagate new tokens through PreferencesConfig lifecycle methods:
	- save-current capture
	- upsert/duplicate/reset/preset
	- divergence/customized token reporting
3. Apply runtime resource overrides in NexenThemeManager.
4. Ensure Light and Dark style dictionaries define fallback defaults for added keys.
5. Add Preferences view model properties/preview brushes and mutation method.
6. Add Settings UI picker rows and code-behind handlers.
7. Add localization strings (en/es/ja).
8. Update focused tests and docs/manual-test instructions.

## Acceptance Criteria

- Theme profile import/export contains new semantic tokens and validates them.
- Settings can edit all new tokens and show live preview chips.
- Runtime theme application updates targeted checkbox/radio/slider states.
- Focused theme/profile/UI markup tests pass.
- Release x64 build succeeds.
