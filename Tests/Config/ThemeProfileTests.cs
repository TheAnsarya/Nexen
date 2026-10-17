using Nexen.Config;
using System.Text.Json;
using Xunit;

namespace Nexen.Tests.Config;

public sealed class ThemeProfileTests {
	[Fact]
	public void ThemeProfileFile_IsValid_RejectsUnsupportedFormat() {
		ThemeProfileFile file = new ThemeProfileFile {
			Format = "other-format",
			Version = 1,
			Profile = ThemeProfile.CreateDefault("Test", NexenTheme.Dark)
		};

		Assert.False(file.IsValid());
	}

	[Fact]
	public void ThemeProfileFile_IsValid_RejectsInvalidColor() {
		ThemeProfile profile = ThemeProfile.CreateDefault("Test", NexenTheme.Dark);
		profile.DataGridHeaderForegroundColor = "bad-color";

		ThemeProfileFile file = new ThemeProfileFile {
			Format = "nexen-theme-profile",
			Version = 1,
			Profile = profile
		};

		Assert.False(file.IsValid());
	}

	[Fact]
	public void ThemeProfileFile_IsValid_RejectsInvalidNavigationViewSemanticColors() {
		ThemeProfile profile = ThemeProfile.CreateDefault("Test", NexenTheme.Dark);
		profile.NavigationViewItemHoverBackgroundColor = "bad-color";

		ThemeProfileFile file = new ThemeProfileFile {
			Format = "nexen-theme-profile",
			Version = 1,
			Profile = profile
		};

		Assert.False(file.IsValid());
	}

	[Fact]
	public void ThemeProfileFile_IsValid_RejectsInvalidListViewSemanticColors() {
		ThemeProfile profile = ThemeProfile.CreateDefault("Test", NexenTheme.Dark);
		profile.ListViewItemSelectedForegroundColor = "bad-color";

		ThemeProfileFile file = new ThemeProfileFile {
			Format = "nexen-theme-profile",
			Version = 1,
			Profile = profile
		};

		Assert.False(file.IsValid());
	}

