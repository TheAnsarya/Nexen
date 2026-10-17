using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// A reusable input dialog for the TAS Editor.
/// Supports numeric frame input and text input modes.
/// </summary>
public partial class TasInputDialog : NexenWindow {
	private bool _cancelled = true;
	private bool _numericOnly;
	private int _minValue;
	private int _maxValue;

	public TasInputDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);
		txtInput.Focus();
		txtInput.SelectAll();
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
		if (!_numericOnly) {
			return true;
		}

		string text = txtInput.Text?.Trim() ?? "";
		if (!int.TryParse(text, out int value)) {
			txtError.Text = "Please enter a valid number.";
			txtError.IsVisible = true;
			return false;
		}

		if (value < _minValue || value > _maxValue) {
			txtError.Text = $"Value must be between {_minValue:N0} and {_maxValue:N0}.";
			txtError.IsVisible = true;
			return false;
		}

		txtError.IsVisible = false;
		return true;
	}

	/// <summary>
	/// Shows a numeric input dialog and returns the entered value, or null if cancelled.
	/// </summary>
	/// <param name="parent">Parent window.</param>
	/// <param name="title">Dialog title.</param>
	/// <param name="prompt">Prompt text shown above the input field.</param>
	/// <param name="defaultValue">Initial value in the input field.</param>
	/// <param name="minValue">Minimum allowed value.</param>
	/// <param name="maxValue">Maximum allowed value.</param>
	/// <returns>The entered number, or null if cancelled.</returns>
	public static Task<int?> ShowNumericAsync(Window? parent, string title, string prompt, int defaultValue, int minValue, int maxValue) {
		var dialog = new TasInputDialog();
		dialog.Title = title;
		dialog.txtPrompt.Text = prompt;
		dialog.txtInput.Text = defaultValue.ToString();
		dialog.txtInput.PlaceholderText = $"{minValue:N0} - {maxValue:N0}";
		dialog._numericOnly = true;
		dialog._minValue = minValue;
		dialog._maxValue = maxValue;

		var tcs = new TaskCompletionSource<int?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
			} else {
				int.TryParse(dialog.txtInput.Text?.Trim(), out int result);
				tcs.TrySetResult(result);
			}
		};

		parent ??= ApplicationHelper.GetActiveOrMainWindow();
		if (parent != null) {
			if (!OperatingSystem.IsWindows()) {
				dialog.Opened += (_, _) => WindowExtensions.CenterWindow(dialog, parent);
			}
			dialog.ShowDialog(parent);
		} else {
			tcs.TrySetResult(null);
		}

		return tcs.Task;
	}

	/// <summary>
	/// Shows a text input dialog and returns the entered text, or null if cancelled.
	/// </summary>
	/// <param name="parent">Parent window.</param>
	/// <param name="title">Dialog title.</param>
	/// <param name="prompt">Prompt text shown above the input field.</param>
	/// <param name="defaultValue">Initial value in the input field.</param>
	/// <returns>The entered text, or null if cancelled.</returns>
	public static Task<string?> ShowTextAsync(Window? parent, string title, string prompt, string defaultValue = "") {
		var dialog = new TasInputDialog();
		dialog.Title = title;
		dialog.txtPrompt.Text = prompt;
		dialog.txtInput.Text = defaultValue;
		dialog._numericOnly = false;

		var tcs = new TaskCompletionSource<string?>();
		dialog.Closed += (_, _) => {
			if (dialog._cancelled) {
				tcs.TrySetResult(null);
			} else {
				tcs.TrySetResult(dialog.txtInput.Text?.Trim() ?? "");
			}
		};

		parent ??= ApplicationHelper.GetActiveOrMainWindow();
		if (parent != null) {
			if (!OperatingSystem.IsWindows()) {
				dialog.Opened += (_, _) => WindowExtensions.CenterWindow(dialog, parent);
			}
			dialog.ShowDialog(parent);
		} else {
			tcs.TrySetResult(null);
		}

		return tcs.Task;
	}
}
