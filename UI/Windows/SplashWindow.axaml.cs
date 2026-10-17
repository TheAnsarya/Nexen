using System;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;

namespace Nexen.Windows;

/// <summary>
/// Splash screen window displayed during application startup while the main window initializes.
/// Shows the Nexen logo with a loading indicator.
/// </summary>
public partial class SplashWindow : Window {
	private readonly DispatcherTimer _progressTimer = new DispatcherTimer();
	private double _progressValue = 12;

	public SplashWindow() {
		InitializeComponent();
		_progressTimer.Interval = TimeSpan.FromMilliseconds(90);
		_progressTimer.Tick += (_, _) => {
			_progressValue += 5;
			if (_progressValue > 92) {
				_progressValue = 14;
			}

			ProgressBar? loadingBar = this.FindControl<ProgressBar>("LoadingBar");
			if (loadingBar is not null) {
				loadingBar.Value = _progressValue;
			}
		};
		_progressTimer.Start();
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

	protected override void OnClosed(EventArgs e) {
		_progressTimer.Stop();
		base.OnClosed(e);
	}
}
