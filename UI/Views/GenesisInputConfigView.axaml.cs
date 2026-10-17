using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views;

public partial class GenesisInputConfigView : UserControl {
	public GenesisInputConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
