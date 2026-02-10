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
public class PreferencesConfigView : UserControl {
	public PreferencesConfigView() {
		InitializeComponent();
		UpdateMigrationStatus();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void btnResetLagCounter_OnClick(object sender, RoutedEventArgs e) {
		InputApi.ResetLagCounter();
	}

	private void btnChangeStorageFolder_OnClick(object sender, RoutedEventArgs e) {
		ShowSelectFolderWindow();
	}

	private async void ShowSelectFolderWindow() {
		SelectStorageFolderWindow wnd = new();
		if (await wnd.ShowCenteredDialog<bool>(this.GetVisualRoot() as Visual)) {
			(this.GetVisualRoot() as Window)?.Close();
			ApplicationHelper.GetMainWindow()?.Close();
			ConfigManager.RestartNexen();
		}
	}

	private void UpdateMigrationStatus() {
		var (legacyFiles, gameCount) = GameDataManager.GetCleanupStatus();
		var statusLabel = this.FindControl<TextBlock>("lblMigrationStatus");
		if (statusLabel is not null) {
			if (legacyFiles > 0) {
				statusLabel.Text = $"{legacyFiles} legacy file(s) from {gameCount} game(s) can be cleaned up";
			} else {
				statusLabel.Text = "No legacy files to clean up";
			}
		}
	}

	private async void btnCleanupLegacyData_OnClick(object sender, RoutedEventArgs e) {
		var (legacyFiles, gameCount) = GameDataManager.GetCleanupStatus();
		if (legacyFiles == 0) {
			await NexenMsgBox.Show(
				this.GetVisualRoot() as Window,
				"NoLegacyFilesToCleanup",
				MessageBoxButtons.OK,
				MessageBoxIcon.Info
			);
			return;
		}

		var result = await NexenMsgBox.Show(
			this.GetVisualRoot() as Window,
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
				this.GetVisualRoot() as Window,
				"CleanupComplete",
				MessageBoxButtons.OK,
				MessageBoxIcon.Info,
				cleaned.ToString()
			);
		}
	}
}
