using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class SelectStorageFolderWindow : NexenWindow {
	private SelectStorageFolderViewModel _model;

	public SelectStorageFolderWindow() {
		_model = new SelectStorageFolderViewModel() { StoreInUserProfile = ConfigManager.HomeFolder == ConfigManager.DefaultDocumentsFolder };
		DataContext = _model;

		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		if (_model.IsCopying) {
			e.Cancel = true;
		}

		base.OnClosing(e);
	}

	private async void StartProcess() {
		if (await _model.MigrateData()) {
			Close(true);
		}
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		StartProcess();
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(false);
	}
}
