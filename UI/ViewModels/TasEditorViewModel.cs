using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text.Json;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;

/// <summary>
/// ViewModel for the TAS Editor window.
/// Provides frame-by-frame editing of movie files with greenzone support,
/// input recording, and piano roll visualization.
/// </summary>
public class TasEditorViewModel : DisposableViewModel {
	/// <summary>Gets the currently loaded movie data.</summary>
	[Reactive] public MovieData? Movie { get; private set; }

	/// <summary>Gets or sets the current file path.</summary>
	[Reactive] public string? FilePath { get; set; }

	/// <summary>Gets or sets whether the movie has unsaved changes.</summary>
	[Reactive] public bool HasUnsavedChanges { get; set; }

	/// <summary>Gets or sets the currently selected frame index.</summary>
	[Reactive] public int SelectedFrameIndex { get; set; } = -1;

	/// <summary>Gets the observable collection of frame view models.</summary>
	public ObservableCollection<TasFrameViewModel> Frames { get; } = new();

	/// <summary>Gets or sets the status message.</summary>
	[Reactive] public string StatusMessage { get; set; } = "No movie loaded";

	/// <summary>Gets or sets whether greenzone (rerecord safe zone) is enabled.</summary>
	[Reactive] public bool IsGreenzoneEnabled { get; set; } = true;

	/// <summary>Gets or sets the greenzone start frame.</summary>
	[Reactive] public int GreenzoneStart { get; set; }

	/// <summary>Gets the window title.</summary>
	[Reactive] public string WindowTitle { get; private set; } = "TAS Editor";

	/// <summary>Gets the file menu items.</summary>
	[Reactive] public List<object> FileMenuItems { get; private set; } = new();

	/// <summary>Gets the edit menu items.</summary>
	[Reactive] public List<object> EditMenuItems { get; private set; } = new();

	/// <summary>Gets the view menu items.</summary>
	[Reactive] public List<object> ViewMenuItems { get; private set; } = new();

	/// <summary>Gets the playback menu items.</summary>
	[Reactive] public List<object> PlaybackMenuItems { get; private set; } = new();

	/// <summary>Gets the recording menu items.</summary>
	[Reactive] public List<object> RecordingMenuItems { get; private set; } = new();

	/// <summary>Gets whether undo is available.</summary>
	[Reactive] public bool CanUndo { get; private set; }

	/// <summary>Gets whether redo is available.</summary>
	[Reactive] public bool CanRedo { get; private set; }

	/// <summary>Gets whether clipboard has content.</summary>
	[Reactive] public bool HasClipboard { get; private set; }

	/// <summary>Gets or sets whether playback is active.</summary>
	[Reactive] public bool IsPlaying { get; set; }

	/// <summary>Gets or sets the current playback frame.</summary>
	[Reactive] public int PlaybackFrame { get; set; }

	/// <summary>Gets or sets the playback speed (1.0 = normal).</summary>
	[Reactive] public double PlaybackSpeed { get; set; } = 1.0;

	/// <summary>Gets or sets the current controller layout.</summary>
	[Reactive] public ControllerLayout CurrentLayout { get; set; } = ControllerLayout.Snes;

	/// <summary>Gets the controller buttons for the current layout.</summary>
	[Reactive] public List<ControllerButtonInfo> ControllerButtons { get; private set; } = new();

	/// <summary>Gets the available controller layouts.</summary>
	public static IReadOnlyList<ControllerLayout> AvailableLayouts { get; } =
		Enum.GetValues<ControllerLayout>().ToList();

	/// <summary>Gets or sets whether recording is active.</summary>
	[Reactive] public bool IsRecording { get; set; }

	/// <summary>Gets or sets the current recording mode.</summary>
	[Reactive] public RecordingMode RecordMode { get; set; } = RecordingMode.Append;

	/// <summary>Gets the greenzone savestate count.</summary>
	[Reactive] public int SavestateCount { get; private set; }

	/// <summary>Gets the greenzone memory usage in MB.</summary>
	[Reactive] public double GreenzoneMemoryMB { get; private set; }

	/// <summary>Gets or sets whether piano roll view is visible.</summary>
	[Reactive] public bool ShowPianoRoll { get; set; }

	/// <summary>Gets the rerecord count for the current movie.</summary>
	[Reactive] public int RerecordCount { get; private set; }

	/// <summary>Gets the list of saved branches.</summary>
	public ObservableCollection<BranchData> Branches { get; } = new();

	/// <summary>Gets the script menu items.</summary>
	[Reactive] public List<object> ScriptMenuItems { get; private set; } = new();

	/// <summary>Gets the Lua API for TAS scripting.</summary>
	public TasLuaApi LuaApi { get; }

	/// <summary>Gets or sets the current Lua script path.</summary>
	[Reactive] public string? CurrentScriptPath { get; set; }

	/// <summary>Gets or sets whether a script is running.</summary>
	[Reactive] public bool IsScriptRunning { get; set; }

	/// <summary>The ID of the currently loaded script (-1 if none).</summary>
	private int _currentScriptId = -1;

	private readonly Stack<UndoableAction> _undoStack = new();
	private readonly Stack<UndoableAction> _redoStack = new();
	private List<InputFrame>? _clipboard;
	private int _clipboardStartIndex;
	private IMovieConverter? _currentConverter;
	private Windows.TasEditorWindow? _window;

	/// <summary>Gets the greenzone manager for savestate management.</summary>
	public GreenzoneManager Greenzone { get; } = new();

	/// <summary>Gets the input recorder for TAS recording.</summary>
	public InputRecorder Recorder { get; }

