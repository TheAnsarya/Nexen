# Theme Customization Guide

This guide explains where Nexen theme values are defined, how they are applied at runtime, and how to update them safely.

## Theme Sources

Nexen theme values come from two layers:

1. Base resource dictionaries:
	- UI/Styles/NexenBrandTheme.xaml
	- UI/App.axaml
	- UI/Styles/NexenStyles.xaml
2. User-overridable theme profile values:
	- UI/Config/ThemeProfile.cs
	- UI/Config/PreferencesConfig.cs

## Runtime Theme Flow

Theme resources are applied in this order:

1. Avalonia loads base resources from UI/App.axaml and style includes.
2. UI/Styles/NexenBrandTheme.xaml provides Dark/Light brand dictionaries.
3. UI/Config/PreferencesConfig.cs resolves the active theme profile.
4. UI/Utilities/NexenThemeManager.cs writes profile overrides into Application resources.

This keeps static defaults in XAML while allowing user profile overrides at runtime.

## Key Resource Tokens

Primary startup and splash tokens are in UI/Styles/NexenBrandTheme.xaml:

- NexenStartupWindowBackgroundBrush
- NexenStartupSplashBackgroundBrush
- NexenStartupSplashStatusBrush
- NexenStartupProgressForegroundBrush
- NexenStartupProgressBackgroundBrush

Global typography defaults are in UI/App.axaml:

- NexenFont / NexenFontSize
- NexenMenuFont / NexenMenuFontSize

Touch sizing defaults are in UI/Styles/NexenStyles.xaml through control styles for Button, TextBox, ComboBox, and CheckBox.

Main menu chrome tokens are in UI/Styles/NexenStyles.Light.xaml and UI/Styles/NexenStyles.Dark.xaml:

- NexenMenuBackground
- NexenMenuBackgroundHighlight
- NexenMenuBorderHighlight
- NexenMenuSeparatorBackground
- NexenMenuMarginStartColor
- NexenMenuMarginEndColor

## Theme Value Map (Where Values Are Used)

Use this quick map when changing the default look:

| Surface | Primary file | Key tokens |
|---|---|---|
| Splash startup card/background | UI/Styles/NexenBrandTheme.xaml | NexenStartupWindowBackgroundBrush, NexenStartupSplashBackgroundBrush, NexenStartupSplashBorderBrush |
| Splash loading indicator | UI/Styles/NexenBrandTheme.xaml + UI/Windows/SplashWindow.axaml | NexenStartupProgressForegroundBrush, NexenStartupProgressBackgroundBrush |
| Global app text | UI/App.axaml | NexenFont, NexenFontSize |
| Global menu typography | UI/App.axaml | NexenMenuFont, NexenMenuFontSize |
| Main menu chrome colors | UI/Styles/NexenStyles.Light.xaml + UI/Styles/NexenStyles.Dark.xaml | NexenMenuBackground, NexenMenuBackgroundHighlight, NexenMenuBorderHighlight, NexenMenuMarginStartColor, NexenMenuMarginEndColor |
| Menu geometry and hit targets | UI/Styles/NexenStyles.xaml | Menu, MenuItem, ContextMenu style selectors |
| Button and input hit targets | UI/Styles/NexenStyles.xaml | Button/TextBox/ComboBox style selectors and MinHeight/Padding setters |
| Runtime profile overrides | UI/Utilities/NexenThemeManager.cs + UI/Config/ThemeProfile.cs | profile color and font tokens |

## How To Update Theme Values

1. Update base defaults in UI/Styles/NexenBrandTheme.xaml.
2. Keep both `Dark` and `Light` dictionaries updated for parity.
3. If changing fonts or menu readability, update UI/App.axaml tokens first.
4. If changing control touch targets, update sizes in UI/Styles/NexenStyles.xaml.
5. Launch Nexen and confirm splash, menus, and settings readability in both themes.

## How To Update Startup Splash Look

Startup visuals are split between token values and layout:

- Colors and brushes: UI/Styles/NexenBrandTheme.xaml
- Layout and control sizing: UI/Windows/SplashWindow.axaml
- Runtime status/progress behavior: UI/Windows/SplashWindow.axaml.cs

For visibility issues, always check all three files together.

