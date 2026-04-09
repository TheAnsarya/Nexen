using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.StatusViews; 
public partial class NesStatusView : UserControl {
	public NesStatusView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
