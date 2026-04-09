using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views;

public partial class ChannelFConfigView : UserControl {
	public ChannelFConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
