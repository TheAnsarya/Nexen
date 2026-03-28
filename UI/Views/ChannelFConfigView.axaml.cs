using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views;

public class ChannelFConfigView : UserControl {
	public ChannelFConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
