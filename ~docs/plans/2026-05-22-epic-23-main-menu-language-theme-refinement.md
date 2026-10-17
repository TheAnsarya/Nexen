# Epic 23 Plan: Main Menu Language Switcher and Brown/Orange Theme Refinement

## Linked Issues

- Epic: #2284
- Implementation: #2285

## Scope

- Add a right-aligned globe language selector to the main menu bar.
- Make language switching immediate and persistent from this top-level control.
- Refine default menu and control chrome tokens to a clearer brown/orange brand direction.
- Add regression tests and update user/developer documentation.

## Implementation Notes

1. Main menu language entry point:
	- Add a right-side menu entry with globe icon and language items.
	- Keep existing menu model intact; implement language handlers in main menu view code-behind.
2. Runtime behavior:
	- Update Preferences UiLanguage.
	- Reload localized resources immediately.
	- Save configuration immediately for persistence.
3. Theme token update:
	- Update light and dark menu/chrome token defaults in style dictionaries.
	- Keep contrast/readability acceptable while emphasizing brown/orange accents.
4. Validation:
	- Add markup assertions for globe-menu presence and token values.
	- Build Release x64 and run focused UI/localization tests.

## Acceptance Criteria

- Globe icon appears at the right edge of the main menu bar.
- Language can be switched from the globe menu without restart.
- Selected language persists after app restart.
- Default menu/chrome theme colors are brown/orange-forward in light and dark themes.
- Build and focused tests pass.

## Risks and Mitigations

- Risk: hardcoded theme assertions can become brittle.
	- Mitigation: keep assertions on a small set of intentional defaults only.
- Risk: language check-mark state drifts after changes from other windows.
	- Mitigation: refresh check states on LanguageChanged event.

## Validation Plan

- Build: Release x64 full solution build.
- Tests: focused markup/localization tests in Nexen.Tests.
- Manual: verify globe menu keyboard/mouse access and cross-window immediate translation.

## Follow-Ups

- Add country/language glyph alternatives for platforms where emoji rendering is poor.
- Consider exposing the same language selector in startup flow for first-run onboarding.
