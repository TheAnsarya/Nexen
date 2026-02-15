using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Nexen.Controls;
using Nexen.Interop;
using Nexen.MovieConverter;
using Nexen.ViewModels;

namespace Nexen.Windows;

/// <summary>
/// TAS Editor window for frame-by-frame movie editing.
/// </summary>
public class TasEditorWindow : NexenWindow, IDisposable {
	private ListBox _frameList = null!;
	private PianoRollControl _pianoRoll = null!;
	private NotificationListener? _notificationListener;
	private bool _disposed;
	private bool _showingInterruptDialog;

	public TasEditorWindow() {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
		DataContext = new TasEditorViewModel();

		if (DataContext is TasEditorViewModel vm) {
			vm.InitActions(this);
		}

		SetupPianoRollEvents();
		SetupNotificationListener();
	}

	public TasEditorWindow(TasEditorViewModel viewModel) {
		InitializeComponent();
#if DEBUG
		this.AttachDevTools();
#endif
		DataContext = viewModel;
		viewModel.InitActions(this);

		SetupPianoRollEvents();
		SetupNotificationListener();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
		_frameList = this.FindControl<ListBox>("FrameList")!;
		_pianoRoll = this.FindControl<PianoRollControl>("PianoRoll")!;
	}

	/// <summary>
	/// Sets up event handlers for the piano roll control.
	/// </summary>
	private void SetupPianoRollEvents() {
		_pianoRoll.CellClicked += OnPianoRollCellClicked;
		_pianoRoll.CellsPainted += OnPianoRollCellsPainted;
		_pianoRoll.SelectionChanged += OnPianoRollSelectionChanged;
	}

	/// <summary>
	/// Sets up the notification listener for emulation events.
	/// </summary>
	private void SetupNotificationListener() {
		if (Design.IsDesignMode) {
			return;
		}

		_notificationListener = new NotificationListener();
		_notificationListener.OnNotification += OnEmulatorNotification;
	}

	/// <summary>
	/// Handles emulator notifications for movie playback integration.
	/// </summary>
	private void OnEmulatorNotification(NotificationEventArgs e) {
		switch (e.NotificationType) {
			case ConsoleNotificationType.PpuFrameDone:
				HandleFrameDone();
				break;

			case ConsoleNotificationType.GamePaused:
				Dispatcher.UIThread.Post(() => {
					if (ViewModel != null) {
						ViewModel.IsPlaying = false;
						ViewModel.StatusMessage = "Paused";
					}
				});
				break;

			case ConsoleNotificationType.GameResumed:
				Dispatcher.UIThread.Post(() => {
					if (ViewModel?.IsPlaying == true) {
						ViewModel.StatusMessage = "Playing...";
					}
				});
				break;

			case ConsoleNotificationType.StateLoaded:
				Dispatcher.UIThread.Post(() => {
					// Update frame position after state load
					ViewModel?.RefreshFrames();
				});
				break;
		}
	}

	/// <summary>
	/// Handles the frame done notification - feeds movie input to emulator.
	/// </summary>
	private void HandleFrameDone() {
		var vm = ViewModel;
		if (vm?.IsPlaying != true || vm.Movie == null) {
			return;
		}

		int frame = vm.PlaybackFrame;
		var movie = vm.Movie;

		// Check if we've reached the end
		if (frame >= movie.InputFrames.Count) {
			Dispatcher.UIThread.Post(() => {
				vm.StopPlayback();
				vm.StatusMessage = "Playback finished";
			});
			return;
		}

		// Check for user input interrupt (if enabled and not already showing dialog)
		if (vm.InterruptOnInput && !_showingInterruptDialog && !vm.Recorder.IsRecording) {
			var pressedKeys = InputApi.GetPressedKeys();
			if (pressedKeys.Count > 0) {
				_showingInterruptDialog = true;
				Dispatcher.UIThread.Post(async () => {
					await ShowInputInterruptDialog(vm, frame, movie.InputFrames.Count);
				});
				return; // Don't process this frame until user decides
			}
		}

		// Feed input to emulator for each controller
		var inputFrame = movie.InputFrames[frame];
		for (int i = 0; i < inputFrame.Controllers.Length && i < 4; i++) {
			var ctrl = inputFrame.Controllers[i];
			if (ctrl != null) {
				FeedControllerInput(i, ctrl);
			}
		}

		// Advance to next frame
		Dispatcher.UIThread.Post(() => {
			if (vm.IsPlaying) {
				vm.PlaybackFrame++;
				// Update UI less frequently for performance
				if (frame % 10 == 0 || frame == movie.InputFrames.Count - 1) {
					vm.SelectedFrameIndex = vm.PlaybackFrame;
					vm.StatusMessage = $"Frame {vm.PlaybackFrame + 1} / {movie.InputFrames.Count}";
				}
			}
		});
	}

