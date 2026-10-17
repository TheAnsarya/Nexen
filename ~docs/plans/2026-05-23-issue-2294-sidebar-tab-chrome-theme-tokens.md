# Issue #2294 Sidebar and Tab Chrome Theme Tokens

## Scope

Extend theme profile customization to cover sidebar and dock tab chrome tokens, including runtime application and settings picker controls.

## Acceptance Criteria

- Add profile fields for sidebar/tab chrome tokens:
	- sidebar border
	- dock tab strip background
	- dock tab hover background
	- dock tab active background
	- dock tab active border
- Wire runtime resource mapping in `NexenThemeManager`.
- Ensure dark/light style dictionaries define corresponding color resources.
- Add settings pickers and live preview swatches for each new token.
- Include import/export validation compatibility via `ThemeProfileFile.IsValid()`.
- Update focused tests and docs.

## Implementation Plan

1. Add token properties and defaults in `ThemeProfile`.
2. Propagate token fields through `PreferencesConfig` lifecycle methods:
	- save current
	- upsert/duplicate
	- reset defaults
	- apply preset
	- divergence/customized token reporting
3. Apply runtime overrides in `NexenThemeManager`.
4. Add style resource keys and consume them in `NexenStyles`/`DockStyles`.
5. Add view-model fields and picker handlers in preferences view.
6. Add localization strings (en/es/ja).
7. Update tests and docs.

## Validation Plan

- Build Release x64.
- Run focused tests:
	- `Nexen.Tests.Config.ThemeProfileTests`
	- `Nexen.Tests.UI.UiScrollabilityMarkupTests`

## Risks

- Dock style token overrides can affect multiple debugger/tool windows.
- Theme profile field expansion requires strict parity across default values and copy/reset/preset paths.
