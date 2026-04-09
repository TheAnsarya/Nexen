using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Localization;
using Nexen.Utilities;

namespace Nexen.Debugger.Windows;

/// <summary>
/// Progress window for Pansy export operations.
/// Shows progress, current operation, and statistics on completion.
/// </summary>
public partial class PansyExportProgressWindow : NexenWindow {
	private CancellationTokenSource? _cts;
	private readonly Stopwatch _stopwatch = new();
	private bool _isComplete;
	private ExportStatistics? _stats;

	public PansyExportProgressWindow() {
		InitializeComponent();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	/// <summary>
	/// Show the progress window and run an export operation.
	/// </summary>
	public static async Task<bool> ShowAndRun(
		Window parent,
		string romName,
		Func<IProgress<ExportProgress>, CancellationToken, Task<ExportStatistics>> exportAction) {
		var window = new PansyExportProgressWindow();
		window.SetRomName(romName);

		var tcs = new TaskCompletionSource<bool>();

		window.Closed += (_, _) => {
			if (!tcs.Task.IsCompleted)
				tcs.TrySetResult(window._isComplete);
		};

		_ = window.ShowDialog(parent);

		// Start the export operation
		window._cts = new CancellationTokenSource();
		window._stopwatch.Start();

		var progress = new Progress<ExportProgress>(p => Dispatcher.UIThread.Post(() => window.UpdateProgress(p)));

		try {
			var stats = await exportAction(progress, window._cts.Token);
			window._stopwatch.Stop();
			window._stats = stats;
			window._isComplete = true;

			await Dispatcher.UIThread.InvokeAsync(() => window.ShowCompletion(stats));

			return await tcs.Task;
		} catch (OperationCanceledException) {
			window.Close();
			return false;
		} catch (Exception ex) {
			await Dispatcher.UIThread.InvokeAsync(() => window.ShowError(ex.Message));
			return false;
		}
	}

	private void SetRomName(string romName) {
		this.FindControl<TextBlock>("txtRomName")!.Text = romName;
	}

	private void UpdateProgress(ExportProgress progress) {
		var progressMain = this.FindControl<ProgressBar>("progressMain")!;
		var txtProgressPercent = this.FindControl<TextBlock>("txtProgressPercent")!;
		var txtStatus = this.FindControl<TextBlock>("txtStatus")!;
		var txtCurrentOp = this.FindControl<TextBlock>("txtCurrentOp")!;
		var progressDetail = this.FindControl<ProgressBar>("progressDetail")!;

		progressMain.Value = progress.OverallPercent;
		txtProgressPercent.Text = $"{progress.OverallPercent}%";
		txtStatus.Text = progress.StatusMessage;

		if (!string.IsNullOrEmpty(progress.CurrentOperation)) {
			txtCurrentOp.Text = progress.CurrentOperation;
			progressDetail.IsIndeterminate = progress.DetailIsIndeterminate;
			if (!progress.DetailIsIndeterminate) {
				progressDetail.Value = progress.DetailPercent;
			}
		}
	}

	private void ShowCompletion(ExportStatistics stats) {
		var txtTitle = this.FindControl<TextBlock>("txtTitle")!;
		var txtStatus = this.FindControl<TextBlock>("txtStatus")!;
		var progressMain = this.FindControl<ProgressBar>("progressMain")!;
		var txtProgressPercent = this.FindControl<TextBlock>("txtProgressPercent")!;
		var progressDetail = this.FindControl<ProgressBar>("progressDetail")!;
		var txtCurrentOp = this.FindControl<TextBlock>("txtCurrentOp")!;
		var pnlStats = this.FindControl<Border>("pnlStats")!;
		var btnCancel = this.FindControl<Button>("btnCancel")!;
		var btnClose = this.FindControl<Button>("btnClose")!;

		txtTitle.Text = ResourceHelper.GetMessage("lblExportComplete");
		txtStatus.Text = ResourceHelper.GetMessage("lblExportSuccessful");
		progressMain.Value = 100;
		txtProgressPercent.Text = "100%";
		progressDetail.IsVisible = false;
		txtCurrentOp.IsVisible = false;

		// Show statistics
		pnlStats.IsVisible = true;
		this.FindControl<TextBlock>("txtSymbolCount")!.Text = stats.SymbolCount.ToString("N0");
		this.FindControl<TextBlock>("txtCommentCount")!.Text = stats.CommentCount.ToString("N0");
		this.FindControl<TextBlock>("txtCodeBytes")!.Text = FormatBytes(stats.CodeBytes);
		this.FindControl<TextBlock>("txtDataBytes")!.Text = FormatBytes(stats.DataBytes);
		this.FindControl<TextBlock>("txtFileSize")!.Text = FormatBytes(stats.FileSize);
		this.FindControl<TextBlock>("txtExportTime")!.Text = $"{_stopwatch.ElapsedMilliseconds}ms";

		// Swap buttons
		btnCancel.IsVisible = false;
		btnClose.IsVisible = true;
	}

	private void ShowError(string message) {
		var txtTitle = this.FindControl<TextBlock>("txtTitle")!;
		var txtStatus = this.FindControl<TextBlock>("txtStatus")!;
		var progressDetail = this.FindControl<ProgressBar>("progressDetail")!;
		var btnCancel = this.FindControl<Button>("btnCancel")!;
		var btnClose = this.FindControl<Button>("btnClose")!;

		txtTitle.Text = ResourceHelper.GetMessage("lblExportFailed");
		txtStatus.Text = message;
		progressDetail.IsVisible = false;

		btnCancel.IsVisible = false;
		btnClose.IsVisible = true;
	}

	private static string FormatBytes(long bytes) {
		if (bytes < 1024)
			return $"{bytes} B";
		if (bytes < 1024 * 1024)
			return $"{bytes / 1024.0:F1} KB";
		return $"{bytes / (1024.0 * 1024.0):F2} MB";
	}

	private void Cancel_OnClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_cts?.Cancel();
		Close();
	}

	private void Close_OnClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		Close();
	}
}

/// <summary>
/// Progress information for export operations.
/// </summary>
public partial class ExportProgress {
	public int OverallPercent { get; set; }
	public string StatusMessage { get; set; } = "";
	public string? CurrentOperation { get; set; }
	public bool DetailIsIndeterminate { get; set; } = true;
	public int DetailPercent { get; set; }
}

/// <summary>
/// Statistics from a completed export operation.
/// </summary>
public partial class ExportStatistics {
	public int SymbolCount { get; set; }
	public int CommentCount { get; set; }
	public long CodeBytes { get; set; }
	public long DataBytes { get; set; }
	public long FileSize { get; set; }
	public TimeSpan Duration { get; set; }
}
