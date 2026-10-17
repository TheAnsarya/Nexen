using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.Views;

public partial class GenesisEventViewerConfigView : UserControl {
	public GenesisEventViewerConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
