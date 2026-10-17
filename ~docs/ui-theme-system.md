# UI Theme System Guide

## Purpose

Nexen UI theme values must be centralized so colors, fonts, and sizes are not duplicated across views.

This guide documents how to use the new brand theme tokens and how to migrate existing UI code.

## Theme Architecture

The theme system is split into three layers:

1. Base app resources in `UI/App.axaml`:
	- Global font families and baseline font sizes.
	- Global style includes.
2. General app theme dictionaries in `UI/Styles/NexenStyles.Light.xaml` and `UI/Styles/NexenStyles.Dark.xaml`:
	- Existing Fluent and control-level theme resources.
3. Brand token dictionary in `UI/Styles/NexenBrandTheme.xaml`:
	- Brown/orange/yellow/white/cream token set.
	- Startup/setup semantic brushes.
	- Shared setup typography size tokens.

## Theme Variant Behavior

Theme variant switching remains driven by user preferences (`NexenTheme.Light`/`NexenTheme.Dark`) and Avalonia `RequestedThemeVariant`.

`UI/Utilities/NexenThemeManager.cs` now provides the canonical mapping and apply path:

- `ResolveThemeVariant(...)`
- `ApplyTheme(...)`

Use this manager when changing active theme behavior to avoid duplicated conversion logic.

## Brand Tokens

Primary token file:

- `UI/Styles/NexenBrandTheme.xaml`

Key semantic resources:

- `NexenStartupWindowBackgroundBrush`
- `NexenStartupSurfaceBackgroundBrush`
- `NexenStartupBorderBrush`
- `NexenStartupCardBackgroundBrush`
- `NexenStartupActionBackgroundBrush`
- `NexenStartupActionBorderBrush`
- `NexenStartupMutedTextBrush`
- `NexenStartupTextBrush`
- `NexenStartupWarningTextBrush`
- `NexenSetupCaptionFontSize`
- `NexenSetupTitleFontSize`
- `NexenSetupSectionTitleFontSize`

## Migration Rules

1. Do not add hardcoded hex colors in UI views for branded surfaces.
2. Prefer semantic `DynamicResource` keys over direct palette keys.
3. Reuse setup/startup typography tokens instead of duplicating size constants.
4. If a needed semantic token does not exist, add it in `NexenBrandTheme.xaml` instead of embedding a literal in a view.

## Migration Example

Before:

```xml
<TextBlock Foreground="#ffefcfb6" FontSize="16" />
```

After:

```xml
<TextBlock
	Foreground="{DynamicResource NexenStartupMutedTextBrush}"
	FontSize="{DynamicResource NexenSetupSubtitleFontSize}" />
```

## Applied Surfaces

This migration currently covers:

- `UI/Windows/SplashWindow.axaml`
- `UI/Windows/SetupWizardWindow.axaml`

Additional UI components should migrate to semantic theme tokens in future passes.