	[Fact]
	public void ThemeProfile_CreateDefault_IncludesCentralizedMenuAndAccentTokens() {
		ThemeProfile dark = ThemeProfile.CreateDefault("Default Dark", NexenTheme.Dark);
		ThemeProfile light = ThemeProfile.CreateDefault("Default Light", NexenTheme.Light);

		Assert.Equal("#55281a10", dark.MenuBackgroundColor);
		Assert.Equal("#80573721", dark.MenuBackgroundHighlightColor);
		Assert.Equal("#ccc87423", dark.ThemeAccentColor);
		Assert.Equal("#80573721", dark.ControlPointerOverBackgroundColor);
		Assert.Equal("#c27b38", dark.ControlPressedBorderColor);
		Assert.Equal("#80808080", dark.SettingsTabStripBorderColor);
		Assert.Equal("#ff282828", dark.DockTabStripBackgroundColor);
		Assert.Equal("#40808080", dark.DockTabPointerOverColor);
		Assert.Equal("#ff282828", dark.DockTabActiveBackgroundColor);
		Assert.Equal("#ff505050", dark.DockTabActiveBorderColor);
		Assert.Equal("#e09a52", dark.CheckBoxPointerOverBorderColor);
		Assert.Equal("#9960382a", dark.CheckBoxPressedBackgroundColor);
		Assert.Equal("#c27b38", dark.CheckBoxPressedBorderColor);
		Assert.Equal("#e09a52", dark.RadioButtonPointerOverBorderColor);
		Assert.Equal("#9960382a", dark.RadioButtonPressedBackgroundColor);
		Assert.Equal("#c27b38", dark.RadioButtonPressedBorderColor);
		Assert.Equal("#e09a52", dark.SliderTrackPointerOverColor);
		Assert.Equal("#c27b38", dark.SliderTrackPressedColor);
		Assert.Equal("#2a509f", dark.TextBoxSelectionColor);
		Assert.Equal("#ff333333", dark.TextControlDisabledBackgroundColor);
		Assert.Equal("#181818", dark.ToolTipBackgroundColor);
		Assert.Equal("#ff2a1a10", dark.MenuFlyoutBackgroundColor);
		Assert.Equal("#a87343", dark.MenuFlyoutBorderColor);
		Assert.Equal("#ff2a2a2a", dark.ComboBoxDropDownBackgroundColor);
		Assert.Equal("#ff909090", dark.ComboBoxDropDownBorderColor);
		Assert.Equal("#ff303030", dark.DataGridHeaderBackgroundColor);
		Assert.Equal("#ffdedede", dark.DataGridHeaderForegroundColor);
		Assert.Equal("#ffffffff", dark.DataGridSelectedRowForegroundColor);
		Assert.Equal("#66403020", dark.ListBoxItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", dark.ListBoxItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", dark.ListBoxItemSelectedForegroundColor);
		Assert.Equal("#663d2a1b", dark.TreeViewItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", dark.TreeViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", dark.TreeViewItemSelectedForegroundColor);
		Assert.Equal("#66403020", dark.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", dark.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", dark.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66403020", dark.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", dark.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", dark.ListViewItemSelectedForegroundColor);

		Assert.Equal("#f3e5d7", light.MenuBackgroundColor);
		Assert.Equal("#efb57b", light.MenuBackgroundHighlightColor);
		Assert.Equal("#ffc56e21", light.ThemeAccentColor);
		Assert.Equal("#f8d9b8", light.ControlPointerOverBackgroundColor);
		Assert.Equal("#7b4517", light.ControlPressedBorderColor);
		Assert.Equal("#80808080", light.SettingsTabStripBorderColor);
		Assert.Equal("#ffffffff", light.DockTabStripBackgroundColor);
		Assert.Equal("#40808080", light.DockTabPointerOverColor);
		Assert.Equal("#ffffffff", light.DockTabActiveBackgroundColor);
		Assert.Equal("#ff808080", light.DockTabActiveBorderColor);
		Assert.Equal("#9f5c22", light.CheckBoxPointerOverBorderColor);
		Assert.Equal("#e9c098", light.CheckBoxPressedBackgroundColor);
		Assert.Equal("#7b4517", light.CheckBoxPressedBorderColor);
		Assert.Equal("#9f5c22", light.RadioButtonPointerOverBorderColor);
		Assert.Equal("#e9c098", light.RadioButtonPressedBackgroundColor);
		Assert.Equal("#7b4517", light.RadioButtonPressedBorderColor);
		Assert.Equal("#9f5c22", light.SliderTrackPointerOverColor);
		Assert.Equal("#7b4517", light.SliderTrackPressedColor);
		Assert.Equal("#70c0ff", light.TextBoxSelectionColor);
		Assert.Equal("#e0e0e0", light.TextControlDisabledBackgroundColor);
		Assert.Equal("#ffffed", light.ToolTipBackgroundColor);
		Assert.Equal("#fff6ee", light.MenuFlyoutBackgroundColor);
		Assert.Equal("#a87343", light.MenuFlyoutBorderColor);
		Assert.Equal("#ffffffff", light.ComboBoxDropDownBackgroundColor);
		Assert.Equal("#ff909090", light.ComboBoxDropDownBorderColor);
		Assert.Equal("#ffededed", light.DataGridHeaderBackgroundColor);
		Assert.Equal("#ff000000", light.DataGridHeaderForegroundColor);
		Assert.Equal("#ffffffff", light.DataGridSelectedRowForegroundColor);
		Assert.Equal("#fff4ddc7", light.ListBoxItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", light.ListBoxItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", light.ListBoxItemSelectedForegroundColor);
		Assert.Equal("#fff2dcc8", light.TreeViewItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", light.TreeViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", light.TreeViewItemSelectedForegroundColor);
		Assert.Equal("#fff4ddc7", light.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", light.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", light.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#fff4ddc7", light.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", light.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", light.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void ThemeProfile_CreateDefault_PreservesSemanticFamilyDefaults_ByThemeMode() {
		ThemeProfile dark = ThemeProfile.CreateDefault("Default Dark", NexenTheme.Dark);
		ThemeProfile light = ThemeProfile.CreateDefault("Default Light", NexenTheme.Light);

		string[] darkSemanticTokens = [
			dark.ComboBoxDropDownBackgroundColor,
			dark.ComboBoxDropDownBorderColor,
			dark.DataGridHeaderBackgroundColor,
			dark.DataGridHeaderForegroundColor,
			dark.DataGridSelectedRowForegroundColor,
			dark.ListBoxItemHoverBackgroundColor,
			dark.ListBoxItemSelectedBackgroundColor,
			dark.ListBoxItemSelectedForegroundColor,
			dark.TreeViewItemHoverBackgroundColor,
			dark.TreeViewItemSelectedBackgroundColor,
			dark.TreeViewItemSelectedForegroundColor,
			dark.NavigationViewItemHoverBackgroundColor,
			dark.NavigationViewItemSelectedBackgroundColor,
			dark.NavigationViewItemSelectedForegroundColor,
			dark.ListViewItemHoverBackgroundColor,
			dark.ListViewItemSelectedBackgroundColor,
			dark.ListViewItemSelectedForegroundColor
		];

		string[] lightSemanticTokens = [
			light.ComboBoxDropDownBackgroundColor,
			light.ComboBoxDropDownBorderColor,
			light.DataGridHeaderBackgroundColor,
			light.DataGridHeaderForegroundColor,
			light.DataGridSelectedRowForegroundColor,
			light.ListBoxItemHoverBackgroundColor,
			light.ListBoxItemSelectedBackgroundColor,
			light.ListBoxItemSelectedForegroundColor,
			light.TreeViewItemHoverBackgroundColor,
			light.TreeViewItemSelectedBackgroundColor,
			light.TreeViewItemSelectedForegroundColor,
			light.NavigationViewItemHoverBackgroundColor,
			light.NavigationViewItemSelectedBackgroundColor,
			light.NavigationViewItemSelectedForegroundColor,
			light.ListViewItemHoverBackgroundColor,
			light.ListViewItemSelectedBackgroundColor,
			light.ListViewItemSelectedForegroundColor
		];

		Assert.All(darkSemanticTokens, color => Assert.True(ThemeProfile.IsValidColor(color), $"Invalid dark semantic token color: {color}"));
		Assert.All(lightSemanticTokens, color => Assert.True(ThemeProfile.IsValidColor(color), $"Invalid light semantic token color: {color}"));

		Assert.NotEqual(dark.ComboBoxDropDownBackgroundColor, light.ComboBoxDropDownBackgroundColor);
		Assert.NotEqual(dark.DataGridHeaderBackgroundColor, light.DataGridHeaderBackgroundColor);
		Assert.NotEqual(dark.ListBoxItemHoverBackgroundColor, light.ListBoxItemHoverBackgroundColor);
		Assert.NotEqual(dark.TreeViewItemHoverBackgroundColor, light.TreeViewItemHoverBackgroundColor);
		Assert.NotEqual(dark.NavigationViewItemHoverBackgroundColor, light.NavigationViewItemHoverBackgroundColor);
		Assert.NotEqual(dark.ListViewItemHoverBackgroundColor, light.ListViewItemHoverBackgroundColor);
	}

	[Fact]
	public void PreferencesConfig_RenameThemeProfile_UpdatesActiveName() {
		PreferencesConfig config = new PreferencesConfig();
		config.SetActiveThemeProfile("Default Dark");

		bool renamed = config.RenameThemeProfile("Default Dark", "Dark Variant A");

		Assert.True(renamed);
		Assert.Equal("Dark Variant A", config.ActiveThemeProfileName);
		Assert.NotNull(config.GetThemeProfileByName("Dark Variant A"));
	}

	[Fact]
	public void PreferencesConfig_DuplicateThemeProfile_AddsNewProfile() {
		PreferencesConfig config = new PreferencesConfig();

		bool duplicated = config.DuplicateThemeProfile("Default Light", "Default Light Copy");

		Assert.True(duplicated);
		Assert.NotNull(config.GetThemeProfileByName("Default Light Copy"));
	}

	[Fact]
	public void PreferencesConfig_DuplicateThemeProfile_PreservesNavigationAndListViewSemanticTokens() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? source = config.GetThemeProfileByName("Default Light");
		Assert.NotNull(source);

		source!.NavigationViewItemHoverBackgroundColor = "#66aa7733";
		source.NavigationViewItemSelectedBackgroundColor = "#ffbb8844";
		source.NavigationViewItemSelectedForegroundColor = "#ff112233";
		source.ListViewItemHoverBackgroundColor = "#66774422";
		source.ListViewItemSelectedBackgroundColor = "#ff775522";
		source.ListViewItemSelectedForegroundColor = "#ffeeccaa";

		bool duplicated = config.DuplicateThemeProfile("Default Light", "Default Light Copy");
		ThemeProfile? duplicate = config.GetThemeProfileByName("Default Light Copy");

		Assert.True(duplicated);
		Assert.NotNull(duplicate);
		Assert.Equal("#66aa7733", duplicate!.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ffbb8844", duplicate.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ff112233", duplicate.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66774422", duplicate.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff775522", duplicate.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffeeccaa", duplicate.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void PreferencesConfig_SetActiveThemeProfile_AppliesFontsAndTheme() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile profile = new ThemeProfile {
			Name = "MyProfile",
			Theme = NexenTheme.Dark,
			UiFontFamily = "Segoe UI",
			UiFontSize = 13,
			MenuFontFamily = "Consolas",
			MenuFontSize = 14,
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
			SetupTitleFontSize = 36,
			SetupSubtitleFontSize = 17,
			SetupPrimaryActionFontSize = 35
		};
		config.UpsertThemeProfile(profile, false);

		config.SetActiveThemeProfile("MyProfile");

		Assert.Equal(NexenTheme.Dark, config.Theme);
		Assert.Equal("Segoe UI", config.NexenFont.FontFamily);
		Assert.Equal(13, config.NexenFont.FontSize);
		Assert.Equal("Consolas", config.NexenMenuFont.FontFamily);
		Assert.Equal(14, config.NexenMenuFont.FontSize);
	}

	[Fact]
	public void PreferencesConfig_SetActiveThemeProfile_InvalidName_IsNoOp() {
		PreferencesConfig config = new PreferencesConfig();
		config.SetActiveThemeProfile("Default Light");

		config.SetActiveThemeProfile("Not A Real Profile");

		Assert.Equal("Default Light", config.ActiveThemeProfileName);
		Assert.Equal(NexenTheme.Light, config.Theme);
	}

	[Fact]
	public void PreferencesConfig_GenerateUniqueThemeProfileName_ReturnsCopyNameOnConflict() {
		PreferencesConfig config = new PreferencesConfig();

		string unique = config.GenerateUniqueThemeProfileName("Default Dark");

		Assert.Equal("Default Dark (Copy)", unique);
	}

	[Fact]
	public void PreferencesConfig_GenerateUniqueThemeProfileName_IncrementsCopySuffixWhenNeeded() {
		PreferencesConfig config = new PreferencesConfig();
		config.UpsertThemeProfile(ThemeProfile.CreateDefault("Default Dark (Copy)", NexenTheme.Dark), false);
		config.UpsertThemeProfile(ThemeProfile.CreateDefault("Default Dark (Copy 2)", NexenTheme.Dark), false);

		string unique = config.GenerateUniqueThemeProfileName("Default Dark");

		Assert.Equal("Default Dark (Copy 3)", unique);
	}

	[Fact]
	public void PreferencesConfig_GenerateUniqueThemeProfileName_TrimsIncomingNameBeforeConflictResolution() {
		PreferencesConfig config = new PreferencesConfig();

		string unique = config.GenerateUniqueThemeProfileName("  Default Dark  ");

		Assert.Equal("Default Dark (Copy)", unique);
	}

	[Fact]
	public void PreferencesConfig_RenameThemeProfile_RejectsTrimmedNameConflict() {
		PreferencesConfig config = new PreferencesConfig();
		config.SetActiveThemeProfile("Default Dark");

		bool renamed = config.RenameThemeProfile("Default Dark", "  Default Light  ");

		Assert.False(renamed);
		Assert.Equal("Default Dark", config.ActiveThemeProfileName);
		Assert.NotNull(config.GetThemeProfileByName("Default Dark"));
	}

	[Fact]
	public void PreferencesConfig_ResetThemeProfileToDefaults_RestoresCanonicalValues() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile dark = ThemeProfile.CreateDefault("Default Dark", NexenTheme.Dark);
		dark.StartupWindowBackgroundColor = "#ff000000";
		dark.StartupPrimaryActionColor = "#ff101010";
		dark.UiFontSize = 99;
		config.UpsertThemeProfile(dark, false);

		bool reset = config.ResetThemeProfileToDefaults("Default Dark");
		ThemeProfile? resetProfile = config.GetThemeProfileByName("Default Dark");

		Assert.True(reset);
		Assert.NotNull(resetProfile);
		Assert.Equal("#ff1d130d", resetProfile!.StartupWindowBackgroundColor);
		Assert.Equal("#ffd47a22", resetProfile.StartupPrimaryActionColor);
		Assert.Equal(11, resetProfile.UiFontSize);
	}