## Theme Profile Import and Export

Theme picker layout is organized by semantic component groups for faster editing:

- Startup
- Chrome and Accent
- Control States
- Inputs and Flyouts
- Combo and DataGrid
- List and Tree

Theme profile import/export uses JSON-backed profile models:

- Schema and validation:
	- UI/Config/ThemeProfile.cs
- Persistence and profile lifecycle:
	- UI/Config/PreferencesConfig.cs
- Settings UI actions:
	- UI/ViewModels/PreferencesConfigViewModel.cs
	- UI/Views/PreferencesConfigView.axaml
	- UI/Views/PreferencesConfigView.axaml.cs

When importing, values are validated and then applied through the same profile pipeline used for in-app edits.

Theme profile import/export now covers startup/setup tokens and key UI chrome tokens used by the always-visible shell:

- Menu background/highlight/border/separator
- Settings tab strip background
- Accent and highlight colors

Theme profiles also cover semantic control interaction tokens used across button/combo/repeat families:

- Control hover background
- Control pressed background
- Control hover border
- Control pressed border

Theme profiles also cover sidebar/tab chrome tokens for settings and docked tool tabs:

- Settings sidebar tab strip border color
- Dock tab strip background
- Dock tab hover background
- Dock tab active background
- Dock tab active border

Theme profiles also cover semantic checkbox/radio/slider interaction tokens:

- Checkbox hover border
- Checkbox pressed background
- Checkbox pressed border
- Radio button hover border
- Radio button pressed background
- Radio button pressed border
- Slider hover track color
- Slider pressed track color

Theme profiles also cover semantic text-input and flyout/tooltip tokens:

- Text selection color
- Disabled text-control background
- Tooltip background
- Menu flyout background
- Menu flyout border

Theme profiles also cover semantic combo-dropdown and datagrid tokens:

- ComboBox dropdown background
- ComboBox dropdown border
- DataGrid header background
- DataGrid header foreground
- DataGrid selected-row foreground

Theme profiles also cover semantic list/tree selection tokens:

- ListBox hover background
- ListBox selected background
- ListBox selected foreground
- TreeView hover background
- TreeView selected background
- TreeView selected foreground

These tokens are centralized in `ThemeProfile` and applied through `NexenThemeManager` resource overrides so they are not duplicated as ad-hoc values across the UI runtime path.

## Settings UI Coverage

The Preferences theme section includes profile selection and import/export plus picker-based editing for:

- Startup background, startup text, startup primary action
- Menu background, menu highlight, accent color
- Control hover/pressed backgrounds and control hover/pressed borders
- Sidebar border, dock tab strip background, dock tab hover background, dock tab active background, dock tab active border
- Checkbox hover/pressed border/background, radio hover/pressed border/background, slider hover/pressed track
- Text selection, disabled text-control background, tooltip background, menu flyout background, menu flyout border
- Combo dropdown background/border and DataGrid header/selected-row semantic colors
- ListBox and TreeView hover/selected semantic colors

This gives a settings-surface customization path while preserving JSON import/export for advanced profile editing and sharing.

## Updating Default Theme Values Safely

When changing default colors or sizes:

1. Update Dark and Light entries in UI/Styles/NexenBrandTheme.xaml for parity.
2. Keep contrast high for startup text/progress tokens.
3. If changing sizing defaults, update both UI/App.axaml (font tokens) and UI/Styles/NexenStyles.xaml (control metrics).
4. Build and run UI checks in both Dark and Light themes.
5. Verify startup splash readability and progress visibility.

## Language and Theme Interaction

Display language resources are loaded from:

- UI/Localization/resources.en.xml
- UI/Localization/resources.es.xml
- UI/Localization/resources.ja.xml

Language changes in settings now apply immediately without restarting the process.

Main menu language switching is also available directly from the globe menu at the right side of the top menu bar (UI/Views/MainMenuView.axaml).

The globe menu generates language entries from bundled localization resources, updates Preferences UiLanguage, reloads localization resources, and persists config immediately.

## Validation Checklist

- Build passes for Release x64.
- Theme profile import/export still works.
- Splash colors stay in brown/orange branding.
- Progress bar remains visible in both themes.
- Default text/menu/control sizes are touch-friendly.
