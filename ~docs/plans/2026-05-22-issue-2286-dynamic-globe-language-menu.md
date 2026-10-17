# Issue 2286 Plan: Dynamic Globe Language Menu

## Linked Issues

- Parent Epic: #2284
- Implementation: #2286

## Goal

Replace hardcoded globe-language submenu items with a dynamic list generated from bundled localization resources.

## Scope

- Keep globe trigger in the right side of the main menu bar.
- Build language submenu entries from `ResourceHelper.GetAvailableLanguageCodes()`.
- Map language codes to `UiLanguage` for immediate apply/persistence.
- Keep current selected language check-mark synced with runtime language changes.
- Update tests and docs.

## Acceptance Criteria

- Globe menu lists all bundled languages without hardcoded item declarations.
- Selected language is check-marked.
- Switching language updates UI immediately and persists.
- Release build and focused tests pass.

## Validation

- Build Release x64.
- Focused test filter:
	- `Nexen.Tests.UI.UiScrollabilityMarkupTests`
	- `Nexen.Tests.Localization.ResourceHelperTests`
	- `Nexen.Tests.Localization.StartupLanguageResolverTests`
	- `Nexen.Tests.Localization.LocalizationRuntimeRefreshTests`
