# Issue #2296 Text-Input and Tooltip/Flyout Semantic Theme Tokens (2026-05-23)

## Scope

Extend centralized theme profiles with semantic tokens for text input selection/disabled states and tooltip/menu-flyout chrome where stable keys are present.

## Resource-Key Targets

- Text selection:
	- TextBoxSelectionBrush
- Disabled text-control background:
	- TextControlBackgroundDisabled
- Tooltip background:
	- ToolTipBackground
- Menu flyout chrome:
	- MenuFlyoutPresenterBackground
	- MenuFlyoutPresenterBorderBrush

## Implementation Plan

1. Add token fields/defaults/validation in ThemeProfile and ThemeProfileFile.
2. Propagate tokens through PreferencesConfig lifecycle:
	- save-current capture (including brush-resource extraction for TextBoxSelectionBrush)
	- upsert/duplicate/reset/preset
	- divergence/customized token reporting
3. Apply runtime resource overrides in NexenThemeManager.
4. Ensure Dark/Light style dictionaries include fallback keys for targeted resources.
5. Add Preferences view model properties/preview brushes and mutation method.
6. Add Settings UI picker rows and code-behind handlers.
7. Add localization entries (en/es/ja).
8. Update focused tests and docs/manual test coverage.

## Acceptance Criteria

- Theme profile import/export includes and validates all new semantic tokens.
- Settings can edit all new tokens with live previews.
- Runtime theme application updates text selection, disabled text background, tooltip background, and menu flyout background/border.
- Focused tests pass and Release x64 build succeeds.
