# Startup UX, Theme, and Accessibility Manual Test

Use this checklist to validate the default startup UX refresh for splash branding, progress visibility, larger startup defaults, and touch-friendly UI sizing.

## Preconditions

- Use a fresh launch of [Nexen](../README.md) from a recent Release x64 build.
- Verify themes are available in settings (Dark and Light).
- Test on at least one 1080p display; optionally repeat on 4k.

## Splash Validation

1. Launch Nexen and observe the splash screen before main window appears.
2. Verify splash background is dark brown (not white).
3. Verify splash progress indicator is visible and active.
4. Verify splash status text is readable against the background.

## Main Window Startup Validation

1. After splash closes, verify main window opens at a larger default size.
2. Verify startup window is not cramped and core UI is readable without immediate resizing.
3. Verify maximize behavior still works as expected.

## Menu and Touch-Target Validation

1. Open top-level menus and verify menu text is larger and readable.
2. Verify menu rows are tall enough for touch/mouse targeting.
3. Open context menus and verify submenu item size/readability.
4. Navigate settings tabs and verify buttons and common controls have larger click targets.

## Language Discoverability Validation

1. Open Preferences and locate Display Language control.
2. Verify the language selector is easy to find and wide enough for labels.
3. Verify helper text clearly indicates language applies immediately and no restart is required.
4. Change language and confirm immediate UI text updates.

## Theme Color Validation

1. Verify startup/splash surfaces use brown/orange brand tones.
2. Verify progress bar and status text maintain clear contrast.
3. Switch between Dark/Light and ensure startup visual identity remains brand-consistent.

## Pass Criteria

- Splash no longer appears white.
- Startup progress indicator is visibly active.
- Main window default startup footprint is comfortably large.
- Menus/buttons/text are visibly larger and easier to interact with.
- Language change discoverability and no-restart behavior are clear.
- Theme colors align with brown/orange branding in startup surfaces.

## Failure Notes Template

Record each failure with:

- Build/commit hash
- Theme (Dark/Light)
- Resolution (e.g. 1920x1080)
- Exact screen/menu path
- Observed behavior
- Expected behavior
