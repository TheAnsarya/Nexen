using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.StatusViews;
public partial class LynxStatusView : UserControl {
	public LynxStatusView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
