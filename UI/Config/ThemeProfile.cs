using System;
using Avalonia.Media;

namespace Nexen.Config;

public sealed class ThemeProfile {
	public string Name { get; set; } = "Custom Theme";
	public NexenTheme Theme { get; set; } = NexenTheme.Light;
	public string UiFontFamily { get; set; } = "Microsoft Sans Serif";
	public double UiFontSize { get; set; } = 11;
	public string MenuFontFamily { get; set; } = "Segoe UI";
	public double MenuFontSize { get; set; } = 12;
	public string StartupWindowBackgroundColor { get; set; } = "#ff1d130d";
	public string StartupSurfaceBackgroundColor { get; set; } = "#ff2a1a10";
	public string StartupBorderColor { get; set; } = "#ff6b3f24";
	public string StartupCardBackgroundColor { get; set; } = "#ff2a1a10";
	public string StartupCardCheckedBackgroundColor { get; set; } = "#ffb65e18";
	public string StartupTextColor { get; set; } = "#ffffffff";
	public string StartupMutedTextColor { get; set; } = "#ffefcfb6";
	public string StartupActionBorderColor { get; set; } = "#ffffce87";
	public string StartupPrimaryActionColor { get; set; } = "#ffd47a22";
	public string StartupDividerColor { get; set; } = "#553f2b1f";
	public string SetupSubtitleColor { get; set; } = "#ffefcfb6";
	public string MenuBackgroundColor { get; set; } = "#55281a10";
	public string MenuBackgroundHighlightColor { get; set; } = "#80573721";
	public string MenuBorderHighlightColor { get; set; } = "#e09a52";
	public string MenuSeparatorBackgroundColor { get; set; } = "#66c2864f";
	public string SettingsTabStripBackgroundColor { get; set; } = "#2f2015";
	public string SettingsTabStripBorderColor { get; set; } = "#80808080";
	public string DockTabStripBackgroundColor { get; set; } = "#ff282828";
	public string DockTabPointerOverColor { get; set; } = "#40808080";
	public string DockTabActiveBackgroundColor { get; set; } = "#ff282828";
	public string DockTabActiveBorderColor { get; set; } = "#ff505050";
	public string CheckBoxPointerOverBorderColor { get; set; } = "#e09a52";
	public string CheckBoxPressedBackgroundColor { get; set; } = "#9960382a";
	public string CheckBoxPressedBorderColor { get; set; } = "#c27b38";
	public string RadioButtonPointerOverBorderColor { get; set; } = "#e09a52";
	public string RadioButtonPressedBackgroundColor { get; set; } = "#9960382a";
	public string RadioButtonPressedBorderColor { get; set; } = "#c27b38";
	public string SliderTrackPointerOverColor { get; set; } = "#e09a52";
	public string SliderTrackPressedColor { get; set; } = "#c27b38";
	public string TextBoxSelectionColor { get; set; } = "#2a509f";
	public string TextControlDisabledBackgroundColor { get; set; } = "#ff333333";
	public string ToolTipBackgroundColor { get; set; } = "#181818";
	public string MenuFlyoutBackgroundColor { get; set; } = "#ff2a1a10";
	public string MenuFlyoutBorderColor { get; set; } = "#a87343";
	public string ComboBoxDropDownBackgroundColor { get; set; } = "#ff2a2a2a";
	public string ComboBoxDropDownBorderColor { get; set; } = "#ff909090";
	public string DataGridHeaderBackgroundColor { get; set; } = "#ff303030";
	public string DataGridHeaderForegroundColor { get; set; } = "#ffdedede";
	public string DataGridSelectedRowForegroundColor { get; set; } = "#ffffffff";
	public string ListBoxItemHoverBackgroundColor { get; set; } = "#66403020";
	public string ListBoxItemSelectedBackgroundColor { get; set; } = "#ff6a3f20";
	public string ListBoxItemSelectedForegroundColor { get; set; } = "#ffffffff";
	public string TreeViewItemHoverBackgroundColor { get; set; } = "#663d2a1b";
	public string TreeViewItemSelectedBackgroundColor { get; set; } = "#ff6a3f20";
	public string TreeViewItemSelectedForegroundColor { get; set; } = "#ffffffff";
	public string NavigationViewItemHoverBackgroundColor { get; set; } = "#66403020";
	public string NavigationViewItemSelectedBackgroundColor { get; set; } = "#ff6a3f20";
	public string NavigationViewItemSelectedForegroundColor { get; set; } = "#ffffffff";
	public string ListViewItemHoverBackgroundColor { get; set; } = "#66403020";
	public string ListViewItemSelectedBackgroundColor { get; set; } = "#ff6a3f20";
	public string ListViewItemSelectedForegroundColor { get; set; } = "#ffffffff";
	public string ThemeAccentColor { get; set; } = "#ccc87423";
	public string HighlightColor { get; set; } = "#ffc87423";
	public string ControlPointerOverBackgroundColor { get; set; } = "#80573721";
	public string ControlPressedBackgroundColor { get; set; } = "#9960382a";
	public string ControlPointerOverBorderColor { get; set; } = "#e09a52";
	public string ControlPressedBorderColor { get; set; } = "#c27b38";
	public double SetupTitleFontSize { get; set; } = 34;
	public double SetupSubtitleFontSize { get; set; } = 16;
	public double SetupPrimaryActionFontSize { get; set; } = 34;

