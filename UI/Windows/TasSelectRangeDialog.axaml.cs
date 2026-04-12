using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// Dialog for selecting an inclusive frame range.
/// </summary>
public partial class TasSelectRangeDialog : NexenWindow {
	private bool _cancelled = true;
	private int _maxFrame;

	public TasSelectRangeDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		txtFrom.Focus();
		txtFrom.SelectAll();
	}

	private void OnOkClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (Validate()) {
			_cancelled = false;
			Close();
		}
	}

	private void OnCancelClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_cancelled = true;
		Close();
	}

	private bool Validate() {
		if (!int.TryParse(txtFrom.Text?.Trim(), out int fromFrame)) {
			txtError.Text = "Please enter a valid start frame.";
			txtError.IsVisible = true;
			return false;
		}

		if (!int.TryParse(txtTo.Text?.Trim(), out int toFrame)) {
			txtError.Text = "Please enter a valid end frame.";
			txtError.IsVisible = true;
			return false;
		}

		if (fromFrame < 0 || fromFrame > _maxFrame || toFrame < 0 || toFrame > _maxFrame) {
			txtError.Text = $"Frames must be between 0 and {_maxFrame:N0}.";
			txtError.IsVisible = true;
			return false;
		}

		txtError.IsVisible = false;
		return true;
	}

	/// <summary>
	/// Shows a dialog for selecting an inclusive frame range.
	/// </summary>
	public static Task<(int FromFrame, int ToFrame)?> ShowAsync(Window? parent, int maxFrame, int defaultFrom, int defaultTo) {
		var dialog = new TasSelectRangeDialog {
			_maxFrame = Math.Max(0, maxFrame)
		};

		dialog.txtFrom.Text = Math.Clamp(defaultFrom, 0, dialog._maxFrame).ToString();
		dialog.txtTo.Text = Math.Clamp(defaultTo, 0, dialog._maxFrame).ToString();

		var tcs = new TaskCompletionSource<(int FromFrame, int ToFrame)?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
				return;
			}

			int.TryParse(dialog.txtFrom.Text?.Trim(), out int fromFrame);
			int.TryParse(dialog.txtTo.Text?.Trim(), out int toFrame);
			tcs.TrySetResult((fromFrame, toFrame));
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
