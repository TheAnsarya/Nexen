using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Views;

public partial class Atari2600ConfigView : UserControl {
	public Atari2600ConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}
}
