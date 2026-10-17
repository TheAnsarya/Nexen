using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.Localization;
using Nexen.TAS;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// Settings for the greenzone savestate manager.
/// </summary>
public sealed partial class GreenzoneSettings {
	public int CaptureInterval { get; set; } = 60;
	public int MaxSavestates { get; set; } = 1000;
	public long MaxMemoryMB { get; set; } = 256;
	public int RingBufferSize { get; set; } = 120;
	public bool CompressionEnabled { get; set; } = true;
	public int CompressionThreshold { get; set; } = 300;
	public bool ClearRequested { get; set; }
}

/// <summary>
/// Dialog for configuring greenzone settings.
/// </summary>
public partial class GreenzoneSettingsDialog : NexenWindow {
	private bool _cancelled = true;
	private bool _clearRequested;

	public GreenzoneSettingsDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void OnOkClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_cancelled = false;
		Close();
	}

	private void OnCancelClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_cancelled = true;
		Close();
	}

	private void OnClearClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_clearRequested = true;
		_cancelled = false;
		Close();
	}

	/// <summary>
	/// Shows the greenzone settings dialog.
	/// </summary>
	/// <param name="parent">Parent window.</param>
	/// <param name="greenzone">Current greenzone to read settings from.</param>
	/// <returns>New settings, or null if cancelled.</returns>
	public static Task<GreenzoneSettings?> ShowAsync(Window? parent, GreenzoneManager greenzone) {
		var dialog = new GreenzoneSettingsDialog();

		// Populate current values
		dialog.nudInterval.Value = greenzone.CaptureInterval;
		dialog.nudMaxStates.Value = greenzone.MaxSavestates;
		dialog.nudMaxMemoryMB.Value = greenzone.MaxMemoryBytes / (1024 * 1024);
		dialog.nudRingBuffer.Value = greenzone.RingBufferSize;
		dialog.chkCompression.IsChecked = greenzone.CompressionEnabled;
		dialog.nudCompThreshold.Value = greenzone.CompressionThreshold;

		// Status display
		dialog.txtStateCount.Text = greenzone.SavestateCount.ToString("N0");
		dialog.txtMemory.Text = ResourceHelper.GetMessage("GreenzoneMemoryUsageMB", (greenzone.TotalMemoryUsage / (1024.0 * 1024.0)).ToString("F1"));

		var tcs = new TaskCompletionSource<GreenzoneSettings?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
			} else {
				tcs.TrySetResult(new GreenzoneSettings {
					CaptureInterval = (int)(dialog.nudInterval.Value ?? 60),
					MaxSavestates = (int)(dialog.nudMaxStates.Value ?? 1000),
					MaxMemoryMB = (long)(dialog.nudMaxMemoryMB.Value ?? 256),
					RingBufferSize = (int)(dialog.nudRingBuffer.Value ?? 120),
					CompressionEnabled = dialog.chkCompression.IsChecked ?? true,
					CompressionThreshold = (int)(dialog.nudCompThreshold.Value ?? 300),
					ClearRequested = dialog._clearRequested
				});
			}
		};

		parent ??= ApplicationHelper.GetActiveOrMainWindow();
		if (parent != null) {
			if (!OperatingSystem.IsWindows()) {
				dialog.Opened += (_, _) => WindowExtensions.CenterWindow(dialog, parent);
			}
			dialog.ShowDialog(parent);
		}

		return tcs.Task;
	}
}
