using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
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
}
