using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Styling;
using Avalonia.Themes.Fluent;
using Avalonia.VisualTree;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Windows;

namespace Nexen.Views;
public partial class PreferencesConfigView : UserControl {
	public PreferencesConfigView() {
		InitializeComponent();
		UpdateMigrationStatus();
	}

	private void btnResetLagCounter_OnClick(object sender, RoutedEventArgs e) {
		InputApi.ResetLagCounter();
	}

	private void btnChangeStorageFolder_OnClick(object sender, RoutedEventArgs e) {
		ShowSelectFolderWindow();
	}

	private async void ShowSelectFolderWindow() {
		SelectStorageFolderWindow wnd = new();
		if (await wnd.ShowCenteredDialog<bool>(TopLevel.GetTopLevel(this) as Visual)) {
			(TopLevel.GetTopLevel(this) as Window)?.Close();
			ApplicationHelper.GetMainWindow()?.Close();
			ConfigManager.RestartNexen();
		}
	}

	private void UpdateMigrationStatus() {
		var (legacyFiles, gameCount) = GameDataManager.GetCleanupStatus();
		if (lblMigrationStatus is not null) {
			if (legacyFiles > 0) {
				lblMigrationStatus.Text = $"{legacyFiles} legacy file(s) from {gameCount} game(s) can be cleaned up";
			} else {
				lblMigrationStatus.Text = "No legacy files to clean up";
			}
		}
	}

	private async void btnCleanupLegacyData_OnClick(object sender, RoutedEventArgs e) {
		var (legacyFiles, gameCount) = GameDataManager.GetCleanupStatus();
		if (legacyFiles == 0) {
			await NexenMsgBox.Show(
				TopLevel.GetTopLevel(this) as Window,
				"NoLegacyFilesToCleanup",
				MessageBoxButtons.OK,
				MessageBoxIcon.Info
			);
			return;
		}

		var result = await NexenMsgBox.Show(
			TopLevel.GetTopLevel(this) as Window,
			"ConfirmCleanupLegacyData",
			MessageBoxButtons.OKCancel,
			MessageBoxIcon.Warning,
			legacyFiles.ToString(),
			gameCount.ToString()
		);

		if (result == DialogResult.OK) {
			int cleaned = GameDataManager.CleanupMigratedLegacyFiles();
			UpdateMigrationStatus();

			await NexenMsgBox.Show(
				TopLevel.GetTopLevel(this) as Window,
				"CleanupComplete",
				MessageBoxButtons.OK,
				MessageBoxIcon.Info,
				cleaned.ToString()
			);
		}
	}

	private PreferencesConfigViewModel? GetViewModel() {
		return DataContext as PreferencesConfigViewModel;
	}

