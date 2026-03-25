using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.Views;
public class Atari2600EventViewerConfigView : UserControl {
	public Atari2600EventViewerConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