	[Fact]
	public void PreferencesConfig_DeleteThemeProfile_WhenSingleProfileRemaining_ReturnsFalse() {
		PreferencesConfig config = new PreferencesConfig();
		Assert.True(config.DeleteThemeProfile("Default Light"));

		bool deletedLast = config.DeleteThemeProfile("Default Dark");

		Assert.False(deletedLast);
		Assert.NotNull(config.GetThemeProfileByName("Default Dark"));
	}

	[Fact]
	public void PreferencesConfig_DeleteThemeProfile_ActiveSemanticProfile_FallsBackAndPreservesFallbackSemanticTokens() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? fallback = config.GetThemeProfileByName("Default Light");
		Assert.NotNull(fallback);
		fallback!.NavigationViewItemHoverBackgroundColor = "#66aabbcc";
		fallback.NavigationViewItemSelectedBackgroundColor = "#ffbbccdd";
		fallback.NavigationViewItemSelectedForegroundColor = "#ffccddee";
		fallback.ListViewItemHoverBackgroundColor = "#66ddeeff";
		fallback.ListViewItemSelectedBackgroundColor = "#ff112244";
		fallback.ListViewItemSelectedForegroundColor = "#ff334466";

		config.SetActiveThemeProfile("Default Dark");

		bool deleted = config.DeleteThemeProfile("Default Dark");
		ThemeProfile? remaining = config.GetThemeProfileByName("Default Light");

