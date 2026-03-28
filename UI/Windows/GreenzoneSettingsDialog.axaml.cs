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
public sealed class GreenzoneSettings {
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
	private NumericUpDown _nudInterval = null!;
	private NumericUpDown _nudMaxStates = null!;
	private NumericUpDown _nudMaxMemoryMB = null!;
	private NumericUpDown _nudRingBuffer = null!;
	private CheckBox _chkCompression = null!;
	private NumericUpDown _nudCompThreshold = null!;
	private bool _cancelled = true;
	private bool _clearRequested;

	public GreenzoneSettingsDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
		_nudInterval = this.FindControl<NumericUpDown>("nudInterval") ?? throw new InvalidOperationException("nudInterval control is missing.");
		_nudMaxStates = this.FindControl<NumericUpDown>("nudMaxStates") ?? throw new InvalidOperationException("nudMaxStates control is missing.");
		_nudMaxMemoryMB = this.FindControl<NumericUpDown>("nudMaxMemoryMB") ?? throw new InvalidOperationException("nudMaxMemoryMB control is missing.");
		_nudRingBuffer = this.FindControl<NumericUpDown>("nudRingBuffer") ?? throw new InvalidOperationException("nudRingBuffer control is missing.");
		_chkCompression = this.FindControl<CheckBox>("chkCompression") ?? throw new InvalidOperationException("chkCompression control is missing.");
		_nudCompThreshold = this.FindControl<NumericUpDown>("nudCompThreshold") ?? throw new InvalidOperationException("nudCompThreshold control is missing.");
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
		dialog._nudInterval.Value = greenzone.CaptureInterval;
		dialog._nudMaxStates.Value = greenzone.MaxSavestates;
		dialog._nudMaxMemoryMB.Value = greenzone.MaxMemoryBytes / (1024 * 1024);
		dialog._nudRingBuffer.Value = greenzone.RingBufferSize;
		dialog._chkCompression.IsChecked = greenzone.CompressionEnabled;
		dialog._nudCompThreshold.Value = greenzone.CompressionThreshold;

		// Status display
		TextBlock? stateCount = dialog.FindControl<TextBlock>("txtStateCount");
		TextBlock? memory = dialog.FindControl<TextBlock>("txtMemory");
		if (stateCount != null) {
			stateCount.Text = greenzone.SavestateCount.ToString("N0");
		}

		if (memory != null) {
			memory.Text = ResourceHelper.GetMessage("GreenzoneMemoryUsageMB", (greenzone.TotalMemoryUsage / (1024.0 * 1024.0)).ToString("F1"));
		}

		var tcs = new TaskCompletionSource<GreenzoneSettings?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
			} else {
				tcs.TrySetResult(new GreenzoneSettings {
					CaptureInterval = (int)(dialog._nudInterval.Value ?? 60),
					MaxSavestates = (int)(dialog._nudMaxStates.Value ?? 1000),
					MaxMemoryMB = (long)(dialog._nudMaxMemoryMB.Value ?? 256),
					RingBufferSize = (int)(dialog._nudRingBuffer.Value ?? 120),
					CompressionEnabled = dialog._chkCompression.IsChecked ?? true,
					CompressionThreshold = (int)(dialog._nudCompThreshold.Value ?? 300),
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
