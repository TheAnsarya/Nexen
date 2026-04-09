using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Debugger.StatusViews;

public partial class ChannelFStatusView : UserControl {
	public ChannelFStatusView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