	private async void btnApplyThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		model.ActivateSelectedThemeProfile();
		await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileApplied", MessageBoxButtons.OK, MessageBoxIcon.Info);
	}

	private async void btnSaveThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		model.SaveCurrentToSelectedThemeProfile();
		await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileSaved", MessageBoxButtons.OK, MessageBoxIcon.Info);
	}

	private async void btnDeleteThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		bool removed = model.DeleteSelectedThemeProfile();
		if (!removed) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileDeleteFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnRenameThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		if (!model.RenameSelectedThemeProfile()) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileRenameFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnDuplicateThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		if (!model.DuplicateSelectedThemeProfile()) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileDuplicateFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnResetThemeProfileDefaults_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		if (model.ResetSelectedThemeProfileToDefaults()) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileResetDefaultsComplete", MessageBoxButtons.OK, MessageBoxIcon.Info);
		} else {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileResetDefaultsFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnApplyThemeLightPreset_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		if (model.ApplyLightPresetToSelectedThemeProfile()) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfilePresetLightApplied", MessageBoxButtons.OK, MessageBoxIcon.Info);
		} else {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfilePresetApplyFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnApplyThemeDarkPreset_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		if (model.ApplyDarkPresetToSelectedThemeProfile()) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfilePresetDarkApplied", MessageBoxButtons.OK, MessageBoxIcon.Info);
		} else {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfilePresetApplyFailed", MessageBoxButtons.OK, MessageBoxIcon.Info);
		}
	}

	private async void btnImportThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		string? path = await FileDialogHelper.OpenFile(ConfigManager.HomeFolder, this, "nexen-theme.json", "json");
		if (string.IsNullOrWhiteSpace(path)) {
			return;
		}

		try {
			string json = File.ReadAllText(path);
			ThemeProfileFile? profileFile = (ThemeProfileFile?)JsonSerializer.Deserialize(json, typeof(ThemeProfileFile), NexenSerializerContext.Default);
			if (profileFile?.Profile is null || !profileFile.IsValid()) {
				await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileImportInvalid", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			ThemeProfile importProfile = profileFile.Profile;
			ThemeProfile? existing = model.Config.GetThemeProfileByName(importProfile.Name);
			if (existing is not null) {
				DialogResult overwrite = await NexenMsgBox.Show(
					TopLevel.GetTopLevel(this) as Window,
					"ThemeProfileImportConflict",
					MessageBoxButtons.OKCancel,
					MessageBoxIcon.Question,
					importProfile.Name
				);

				if (overwrite != DialogResult.OK) {
					importProfile.Name = model.Config.GenerateUniqueThemeProfileName(importProfile.Name);
				}
			}

			model.UpsertImportedThemeProfile(importProfile, true);
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileImportComplete", MessageBoxButtons.OK, MessageBoxIcon.Info, importProfile.Name);
		} catch (Exception ex) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileImportFailed", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.Message);
		}
	}

	private async void btnExportThemeProfile_OnClick(object sender, RoutedEventArgs e) {
		PreferencesConfigViewModel? model = GetViewModel();
		if (model is null) {
			return;
		}

		ThemeProfile? profile = model.GetSelectedThemeProfile();
		if (profile is null) {
			await NexenMsgBox.Show(TopLevel.GetTopLevel(this) as Window, "ThemeProfileExportNoSelection", MessageBoxButtons.OK, MessageBoxIcon.Info);
			return;
		}

		string suggested = profile.Name.Replace(' ', '-').ToLowerInvariant() + ".nexen-theme.json";
		string? path = await FileDialogHelper.SaveFile(ConfigManager.HomeFolder, suggested, this, "nexen-theme.json", "json");
		if (string.IsNullOrWhiteSpace(path)) {
			return;
		}

		ThemeProfileFile profileFile = new ThemeProfileFile { Profile = profile };
		string json = JsonSerializer.Serialize(profileFile, typeof(ThemeProfileFile), NexenSerializerContext.Default);
		File.WriteAllText(path, json);
	}

	private async void btnPickThemeBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.StartupWindowBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileColors(colorHex, model.SelectedProfileStartupTextColor, model.SelectedProfileStartupPrimaryActionColor);
		});
	}

	private async void btnPickThemeTextColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.StartupTextColor, (model, colorHex) => {
			model.SetSelectedProfileColors(model.SelectedProfileStartupBackgroundColor, colorHex, model.SelectedProfileStartupPrimaryActionColor);
		});
	}

	private async void btnPickThemePrimaryActionColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.StartupPrimaryActionColor, (model, colorHex) => {
			model.SetSelectedProfileColors(model.SelectedProfileStartupBackgroundColor, model.SelectedProfileStartupTextColor, colorHex);
		});
	}

	private async void btnPickThemeMenuBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.MenuBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileUiChromeColors(colorHex, model.SelectedProfileMenuHighlightColor, model.SelectedProfileAccentColor);
		});
	}

	private async void btnPickThemeMenuHighlightColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.MenuBackgroundHighlightColor, (model, colorHex) => {
			model.SetSelectedProfileUiChromeColors(model.SelectedProfileMenuBackgroundColor, colorHex, model.SelectedProfileAccentColor);
		});
	}

	private async void btnPickThemeAccentColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ThemeAccentColor, (model, colorHex) => {
			model.SetSelectedProfileUiChromeColors(model.SelectedProfileMenuBackgroundColor, model.SelectedProfileMenuHighlightColor, colorHex);
		});
	}

	private async void btnPickThemeControlHoverBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ControlPointerOverBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileControlChromeColors(colorHex, model.SelectedProfileControlPressedBackgroundColor, model.SelectedProfileControlHoverBorderColor, model.SelectedProfileControlPressedBorderColor);
		});
	}

	private async void btnPickThemeControlPressedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ControlPressedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileControlChromeColors(model.SelectedProfileControlHoverBackgroundColor, colorHex, model.SelectedProfileControlHoverBorderColor, model.SelectedProfileControlPressedBorderColor);
		});
	}

	private async void btnPickThemeControlHoverBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ControlPointerOverBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlChromeColors(model.SelectedProfileControlHoverBackgroundColor, model.SelectedProfileControlPressedBackgroundColor, colorHex, model.SelectedProfileControlPressedBorderColor);
		});
	}

	private async void btnPickThemeControlPressedBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ControlPressedBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlChromeColors(model.SelectedProfileControlHoverBackgroundColor, model.SelectedProfileControlPressedBackgroundColor, model.SelectedProfileControlHoverBorderColor, colorHex);
		});
	}

	private async void btnPickThemeSidebarBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.SettingsTabStripBorderColor, (model, colorHex) => {
			model.SetSelectedProfileSidebarTabChromeColors(colorHex, model.SelectedProfileDockTabStripBackgroundColor, model.SelectedProfileDockTabHoverColor, model.SelectedProfileDockTabActiveBackgroundColor, model.SelectedProfileDockTabActiveBorderColor);
		});
	}

	private async void btnPickThemeDockTabStripBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DockTabStripBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileSidebarTabChromeColors(model.SelectedProfileSidebarBorderColor, colorHex, model.SelectedProfileDockTabHoverColor, model.SelectedProfileDockTabActiveBackgroundColor, model.SelectedProfileDockTabActiveBorderColor);
		});
	}

	private async void btnPickThemeDockTabHoverColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DockTabPointerOverColor, (model, colorHex) => {
			model.SetSelectedProfileSidebarTabChromeColors(model.SelectedProfileSidebarBorderColor, model.SelectedProfileDockTabStripBackgroundColor, colorHex, model.SelectedProfileDockTabActiveBackgroundColor, model.SelectedProfileDockTabActiveBorderColor);
		});
	}

	private async void btnPickThemeDockTabActiveBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DockTabActiveBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileSidebarTabChromeColors(model.SelectedProfileSidebarBorderColor, model.SelectedProfileDockTabStripBackgroundColor, model.SelectedProfileDockTabHoverColor, colorHex, model.SelectedProfileDockTabActiveBorderColor);
		});
	}

	private async void btnPickThemeDockTabActiveBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DockTabActiveBorderColor, (model, colorHex) => {
			model.SetSelectedProfileSidebarTabChromeColors(model.SelectedProfileSidebarBorderColor, model.SelectedProfileDockTabStripBackgroundColor, model.SelectedProfileDockTabHoverColor, model.SelectedProfileDockTabActiveBackgroundColor, colorHex);
		});
	}

	private async void btnPickThemeCheckBoxHoverBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.CheckBoxPointerOverBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(colorHex, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeCheckBoxPressedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.CheckBoxPressedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, colorHex, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeCheckBoxPressedBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.CheckBoxPressedBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, colorHex, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeRadioButtonHoverBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.RadioButtonPointerOverBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, colorHex, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeRadioButtonPressedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.RadioButtonPressedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, colorHex, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeRadioButtonPressedBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.RadioButtonPressedBorderColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, colorHex, model.SelectedProfileSliderHoverTrackColor, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeSliderHoverTrackColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.SliderTrackPointerOverColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, colorHex, model.SelectedProfileSliderPressedTrackColor);
		});
	}

	private async void btnPickThemeSliderPressedTrackColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.SliderTrackPressedColor, (model, colorHex) => {
			model.SetSelectedProfileControlStateSemanticColors(model.SelectedProfileCheckBoxHoverBorderColor, model.SelectedProfileCheckBoxPressedBackgroundColor, model.SelectedProfileCheckBoxPressedBorderColor, model.SelectedProfileRadioButtonHoverBorderColor, model.SelectedProfileRadioButtonPressedBackgroundColor, model.SelectedProfileRadioButtonPressedBorderColor, model.SelectedProfileSliderHoverTrackColor, colorHex);
		});
	}

	private async void btnPickThemeTextSelectionColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.TextBoxSelectionColor, (model, colorHex) => {
			model.SetSelectedProfileTextInputAndFlyoutSemanticColors(colorHex, model.SelectedProfileTextControlDisabledBackgroundColor, model.SelectedProfileToolTipBackgroundColor, model.SelectedProfileMenuFlyoutBackgroundColor, model.SelectedProfileMenuFlyoutBorderColor);
		});
	}

	private async void btnPickThemeTextControlDisabledBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.TextControlDisabledBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileTextInputAndFlyoutSemanticColors(model.SelectedProfileTextSelectionColor, colorHex, model.SelectedProfileToolTipBackgroundColor, model.SelectedProfileMenuFlyoutBackgroundColor, model.SelectedProfileMenuFlyoutBorderColor);
		});
	}

	private async void btnPickThemeToolTipBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ToolTipBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileTextInputAndFlyoutSemanticColors(model.SelectedProfileTextSelectionColor, model.SelectedProfileTextControlDisabledBackgroundColor, colorHex, model.SelectedProfileMenuFlyoutBackgroundColor, model.SelectedProfileMenuFlyoutBorderColor);
		});
	}

	private async void btnPickThemeMenuFlyoutBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.MenuFlyoutBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileTextInputAndFlyoutSemanticColors(model.SelectedProfileTextSelectionColor, model.SelectedProfileTextControlDisabledBackgroundColor, model.SelectedProfileToolTipBackgroundColor, colorHex, model.SelectedProfileMenuFlyoutBorderColor);
		});
	}

	private async void btnPickThemeMenuFlyoutBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.MenuFlyoutBorderColor, (model, colorHex) => {
			model.SetSelectedProfileTextInputAndFlyoutSemanticColors(model.SelectedProfileTextSelectionColor, model.SelectedProfileTextControlDisabledBackgroundColor, model.SelectedProfileToolTipBackgroundColor, model.SelectedProfileMenuFlyoutBackgroundColor, colorHex);
		});
	}

	private async void btnPickThemeComboBoxDropDownBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ComboBoxDropDownBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileComboDropDownAndDataGridSemanticColors(colorHex, model.SelectedProfileComboBoxDropDownBorderColor, model.SelectedProfileDataGridHeaderBackgroundColor, model.SelectedProfileDataGridHeaderForegroundColor, model.SelectedProfileDataGridSelectedRowForegroundColor);
		});
	}

	private async void btnPickThemeComboBoxDropDownBorderColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ComboBoxDropDownBorderColor, (model, colorHex) => {
			model.SetSelectedProfileComboDropDownAndDataGridSemanticColors(model.SelectedProfileComboBoxDropDownBackgroundColor, colorHex, model.SelectedProfileDataGridHeaderBackgroundColor, model.SelectedProfileDataGridHeaderForegroundColor, model.SelectedProfileDataGridSelectedRowForegroundColor);
		});
	}

	private async void btnPickThemeDataGridHeaderBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DataGridHeaderBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileComboDropDownAndDataGridSemanticColors(model.SelectedProfileComboBoxDropDownBackgroundColor, model.SelectedProfileComboBoxDropDownBorderColor, colorHex, model.SelectedProfileDataGridHeaderForegroundColor, model.SelectedProfileDataGridSelectedRowForegroundColor);
		});
	}

	private async void btnPickThemeDataGridHeaderForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DataGridHeaderForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileComboDropDownAndDataGridSemanticColors(model.SelectedProfileComboBoxDropDownBackgroundColor, model.SelectedProfileComboBoxDropDownBorderColor, model.SelectedProfileDataGridHeaderBackgroundColor, colorHex, model.SelectedProfileDataGridSelectedRowForegroundColor);
		});
	}

	private async void btnPickThemeDataGridSelectedRowForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.DataGridSelectedRowForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileComboDropDownAndDataGridSemanticColors(model.SelectedProfileComboBoxDropDownBackgroundColor, model.SelectedProfileComboBoxDropDownBorderColor, model.SelectedProfileDataGridHeaderBackgroundColor, model.SelectedProfileDataGridHeaderForegroundColor, colorHex);
		});
	}

	private async void btnPickThemeListBoxHoverBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListBoxItemHoverBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(colorHex, model.SelectedProfileListBoxSelectedBackgroundColor, model.SelectedProfileListBoxSelectedForegroundColor, model.SelectedProfileTreeViewHoverBackgroundColor, model.SelectedProfileTreeViewSelectedBackgroundColor, model.SelectedProfileTreeViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeListBoxSelectedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListBoxItemSelectedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(model.SelectedProfileListBoxHoverBackgroundColor, colorHex, model.SelectedProfileListBoxSelectedForegroundColor, model.SelectedProfileTreeViewHoverBackgroundColor, model.SelectedProfileTreeViewSelectedBackgroundColor, model.SelectedProfileTreeViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeListBoxSelectedForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListBoxItemSelectedForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(model.SelectedProfileListBoxHoverBackgroundColor, model.SelectedProfileListBoxSelectedBackgroundColor, colorHex, model.SelectedProfileTreeViewHoverBackgroundColor, model.SelectedProfileTreeViewSelectedBackgroundColor, model.SelectedProfileTreeViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeTreeViewHoverBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.TreeViewItemHoverBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(model.SelectedProfileListBoxHoverBackgroundColor, model.SelectedProfileListBoxSelectedBackgroundColor, model.SelectedProfileListBoxSelectedForegroundColor, colorHex, model.SelectedProfileTreeViewSelectedBackgroundColor, model.SelectedProfileTreeViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeTreeViewSelectedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.TreeViewItemSelectedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(model.SelectedProfileListBoxHoverBackgroundColor, model.SelectedProfileListBoxSelectedBackgroundColor, model.SelectedProfileListBoxSelectedForegroundColor, model.SelectedProfileTreeViewHoverBackgroundColor, colorHex, model.SelectedProfileTreeViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeTreeViewSelectedForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.TreeViewItemSelectedForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileListBoxAndTreeViewSemanticColors(model.SelectedProfileListBoxHoverBackgroundColor, model.SelectedProfileListBoxSelectedBackgroundColor, model.SelectedProfileListBoxSelectedForegroundColor, model.SelectedProfileTreeViewHoverBackgroundColor, model.SelectedProfileTreeViewSelectedBackgroundColor, colorHex);
		});
	}

	private async void btnPickThemeNavigationViewHoverBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.NavigationViewItemHoverBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(colorHex, model.SelectedProfileNavigationViewSelectedBackgroundColor, model.SelectedProfileNavigationViewSelectedForegroundColor, model.SelectedProfileListViewHoverBackgroundColor, model.SelectedProfileListViewSelectedBackgroundColor, model.SelectedProfileListViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeNavigationViewSelectedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.NavigationViewItemSelectedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(model.SelectedProfileNavigationViewHoverBackgroundColor, colorHex, model.SelectedProfileNavigationViewSelectedForegroundColor, model.SelectedProfileListViewHoverBackgroundColor, model.SelectedProfileListViewSelectedBackgroundColor, model.SelectedProfileListViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeNavigationViewSelectedForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.NavigationViewItemSelectedForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(model.SelectedProfileNavigationViewHoverBackgroundColor, model.SelectedProfileNavigationViewSelectedBackgroundColor, colorHex, model.SelectedProfileListViewHoverBackgroundColor, model.SelectedProfileListViewSelectedBackgroundColor, model.SelectedProfileListViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeListViewHoverBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListViewItemHoverBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(model.SelectedProfileNavigationViewHoverBackgroundColor, model.SelectedProfileNavigationViewSelectedBackgroundColor, model.SelectedProfileNavigationViewSelectedForegroundColor, colorHex, model.SelectedProfileListViewSelectedBackgroundColor, model.SelectedProfileListViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeListViewSelectedBackgroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListViewItemSelectedBackgroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(model.SelectedProfileNavigationViewHoverBackgroundColor, model.SelectedProfileNavigationViewSelectedBackgroundColor, model.SelectedProfileNavigationViewSelectedForegroundColor, model.SelectedProfileListViewHoverBackgroundColor, colorHex, model.SelectedProfileListViewSelectedForegroundColor);
		});
	}

	private async void btnPickThemeListViewSelectedForegroundColor_OnClick(object sender, RoutedEventArgs e) {
		await PickThemeProfileColor(profile => profile.ListViewItemSelectedForegroundColor, (model, colorHex) => {
			model.SetSelectedProfileNavigationViewAndListViewSemanticColors(model.SelectedProfileNavigationViewHoverBackgroundColor, model.SelectedProfileNavigationViewSelectedBackgroundColor, model.SelectedProfileNavigationViewSelectedForegroundColor, model.SelectedProfileListViewHoverBackgroundColor, model.SelectedProfileListViewSelectedBackgroundColor, colorHex);
		});
	}

	private async Task PickThemeProfileColor(Func<ThemeProfile, string> colorSelector, Action<PreferencesConfigViewModel, string> updateAction) {
		PreferencesConfigViewModel? model = GetViewModel();
		ThemeProfile? profile = model?.GetSelectedThemeProfile();
		if (model is null || profile is null) {
			return;
		}

		Color startColor = ParseColorOrDefault(colorSelector(profile), Colors.White);
		ColorPickerWindow wnd = new ColorPickerWindow() {
			DataContext = new ColorPickerViewModel() { Color = startColor }
		};

		if (await wnd.ShowCenteredDialog<bool>(TopLevel.GetTopLevel(this) as Visual) && wnd.DataContext is ColorPickerViewModel colorVm) {
			updateAction(model, ToHexArgb(colorVm.Color));
		}
	}

	private static Color ParseColorOrDefault(string colorValue, Color fallback) {
		try {
			return Color.Parse(colorValue);
		} catch {
			return fallback;
		}
	}

	private static string ToHexArgb(Color color) {
		return $"#{color.A:x2}{color.R:x2}{color.G:x2}{color.B:x2}";
	}
}
