using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Interop;
using Nexen.ViewModels;

namespace Nexen.Windows; 
public partial class NetplayStartServerWindow : NexenWindow {
	public NetplayStartServerWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void Ok_OnClick(object sender, RoutedEventArgs e) {
		NetplayConfig cfg = (NetplayConfig)DataContext!;
		ConfigManager.Config.Netplay = cfg.Clone();

		Close(true);

		NetplayApi.StartServer(cfg.ServerPort, cfg.ServerPassword);
	}

	private void Cancel_OnClick(object sender, RoutedEventArgs e) {
		Close(false);
	}
}
