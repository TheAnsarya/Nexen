using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views;

public class Atari2600InputConfigView : UserControl {
	public Atari2600InputConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
