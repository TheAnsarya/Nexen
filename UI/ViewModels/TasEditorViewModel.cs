using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using Avalonia.Media;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.MovieConverter;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;

/// <summary>
/// ViewModel for the TAS Editor window.
/// Provides frame-by-frame editing of movie files.
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

	private IMovieConverter? _currentConverter;
	private Windows.TasEditorWindow? _window;

	public TasEditorViewModel() {
		AddDisposable(this.WhenAnyValue(x => x.Movie).Subscribe(_ => UpdateFrames()));
		AddDisposable(this.WhenAnyValue(x => x.FilePath, x => x.HasUnsavedChanges).Subscribe(_ => UpdateWindowTitle()));
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