	public TasEditorViewModel() {
		// Initialize Lua API
		LuaApi = new TasLuaApi(this);

		// Initialize recorder with greenzone
		Recorder = new InputRecorder(Greenzone);

		// Subscribe to greenzone events
		Greenzone.SavestateCaptured += (_, e) => {
			SavestateCount = Greenzone.SavestateCount;
			GreenzoneMemoryMB = Greenzone.TotalMemoryUsage / (1024.0 * 1024.0);
		};

		Greenzone.SavestatesPruned += (_, e) => {
			SavestateCount = Greenzone.SavestateCount;
			GreenzoneMemoryMB = Greenzone.TotalMemoryUsage / (1024.0 * 1024.0);
			StatusMessage = $"Pruned {e.PrunedCount} states, freed {e.FreedMemory / 1024.0:F1} KB";
		};

		// Subscribe to recorder events
		Recorder.RecordingStarted += (_, e) => {
			IsRecording = true;
			StatusMessage = $"Recording started at frame {e.StartFrame} ({e.Mode} mode)";
		};

		Recorder.RecordingStopped += (_, e) => {
			IsRecording = false;
			StatusMessage = $"Recording stopped. {e.FramesRecorded} frames recorded.";
			UpdateFrames();
		};

		Recorder.FrameRecorded += (_, e) => {
			PlaybackFrame = e.FrameIndex;
			// Update UI less frequently for performance
			if (e.FrameIndex % 10 == 0) {
				UpdateFrames();
			}
		};

		Recorder.Rerecording += (_, e) => {
			RerecordCount = e.RerecordCount;
			StatusMessage = $"Rerecording from frame {e.Frame}. Total rerecords: {e.RerecordCount}";
		};

		AddDisposable(this.WhenAnyValue(x => x.Movie).Subscribe(_ => {
			UpdateFrames();
			DetectControllerLayout();
			Recorder.Movie = Movie;
			Greenzone.Clear();
		}));
		AddDisposable(this.WhenAnyValue(x => x.FilePath, x => x.HasUnsavedChanges).Subscribe(_ => UpdateWindowTitle()));
		AddDisposable(this.WhenAnyValue(x => x.CurrentLayout).Subscribe(_ => UpdateControllerButtons()));
		UpdateControllerButtons();
	}

