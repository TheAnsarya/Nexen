# Epic 22 Startup UX and Accessibility Theme Refresh Plan

Linked issues:

- Epic: #2280
- Implementation slice: #2281

## Objectives

- Make splash startup visuals consistently dark-brown branded.
- Ensure startup loading progress is clearly visible.
- Increase default startup window ergonomics (larger baseline and maximize-friendly behavior).
- Increase menu/control/text sizing for touch-friendly use.
- Keep language switching highly discoverable and clearly no-restart.
- Document theme token flow and update workflow.

## Implementation Steps

1. Splash styling and progress visibility

- Update splash dimensions, logo area, and progress bar size in [UI/Windows/SplashWindow.axaml](UI/Windows/SplashWindow.axaml).
- Add lightweight animated progress value updates in [UI/Windows/SplashWindow.axaml.cs](UI/Windows/SplashWindow.axaml.cs).
- Align splash startup colors for light/dark dictionaries in [UI/Styles/NexenBrandTheme.xaml](UI/Styles/NexenBrandTheme.xaml).

2. Startup and accessibility sizing

- Increase default app and menu font tokens in [UI/App.axaml](UI/App.axaml).
- Increase menu row heights, submenu/context item sizes, and button hit targets in [UI/Styles/NexenStyles.xaml](UI/Styles/NexenStyles.xaml).
- Increase main window initial size and minimum constraints in [UI/Windows/MainWindow.axaml](UI/Windows/MainWindow.axaml).
- Increase persisted default main window size in [UI/Config/MainWindowConfig.cs](UI/Config/MainWindowConfig.cs).

3. Language discoverability

- Increase language selector width and emphasize immediate-apply text in [UI/Views/PreferencesConfigView.axaml](UI/Views/PreferencesConfigView.axaml).
- Clarify no-restart language hint text in localization files:
	- [UI/Localization/resources.en.xml](UI/Localization/resources.en.xml)
	- [UI/Localization/resources.es.xml](UI/Localization/resources.es.xml)
	- [UI/Localization/resources.ja.xml](UI/Localization/resources.ja.xml)

4. Theme documentation

- Expand theme usage/update guidance in [docs/THEME-CUSTOMIZATION.md](docs/THEME-CUSTOMIZATION.md) with token mapping and update steps.

## Validation Plan

- Build Release x64 via existing VS Code task.
- Run focused test subset:
	- Theme profile tests.
	- Localization refresh tests.
- Manual smoke:
	- Launch Nexen and verify splash background/progress visibility.
	- Validate larger menu/control hit targets.
	- Validate language selector discoverability and immediate switch behavior.

## Completion Criteria

- Splash screen no longer appears white in startup workflow.
- Progress bar is clearly visible during startup.
- Main window defaults and menu/control sizes are noticeably larger and touch-friendlier.
- Theme update documentation is explicit and current.
- Build/tests pass and implementation issue is closed with commit references.
