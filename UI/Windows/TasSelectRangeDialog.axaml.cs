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
	private TextBox _txtFrom = null!;
	private TextBox _txtTo = null!;
	private TextBlock _txtError = null!;
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
		_txtFrom = this.FindControl<TextBox>("txtFrom")!;
		_txtTo = this.FindControl<TextBox>("txtTo")!;
		_txtError = this.FindControl<TextBlock>("txtError")!;
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		_txtFrom.Focus();
		_txtFrom.SelectAll();
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
		if (!int.TryParse(_txtFrom.Text?.Trim(), out int fromFrame)) {
			_txtError.Text = "Please enter a valid start frame.";
			_txtError.IsVisible = true;
			return false;
		}

		if (!int.TryParse(_txtTo.Text?.Trim(), out int toFrame)) {
			_txtError.Text = "Please enter a valid end frame.";
			_txtError.IsVisible = true;
			return false;
		}

		if (fromFrame < 0 || fromFrame > _maxFrame || toFrame < 0 || toFrame > _maxFrame) {
			_txtError.Text = $"Frames must be between 0 and {_maxFrame:N0}.";
			_txtError.IsVisible = true;
			return false;
		}

		_txtError.IsVisible = false;
		return true;
	}

	/// <summary>
	/// Shows a dialog for selecting an inclusive frame range.
	/// </summary>
	public static Task<(int FromFrame, int ToFrame)?> ShowAsync(Window? parent, int maxFrame, int defaultFrom, int defaultTo) {
		var dialog = new TasSelectRangeDialog {
			_maxFrame = Math.Max(0, maxFrame)
		};

		dialog._txtFrom.Text = Math.Clamp(defaultFrom, 0, dialog._maxFrame).ToString();
		dialog._txtTo.Text = Math.Clamp(defaultTo, 0, dialog._maxFrame).ToString();

		var tcs = new TaskCompletionSource<(int FromFrame, int ToFrame)?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
				return;
			}

			int.TryParse(dialog._txtFrom.Text?.Trim(), out int fromFrame);
			int.TryParse(dialog._txtTo.Text?.Trim(), out int toFrame);
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
