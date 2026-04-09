using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Utilities;

namespace Nexen.Views; 
public partial class VideoConfigView : UserControl {
	public VideoConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void btnPreset_OnClick(object sender, RoutedEventArgs e) {
		((Button)sender).ContextMenu?.Open();
	}
}
