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
public partial class TasEditorWindow : NexenWindow, IDisposable {
	private NotificationListener? _notificationListener;
	private bool _disposed;
	private bool _showingInterruptDialog;
	private ListBox _frameList = null!;
	private PianoRollControl _pianoRoll = null!;
	private TextBox _searchBox = null!;

	public TasEditorWindow() {
		InitializeComponent();
		SetupNamedControls();
#if DEBUG
		this.AttachDeveloperTools();
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
		SetupNamedControls();
#if DEBUG
		this.AttachDeveloperTools();
#endif
		DataContext = viewModel;
		viewModel.InitActions(this);

		SetupPianoRollEvents();
		SetupNotificationListener();
	}

	private void SetupNamedControls() {
		_frameList = FrameList ?? this.FindControl<ListBox>("FrameList")
			?? throw new InvalidOperationException("TAS editor FrameList control failed to initialize.");
		_pianoRoll = PianoRoll ?? this.FindControl<PianoRollControl>("PianoRoll")
			?? throw new InvalidOperationException("TAS editor PianoRoll control failed to initialize.");
		_searchBox = SearchBox ?? this.FindControl<TextBox>("SearchBox")
			?? throw new InvalidOperationException("TAS editor SearchBox control failed to initialize.");

		_frameList.SelectionChanged += OnFrameListSelectionChanged;
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
				Dispatcher.UIThread.Post(HandleFrameDone);
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
					// Refresh only the current frame's display — state load doesn't change movie data,
					// just the playback position. Avoids full O(n) rebuild.
					ViewModel?.RefreshFrameAtPlayback();
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

		// Apply Atari 2600 console switch commands only when editing/playing Atari 2600 movies.
		var switchState = ComputeAtari2600ConsoleSwitchState(vm.CurrentLayout, inputFrame.Command);
		if (switchState.Apply) {
			InputApi.SetAtari2600ConsoleSwitches(switchState.Select, switchState.Reset);
		}

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
				int nextFrame = vm.PlaybackFrame;
				if (TasEditorViewModel.ShouldRefreshPlaybackUi(nextFrame, movie.InputFrames.Count, vm.PlaybackSpeed)) {
					vm.SelectedFrameIndex = vm.PlaybackFrame;
					vm.StatusMessage = $"Frame {vm.PlaybackFrame + 1} / {movie.InputFrames.Count}";
				}
			}
		});
	}

	internal static (bool Apply, bool Select, bool Reset) ComputeAtari2600ConsoleSwitchState(
		ControllerLayout layout,
		MovieConverter.FrameCommand command) {
		if (layout != ControllerLayout.Atari2600) {
			return (false, false, false);
		}

		bool select = command.HasFlag(MovieConverter.FrameCommand.Atari2600Select);
		bool reset = command.HasFlag(MovieConverter.FrameCommand.Atari2600Reset);
		return (true, select, reset);
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
			ViewModel.ToggleButton(ViewModel.SelectedEditPort, buttonName);
		}
	}

	private void OnConsoleSwitchSelectClick(object? sender, RoutedEventArgs e) {
		ViewModel?.ToggleConsoleSwitchSelect();
	}

	private void OnConsoleSwitchResetClick(object? sender, RoutedEventArgs e) {
		ViewModel?.ToggleConsoleSwitchReset();
	}

	/// <summary>
	/// Called when the window is initialized.
	/// </summary>
	protected override void OnInitialized() {
		base.OnInitialized();

		// Set up keyboard shortcuts
		KeyDown += async (s, e) => {
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
						await ViewModel.SaveFileAsync();
						e.Handled = true;
						return;
					case Avalonia.Input.Key.O:
						await ViewModel.OpenFileAsync();
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
					case Avalonia.Input.Key.A:
						if (e.KeyModifiers.HasFlag(Avalonia.Input.KeyModifiers.Shift)) {
							SelectNoFramesFromUi();
						} else {
							SelectAllFramesFromUi();
						}
						e.Handled = true;
						return;
					case Avalonia.Input.Key.F:
						// Toggle search bar
						ViewModel.ToggleSearch();
						if (ViewModel.IsSearchVisible) {
							_searchBox.Focus();
						}
						e.Handled = true;
						return;
					case Avalonia.Input.Key.G:
						await ViewModel.GoToFrameAsync();
						e.Handled = true;
						return;
				}
			}

			// F3 / Shift+F3 for search navigation
			if (e.Key == Avalonia.Input.Key.F3) {
				if (e.KeyModifiers.HasFlag(Avalonia.Input.KeyModifiers.Shift)) {
					ViewModel.SearchPrevious();
				} else {
					ViewModel.SearchNext();
				}
				e.Handled = true;
				return;
			}

			// Frame navigation with arrow keys when ListBox doesn't have focus
			// Skip single-key shortcuts when a TextBox has focus (e.g., SearchBox)
			bool isTextBoxFocused = TopLevel.GetTopLevel(this)?.FocusManager?.GetFocusedElement() is TextBox;
			if (!_frameList.IsFocused && !isTextBoxFocused) {
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
	protected override async void OnClosing(WindowClosingEventArgs e) {
		if (ViewModel?.HasUnsavedChanges == true) {
			e.Cancel = true;
			var result = await MessageBox.Show(
				this,
				"You have unsaved changes. Save before closing?",
				"TAS Editor - Unsaved Changes",
				MessageBoxButtons.YesNoCancel,
				MessageBoxIcon.Question);

			switch (result) {
				case DialogResult.Yes:
					await ViewModel.SaveFileAsync();
					ViewModel.HasUnsavedChanges = false;
					Close();
					break;
				case DialogResult.No:
					ViewModel.HasUnsavedChanges = false;
					Close();
					break;
				// Cancel: do nothing, window stays open
			}
			return;
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

	private void OnSelectAllFramesClick(object? sender, RoutedEventArgs e) {
		SelectAllFramesFromUi();
	}

	private void OnSelectNoFramesClick(object? sender, RoutedEventArgs e) {
		SelectNoFramesFromUi();
	}

	private async void OnSelectRangeDialogClick(object? sender, RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.SelectRangeDialogAsync();
		ApplySelectionFromViewModel();
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

	private void OnRecordToggleClick(object? sender, RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		if (ViewModel.IsRecording) {
			ViewModel.StopRecording();
		} else {
			ViewModel.StartRecording();
		}
	}

	#endregion

	#region Frame List Selection

	private void OnFrameListSelectionChanged(object? sender, SelectionChangedEventArgs e) {
		if (ViewModel is null) return;

		var indices = new SortedSet<int>();
		foreach (var item in _frameList.SelectedItems!) {
			int idx = _frameList.ItemsSource is System.Collections.IList list ? list.IndexOf(item) : -1;
			if (idx >= 0) {
				indices.Add(idx);
			}
		}
		ViewModel.SelectedFrameIndices = indices;
	}

	private void ApplySelectionFromViewModel() {
		if (ViewModel is null || _frameList.ItemsSource is not System.Collections.IList items || _frameList.SelectedItems is null) {
			return;
		}

		_frameList.SelectedItems.Clear();
		foreach (int idx in ViewModel.SelectedFrameIndices) {
			if (idx >= 0 && idx < items.Count) {
				_frameList.SelectedItems.Add(items[idx]);
			}
		}

		_frameList.SelectedIndex = ViewModel.SelectedFrameIndex;
	}

	private void SelectAllFramesFromUi() {
		if (ViewModel is null) {
			return;
		}

		ViewModel.SelectAllFrames();
		ApplySelectionFromViewModel();
	}

	private void SelectNoFramesFromUi() {
		if (ViewModel is null) {
			return;
		}

		ViewModel.SelectNoFrames();
		ApplySelectionFromViewModel();
	}

	#endregion

	#region Context Menu Handlers

	private void OnInsertAboveClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.InsertFrameAbove();
	}

	private void OnInsertBelowClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.InsertFrameBelow();
	}

	private void OnCutClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.Cut();
	}

	private void OnCopyClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.Copy();
	}

	private void OnPasteClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.Paste();
	}

	private void OnDeleteClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.DeleteFrames();
	}

	private void OnClearInputClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.ClearSelectedInput();
	}

	private void OnSelectAllClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		SelectAllFramesFromUi();
	}

	private void OnSelectNoneClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		SelectNoFramesFromUi();
	}

	private async void OnSelectRangeDialogFromMenuClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.SelectRangeDialogAsync();
		ApplySelectionFromViewModel();
	}

	private void OnSelectRangeToHereClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) return;
		int target = _frameList.SelectedIndex;
		ViewModel.SelectRangeTo(target);
		ApplySelectionFromViewModel();
	}

	private async void OnBulkInsertClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.BulkInsertDialogAsync();
		ApplySelectionFromViewModel();
	}

	private async void OnBulkSetButtonClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.BulkSetButtonDialogAsync(true);
		ApplySelectionFromViewModel();
	}

	private async void OnBulkClearButtonClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.BulkSetButtonDialogAsync(false);
		ApplySelectionFromViewModel();
	}

	private async void OnPatternFillClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.PatternFillDialogAsync();
		ApplySelectionFromViewModel();
	}

	private void OnSetCommentClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		_ = ViewModel?.SetCommentAsync();
	}

	private void OnSetMarkerClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.ToggleMarker();
	}

	private void OnMarkerEntryDoubleTapped(object? sender, Avalonia.Input.TappedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		ViewModel.NavigateToMarkerEntry(ViewModel.SelectedMarkerEntry);
		ApplySelectionFromViewModel();
	}

	private async void OnMarkerAddClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.AddMarkerEntryAsync();
		ApplySelectionFromViewModel();
	}

	private async void OnMarkerEditClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.EditSelectedMarkerEntryAsync();
		ApplySelectionFromViewModel();
	}

	private void OnMarkerDeleteClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		ViewModel.DeleteSelectedMarkerEntry();
		ApplySelectionFromViewModel();
	}

	private async void OnMarkerExportClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is null) {
			return;
		}

		await ViewModel.ExportMarkerEntriesAsync();
	}

	#endregion

	#region Search Handlers

	private void OnSearchClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.ExecuteSearch();
	}

	private void OnSearchNextClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.SearchNext();
	}

	private void OnSearchPreviousClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		ViewModel?.SearchPrevious();
	}

	private void OnSearchCloseClick(object? sender, Avalonia.Interactivity.RoutedEventArgs e) {
		if (ViewModel is not null) {
			ViewModel.IsSearchVisible = false;
		}
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
			ViewModel.ToggleButtonAtFrame(e.Frame, ViewModel.SelectedEditPort, buttonName, e.NewState);
		}
	}

	private void OnPianoRollCellsPainted(object? sender, PianoRollPaintEventArgs e) {
		if (ViewModel == null) {
			return;
		}

		var buttonLabels = _pianoRoll.ButtonLabels ?? GetDefaultButtonLabels();
		if (e.ButtonIndex >= 0 && e.ButtonIndex < buttonLabels.Count) {
			string buttonName = MapButtonLabelToName(buttonLabels[e.ButtonIndex]);
			ViewModel.PaintButton(e.Frames, ViewModel.SelectedEditPort, buttonName, e.PaintValue);
		}
	}

	private void OnPianoRollSelectionChanged(object? sender, PianoRollSelectionEventArgs e) {
		if (ViewModel != null) {
			ViewModel.SelectFrameRange(e.SelectionStart, e.SelectionEnd);
			ApplySelectionFromViewModel();
		}
	}

	private static IReadOnlyList<string> GetDefaultButtonLabels() =>
		new[] { "A", "B", "X", "Y", "L", "R", "↑", "↓", "←", "→", "ST", "SE" };

	private static string MapButtonLabelToName(string label) {
		string normalized = label.Trim().ToUpperInvariant();

		return normalized switch {
			"↑" => "UP",
			"↓" => "DOWN",
			"←" => "LEFT",
			"→" => "RIGHT",
			"LEFT" => "LEFT",
			"RIGHT" => "RIGHT",
			"ST" => "START",
			"SE" => "SELECT",
			"*" => "STAR",
			"#" => "POUND",
			"BK" => "BACK",
			"BACK" => "BACK",
			"FW" => "FORWARD",
			"FWD" => "FORWARD",
			"FORWARD" => "FORWARD",
			"↺" => "TWISTCCW",
			"↻" => "TWISTCW",
			"TWL" => "TWISTCCW",
			"TWR" => "TWISTCW",
			"PL" => "PULL",
			"PULL" => "PULL",
			"PS" => "PUSH",
			"PUSH" => "PUSH",
			_ => normalized
		};
	}

	#endregion
}