	public static ThemeProfile CreateDefault(string name, NexenTheme theme) {
		if (theme == NexenTheme.Dark) {
			return new ThemeProfile {
				Name = name,
				Theme = NexenTheme.Dark,
				UiFontFamily = "Microsoft Sans Serif",
				UiFontSize = 11,
				MenuFontFamily = "Segoe UI",
				MenuFontSize = 12,
				StartupWindowBackgroundColor = "#ff1d130d",
				StartupSurfaceBackgroundColor = "#ff2a1a10",
				StartupBorderColor = "#ff6b3f24",
				StartupCardBackgroundColor = "#ff2a1a10",
				StartupCardCheckedBackgroundColor = "#ffb65e18",
				StartupTextColor = "#ffffffff",
				StartupMutedTextColor = "#ffefcfb6",
				StartupActionBorderColor = "#ffffce87",
				StartupPrimaryActionColor = "#ffd47a22",
				StartupDividerColor = "#553f2b1f",
				SetupSubtitleColor = "#ffefcfb6",
				MenuBackgroundColor = "#55281a10",
				MenuBackgroundHighlightColor = "#80573721",
				MenuBorderHighlightColor = "#e09a52",
				MenuSeparatorBackgroundColor = "#66c2864f",
				SettingsTabStripBackgroundColor = "#2f2015",
				SettingsTabStripBorderColor = "#80808080",
				DockTabStripBackgroundColor = "#ff282828",
				DockTabPointerOverColor = "#40808080",
				DockTabActiveBackgroundColor = "#ff282828",
				DockTabActiveBorderColor = "#ff505050",
				CheckBoxPointerOverBorderColor = "#e09a52",
				CheckBoxPressedBackgroundColor = "#9960382a",
				CheckBoxPressedBorderColor = "#c27b38",
				RadioButtonPointerOverBorderColor = "#e09a52",
				RadioButtonPressedBackgroundColor = "#9960382a",
				RadioButtonPressedBorderColor = "#c27b38",
				SliderTrackPointerOverColor = "#e09a52",
				SliderTrackPressedColor = "#c27b38",
				TextBoxSelectionColor = "#2a509f",
				TextControlDisabledBackgroundColor = "#ff333333",
				ToolTipBackgroundColor = "#181818",
				MenuFlyoutBackgroundColor = "#ff2a1a10",
				MenuFlyoutBorderColor = "#a87343",
				ComboBoxDropDownBackgroundColor = "#ff2a2a2a",
				ComboBoxDropDownBorderColor = "#ff909090",
				DataGridHeaderBackgroundColor = "#ff303030",
				DataGridHeaderForegroundColor = "#ffdedede",
				DataGridSelectedRowForegroundColor = "#ffffffff",
				ListBoxItemHoverBackgroundColor = "#66403020",
				ListBoxItemSelectedBackgroundColor = "#ff6a3f20",
				ListBoxItemSelectedForegroundColor = "#ffffffff",
				TreeViewItemHoverBackgroundColor = "#663d2a1b",
				TreeViewItemSelectedBackgroundColor = "#ff6a3f20",
				TreeViewItemSelectedForegroundColor = "#ffffffff",
				NavigationViewItemHoverBackgroundColor = "#66403020",
				NavigationViewItemSelectedBackgroundColor = "#ff6a3f20",
				NavigationViewItemSelectedForegroundColor = "#ffffffff",
				ListViewItemHoverBackgroundColor = "#66403020",
				ListViewItemSelectedBackgroundColor = "#ff6a3f20",
				ListViewItemSelectedForegroundColor = "#ffffffff",
				ThemeAccentColor = "#ccc87423",
				HighlightColor = "#ffc87423",
				ControlPointerOverBackgroundColor = "#80573721",
				ControlPressedBackgroundColor = "#9960382a",
				ControlPointerOverBorderColor = "#e09a52",
				ControlPressedBorderColor = "#c27b38",
				SetupTitleFontSize = 34,
				SetupSubtitleFontSize = 16,
				SetupPrimaryActionFontSize = 34
			};
		}

		return new ThemeProfile {
			Name = name,
			Theme = NexenTheme.Light,
			UiFontFamily = "Microsoft Sans Serif",
			UiFontSize = 11,
			MenuFontFamily = "Segoe UI",
			MenuFontSize = 12,
			StartupWindowBackgroundColor = "#fff6e7da",
			StartupSurfaceBackgroundColor = "#fffbeee2",
			StartupBorderColor = "#ffd2a880",
			StartupCardBackgroundColor = "#fffbeee2",
			StartupCardCheckedBackgroundColor = "#ffe28f43",
			StartupTextColor = "#ff2a1a10",
			StartupMutedTextColor = "#ff6a3f20",
			StartupActionBorderColor = "#ff8a5116",
			StartupPrimaryActionColor = "#ffeea55d",
			StartupDividerColor = "#66bf8b66",
			SetupSubtitleColor = "#ff6a3f20",
			MenuBackgroundColor = "#f3e5d7",
			MenuBackgroundHighlightColor = "#efb57b",
			MenuBorderHighlightColor = "#9f5c22",
			MenuSeparatorBackgroundColor = "#c9a788",
			SettingsTabStripBackgroundColor = "#f5e8dc",
			SettingsTabStripBorderColor = "#80808080",
			DockTabStripBackgroundColor = "#ffffffff",
			DockTabPointerOverColor = "#40808080",
			DockTabActiveBackgroundColor = "#ffffffff",
			DockTabActiveBorderColor = "#ff808080",
			CheckBoxPointerOverBorderColor = "#9f5c22",
			CheckBoxPressedBackgroundColor = "#e9c098",
			CheckBoxPressedBorderColor = "#7b4517",
			RadioButtonPointerOverBorderColor = "#9f5c22",
			RadioButtonPressedBackgroundColor = "#e9c098",
			RadioButtonPressedBorderColor = "#7b4517",
			SliderTrackPointerOverColor = "#9f5c22",
			SliderTrackPressedColor = "#7b4517",
			TextBoxSelectionColor = "#70c0ff",
			TextControlDisabledBackgroundColor = "#e0e0e0",
			ToolTipBackgroundColor = "#ffffed",
			MenuFlyoutBackgroundColor = "#fff6ee",
			MenuFlyoutBorderColor = "#a87343",
			ComboBoxDropDownBackgroundColor = "#ffffffff",
			ComboBoxDropDownBorderColor = "#ff909090",
			DataGridHeaderBackgroundColor = "#ffededed",
			DataGridHeaderForegroundColor = "#ff000000",
			DataGridSelectedRowForegroundColor = "#ffffffff",
			ListBoxItemHoverBackgroundColor = "#fff4ddc7",
			ListBoxItemSelectedBackgroundColor = "#ffb56f33",
			ListBoxItemSelectedForegroundColor = "#ffffffff",
			TreeViewItemHoverBackgroundColor = "#fff2dcc8",
			TreeViewItemSelectedBackgroundColor = "#ffb56f33",
			TreeViewItemSelectedForegroundColor = "#ffffffff",
			NavigationViewItemHoverBackgroundColor = "#fff4ddc7",
			NavigationViewItemSelectedBackgroundColor = "#ffb56f33",
			NavigationViewItemSelectedForegroundColor = "#ffffffff",
			ListViewItemHoverBackgroundColor = "#fff4ddc7",
			ListViewItemSelectedBackgroundColor = "#ffb56f33",
			ListViewItemSelectedForegroundColor = "#ffffffff",
			ThemeAccentColor = "#ffc56e21",
			HighlightColor = "#ff8a5116",
			ControlPointerOverBackgroundColor = "#f8d9b8",
			ControlPressedBackgroundColor = "#e9c098",
			ControlPointerOverBorderColor = "#9f5c22",
			ControlPressedBorderColor = "#7b4517",
			SetupTitleFontSize = 34,
			SetupSubtitleFontSize = 16,
			SetupPrimaryActionFontSize = 34
		};
	}

