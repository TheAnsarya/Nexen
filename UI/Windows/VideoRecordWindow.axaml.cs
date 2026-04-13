using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class VideoRecordWindow : NexenWindow {
	public VideoRecordWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private async void OnBrowseClick(object sender, RoutedEventArgs e) {
		VideoRecordConfigViewModel model = (VideoRecordConfigViewModel)DataContext!;
		bool isGif = model.Config.Codec == VideoCodec.GIF;

		string romName = EmuApi.GetRomInfo().GetRomName();
		string timestamp = DateTime.Now.ToString("yyyy-MM-dd_HH-mm-ss");
		string initFilename = $"{romName}_{timestamp}" + (isGif ? ".gif" : ".avi");
		string? filename = await FileDialogHelper.SaveFile(ConfigManager.AviFolder, initFilename, VisualRoot, isGif ? FileDialogHelper.GifExt : FileDialogHelper.AviExt);

		if (filename != null) {
			model.SavePath = filename;
		}
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		VideoRecordConfigViewModel model = (VideoRecordConfigViewModel)DataContext!;
		model.SaveConfig();

		RecordApi.AviRecord(model.SavePath, new RecordAviOptions() {
			Codec = model.Config.Codec,
			CompressionLevel = model.Config.CompressionLevel,
			RecordSystemHud = model.Config.RecordSystemHud,
			RecordInputHud = model.Config.RecordInputHud
		});

		Close(true);
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(false);
	}
}
