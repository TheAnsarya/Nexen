using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Nexen.Utilities;

namespace Nexen.Windows;

/// <summary>
/// Dialog shown when user presses a joypad button during movie playback.
/// Allows the user to fork the playback, edit the movie, or continue playing.
/// </summary>
public partial class PlaybackInterruptDialog : NexenWindow {
	private PlaybackInterruptAction _result = PlaybackInterruptAction.Continue;

	/// <summary>
	/// The frame number where playback was interrupted.
	/// </summary>
	public int InterruptFrame { get; private set; }

	/// <summary>
	/// The total frame count of the movie.
	/// </summary>
	public int TotalFrames { get; private set; }

	public PlaybackInterruptDialog() {
		InitializeComponent();
#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	/// <summary>
	/// Creates a new interrupt dialog with frame information.
	/// </summary>
	/// <param name="interruptFrame">The frame where playback was interrupted.</param>
	/// <param name="totalFrames">The total number of frames in the movie.</param>
	public PlaybackInterruptDialog(int interruptFrame, int totalFrames) : this() {
		InterruptFrame = interruptFrame;
		TotalFrames = totalFrames;
		UpdateFrameInfo();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void UpdateFrameInfo() {
		txtFrameInfo.Text = $"Frame {InterruptFrame + 1:N0} of {TotalFrames:N0}";
	}

	protected override void OnKeyDown(KeyEventArgs e) {
		base.OnKeyDown(e);

		if (e.Key == Key.Escape) {
			_result = PlaybackInterruptAction.Continue;
			Close(_result);
			e.Handled = true;
		}
	}

	private void OnForkClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_result = PlaybackInterruptAction.Fork;
		Close(_result);
	}

	private void OnEditClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_result = PlaybackInterruptAction.Edit;
		Close(_result);
	}

	private void OnContinueClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_result = PlaybackInterruptAction.Continue;
		Close(_result);
	}

	/// <summary>
	/// Shows the interrupt dialog and returns the user's choice.
	/// </summary>
	/// <param name="parent">Parent window.</param>
	/// <param name="interruptFrame">Frame where playback was interrupted.</param>
	/// <param name="totalFrames">Total frame count.</param>
	/// <returns>The action selected by the user.</returns>
	public static Task<PlaybackInterruptAction> ShowDialog(Window? parent, int interruptFrame, int totalFrames) {
		var dialog = new PlaybackInterruptDialog(interruptFrame, totalFrames);
		var tcs = new TaskCompletionSource<PlaybackInterruptAction>();

		dialog.Closed += (_, _) => tcs.TrySetResult(dialog._result);

		parent ??= ApplicationHelper.GetActiveOrMainWindow();

		if (parent != null) {
			if (!OperatingSystem.IsWindows()) {
				// Fix for CenterOwner on X11 with SizeToContent
				dialog.Opened += (_, _) => WindowExtensions.CenterWindow(dialog, parent);
			}

			dialog.ShowDialog(parent);
		}

		return tcs.Task;
	}
}

/// <summary>
/// Actions available when playback is interrupted by user input.
/// </summary>
public enum PlaybackInterruptAction {
	/// <summary>
	/// Fork the movie from the current frame and start recording.
	/// </summary>
	Fork,

	/// <summary>
	/// Open the TAS editor at the current frame for manual editing.
	/// </summary>
	Edit,

	/// <summary>
	/// Continue playback, ignoring the user's input.
	/// </summary>
	Continue
}