		Assert.True(deleted);
		Assert.Equal("Default Light", config.ActiveThemeProfileName);
		Assert.NotNull(remaining);
		Assert.Equal("#66aabbcc", remaining!.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ffbbccdd", remaining.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ffccddee", remaining.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66ddeeff", remaining.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff112244", remaining.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ff334466", remaining.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void PreferencesConfig_ResetThemeProfileToDefaults_RestoresNavigationAndListViewSemanticTokens() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? dark = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(dark);

		dark!.NavigationViewItemHoverBackgroundColor = "#66998877";
		dark.NavigationViewItemSelectedBackgroundColor = "#ff998877";
		dark.NavigationViewItemSelectedForegroundColor = "#ff223344";
		dark.ListViewItemHoverBackgroundColor = "#66665544";
		dark.ListViewItemSelectedBackgroundColor = "#ff554433";
		dark.ListViewItemSelectedForegroundColor = "#ffeeeecc";

		bool reset = config.ResetThemeProfileToDefaults("Default Dark");
		ThemeProfile? resetProfile = config.GetThemeProfileByName("Default Dark");

		Assert.True(reset);
		Assert.NotNull(resetProfile);
		Assert.Equal("#66403020", resetProfile!.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", resetProfile.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", resetProfile.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66403020", resetProfile.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff6a3f20", resetProfile.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", resetProfile.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void PreferencesConfig_ApplyThemePresetToProfile_AppliesLightDefaults() {
		PreferencesConfig config = new PreferencesConfig();

		bool applied = config.ApplyThemePresetToProfile("Default Dark", NexenTheme.Light);
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");

		Assert.True(applied);
		Assert.NotNull(profile);
		Assert.Equal(NexenTheme.Light, profile!.Theme);
		Assert.Equal("#fff6e7da", profile.StartupWindowBackgroundColor);
		Assert.Equal("#ffeea55d", profile.StartupPrimaryActionColor);
	}

	[Fact]
	public void PreferencesConfig_ApplyThemePresetToProfile_AppliesDarkDefaults() {
		PreferencesConfig config = new PreferencesConfig();
		config.ApplyThemePresetToProfile("Default Light", NexenTheme.Light);

		bool applied = config.ApplyThemePresetToProfile("Default Light", NexenTheme.Dark);
		ThemeProfile? profile = config.GetThemeProfileByName("Default Light");

		Assert.True(applied);
		Assert.NotNull(profile);
		Assert.Equal(NexenTheme.Dark, profile!.Theme);
		Assert.Equal("#ff1d130d", profile.StartupWindowBackgroundColor);
		Assert.Equal("#ffd47a22", profile.StartupPrimaryActionColor);
	}

	[Fact]
	public void PreferencesConfig_ApplyThemePresetToProfile_AppliesLightNavigationAndListViewDefaults() {
		PreferencesConfig config = new PreferencesConfig();

		bool applied = config.ApplyThemePresetToProfile("Default Dark", NexenTheme.Light);
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");

		Assert.True(applied);
		Assert.NotNull(profile);
		Assert.Equal("#fff4ddc7", profile!.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", profile.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", profile.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#fff4ddc7", profile.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ffb56f33", profile.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffffffff", profile.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void PreferencesConfig_GetThemeProfileDivergenceCount_DefaultProfileHasNoDivergence() {
		PreferencesConfig config = new PreferencesConfig();

		int count = config.GetThemeProfileDivergenceCount("Default Dark");

		Assert.Equal(0, count);
	}

	[Fact]
	public void PreferencesConfig_GetThemeProfileDivergenceCount_CustomizedProfileHasDivergence() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(profile);
		profile!.StartupTextColor = "#ff000000";
		profile.UiFontSize = 13;

		int count = config.GetThemeProfileDivergenceCount("Default Dark");

		Assert.Equal(2, count);
	}

	[Fact]
	public void PreferencesConfig_GetThemeProfileCustomizedTokenNames_ReturnsExpectedTokenNames() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(profile);
		profile!.StartupTextColor = "#ff000000";
		profile.UiFontSize = 13;

		List<string> tokens = config.GetThemeProfileCustomizedTokenNames("Default Dark");

		Assert.Contains("StartupTextColor", tokens);
		Assert.Contains("UiFontSize", tokens);
		Assert.Equal(2, tokens.Count);
		Assert.Equal("UiFontSize", tokens[0]);
		Assert.Equal("StartupTextColor", tokens[1]);
	}

