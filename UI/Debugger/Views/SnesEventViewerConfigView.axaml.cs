using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.Views; 
public partial class SnesEventViewerConfigView : UserControl {
	public SnesEventViewerConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
