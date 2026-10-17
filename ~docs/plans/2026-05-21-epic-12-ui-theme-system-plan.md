# Epic 12 UI Theme System Plan (2026-05-21)

## Scope

Create a centralized UI theme system for Nexen that provides reusable tokens for colors, fonts, and sizes, with dark and light support, and apply the default brown/orange brand styling to startup and setup surfaces.

## Linked Issues

- Epic: #2257
- Foundation: #2258
- Application pass: #2259
- Docs and migration guide: #2260

## Goals

- Eliminate repeated hardcoded startup/setup color literals from view files.
- Define a single token source for brown/orange/yellow/white/cream brand colors.
- Keep dark/light variants aligned with existing `RequestedThemeVariant` behavior.
- Provide contributor documentation for token usage and migration.

## Implementation Plan

1. Introduce a centralized theme token dictionary in UI styles.
2. Add brand tokens for dark and light variants:
	- Browns: base, dark, and light.
	- Oranges: base, dark, and light.
	- Supporting tokens: yellow, white, cream.
3. Add reusable semantic brushes for startup/setup surfaces:
	- window background, panel background, card background, border, title/accent text, warning text, action button states.
4. Wire the theme dictionary into application style loading.
5. Replace hardcoded literals in startup/setup UI with `DynamicResource` references.
6. Keep existing emulator/theme preference flow unchanged.
7. Build and run focused validation to ensure no regressions.

## Acceptance Criteria

- Startup and setup windows compile and render using centralized theme tokens.
- No duplicated brown/orange brand literals remain in the startup/setup view files.
- Theme token dictionary supports both dark and light variants.
- Documentation exists for extending and consuming theme tokens.

## Risk Notes

- Startup visuals are style-heavy and rely on custom templates; incorrect key names can cause missing resources.
- Light variant values must preserve readability and contrast.

## Validation

- Build UI project and full solution build target used by regular workflow.
- Run focused startup/setup smoke checks.
- Run existing focused Genesis test filters used in ongoing runtime work to confirm no unrelated regressions.
