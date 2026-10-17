# Issue #2298 ListBox and TreeView Semantic Theme Tokens (2026-05-23)

## Scope

Extend centralized theme profiles with semantic tokens for ListBox and TreeView hover and selected states.

## Resource-Key Targets

- ListBox:
	- ListBoxItemBackgroundPointerOver
	- ListBoxItemBackgroundSelected
	- ListBoxItemForegroundSelected
- TreeView:
	- TreeViewItemBackgroundPointerOver
	- TreeViewItemBackgroundSelected
	- TreeViewItemForegroundSelected

## Implementation Plan

1. Add token fields/defaults/validation in ThemeProfile and ThemeProfileFile.
2. Propagate through PreferencesConfig save/upsert/duplicate/reset/preset/divergence/customized token reporting.
3. Add runtime override mapping in NexenThemeManager.
4. Define fallback resource defaults in Light/Dark dictionaries.
5. Add ViewModel state, pickers, and preview chips in Preferences UI.
6. Add localization entries (en/es/ja).
7. Update tests and docs/manual testing.

## Acceptance Criteria

- Import/export includes and validates all ListBox/TreeView semantic tokens.
- Preferences UI allows editing all new tokens with live preview chips.
- Active profile applies hover/selected visuals for ListBox and TreeView.
- Focused tests pass and Release x64 build succeeds.
