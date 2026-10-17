using System;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Windows;
public partial class MovieRecordWindow : NexenWindow {
	public MovieRecordWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private async void OnBrowseClick(object sender, RoutedEventArgs e) {
		if (DataContext is not MovieRecordConfigViewModel model) {
			await NexenMsgBox.Show(this, "UnexpectedError", MessageBoxButtons.OK, MessageBoxIcon.Error, "Movie recording configuration is not initialized.");
			return;
		}

		string? filename = await FileDialogHelper.SaveFile(ConfigManager.MovieFolder, EmuApi.GetRomInfo().GetRomName() + "." + FileDialogHelper.NexenMovieExt, VisualRoot, FileDialogHelper.NexenMovieExt);
		if (filename != null) {
			model.SavePath = filename;
		}
	}

	private async void Ok_OnClick(object sender, RoutedEventArgs e) {
		if (DataContext is not MovieRecordConfigViewModel model) {
			await NexenMsgBox.Show(this, "UnexpectedError", MessageBoxButtons.OK, MessageBoxIcon.Error, "Movie recording configuration is not initialized.");
			return;
		}

		if (string.IsNullOrWhiteSpace(model.SavePath)) {
			await NexenMsgBox.Show(this, "InvalidMoviePath", MessageBoxButtons.OK, MessageBoxIcon.Warning);
			return;
		}

		string? folderPath = Path.GetDirectoryName(model.SavePath);
		if (string.IsNullOrWhiteSpace(folderPath)) {
			await NexenMsgBox.Show(this, "InvalidMoviePath", MessageBoxButtons.OK, MessageBoxIcon.Warning);
			return;
		}

		try {
			Directory.CreateDirectory(folderPath);
		} catch (Exception ex) {
			await NexenMsgBox.Show(this, "MovieRecordStartError", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.Message);
			return;
		}

		model.SaveConfig();

		try {
			RecordApi.MovieRecord(new RecordMovieOptions(model.SavePath, model.Config.Author, model.Config.Description, model.Config.RecordFrom));
		} catch (Exception ex) {
			await NexenMsgBox.Show(this, "MovieRecordStartError", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.Message);
			return;
		}

		Close(true);
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(false);
	}
}
