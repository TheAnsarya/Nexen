# Issue #2300 Theme Customization Semantic Grouping (2026-05-23)

## Scope

Reorganize the Preferences theme customization picker surface into semantic component groups while preserving all existing token bindings and click handlers.

## Group Targets

- Startup
- Chrome and Accent
- Control States
- Inputs and Flyouts
- Combo and DataGrid
- List and Tree

## Acceptance Criteria

1. Theme picker rows are split into grouped sections with clear headers.
2. Existing token bindings and picker handlers remain intact.
3. Localization includes section header labels (en/es/ja).
4. UI markup tests cover group header presence and existing token wiring.
5. Focused tests pass and Release x64 build succeeds.