	/// <summary>
	/// Shows the playback interrupt dialog when user presses input during playback.
	/// </summary>
	private async System.Threading.Tasks.Task ShowInputInterruptDialog(TasEditorViewModel vm, int currentFrame, int totalFrames) {
		try {
			// Pause emulation while showing dialog
			EmuApi.Pause();
			vm.IsPlaying = false;

			var result = await PlaybackInterruptDialog.ShowDialog(this, currentFrame, totalFrames);

			// Let ViewModel handle the action (Fork/Edit/Continue)
			await vm.HandleInterruptActionAsync(result, currentFrame);
		} finally {
			_showingInterruptDialog = false;
		}
	}

	/// <summary>
	/// Feeds a controller input to the emulator.
	/// </summary>
	private static void FeedControllerInput(int controllerIndex, ControllerInput input) {
		var state = new DebugControllerState {
			A = input.A,
			B = input.B,
			X = input.X,
			Y = input.Y,
			L = input.L,
			R = input.R,
			Up = input.Up,
			Down = input.Down,
			Left = input.Left,
			Right = input.Right,
			Select = input.Select,
			Start = input.Start,
			// Genesis buttons map to U/D
			U = input.C,
			D = input.Z
		};

		DebugApi.SetInputOverrides((uint)controllerIndex, state);
	}

	/// <summary>
	/// Cleans up resources.
	/// </summary>
	public void Dispose() {
		if (_disposed) {
			return;
		}

		_disposed = true;
		_notificationListener?.Dispose();
		_notificationListener = null;

		if (DataContext is IDisposable disposable) {
			disposable.Dispose();
		}
	}

	/// <summary>
	/// Called when the window is closed.
	/// </summary>
	protected override void OnClosed(EventArgs e) {
		base.OnClosed(e);
		Dispose();
	}

	/// <summary>
	/// Gets the view model for this window.
	/// </summary>
	public TasEditorViewModel? ViewModel => DataContext as TasEditorViewModel;

	/// <summary>
	/// Handles button clicks for controller input.
	/// Toggles the button state for the currently selected frame.
	/// </summary>
	private void OnButtonClick(object? sender, RoutedEventArgs e) {
		if (sender is Button button && button.Tag is string buttonName && ViewModel != null) {
			ViewModel.ToggleButton(0, buttonName);
		}
	}

	/// <summary>
	/// Called when the window is initialized.
	/// </summary>
	protected override void OnInitialized() {
		base.OnInitialized();

		// Set up keyboard shortcuts
		KeyDown += (s, e) => {
			if (ViewModel == null) {
				return;
			}

			// Ctrl+key shortcuts
			if (e.KeyModifiers.HasFlag(Avalonia.Input.KeyModifiers.Control)) {
				switch (e.Key) {
					case Avalonia.Input.Key.Z:
						ViewModel.Undo();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.Y:
						ViewModel.Redo();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.S:
						_ = ViewModel.SaveFileAsync();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.O:
						_ = ViewModel.OpenFileAsync();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.C:
						ViewModel.Copy();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.V:
						ViewModel.Paste();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.X:
						ViewModel.Cut();
						e.Handled = true;
						return;
				}
			}

			// Frame navigation with arrow keys when ListBox doesn't have focus
			if (!_frameList.IsFocused) {
				switch (e.Key) {
					case Avalonia.Input.Key.PageUp:
						ViewModel.SelectedFrameIndex = Math.Max(0, ViewModel.SelectedFrameIndex - 10);
						e.Handled = true;
						break;
					case Avalonia.Input.Key.PageDown:
						ViewModel.SelectedFrameIndex = Math.Min(ViewModel.Frames.Count - 1, ViewModel.SelectedFrameIndex + 10);
						e.Handled = true;
						break;
					case Avalonia.Input.Key.Home:
						ViewModel.SelectedFrameIndex = 0;
						e.Handled = true;
						break;
					case Avalonia.Input.Key.End:
						ViewModel.SelectedFrameIndex = ViewModel.Frames.Count - 1;
						e.Handled = true;
						break;
					case Avalonia.Input.Key.Space:
						ViewModel.TogglePlayback();
						e.Handled = true;
						break;
					case Avalonia.Input.Key.F:
						ViewModel.FrameAdvance();
						e.Handled = true;
						break;
					case Avalonia.Input.Key.R:
						ViewModel.FrameRewind();
						e.Handled = true;
						break;
					case Avalonia.Input.Key.Insert:
						ViewModel.InsertFrames();
						e.Handled = true;
						break;
					case Avalonia.Input.Key.Delete:
						ViewModel.DeleteFrames();
						e.Handled = true;
						break;
				}
			}
		};
	}

