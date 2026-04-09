using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Config;
using Nexen.Utilities;
using Nexen.ViewModels;

namespace Nexen.Views; 
public partial class SnesConfigView : UserControl {
	public SnesConfigView() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void nudGsuSpeed_LostFocus(object? sender, RoutedEventArgs e) {
		if (DataContext is SnesConfigViewModel model) {
			//Clock speed must be a multiple of 100
			model.Config.GsuClockSpeed = (uint)Math.Round((double)model.Config.GsuClockSpeed / 100) * 100;
		}
	}
}
