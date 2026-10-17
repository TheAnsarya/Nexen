# Main Menu Globe Language and Theme Manual Test

## Purpose

Validate the right-aligned globe language selector and brown/orange default menu-theme updates.

## Preconditions

- Build uses latest local `master` branch.
- Nexen launches successfully.
- A ROM can be loaded to keep full menus active.

## Test Steps

1. Launch Nexen and confirm a globe-plus-text language trigger appears on the right side of the top menu bar.
2. Open the globe menu with mouse.
3. Confirm language entries are populated from bundled localization resources.
4. Confirm current bundle entries include English, Spanish, and Japanese using localized labels for the active UI language.
5. Select Spanish.
6. Verify top-level menu labels update immediately without restart.
7. Open Settings and verify labels are translated there too.
8. Select Japanese from the globe menu.
9. Verify menu and settings labels update immediately.
10. Select English and verify labels return to English.
11. Close Nexen.
12. Reopen Nexen and verify the last selected language persisted.
13. In both light and dark theme profiles, visually verify menu/background/highlight accents are brown/orange-forward and readable.
14. Hover menu and button surfaces to confirm pointer-over accents use orange highlight tones.

## Pass Criteria

- Globe menu is visible, accessible, and right aligned.
- Globe trigger remains understandable as text+icon even when emoji rendering differs.
- Language changes apply immediately and persist across restart.
- Menu/theme accents visibly reflect brown/orange branding in both light and dark themes.
- Text and focus/hover states remain readable.

## Evidence to Capture

- Screenshot of globe menu expanded.
- Screenshot of Spanish or Japanese main menu labels.
- Screenshot of light theme menu hover state.
- Screenshot of dark theme menu hover state.