	public static bool IsValidColor(string? value) {
		if (string.IsNullOrWhiteSpace(value)) {
			return false;
		}

		try {
			Color.Parse(value);
			return true;
		} catch {
			return false;
		}
	}
}

public sealed class ThemeProfileFile {
	public string Format { get; set; } = "nexen-theme-profile";
	public UInt32 Version { get; set; } = 1;
	public ThemeProfile Profile { get; set; } = new ThemeProfile();

	public bool IsValid() {
		if (Format != "nexen-theme-profile" || Version != 1) {
			return false;
		}

		if (string.IsNullOrWhiteSpace(Profile.Name)) {
			return false;
		}

		return ThemeProfile.IsValidColor(Profile.StartupWindowBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.StartupSurfaceBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.StartupBorderColor)
			&& ThemeProfile.IsValidColor(Profile.StartupCardBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.StartupCardCheckedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.StartupTextColor)
			&& ThemeProfile.IsValidColor(Profile.StartupMutedTextColor)
			&& ThemeProfile.IsValidColor(Profile.StartupActionBorderColor)
			&& ThemeProfile.IsValidColor(Profile.StartupPrimaryActionColor)
			&& ThemeProfile.IsValidColor(Profile.StartupDividerColor)
			&& ThemeProfile.IsValidColor(Profile.SetupSubtitleColor)
			&& ThemeProfile.IsValidColor(Profile.MenuBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.MenuBackgroundHighlightColor)
			&& ThemeProfile.IsValidColor(Profile.MenuBorderHighlightColor)
			&& ThemeProfile.IsValidColor(Profile.MenuSeparatorBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.SettingsTabStripBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.SettingsTabStripBorderColor)
			&& ThemeProfile.IsValidColor(Profile.DockTabStripBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.DockTabPointerOverColor)
			&& ThemeProfile.IsValidColor(Profile.DockTabActiveBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.DockTabActiveBorderColor)
			&& ThemeProfile.IsValidColor(Profile.CheckBoxPointerOverBorderColor)
			&& ThemeProfile.IsValidColor(Profile.CheckBoxPressedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.CheckBoxPressedBorderColor)
			&& ThemeProfile.IsValidColor(Profile.RadioButtonPointerOverBorderColor)
			&& ThemeProfile.IsValidColor(Profile.RadioButtonPressedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.RadioButtonPressedBorderColor)
			&& ThemeProfile.IsValidColor(Profile.SliderTrackPointerOverColor)
			&& ThemeProfile.IsValidColor(Profile.SliderTrackPressedColor)
			&& ThemeProfile.IsValidColor(Profile.TextBoxSelectionColor)
			&& ThemeProfile.IsValidColor(Profile.TextControlDisabledBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ToolTipBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.MenuFlyoutBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.MenuFlyoutBorderColor)
			&& ThemeProfile.IsValidColor(Profile.ComboBoxDropDownBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ComboBoxDropDownBorderColor)
			&& ThemeProfile.IsValidColor(Profile.DataGridHeaderBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.DataGridHeaderForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.DataGridSelectedRowForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListBoxItemHoverBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListBoxItemSelectedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListBoxItemSelectedForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.TreeViewItemHoverBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.TreeViewItemSelectedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.TreeViewItemSelectedForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.NavigationViewItemHoverBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.NavigationViewItemSelectedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.NavigationViewItemSelectedForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListViewItemHoverBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListViewItemSelectedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ListViewItemSelectedForegroundColor)
			&& ThemeProfile.IsValidColor(Profile.ThemeAccentColor)
			&& ThemeProfile.IsValidColor(Profile.HighlightColor)
			&& ThemeProfile.IsValidColor(Profile.ControlPointerOverBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ControlPressedBackgroundColor)
			&& ThemeProfile.IsValidColor(Profile.ControlPointerOverBorderColor)
			&& ThemeProfile.IsValidColor(Profile.ControlPressedBorderColor);
	}
}
