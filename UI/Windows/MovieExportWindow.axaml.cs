using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Mesen.Config;
using Mesen.Interop;
using Mesen.Utilities;
using Mesen.ViewModels;
using System.Collections.Generic;
using System.IO;

namespace Mesen.Windows
{
	public class MovieExportWindow : MesenWindow
	{
		public MovieExportWindow()
		{
			InitializeComponent();
#if DEBUG
			this.AttachDevTools();
#endif
		}

		private void InitializeComponent()
		{
			AvaloniaXamlLoader.Load(this);
		}

		private async void OnBrowseSourceClick(object sender, RoutedEventArgs e)
		{
			MovieExportViewModel model = (MovieExportViewModel)DataContext!;

			string? filename = await FileDialogHelper.OpenFile(ConfigManager.MovieFolder, VisualRoot, FileDialogHelper.MesenMovieExt);
			if(filename != null) {
				model.SourcePath = filename;
			}
		}

		private async void OnBrowseExportClick(object sender, RoutedEventArgs e)
		{
			MovieExportViewModel model = (MovieExportViewModel)DataContext!;
			
			string extension = model.GetSelectedExtension();
			string initialName = Path.GetFileNameWithoutExtension(model.SourcePath) + extension;
			
			string? filename = await FileDialogHelper.SaveFile(ConfigManager.MovieFolder, initialName, VisualRoot, extension.TrimStart('.'));
			if(filename != null) {
				model.ExportPath = filename;
			}
		}

		private async void Ok_OnClick(object sender, RoutedEventArgs e)
		{
			MovieExportViewModel model = (MovieExportViewModel)DataContext!;
			
			bool success = await model.Export();
			if(success) {
				Close(true);
			}
		}

		private void Cancel_OnClick(object sender, RoutedEventArgs e)
		{
			Close(false);
		}
	}
}
