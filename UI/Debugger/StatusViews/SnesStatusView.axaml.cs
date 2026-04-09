using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.StatusViews; 
public partial class SnesStatusView : UserControl {
	public SnesStatusView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
