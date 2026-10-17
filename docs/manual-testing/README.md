# Manual Testing Hub

This section is the starting point for manual release validation in Nexen.

## Start Here

1. Read the ranked checklist in [Manual Step Priority Ranking](manual-step-priority-ranking.md).
2. Execute all release-blocking steps first.
3. Run the platform workflows below.
4. Record pass and fail evidence in the active session log.

## Platform Workflows

| Document | Purpose |
|---|---|
| [Manual Step Priority Ranking](manual-step-priority-ranking.md) | Rank each manual test by release impact and failure severity |
| [Main Menu Globe Language and Theme Manual Test](main-menu-language-globe-theme-manual-test.md) | Validate right-side globe language switching and brown/orange menu-theme defaults |
| [Theme Profile Chrome Customization Manual Test](theme-profile-chrome-customization-manual-test.md) | Validate profile-driven menu/chrome/accent customization and import/export roundtrip |
| [Live Language Switch Manual Test](live-language-switch-manual-test.md) | Validate no-restart language switching across menus, debugger, and tool windows |
| [Startup UX, Theme, and Accessibility Manual Test](startup-ux-accessibility-theme-manual-test.md) | Validate splash branding/progress visibility, larger startup defaults, and touch-friendly sizing |
| [Atari 2600 Debug and TAS Manual Test](atari2600-debug-and-tas-manual-test.md) | Atari 2600 end-to-end debugger UI, TAS UI, and movie workflows |
| [Atari Lynx Debug and TAS Manual Test](lynx-debug-and-tas-manual-test.md) | Atari Lynx end-to-end ROM loading, debugger, TAS, save states, and audio |
| [Channel F Debug and TAS Manual Test](channelf-debug-and-tas-manual-test.md) | Fairchild Channel F end-to-end debugger, TAS, movie, and persistence workflows |
| [Channel F Controller Profile Presets](channelf-controller-profile-presets.md) | Baseline and accessibility-oriented mapping profiles for Channel F controls |

## Full Testing Tree

For the full developer-facing manual and automated testing catalog, use:

- [Developer Testing and Manual Validation Index](../../~docs/testing/README.md)

## Release Rule

Do not mark a platform release-ready unless every Critical step and every High step passes.