	/// <summary>
	/// Initializes the menu actions.
	/// </summary>
	public void InitActions(Windows.TasEditorWindow wnd) {
		_window = wnd;

		FileMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Open",
				OnClick = () => _ = OpenFileAsync()
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Save",
				OnClick = () => _ = SaveFileAsync(),
				IsEnabled = () => Movie != null && _currentConverter?.CanWrite == true
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Save As...",
				OnClick = () => _ = SaveAsFileAsync(),
				IsEnabled = () => Movie != null
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Import...",
				OnClick = () => _ = ImportMovieAsync()
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Export...",
				OnClick = () => _ = ExportMovieAsync(),
				IsEnabled = () => Movie != null
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd.Close()
			}
		};

		EditMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Undo,
				OnClick = Undo,
				IsEnabled = () => CanUndo
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Redo",
				OnClick = Redo,
				IsEnabled = () => CanRedo
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Cut",
				OnClick = Cut,
				IsEnabled = () => Movie != null && SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Copy,
				OnClick = Copy,
				IsEnabled = () => Movie != null && SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Paste,
				OnClick = Paste,
				IsEnabled = () => HasClipboard && Movie != null
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Insert Frame(s)",
				OnClick = InsertFrames,
				IsEnabled = () => Movie != null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Delete Frame(s)",
				OnClick = DeleteFrames,
				IsEnabled = () => Movie != null && SelectedFrameIndex >= 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Clear Input",
				OnClick = ClearSelectedInput,
				IsEnabled = () => SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Set Comment",
				OnClick = SetComment,
				IsEnabled = () => SelectedFrameIndex >= 0
			}
		};

		ViewMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Go to Frame...",
				OnClick = () => _ = GoToFrameAsync()
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Toggle Greenzone",
				OnClick = () => IsGreenzoneEnabled = !IsGreenzoneEnabled
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Show Piano Roll",
				OnClick = () => ShowPianoRoll = !ShowPianoRoll
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Greenzone Settings...",
				OnClick = ShowGreenzoneSettings
			}
		};

		PlaybackMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Play/Pause",
				OnClick = TogglePlayback,
				IsEnabled = () => Movie != null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Stop",
				OnClick = StopPlayback,
				IsEnabled = () => IsPlaying
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Frame Advance",
				OnClick = FrameAdvance,
				IsEnabled = () => Movie != null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Frame Rewind",
				OnClick = FrameRewind,
				IsEnabled = () => Movie != null && PlaybackFrame > 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Seek to Frame...",
				OnClick = () => _ = SeekToFrameAsync(),
				IsEnabled = () => Movie != null && Greenzone.SavestateCount > 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Speed: 0.25x",
				OnClick = () => PlaybackSpeed = 0.25
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Speed: 0.5x",
				OnClick = () => PlaybackSpeed = 0.5
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Speed: 1.0x",
				OnClick = () => PlaybackSpeed = 1.0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Speed: 2.0x",
				OnClick = () => PlaybackSpeed = 2.0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Speed: 4.0x",
				OnClick = () => PlaybackSpeed = 4.0
			}
		};

		RecordingMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Start Recording",
				OnClick = StartRecording,
				IsEnabled = () => Movie != null && !IsRecording
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Stop Recording",
				OnClick = StopRecording,
				IsEnabled = () => IsRecording
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Rerecord from Here",
				OnClick = RerecordFromSelected,
				IsEnabled = () => Movie != null && SelectedFrameIndex >= 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Record Mode: Append",
				OnClick = () => RecordMode = RecordingMode.Append
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Record Mode: Insert",
				OnClick = () => RecordMode = RecordingMode.Insert
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Record Mode: Overwrite",
				OnClick = () => RecordMode = RecordingMode.Overwrite
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Create Branch",
				OnClick = CreateBranch,
				IsEnabled = () => Movie != null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Manage Branches...",
				OnClick = () => _ = ManageBranchesAsync(),
				IsEnabled = () => Branches.Count > 0
			}
		};

		ScriptMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Run Script...",
				OnClick = () => _ = RunScriptAsync(),
				IsEnabled = () => Movie != null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Stop Script",
				OnClick = StopScript,
				IsEnabled = () => IsScriptRunning
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Edit Script...",
				OnClick = () => _ = EditScriptAsync()
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "New Script...",
				OnClick = () => _ = NewScriptAsync()
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Script API Help",
				OnClick = ShowScriptApiHelp
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Open Script Folder",
				OnClick = OpenScriptFolder
			}
		};
	}

	/// <summary>
	/// Opens a movie file for editing.
	/// </summary>
	public async System.Threading.Tasks.Task OpenFileAsync() {
		string? path = await FileDialogHelper.OpenFile(
			ConfigManager.MovieFolder,
			_window,
			FileDialogHelper.NexenMovieExt,
			FileDialogHelper.MesenMovieExt,
			"bk2", "fm2", "smv", "lsmv", "vbm", "gmv");

		if (!string.IsNullOrEmpty(path)) {
			await LoadMovieAsync(path);
		}
	}

	/// <summary>
	/// Loads a movie from the specified path.
	/// </summary>
	public async System.Threading.Tasks.Task LoadMovieAsync(string path) {
		try {
			var format = MovieConverterRegistry.DetectFormat(path);
			var converter = MovieConverterRegistry.GetConverter(format);

			if (converter == null) {
				StatusMessage = $"Unsupported format: {format}";
				return;
			}

			await using var stream = File.OpenRead(path);
			Movie = await converter.ReadAsync(stream);
			FilePath = path;
			_currentConverter = converter;
			HasUnsavedChanges = false;
			StatusMessage = $"Loaded {Movie.InputFrames.Count:N0} frames from {Path.GetFileName(path)}";
		} catch (Exception ex) {
			StatusMessage = $"Error loading file: {ex.Message}";
		}
	}

	/// <summary>
	/// Saves the current movie to the existing file.
	/// </summary>
	public async System.Threading.Tasks.Task SaveFileAsync() {
		if (Movie == null || _currentConverter == null || !_currentConverter.CanWrite) {
			return;
		}

		if (string.IsNullOrEmpty(FilePath)) {
			await SaveAsFileAsync();
			return;
		}

		try {
			await using var stream = File.Create(FilePath);
			await _currentConverter.WriteAsync(Movie, stream);
			HasUnsavedChanges = false;
			StatusMessage = $"Saved {Movie.InputFrames.Count:N0} frames to {Path.GetFileName(FilePath)}";
		} catch (Exception ex) {
			StatusMessage = $"Error saving file: {ex.Message}";
		}
	}

	/// <summary>
	/// Saves the current movie to a new file.
	/// </summary>
	public async System.Threading.Tasks.Task SaveAsFileAsync() {
		if (Movie == null) {
			return;
		}

		string? path = await FileDialogHelper.SaveFile(
			ConfigManager.MovieFolder,
			null,
			_window,
			FileDialogHelper.NexenMovieExt);

		if (!string.IsNullOrEmpty(path)) {
			var format = MovieConverterRegistry.DetectFormat(path);
			var converter = MovieConverterRegistry.GetConverter(format);

			if (converter == null || !converter.CanWrite) {
				StatusMessage = $"Cannot write to format: {format}";
				return;
			}

			try {
				await using var stream = File.Create(path);
				await converter.WriteAsync(Movie, stream);
				FilePath = path;
				_currentConverter = converter;
				HasUnsavedChanges = false;
				StatusMessage = $"Saved {Movie.InputFrames.Count:N0} frames to {Path.GetFileName(path)}";
			} catch (Exception ex) {
				StatusMessage = $"Error saving file: {ex.Message}";
			}
		}
	}

	/// <summary>
	/// Imports a movie from another format.
	/// </summary>
	public async System.Threading.Tasks.Task ImportMovieAsync() {
		string? path = await FileDialogHelper.OpenFile(
			ConfigManager.MovieFolder,
			_window,
			"bk2", "fm2", "smv", "lsmv", "vbm", "gmv");

		if (!string.IsNullOrEmpty(path)) {
			await LoadMovieAsync(path);
			HasUnsavedChanges = true; // Mark as needing save since it's imported
		}
	}

	/// <summary>
	/// Exports the movie to another format.
	/// </summary>
	public async System.Threading.Tasks.Task ExportMovieAsync() {
		if (Movie == null) {
			return;
		}

		string? path = await FileDialogHelper.SaveFile(
			ConfigManager.MovieFolder,
			null,
			_window,
			"bk2", "fm2");

		if (!string.IsNullOrEmpty(path)) {
			var format = MovieConverterRegistry.DetectFormat(path);
			var converter = MovieConverterRegistry.GetConverter(format);

			if (converter == null || !converter.CanWrite) {
				StatusMessage = $"Cannot export to format: {format}";
				return;
			}

			try {
				await using var stream = File.Create(path);
				await converter.WriteAsync(Movie, stream);
				StatusMessage = $"Exported {Movie.InputFrames.Count:N0} frames to {Path.GetFileName(path)}";
			} catch (Exception ex) {
				StatusMessage = $"Error exporting file: {ex.Message}";
			}
		}
	}

	/// <summary>
	/// Inserts new frames at the current position.
	/// </summary>
	public void InsertFrames() {
		if (Movie == null) {
			return;
		}

		int insertAt = Math.Max(0, SelectedFrameIndex);
		Movie.InputFrames.Insert(insertAt, new InputFrame(insertAt));
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Inserted frame at {insertAt}";
	}

	/// <summary>
	/// Deletes the selected frames.
	/// </summary>
	public void DeleteFrames() {
		if (Movie == null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		int deleteAt = SelectedFrameIndex;
		Movie.InputFrames.RemoveAt(deleteAt);
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Deleted frame {deleteAt}";

		// Adjust selection
		if (SelectedFrameIndex >= Movie.InputFrames.Count) {
			SelectedFrameIndex = Movie.InputFrames.Count - 1;
		}
	}

	/// <summary>
	/// Clears the input on the selected frame.
	/// </summary>
	public void ClearSelectedInput() {
		if (Movie == null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		for (int i = 0; i < frame.Controllers.Length; i++) {
			frame.Controllers[i] = new ControllerInput();
		}

		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Cleared input on frame {SelectedFrameIndex}";
	}

	/// <summary>
	/// Sets a comment on the selected frame.
	/// </summary>
	public void SetComment() {
		if (Movie == null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		if (string.IsNullOrEmpty(frame.Comment)) {
			frame.Comment = $"Comment {SelectedFrameIndex}";
		} else {
			frame.Comment = null;
		}

		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = frame.Comment != null ? $"Set comment on frame {SelectedFrameIndex}" : $"Removed comment from frame {SelectedFrameIndex}";
	}

	/// <summary>
	/// Goes to a specific frame.
	/// </summary>
	public async System.Threading.Tasks.Task GoToFrameAsync() {
		// TODO: Implement frame input dialog
		// For now, just go to first frame
		if (Movie != null && Movie.InputFrames.Count > 0) {
			SelectedFrameIndex = 0;
		}

		await System.Threading.Tasks.Task.CompletedTask;
	}

	/// <summary>
	/// Toggles a button on the selected frame.
	/// </summary>
	public void ToggleButton(int port, string button) {
		if (Movie == null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		if (port < 0 || port >= frame.Controllers.Length) {
			return;
		}

		var controller = frame.Controllers[port];
		switch (button.ToUpperInvariant()) {
			case "A": controller.A = !controller.A; break;
			case "B": controller.B = !controller.B; break;
			case "X": controller.X = !controller.X; break;
			case "Y": controller.Y = !controller.Y; break;
			case "L": controller.L = !controller.L; break;
			case "R": controller.R = !controller.R; break;
			case "UP": controller.Up = !controller.Up; break;
			case "DOWN": controller.Down = !controller.Down; break;
			case "LEFT": controller.Left = !controller.Left; break;
			case "RIGHT": controller.Right = !controller.Right; break;
			case "START": controller.Start = !controller.Start; break;
			case "SELECT": controller.Select = !controller.Select; break;
		}

		UpdateFrames();
		HasUnsavedChanges = true;
	}

	private void UpdateFrames() {
		Frames.Clear();

		if (Movie == null) {
			return;
		}

		for (int i = 0; i < Movie.InputFrames.Count; i++) {
			Frames.Add(new TasFrameViewModel(Movie.InputFrames[i], i, IsGreenzoneEnabled && i >= GreenzoneStart));
		}
	}

	#region Undo/Redo

	/// <summary>
	/// Undoes the last action.
	/// </summary>
	public void Undo() {
		if (!CanUndo || _undoStack.Count == 0) {
			return;
		}

		var action = _undoStack.Pop();
		action.Undo();
		_redoStack.Push(action);

		UpdateUndoRedoState();
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Undid: {action.Description}";
	}

	/// <summary>
	/// Redoes the last undone action.
	/// </summary>
	public void Redo() {
		if (!CanRedo || _redoStack.Count == 0) {
			return;
		}

		var action = _redoStack.Pop();
		action.Execute();
		_undoStack.Push(action);

		UpdateUndoRedoState();
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Redid: {action.Description}";
	}

	/// <summary>
	/// Executes an action and adds it to the undo stack.
	/// </summary>
	private void ExecuteAction(UndoableAction action) {
		action.Execute();
		_undoStack.Push(action);
		_redoStack.Clear(); // Clear redo stack on new action

		UpdateUndoRedoState();
		HasUnsavedChanges = true;
	}

	private void UpdateUndoRedoState() {
		CanUndo = _undoStack.Count > 0;
		CanRedo = _redoStack.Count > 0;
	}

	#endregion

	#region Copy/Paste

	/// <summary>
	/// Cuts the selected frame(s) to clipboard.
	/// </summary>
	public void Cut() {
		if (Movie == null || SelectedFrameIndex < 0) {
			return;
		}

		Copy();
		DeleteFrames();
	}

	/// <summary>
	/// Copies the selected frame(s) to clipboard.
	/// </summary>
	public void Copy() {
		if (Movie == null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		// Clone the selected frame
		var frame = Movie.InputFrames[SelectedFrameIndex];
		_clipboard = new List<InputFrame> { CloneFrame(frame) };
		_clipboardStartIndex = SelectedFrameIndex;
		HasClipboard = true;

		StatusMessage = $"Copied frame {SelectedFrameIndex + 1}";
	}

	/// <summary>
	/// Pastes clipboard content at the current position.
	/// </summary>
	public void Paste() {
		if (Movie == null || _clipboard == null || _clipboard.Count == 0) {
			return;
		}

		int insertAt = Math.Max(0, SelectedFrameIndex >= 0 ? SelectedFrameIndex + 1 : Movie.InputFrames.Count);
		var clonedFrames = _clipboard.Select(CloneFrame).ToList();

		ExecuteAction(new InsertFramesAction(Movie, insertAt, clonedFrames));
		UpdateFrames();
		SelectedFrameIndex = insertAt;

		StatusMessage = $"Pasted {clonedFrames.Count} frame(s) at {insertAt + 1}";
	}

	private static InputFrame CloneFrame(InputFrame original) {
		var clone = new InputFrame(original.FrameNumber) {
			IsLagFrame = original.IsLagFrame,
			Comment = original.Comment
		};

		for (int i = 0; i < original.Controllers.Length && i < clone.Controllers.Length; i++) {
			var src = original.Controllers[i];
			clone.Controllers[i] = new ControllerInput {
				A = src.A,
				B = src.B,
				X = src.X,
				Y = src.Y,
				L = src.L,
				R = src.R,
				Up = src.Up,
				Down = src.Down,
				Left = src.Left,
				Right = src.Right,
				Start = src.Start,
				Select = src.Select
			};
		}

		return clone;
	}

	#endregion

	#region Playback Controls

	/// <summary>
	/// Toggles playback on/off.
	/// </summary>
	public void TogglePlayback() {
		if (IsPlaying) {
			StopPlayback();
		} else {
			StartPlayback();
		}
	}

	/// <summary>
	/// Starts playback from the current frame.
	/// </summary>
	public void StartPlayback() {
		if (Movie == null) {
			return;
		}

		IsPlaying = true;
		PlaybackFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;
		StatusMessage = $"Playing at {PlaybackSpeed:F2}x speed";

		// TODO: Connect to emulation core for actual playback
		// For now, just update the status
	}

	/// <summary>
	/// Stops playback.
	/// </summary>
	public void StopPlayback() {
		IsPlaying = false;
		StatusMessage = "Playback stopped";

		// Pause emulation when stopping playback
		if (EmuApi.IsRunning() && !EmuApi.IsPaused()) {
			EmuApi.Pause();
		}
	}

	/// <summary>
	/// Advances playback by one frame.
	/// </summary>
	public void FrameAdvance() {
		if (Movie == null) {
			return;
		}

		if (PlaybackFrame < Movie.InputFrames.Count - 1) {
			PlaybackFrame++;
			SelectedFrameIndex = PlaybackFrame;
			StatusMessage = $"Frame {PlaybackFrame + 1} / {Movie.InputFrames.Count}";

			// Execute single frame in emulator if running
			if (EmuApi.IsRunning()) {
				EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
					Shortcut = EmulatorShortcut.RunSingleFrame
				});
			}
		}
	}

	/// <summary>
	/// Rewinds playback by one frame.
	/// </summary>
	public void FrameRewind() {
		if (Movie == null) {
			return;
		}

		if (PlaybackFrame > 0) {
			PlaybackFrame--;
			SelectedFrameIndex = PlaybackFrame;
			StatusMessage = $"Frame {PlaybackFrame + 1} / {Movie.InputFrames.Count}";

			// Use rewind shortcut if available
			if (EmuApi.IsRunning()) {
				// Try to load a savestate for frame-accurate rewind
				// For now, use the rewind feature
				EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
					Shortcut = EmulatorShortcut.Rewind
				});
			}
		}
	}

	/// <summary>
	/// Seeks to a specific frame using the greenzone.
	/// </summary>
	public async System.Threading.Tasks.Task SeekToFrameAsync() {
		if (Movie == null || Greenzone.SavestateCount == 0) {
			return;
		}

		// TODO: Implement frame input dialog
		int targetFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;

		int actualFrame = await Greenzone.SeekToFrameAsync(targetFrame);

		if (actualFrame >= 0) {
			PlaybackFrame = actualFrame;
			SelectedFrameIndex = actualFrame;
			StatusMessage = $"Seeked to frame {actualFrame + 1}";
		} else {
			StatusMessage = "Failed to seek - no greenzone state available";
		}
	}

	#endregion

	#region Recording Controls

	/// <summary>
	/// Starts input recording.
	/// </summary>
	public void StartRecording() {
		if (Movie == null || IsRecording) {
			return;
		}

		Recorder.StartRecording(RecordMode, SelectedFrameIndex);
	}

	/// <summary>
	/// Stops input recording.
	/// </summary>
	public void StopRecording() {
		if (!IsRecording) {
			return;
		}

		Recorder.StopRecording();
		UpdateFrames();
		HasUnsavedChanges = true;
	}

	/// <summary>
	/// Starts rerecording from the selected frame.
	/// </summary>
	public void RerecordFromSelected() {
		if (Movie == null || SelectedFrameIndex < 0) {
			return;
		}

		if (Recorder.RerecordFrom(SelectedFrameIndex)) {
			UpdateFrames();
			HasUnsavedChanges = true;
		} else {
			StatusMessage = "Failed to rerecord - no greenzone state available for this frame";
		}
	}

	/// <summary>
	/// Creates a branch from the current movie state.
	/// </summary>
	public void CreateBranch() {
		if (Movie == null) {
			return;
		}

		var branch = Recorder.CreateBranch();
		Branches.Add(branch);
		StatusMessage = $"Created branch: {branch.Name}";
	}

	/// <summary>
	/// Loads a saved branch.
	/// </summary>
	/// <param name="branch">The branch to load.</param>
	public void LoadBranch(BranchData branch) {
		if (Movie == null) {
			return;
		}

		Recorder.LoadBranch(branch);
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Loaded branch: {branch.Name}";
	}

	/// <summary>
	/// Shows the branch management dialog.
	/// </summary>
	public async System.Threading.Tasks.Task ManageBranchesAsync() {
		// TODO: Implement branch management dialog
		await System.Threading.Tasks.Task.CompletedTask;
	}

	/// <summary>
	/// Shows greenzone settings dialog.
	/// </summary>
	public void ShowGreenzoneSettings() {
		// TODO: Implement greenzone settings dialog
		StatusMessage = $"Greenzone: {SavestateCount} states, {GreenzoneMemoryMB:F1} MB";
	}

	#endregion

	#region Lua Scripting

	/// <summary>
	/// Opens a Lua script file and runs it.
	/// </summary>
	public async System.Threading.Tasks.Task RunScriptAsync() {
		string? path = await FileDialogHelper.OpenFile(
			Path.Combine(ConfigManager.HomeFolder, "TasScripts"),
			_window,
			"lua");

		if (!string.IsNullOrEmpty(path)) {
			await RunScriptAsync(path);
		}
	}

	/// <summary>
	/// Runs a Lua script from the specified path.
	/// </summary>
	public async System.Threading.Tasks.Task RunScriptAsync(string path) {
		try {
			if (!File.Exists(path)) {
				StatusMessage = $"Script not found: {path}";
				return;
			}

			// Stop any existing script
			if (_currentScriptId >= 0) {
				DebugApi.RemoveScript(_currentScriptId);
			}

			CurrentScriptPath = path;
			IsScriptRunning = true;
			StatusMessage = $"Running script: {Path.GetFileName(path)}";

			// Run the script through the debugger's Lua API
			string scriptContent = await File.ReadAllTextAsync(path);
			_currentScriptId = DebugApi.LoadScript(Path.GetFileName(path), path, scriptContent, -1);

			StatusMessage = $"Script loaded: {Path.GetFileName(path)}";
		} catch (Exception ex) {
			IsScriptRunning = false;
			_currentScriptId = -1;
			StatusMessage = $"Script error: {ex.Message}";
		}
	}

	/// <summary>
	/// Stops the currently running script.
	/// </summary>
	public void StopScript() {
		if (!IsScriptRunning || _currentScriptId < 0) return;

		try {
			// Stop the script through the debugger API
			DebugApi.RemoveScript(_currentScriptId);
			_currentScriptId = -1;
			IsScriptRunning = false;
			CurrentScriptPath = null;
			StatusMessage = "Script stopped";
		} catch (Exception ex) {
			StatusMessage = $"Error stopping script: {ex.Message}";
		}
	}

	/// <summary>
	/// Opens the script editor for an existing script.
	/// </summary>
	public async System.Threading.Tasks.Task EditScriptAsync() {
		string? path = await FileDialogHelper.OpenFile(
			Path.Combine(ConfigManager.HomeFolder, "TasScripts"),
			_window,
			"lua");

		if (!string.IsNullOrEmpty(path)) {
			// Open script in the debugger's script window
			var model = new ScriptWindowViewModel(null);
			var wnd = new ScriptWindow(model);
			DebugWindowManager.OpenDebugWindow(() => wnd);
			model.LoadScript(path);
		}
	}

	/// <summary>
	/// Creates a new TAS script from template.
	/// </summary>
	public async System.Threading.Tasks.Task NewScriptAsync() {
		string scriptFolder = Path.Combine(ConfigManager.HomeFolder, "TasScripts");
		Directory.CreateDirectory(scriptFolder);

		string? path = await FileDialogHelper.SaveFile(
			scriptFolder,
			"TasScript",
			_window,
			"lua");

		if (!string.IsNullOrEmpty(path)) {
			// Create template script
			string template = GetScriptTemplate();
			await File.WriteAllTextAsync(path, template);
			StatusMessage = $"Created script: {Path.GetFileName(path)}";

			// Open script in the debugger's script window
			var model = new ScriptWindowViewModel(null);
			var wnd = new ScriptWindow(model);
			DebugWindowManager.OpenDebugWindow(() => wnd);
			model.LoadScript(path);
		}
	}

	/// <summary>
	/// Gets the template for a new TAS Lua script.
	/// </summary>
	private string GetScriptTemplate() {
		return @"-- TAS Script for Nexen
-- This script provides helper functions for TAS creation

-- Global state
local startFrame = emu.getState().frameCount
local bestResult = nil

-- Input combinations to try
local buttons = {""a"", ""b"", ""up"", ""down"", ""left"", ""right""}

-- Helper: Generate all button combinations
function generateCombinations(buttons)
	local combos = {}
	local n = #buttons
	local total = 2 ^ n
	for i = 0, total - 1 do
		local combo = {}
		for j = 1, n do
			if bit32.band(i, bit32.lshift(1, j - 1)) ~= 0 then
				combo[buttons[j]] = true
			else
				combo[buttons[j]] = false
			end
		end
		table.insert(combos, combo)
	end
	return combos
end

-- Helper: Read player position from RAM
-- TODO: Replace with actual game-specific addresses
function getPlayerX()
	return emu.read(0x0010, emu.memType.cpuMemory)
end

function getPlayerY()
	return emu.read(0x0011, emu.memType.cpuMemory)
end

-- Main search function
function searchBestInput()
	local combos = generateCombinations(buttons)
	local best = nil
	local bestX = getPlayerX()

	for i, combo in ipairs(combos) do
		-- Create savestate before test
		local state = emu.createSavestate()

		-- Apply input
		emu.setInput(combo, 0)

		-- Advance frame
		emu.step(1, emu.stepType.step)

		-- Check result
		local newX = getPlayerX()
		if newX > bestX then
			bestX = newX
			best = combo
		end

		-- Restore state
		emu.loadSavestate(state)
	end

	if best then
		emu.log(""Best input found: "" .. tableToString(best))
		emu.setInput(best, 0)
	end
end

-- Helper: Convert table to string
function tableToString(t)
	local result = ""{}""
	for k, v in pairs(t) do
		if v then
			result = result .. k .. "", ""
		end
	end
	return result .. ""}""
end

-- Frame callback
function onFrame()
	-- Example: Search every 60 frames
	local frame = emu.getState().frameCount
	if (frame - startFrame) % 60 == 0 then
		-- searchBestInput()
	end
end

-- Register callback
emu.addEventCallback(onFrame, emu.eventType.endFrame)

emu.log(""TAS Script loaded"")
emu.displayMessage(""TAS"", ""Script active"")
";
	}

	/// <summary>
	/// Shows the script API help documentation.
	/// </summary>
	public void ShowScriptApiHelp() {
		// Open the Lua API documentation in browser
		string docPath = Path.Combine(ConfigManager.HomeFolder, "TasLuaApiDoc.html");

		// Generate doc if doesn't exist
		if (!File.Exists(docPath)) {
			GenerateScriptApiDoc(docPath);
		}

		// Open in browser
		try {
			System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo {
				FileName = docPath,
				UseShellExecute = true
			});
		} catch {
			StatusMessage = "Could not open API documentation";
		}
	}

	/// <summary>
	/// Generates the script API documentation.
	/// </summary>
	private void GenerateScriptApiDoc(string path) {
		string html = @"<!DOCTYPE html>
<html>
<head>
	<title>Nexen TAS Lua API Reference</title>
	<style>
		body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 900px; margin: 0 auto; padding: 20px; }
		h1 { color: #2c3e50; }
		h2 { color: #34495e; border-bottom: 2px solid #3498db; padding-bottom: 5px; }
		h3 { color: #7f8c8d; }
		code { background: #f4f4f4; padding: 2px 6px; border-radius: 3px; }
		pre { background: #2d3436; color: #dfe6e9; padding: 15px; border-radius: 5px; overflow-x: auto; }
		.param { color: #e74c3c; }
		.return { color: #27ae60; }
		table { border-collapse: collapse; width: 100%; }
		th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
		th { background: #3498db; color: white; }
	</style>
</head>
<body>
	<h1>🎮 Nexen TAS Lua API Reference</h1>

	<h2>Movie Information</h2>
	<table>
		<tr><th>Function</th><th>Description</th><th>Returns</th></tr>
		<tr><td><code>tas.getCurrentFrame()</code></td><td>Get current frame number</td><td>int</td></tr>
		<tr><td><code>tas.getTotalFrames()</code></td><td>Get total frames in movie</td><td>int</td></tr>
		<tr><td><code>tas.getRerecordCount()</code></td><td>Get rerecord count</td><td>int</td></tr>
		<tr><td><code>tas.getMovieInfo()</code></td><td>Get movie metadata table</td><td>table</td></tr>
	</table>

	<h2>Frame Navigation</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.seekToFrame(frame)</code></td><td>Seek to specific frame using greenzone</td></tr>
		<tr><td><code>tas.frameAdvance()</code></td><td>Advance one frame</td></tr>
		<tr><td><code>tas.frameRewind()</code></td><td>Rewind one frame</td></tr>
		<tr><td><code>tas.pause()</code></td><td>Pause emulation</td></tr>
		<tr><td><code>tas.resume()</code></td><td>Resume emulation</td></tr>
	</table>

	<h2>Input Manipulation</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.getFrameInput(frame, [controller])</code></td><td>Get input for a frame</td></tr>
		<tr><td><code>tas.setFrameInput(frame, input, [controller])</code></td><td>Set input for a frame</td></tr>
		<tr><td><code>tas.clearFrameInput(frame, [controller])</code></td><td>Clear all input for a frame</td></tr>
		<tr><td><code>tas.insertFrames(position, count)</code></td><td>Insert empty frames</td></tr>
		<tr><td><code>tas.deleteFrames(position, count)</code></td><td>Delete frames</td></tr>
	</table>

	<h2>Input Table Format</h2>
	<pre>{
	a = true/false,
	b = true/false,
	x = true/false,
	y = true/false,
	l = true/false,
	r = true/false,
	select = true/false,
	start = true/false,
	up = true/false,
	down = true/false,
	left = true/false,
	right = true/false
}</pre>

	<h2>Greenzone Operations</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.getGreenzoneInfo()</code></td><td>Get greenzone stats</td></tr>
		<tr><td><code>tas.captureGreenzone()</code></td><td>Capture savestate now</td></tr>
		<tr><td><code>tas.clearGreenzone()</code></td><td>Clear all savestates</td></tr>
		<tr><td><code>tas.hasSavestate(frame)</code></td><td>Check if frame has savestate</td></tr>
	</table>

	<h2>Recording Operations</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.startRecording([mode])</code></td><td>Start recording (""append"", ""insert"", ""overwrite"")</td></tr>
		<tr><td><code>tas.stopRecording()</code></td><td>Stop recording</td></tr>
		<tr><td><code>tas.isRecording()</code></td><td>Check if recording active</td></tr>
		<tr><td><code>tas.rerecordFrom(frame)</code></td><td>Rerecord from frame</td></tr>
	</table>

	<h2>Branch Operations</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.createBranch([name])</code></td><td>Create a new branch</td></tr>
		<tr><td><code>tas.getBranches()</code></td><td>Get list of branch names</td></tr>
		<tr><td><code>tas.loadBranch(name)</code></td><td>Load a branch by name</td></tr>
	</table>

	<h2>Input Search (Brute Force)</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.beginSearch()</code></td><td>Begin input search session</td></tr>
		<tr><td><code>tas.testInput(input, [frames])</code></td><td>Test input combination</td></tr>
		<tr><td><code>tas.setSearchState(key, value)</code></td><td>Store search state</td></tr>
		<tr><td><code>tas.getSearchState(key)</code></td><td>Get search state value</td></tr>
		<tr><td><code>tas.markBestResult()</code></td><td>Mark current as best</td></tr>
		<tr><td><code>tas.finishSearch([loadBest])</code></td><td>End search session</td></tr>
	</table>

	<h2>Utility Functions</h2>
	<table>
		<tr><th>Function</th><th>Description</th></tr>
		<tr><td><code>tas.generateInputCombinations([buttons])</code></td><td>Generate all button combos</td></tr>
		<tr><td><code>tas.readMemory(address, [memType])</code></td><td>Read memory byte</td></tr>
		<tr><td><code>tas.writeMemory(address, value, [memType])</code></td><td>Write memory byte</td></tr>
		<tr><td><code>tas.readMemory16(address, [memType])</code></td><td>Read 16-bit value</td></tr>
		<tr><td><code>tas.log(message)</code></td><td>Log to script output</td></tr>
		<tr><td><code>tas.displayMessage(message)</code></td><td>Show on-screen message</td></tr>
	</table>

	<h2>Example: Brute Force Search</h2>
	<pre>-- Find the input that maximizes X position
tas.beginSearch()

local buttons = {""a"", ""b"", ""right""}
local combos = tas.generateInputCombinations(buttons)
local bestX = tas.readMemory(0x0010)

for i, combo in ipairs(combos) do
	local frame = tas.testInput(combo, 1)
	local newX = tas.readMemory(0x0010)

	if newX > bestX then
		bestX = newX
		tas.setSearchState(""bestInput"", combo)
		tas.markBestResult()
	end
end

tas.finishSearch(true) -- Load best result</pre>

</body>
</html>";

		File.WriteAllText(path, html);
	}

	/// <summary>
	/// Opens the TAS scripts folder.
	/// </summary>
	public void OpenScriptFolder() {
		string scriptFolder = Path.Combine(ConfigManager.HomeFolder, "TasScripts");
		Directory.CreateDirectory(scriptFolder);

		try {
			System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo {
				FileName = scriptFolder,
				UseShellExecute = true
			});
		} catch {
			StatusMessage = "Could not open script folder";
		}
	}

	#endregion

	#region Controller Layout

	/// <summary>
	/// Detects the controller layout from the loaded movie.
	/// </summary>
	private void DetectControllerLayout() {
		if (Movie == null) {
			CurrentLayout = ControllerLayout.Snes;
			return;
		}

		// Detect from SystemType first (most reliable)
		CurrentLayout = Movie.SystemType switch {
			SystemType.Nes => ControllerLayout.Nes,
			SystemType.Snes => ControllerLayout.Snes,
			SystemType.Gb or SystemType.Gbc => ControllerLayout.GameBoy,
			SystemType.Gba => ControllerLayout.Gba,
			SystemType.Genesis => ControllerLayout.Genesis,
			SystemType.Sms => ControllerLayout.MasterSystem,
			SystemType.Pce => ControllerLayout.PcEngine,
			SystemType.Ws => ControllerLayout.WonderSwan,
			_ => Movie.SourceFormat switch {
				// Fallback to source format hints
				MovieFormat.Fm2 => ControllerLayout.Nes,
				MovieFormat.Vbm => ControllerLayout.Gba,
				MovieFormat.Gmv => ControllerLayout.Genesis,
				MovieFormat.Smv or MovieFormat.Lsmv => ControllerLayout.Snes,
				_ => ControllerLayout.Snes
			}
		};

		StatusMessage = $"Detected controller layout: {CurrentLayout}";
	}

	/// <summary>
	/// Updates the controller buttons for the current layout.
	/// </summary>
	private void UpdateControllerButtons() {
		ControllerButtons = CurrentLayout switch {
			ControllerLayout.Nes => GetNesButtons(),
			ControllerLayout.Snes => GetSnesButtons(),
			ControllerLayout.GameBoy => GetGameBoyButtons(),
			ControllerLayout.Gba => GetGbaButtons(),
			ControllerLayout.Genesis => GetGenesisButtons(),
			ControllerLayout.MasterSystem => GetMasterSystemButtons(),
			ControllerLayout.PcEngine => GetPcEngineButtons(),
			ControllerLayout.WonderSwan => GetWonderSwanButtons(),
			_ => GetSnesButtons()
		};
	}

	private static List<ControllerButtonInfo> GetNesButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("SELECT", "SEL", 2, 0),
		new("START", "STA", 3, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetSnesButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("X", "X", 2, 0),
		new("Y", "Y", 3, 0),
		new("L", "L", 0, 1),
		new("R", "R", 1, 1),
		new("SELECT", "SEL", 2, 1),
		new("START", "STA", 3, 1),
		new("UP", "↑", 0, 2),
		new("DOWN", "↓", 1, 2),
		new("LEFT", "←", 2, 2),
		new("RIGHT", "→", 3, 2),
	};

	private static List<ControllerButtonInfo> GetGameBoyButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("SELECT", "SEL", 2, 0),
		new("START", "STA", 3, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetGbaButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("L", "L", 2, 0),
		new("R", "R", 3, 0),
		new("SELECT", "SEL", 0, 1),
		new("START", "STA", 1, 1),
		new("UP", "↑", 0, 2),
		new("DOWN", "↓", 1, 2),
		new("LEFT", "←", 2, 2),
		new("RIGHT", "→", 3, 2),
	};

	private static List<ControllerButtonInfo> GetGenesisButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("C", "C", 2, 0),
		new("X", "X", 0, 1),
		new("Y", "Y", 1, 1),
		new("Z", "Z", 2, 1),
		new("START", "STA", 3, 1),
		new("UP", "↑", 0, 2),
		new("DOWN", "↓", 1, 2),
		new("LEFT", "←", 2, 2),
		new("RIGHT", "→", 3, 2),
	};

	private static List<ControllerButtonInfo> GetMasterSystemButtons() => new() {
		new("A", "1", 0, 0),
		new("B", "2", 1, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetPcEngineButtons() => new() {
		new("A", "I", 0, 0),
		new("B", "II", 1, 0),
		new("SELECT", "SEL", 2, 0),
		new("START", "RUN", 3, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetWonderSwanButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("START", "STA", 2, 0),
		new("X", "X1", 0, 1),
		new("Y", "X2", 1, 1),
		new("L", "X3", 2, 1),
		new("R", "X4", 3, 1),
		new("UP", "Y1", 0, 2),
		new("DOWN", "Y2", 1, 2),
		new("LEFT", "Y3", 2, 2),
		new("RIGHT", "Y4", 3, 2),
	};

	#endregion

	private void UpdateWindowTitle() {
		string title = "TAS Editor";

		if (!string.IsNullOrEmpty(FilePath)) {
			title += $" - {Path.GetFileName(FilePath)}";
		}

		if (HasUnsavedChanges) {
			title += " *";
		}

		WindowTitle = title;
	}
}

/// <summary>
/// ViewModel representing a single frame in the TAS editor.
/// </summary>
public class TasFrameViewModel : ViewModelBase {
	/// <summary>Gets the underlying frame.</summary>
	public InputFrame Frame { get; }

	/// <summary>Gets the frame number (1-based for display).</summary>
	public int FrameNumber { get; }

	/// <summary>Gets whether this frame is in the greenzone.</summary>
	public bool IsGreenzone { get; }

	/// <summary>Gets the background color based on frame state.</summary>
	public IBrush Background => IsGreenzone ? Brushes.LightGreen
		: Frame.IsLagFrame ? Brushes.LightCoral
		: Brushes.Transparent;

	/// <summary>Gets the formatted input string for player 1.</summary>
	public string P1Input => Frame.Controllers[0].ToNexenFormat();

	/// <summary>Gets the formatted input string for player 2.</summary>
	public string P2Input => Frame.Controllers.Length > 1 ? Frame.Controllers[1].ToNexenFormat() : "";

	/// <summary>Gets the marker text (using Comment as marker).</summary>
	public string MarkerText => Frame.Comment ?? "";

	/// <summary>Gets the comment text.</summary>
	public string CommentText => Frame.Comment ?? "";

	/// <summary>Gets whether this is a lag frame.</summary>
	public bool IsLagFrame => Frame.IsLagFrame;

	/// <summary>Gets whether this frame has a marker/comment.</summary>
	public bool HasMarker => !string.IsNullOrEmpty(Frame.Comment);

	public TasFrameViewModel(InputFrame frame, int index, bool isGreenzone) {
		Frame = frame;
		FrameNumber = index + 1; // 1-based display
		IsGreenzone = isGreenzone;
	}
}

#region Undoable Actions

/// <summary>
/// Base class for undoable actions in the TAS editor.
/// </summary>
public abstract class UndoableAction {
	/// <summary>Gets a description of this action.</summary>
	public abstract string Description { get; }

	/// <summary>Executes the action.</summary>
	public abstract void Execute();

	/// <summary>Undoes the action.</summary>
	public abstract void Undo();
}

/// <summary>
/// Action for inserting frames.
/// </summary>
public class InsertFramesAction : UndoableAction {
	private readonly MovieData _movie;
	private readonly int _index;
	private readonly List<InputFrame> _frames;

	public override string Description => $"Insert {_frames.Count} frame(s)";

	public InsertFramesAction(MovieData movie, int index, List<InputFrame> frames) {
		_movie = movie;
		_index = index;
		_frames = frames;
	}

	public override void Execute() {
		for (int i = 0; i < _frames.Count; i++) {
			_movie.InputFrames.Insert(_index + i, _frames[i]);
		}
	}

	public override void Undo() {
		for (int i = 0; i < _frames.Count; i++) {
			_movie.InputFrames.RemoveAt(_index);
		}
	}
}

/// <summary>
/// Action for deleting frames.
/// </summary>
public class DeleteFramesAction : UndoableAction {
	private readonly MovieData _movie;
	private readonly int _index;
	private readonly List<InputFrame> _deletedFrames;

	public override string Description => $"Delete {_deletedFrames.Count} frame(s)";

	public DeleteFramesAction(MovieData movie, int index, int count) {
		_movie = movie;
		_index = index;
		_deletedFrames = _movie.InputFrames.Skip(index).Take(count).ToList();
	}

	public override void Execute() {
		for (int i = 0; i < _deletedFrames.Count; i++) {
			_movie.InputFrames.RemoveAt(_index);
		}
	}

	public override void Undo() {
		for (int i = 0; i < _deletedFrames.Count; i++) {
			_movie.InputFrames.Insert(_index + i, _deletedFrames[i]);
		}
	}
}

/// <summary>
/// Action for modifying controller input.
/// </summary>
public class ModifyInputAction : UndoableAction {
	private readonly InputFrame _frame;
	private readonly int _port;
	private readonly ControllerInput _oldInput;
	private readonly ControllerInput _newInput;

	public override string Description => "Modify input";

	public ModifyInputAction(InputFrame frame, int port, ControllerInput newInput) {
		_frame = frame;
		_port = port;
		_oldInput = CloneInput(frame.Controllers[port]);
		_newInput = newInput;
	}

	public override void Execute() {
		_frame.Controllers[_port] = _newInput;
	}

	public override void Undo() {
		_frame.Controllers[_port] = _oldInput;
	}

	private static ControllerInput CloneInput(ControllerInput src) => new() {
		A = src.A,
		B = src.B,
		X = src.X,
		Y = src.Y,
		L = src.L,
		R = src.R,
		Up = src.Up,
		Down = src.Down,
		Left = src.Left,
		Right = src.Right,
		Start = src.Start,
		Select = src.Select
	};
}

#endregion

#region Controller Layout Types

/// <summary>
/// Supported controller layouts for different systems.
/// </summary>
public enum ControllerLayout {
	/// <summary>NES - A, B, Select, Start, D-Pad</summary>
	Nes,

	/// <summary>SNES - A, B, X, Y, L, R, Select, Start, D-Pad</summary>
	Snes,

	/// <summary>Game Boy / Game Boy Color - A, B, Select, Start, D-Pad</summary>
	GameBoy,

	/// <summary>Game Boy Advance - A, B, L, R, Select, Start, D-Pad</summary>
	Gba,

	/// <summary>Sega Genesis/Mega Drive - A, B, C, X, Y, Z, Start, D-Pad</summary>
	Genesis,

	/// <summary>Sega Master System / Game Gear - 1, 2, D-Pad</summary>
	MasterSystem,

	/// <summary>PC Engine / TurboGrafx-16 - I, II, Select, Run, D-Pad</summary>
	PcEngine,

	/// <summary>WonderSwan - A, B, Start, X1-X4, Y1-Y4</summary>
	WonderSwan
}

/// <summary>
/// Information about a controller button for UI display.
/// </summary>
public class ControllerButtonInfo {
	/// <summary>Gets the button identifier used in ControllerInput.</summary>
	public string ButtonId { get; }

	/// <summary>Gets the display label for the button.</summary>
	public string Label { get; }

	/// <summary>Gets the column position in the button grid.</summary>
	public int Column { get; }

	/// <summary>Gets the row position in the button grid.</summary>
	public int Row { get; }

	public ControllerButtonInfo(string buttonId, string label, int column, int row) {
		ButtonId = buttonId;
		Label = label;
		Column = column;
		Row = row;
	}
}

#endregion