	/// <summary>
	/// Called when the window is closing. Prompts to save unsaved changes.
	/// </summary>
	protected override void OnClosing(WindowClosingEventArgs e) {
		if (ViewModel?.HasUnsavedChanges == true) {
			// TODO: Show confirmation dialog
			// For now, allow close without prompting
		}
		base.OnClosing(e);
	}

	#region Toolbar Click Handlers

	private void OnOpenClick(object? sender, RoutedEventArgs e) {
		_ = ViewModel?.OpenFileAsync();
	}

	private void OnSaveClick(object? sender, RoutedEventArgs e) {
		_ = ViewModel?.SaveFileAsync();
	}

	private void OnUndoClick(object? sender, RoutedEventArgs e) {
		ViewModel?.Undo();
	}

	private void OnRedoClick(object? sender, RoutedEventArgs e) {
		ViewModel?.Redo();
	}

	private void OnInsertFrameClick(object? sender, RoutedEventArgs e) {
		ViewModel?.InsertFrames();
	}

	private void OnDeleteFrameClick(object? sender, RoutedEventArgs e) {
		ViewModel?.DeleteFrames();
	}

	private void OnFrameRewindClick(object? sender, RoutedEventArgs e) {
		ViewModel?.FrameRewind();
	}

	private void OnPlayPauseClick(object? sender, RoutedEventArgs e) {
		ViewModel?.TogglePlayback();
	}

	private void OnFrameAdvanceClick(object? sender, RoutedEventArgs e) {
		ViewModel?.FrameAdvance();
	}

	#endregion

	#region Piano Roll Event Handlers

	private void OnPianoRollCellClicked(object? sender, PianoRollCellEventArgs e) {
		if (ViewModel == null) {
			return;
		}

		// Map button index to button name and toggle at the specific frame
		var buttonLabels = _pianoRoll.ButtonLabels ?? GetDefaultButtonLabels();
		if (e.ButtonIndex >= 0 && e.ButtonIndex < buttonLabels.Count) {
			string buttonName = MapButtonLabelToName(buttonLabels[e.ButtonIndex]);
			ViewModel.ToggleButtonAtFrame(e.Frame, 0, buttonName, e.NewState);
		}
	}

	private void OnPianoRollCellsPainted(object? sender, PianoRollPaintEventArgs e) {
		if (ViewModel == null) {
			return;
		}

		var buttonLabels = _pianoRoll.ButtonLabels ?? GetDefaultButtonLabels();
		if (e.ButtonIndex >= 0 && e.ButtonIndex < buttonLabels.Count) {
			string buttonName = MapButtonLabelToName(buttonLabels[e.ButtonIndex]);
			foreach (int frame in e.Frames) {
				ViewModel.SetButtonAtFrame(frame, 0, buttonName, e.PaintValue);
			}
			ViewModel.RefreshFrames();
		}
	}

	private void OnPianoRollSelectionChanged(object? sender, PianoRollSelectionEventArgs e) {
		if (ViewModel != null) {
			ViewModel.SelectedFrameIndex = e.SelectionStart;
		}
	}

	private static IReadOnlyList<string> GetDefaultButtonLabels() =>
		new[] { "A", "B", "X", "Y", "L", "R", "↑", "↓", "←", "→", "ST", "SE" };

	private static string MapButtonLabelToName(string label) {
		return label.ToUpperInvariant() switch {
			"↑" => "UP",
			"↓" => "DOWN",
			"←" => "LEFT",
			"→" => "RIGHT",
			"ST" => "START",
			"SE" => "SELECT",
			_ => label.ToUpperInvariant()
		};
	}

	#endregion
}