	[Fact]
	public void PreferencesConfig_DivergenceAndCustomizedTokens_IncludeNavigationAndListViewSemanticTokens() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(profile);

		profile!.NavigationViewItemSelectedBackgroundColor = "#ff123456";
		profile.ListViewItemSelectedForegroundColor = "#ffabcdef";

		int divergenceCount = config.GetThemeProfileDivergenceCount("Default Dark");
		List<string> customized = config.GetThemeProfileCustomizedTokenNames("Default Dark");

		Assert.Equal(2, divergenceCount);
		Assert.Equal(2, customized.Count);
		Assert.Contains("NavigationViewItemSelectedBackgroundColor", customized);
		Assert.Contains("ListViewItemSelectedForegroundColor", customized);
	}

	[Fact]
	public void PreferencesConfig_UpsertThemeProfile_UpdatesNavigationAndListViewSemanticTokens() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile profile = ThemeProfile.CreateDefault("Semantic Upsert", NexenTheme.Dark);
		profile.NavigationViewItemHoverBackgroundColor = "#66111111";
		profile.NavigationViewItemSelectedBackgroundColor = "#ff222222";
		profile.NavigationViewItemSelectedForegroundColor = "#ff333333";
		profile.ListViewItemHoverBackgroundColor = "#66444444";
		profile.ListViewItemSelectedBackgroundColor = "#ff555555";
		profile.ListViewItemSelectedForegroundColor = "#ff666666";
		config.UpsertThemeProfile(profile, false);

		ThemeProfile updated = ThemeProfile.CreateDefault("Semantic Upsert", NexenTheme.Dark);
		updated.NavigationViewItemHoverBackgroundColor = "#66777777";
		updated.NavigationViewItemSelectedBackgroundColor = "#ff888888";
		updated.NavigationViewItemSelectedForegroundColor = "#ff999999";
		updated.ListViewItemHoverBackgroundColor = "#66aaaaaa";
		updated.ListViewItemSelectedBackgroundColor = "#ffbbbbbb";
		updated.ListViewItemSelectedForegroundColor = "#ffcccccc";
		config.UpsertThemeProfile(updated, false);

		ThemeProfile? roundTripped = config.GetThemeProfileByName("Semantic Upsert");
		Assert.NotNull(roundTripped);
		Assert.Equal("#66777777", roundTripped!.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ff888888", roundTripped.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ff999999", roundTripped.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66aaaaaa", roundTripped.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ffbbbbbb", roundTripped.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffcccccc", roundTripped.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void ThemeProfile_ImportExportRoundtrip_PreservesComboDataGridListTreeNavigationListViewSemanticTokens_AfterSaveCurrentCycle() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? sourceProfile = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(sourceProfile);

		sourceProfile!.ComboBoxDropDownBackgroundColor = "#ff112233";
		sourceProfile.ComboBoxDropDownBorderColor = "#ff223344";
		sourceProfile.DataGridHeaderBackgroundColor = "#ff334455";
		sourceProfile.DataGridHeaderForegroundColor = "#ff445566";
		sourceProfile.DataGridSelectedRowForegroundColor = "#ff556677";
		sourceProfile.ListBoxItemHoverBackgroundColor = "#66778899";
		sourceProfile.ListBoxItemSelectedBackgroundColor = "#ff778899";
		sourceProfile.ListBoxItemSelectedForegroundColor = "#ff8899aa";
		sourceProfile.TreeViewItemHoverBackgroundColor = "#6699aabb";
		sourceProfile.TreeViewItemSelectedBackgroundColor = "#ffaabbcc";
		sourceProfile.TreeViewItemSelectedForegroundColor = "#ffbbccee";
		sourceProfile.NavigationViewItemHoverBackgroundColor = "#66ccddee";
		sourceProfile.NavigationViewItemSelectedBackgroundColor = "#ffddeeff";
		sourceProfile.NavigationViewItemSelectedForegroundColor = "#ff112244";
		sourceProfile.ListViewItemHoverBackgroundColor = "#66224466";
		sourceProfile.ListViewItemSelectedBackgroundColor = "#ff334477";
		sourceProfile.ListViewItemSelectedForegroundColor = "#ffddeeff";

		ThemeProfileFile exportFile = new ThemeProfileFile { Profile = sourceProfile };
		string json = JsonSerializer.Serialize(exportFile, typeof(ThemeProfileFile), NexenSerializerContext.Default);
		ThemeProfileFile? importFile = (ThemeProfileFile?)JsonSerializer.Deserialize(json, typeof(ThemeProfileFile), NexenSerializerContext.Default);

		Assert.NotNull(importFile);
		Assert.True(importFile!.IsValid());
		importFile.Profile.Name = "Roundtrip Semantic Token Profile";
		config.UpsertThemeProfile(importFile.Profile, false);
		config.SaveCurrentToProfile(importFile.Profile.Name);

		ThemeProfile? roundTripped = config.GetThemeProfileByName(importFile.Profile.Name);
		Assert.NotNull(roundTripped);
		Assert.Equal("#ff112233", roundTripped!.ComboBoxDropDownBackgroundColor);
		Assert.Equal("#ff223344", roundTripped.ComboBoxDropDownBorderColor);
		Assert.Equal("#ff334455", roundTripped.DataGridHeaderBackgroundColor);
		Assert.Equal("#ff445566", roundTripped.DataGridHeaderForegroundColor);
		Assert.Equal("#ff556677", roundTripped.DataGridSelectedRowForegroundColor);
		Assert.Equal("#66778899", roundTripped.ListBoxItemHoverBackgroundColor);
		Assert.Equal("#ff778899", roundTripped.ListBoxItemSelectedBackgroundColor);
		Assert.Equal("#ff8899aa", roundTripped.ListBoxItemSelectedForegroundColor);
		Assert.Equal("#6699aabb", roundTripped.TreeViewItemHoverBackgroundColor);
		Assert.Equal("#ffaabbcc", roundTripped.TreeViewItemSelectedBackgroundColor);
		Assert.Equal("#ffbbccee", roundTripped.TreeViewItemSelectedForegroundColor);
		Assert.Equal("#66ccddee", roundTripped.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ffddeeff", roundTripped.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ff112244", roundTripped.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66224466", roundTripped.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff334477", roundTripped.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ffddeeff", roundTripped.ListViewItemSelectedForegroundColor);
	}

	[Fact]
	public void PreferencesConfig_SaveCurrentToProfile_WithoutApplicationResources_PreservesNavigationAndListViewSemanticTokenValues() {
		PreferencesConfig config = new PreferencesConfig();
		ThemeProfile? profile = config.GetThemeProfileByName("Default Dark");
		Assert.NotNull(profile);

		profile!.NavigationViewItemHoverBackgroundColor = "#66123456";
		profile.NavigationViewItemSelectedBackgroundColor = "#ff345678";
		profile.NavigationViewItemSelectedForegroundColor = "#ff56789a";
		profile.ListViewItemHoverBackgroundColor = "#66789abc";
		profile.ListViewItemSelectedBackgroundColor = "#ff89abcd";
		profile.ListViewItemSelectedForegroundColor = "#ff9abcde";

		config.Theme = NexenTheme.Light;
		config.NexenFont.FontFamily = "Cascadia Code";
		config.NexenFont.FontSize = 15;
		config.NexenMenuFont.FontFamily = "Consolas";
		config.NexenMenuFont.FontSize = 16;

		config.SaveCurrentToProfile("Default Dark");

		Assert.Equal(NexenTheme.Light, profile.Theme);
		Assert.Equal("Cascadia Code", profile.UiFontFamily);
		Assert.Equal(15, profile.UiFontSize);
		Assert.Equal("Consolas", profile.MenuFontFamily);
		Assert.Equal(16, profile.MenuFontSize);
		Assert.Equal("#66123456", profile.NavigationViewItemHoverBackgroundColor);
		Assert.Equal("#ff345678", profile.NavigationViewItemSelectedBackgroundColor);
		Assert.Equal("#ff56789a", profile.NavigationViewItemSelectedForegroundColor);
		Assert.Equal("#66789abc", profile.ListViewItemHoverBackgroundColor);
		Assert.Equal("#ff89abcd", profile.ListViewItemSelectedBackgroundColor);
		Assert.Equal("#ff9abcde", profile.ListViewItemSelectedForegroundColor);
	}
}
