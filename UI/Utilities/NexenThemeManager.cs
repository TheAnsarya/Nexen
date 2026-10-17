using Avalonia;
using Avalonia.Media;
using Avalonia.Styling;
using Nexen.Config;

namespace Nexen.Utilities;

public static class NexenThemeManager {
	public static ThemeVariant ResolveThemeVariant(NexenTheme theme) {
		return theme == NexenTheme.Dark ? ThemeVariant.Dark : ThemeVariant.Light;
	}

	public static void ApplyTheme(Application app, NexenTheme theme) {
		if (app is null) {
			return;
		}

		ThemeVariant newTheme = ResolveThemeVariant(theme);
		if (app.RequestedThemeVariant != newTheme) {
			app.RequestedThemeVariant = newTheme;
		}

		ConfigManager.ActiveTheme = theme;
	}

	public static void ApplyTheme(Application app, PreferencesConfig preferences) {
		if (app is null) {
			return;
		}

		ThemeProfile profile = preferences.GetActiveThemeProfile() ?? ThemeProfile.CreateDefault("Default", preferences.Theme);
		ApplyTheme(app, profile.Theme);

		ApplyBrushOverride(app, "NexenStartupWindowBackgroundBrush", profile.StartupWindowBackgroundColor);
		ApplyBrushOverride(app, "NexenStartupSurfaceBackgroundBrush", profile.StartupSurfaceBackgroundColor);
		ApplyBrushOverride(app, "NexenStartupBorderBrush", profile.StartupBorderColor);
		ApplyBrushOverride(app, "NexenStartupCardBackgroundBrush", profile.StartupCardBackgroundColor);
		ApplyBrushOverride(app, "NexenStartupCardCheckedBackgroundBrush", profile.StartupCardCheckedBackgroundColor);
		ApplyBrushOverride(app, "NexenStartupTextBrush", profile.StartupTextColor);
		ApplyBrushOverride(app, "NexenStartupMutedTextBrush", profile.StartupMutedTextColor);
		ApplyBrushOverride(app, "NexenStartupActionBorderBrush", profile.StartupActionBorderColor);
		ApplyBrushOverride(app, "NexenStartupPrimaryActionBackgroundBrush", profile.StartupPrimaryActionColor);
		ApplyBrushOverride(app, "NexenStartupDividerBrush", profile.StartupDividerColor);
		ApplyBrushOverride(app, "NexenSetupSubtitleBrush", profile.SetupSubtitleColor);

		ApplyColorOverride(app, "NexenMenuBackground", profile.MenuBackgroundColor);
		ApplyColorOverride(app, "NexenMenuBackgroundHighlight", profile.MenuBackgroundHighlightColor);
		ApplyColorOverride(app, "NexenMenuBorderHighlight", profile.MenuBorderHighlightColor);
		ApplyColorOverride(app, "NexenMenuSeparatorBackground", profile.MenuSeparatorBackgroundColor);
		ApplyColorOverride(app, "SettingsTabStripBackgroundColor", profile.SettingsTabStripBackgroundColor);
		ApplyColorOverride(app, "SettingsTabStripBorderColor", profile.SettingsTabStripBorderColor);
		ApplyColorOverride(app, "NexenDockTabStripBackgroundColor", profile.DockTabStripBackgroundColor);
		ApplyColorOverride(app, "NexenDockTabPointerOverColor", profile.DockTabPointerOverColor);
		ApplyColorOverride(app, "NexenDockTabActiveBackgroundColor", profile.DockTabActiveBackgroundColor);
		ApplyColorOverride(app, "NexenDockTabActiveBorderColor", profile.DockTabActiveBorderColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundStrokeUncheckedPointerOver", profile.CheckBoxPointerOverBorderColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundStrokeCheckedPointerOver", profile.CheckBoxPointerOverBorderColor);
		ApplyColorOverride(app, "CheckBoxBorderBrushCheckedPointerOver", profile.CheckBoxPointerOverBorderColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundFillCheckedPressed", profile.CheckBoxPressedBackgroundColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundFillUncheckedPressed", profile.CheckBoxPressedBackgroundColor);
		ApplyColorOverride(app, "CheckBoxBackgroundCheckedPressed", profile.CheckBoxPressedBackgroundColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundStrokeCheckedPressed", profile.CheckBoxPressedBorderColor);
		ApplyColorOverride(app, "CheckBoxCheckBackgroundStrokeUncheckedPressed", profile.CheckBoxPressedBorderColor);
		ApplyColorOverride(app, "CheckBoxBorderBrushCheckedPressed", profile.CheckBoxPressedBorderColor);
		ApplyColorOverride(app, "RadioButtonOuterEllipseStrokePointerOver", profile.RadioButtonPointerOverBorderColor);
		ApplyColorOverride(app, "RadioButtonOuterEllipseFillPressed", profile.RadioButtonPressedBackgroundColor);
		ApplyColorOverride(app, "RadioButtonOuterEllipseStrokePressed", profile.RadioButtonPressedBorderColor);
		ApplyColorOverride(app, "SliderTrackFillPointerOver", profile.SliderTrackPointerOverColor);
		ApplyColorOverride(app, "SliderTrackFillPressed", profile.SliderTrackPressedColor);
		ApplyBrushOverride(app, "TextBoxSelectionBrush", profile.TextBoxSelectionColor);
		ApplyColorOverride(app, "TextControlBackgroundDisabled", profile.TextControlDisabledBackgroundColor);
		ApplyColorOverride(app, "ToolTipBackground", profile.ToolTipBackgroundColor);
		ApplyColorOverride(app, "MenuFlyoutPresenterBackground", profile.MenuFlyoutBackgroundColor);
		ApplyColorOverride(app, "MenuFlyoutPresenterBorderBrush", profile.MenuFlyoutBorderColor);
		ApplyColorOverride(app, "ComboBoxDropDownBackground", profile.ComboBoxDropDownBackgroundColor);
		ApplyColorOverride(app, "ComboBoxDropDownBorderBrush", profile.ComboBoxDropDownBorderColor);
		ApplyBrushOverride(app, "DataGridColumnHeaderBackgroundBrush", profile.DataGridHeaderBackgroundColor);
		ApplyBrushOverride(app, "DataGridColumnHeaderForegroundBrush", profile.DataGridHeaderForegroundColor);
		ApplyBrushOverride(app, "DataGridRowSelectedForegroundBrush", profile.DataGridSelectedRowForegroundColor);
		ApplyColorOverride(app, "ListBoxItemBackgroundPointerOver", profile.ListBoxItemHoverBackgroundColor);
		ApplyColorOverride(app, "ListBoxItemBackgroundSelected", profile.ListBoxItemSelectedBackgroundColor);
		ApplyColorOverride(app, "ListBoxItemForegroundSelected", profile.ListBoxItemSelectedForegroundColor);
		ApplyColorOverride(app, "TreeViewItemBackgroundPointerOver", profile.TreeViewItemHoverBackgroundColor);
		ApplyColorOverride(app, "TreeViewItemBackgroundSelected", profile.TreeViewItemSelectedBackgroundColor);
		ApplyColorOverride(app, "TreeViewItemForegroundSelected", profile.TreeViewItemSelectedForegroundColor);
		ApplyColorOverride(app, "NavigationViewItemBackgroundPointerOver", profile.NavigationViewItemHoverBackgroundColor);
		ApplyColorOverride(app, "NavigationViewItemBackgroundSelected", profile.NavigationViewItemSelectedBackgroundColor);
		ApplyColorOverride(app, "NavigationViewItemForegroundSelected", profile.NavigationViewItemSelectedForegroundColor);
		ApplyColorOverride(app, "ListViewItemBackgroundPointerOver", profile.ListViewItemHoverBackgroundColor);
		ApplyColorOverride(app, "ListViewItemBackgroundSelected", profile.ListViewItemSelectedBackgroundColor);
		ApplyColorOverride(app, "ListViewItemForegroundSelected", profile.ListViewItemSelectedForegroundColor);
		ApplyColorOverride(app, "ThemeAccentColor", profile.ThemeAccentColor);
		ApplyColorOverride(app, "HighlightColor", profile.HighlightColor);

		ApplyColorOverride(app, "ButtonBackgroundPointerOver", profile.ControlPointerOverBackgroundColor);
		ApplyColorOverride(app, "ButtonBackgroundPressed", profile.ControlPressedBackgroundColor);
		ApplyColorOverride(app, "ButtonBorderBrushPointerOver", profile.ControlPointerOverBorderColor);
		ApplyColorOverride(app, "ButtonBorderBrushPressed", profile.ControlPressedBorderColor);
		ApplyColorOverride(app, "ComboBoxBackgroundPointerOver", profile.ControlPointerOverBackgroundColor);
		ApplyColorOverride(app, "ComboBoxBorderBrushPointerOver", profile.ControlPointerOverBorderColor);
		ApplyColorOverride(app, "RepeatButtonBackgroundPointerOver", profile.ControlPointerOverBackgroundColor);
		ApplyColorOverride(app, "RepeatButtonBackgroundPressed", profile.ControlPressedBackgroundColor);
		ApplyColorOverride(app, "RepeatButtonBorderBrushPointerOver", profile.ControlPointerOverBorderColor);
		ApplyColorOverride(app, "RepeatButtonBorderBrushPressed", profile.ControlPressedBorderColor);
		ApplyDerivedAccentVariants(app, profile.ThemeAccentColor);

		app.Resources["NexenSetupTitleFontSize"] = profile.SetupTitleFontSize;
		app.Resources["NexenSetupSubtitleFontSize"] = profile.SetupSubtitleFontSize;
		app.Resources["NexenSetupPrimaryActionFontSize"] = profile.SetupPrimaryActionFontSize;
	}

	private static void ApplyBrushOverride(Application app, string resourceKey, string colorValue) {
		try {
			Color color = Color.Parse(colorValue);
			app.Resources[resourceKey] = new SolidColorBrush(color);
		} catch {
			// Ignore invalid color values to keep startup resilient when importing malformed profiles.
		}
	}

	private static void ApplyColorOverride(Application app, string resourceKey, string colorValue) {
		try {
			app.Resources[resourceKey] = Color.Parse(colorValue);
		} catch {
			// Ignore invalid color values to keep runtime resilient when importing malformed profiles.
		}
	}

	private static void ApplyDerivedAccentVariants(Application app, string accentColorValue) {
		try {
			Color accent = Color.Parse(accentColorValue);
			app.Resources["ThemeAccentColor2"] = new Color(0x99, accent.R, accent.G, accent.B);
			app.Resources["ThemeAccentColor3"] = new Color(0x66, accent.R, accent.G, accent.B);
			app.Resources["ThemeAccentColor4"] = new Color(0x33, accent.R, accent.G, accent.B);
		} catch {
			// Ignore invalid accent values to keep runtime resilient when importing malformed profiles.
		}
	}
}
