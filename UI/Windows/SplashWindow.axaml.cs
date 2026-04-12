using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace Nexen.Windows;

/// <summary>
/// Splash screen window displayed during application startup while the main window initializes.
/// Shows the Nexen logo with a loading indicator.
/// </summary>
public partial class SplashWindow : Window {
	public SplashWindow() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	/// <summary>
	/// Updates the status text displayed below the loading indicator.
	/// </summary>
	public void SetStatus(string text) {
		var statusText = this.FindControl<TextBlock>("StatusText");
		if (statusText is not null) {
			statusText.Text = text;
		}
	}
}
