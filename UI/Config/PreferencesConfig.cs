using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Media;
using Avalonia.Styling;
using Avalonia.Threading;
using Nexen.Config.Shortcuts;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class PreferencesConfig : BaseConfig<PreferencesConfig> {
	[Reactive] public partial PrimaryUsageProfile PrimaryUsageProfile { get; set; } = PrimaryUsageProfile.Playing;
	[Reactive] public partial bool PreferUserProfileStorage { get; set; } = true;
	[Reactive] public partial NexenTheme Theme { get; set; } = NexenTheme.Light;
	[Reactive] public partial List<ThemeProfile> ThemeProfiles { get; set; } = [];
	[Reactive] public partial string ActiveThemeProfileName { get; set; } = "";
	[Reactive] public partial UiLanguage UiLanguage { get; set; } = UiLanguage.English;
	[Reactive] public partial bool AutomaticallyCheckForUpdates { get; set; } = true;
	[Reactive] public partial bool SingleInstance { get; set; } = true;
	[Reactive] public partial bool AutoLoadPatches { get; set; } = true;

	[Reactive] public partial bool PauseWhenInBackground { get; set; } = false;
	[Reactive] public partial bool PauseWhenInMenusAndConfig { get; set; } = false;
	[Reactive] public partial bool AllowBackgroundInput { get; set; } = false;
	[Reactive] public partial bool PauseOnMovieEnd { get; set; } = true;
	[Reactive] public partial bool ShowMovieIcons { get; set; } = true;
	[Reactive] public partial bool ShowTurboRewindIcons { get; set; } = true;
	[Reactive] public partial bool ConfirmExitResetPower { get; set; } = false;

	[Reactive] public partial bool AssociateSnesRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSnesMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateNesRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateNesMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbaRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociatePceRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociatePceMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSmsRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGameGearRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSgRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateCvRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateWsRomFiles { get; set; } = false;

	[Reactive] public partial bool EnableAutoSaveState { get; set; } = true;
	[Reactive] public partial UInt32 AutoSaveStateDelay { get; set; } = 20;
	[Reactive] public partial bool ShowAutoSaveNotifications { get; set; } = true;
	[Reactive] public partial bool ShowRecentPlayNotifications { get; set; } = false;

	[Reactive] public partial bool EnableRewind { get; set; } = true;
	[Reactive] public partial UInt32 RewindBufferSize { get; set; } = 300;

	[Reactive] public partial bool AlwaysOnTop { get; set; } = false;

	[Reactive] public partial bool AutoHideMenu { get; set; } = false;

	[Reactive] public partial bool ShowFps { get; set; } = false;
	[Reactive] public partial bool ShowFrameCounter { get; set; } = false;
	[Reactive] public partial bool ShowGameTimer { get; set; } = false;
	[Reactive] public partial bool ShowLagCounter { get; set; } = false;
	[Reactive] public partial bool ShowRerecordCounter { get; set; } = false;
	[Reactive] public partial bool ShowLagFrameIndicator { get; set; } = false;
	[Reactive] public partial bool ShowTitleBarInfo { get; set; } = false;
	[Reactive] public partial bool ShowDebugInfo { get; set; } = false;
	[Reactive] public partial bool DisableOsd { get; set; } = false;
	[Reactive] public partial HudDisplaySize HudSize { get; set; } = HudDisplaySize.Fixed;
	[Reactive] public partial GameSelectionMode GameSelectionScreenMode { get; set; } = GameSelectionMode.ResumeState;

	[Reactive] public partial FontAntialiasing FontAntialiasing { get; set; } = FontAntialiasing.SubPixelAntialias;
	[Reactive] public partial FontConfig NexenFont { get; set; } = new FontConfig() { FontFamily = "Microsoft Sans Serif", FontSize = 11 };
	[Reactive] public partial FontConfig NexenMenuFont { get; set; } = new FontConfig() { FontFamily = "Segoe UI", FontSize = 12 };

	[Reactive] public partial List<ShortcutKeyInfo> ShortcutKeys { get; set; } = [];

	[Reactive] public partial bool OverrideGameFolder { get; set; } = false;
	[Reactive] public partial bool OverrideAviFolder { get; set; } = false;
	[Reactive] public partial bool OverrideMovieFolder { get; set; } = false;
	[Reactive] public partial bool OverrideSaveDataFolder { get; set; } = false;
	[Reactive] public partial bool OverrideSaveStateFolder { get; set; } = false;
	[Reactive] public partial bool OverrideScreenshotFolder { get; set; } = false;
	[Reactive] public partial bool OverrideWaveFolder { get; set; } = false;

	[Reactive] public partial string GameFolder { get; set; } = "";
	[Reactive] public partial string AviFolder { get; set; } = "";
	[Reactive] public partial string MovieFolder { get; set; } = "";
	[Reactive] public partial string SaveDataFolder { get; set; } = "";
	[Reactive] public partial string SaveStateFolder { get; set; } = "";
	[Reactive] public partial string ScreenshotFolder { get; set; } = "";
	[Reactive] public partial string WaveFolder { get; set; } = "";

	public PreferencesConfig() {
		EnsureThemeProfiles();
	}

	public void EnsureThemeProfiles() {
		if (ThemeProfiles.Count == 0) {
			ThemeProfiles.Add(ThemeProfile.CreateDefault("Default Light", NexenTheme.Light));
			ThemeProfiles.Add(ThemeProfile.CreateDefault("Default Dark", NexenTheme.Dark));
		}

		if (string.IsNullOrWhiteSpace(ActiveThemeProfileName) || !ThemeProfiles.Any(profile => profile.Name == ActiveThemeProfileName)) {
			ActiveThemeProfileName = Theme == NexenTheme.Dark ? "Default Dark" : "Default Light";
		}
	}

	public ThemeProfile? GetActiveThemeProfile() {
		EnsureThemeProfiles();
		return ThemeProfiles.FirstOrDefault(profile => profile.Name == ActiveThemeProfileName);
	}

	public ThemeProfile? GetThemeProfileByName(string? profileName) {
		if (string.IsNullOrWhiteSpace(profileName)) {
			return null;
		}

		EnsureThemeProfiles();
		return ThemeProfiles.FirstOrDefault(profile => profile.Name == profileName);
	}

	public void SetActiveThemeProfile(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return;
		}

		ActiveThemeProfileName = profile.Name;
		ApplyThemeProfile(profile);
	}

	public void SaveCurrentToProfile(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return;
		}

		profile.Theme = Theme;
		profile.UiFontFamily = NexenFont.FontFamily;
		profile.UiFontSize = NexenFont.FontSize;
		profile.MenuFontFamily = NexenMenuFont.FontFamily;
		profile.MenuFontSize = NexenMenuFont.FontSize;
		if (Application.Current?.Resources is not null) {
			profile.MenuBackgroundColor = TryReadResourceColor("NexenMenuBackground", profile.MenuBackgroundColor);
			profile.MenuBackgroundHighlightColor = TryReadResourceColor("NexenMenuBackgroundHighlight", profile.MenuBackgroundHighlightColor);
			profile.MenuBorderHighlightColor = TryReadResourceColor("NexenMenuBorderHighlight", profile.MenuBorderHighlightColor);
			profile.MenuSeparatorBackgroundColor = TryReadResourceColor("NexenMenuSeparatorBackground", profile.MenuSeparatorBackgroundColor);
			profile.SettingsTabStripBackgroundColor = TryReadResourceColor("SettingsTabStripBackgroundColor", profile.SettingsTabStripBackgroundColor);
			profile.SettingsTabStripBorderColor = TryReadResourceColor("SettingsTabStripBorderColor", profile.SettingsTabStripBorderColor);
			profile.DockTabStripBackgroundColor = TryReadResourceColor("NexenDockTabStripBackgroundColor", profile.DockTabStripBackgroundColor);
			profile.DockTabPointerOverColor = TryReadResourceColor("NexenDockTabPointerOverColor", profile.DockTabPointerOverColor);
			profile.DockTabActiveBackgroundColor = TryReadResourceColor("NexenDockTabActiveBackgroundColor", profile.DockTabActiveBackgroundColor);
			profile.DockTabActiveBorderColor = TryReadResourceColor("NexenDockTabActiveBorderColor", profile.DockTabActiveBorderColor);
			profile.CheckBoxPointerOverBorderColor = TryReadResourceColor("CheckBoxBorderBrushCheckedPointerOver", profile.CheckBoxPointerOverBorderColor);
			profile.CheckBoxPressedBackgroundColor = TryReadResourceColor("CheckBoxCheckBackgroundFillCheckedPressed", profile.CheckBoxPressedBackgroundColor);
			profile.CheckBoxPressedBorderColor = TryReadResourceColor("CheckBoxCheckBackgroundStrokeCheckedPressed", profile.CheckBoxPressedBorderColor);
			profile.RadioButtonPointerOverBorderColor = TryReadResourceColor("RadioButtonOuterEllipseStrokePointerOver", profile.RadioButtonPointerOverBorderColor);
			profile.RadioButtonPressedBackgroundColor = TryReadResourceColor("RadioButtonOuterEllipseFillPressed", profile.RadioButtonPressedBackgroundColor);
			profile.RadioButtonPressedBorderColor = TryReadResourceColor("RadioButtonOuterEllipseStrokePressed", profile.RadioButtonPressedBorderColor);
			profile.SliderTrackPointerOverColor = TryReadResourceColor("SliderTrackFillPointerOver", profile.SliderTrackPointerOverColor);
			profile.SliderTrackPressedColor = TryReadResourceColor("SliderTrackFillPressed", profile.SliderTrackPressedColor);
			profile.TextBoxSelectionColor = TryReadResourceBrushColor("TextBoxSelectionBrush", profile.TextBoxSelectionColor);
			profile.TextControlDisabledBackgroundColor = TryReadResourceColor("TextControlBackgroundDisabled", profile.TextControlDisabledBackgroundColor);
			profile.ToolTipBackgroundColor = TryReadResourceColor("ToolTipBackground", profile.ToolTipBackgroundColor);
			profile.MenuFlyoutBackgroundColor = TryReadResourceColor("MenuFlyoutPresenterBackground", profile.MenuFlyoutBackgroundColor);
			profile.MenuFlyoutBorderColor = TryReadResourceColor("MenuFlyoutPresenterBorderBrush", profile.MenuFlyoutBorderColor);
			profile.ComboBoxDropDownBackgroundColor = TryReadResourceColor("ComboBoxDropDownBackground", profile.ComboBoxDropDownBackgroundColor);
			profile.ComboBoxDropDownBorderColor = TryReadResourceColor("ComboBoxDropDownBorderBrush", profile.ComboBoxDropDownBorderColor);
			profile.DataGridHeaderBackgroundColor = TryReadResourceBrushColor("DataGridColumnHeaderBackgroundBrush", profile.DataGridHeaderBackgroundColor);
			profile.DataGridHeaderForegroundColor = TryReadResourceBrushColor("DataGridColumnHeaderForegroundBrush", profile.DataGridHeaderForegroundColor);
			profile.DataGridSelectedRowForegroundColor = TryReadResourceBrushColor("DataGridRowSelectedForegroundBrush", profile.DataGridSelectedRowForegroundColor);
			profile.ListBoxItemHoverBackgroundColor = TryReadResourceColor("ListBoxItemBackgroundPointerOver", profile.ListBoxItemHoverBackgroundColor);
			profile.ListBoxItemSelectedBackgroundColor = TryReadResourceColor("ListBoxItemBackgroundSelected", profile.ListBoxItemSelectedBackgroundColor);
			profile.ListBoxItemSelectedForegroundColor = TryReadResourceColor("ListBoxItemForegroundSelected", profile.ListBoxItemSelectedForegroundColor);
			profile.TreeViewItemHoverBackgroundColor = TryReadResourceColor("TreeViewItemBackgroundPointerOver", profile.TreeViewItemHoverBackgroundColor);
			profile.TreeViewItemSelectedBackgroundColor = TryReadResourceColor("TreeViewItemBackgroundSelected", profile.TreeViewItemSelectedBackgroundColor);
			profile.TreeViewItemSelectedForegroundColor = TryReadResourceColor("TreeViewItemForegroundSelected", profile.TreeViewItemSelectedForegroundColor);
			profile.NavigationViewItemHoverBackgroundColor = TryReadResourceColor("NavigationViewItemBackgroundPointerOver", profile.NavigationViewItemHoverBackgroundColor);
			profile.NavigationViewItemSelectedBackgroundColor = TryReadResourceColor("NavigationViewItemBackgroundSelected", profile.NavigationViewItemSelectedBackgroundColor);
			profile.NavigationViewItemSelectedForegroundColor = TryReadResourceColor("NavigationViewItemForegroundSelected", profile.NavigationViewItemSelectedForegroundColor);
			profile.ListViewItemHoverBackgroundColor = TryReadResourceColor("ListViewItemBackgroundPointerOver", profile.ListViewItemHoverBackgroundColor);
			profile.ListViewItemSelectedBackgroundColor = TryReadResourceColor("ListViewItemBackgroundSelected", profile.ListViewItemSelectedBackgroundColor);
			profile.ListViewItemSelectedForegroundColor = TryReadResourceColor("ListViewItemForegroundSelected", profile.ListViewItemSelectedForegroundColor);
			profile.ThemeAccentColor = TryReadResourceColor("ThemeAccentColor", profile.ThemeAccentColor);
			profile.HighlightColor = TryReadResourceColor("HighlightColor", profile.HighlightColor);
			profile.ControlPointerOverBackgroundColor = TryReadResourceColor("ButtonBackgroundPointerOver", profile.ControlPointerOverBackgroundColor);
			profile.ControlPressedBackgroundColor = TryReadResourceColor("ButtonBackgroundPressed", profile.ControlPressedBackgroundColor);
			profile.ControlPointerOverBorderColor = TryReadResourceColor("ButtonBorderBrushPointerOver", profile.ControlPointerOverBorderColor);
			profile.ControlPressedBorderColor = TryReadResourceColor("ButtonBorderBrushPressed", profile.ControlPressedBorderColor);
		}
	}

	public bool DeleteThemeProfile(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return false;
		}

		if (ThemeProfiles.Count <= 1) {
			return false;
		}

		ThemeProfiles.Remove(profile);
		if (ActiveThemeProfileName == profile.Name) {
			ThemeProfile fallback = ThemeProfiles[0];
			ActiveThemeProfileName = fallback.Name;
			ApplyThemeProfile(fallback);
		}

		return true;
	}

	public void UpsertThemeProfile(ThemeProfile profile, bool activateProfile) {
		EnsureThemeProfiles();
		profile.Name = profile.Name.Trim();
		ThemeProfile? existing = ThemeProfiles.FirstOrDefault(p => p.Name == profile.Name);
		if (existing is not null) {
			existing.Theme = profile.Theme;
			existing.UiFontFamily = profile.UiFontFamily;
			existing.UiFontSize = profile.UiFontSize;
			existing.MenuFontFamily = profile.MenuFontFamily;
			existing.MenuFontSize = profile.MenuFontSize;
			existing.StartupWindowBackgroundColor = profile.StartupWindowBackgroundColor;
			existing.StartupSurfaceBackgroundColor = profile.StartupSurfaceBackgroundColor;
			existing.StartupBorderColor = profile.StartupBorderColor;
			existing.StartupCardBackgroundColor = profile.StartupCardBackgroundColor;
			existing.StartupCardCheckedBackgroundColor = profile.StartupCardCheckedBackgroundColor;
			existing.StartupTextColor = profile.StartupTextColor;
			existing.StartupMutedTextColor = profile.StartupMutedTextColor;
			existing.StartupActionBorderColor = profile.StartupActionBorderColor;
			existing.StartupPrimaryActionColor = profile.StartupPrimaryActionColor;
			existing.StartupDividerColor = profile.StartupDividerColor;
			existing.SetupSubtitleColor = profile.SetupSubtitleColor;
			existing.MenuBackgroundColor = profile.MenuBackgroundColor;
			existing.MenuBackgroundHighlightColor = profile.MenuBackgroundHighlightColor;
			existing.MenuBorderHighlightColor = profile.MenuBorderHighlightColor;
			existing.MenuSeparatorBackgroundColor = profile.MenuSeparatorBackgroundColor;
			existing.SettingsTabStripBackgroundColor = profile.SettingsTabStripBackgroundColor;
			existing.SettingsTabStripBorderColor = profile.SettingsTabStripBorderColor;
			existing.DockTabStripBackgroundColor = profile.DockTabStripBackgroundColor;
			existing.DockTabPointerOverColor = profile.DockTabPointerOverColor;
			existing.DockTabActiveBackgroundColor = profile.DockTabActiveBackgroundColor;
			existing.DockTabActiveBorderColor = profile.DockTabActiveBorderColor;
			existing.CheckBoxPointerOverBorderColor = profile.CheckBoxPointerOverBorderColor;
			existing.CheckBoxPressedBackgroundColor = profile.CheckBoxPressedBackgroundColor;
			existing.CheckBoxPressedBorderColor = profile.CheckBoxPressedBorderColor;
			existing.RadioButtonPointerOverBorderColor = profile.RadioButtonPointerOverBorderColor;
			existing.RadioButtonPressedBackgroundColor = profile.RadioButtonPressedBackgroundColor;
			existing.RadioButtonPressedBorderColor = profile.RadioButtonPressedBorderColor;
			existing.SliderTrackPointerOverColor = profile.SliderTrackPointerOverColor;
			existing.SliderTrackPressedColor = profile.SliderTrackPressedColor;
			existing.TextBoxSelectionColor = profile.TextBoxSelectionColor;
			existing.TextControlDisabledBackgroundColor = profile.TextControlDisabledBackgroundColor;
			existing.ToolTipBackgroundColor = profile.ToolTipBackgroundColor;
			existing.MenuFlyoutBackgroundColor = profile.MenuFlyoutBackgroundColor;
			existing.MenuFlyoutBorderColor = profile.MenuFlyoutBorderColor;
			existing.ComboBoxDropDownBackgroundColor = profile.ComboBoxDropDownBackgroundColor;
			existing.ComboBoxDropDownBorderColor = profile.ComboBoxDropDownBorderColor;
			existing.DataGridHeaderBackgroundColor = profile.DataGridHeaderBackgroundColor;
			existing.DataGridHeaderForegroundColor = profile.DataGridHeaderForegroundColor;
			existing.DataGridSelectedRowForegroundColor = profile.DataGridSelectedRowForegroundColor;
			existing.ListBoxItemHoverBackgroundColor = profile.ListBoxItemHoverBackgroundColor;
			existing.ListBoxItemSelectedBackgroundColor = profile.ListBoxItemSelectedBackgroundColor;
			existing.ListBoxItemSelectedForegroundColor = profile.ListBoxItemSelectedForegroundColor;
			existing.TreeViewItemHoverBackgroundColor = profile.TreeViewItemHoverBackgroundColor;
			existing.TreeViewItemSelectedBackgroundColor = profile.TreeViewItemSelectedBackgroundColor;
			existing.TreeViewItemSelectedForegroundColor = profile.TreeViewItemSelectedForegroundColor;
			existing.NavigationViewItemHoverBackgroundColor = profile.NavigationViewItemHoverBackgroundColor;
			existing.NavigationViewItemSelectedBackgroundColor = profile.NavigationViewItemSelectedBackgroundColor;
			existing.NavigationViewItemSelectedForegroundColor = profile.NavigationViewItemSelectedForegroundColor;
			existing.ListViewItemHoverBackgroundColor = profile.ListViewItemHoverBackgroundColor;
			existing.ListViewItemSelectedBackgroundColor = profile.ListViewItemSelectedBackgroundColor;
			existing.ListViewItemSelectedForegroundColor = profile.ListViewItemSelectedForegroundColor;
			existing.ThemeAccentColor = profile.ThemeAccentColor;
			existing.HighlightColor = profile.HighlightColor;
			existing.ControlPointerOverBackgroundColor = profile.ControlPointerOverBackgroundColor;
			existing.ControlPressedBackgroundColor = profile.ControlPressedBackgroundColor;
			existing.ControlPointerOverBorderColor = profile.ControlPointerOverBorderColor;
			existing.ControlPressedBorderColor = profile.ControlPressedBorderColor;
			existing.SetupTitleFontSize = profile.SetupTitleFontSize;
			existing.SetupSubtitleFontSize = profile.SetupSubtitleFontSize;
			existing.SetupPrimaryActionFontSize = profile.SetupPrimaryActionFontSize;
		} else {
			ThemeProfiles.Add(profile);
		}

		if (activateProfile) {
			SetActiveThemeProfile(profile.Name);
		}
	}

	public string GenerateUniqueThemeProfileName(string baseName) {
		string sanitized = string.IsNullOrWhiteSpace(baseName) ? "Imported Theme" : baseName.Trim();
		if (!ThemeProfiles.Any(p => p.Name == sanitized)) {
			return sanitized;
		}

		int index = 2;
		string candidate = sanitized + " (Copy)";
		while (ThemeProfiles.Any(p => p.Name == candidate)) {
			candidate = sanitized + " (Copy " + index + ")";
			index++;
		}

		return candidate;
	}

	public void ApplyThemeProfile(ThemeProfile profile) {
		Theme = profile.Theme;
		NexenFont.FontFamily = profile.UiFontFamily;
		NexenFont.FontSize = profile.UiFontSize;
		NexenMenuFont.FontFamily = profile.MenuFontFamily;
		NexenMenuFont.FontSize = profile.MenuFontSize;
		UpdateFonts();
		UpdateTheme();
	}

	public bool RenameThemeProfile(string? oldName, string? newName) {
		ThemeProfile? profile = GetThemeProfileByName(oldName);
		if (profile is null || string.IsNullOrWhiteSpace(newName)) {
			return false;
		}

		string trimmed = newName.Trim();
		if (ThemeProfiles.Any(p => p.Name == trimmed)) {
			return false;
		}

		profile.Name = trimmed;
		if (ActiveThemeProfileName == oldName) {
			ActiveThemeProfileName = trimmed;
		}

		return true;
	}

	public bool DuplicateThemeProfile(string? sourceName, string? newName) {
		ThemeProfile? source = GetThemeProfileByName(sourceName);
		if (source is null || string.IsNullOrWhiteSpace(newName)) {
			return false;
		}

		string trimmed = newName.Trim();
		if (ThemeProfiles.Any(p => p.Name == trimmed)) {
			return false;
		}

		ThemeProfile duplicate = new ThemeProfile {
			Name = trimmed,
			Theme = source.Theme,
			UiFontFamily = source.UiFontFamily,
			UiFontSize = source.UiFontSize,
			MenuFontFamily = source.MenuFontFamily,
			MenuFontSize = source.MenuFontSize,
			StartupWindowBackgroundColor = source.StartupWindowBackgroundColor,
			StartupSurfaceBackgroundColor = source.StartupSurfaceBackgroundColor,
			StartupBorderColor = source.StartupBorderColor,
			StartupCardBackgroundColor = source.StartupCardBackgroundColor,
			StartupCardCheckedBackgroundColor = source.StartupCardCheckedBackgroundColor,
			StartupTextColor = source.StartupTextColor,
			StartupMutedTextColor = source.StartupMutedTextColor,
			StartupActionBorderColor = source.StartupActionBorderColor,
			StartupPrimaryActionColor = source.StartupPrimaryActionColor,
			StartupDividerColor = source.StartupDividerColor,
			SetupSubtitleColor = source.SetupSubtitleColor,
			MenuBackgroundColor = source.MenuBackgroundColor,
			MenuBackgroundHighlightColor = source.MenuBackgroundHighlightColor,
			MenuBorderHighlightColor = source.MenuBorderHighlightColor,
			MenuSeparatorBackgroundColor = source.MenuSeparatorBackgroundColor,
			SettingsTabStripBackgroundColor = source.SettingsTabStripBackgroundColor,
			SettingsTabStripBorderColor = source.SettingsTabStripBorderColor,
			DockTabStripBackgroundColor = source.DockTabStripBackgroundColor,
			DockTabPointerOverColor = source.DockTabPointerOverColor,
			DockTabActiveBackgroundColor = source.DockTabActiveBackgroundColor,
			DockTabActiveBorderColor = source.DockTabActiveBorderColor,
			CheckBoxPointerOverBorderColor = source.CheckBoxPointerOverBorderColor,
			CheckBoxPressedBackgroundColor = source.CheckBoxPressedBackgroundColor,
			CheckBoxPressedBorderColor = source.CheckBoxPressedBorderColor,
			RadioButtonPointerOverBorderColor = source.RadioButtonPointerOverBorderColor,
			RadioButtonPressedBackgroundColor = source.RadioButtonPressedBackgroundColor,
			RadioButtonPressedBorderColor = source.RadioButtonPressedBorderColor,
			SliderTrackPointerOverColor = source.SliderTrackPointerOverColor,
			SliderTrackPressedColor = source.SliderTrackPressedColor,
			TextBoxSelectionColor = source.TextBoxSelectionColor,
			TextControlDisabledBackgroundColor = source.TextControlDisabledBackgroundColor,
			ToolTipBackgroundColor = source.ToolTipBackgroundColor,
			MenuFlyoutBackgroundColor = source.MenuFlyoutBackgroundColor,
			MenuFlyoutBorderColor = source.MenuFlyoutBorderColor,
			ComboBoxDropDownBackgroundColor = source.ComboBoxDropDownBackgroundColor,
			ComboBoxDropDownBorderColor = source.ComboBoxDropDownBorderColor,
			DataGridHeaderBackgroundColor = source.DataGridHeaderBackgroundColor,
			DataGridHeaderForegroundColor = source.DataGridHeaderForegroundColor,
			DataGridSelectedRowForegroundColor = source.DataGridSelectedRowForegroundColor,
			ListBoxItemHoverBackgroundColor = source.ListBoxItemHoverBackgroundColor,
			ListBoxItemSelectedBackgroundColor = source.ListBoxItemSelectedBackgroundColor,
			ListBoxItemSelectedForegroundColor = source.ListBoxItemSelectedForegroundColor,
			TreeViewItemHoverBackgroundColor = source.TreeViewItemHoverBackgroundColor,
			TreeViewItemSelectedBackgroundColor = source.TreeViewItemSelectedBackgroundColor,
			TreeViewItemSelectedForegroundColor = source.TreeViewItemSelectedForegroundColor,
			NavigationViewItemHoverBackgroundColor = source.NavigationViewItemHoverBackgroundColor,
			NavigationViewItemSelectedBackgroundColor = source.NavigationViewItemSelectedBackgroundColor,
			NavigationViewItemSelectedForegroundColor = source.NavigationViewItemSelectedForegroundColor,
			ListViewItemHoverBackgroundColor = source.ListViewItemHoverBackgroundColor,
			ListViewItemSelectedBackgroundColor = source.ListViewItemSelectedBackgroundColor,
			ListViewItemSelectedForegroundColor = source.ListViewItemSelectedForegroundColor,
			ThemeAccentColor = source.ThemeAccentColor,
			HighlightColor = source.HighlightColor,
			ControlPointerOverBackgroundColor = source.ControlPointerOverBackgroundColor,
			ControlPressedBackgroundColor = source.ControlPressedBackgroundColor,
			ControlPointerOverBorderColor = source.ControlPointerOverBorderColor,
			ControlPressedBorderColor = source.ControlPressedBorderColor,
			SetupTitleFontSize = source.SetupTitleFontSize,
			SetupSubtitleFontSize = source.SetupSubtitleFontSize,
			SetupPrimaryActionFontSize = source.SetupPrimaryActionFontSize
		};

		ThemeProfiles.Add(duplicate);
		return true;
	}

	public bool ResetThemeProfileToDefaults(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return false;
		}

		ThemeProfile defaults = ThemeProfile.CreateDefault(profile.Name, profile.Theme);
		profile.UiFontFamily = defaults.UiFontFamily;
		profile.UiFontSize = defaults.UiFontSize;
		profile.MenuFontFamily = defaults.MenuFontFamily;
		profile.MenuFontSize = defaults.MenuFontSize;
		profile.StartupWindowBackgroundColor = defaults.StartupWindowBackgroundColor;
		profile.StartupSurfaceBackgroundColor = defaults.StartupSurfaceBackgroundColor;
		profile.StartupBorderColor = defaults.StartupBorderColor;
		profile.StartupCardBackgroundColor = defaults.StartupCardBackgroundColor;
		profile.StartupCardCheckedBackgroundColor = defaults.StartupCardCheckedBackgroundColor;
		profile.StartupTextColor = defaults.StartupTextColor;
		profile.StartupMutedTextColor = defaults.StartupMutedTextColor;
		profile.StartupActionBorderColor = defaults.StartupActionBorderColor;
		profile.StartupPrimaryActionColor = defaults.StartupPrimaryActionColor;
		profile.StartupDividerColor = defaults.StartupDividerColor;
		profile.SetupSubtitleColor = defaults.SetupSubtitleColor;
		profile.MenuBackgroundColor = defaults.MenuBackgroundColor;
		profile.MenuBackgroundHighlightColor = defaults.MenuBackgroundHighlightColor;
		profile.MenuBorderHighlightColor = defaults.MenuBorderHighlightColor;
		profile.MenuSeparatorBackgroundColor = defaults.MenuSeparatorBackgroundColor;
		profile.SettingsTabStripBackgroundColor = defaults.SettingsTabStripBackgroundColor;
		profile.SettingsTabStripBorderColor = defaults.SettingsTabStripBorderColor;
		profile.DockTabStripBackgroundColor = defaults.DockTabStripBackgroundColor;
		profile.DockTabPointerOverColor = defaults.DockTabPointerOverColor;
		profile.DockTabActiveBackgroundColor = defaults.DockTabActiveBackgroundColor;
		profile.DockTabActiveBorderColor = defaults.DockTabActiveBorderColor;
		profile.CheckBoxPointerOverBorderColor = defaults.CheckBoxPointerOverBorderColor;
		profile.CheckBoxPressedBackgroundColor = defaults.CheckBoxPressedBackgroundColor;
		profile.CheckBoxPressedBorderColor = defaults.CheckBoxPressedBorderColor;
		profile.RadioButtonPointerOverBorderColor = defaults.RadioButtonPointerOverBorderColor;
		profile.RadioButtonPressedBackgroundColor = defaults.RadioButtonPressedBackgroundColor;
		profile.RadioButtonPressedBorderColor = defaults.RadioButtonPressedBorderColor;
		profile.SliderTrackPointerOverColor = defaults.SliderTrackPointerOverColor;
		profile.SliderTrackPressedColor = defaults.SliderTrackPressedColor;
		profile.TextBoxSelectionColor = defaults.TextBoxSelectionColor;
		profile.TextControlDisabledBackgroundColor = defaults.TextControlDisabledBackgroundColor;
		profile.ToolTipBackgroundColor = defaults.ToolTipBackgroundColor;
		profile.MenuFlyoutBackgroundColor = defaults.MenuFlyoutBackgroundColor;
		profile.MenuFlyoutBorderColor = defaults.MenuFlyoutBorderColor;
		profile.ComboBoxDropDownBackgroundColor = defaults.ComboBoxDropDownBackgroundColor;
		profile.ComboBoxDropDownBorderColor = defaults.ComboBoxDropDownBorderColor;
		profile.DataGridHeaderBackgroundColor = defaults.DataGridHeaderBackgroundColor;
		profile.DataGridHeaderForegroundColor = defaults.DataGridHeaderForegroundColor;
		profile.DataGridSelectedRowForegroundColor = defaults.DataGridSelectedRowForegroundColor;
		profile.ListBoxItemHoverBackgroundColor = defaults.ListBoxItemHoverBackgroundColor;
		profile.ListBoxItemSelectedBackgroundColor = defaults.ListBoxItemSelectedBackgroundColor;
		profile.ListBoxItemSelectedForegroundColor = defaults.ListBoxItemSelectedForegroundColor;
		profile.TreeViewItemHoverBackgroundColor = defaults.TreeViewItemHoverBackgroundColor;
		profile.TreeViewItemSelectedBackgroundColor = defaults.TreeViewItemSelectedBackgroundColor;
		profile.TreeViewItemSelectedForegroundColor = defaults.TreeViewItemSelectedForegroundColor;
		profile.NavigationViewItemHoverBackgroundColor = defaults.NavigationViewItemHoverBackgroundColor;
		profile.NavigationViewItemSelectedBackgroundColor = defaults.NavigationViewItemSelectedBackgroundColor;
		profile.NavigationViewItemSelectedForegroundColor = defaults.NavigationViewItemSelectedForegroundColor;
		profile.ListViewItemHoverBackgroundColor = defaults.ListViewItemHoverBackgroundColor;
		profile.ListViewItemSelectedBackgroundColor = defaults.ListViewItemSelectedBackgroundColor;
		profile.ListViewItemSelectedForegroundColor = defaults.ListViewItemSelectedForegroundColor;
		profile.ThemeAccentColor = defaults.ThemeAccentColor;
		profile.HighlightColor = defaults.HighlightColor;
		profile.ControlPointerOverBackgroundColor = defaults.ControlPointerOverBackgroundColor;
		profile.ControlPressedBackgroundColor = defaults.ControlPressedBackgroundColor;
		profile.ControlPointerOverBorderColor = defaults.ControlPointerOverBorderColor;
		profile.ControlPressedBorderColor = defaults.ControlPressedBorderColor;
		profile.SetupTitleFontSize = defaults.SetupTitleFontSize;
		profile.SetupSubtitleFontSize = defaults.SetupSubtitleFontSize;
		profile.SetupPrimaryActionFontSize = defaults.SetupPrimaryActionFontSize;

		if (ActiveThemeProfileName == profile.Name) {
			ApplyThemeProfile(profile);
		}

		return true;
	}

	public bool ApplyThemePresetToProfile(string? profileName, NexenTheme theme) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return false;
		}

		ThemeProfile preset = ThemeProfile.CreateDefault(profile.Name, theme);
		profile.Theme = preset.Theme;
		profile.UiFontFamily = preset.UiFontFamily;
		profile.UiFontSize = preset.UiFontSize;
		profile.MenuFontFamily = preset.MenuFontFamily;
		profile.MenuFontSize = preset.MenuFontSize;
		profile.StartupWindowBackgroundColor = preset.StartupWindowBackgroundColor;
		profile.StartupSurfaceBackgroundColor = preset.StartupSurfaceBackgroundColor;
		profile.StartupBorderColor = preset.StartupBorderColor;
		profile.StartupCardBackgroundColor = preset.StartupCardBackgroundColor;
		profile.StartupCardCheckedBackgroundColor = preset.StartupCardCheckedBackgroundColor;
		profile.StartupTextColor = preset.StartupTextColor;
		profile.StartupMutedTextColor = preset.StartupMutedTextColor;
		profile.StartupActionBorderColor = preset.StartupActionBorderColor;
		profile.StartupPrimaryActionColor = preset.StartupPrimaryActionColor;
		profile.StartupDividerColor = preset.StartupDividerColor;
		profile.SetupSubtitleColor = preset.SetupSubtitleColor;
		profile.MenuBackgroundColor = preset.MenuBackgroundColor;
		profile.MenuBackgroundHighlightColor = preset.MenuBackgroundHighlightColor;
		profile.MenuBorderHighlightColor = preset.MenuBorderHighlightColor;
		profile.MenuSeparatorBackgroundColor = preset.MenuSeparatorBackgroundColor;
		profile.SettingsTabStripBackgroundColor = preset.SettingsTabStripBackgroundColor;
		profile.SettingsTabStripBorderColor = preset.SettingsTabStripBorderColor;
		profile.DockTabStripBackgroundColor = preset.DockTabStripBackgroundColor;
		profile.DockTabPointerOverColor = preset.DockTabPointerOverColor;
		profile.DockTabActiveBackgroundColor = preset.DockTabActiveBackgroundColor;
		profile.DockTabActiveBorderColor = preset.DockTabActiveBorderColor;
		profile.CheckBoxPointerOverBorderColor = preset.CheckBoxPointerOverBorderColor;
		profile.CheckBoxPressedBackgroundColor = preset.CheckBoxPressedBackgroundColor;
		profile.CheckBoxPressedBorderColor = preset.CheckBoxPressedBorderColor;
		profile.RadioButtonPointerOverBorderColor = preset.RadioButtonPointerOverBorderColor;
		profile.RadioButtonPressedBackgroundColor = preset.RadioButtonPressedBackgroundColor;
		profile.RadioButtonPressedBorderColor = preset.RadioButtonPressedBorderColor;
		profile.SliderTrackPointerOverColor = preset.SliderTrackPointerOverColor;
		profile.SliderTrackPressedColor = preset.SliderTrackPressedColor;
		profile.TextBoxSelectionColor = preset.TextBoxSelectionColor;
		profile.TextControlDisabledBackgroundColor = preset.TextControlDisabledBackgroundColor;
		profile.ToolTipBackgroundColor = preset.ToolTipBackgroundColor;
		profile.MenuFlyoutBackgroundColor = preset.MenuFlyoutBackgroundColor;
		profile.MenuFlyoutBorderColor = preset.MenuFlyoutBorderColor;
		profile.ComboBoxDropDownBackgroundColor = preset.ComboBoxDropDownBackgroundColor;
		profile.ComboBoxDropDownBorderColor = preset.ComboBoxDropDownBorderColor;
		profile.DataGridHeaderBackgroundColor = preset.DataGridHeaderBackgroundColor;
		profile.DataGridHeaderForegroundColor = preset.DataGridHeaderForegroundColor;
		profile.DataGridSelectedRowForegroundColor = preset.DataGridSelectedRowForegroundColor;
		profile.ListBoxItemHoverBackgroundColor = preset.ListBoxItemHoverBackgroundColor;
		profile.ListBoxItemSelectedBackgroundColor = preset.ListBoxItemSelectedBackgroundColor;
		profile.ListBoxItemSelectedForegroundColor = preset.ListBoxItemSelectedForegroundColor;
		profile.TreeViewItemHoverBackgroundColor = preset.TreeViewItemHoverBackgroundColor;
		profile.TreeViewItemSelectedBackgroundColor = preset.TreeViewItemSelectedBackgroundColor;
		profile.TreeViewItemSelectedForegroundColor = preset.TreeViewItemSelectedForegroundColor;
		profile.NavigationViewItemHoverBackgroundColor = preset.NavigationViewItemHoverBackgroundColor;
		profile.NavigationViewItemSelectedBackgroundColor = preset.NavigationViewItemSelectedBackgroundColor;
		profile.NavigationViewItemSelectedForegroundColor = preset.NavigationViewItemSelectedForegroundColor;
		profile.ListViewItemHoverBackgroundColor = preset.ListViewItemHoverBackgroundColor;
		profile.ListViewItemSelectedBackgroundColor = preset.ListViewItemSelectedBackgroundColor;
		profile.ListViewItemSelectedForegroundColor = preset.ListViewItemSelectedForegroundColor;
		profile.ThemeAccentColor = preset.ThemeAccentColor;
		profile.HighlightColor = preset.HighlightColor;
		profile.ControlPointerOverBackgroundColor = preset.ControlPointerOverBackgroundColor;
		profile.ControlPressedBackgroundColor = preset.ControlPressedBackgroundColor;
		profile.ControlPointerOverBorderColor = preset.ControlPointerOverBorderColor;
		profile.ControlPressedBorderColor = preset.ControlPressedBorderColor;
		profile.SetupTitleFontSize = preset.SetupTitleFontSize;
		profile.SetupSubtitleFontSize = preset.SetupSubtitleFontSize;
		profile.SetupPrimaryActionFontSize = preset.SetupPrimaryActionFontSize;

		if (ActiveThemeProfileName == profile.Name) {
			ApplyThemeProfile(profile);
		}

		return true;
	}

	public int GetThemeProfileDivergenceCount(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return 0;
		}

		ThemeProfile defaults = ThemeProfile.CreateDefault(profile.Name, profile.Theme);
		int divergenceCount = 0;

		if (profile.UiFontFamily != defaults.UiFontFamily) divergenceCount++;
		if (profile.UiFontSize != defaults.UiFontSize) divergenceCount++;
		if (profile.MenuFontFamily != defaults.MenuFontFamily) divergenceCount++;
		if (profile.MenuFontSize != defaults.MenuFontSize) divergenceCount++;
		if (profile.StartupWindowBackgroundColor != defaults.StartupWindowBackgroundColor) divergenceCount++;
		if (profile.StartupSurfaceBackgroundColor != defaults.StartupSurfaceBackgroundColor) divergenceCount++;
		if (profile.StartupBorderColor != defaults.StartupBorderColor) divergenceCount++;
		if (profile.StartupCardBackgroundColor != defaults.StartupCardBackgroundColor) divergenceCount++;
		if (profile.StartupCardCheckedBackgroundColor != defaults.StartupCardCheckedBackgroundColor) divergenceCount++;
		if (profile.StartupTextColor != defaults.StartupTextColor) divergenceCount++;
		if (profile.StartupMutedTextColor != defaults.StartupMutedTextColor) divergenceCount++;
		if (profile.StartupActionBorderColor != defaults.StartupActionBorderColor) divergenceCount++;
		if (profile.StartupPrimaryActionColor != defaults.StartupPrimaryActionColor) divergenceCount++;
		if (profile.StartupDividerColor != defaults.StartupDividerColor) divergenceCount++;
		if (profile.SetupSubtitleColor != defaults.SetupSubtitleColor) divergenceCount++;
		if (profile.MenuBackgroundColor != defaults.MenuBackgroundColor) divergenceCount++;
		if (profile.MenuBackgroundHighlightColor != defaults.MenuBackgroundHighlightColor) divergenceCount++;
		if (profile.MenuBorderHighlightColor != defaults.MenuBorderHighlightColor) divergenceCount++;
		if (profile.MenuSeparatorBackgroundColor != defaults.MenuSeparatorBackgroundColor) divergenceCount++;
		if (profile.SettingsTabStripBackgroundColor != defaults.SettingsTabStripBackgroundColor) divergenceCount++;
		if (profile.SettingsTabStripBorderColor != defaults.SettingsTabStripBorderColor) divergenceCount++;
		if (profile.DockTabStripBackgroundColor != defaults.DockTabStripBackgroundColor) divergenceCount++;
		if (profile.DockTabPointerOverColor != defaults.DockTabPointerOverColor) divergenceCount++;
		if (profile.DockTabActiveBackgroundColor != defaults.DockTabActiveBackgroundColor) divergenceCount++;
		if (profile.DockTabActiveBorderColor != defaults.DockTabActiveBorderColor) divergenceCount++;
		if (profile.CheckBoxPointerOverBorderColor != defaults.CheckBoxPointerOverBorderColor) divergenceCount++;
		if (profile.CheckBoxPressedBackgroundColor != defaults.CheckBoxPressedBackgroundColor) divergenceCount++;
		if (profile.CheckBoxPressedBorderColor != defaults.CheckBoxPressedBorderColor) divergenceCount++;
		if (profile.RadioButtonPointerOverBorderColor != defaults.RadioButtonPointerOverBorderColor) divergenceCount++;
		if (profile.RadioButtonPressedBackgroundColor != defaults.RadioButtonPressedBackgroundColor) divergenceCount++;
		if (profile.RadioButtonPressedBorderColor != defaults.RadioButtonPressedBorderColor) divergenceCount++;
		if (profile.SliderTrackPointerOverColor != defaults.SliderTrackPointerOverColor) divergenceCount++;
		if (profile.SliderTrackPressedColor != defaults.SliderTrackPressedColor) divergenceCount++;
		if (profile.TextBoxSelectionColor != defaults.TextBoxSelectionColor) divergenceCount++;
		if (profile.TextControlDisabledBackgroundColor != defaults.TextControlDisabledBackgroundColor) divergenceCount++;
		if (profile.ToolTipBackgroundColor != defaults.ToolTipBackgroundColor) divergenceCount++;
		if (profile.MenuFlyoutBackgroundColor != defaults.MenuFlyoutBackgroundColor) divergenceCount++;
		if (profile.MenuFlyoutBorderColor != defaults.MenuFlyoutBorderColor) divergenceCount++;
		if (profile.ComboBoxDropDownBackgroundColor != defaults.ComboBoxDropDownBackgroundColor) divergenceCount++;
		if (profile.ComboBoxDropDownBorderColor != defaults.ComboBoxDropDownBorderColor) divergenceCount++;
		if (profile.DataGridHeaderBackgroundColor != defaults.DataGridHeaderBackgroundColor) divergenceCount++;
		if (profile.DataGridHeaderForegroundColor != defaults.DataGridHeaderForegroundColor) divergenceCount++;
		if (profile.DataGridSelectedRowForegroundColor != defaults.DataGridSelectedRowForegroundColor) divergenceCount++;
		if (profile.ListBoxItemHoverBackgroundColor != defaults.ListBoxItemHoverBackgroundColor) divergenceCount++;
		if (profile.ListBoxItemSelectedBackgroundColor != defaults.ListBoxItemSelectedBackgroundColor) divergenceCount++;
		if (profile.ListBoxItemSelectedForegroundColor != defaults.ListBoxItemSelectedForegroundColor) divergenceCount++;
		if (profile.TreeViewItemHoverBackgroundColor != defaults.TreeViewItemHoverBackgroundColor) divergenceCount++;
		if (profile.TreeViewItemSelectedBackgroundColor != defaults.TreeViewItemSelectedBackgroundColor) divergenceCount++;
		if (profile.TreeViewItemSelectedForegroundColor != defaults.TreeViewItemSelectedForegroundColor) divergenceCount++;
		if (profile.NavigationViewItemHoverBackgroundColor != defaults.NavigationViewItemHoverBackgroundColor) divergenceCount++;
		if (profile.NavigationViewItemSelectedBackgroundColor != defaults.NavigationViewItemSelectedBackgroundColor) divergenceCount++;
		if (profile.NavigationViewItemSelectedForegroundColor != defaults.NavigationViewItemSelectedForegroundColor) divergenceCount++;
		if (profile.ListViewItemHoverBackgroundColor != defaults.ListViewItemHoverBackgroundColor) divergenceCount++;
		if (profile.ListViewItemSelectedBackgroundColor != defaults.ListViewItemSelectedBackgroundColor) divergenceCount++;
		if (profile.ListViewItemSelectedForegroundColor != defaults.ListViewItemSelectedForegroundColor) divergenceCount++;
		if (profile.ThemeAccentColor != defaults.ThemeAccentColor) divergenceCount++;
		if (profile.HighlightColor != defaults.HighlightColor) divergenceCount++;
		if (profile.ControlPointerOverBackgroundColor != defaults.ControlPointerOverBackgroundColor) divergenceCount++;
		if (profile.ControlPressedBackgroundColor != defaults.ControlPressedBackgroundColor) divergenceCount++;
		if (profile.ControlPointerOverBorderColor != defaults.ControlPointerOverBorderColor) divergenceCount++;
		if (profile.ControlPressedBorderColor != defaults.ControlPressedBorderColor) divergenceCount++;
		if (profile.SetupTitleFontSize != defaults.SetupTitleFontSize) divergenceCount++;
		if (profile.SetupSubtitleFontSize != defaults.SetupSubtitleFontSize) divergenceCount++;
		if (profile.SetupPrimaryActionFontSize != defaults.SetupPrimaryActionFontSize) divergenceCount++;

		return divergenceCount;
	}

	public List<string> GetThemeProfileCustomizedTokenNames(string? profileName) {
		ThemeProfile? profile = GetThemeProfileByName(profileName);
		if (profile is null) {
			return [];
		}

		ThemeProfile defaults = ThemeProfile.CreateDefault(profile.Name, profile.Theme);
		List<string> customized = [];

		if (profile.UiFontFamily != defaults.UiFontFamily) customized.Add(nameof(ThemeProfile.UiFontFamily));
		if (profile.UiFontSize != defaults.UiFontSize) customized.Add(nameof(ThemeProfile.UiFontSize));
		if (profile.MenuFontFamily != defaults.MenuFontFamily) customized.Add(nameof(ThemeProfile.MenuFontFamily));
		if (profile.MenuFontSize != defaults.MenuFontSize) customized.Add(nameof(ThemeProfile.MenuFontSize));
		if (profile.StartupWindowBackgroundColor != defaults.StartupWindowBackgroundColor) customized.Add(nameof(ThemeProfile.StartupWindowBackgroundColor));
		if (profile.StartupSurfaceBackgroundColor != defaults.StartupSurfaceBackgroundColor) customized.Add(nameof(ThemeProfile.StartupSurfaceBackgroundColor));
		if (profile.StartupBorderColor != defaults.StartupBorderColor) customized.Add(nameof(ThemeProfile.StartupBorderColor));
		if (profile.StartupCardBackgroundColor != defaults.StartupCardBackgroundColor) customized.Add(nameof(ThemeProfile.StartupCardBackgroundColor));
		if (profile.StartupCardCheckedBackgroundColor != defaults.StartupCardCheckedBackgroundColor) customized.Add(nameof(ThemeProfile.StartupCardCheckedBackgroundColor));
		if (profile.StartupTextColor != defaults.StartupTextColor) customized.Add(nameof(ThemeProfile.StartupTextColor));
		if (profile.StartupMutedTextColor != defaults.StartupMutedTextColor) customized.Add(nameof(ThemeProfile.StartupMutedTextColor));
		if (profile.StartupActionBorderColor != defaults.StartupActionBorderColor) customized.Add(nameof(ThemeProfile.StartupActionBorderColor));
		if (profile.StartupPrimaryActionColor != defaults.StartupPrimaryActionColor) customized.Add(nameof(ThemeProfile.StartupPrimaryActionColor));
		if (profile.StartupDividerColor != defaults.StartupDividerColor) customized.Add(nameof(ThemeProfile.StartupDividerColor));
		if (profile.SetupSubtitleColor != defaults.SetupSubtitleColor) customized.Add(nameof(ThemeProfile.SetupSubtitleColor));
		if (profile.MenuBackgroundColor != defaults.MenuBackgroundColor) customized.Add(nameof(ThemeProfile.MenuBackgroundColor));
		if (profile.MenuBackgroundHighlightColor != defaults.MenuBackgroundHighlightColor) customized.Add(nameof(ThemeProfile.MenuBackgroundHighlightColor));
		if (profile.MenuBorderHighlightColor != defaults.MenuBorderHighlightColor) customized.Add(nameof(ThemeProfile.MenuBorderHighlightColor));
		if (profile.MenuSeparatorBackgroundColor != defaults.MenuSeparatorBackgroundColor) customized.Add(nameof(ThemeProfile.MenuSeparatorBackgroundColor));
		if (profile.SettingsTabStripBackgroundColor != defaults.SettingsTabStripBackgroundColor) customized.Add(nameof(ThemeProfile.SettingsTabStripBackgroundColor));
		if (profile.SettingsTabStripBorderColor != defaults.SettingsTabStripBorderColor) customized.Add(nameof(ThemeProfile.SettingsTabStripBorderColor));
		if (profile.DockTabStripBackgroundColor != defaults.DockTabStripBackgroundColor) customized.Add(nameof(ThemeProfile.DockTabStripBackgroundColor));
		if (profile.DockTabPointerOverColor != defaults.DockTabPointerOverColor) customized.Add(nameof(ThemeProfile.DockTabPointerOverColor));
		if (profile.DockTabActiveBackgroundColor != defaults.DockTabActiveBackgroundColor) customized.Add(nameof(ThemeProfile.DockTabActiveBackgroundColor));
		if (profile.DockTabActiveBorderColor != defaults.DockTabActiveBorderColor) customized.Add(nameof(ThemeProfile.DockTabActiveBorderColor));
		if (profile.CheckBoxPointerOverBorderColor != defaults.CheckBoxPointerOverBorderColor) customized.Add(nameof(ThemeProfile.CheckBoxPointerOverBorderColor));
		if (profile.CheckBoxPressedBackgroundColor != defaults.CheckBoxPressedBackgroundColor) customized.Add(nameof(ThemeProfile.CheckBoxPressedBackgroundColor));
		if (profile.CheckBoxPressedBorderColor != defaults.CheckBoxPressedBorderColor) customized.Add(nameof(ThemeProfile.CheckBoxPressedBorderColor));
		if (profile.RadioButtonPointerOverBorderColor != defaults.RadioButtonPointerOverBorderColor) customized.Add(nameof(ThemeProfile.RadioButtonPointerOverBorderColor));
		if (profile.RadioButtonPressedBackgroundColor != defaults.RadioButtonPressedBackgroundColor) customized.Add(nameof(ThemeProfile.RadioButtonPressedBackgroundColor));
		if (profile.RadioButtonPressedBorderColor != defaults.RadioButtonPressedBorderColor) customized.Add(nameof(ThemeProfile.RadioButtonPressedBorderColor));
		if (profile.SliderTrackPointerOverColor != defaults.SliderTrackPointerOverColor) customized.Add(nameof(ThemeProfile.SliderTrackPointerOverColor));
		if (profile.SliderTrackPressedColor != defaults.SliderTrackPressedColor) customized.Add(nameof(ThemeProfile.SliderTrackPressedColor));
		if (profile.TextBoxSelectionColor != defaults.TextBoxSelectionColor) customized.Add(nameof(ThemeProfile.TextBoxSelectionColor));
		if (profile.TextControlDisabledBackgroundColor != defaults.TextControlDisabledBackgroundColor) customized.Add(nameof(ThemeProfile.TextControlDisabledBackgroundColor));
		if (profile.ToolTipBackgroundColor != defaults.ToolTipBackgroundColor) customized.Add(nameof(ThemeProfile.ToolTipBackgroundColor));
		if (profile.MenuFlyoutBackgroundColor != defaults.MenuFlyoutBackgroundColor) customized.Add(nameof(ThemeProfile.MenuFlyoutBackgroundColor));
		if (profile.MenuFlyoutBorderColor != defaults.MenuFlyoutBorderColor) customized.Add(nameof(ThemeProfile.MenuFlyoutBorderColor));
		if (profile.ComboBoxDropDownBackgroundColor != defaults.ComboBoxDropDownBackgroundColor) customized.Add(nameof(ThemeProfile.ComboBoxDropDownBackgroundColor));
		if (profile.ComboBoxDropDownBorderColor != defaults.ComboBoxDropDownBorderColor) customized.Add(nameof(ThemeProfile.ComboBoxDropDownBorderColor));
		if (profile.DataGridHeaderBackgroundColor != defaults.DataGridHeaderBackgroundColor) customized.Add(nameof(ThemeProfile.DataGridHeaderBackgroundColor));
		if (profile.DataGridHeaderForegroundColor != defaults.DataGridHeaderForegroundColor) customized.Add(nameof(ThemeProfile.DataGridHeaderForegroundColor));
		if (profile.DataGridSelectedRowForegroundColor != defaults.DataGridSelectedRowForegroundColor) customized.Add(nameof(ThemeProfile.DataGridSelectedRowForegroundColor));
		if (profile.ListBoxItemHoverBackgroundColor != defaults.ListBoxItemHoverBackgroundColor) customized.Add(nameof(ThemeProfile.ListBoxItemHoverBackgroundColor));
		if (profile.ListBoxItemSelectedBackgroundColor != defaults.ListBoxItemSelectedBackgroundColor) customized.Add(nameof(ThemeProfile.ListBoxItemSelectedBackgroundColor));
		if (profile.ListBoxItemSelectedForegroundColor != defaults.ListBoxItemSelectedForegroundColor) customized.Add(nameof(ThemeProfile.ListBoxItemSelectedForegroundColor));
		if (profile.TreeViewItemHoverBackgroundColor != defaults.TreeViewItemHoverBackgroundColor) customized.Add(nameof(ThemeProfile.TreeViewItemHoverBackgroundColor));
		if (profile.TreeViewItemSelectedBackgroundColor != defaults.TreeViewItemSelectedBackgroundColor) customized.Add(nameof(ThemeProfile.TreeViewItemSelectedBackgroundColor));
		if (profile.TreeViewItemSelectedForegroundColor != defaults.TreeViewItemSelectedForegroundColor) customized.Add(nameof(ThemeProfile.TreeViewItemSelectedForegroundColor));
		if (profile.NavigationViewItemHoverBackgroundColor != defaults.NavigationViewItemHoverBackgroundColor) customized.Add(nameof(ThemeProfile.NavigationViewItemHoverBackgroundColor));
		if (profile.NavigationViewItemSelectedBackgroundColor != defaults.NavigationViewItemSelectedBackgroundColor) customized.Add(nameof(ThemeProfile.NavigationViewItemSelectedBackgroundColor));
		if (profile.NavigationViewItemSelectedForegroundColor != defaults.NavigationViewItemSelectedForegroundColor) customized.Add(nameof(ThemeProfile.NavigationViewItemSelectedForegroundColor));
		if (profile.ListViewItemHoverBackgroundColor != defaults.ListViewItemHoverBackgroundColor) customized.Add(nameof(ThemeProfile.ListViewItemHoverBackgroundColor));
		if (profile.ListViewItemSelectedBackgroundColor != defaults.ListViewItemSelectedBackgroundColor) customized.Add(nameof(ThemeProfile.ListViewItemSelectedBackgroundColor));
		if (profile.ListViewItemSelectedForegroundColor != defaults.ListViewItemSelectedForegroundColor) customized.Add(nameof(ThemeProfile.ListViewItemSelectedForegroundColor));
		if (profile.ThemeAccentColor != defaults.ThemeAccentColor) customized.Add(nameof(ThemeProfile.ThemeAccentColor));
		if (profile.HighlightColor != defaults.HighlightColor) customized.Add(nameof(ThemeProfile.HighlightColor));
		if (profile.ControlPointerOverBackgroundColor != defaults.ControlPointerOverBackgroundColor) customized.Add(nameof(ThemeProfile.ControlPointerOverBackgroundColor));
		if (profile.ControlPressedBackgroundColor != defaults.ControlPressedBackgroundColor) customized.Add(nameof(ThemeProfile.ControlPressedBackgroundColor));
		if (profile.ControlPointerOverBorderColor != defaults.ControlPointerOverBorderColor) customized.Add(nameof(ThemeProfile.ControlPointerOverBorderColor));
		if (profile.ControlPressedBorderColor != defaults.ControlPressedBorderColor) customized.Add(nameof(ThemeProfile.ControlPressedBorderColor));
		if (profile.SetupTitleFontSize != defaults.SetupTitleFontSize) customized.Add(nameof(ThemeProfile.SetupTitleFontSize));
		if (profile.SetupSubtitleFontSize != defaults.SetupSubtitleFontSize) customized.Add(nameof(ThemeProfile.SetupSubtitleFontSize));
		if (profile.SetupPrimaryActionFontSize != defaults.SetupPrimaryActionFontSize) customized.Add(nameof(ThemeProfile.SetupPrimaryActionFontSize));

		return customized;
	}

	private void AddShortcut(ShortcutKeyInfo shortcut) {
		if (!ShortcutKeys.Exists(a => a.Shortcut == shortcut.Shortcut)) {
			ShortcutKeys.Add(shortcut);
		}
	}

	public void InitializeDefaultShortcuts() {
		UInt16 ctrl = InputApi.GetKeyCode("Left Ctrl");
		UInt16 alt = InputApi.GetKeyCode("Left Alt");
		UInt16 shift = InputApi.GetKeyCode("Left Shift");

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.FastForward, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Tab") }, KeyCombination2 = new KeyCombination() { Key1 = InputApi.GetKeyCode("Pad1 R2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Rewind, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Backspace") }, KeyCombination2 = new KeyCombination() { Key1 = InputApi.GetKeyCode("Pad1 L2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.IncreaseSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("=") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.DecreaseSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("-") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.MaxSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F9") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.IncreaseVolume, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("=") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.DecreaseVolume, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("-") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ToggleFps, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F10") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ToggleFullscreen, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F11") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.TakeScreenshot, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F12") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Reset, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("R") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.PowerCycle, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("T") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ReloadRom, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = shift, Key3 = InputApi.GetKeyCode("R") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Pause, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Esc") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.RunSingleFrame, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("`") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale1x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale2x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale3x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale4x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("4") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale5x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("5") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale6x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("6") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale7x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("7") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale8x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("8") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale9x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("9") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale10x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("0") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.OpenFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("O") } });

		// Infinite save states - F1/Ctrl+S creates new save, Shift+F1 opens picker
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.OpenSaveStatePicker, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("S") } });

		// Save/Load state to/from file dialog
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveStateToFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = shift, Key3 = InputApi.GetKeyCode("S") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadStateFromFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("L") } });

		// Designated slots - F2-F4 for 3 quick save/load slots (F = save, Shift+F = load, matching F1 pattern)
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot2, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot2, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot3, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F4") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot3, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F4") } });

		foreach (EmulatorShortcut value in Enum.GetValues<EmulatorShortcut>()) {
			if (value < EmulatorShortcut.LastValidValue) {
				AddShortcut(new ShortcutKeyInfo { Shortcut = value });
			}
		}
	}

	public void UpdateFileAssociations() {
		FileAssociationHelper.UpdateFileAssociations();
	}

	public void ApplyFontOptions() {
		UpdateFonts();
	}

	private void UpdateFonts() {
		if (Application.Current is not null) {
			string nexenFontFamily = Configuration.GetValidFontFamily(NexenFont.FontFamily, false);
			string menuFont = Configuration.GetValidFontFamily(NexenMenuFont.FontFamily, false);

			if (Application.Current.Resources["NexenFont"] is FontFamily curNexenFont && curNexenFont.Name != nexenFontFamily) {
				Application.Current.Resources["NexenFont"] = new FontFamily(nexenFontFamily);
			}

			if (Application.Current.Resources["NexenMenuFont"] is FontFamily curNexenMenuFont && curNexenMenuFont.Name != menuFont) {
				Application.Current.Resources["NexenMenuFont"] = new FontFamily(menuFont);
			}

			if (Application.Current.Resources["NexenFontSize"] is double curNexenFontSize && curNexenFontSize != NexenFont.FontSize) {
				Application.Current.Resources["NexenFontSize"] = (double)NexenFont.FontSize;
			}

			if (Application.Current.Resources["NexenMenuFontSize"] is double curNexenMenuFontSize && curNexenMenuFontSize != NexenMenuFont.FontSize) {
				Application.Current.Resources["NexenMenuFontSize"] = (double)NexenMenuFont.FontSize;
			}
		}
	}

	private static string TryReadResourceColor(string resourceKey, string fallback) {
		if (Application.Current?.Resources is null) {
			return fallback;
		}

		if (Application.Current.Resources.TryGetValue(resourceKey, out object? resource) && resource is Color color) {
			return "#" + color.A.ToString("x2") + color.R.ToString("x2") + color.G.ToString("x2") + color.B.ToString("x2");
		}

		return fallback;
	}

	private static string TryReadResourceBrushColor(string resourceKey, string fallback) {
		if (Application.Current?.Resources is null) {
			return fallback;
		}

		if (Application.Current.Resources.TryGetValue(resourceKey, out object? resource) && resource is ISolidColorBrush brush) {
			Color color = brush.Color;
			return "#" + color.A.ToString("x2") + color.R.ToString("x2") + color.G.ToString("x2") + color.B.ToString("x2");
		}

		return fallback;
	}

	public void InitializeFontDefaults() {
		NexenFont = Configuration.GetDefaultFont();
		NexenMenuFont = Configuration.GetDefaultMenuFont();
		ApplyFontOptions();
	}

	public static void UpdateTheme() {
		if (Application.Current is not null) {
			ConfigManager.Config.Preferences.EnsureThemeProfiles();
			NexenThemeManager.ApplyTheme(Application.Current, ConfigManager.Config.Preferences);
		}
	}

	public static string GetLanguageCode(UiLanguage language) {
		return language switch {
			UiLanguage.English => "en",
			UiLanguage.Spanish => "es",
			UiLanguage.Japanese => "ja",
			_ => "en"
		};
	}

	public void ApplyConfig() {
		UpdateFonts();

		List<InteropShortcutKeyInfo> shortcutKeys = [];
		foreach (ShortcutKeyInfo shortcutInfo in ShortcutKeys) {
			if (!shortcutInfo.KeyCombination.IsEmpty) {
				shortcutKeys.Add(new InteropShortcutKeyInfo(shortcutInfo.Shortcut, shortcutInfo.KeyCombination.ToInterop()));
			}

			if (!shortcutInfo.KeyCombination2.IsEmpty) {
				shortcutKeys.Add(new InteropShortcutKeyInfo(shortcutInfo.Shortcut, shortcutInfo.KeyCombination2.ToInterop()));
			}
		}

		ConfigApi.SetShortcutKeys(shortcutKeys.ToArray(), (UInt32)shortcutKeys.Count);

		ConfigApi.SetPreferences(new InteropPreferencesConfig() {
			ShowFps = ShowFps,
			ShowFrameCounter = ShowFrameCounter,
			ShowGameTimer = ShowGameTimer,
			ShowDebugInfo = ShowDebugInfo,
			ShowLagCounter = ShowLagCounter,
			ShowRerecordCounter = ShowRerecordCounter,
			ShowLagFrameIndicator = ShowLagFrameIndicator,
			DisableOsd = DisableOsd,
			AllowBackgroundInput = AllowBackgroundInput,
			PauseOnMovieEnd = PauseOnMovieEnd,
			ShowMovieIcons = ShowMovieIcons,
			ShowTurboRewindIcons = ShowTurboRewindIcons,
			DisableGameSelectionScreen = GameSelectionScreenMode == GameSelectionMode.Disabled,
			HudSize = HudSize,
			SaveFolderOverride = OverrideSaveDataFolder ? SaveDataFolder : "",
			SaveStateFolderOverride = OverrideSaveStateFolder ? SaveStateFolder : "",
			ScreenshotFolderOverride = OverrideScreenshotFolder ? ScreenshotFolder : "",
			RewindBufferSize = EnableRewind ? RewindBufferSize : 0,
			AutoSaveStateDelay = EnableAutoSaveState ? AutoSaveStateDelay : 0,
			ShowAutoSaveNotifications = ShowAutoSaveNotifications,
			ShowRecentPlayNotifications = ShowRecentPlayNotifications
		});
	}
}

public enum NexenTheme {
	Light = 0,
	Dark = 1
}

public enum PrimaryUsageProfile {
	Playing = 0,
	Debugging = 1
}

public enum UiLanguage {
	English = 0,
	Spanish = 1,
	Japanese = 2
}

public enum FontAntialiasing {
	Disabled,
	Antialias,
	SubPixelAntialias
}

public enum GameSelectionMode {
	Disabled,
	ResumeState,
	PowerOn
}

public enum HudDisplaySize {
	Fixed,
	Scaled,
}

public struct InteropPreferencesConfig {
	[MarshalAs(UnmanagedType.I1)] public bool ShowFps;
	[MarshalAs(UnmanagedType.I1)] public bool ShowFrameCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowGameTimer;
	[MarshalAs(UnmanagedType.I1)] public bool ShowLagCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowRerecordCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowLagFrameIndicator;
	[MarshalAs(UnmanagedType.I1)] public bool ShowDebugInfo;
	[MarshalAs(UnmanagedType.I1)] public bool DisableOsd;
	[MarshalAs(UnmanagedType.I1)] public bool AllowBackgroundInput;
	[MarshalAs(UnmanagedType.I1)] public bool PauseOnMovieEnd;
	[MarshalAs(UnmanagedType.I1)] public bool ShowMovieIcons;
	[MarshalAs(UnmanagedType.I1)] public bool ShowTurboRewindIcons;
	[MarshalAs(UnmanagedType.I1)] public bool DisableGameSelectionScreen;

	public HudDisplaySize HudSize;

	public UInt32 AutoSaveStateDelay;
	[MarshalAs(UnmanagedType.I1)] public bool ShowAutoSaveNotifications;
	[MarshalAs(UnmanagedType.I1)] public bool ShowRecentPlayNotifications;
	public UInt32 RewindBufferSize;

	public string SaveFolderOverride;
	public string SaveStateFolderOverride;
	public string ScreenshotFolderOverride;
}
