using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using Avalonia.Media;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.MovieConverter;
using Nexen.Services;
using Nexen.TAS;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;

/// <summary>
/// ViewModel for the TAS Editor window.
/// Provides frame-by-frame editing of movie files with greenzone support,
/// input recording, and piano roll visualization.
/// </summary>
public sealed partial class TasEditorViewModel : DisposableViewModel {
	/// <summary>Gets the currently loaded movie data.</summary>
	[Reactive] public partial MovieData? Movie { get; private set; }

	/// <summary>Gets or sets the current file path.</summary>
	[Reactive] public partial string? FilePath { get; set; }

	/// <summary>Gets or sets whether the movie has unsaved changes.</summary>
	[Reactive] public partial bool HasUnsavedChanges { get; set; }

	/// <summary>Gets or sets the currently selected frame index.</summary>
	[Reactive] public partial int SelectedFrameIndex { get; set; } = -1;

	/// <summary>Gets or sets the list of selected frame indices for multi-selection.</summary>
	[Reactive] public partial List<int> SelectedFrameIndices { get; set; } = new();

	/// <summary>Gets whether multiple frames are selected.</summary>
	public bool HasMultipleSelection => SelectedFrameIndices.Count > 1;

	/// <summary>Gets whether the selected frame is a lag frame.</summary>
	public bool SelectedFrameIsLag =>
		Movie is not null &&
		SelectedFrameIndex >= 0 &&
		SelectedFrameIndex < Movie.InputFrames.Count &&
		Movie.InputFrames[SelectedFrameIndex].IsLagFrame;

	/// <summary>Gets the observable collection of frame view models.</summary>
	public RangeObservableCollection<TasFrameViewModel> Frames { get; } = new();

	/// <summary>Gets or sets the status message.</summary>
	[Reactive] public partial string StatusMessage { get; set; } = "No movie loaded";

	/// <summary>Gets or sets whether greenzone (rerecord safe zone) is enabled.</summary>
	[Reactive] public partial bool IsGreenzoneEnabled { get; set; } = true;

	/// <summary>Gets or sets the greenzone start frame.</summary>
	[Reactive] public partial int GreenzoneStart { get; set; }

	/// <summary>Gets the window title.</summary>
	[Reactive] public partial string WindowTitle { get; private set; } = "TAS Editor";

	/// <summary>Gets the file menu items.</summary>
	[Reactive] public partial List<object> FileMenuItems { get; private set; } = new();

	/// <summary>Gets the edit menu items.</summary>
	[Reactive] public partial List<object> EditMenuItems { get; private set; } = new();

	/// <summary>Gets the view menu items.</summary>
	[Reactive] public partial List<object> ViewMenuItems { get; private set; } = new();

	/// <summary>Gets the playback menu items.</summary>
	[Reactive] public partial List<object> PlaybackMenuItems { get; private set; } = new();

	/// <summary>Gets the recording menu items.</summary>
	[Reactive] public partial List<object> RecordingMenuItems { get; private set; } = new();

	/// <summary>Gets whether undo is available.</summary>
	[Reactive] public partial bool CanUndo { get; private set; }

	/// <summary>Gets whether redo is available.</summary>
	[Reactive] public partial bool CanRedo { get; private set; }

	/// <summary>Gets whether clipboard has content.</summary>
	[Reactive] public partial bool HasClipboard { get; private set; }

	/// <summary>Gets or sets whether playback is active.</summary>
	[Reactive] public partial bool IsPlaying { get; set; }

	/// <summary>Gets or sets whether TAS auto-save is enabled.</summary>
	[Reactive] public partial bool AutoSaveEnabled { get; set; } = true;

	/// <summary>Gets or sets the TAS auto-save interval in minutes.</summary>
	[Reactive] public partial int AutoSaveIntervalMinutes { get; set; } = 5;

	/// <summary>Gets or sets the current playback frame.</summary>
	[Reactive] public partial int PlaybackFrame { get; set; }

	/// <summary>Gets or sets the playback speed (1.0 = normal).</summary>
	[Reactive] public partial double PlaybackSpeed { get; set; } = 1.0;

	/// <summary>Gets or sets the current controller layout.</summary>
	[Reactive] public partial ControllerLayout CurrentLayout { get; set; } = ControllerLayout.Snes;

	/// <summary>Gets the controller buttons for the current layout.</summary>
	[Reactive] public partial List<ControllerButtonInfo> ControllerButtons { get; private set; } = new();

	/// <summary>Gets a raw hex dump of the selected frame's controller bytes.</summary>
	[Reactive] public partial string SelectedFrameHexPreview { get; private set; } = "No frame selected";

	/// <summary>Gets the visual input preview buttons for the selected frame.</summary>
	public ObservableCollection<ControllerButtonPreviewViewModel> InputPreviewButtons { get; } = new();

	/// <summary>Gets the grid column count for the visual input preview.</summary>
	[Reactive] public partial int InputPreviewColumns { get; private set; } = 4;

	/// <summary>Gets whether paddle coordinate editing is visible for the selected frame/controller.</summary>
	[Reactive] public partial bool IsPaddleCoordinateEditorVisible { get; private set; }

	/// <summary>Gets or sets the selected paddle position (0-255) for the selected port on the selected frame.</summary>
	[Reactive] public partial int SelectedPaddlePosition { get; set; }

	/// <summary>Gets whether the console switch panel is visible (Atari 2600 only).</summary>
	[Reactive] public partial bool IsConsoleSwitchPanelVisible { get; private set; }

	/// <summary>Gets whether the Select console switch is active on the selected frame.</summary>
	[Reactive] public partial bool IsSelectedFrameSelectActive { get; private set; }

	/// <summary>Gets whether the Reset console switch is active on the selected frame.</summary>
	[Reactive] public partial bool IsSelectedFrameResetActive { get; private set; }

	/// <summary>Gets or sets the currently selected controller port for editing (0-based).</summary>
	[Reactive] public partial int SelectedEditPort { get; set; }

	/// <summary>Gets the number of active controller ports in the current movie.</summary>
	[Reactive] public partial int ActivePortCount { get; private set; } = 1;

	/// <summary>Gets whether the port selector should be visible (more than one active port).</summary>
	public bool IsPortSelectorVisible => ActivePortCount > 1;

	/// <summary>Gets the maximum valid port index (0-based, for UI binding).</summary>
	public int MaxEditPort => Math.Max(0, ActivePortCount - 1);

	/// <summary>Gets the piano roll button labels derived from the current controller layout.</summary>
	public IReadOnlyList<string> PianoRollButtonLabels => _pianoRollButtonLabelsCache ??= ControllerButtons.Select(b => b.Label).ToList();
	private IReadOnlyList<string>? _pianoRollButtonLabelsCache;

	/// <summary>Gets the available controller layouts.</summary>
	public static IReadOnlyList<ControllerLayout> AvailableLayouts { get; } =
		Enum.GetValues<ControllerLayout>();

	/// <summary>Gets the available search modes for the ComboBox.</summary>
	public static IReadOnlyList<FrameSearchMode> SearchModes { get; } =
		Enum.GetValues<FrameSearchMode>();

	/// <summary>Gets or sets whether recording is active.</summary>
	[Reactive] public partial bool IsRecording { get; set; }

	/// <summary>Gets or sets the current recording mode.</summary>
	[Reactive] public partial RecordingMode RecordMode { get; set; } = RecordingMode.Append;

	/// <summary>Gets the greenzone savestate count.</summary>
	[Reactive] public partial int SavestateCount { get; private set; }

	/// <summary>Gets the greenzone memory usage in MB.</summary>
	[Reactive] public partial double GreenzoneMemoryMB { get; private set; }

	/// <summary>Gets or sets whether piano roll view is visible.</summary>
	[Reactive] public partial bool ShowPianoRoll { get; set; }

	/// <summary>Gets the rerecord count for the current movie.</summary>
	[Reactive] public partial int RerecordCount { get; private set; }

	/// <summary>Gets the list of saved branches.</summary>
	public ObservableCollection<BranchData> Branches { get; } = new();

	/// <summary>Gets the script menu items.</summary>
	[Reactive] public partial List<object> ScriptMenuItems { get; private set; } = new();

	/// <summary>Gets the Lua API for TAS scripting.</summary>
	public TasLuaApi LuaApi { get; }

	/// <summary>Gets or sets the current Lua script path.</summary>
	[Reactive] public partial string? CurrentScriptPath { get; set; }

	/// <summary>Gets or sets whether a script is running.</summary>
	[Reactive] public partial bool IsScriptRunning { get; set; }

	/// <summary>The ID of the currently loaded script (-1 if none).</summary>
	private int _currentScriptId = -1;

	/// <summary>Maximum number of undo actions retained before oldest entries are discarded.</summary>
	private const int MaxUndoHistory = 500;

	/// <summary>
	/// When the undo stack exceeds MaxUndoHistory, trim to this size.
	/// This hysteresis avoids O(n) rebuild on every single action past the cap.
	/// </summary>
	private const int UndoTrimTarget = 400;

	private readonly Stack<UndoableAction> _undoStack = new();
	private readonly Stack<UndoableAction> _redoStack = new();
	private List<InputFrame>? _clipboard;
	private int _clipboardStartIndex;
	private IMovieConverter? _currentConverter;
	private Windows.TasEditorWindow? _window;
	private DispatcherTimer? _autoSaveTimer;
	private bool _isAutoSaving;
	private bool _isUpdatingSelectedPaddlePosition;
	private Func<string, System.Threading.Tasks.Task<DialogResult>>? _recoveryPromptOverride;

	/// <summary>Gets the greenzone manager for savestate management.</summary>
	public GreenzoneManager Greenzone { get; } = new();

	/// <summary>Gets the input recorder for TAS recording.</summary>
	public InputRecorder Recorder { get; }

	/// <summary>Gets or sets whether the search bar is visible.</summary>
	[Reactive] public partial bool IsSearchVisible { get; set; }

	/// <summary>Gets or sets the search query text.</summary>
	[Reactive] public partial string SearchText { get; set; } = "";

	/// <summary>Gets or sets the search mode.</summary>
	[Reactive] public partial FrameSearchMode SearchMode { get; set; } = FrameSearchMode.Comment;

	/// <summary>Gets the search results (matching frame indices).</summary>
	[Reactive] public partial List<int> SearchResults { get; private set; } = new();

	/// <summary>Gets the current search result index within SearchResults.</summary>
	[Reactive] public partial int SearchResultIndex { get; set; } = -1;

	/// <summary>Gets the search results summary text.</summary>
	public string SearchSummary =>
		SearchResults.Count > 0
			? $"{SearchResultIndex + 1} of {SearchResults.Count}"
			: "No results";

	/// <summary>Gets marker/comment entries for the marker panel.</summary>
	public ObservableCollection<TasMarkerEntryViewModel> MarkerEntries { get; } = new();

	/// <summary>Gets or sets the selected marker/comment entry.</summary>
	[Reactive] public partial TasMarkerEntryViewModel? SelectedMarkerEntry { get; set; }

	/// <summary>Gets or sets the marker entry filter mode.</summary>
	[Reactive] public partial MarkerEntryFilterType MarkerEntryFilter { get; set; } = MarkerEntryFilterType.All;

	/// <summary>Gets the available marker entry filters.</summary>
	public static IReadOnlyList<MarkerEntryFilterType> MarkerEntryFilters { get; } =
		Enum.GetValues<MarkerEntryFilterType>();

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
			SyncNewFrames();
		};

		Recorder.FrameRecorded += (_, e) => {
			PlaybackFrame = e.FrameIndex;
			// Only append new frames instead of full rebuild - O(1) per frame vs O(n)
			SyncNewFrames();
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
			UpdateActivePortCount();
			RefreshSelectedFramePreview();
		}));
		AddDisposable(this.WhenAnyValue(x => x.FilePath, x => x.HasUnsavedChanges).Subscribe(_ => UpdateWindowTitle()));
		AddDisposable(this.WhenAnyValue(x => x.CurrentLayout).Subscribe(_ => {
			UpdateControllerButtons();
			RefreshSelectedFramePreview();
		}));
		AddDisposable(this.WhenAnyValue(x => x.SelectedFrameIndex).Subscribe(_ => {
			this.RaisePropertyChanged(nameof(SelectedFrameIsLag));
			UpdateActivePortCount();
			RefreshSelectedFramePreview();
		}));
		AddDisposable(this.WhenAnyValue(x => x.SelectedEditPort).Subscribe(_ => {
			UpdateControllerButtons();
			RefreshSelectedFramePreview();
		}));
		AddDisposable(this.WhenAnyValue(x => x.SelectedPaddlePosition).Subscribe(ApplySelectedPaddlePosition));
		AddDisposable(this.WhenAnyValue(x => x.AutoSaveEnabled).Subscribe(_ => RestartAutoSaveTimer()));
		AddDisposable(this.WhenAnyValue(x => x.MarkerEntryFilter).Subscribe(_ => RefreshMarkerEntries()));
		AddDisposable(this.WhenAnyValue(x => x.AutoSaveIntervalMinutes).Subscribe(minutes => {
			int clamped = Math.Clamp(minutes, 1, 60);
			if (minutes != clamped) {
				AutoSaveIntervalMinutes = clamped;
				return;
			}
			RestartAutoSaveTimer();
		}));
		UpdateControllerButtons();
		RefreshSelectedFramePreview();
		RestartAutoSaveTimer();
	}

	private void RefreshSelectedFramePreview() {
		SelectedFrameHexPreview = BuildFrameHexPreview(Movie, SelectedFrameIndex);

		InputPreviewButtons.Clear();
		int port = SelectedEditPort;
		ControllerInput? selectedController = GetSelectedControllerInput(port);
		IsPaddleCoordinateEditorVisible = CurrentLayout == ControllerLayout.Atari2600
			&& (Movie?.PortTypes is { Length: > 0 } && port < Movie.PortTypes.Length)
			&& Movie.PortTypes[port] == MovieConverter.ControllerType.Atari2600Paddle
			&& selectedController is not null;

		_isUpdatingSelectedPaddlePosition = true;
		SelectedPaddlePosition = selectedController?.PaddlePosition ?? 0;
		_isUpdatingSelectedPaddlePosition = false;

		// Update console switch panel visibility and state
		IsConsoleSwitchPanelVisible = CurrentLayout == ControllerLayout.Atari2600;
		if (IsConsoleSwitchPanelVisible && Movie is not null
			&& SelectedFrameIndex >= 0 && SelectedFrameIndex < Movie.InputFrames.Count) {
			var cmd = Movie.InputFrames[SelectedFrameIndex].Command;
			IsSelectedFrameSelectActive = cmd.HasFlag(FrameCommand.Atari2600Select);
			IsSelectedFrameResetActive = cmd.HasFlag(FrameCommand.Atari2600Reset);
		} else {
			IsSelectedFrameSelectActive = false;
			IsSelectedFrameResetActive = false;
		}

		List<ControllerButtonPreviewViewModel> previewButtons = BuildInputPreviewButtons(ControllerButtons, selectedController);
		foreach (ControllerButtonPreviewViewModel previewButton in previewButtons) {
			InputPreviewButtons.Add(previewButton);
		}

		InputPreviewColumns = Math.Max(1, ControllerButtons.Count > 0
			? ControllerButtons.Max(button => button.Column) + 1
			: 1);
	}

	private void ApplySelectedPaddlePosition(int position) {
		if (_isUpdatingSelectedPaddlePosition || Movie is null || !IsPaddleCoordinateEditorVisible) {
			return;
		}

		if (SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		InputFrame frame = Movie.InputFrames[SelectedFrameIndex];
		if (frame.Controllers.Length == 0) {
			return;
		}

		byte clamped = (byte)Math.Clamp(position, 0, 255);
		int port = SelectedEditPort;
		if (port < 0 || port >= frame.Controllers.Length) {
			return;
		}

		ControllerInput newInput = CloneControllerInput(frame.Controllers[port]);
		if (newInput.PaddlePosition == clamped) {
			return;
		}

		newInput.PaddlePosition = clamped;
		ExecuteAction(new ModifyInputAction(frame, SelectedFrameIndex, port, newInput));
	}

	private void UpdateActivePortCount() {
		if (Movie is null || Movie.InputFrames.Count == 0) {
			ActivePortCount = 1;
		} else {
			// Use actual controller array length from frames as ground truth,
			// falling back to Movie.ControllerCount
			int fromFrames = Movie.InputFrames[0].Controllers.Length;
			ActivePortCount = Math.Max(1, fromFrames > 0 ? fromFrames : Movie.ControllerCount);
		}

		// Clamp selected port to valid range
		if (SelectedEditPort >= ActivePortCount) {
			SelectedEditPort = 0;
		}

		this.RaisePropertyChanged(nameof(IsPortSelectorVisible));
		this.RaisePropertyChanged(nameof(MaxEditPort));
	}

	private ControllerInput? GetSelectedControllerInput(int port) {
		if (Movie is null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return null;
		}

		ControllerInput[] controllers = Movie.InputFrames[SelectedFrameIndex].Controllers;
		if (port < 0 || port >= controllers.Length) {
			return null;
		}

		return controllers[port];
	}

	internal static string BuildFrameHexPreview(MovieData? movie, int selectedFrameIndex) {
		if (movie is null || selectedFrameIndex < 0 || selectedFrameIndex >= movie.InputFrames.Count) {
			return "No frame selected";
		}

		InputFrame frame = movie.InputFrames[selectedFrameIndex];
		if (frame.Controllers.Length == 0) {
			return "No controller data";
		}

		var sb = new StringBuilder();
		for (int i = 0; i < frame.Controllers.Length; i++) {
			ControllerInput controller = frame.Controllers[i];
			if (i > 0) {
				sb.AppendLine();
			}

			sb.Append('P');
			sb.Append(i + 1);
			sb.Append(" [");
			sb.Append(controller.Type);
			sb.Append("] ");

			byte[] bytes = BuildControllerPreviewBytes(controller);
			for (int j = 0; j < bytes.Length; j++) {
				if (j > 0) {
					sb.Append(' ');
				}
				sb.Append(bytes[j].ToString("x2"));
			}
		}

		return sb.ToString();
	}

	internal static byte[] BuildControllerPreviewBytes(ControllerInput input) {
		var bytes = new List<byte>(16);
		ushort buttonBits = input.ButtonBits;
		bytes.Add((byte)(buttonBits & 0xff));
		bytes.Add((byte)(buttonBits >> 8));

		if (input.AnalogX.HasValue) {
			bytes.Add(unchecked((byte)input.AnalogX.Value));
		}

		if (input.AnalogY.HasValue) {
			bytes.Add(unchecked((byte)input.AnalogY.Value));
		}

		if (input.AnalogRX.HasValue) {
			bytes.Add(unchecked((byte)input.AnalogRX.Value));
		}

		if (input.AnalogRY.HasValue) {
			bytes.Add(unchecked((byte)input.AnalogRY.Value));
		}

		if (input.TriggerL.HasValue) {
			bytes.Add(input.TriggerL.Value);
		}

		if (input.TriggerR.HasValue) {
			bytes.Add(input.TriggerR.Value);
		}

		if (input.KeyboardData is { Length: > 0 }) {
			int len = Math.Min(8, input.KeyboardData.Length);
			for (int i = 0; i < len; i++) {
				bytes.Add(input.KeyboardData[i]);
			}
		}

		return [.. bytes];
	}

	internal static List<ControllerButtonPreviewViewModel> BuildInputPreviewButtons(IReadOnlyList<ControllerButtonInfo> layoutButtons, ControllerInput? input) {
		return layoutButtons
			.OrderBy(button => button.Row)
			.ThenBy(button => button.Column)
			.Select(button => new ControllerButtonPreviewViewModel(
				button.ButtonId,
				button.Label,
				button.Column,
				button.Row,
				input?.GetButton(button.ButtonId) == true))
			.ToList();
	}

	/// <summary>
	/// Initializes the menu actions.
	/// </summary>
	public void InitActions(Windows.TasEditorWindow wnd) {
		_window = wnd;

		FileMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Create New",
				OnClick = CreateNewMovie
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Open",
				OnClick = () => _ = OpenFileAsync()
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Save",
				OnClick = () => _ = SaveFileAsync(),
				IsEnabled = () => Movie is not null && _currentConverter?.CanWrite == true
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Save As...",
				OnClick = () => _ = SaveAsFileAsync(),
				IsEnabled = () => Movie is not null
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
				IsEnabled = () => Movie is not null
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
				IsEnabled = () => Movie is not null && SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Copy,
				OnClick = Copy,
				IsEnabled = () => Movie is not null && SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Paste,
				OnClick = Paste,
				IsEnabled = () => HasClipboard && Movie is not null
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Select All",
				OnClick = SelectAllFrames,
				IsEnabled = () => Movie is not null && Movie.InputFrames.Count > 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Select None",
				OnClick = SelectNoFrames,
				IsEnabled = () => SelectedFrameIndices.Count > 0 || SelectedFrameIndex >= 0
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Select Range...",
				OnClick = () => _ = SelectRangeDialogAsync(),
				IsEnabled = () => Movie is not null && Movie.InputFrames.Count > 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Insert Frame(s)",
				OnClick = InsertFrames,
				IsEnabled = () => Movie is not null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Insert N Frame(s)...",
				OnClick = () => _ = BulkInsertDialogAsync(),
				IsEnabled = () => Movie is not null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Delete Frame(s)",
				OnClick = DeleteFrames,
				IsEnabled = () => Movie is not null && SelectedFrameIndex >= 0
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
				CustomText = "Bulk Set Button...",
				OnClick = () => _ = BulkSetButtonDialogAsync(true),
				IsEnabled = () => Movie is not null && (SelectedFrameIndices.Count > 0 || SelectedFrameIndex >= 0)
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Bulk Clear Button...",
				OnClick = () => _ = BulkSetButtonDialogAsync(false),
				IsEnabled = () => Movie is not null && (SelectedFrameIndices.Count > 0 || SelectedFrameIndex >= 0)
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Pattern Fill...",
				OnClick = () => _ = PatternFillDialogAsync(),
				IsEnabled = () => Movie is not null && (SelectedFrameIndices.Count > 0 || SelectedFrameIndex >= 0)
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Set Comment",
				OnClick = () => _ = SetCommentAsync(),
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
				OnClick = () => _ = ShowGreenzoneSettingsAsync()
			}
		};

		PlaybackMenuItems = new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Play/Pause",
				OnClick = TogglePlayback,
				IsEnabled = () => Movie is not null
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
				IsEnabled = () => Movie is not null
			},
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Frame Rewind",
				OnClick = FrameRewind,
				IsEnabled = () => Movie is not null && PlaybackFrame > 0
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Custom,
				CustomText = "Seek to Frame...",
				OnClick = () => _ = SeekToFrameAsync(),
				IsEnabled = () => Movie is not null && Greenzone.SavestateCount > 0
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
				IsEnabled = () => Movie is not null && !IsRecording
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
				IsEnabled = () => Movie is not null && SelectedFrameIndex >= 0
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
				IsEnabled = () => Movie is not null
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
				IsEnabled = () => Movie is not null
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
			if (await TryRecoverAutoSaveAsync(path)) {
				return;
			}

			var format = MovieConverterRegistry.DetectFormat(path);
			var converter = MovieConverterRegistry.GetConverter(format);

			if (converter is null) {
				StatusMessage = $"Unsupported format: {format}";
				return;
			}

			await using var stream = File.OpenRead(path);
			Movie = await converter.ReadAsync(stream);
			FilePath = path;
			_currentConverter = converter;
			HasUnsavedChanges = false;
			RefreshMarkerEntries();
			StatusMessage = $"Loaded {Movie.InputFrames.Count:N0} frames from {Path.GetFileName(path)}";
		} catch (Exception ex) {
			StatusMessage = $"Error loading file: {ex.Message}";
		}
	}

	/// <summary>
	/// Saves the current movie to the existing file.
	/// </summary>
	public async System.Threading.Tasks.Task SaveFileAsync() {
		if (Movie is null || _currentConverter is null || !_currentConverter.CanWrite) {
			return;
		}

		if (string.IsNullOrEmpty(FilePath)) {
			await SaveAsFileAsync();
			return;
		}

		try {
			await using var stream = File.Create(FilePath);
			await _currentConverter.WriteAsync(Movie, stream);
			DeleteAutoSaveFile(FilePath);
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
		if (Movie is null) {
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

			if (converter is null || !converter.CanWrite) {
				StatusMessage = $"Cannot write to format: {format}";
				return;
			}

			try {
				await using var stream = File.Create(path);
				await converter.WriteAsync(Movie, stream);
				FilePath = path;
				_currentConverter = converter;
				DeleteAutoSaveFile(path);
				HasUnsavedChanges = false;
				StatusMessage = $"Saved {Movie.InputFrames.Count:N0} frames to {Path.GetFileName(path)}";
			} catch (Exception ex) {
				StatusMessage = $"Error saving file: {ex.Message}";
			}
		}
	}

	private void RestartAutoSaveTimer() {
		_autoSaveTimer?.Stop();
		if (!AutoSaveEnabled) {
			return;
		}

		_autoSaveTimer ??= new DispatcherTimer();
		_autoSaveTimer.Interval = TimeSpan.FromMinutes(Math.Clamp(AutoSaveIntervalMinutes, 1, 60));
		_autoSaveTimer.Tick -= OnAutoSaveTimerTick;
		_autoSaveTimer.Tick += OnAutoSaveTimerTick;
		_autoSaveTimer.Start();
	}

	private async void OnAutoSaveTimerTick(object? sender, EventArgs e) {
		try {
			await TryAutoSaveAsync();
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Auto-save timer tick failed: {ex.Message}");
		}
	}

	internal static string BuildAutoSavePath(string moviePath) {
		string dir = Path.GetDirectoryName(moviePath) ?? ".";
		string name = Path.GetFileNameWithoutExtension(moviePath);
		string ext = Path.GetExtension(moviePath);
		return Path.Combine(dir, $"{name}.autosave{ext}");
	}

	internal bool ShouldAutoSaveNow() {
		return AutoSaveEnabled
			&& !IsPlaying
			&& HasUnsavedChanges
			&& Movie is not null
			&& _currentConverter?.CanWrite == true
			&& !string.IsNullOrWhiteSpace(FilePath)
			&& !_isAutoSaving;
	}

	internal async System.Threading.Tasks.Task<bool> TryAutoSaveAsync() {
		if (!ShouldAutoSaveNow() || Movie is null || _currentConverter is null || string.IsNullOrWhiteSpace(FilePath)) {
			return false;
		}

		string autoSavePath = BuildAutoSavePath(FilePath);
		try {
			_isAutoSaving = true;
			string? directory = Path.GetDirectoryName(autoSavePath);
			if (!string.IsNullOrEmpty(directory)) {
				Directory.CreateDirectory(directory);
			}

			await using var stream = File.Create(autoSavePath);
			await _currentConverter.WriteAsync(Movie, stream);
			StatusMessage = $"Auto-saved snapshot to {Path.GetFileName(autoSavePath)}";
			return true;
		} catch (Exception ex) {
			StatusMessage = $"Auto-save failed: {ex.Message}";
			return false;
		} finally {
			_isAutoSaving = false;
		}
	}

	internal static bool ShouldPromptAutoSaveRecovery(string moviePath, string autoSavePath) {
		if (!File.Exists(autoSavePath)) {
			return false;
		}

		if (!File.Exists(moviePath)) {
			return true;
		}

		DateTime autoSaveTime = File.GetLastWriteTimeUtc(autoSavePath);
		DateTime movieTime = File.GetLastWriteTimeUtc(moviePath);
		return autoSaveTime >= movieTime;
	}

	internal async System.Threading.Tasks.Task<bool> TryRecoverAutoSaveAsync(string path) {
		string autoSavePath = BuildAutoSavePath(path);
		if (!ShouldPromptAutoSaveRecovery(path, autoSavePath)) {
			return false;
		}

		DialogResult result = _recoveryPromptOverride is not null
			? await _recoveryPromptOverride(autoSavePath)
			: await MessageBox.Show(
				_window,
				$"An autosave snapshot was found for this movie. Recover it now?\n\nSnapshot: {Path.GetFileName(autoSavePath)}",
				"TAS Editor Recovery",
				MessageBoxButtons.YesNo,
				MessageBoxIcon.Question);

		if (result != DialogResult.Yes) {
			return false;
		}

		var format = MovieConverterRegistry.DetectFormat(path);
		var converter = MovieConverterRegistry.GetConverter(format);
		if (converter is null || !converter.CanRead) {
			return false;
		}

		await using var stream = File.OpenRead(autoSavePath);
		Movie = await converter.ReadAsync(stream);
		FilePath = path;
		_currentConverter = converter;
		HasUnsavedChanges = true;
		StatusMessage = $"Recovered autosave snapshot from {Path.GetFileName(autoSavePath)}";
		return true;
	}

	internal void SetRecoveryPromptOverride(Func<string, System.Threading.Tasks.Task<DialogResult>>? prompt) {
		_recoveryPromptOverride = prompt;
	}

	private static void DeleteAutoSaveFile(string path) {
		string autoSavePath = BuildAutoSavePath(path);
		if (File.Exists(autoSavePath)) {
			File.Delete(autoSavePath);
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
		if (Movie is null) {
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

			if (converter is null || !converter.CanWrite) {
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
		if (Movie is null) {
			return;
		}

		int insertAt = Math.Max(0, SelectedFrameIndex);
		InsertFramesAt(insertAt, 1);
	}

	/// <summary>
	/// Inserts a specified number of frames at the requested position.
	/// </summary>
	public void InsertFramesAt(int position, int count) {
		if (Movie is null || count <= 0) {
			return;
		}

		int insertAt = Math.Clamp(position, 0, Movie.InputFrames.Count);
		var frames = new List<InputFrame>(count);
		for (int i = 0; i < count; i++) {
			frames.Add(CreateEmptyFrame(insertAt + i));
		}

		ExecuteAction(new InsertFramesAction(Movie, insertAt, frames));
		SelectedFrameIndex = insertAt;
		SelectedFrameIndices = Enumerable.Range(insertAt, count).ToList();
		StatusMessage = count == 1
			? $"Inserted frame at {insertAt}"
			: $"Inserted {count} frames at {insertAt}";
	}

	/// <summary>
	/// Inserts a blank frame above the current selection.
	/// </summary>
	public void InsertFrameAbove() {
		if (Movie is null) {
			return;
		}

		int insertAt = Math.Max(0, SelectedFrameIndex);
		InsertFramesAt(insertAt, 1);
		StatusMessage = $"Inserted frame above at {insertAt}";
	}

	/// <summary>
	/// Inserts a blank frame below the current selection.
	/// </summary>
	public void InsertFrameBelow() {
		if (Movie is null) {
			return;
		}

		int insertAt = Math.Min(Movie.InputFrames.Count, SelectedFrameIndex + 1);
		InsertFramesAt(insertAt, 1);
		StatusMessage = $"Inserted frame below at {insertAt}";
	}

	/// <summary>
	/// Selects all frames in the loaded movie.
	/// </summary>
	public void SelectAllFrames() {
		if (Movie is null || Movie.InputFrames.Count == 0) {
			return;
		}

		SelectedFrameIndices = Enumerable.Range(0, Movie.InputFrames.Count).ToList();
		SelectedFrameIndex = 0;
		StatusMessage = $"Selected all {SelectedFrameIndices.Count} frames";
	}

	/// <summary>
	/// Clears the current frame selection.
	/// </summary>
	public void SelectNoFrames() {
		SelectedFrameIndices = new();
		SelectedFrameIndex = -1;
		StatusMessage = "Selection cleared";
	}

	/// <summary>
	/// Selects a contiguous frame range inclusive.
	/// </summary>
	public void SelectFrameRange(int fromFrame, int toFrame) {
		if (Movie is null || Movie.InputFrames.Count == 0) {
			return;
		}

		int start = Math.Clamp(Math.Min(fromFrame, toFrame), 0, Movie.InputFrames.Count - 1);
		int end = Math.Clamp(Math.Max(fromFrame, toFrame), 0, Movie.InputFrames.Count - 1);

		SelectedFrameIndices = Enumerable.Range(start, end - start + 1).ToList();
		SelectedFrameIndex = start;
		StatusMessage = $"Selected range {start} to {end} ({SelectedFrameIndices.Count} frames)";
	}

	/// <summary>
	/// Shows the select-range dialog and applies the chosen range.
	/// </summary>
	public async System.Threading.Tasks.Task SelectRangeDialogAsync() {
		if (Movie is null || Movie.InputFrames.Count == 0 || _window is null) {
			return;
		}

		int maxFrame = Movie.InputFrames.Count - 1;
		int defaultFrom = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;
		int defaultTo = SelectedFrameIndices.Count > 0 ? SelectedFrameIndices.Max() : defaultFrom;
		var range = await Windows.TasSelectRangeDialog.ShowAsync(_window, maxFrame, defaultFrom, defaultTo);
		if (!range.HasValue) {
			return;
		}

		SelectFrameRange(range.Value.FromFrame, range.Value.ToFrame);
	}

	/// <summary>
	/// Shows a dialog for inserting N frames at the selected position.
	/// </summary>
	public async System.Threading.Tasks.Task BulkInsertDialogAsync() {
		if (Movie is null || _window is null) {
			return;
		}

		int? result = await Windows.TasInputDialog.ShowNumericAsync(
			_window,
			"Insert N Frames",
			"How many frames should be inserted?",
			1,
			1,
			100000);

		if (!result.HasValue) {
			return;
		}

		int insertAt = SelectedFrameIndex >= 0 ? SelectedFrameIndex : Movie.InputFrames.Count;
		InsertFramesAt(insertAt, result.Value);
	}

	/// <summary>
	/// Toggles a marker (comment) on the selected frame.
	/// If the frame already has a comment starting with "[M]", removes it.
	/// Otherwise, prepends "[M] " to the existing comment or sets "[M]".
	/// </summary>
	public void ToggleMarker() {
		if (Movie is null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		string? newComment;
		if (frame.Comment is not null && frame.Comment.StartsWith("[M]")) {
			// Remove marker
			string remaining = frame.Comment.Length > 3 ? frame.Comment[3..].TrimStart() : "";
			newComment = string.IsNullOrWhiteSpace(remaining) ? null : remaining;
			StatusMessage = $"Removed marker from frame {SelectedFrameIndex}";
		} else {
			// Add marker
			newComment = frame.Comment is not null ? $"[M] {frame.Comment}" : "[M]";
			StatusMessage = $"Set marker on frame {SelectedFrameIndex}";
		}
		ExecuteAction(new ModifyCommentAction(frame, SelectedFrameIndex, newComment));
	}

	/// <summary>
	/// Selects a range of frames from the anchor (first selected) to the given target index.
	/// </summary>
	public void SelectRangeTo(int targetIndex) {
		if (Movie is null || SelectedFrameIndex < 0) {
			return;
		}
		SelectFrameRange(SelectedFrameIndex, targetIndex);
	}

	/// <summary>
	/// Deletes the selected frames.
	/// </summary>
	public void DeleteFrames() {
		if (Movie is null) {
			return;
		}

		// Multi-selection: delete all selected frames
		if (SelectedFrameIndices.Count > 1) {
			var sorted = SelectedFrameIndices
				.Where(i => i >= 0 && i < Movie.InputFrames.Count)
				.Distinct()
				.OrderByDescending(i => i)
				.ToList();

			if (sorted.Count == 0) {
				return;
			}

			// Coalesce consecutive indices into contiguous ranges (sorted descending).
			// E.g. [9,8,7,3,2] → ranges [(7,3), (2,2)] — far fewer list operations.
			var actions = new List<UndoableAction>();
			int rangeEnd = sorted[0];
			int rangeStart = rangeEnd;
			for (int i = 1; i < sorted.Count; i++) {
				if (sorted[i] == rangeStart - 1) {
					rangeStart = sorted[i];
				} else {
					actions.Add(new DeleteFramesAction(Movie, rangeStart, rangeEnd - rangeStart + 1));
					rangeEnd = sorted[i];
					rangeStart = rangeEnd;
				}
			}
			actions.Add(new DeleteFramesAction(Movie, rangeStart, rangeEnd - rangeStart + 1));

			ExecuteAction(new BulkUndoableAction($"Delete {sorted.Count} frame(s)", actions));

			StatusMessage = $"Deleted {sorted.Count} frames";

			// Adjust selection
			int minDeleted = sorted[^1];
			SelectedFrameIndex = Math.Min(minDeleted, Movie.InputFrames.Count - 1);
			SelectedFrameIndices.Clear();
			return;
		}

		// Single selection fallback
		if (SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		int deleteAt = SelectedFrameIndex;
		ExecuteAction(new DeleteFramesAction(Movie, deleteAt, 1));
		StatusMessage = $"Deleted frame {deleteAt}";

		// Adjust selection
		if (SelectedFrameIndex >= Movie.InputFrames.Count) {
			SelectedFrameIndex = Movie.InputFrames.Count - 1;
		}
	}

	/// <summary>
	/// Clears the input on the selected frame(s).
	/// </summary>
	public void ClearSelectedInput() {
		if (Movie is null) {
			return;
		}

		// Multi-selection: clear all selected frames
		if (SelectedFrameIndices.Count > 1) {
			var targets = SelectedFrameIndices
				.Where(i => i >= 0 && i < Movie.InputFrames.Count)
				.Distinct()
				.OrderBy(i => i)
				.ToList();

			if (targets.Count == 0) {
				return;
			}

			var actions = new List<UndoableAction>(targets.Count);
			foreach (int idx in targets) {
				actions.Add(new ClearInputAction(Movie.InputFrames[idx], idx));
			}

			ExecuteAction(new BulkUndoableAction($"Clear input on {targets.Count} frame(s)", actions));
			int cleared = targets.Count;
			if (cleared > 0) {
				StatusMessage = $"Cleared input on {cleared} frames";
			}
			return;
		}

		// Single selection fallback
		if (SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var singleFrame = Movie.InputFrames[SelectedFrameIndex];
		ExecuteAction(new ClearInputAction(singleFrame, SelectedFrameIndex));
		StatusMessage = $"Cleared input on frame {SelectedFrameIndex}";
	}

	/// <summary>
	/// Sets or clears a specific button for the selected frames.
	/// </summary>
	public void SetButtonOnSelectedFrames(int port, string button, bool state) {
		if (Movie is null || string.IsNullOrWhiteSpace(button)) {
			return;
		}

		List<int> targets = BuildSelectionIndices();
		if (targets.Count == 0) {
			return;
		}

		if (!TryResolveButtonId(button, out string buttonId)) {
			StatusMessage = $"Unknown button: {button}";
			return;
		}

		PaintButton(targets, port, buttonId, state);
		StatusMessage = $"{(state ? "Set" : "Cleared")} {buttonId} on {targets.Count} frame(s)";
	}

	/// <summary>
	/// Repeats the initial N-frame input pattern across the selected range.
	/// </summary>
	public void PatternFillSelectedRange(int intervalFrames, int port = 0) {
		if (Movie is null || intervalFrames <= 0) {
			return;
		}

		List<int> selected = BuildSelectionIndices();
		if (selected.Count == 0) {
			return;
		}

		int start = selected.Min();
		int end = selected.Max();
		if (start < 0 || end >= Movie.InputFrames.Count) {
			return;
		}

		if (port < 0 || port >= Movie.InputFrames[start].Controllers.Length) {
			StatusMessage = $"Invalid port {port} for pattern fill";
			return;
		}

		int patternLength = Math.Min(intervalFrames, end - start + 1);
		var pattern = new List<ControllerInput>(patternLength);
		for (int i = 0; i < patternLength; i++) {
			pattern.Add(CloneControllerInput(Movie.InputFrames[start + i].Controllers[port]));
		}

		var actions = new List<UndoableAction>(end - start + 1);
		for (int frame = start; frame <= end; frame++) {
			ControllerInput source = CloneControllerInput(pattern[(frame - start) % patternLength]);
			actions.Add(new ModifyInputAction(Movie.InputFrames[frame], frame, port, source));
		}

		ExecuteAction(new BulkUndoableAction($"Pattern fill {end - start + 1} frame(s)", actions));
		SelectedFrameIndex = start;
		SelectedFrameIndices = Enumerable.Range(start, end - start + 1).ToList();
		StatusMessage = $"Pattern fill applied from frame {start} to {end} with interval {patternLength}";
	}

	/// <summary>
	/// Shows a dialog for choosing a button to set or clear on the selected frames.
	/// </summary>
	public async System.Threading.Tasks.Task BulkSetButtonDialogAsync(bool state) {
		if (_window is null) {
			return;
		}

		string defaultButton = ControllerButtons.FirstOrDefault()?.ButtonId ?? "A";
		string verb = state ? "Set" : "Clear";
		string? button = await Windows.TasInputDialog.ShowTextAsync(
			_window,
			$"Bulk {verb} Button",
			$"Button to {verb.ToLowerInvariant()} across selected range:",
			defaultButton);

		if (string.IsNullOrWhiteSpace(button)) {
			return;
		}

		SetButtonOnSelectedFrames(0, button, state);
	}

	/// <summary>
	/// Shows a dialog for pattern-fill interval and applies it to the selected range.
	/// </summary>
	public async System.Threading.Tasks.Task PatternFillDialogAsync() {
		if (_window is null) {
			return;
		}

		int? interval = await Windows.TasInputDialog.ShowNumericAsync(
			_window,
			"Pattern Fill",
			"Repeat input pattern every N frames:",
			2,
			1,
			100000);

		if (!interval.HasValue) {
			return;
		}

		PatternFillSelectedRange(interval.Value);
	}

	private List<int> BuildSelectionIndices() {
		if (Movie is null) {
			return new List<int>();
		}

		if (SelectedFrameIndices.Count > 0) {
			return SelectedFrameIndices
				.Where(i => i >= 0 && i < Movie.InputFrames.Count)
				.Distinct()
				.OrderBy(i => i)
				.ToList();
		}

		if (SelectedFrameIndex >= 0 && SelectedFrameIndex < Movie.InputFrames.Count) {
			return new List<int> { SelectedFrameIndex };
		}

		return new List<int>();
	}

	private bool TryResolveButtonId(string rawButton, out string buttonId) {
		buttonId = string.Empty;
		if (string.IsNullOrWhiteSpace(rawButton)) {
			return false;
		}

		string normalized = rawButton.Trim().ToUpperInvariant();
		normalized = normalized switch {
			"ST" => "START",
			"SEL" => "SELECT",
			"↑" => "UP",
			"↓" => "DOWN",
			"←" => "LEFT",
			"→" => "RIGHT",
			_ => normalized
		};

		ControllerButtonInfo? match = ControllerButtons.FirstOrDefault(btn =>
			string.Equals(btn.ButtonId, normalized, StringComparison.OrdinalIgnoreCase) ||
			string.Equals(btn.Label, normalized, StringComparison.OrdinalIgnoreCase));

		if (match is null) {
			return false;
		}

		buttonId = match.ButtonId;
		return true;
	}

	private InputFrame CreateEmptyFrame(int frameNumber) {
		if (Movie is null) {
			return new InputFrame(frameNumber);
		}

		int controllerCount = Movie.InputFrames.Count > 0 ? Movie.InputFrames[0].Controllers.Length : 1;
		var frame = new InputFrame(frameNumber) {
			Controllers = new ControllerInput[controllerCount]
		};
		for (int i = 0; i < controllerCount; i++) {
			frame.Controllers[i] = new ControllerInput();
		}
		return frame;
	}

	/// <summary>
	/// Sets a comment on the selected frame.
	/// </summary>
	public async System.Threading.Tasks.Task SetCommentAsync() {
		if (Movie is null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count || _window is null) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		string currentComment = frame.Comment ?? "";

		string? result = await Windows.TasInputDialog.ShowTextAsync(
			_window,
			"Set Frame Comment",
			$"Comment for frame {SelectedFrameIndex}:\n(Leave empty to remove comment)",
			currentComment);

		if (result is null) {
			return; // cancelled
		}

		string? newComment = string.IsNullOrWhiteSpace(result) ? null : result;
		ExecuteAction(new ModifyCommentAction(frame, SelectedFrameIndex, newComment));
		StatusMessage = newComment is not null ? $"Set comment on frame {SelectedFrameIndex}" : $"Removed comment from frame {SelectedFrameIndex}";
	}

	/// <summary>
	/// Rebuilds the marker/comment list from current movie data.
	/// Builds a temporary list first to minimize ObservableCollection change notifications.
	/// </summary>
	public void RefreshMarkerEntries() {
		if (Movie is null) {
			MarkerEntries.Clear();
			SelectedMarkerEntry = null;
			return;
		}

		var entries = new List<TasMarkerEntryViewModel>();
		for (int i = 0; i < Movie.InputFrames.Count; i++) {
			string? comment = Movie.InputFrames[i].Comment;
			if (string.IsNullOrWhiteSpace(comment)) {
				continue;
			}

			MarkerEntryType type = ClassifyMarkerEntryType(comment);
			if (!MatchesMarkerFilter(type)) {
				continue;
			}

			entries.Add(new TasMarkerEntryViewModel(i, comment, type));
		}

		int? selectedFrame = SelectedMarkerEntry?.FrameIndex;
		MarkerEntries.Clear();
		foreach (var entry in entries) {
			MarkerEntries.Add(entry);
		}

		if (selectedFrame is not null) {
			SelectedMarkerEntry = MarkerEntries.FirstOrDefault(e => e.FrameIndex == selectedFrame.Value);
		}
	}

	/// <summary>
	/// Incrementally updates the marker entries list for a single frame change.
	/// O(k) where k = marker count, instead of O(n) where n = total frames.
	/// </summary>
	internal void RefreshMarkerEntryAt(int frameIndex) {
		if (Movie is null) {
			return;
		}

		// Remove existing entry for this frame (if any)
		for (int i = MarkerEntries.Count - 1; i >= 0; i--) {
			if (MarkerEntries[i].FrameIndex == frameIndex) {
				MarkerEntries.RemoveAt(i);
				break; // At most one entry per frame
			}
		}

		// Check if the frame now has a comment that passes the filter
		if (frameIndex >= 0 && frameIndex < Movie.InputFrames.Count) {
			string? comment = Movie.InputFrames[frameIndex].Comment;
			if (!string.IsNullOrWhiteSpace(comment)) {
				MarkerEntryType type = ClassifyMarkerEntryType(comment);
				if (MatchesMarkerFilter(type)) {
					var entry = new TasMarkerEntryViewModel(frameIndex, comment, type);
					// Insert in sorted position (entries sorted by FrameIndex)
					int insertIndex = 0;
					for (int i = 0; i < MarkerEntries.Count; i++) {
						if (MarkerEntries[i].FrameIndex > frameIndex) {
							break;
						}
						insertIndex = i + 1;
					}
					MarkerEntries.Insert(insertIndex, entry);
				}
			}
		}
	}

	/// <summary>
	/// Navigates to the frame of the selected marker entry.
	/// </summary>
	public void NavigateToMarkerEntry(TasMarkerEntryViewModel? entry) {
		if (entry is null || Movie is null) {
			return;
		}

		if (entry.FrameIndex < 0 || entry.FrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		SelectedFrameIndex = entry.FrameIndex;
		SelectedFrameIndices = new List<int> { entry.FrameIndex };
		StatusMessage = $"Navigated to frame {entry.FrameIndex}";
	}

	/// <summary>
	/// Adds or updates a marker/comment entry at the requested frame.
	/// </summary>
	internal bool UpsertMarkerEntry(int frameIndex, string? text) {
		if (Movie is null || frameIndex < 0 || frameIndex >= Movie.InputFrames.Count) {
			return false;
		}

		string? newComment = string.IsNullOrWhiteSpace(text) ? null : text.Trim();
		ExecuteAction(new ModifyCommentAction(Movie.InputFrames[frameIndex], frameIndex, newComment));
		return true;
	}

	/// <summary>
	/// Deletes the currently selected marker/comment entry.
	/// </summary>
	public void DeleteSelectedMarkerEntry() {
		if (SelectedMarkerEntry is null) {
			return;
		}

		int frameIndex = SelectedMarkerEntry.FrameIndex;

		if (UpsertMarkerEntry(frameIndex, null)) {
			StatusMessage = $"Removed marker/comment from frame {frameIndex}";
		}
	}

	/// <summary>
	/// Exports currently visible marker entries to a text file.
	/// </summary>
	internal bool ExportMarkerEntries(string path) {
		if (string.IsNullOrWhiteSpace(path)) {
			return false;
		}

		var lines = new List<string>(MarkerEntries.Count + 1) {
			"Frame\tType\tComment"
		};

		foreach (var entry in MarkerEntries.OrderBy(e => e.FrameIndex)) {
			lines.Add($"{entry.FrameIndex}\t{entry.Type}\t{entry.Comment}");
		}

		File.WriteAllLines(path, lines);
		StatusMessage = $"Exported {MarkerEntries.Count} marker/comment entries";
		return true;
	}

	/// <summary>
	/// Adds a marker/comment entry from panel UI.
	/// </summary>
	public async System.Threading.Tasks.Task AddMarkerEntryAsync() {
		if (Movie is null || _window is null) {
			return;
		}

		int maxFrame = Math.Max(0, Movie.InputFrames.Count - 1);
		int defaultFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;
		int? frameIndex = await Windows.TasInputDialog.ShowNumericAsync(_window, "Add Marker/Comment", $"Frame index (0 - {maxFrame:N0}):", defaultFrame, 0, maxFrame);
		if (!frameIndex.HasValue) {
			return;
		}

		string? text = await Windows.TasInputDialog.ShowTextAsync(_window, "Add Marker/Comment", "Comment text ([M] prefix marks marker, TODO keyword marks TODO):", "[M]");
		if (text is null) {
			return;
		}

		if (UpsertMarkerEntry(frameIndex.Value, text)) {
			StatusMessage = $"Added marker/comment on frame {frameIndex.Value}";
		}
	}

	/// <summary>
	/// Edits the currently selected marker/comment entry.
	/// </summary>
	public async System.Threading.Tasks.Task EditSelectedMarkerEntryAsync() {
		if (SelectedMarkerEntry is null || _window is null) {
			return;
		}

		string? text = await Windows.TasInputDialog.ShowTextAsync(
			_window,
			"Edit Marker/Comment",
			$"Comment for frame {SelectedMarkerEntry.FrameIndex}:\n(Leave empty to remove)",
			SelectedMarkerEntry.Comment);

		if (text is null) {
			return;
		}

		if (UpsertMarkerEntry(SelectedMarkerEntry.FrameIndex, text)) {
			StatusMessage = string.IsNullOrWhiteSpace(text)
				? $"Removed marker/comment from frame {SelectedMarkerEntry.FrameIndex}"
				: $"Updated marker/comment on frame {SelectedMarkerEntry.FrameIndex}";
		}
	}

	/// <summary>
	/// Exports marker/comment entries via save-file dialog.
	/// </summary>
	public async System.Threading.Tasks.Task ExportMarkerEntriesAsync() {
		if (_window is null) {
			return;
		}

		string? path = await FileDialogHelper.SaveFile(
			ConfigManager.MovieFolder,
			"tas-markers.txt",
			_window,
			"txt");

		if (string.IsNullOrWhiteSpace(path)) {
			return;
		}

		try {
			ExportMarkerEntries(path);
		} catch (Exception ex) {
			StatusMessage = $"Error exporting markers: {ex.Message}";
		}
	}

	private bool MatchesMarkerFilter(MarkerEntryType type) {
		return MarkerEntryFilter switch {
			MarkerEntryFilterType.All => true,
			MarkerEntryFilterType.Comment => type == MarkerEntryType.Comment,
			MarkerEntryFilterType.Marker => type == MarkerEntryType.Marker,
			MarkerEntryFilterType.Todo => type == MarkerEntryType.Todo,
			_ => true
		};
	}

	internal static MarkerEntryType ClassifyMarkerEntryType(string comment) {
		if (comment.StartsWith("[M]", StringComparison.OrdinalIgnoreCase)) {
			return MarkerEntryType.Marker;
		}

		if (comment.Contains("TODO", StringComparison.OrdinalIgnoreCase)) {
			return MarkerEntryType.Todo;
		}

		return MarkerEntryType.Comment;
	}

	/// <summary>
	/// Goes to a specific frame via input dialog.
	/// </summary>
	public async System.Threading.Tasks.Task GoToFrameAsync() {
		if (Movie is null || Movie.InputFrames.Count == 0 || _window is null) {
			return;
		}

		int maxFrame = Movie.InputFrames.Count - 1;
		int defaultFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;

		int? result = await Windows.TasInputDialog.ShowNumericAsync(
			_window,
			"Go To Frame",
			$"Enter frame number (0 - {maxFrame:N0}):",
			defaultFrame,
			0,
			maxFrame);

		if (result.HasValue) {
			SelectedFrameIndex = result.Value;
			StatusMessage = $"Jumped to frame {result.Value}";
		}
	}

	/// <summary>
	/// Toggles the search bar visibility.
	/// </summary>
	public void ToggleSearch() {
		IsSearchVisible = !IsSearchVisible;
		if (!IsSearchVisible) {
			SearchResults = new();
			SearchResultIndex = -1;
		}
	}

	/// <summary>
	/// Executes a search with the current search text and mode.
	/// </summary>
	public void ExecuteSearch() {
		bool needsText = SearchMode != FrameSearchMode.LagFrame && SearchMode != FrameSearchMode.Marker;
		if (Movie is null || (needsText && string.IsNullOrWhiteSpace(SearchText))) {
			SearchResults = new();
			SearchResultIndex = -1;
			return;
		}

		var results = new List<int>();
		for (int i = 0; i < Movie.InputFrames.Count; i++) {
			var frame = Movie.InputFrames[i];
			bool match = SearchMode switch {
				FrameSearchMode.Comment => frame.Comment is not null && frame.Comment.Contains(SearchText, StringComparison.OrdinalIgnoreCase),
				FrameSearchMode.ButtonPressed => frame.Controllers.Length > SelectedEditPort && frame.Controllers[SelectedEditPort].GetButton(SearchText),
				FrameSearchMode.LagFrame => frame.IsLagFrame,
				FrameSearchMode.Marker => frame.Comment is not null && frame.Comment.StartsWith("[M]"),
				_ => false,
			};
			if (match) {
				results.Add(i);
			}
		}

		SearchResults = results;
		SearchResultIndex = results.Count > 0 ? 0 : -1;

		if (results.Count > 0) {
			SelectedFrameIndex = results[0];
			StatusMessage = $"Found {results.Count} matches";
		} else {
			StatusMessage = "No matches found";
		}
	}

	/// <summary>
	/// Navigates to the next search result.
	/// </summary>
	public void SearchNext() {
		if (SearchResults.Count == 0) {
			return;
		}

		SearchResultIndex = (SearchResultIndex + 1) % SearchResults.Count;
		SelectedFrameIndex = SearchResults[SearchResultIndex];
	}

	/// <summary>
	/// Navigates to the previous search result.
	/// </summary>
	public void SearchPrevious() {
		if (SearchResults.Count == 0) {
			return;
		}

		SearchResultIndex = (SearchResultIndex - 1 + SearchResults.Count) % SearchResults.Count;
		SelectedFrameIndex = SearchResults[SearchResultIndex];
	}

	/// <summary>
	/// Toggles a button on the selected frame.
	/// </summary>
	public void ToggleButton(int port, string button) {
		if (Movie is null || SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[SelectedFrameIndex];
		if (port < 0 || port >= frame.Controllers.Length) {
			return;
		}

		var newInput = CloneControllerInput(frame.Controllers[port]);
		newInput.SetButton(button, !newInput.GetButton(button));

		ExecuteAction(new ModifyInputAction(frame, SelectedFrameIndex, port, newInput));
	}

	/// <summary>
	/// Toggles the Atari 2600 Select console switch on the selected frame(s). Undoable.
	/// </summary>
	public void ToggleConsoleSwitchSelect() {
		ToggleConsoleSwitchFlag(FrameCommand.Atari2600Select);
	}

	/// <summary>
	/// Toggles the Atari 2600 Reset console switch on the selected frame(s). Undoable.
	/// </summary>
	public void ToggleConsoleSwitchReset() {
		ToggleConsoleSwitchFlag(FrameCommand.Atari2600Reset);
	}

	private void ToggleConsoleSwitchFlag(FrameCommand flag) {
		if (Movie is null) {
			return;
		}

		// Collect target frames from multi-selection or single selection
		List<InputFrame> targets = new();
		List<int> targetIndices = new();
		if (SelectedFrameIndices.Count > 1) {
			foreach (int idx in SelectedFrameIndices) {
				if (idx >= 0 && idx < Movie.InputFrames.Count) {
					targets.Add(Movie.InputFrames[idx]);
					targetIndices.Add(idx);
				}
			}
		} else if (SelectedFrameIndex >= 0 && SelectedFrameIndex < Movie.InputFrames.Count) {
			targets.Add(Movie.InputFrames[SelectedFrameIndex]);
			targetIndices.Add(SelectedFrameIndex);
		}

		if (targets.Count == 0) {
			return;
		}

		// Toggle based on first frame's current state
		bool currentState = targets[0].Command.HasFlag(flag);
		ExecuteAction(new ModifyCommandAction(targets, targetIndices, flag, !currentState));
		RefreshSelectedFramePreview();
	}

	/// <summary>
	/// Toggles a button at a specific frame (used by piano roll). Undoable.
	/// </summary>
	public void ToggleButtonAtFrame(int frameIndex, int port, string button, bool newState) {
		if (Movie is null || frameIndex < 0 || frameIndex >= Movie.InputFrames.Count) {
			return;
		}

		var frame = Movie.InputFrames[frameIndex];
		if (port < 0 || port >= frame.Controllers.Length) {
			return;
		}

		var newInput = CloneControllerInput(frame.Controllers[port]);
		newInput.SetButton(button, newState);

		ExecuteAction(new ModifyInputAction(frame, frameIndex, port, newInput));
	}

	/// <summary>
	/// Batch-paints a button across multiple frames. Undoable as a single action.
	/// </summary>
	public void PaintButton(IReadOnlyList<int> frameIndices, int port, string button, bool state) {
		if (Movie is null || frameIndices.Count == 0) {
			return;
		}

		var validFrames = new List<int>(frameIndices.Count);
		var oldInputs = new List<ControllerInput>(frameIndices.Count);

		foreach (int idx in frameIndices) {
			if (idx < 0 || idx >= Movie.InputFrames.Count) {
				continue;
			}

			var frame = Movie.InputFrames[idx];
			if (port < 0 || port >= frame.Controllers.Length) {
				continue;
			}

			validFrames.Add(idx);
			oldInputs.Add(CloneControllerInput(frame.Controllers[port]));
		}

		if (validFrames.Count == 0) {
			return;
		}

		ExecuteAction(new PaintInputAction(Movie, validFrames, port, button, state, oldInputs));
	}

	/// <summary>
	/// Refreshes the frame list display.
	/// </summary>
	public void RefreshFrames() {
		UpdateFrames();
	}

	private void UpdateFrames() {
		if (Movie is null) {
			Frames.Clear();

			if (SelectedFrameIndex != -1) {
				SelectedFrameIndex = -1;
			}

			if (PlaybackFrame != 0) {
				PlaybackFrame = 0;
			}

			RefreshMarkerEntries();
			RefreshSelectedFramePreview();
			return;
		}

		Frames.ReplaceAll(GenerateFrameViewModels());

		if (Movie.InputFrames.Count == 0) {
			if (SelectedFrameIndex != -1) {
				SelectedFrameIndex = -1;
			}

			if (PlaybackFrame != 0) {
				PlaybackFrame = 0;
			}
		} else {
			int maxFrame = Movie.InputFrames.Count - 1;

			if (SelectedFrameIndex > maxFrame) {
				SelectedFrameIndex = maxFrame;
			}

			if (SelectedFrameIndex < -1) {
				SelectedFrameIndex = -1;
			}

			if (PlaybackFrame > maxFrame) {
				PlaybackFrame = maxFrame;
			}

			if (PlaybackFrame < 0) {
				PlaybackFrame = 0;
			}
		}

		RefreshMarkerEntries();
		RefreshSelectedFramePreview();
	}

	/// <summary>
	/// Generates frame view models for all input frames. Used by <see cref="UpdateFrames"/> for batch replacement.
	/// </summary>
	private IEnumerable<TasFrameViewModel> GenerateFrameViewModels() {
		for (int i = 0; i < Movie!.InputFrames.Count; i++) {
			yield return new TasFrameViewModel(Movie.InputFrames[i], i, IsGreenzoneEnabled && i >= GreenzoneStart);
		}
	}

	/// <summary>
	/// Refreshes a single frame's display without full rebuild. O(1) instead of O(n).
	/// </summary>
	private void RefreshFrameAt(int index) {
		if (index >= 0 && index < Frames.Count) {
			Frames[index].RefreshFromFrame();
			if (index == SelectedFrameIndex) {
				RefreshSelectedFramePreview();
			}
		}
	}

	/// <summary>
	/// Refreshes the frame at the current playback position. O(1).
	/// Used after state load where movie data hasn't changed, only the emulator position.
	/// </summary>
	public void RefreshFrameAtPlayback() {
		RefreshFrameAt(PlaybackFrame);
	}

	internal static ControllerInput CloneControllerInput(ControllerInput src) => src.Clone();

	/// <summary>
	/// Inserts a frame ViewModel at the specified index and updates subsequent frame numbers. O(n) for renumbering but avoids full collection rebuild.
	/// </summary>
	private void InsertFrameViewModel(int index) {
		if (Movie is null || index < 0 || index > Movie.InputFrames.Count) {
			return;
		}

		// Insert the new ViewModel
		var vm = new TasFrameViewModel(Movie.InputFrames[index], index, IsGreenzoneEnabled && index >= GreenzoneStart);
		Frames.Insert(index, vm);

		// Update frame numbers for subsequent items (they shifted by 1)
		for (int i = index + 1; i < Frames.Count; i++) {
			Frames[i].FrameNumber = i + 1;
		}
	}

	/// <summary>
	/// Removes a frame ViewModel at the specified index and updates subsequent frame numbers. O(n) for renumbering but avoids full collection rebuild.
	/// </summary>
	private void RemoveFrameViewModel(int index) {
		if (index < 0 || index >= Frames.Count) {
			return;
		}

		Frames.RemoveAt(index);

		// Update frame numbers for subsequent items (they shifted by -1)
		for (int i = index; i < Frames.Count; i++) {
			Frames[i].FrameNumber = i + 1;
		}
	}

	/// <summary>
	/// Inserts multiple frame ViewModels at the specified index. Renumbers once at the end.
	/// O(count + tail) — much cheaper than full rebuild for large movies.
	/// </summary>
	private void InsertFrameViewModels(int startIndex, int count) {
		if (Movie is null || count <= 0 || startIndex < 0 || startIndex > Movie.InputFrames.Count) {
			return;
		}

		var viewModels = new List<TasFrameViewModel>(count);
		for (int j = 0; j < count; j++) {
			int idx = startIndex + j;
			viewModels.Add(new TasFrameViewModel(Movie.InputFrames[idx], idx, IsGreenzoneEnabled && idx >= GreenzoneStart));
		}

		Frames.InsertRange(startIndex, viewModels);

		// Renumber everything after the inserted block
		for (int i = startIndex + count; i < Frames.Count; i++) {
			Frames[i].FrameNumber = i + 1;
		}
	}

	/// <summary>
	/// Removes a range of frame ViewModels at the specified index. O(count + tail) for renumbering.
	/// </summary>
	private void RemoveFrameViewModels(int startIndex, int count) {
		if (startIndex < 0 || count <= 0 || startIndex + count > Frames.Count) {
			return;
		}

		Frames.RemoveRange(startIndex, count);

		// Renumber everything after the removed block
		for (int i = startIndex; i < Frames.Count; i++) {
			Frames[i].FrameNumber = i + 1;
		}
	}

	/// <summary>
	/// Removes frame ViewModels from startIndex to the end. O(count) removals, no renumbering needed.
	/// </summary>
	private void TruncateFrameViewModels(int startIndex) {
		if (startIndex < 0 || startIndex >= Frames.Count) {
			return;
		}

		Frames.RemoveRange(startIndex, Frames.Count - startIndex);
	}

	/// <summary>
	/// Syncs the Frames collection to append any new frames from the movie. O(k) where k = new frames, instead of O(n) full rebuild.
	/// Used during recording to efficiently update the UI as frames are appended.
	/// </summary>
	private void SyncNewFrames() {
		if (Movie is null) {
			return;
		}

		int start = Frames.Count;
		int newCount = Movie.InputFrames.Count - start;
		if (newCount <= 0) {
			return;
		}

		var viewModels = new List<TasFrameViewModel>(newCount);
		for (int i = start; i < Movie.InputFrames.Count; i++) {
			viewModels.Add(new TasFrameViewModel(Movie.InputFrames[i], i, IsGreenzoneEnabled && i >= GreenzoneStart));
		}

		Frames.AddRange(viewModels);
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

		int earliest = GetEarliestAffectedFrame(action);
		if (earliest >= 0) {
			Greenzone.InvalidateFrom(earliest);
			GreenzoneStart = earliest;
		}

		UpdateUndoRedoState();
		ApplyIncrementalUpdate(action, isUndo: true);
		RefreshSelectedFramePreview();
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

		int earliest = GetEarliestAffectedFrame(action);
		if (earliest >= 0) {
			Greenzone.InvalidateFrom(earliest);
			GreenzoneStart = earliest;
		}

		UpdateUndoRedoState();
		ApplyIncrementalUpdate(action, isUndo: false);
		RefreshSelectedFramePreview();
		HasUnsavedChanges = true;
		StatusMessage = $"Redid: {action.Description}";
	}

	/// <summary>
	/// Dispatches incremental ObservableCollection updates based on the action type,
	/// avoiding full UpdateFrames() rebuild for known action types.
	/// </summary>
	private void ApplyIncrementalUpdate(UndoableAction action, bool isUndo) {
		switch (action) {
			case BulkUndoableAction bulk:
				if (isUndo) {
					for (int i = bulk.Actions.Count - 1; i >= 0; i--) {
						ApplyIncrementalUpdate(bulk.Actions[i], true);
					}
				} else {
					foreach (UndoableAction child in bulk.Actions) {
						ApplyIncrementalUpdate(child, false);
					}
				}
				break;
			case InsertFramesAction insert:
				if (isUndo) {
					// Undo of insert = frames were removed
					RemoveFrameViewModels(insert.Index, insert.Count);
				} else {
					// Redo of insert = frames were inserted
					InsertFrameViewModels(insert.Index, insert.Count);
				}
				break;
			case DeleteFramesAction delete:
				if (isUndo) {
					// Undo of delete = frames were inserted back
					InsertFrameViewModels(delete.Index, delete.Count);
				} else {
					// Redo of delete = frames were removed
					RemoveFrameViewModels(delete.Index, delete.Count);
				}
				break;
			case ModifyInputAction modify:
				// No structural change — O(1) refresh using stored index
				RefreshFrameAt(modify.FrameIndex);
				break;
			case ClearInputAction clear:
				// No structural change — O(1) refresh using stored index
				RefreshFrameAt(clear.FrameIndex);
				break;
			case PaintInputAction paint:
				// Refresh only the painted frames — O(k) for k painted frames
				foreach (int idx in paint.FrameIndices) {
					RefreshFrameAt(idx);
				}
				break;
			case ModifyCommentAction comment:
				// Comment change — O(1) frame refresh + incremental marker update
				RefreshFrameAt(comment.FrameIndex);
				RefreshMarkerEntryAt(comment.FrameIndex);
				break;
			case ModifyCommandAction command:
				// Command flag change — O(k) refresh for affected frames
				foreach (int idx in command.FrameIndices) {
					RefreshFrameAt(idx);
				}
				break;
			default:
				// Unknown action type — fall back to full rebuild
				UpdateFrames();
				break;
		}
	}

	/// <summary>
	/// Executes an action, adds it to the undo stack, and applies incremental view model updates.
	/// </summary>
	internal void ExecuteAction(UndoableAction action) {
		action.Execute();
		_undoStack.Push(action);
		_redoStack.Clear(); // Clear redo stack on new action

		// Invalidate greenzone savestates from the earliest affected frame onwards.
		// After an edit, cached savestates past that point assumed the old inputs
		// and are no longer valid.
		int earliest = GetEarliestAffectedFrame(action);
		if (earliest >= 0) {
			Greenzone.InvalidateFrom(earliest);
			GreenzoneStart = earliest;
		}

		// Cap undo history to prevent unbounded memory growth.
		// Uses hysteresis: only trim when exceeding max, trim to a lower target
		// so the O(n) rebuild doesn't fire on every single action past the cap.
		if (_undoStack.Count > MaxUndoHistory) {
			var items = _undoStack.ToArray();
			_undoStack.Clear();
			// items[0] is the top (most recent); keep the first UndoTrimTarget items
			for (int i = Math.Min(items.Length, UndoTrimTarget) - 1; i >= 0; i--) {
				_undoStack.Push(items[i]);
			}
		}

		ApplyIncrementalUpdate(action, isUndo: false);
		UpdateUndoRedoState();
		HasUnsavedChanges = true;
	}

	/// <summary>
	/// Returns the earliest frame index affected by an action, for greenzone invalidation.
	/// Returns -1 if the affected frame cannot be determined (falls back to no invalidation).
	/// </summary>
	internal static int GetEarliestAffectedFrame(UndoableAction action) => action switch {
		InsertFramesAction insert => insert.Index,
		DeleteFramesAction delete => delete.Index,
		ModifyInputAction modify => modify.FrameIndex,
		ClearInputAction clear => clear.FrameIndex,
		PaintInputAction paint => paint.FrameIndices.Count > 0 ? paint.FrameIndices.Min() : -1,
		ModifyCommentAction comment => comment.FrameIndex,
		ModifyCommandAction command => command.FrameIndices.Count > 0 ? command.FrameIndices.Min() : -1,
		BulkUndoableAction bulk => bulk.Actions.Count > 0
			? bulk.Actions.Min(a => GetEarliestAffectedFrame(a))
			: -1,
		_ => 0, // Unknown action type — conservatively invalidate everything
	};

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
		if (Movie is null || SelectedFrameIndex < 0) {
			return;
		}

		Copy();
		DeleteFrames();
	}

	/// <summary>
	/// Copies the selected frame(s) to clipboard.
	/// </summary>
	public void Copy() {
		if (Movie is null) {
			return;
		}

		// Multi-selection: copy all selected frames
		if (SelectedFrameIndices.Count > 1) {
			var sorted = SelectedFrameIndices
				.Where(i => i >= 0 && i < Movie.InputFrames.Count)
				.OrderBy(i => i)
				.ToList();

			if (sorted.Count == 0) return;

			_clipboard = sorted.Select(i => Movie.InputFrames[i].Clone()).ToList();
			_clipboardStartIndex = sorted[0];
			HasClipboard = true;
			StatusMessage = $"Copied {_clipboard.Count} frame(s)";
			return;
		}

		// Single selection fallback
		if (SelectedFrameIndex < 0 || SelectedFrameIndex >= Movie.InputFrames.Count) {
			return;
		}

		// Clone the selected frame
		var frame = Movie.InputFrames[SelectedFrameIndex];
		_clipboard = new List<InputFrame> { frame.Clone() };
		_clipboardStartIndex = SelectedFrameIndex;
		HasClipboard = true;

		StatusMessage = $"Copied frame {SelectedFrameIndex + 1}";
	}

	/// <summary>
	/// Pastes clipboard content at the current position.
	/// </summary>
	public void Paste() {
		if (Movie is null || _clipboard is null || _clipboard.Count == 0) {
			return;
		}

		int insertAt = Math.Max(0, SelectedFrameIndex >= 0 ? SelectedFrameIndex + 1 : Movie.InputFrames.Count);
		var clonedFrames = new List<InputFrame>(_clipboard.Count);
		foreach (var f in _clipboard) {
			clonedFrames.Add(f.Clone());
		}

		ExecuteAction(new InsertFramesAction(Movie, insertAt, clonedFrames));
		SelectedFrameIndex = insertAt;
		SelectedFrameIndices = Enumerable.Range(insertAt, clonedFrames.Count).ToList();

		StatusMessage = $"Pasted {clonedFrames.Count} frame(s) at {insertAt + 1}";
	}

	#endregion

	#region Playback Controls

	/// <summary>
	/// Computes the UI refresh interval for playback updates based on effective frame rate.
	/// 60 fps -> every frame, 120 fps -> every 2 frames, 240 fps -> every 4 frames.
	/// </summary>
	internal static int ComputePlaybackUiUpdateInterval(double playbackSpeed) {
		const double baseFps = 60.0;
		double effectiveFps = Math.Max(baseFps, baseFps * Math.Max(playbackSpeed, 0.0));
		return Math.Max(1, (int)Math.Round(effectiveFps / baseFps, MidpointRounding.AwayFromZero));
	}

	/// <summary>
	/// Determines whether playback UI should refresh for the provided next frame.
	/// Always refreshes on the final frame to avoid visual frame skipping.
	/// </summary>
	internal static bool ShouldRefreshPlaybackUi(int nextFrame, int totalFrames, double playbackSpeed) {
		if (totalFrames <= 0) {
			return true;
		}

		if (nextFrame >= totalFrames - 1) {
			return true;
		}

		int interval = ComputePlaybackUiUpdateInterval(playbackSpeed);
		if (interval <= 1) {
			return true;
		}

		return nextFrame % interval == 0;
	}

	private static bool IsEmulatorRunningSafe() {
		try {
			return EmuApi.IsRunning();
		} catch (DllNotFoundException) {
			return false;
		} catch (EntryPointNotFoundException) {
			return false;
		}
	}

	private static bool IsEmulatorPausedSafe() {
		try {
			return EmuApi.IsPaused();
		} catch (DllNotFoundException) {
			return false;
		} catch (EntryPointNotFoundException) {
			return false;
		}
	}

	private static void ResumeEmulatorSafe() {
		try {
			EmuApi.Resume();
		} catch (DllNotFoundException) {
			// Managed tests can execute without native runtime loaded.
		} catch (EntryPointNotFoundException) {
			// Older native runtimes may be missing newer exports.
		}
	}

	private static void PauseEmulatorSafe() {
		try {
			EmuApi.Pause();
		} catch (DllNotFoundException) {
			// Managed tests can execute without native runtime loaded.
		} catch (EntryPointNotFoundException) {
			// Older native runtimes may be missing newer exports.
		}
	}

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
		if (Movie is null) {
			return;
		}

		IsPlaying = true;
		PlaybackFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;
		StatusMessage = $"Playing at {PlaybackSpeed:F2}x speed";

		// Resume emulation — PpuFrameDone handler in TasEditorWindow feeds
		// movie input each frame and advances PlaybackFrame automatically
		if (IsEmulatorRunningSafe() && IsEmulatorPausedSafe()) {
			ResumeEmulatorSafe();
		}
	}

	/// <summary>
	/// Stops playback.
	/// </summary>
	public void StopPlayback() {
		IsPlaying = false;
		StatusMessage = "Playback stopped";

		// Pause emulation when stopping playback
		if (IsEmulatorRunningSafe() && !IsEmulatorPausedSafe()) {
			PauseEmulatorSafe();
		}
	}

	/// <summary>
	/// Advances playback by one frame.
	/// </summary>
	public void FrameAdvance() {
		if (Movie is null) {
			return;
		}

		if (PlaybackFrame < Movie.InputFrames.Count - 1) {
			PlaybackFrame++;
			SelectedFrameIndex = PlaybackFrame;
			StatusMessage = $"Frame {PlaybackFrame + 1} / {Movie.InputFrames.Count}";

			// Execute single frame in emulator if running
			if (IsEmulatorRunningSafe()) {
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
		if (Movie is null) {
			return;
		}

		if (PlaybackFrame > 0) {
			PlaybackFrame--;
			SelectedFrameIndex = PlaybackFrame;
			StatusMessage = $"Frame {PlaybackFrame + 1} / {Movie.InputFrames.Count}";

			// Use rewind shortcut if available
			if (IsEmulatorRunningSafe()) {
				// Try to load a savestate for frame-accurate rewind
				// For now, use the rewind feature
				EmuApi.ExecuteShortcut(new ExecuteShortcutParams {
					Shortcut = EmulatorShortcut.Rewind
				});
			}
		}
	}

	#region Playback Interrupt Detection

	/// <summary>
	/// Whether playback interrupt detection is enabled.
	/// When enabled, user input during movie playback triggers an interrupt dialog.
	/// </summary>
	[Reactive] public partial bool InterruptOnInput { get; set; } = true;

	/// <summary>
	/// Checks if user input differs from the movie's expected input at the current frame.
	/// If so, show the interrupt dialog.
	/// </summary>
	/// <param name="frame">Current playback frame.</param>
	/// <param name="userInput">Actual controller input from the user.</param>
	/// <returns>True if playback should continue, false if interrupted.</returns>
	public async System.Threading.Tasks.Task<bool> CheckInputMismatchAsync(int frame, ControllerInput[] userInput) {
		if (Movie is null || !IsPlaying || IsRecording || !InterruptOnInput) {
			return true; // No interrupt needed
		}

		if (frame < 0 || frame >= Movie.InputFrames.Count) {
			return true;
		}

		// Get expected input from movie
		InputFrame expectedFrame = Movie.InputFrames[frame];

		// Check if any controller has input that differs from expected
		bool hasInputMismatch = false;
		for (int i = 0; i < userInput.Length && i < expectedFrame.Controllers.Length; i++) {
			// Only trigger on actual button press (not releases)
			if (userInput[i].ButtonBits != 0 && userInput[i].ButtonBits != expectedFrame.Controllers[i].ButtonBits) {
				hasInputMismatch = true;
				break;
			}
		}

		if (!hasInputMismatch) {
			return true; // Continue playback
		}

		// Pause emulation immediately
		if (IsEmulatorRunningSafe() && !IsEmulatorPausedSafe()) {
			PauseEmulatorSafe();
		}

		IsPlaying = false; // Pause playback state

		// Show interrupt dialog
		PlaybackInterruptAction action = await PlaybackInterruptDialog.ShowDialog(
			_window,
			frame,
			Movie.InputFrames.Count);

		await HandleInterruptActionAsync(action, frame);

		return action == PlaybackInterruptAction.Continue;
	}

	/// <summary>
	/// Handles the user's choice from the interrupt dialog.
	/// </summary>
	internal System.Threading.Tasks.Task HandleInterruptActionAsync(PlaybackInterruptAction action, int frame) {
		switch (action) {
			case PlaybackInterruptAction.Fork:
				// Create a branch from current state and start recording
				string branchName = $"Fork at frame {frame + 1}";
				var branch = Recorder.CreateBranch(branchName);
				Branches.Add(branch);
				StatusMessage = $"Created branch: {branchName}";

				// Truncate movie to current frame and start recording
				if (Movie != null && frame < Movie.InputFrames.Count) {
					// Remove frames after current position via undo system
					int removeCount = Movie.InputFrames.Count - frame;
					if (removeCount > 0) {
						ExecuteAction(new DeleteFramesAction(Movie, frame, removeCount));
					}
				}

				// Start recording in append mode from this frame (movie was truncated above)
				Recorder.Movie = Movie;
				Recorder.StartRecording(RecordingMode.Append, frame);
				IsRecording = true;

				// Resume emulation
				if (IsEmulatorRunningSafe() && IsEmulatorPausedSafe()) {
					ResumeEmulatorSafe();
				}
				break;

			case PlaybackInterruptAction.Edit:
				// Open/focus piano roll at the interrupted frame
				SelectedFrameIndex = frame;
				PlaybackFrame = frame;
				ShowPianoRoll = true;
				StatusMessage = $"Editing frame {frame + 1}";
				// Emulation stays paused for editing
				break;

			case PlaybackInterruptAction.Continue:
				// Resume playback ignoring the input mismatch
				IsPlaying = true;
				if (IsEmulatorRunningSafe() && IsEmulatorPausedSafe()) {
					ResumeEmulatorSafe();
				}
				StatusMessage = $"Continuing playback at frame {frame + 1}";
				break;
		}

		return System.Threading.Tasks.Task.CompletedTask;
	}

	#endregion

	/// <summary>
	/// Seeks to a specific frame using the greenzone, prompting for target frame.
	/// </summary>
	public async System.Threading.Tasks.Task SeekToFrameAsync() {
		if (Movie is null || Greenzone.SavestateCount == 0 || _window is null) {
			return;
		}

		int maxFrame = Movie.InputFrames.Count - 1;
		int defaultFrame = SelectedFrameIndex >= 0 ? SelectedFrameIndex : 0;

		int? result = await Windows.TasInputDialog.ShowNumericAsync(
			_window,
			"Seek To Frame",
			$"Enter target frame (0 - {maxFrame:N0}):\nSeeks using greenzone savestates.",
			defaultFrame,
			0,
			maxFrame);

		if (!result.HasValue) {
			return;
		}

		int targetFrame = result.Value;
		StatusMessage = $"Seeking to frame {targetFrame + 1:N0}...";

		int lastProgressUpdate = -1;
		int actualFrame = await Greenzone.SeekToFrameAsync(targetFrame, (current, target) => {
			// Throttle UI status updates during long seeks to reduce churn.
			if (current - lastProgressUpdate < 120) {
				return;
			}

			lastProgressUpdate = current;
			StatusMessage = $"Seeking... {current + 1:N0}/{target + 1:N0}";
		});

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
		if (IsRecording) {
			return;
		}

		if (Movie is null) {
			CreateNewMovie();
		}

		if (Movie is null) {
			return;
		}

		PopulatePortTypesFromRuntime();
		Recorder.StartRecording(RecordMode, SelectedFrameIndex);
	}

	public void CreateNewMovieFromMenu() {
		CreateNewMovie();
	}

	private void CreateNewMovie() {
		if (!EmulatorState.Instance.IsRomLoaded) {
			StatusMessage = "Load a ROM before creating a movie";
			return;
		}

		RomInfo romInfo;
		int controllerCount;
		try {
			romInfo = EmuApi.GetRomInfo();
			controllerCount = Math.Max(1, InputApi.GetControllerStates().Length);
		} catch (DllNotFoundException) {
			StatusMessage = "Cannot create movie — emulator core not available";
			return;
		}

		Movie = new MovieData() {
			SourceFormat = MovieFormat.Nexen,
			SystemType = MapConsoleToSystemType(romInfo.ConsoleType),
			Region = RegionType.NTSC,
			GameName = romInfo.GetRomName(),
			RomFileName = Path.GetFileName(romInfo.RomPath),
			CreatedDate = DateTime.UtcNow,
			ModifiedDate = DateTime.UtcNow,
			ControllerCount = controllerCount
		};

		FilePath = null;
		_currentConverter = MovieConverterRegistry.GetConverter(MovieFormat.Nexen);
		SelectedFrameIndex = -1;
		PlaybackFrame = 0;
		HasUnsavedChanges = true;
		StatusMessage = "Created new movie";

		PopulatePortTypesFromRuntime();
		UpdateFrames();
		UpdateActivePortCount();
		UpdateControllerButtons();
		RefreshMarkerEntries();
		RefreshSelectedFramePreview();
	}

	private static SystemType MapConsoleToSystemType(ConsoleType consoleType) {
		return consoleType switch {
			ConsoleType.Nes => SystemType.Nes,
			ConsoleType.Snes => SystemType.Snes,
			ConsoleType.Gameboy => SystemType.Gb,
			ConsoleType.Gba => SystemType.Gba,
			ConsoleType.Sms => SystemType.Sms,
			ConsoleType.PcEngine => SystemType.Pce,
			ConsoleType.Ws => SystemType.Ws,
			ConsoleType.Lynx => SystemType.Lynx,
			ConsoleType.Atari2600 => SystemType.A2600,
			ConsoleType.Genesis => SystemType.Genesis,
			ConsoleType.ChannelF => SystemType.ChannelF,
			_ => SystemType.Other,
		};
	}

	/// <summary>
	/// Stops input recording.
	/// </summary>
	public void StopRecording() {
		if (!IsRecording) {
			return;
		}

		Recorder.StopRecording();
		SyncNewFrames();
		HasUnsavedChanges = true;
	}

	/// <summary>
	/// Populates the movie's PortTypes and ControllerCount from the runtime controller state.
	/// Called at recording start to ensure the movie captures per-port controller types.
	/// </summary>
	private void PopulatePortTypesFromRuntime() {
		if (Movie is null) {
			return;
		}

		try {
			ControllerInput[] states = InputApi.GetControllerStates();
			if (states.Length == 0) {
				return;
			}

			Movie.ControllerCount = states.Length;
			for (int i = 0; i < states.Length && i < Movie.PortTypes.Length; i++) {
				Movie.PortTypes[i] = states[i].Type;
			}

			UpdateActivePortCount();
		} catch (DllNotFoundException) {
			// Expected when emulator core is not loaded
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"PopulatePortTypesFromRuntime failed: {ex.Message}");
		}
	}

	/// <summary>
	/// Starts rerecording from the selected frame.
	/// </summary>
	public void RerecordFromSelected() {
		if (Movie is null || SelectedFrameIndex < 0) {
			return;
		}

		if (SelectedFrameIndex >= Movie.InputFrames.Count) {
			StatusMessage = "Failed to rerecord - selected frame is out of range";
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
		if (Movie is null) {
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
		if (Movie is null) {
			return;
		}

		Recorder.LoadBranch(branch);

		// Loading a branch replaces all frames — stacked undo/redo actions reference
		// stale frame data and indices, so clear both stacks to prevent corruption.
		_undoStack.Clear();
		_redoStack.Clear();
		UpdateUndoRedoState();

		Greenzone.InvalidateFrom(0);
		UpdateFrames();
		HasUnsavedChanges = true;
		StatusMessage = $"Loaded branch: {branch.Name}";
	}

	/// <summary>
	/// Shows the branch management dialog.
	/// </summary>
	public async System.Threading.Tasks.Task ManageBranchesAsync() {
		if (_window is null) {
			return;
		}

		var result = await Windows.BranchManagementDialog.ShowAsync(_window, Branches);

		if (result.Changed) {
			HasUnsavedChanges = true;
		}

		if (result.LoadedBranch is not null) {
			LoadBranch(result.LoadedBranch);
		}
	}

	/// <summary>
	/// Shows greenzone settings dialog.
	/// </summary>
	public async System.Threading.Tasks.Task ShowGreenzoneSettingsAsync() {
		if (_window is null) {
			return;
		}

		var settings = await Windows.GreenzoneSettingsDialog.ShowAsync(_window, Greenzone);
		if (settings is null) {
			return;
		}

		if (settings.ClearRequested) {
			Greenzone.Clear();
			StatusMessage = "Greenzone cleared";
		}

		Greenzone.CaptureInterval = settings.CaptureInterval;
		Greenzone.MaxSavestates = settings.MaxSavestates;
		Greenzone.MaxMemoryBytes = settings.MaxMemoryMB * 1024L * 1024L;
		Greenzone.RingBufferSize = settings.RingBufferSize;
		Greenzone.CompressionEnabled = settings.CompressionEnabled;
		Greenzone.CompressionThreshold = settings.CompressionThreshold;

		SavestateCount = Greenzone.SavestateCount;
		GreenzoneMemoryMB = Greenzone.TotalMemoryUsage / (1024.0 * 1024.0);
		StatusMessage = $"Greenzone settings updated (interval={settings.CaptureInterval}, max={settings.MaxSavestates}, memory={settings.MaxMemoryMB}MB)";
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

			if (_currentScriptId < 0) {
				IsScriptRunning = false;
				CurrentScriptPath = null;
				StatusMessage = "Failed to load script";
				return;
			}

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

		int scriptId = _currentScriptId;
		_currentScriptId = -1;
		IsScriptRunning = false;
		CurrentScriptPath = null;

		try {
			DebugApi.RemoveScript(scriptId);
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
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Failed to open API documentation: {ex.Message}");
			StatusMessage = "Could not open API documentation";
		}
	}

	/// <summary>
	/// Generates the script API documentation.
	/// </summary>
	private void GenerateScriptApiDoc(string path) {
		string html = GenerateScriptApiHtml();
		File.WriteAllText(path, html);
	}

	/// <summary>
	/// Generates the HTML content for the TAS Lua API reference.
	/// </summary>
	internal static string GenerateScriptApiHtml() {
		return @"<!DOCTYPE html>
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
		} catch (Exception ex) {
			System.Diagnostics.Debug.WriteLine($"Failed to open script folder: {ex.Message}");
			StatusMessage = "Could not open script folder";
		}
	}

	#endregion

	#region Controller Layout

	/// <summary>
	/// Detects the controller layout from the loaded movie.
	/// </summary>
	private void DetectControllerLayout() {
		if (Movie is null) {
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
			SystemType.Lynx => ControllerLayout.Lynx,
			SystemType.A2600 => ControllerLayout.Atari2600,
			SystemType.ChannelF => ControllerLayout.ChannelF,
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
			ControllerLayout.Lynx => GetLynxButtons(),
			ControllerLayout.Atari2600 => GetAtari2600Buttons(),
			ControllerLayout.ChannelF => GetChannelFButtons(),
			_ => GetSnesButtons()
		};
		_pianoRollButtonLabelsCache = null;
		this.RaisePropertyChanged(nameof(PianoRollButtonLabels));
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

	private static List<ControllerButtonInfo> GetLynxButtons() => new() {
		new("A", "A", 0, 0),
		new("B", "B", 1, 0),
		new("L", "O1", 2, 0),
		new("R", "O2", 3, 0),
		new("START", "Pau", 4, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	/// <summary>
	/// Returns Atari 2600 button layout based on controller subtype stored in port 1.
	/// </summary>
	private List<ControllerButtonInfo> GetAtari2600Buttons() {
		int port = SelectedEditPort;
		MovieConverter.ControllerType portType = (Movie?.PortTypes is { Length: > 0 } && port < Movie.PortTypes.Length)
			? Movie.PortTypes[port]
			: MovieConverter.ControllerType.Atari2600Joystick;

		return portType switch {
			MovieConverter.ControllerType.Atari2600Paddle => GetAtari2600PaddleButtons(),
			MovieConverter.ControllerType.Atari2600Keypad => GetAtari2600KeypadButtons(),
			MovieConverter.ControllerType.Atari2600DrivingController => GetAtari2600DrivingButtons(),
			MovieConverter.ControllerType.Atari2600BoosterGrip => GetAtari2600BoosterGripButtons(),
			_ => GetAtari2600JoystickButtons(),
		};
	}

	private static List<ControllerButtonInfo> GetAtari2600JoystickButtons() => new() {
		new("FIRE", "Fire", 0, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetAtari2600PaddleButtons() => new() {
		new("FIRE", "Fire", 0, 0),
	};

	private static List<ControllerButtonInfo> GetAtari2600KeypadButtons() => new() {
		new("1", "1", 0, 0),
		new("2", "2", 1, 0),
		new("3", "3", 2, 0),
		new("4", "4", 0, 1),
		new("5", "5", 1, 1),
		new("6", "6", 2, 1),
		new("7", "7", 0, 2),
		new("8", "8", 1, 2),
		new("9", "9", 2, 2),
		new("STAR", "*", 0, 3),
		new("0", "0", 1, 3),
		new("POUND", "#", 2, 3),
	};

	private static List<ControllerButtonInfo> GetAtari2600DrivingButtons() => new() {
		new("FIRE", "Fire", 0, 0),
		new("LEFT", "←", 0, 1),
		new("RIGHT", "→", 1, 1),
	};

	private static List<ControllerButtonInfo> GetAtari2600BoosterGripButtons() => new() {
		new("FIRE", "Fire", 0, 0),
		new("TRIGGER", "Trg", 1, 0),
		new("BOOSTER", "Bst", 2, 0),
		new("UP", "↑", 0, 1),
		new("DOWN", "↓", 1, 1),
		new("LEFT", "←", 2, 1),
		new("RIGHT", "→", 3, 1),
	};

	private static List<ControllerButtonInfo> GetChannelFButtons() => new() {
		new("RIGHT", "Right", 0, 0),
		new("LEFT", "Left", 1, 0),
		new("BACK", "Back", 2, 0),
		new("FORWARD", "Fwd", 3, 0),
		new("TWISTCCW", "TwL", 0, 1),
		new("TWISTCW", "TwR", 1, 1),
		new("PULL", "Pull", 2, 1),
		new("PUSH", "Push", 3, 1),
	};

	#endregion

	protected override void DisposeView() {
		if (_autoSaveTimer is not null) {
			_autoSaveTimer.Tick -= OnAutoSaveTimerTick;
			_autoSaveTimer.Stop();
		}

		Greenzone.CancelSeek();
		Greenzone.Dispose();
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
public sealed partial class TasFrameViewModel : ViewModelBase {
	private int _frameNumber;
	private bool _isGreenzone;

	/// <summary>Gets the underlying frame.</summary>
	public InputFrame Frame { get; }

	/// <summary>Gets or sets the frame number (1-based for display).</summary>
	public int FrameNumber {
		get => _frameNumber;
		set => this.RaiseAndSetIfChanged(ref _frameNumber, value);
	}

	/// <summary>Gets or sets whether this frame is in the greenzone.</summary>
	public bool IsGreenzone {
		get => _isGreenzone;
		set {
			if (_isGreenzone != value) {
				_isGreenzone = value;
				this.RaisePropertyChanged();
				this.RaisePropertyChanged(nameof(Background));
			}
		}
	}

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
		_frameNumber = index + 1; // 1-based display
		_isGreenzone = isGreenzone;
	}

	/// <summary>
	/// Raises property changed for all computed properties that depend on the underlying frame data.
	/// Call this after modifying the underlying InputFrame.
	/// Uses empty property name to signal "all properties changed" in a single notification
	/// instead of 7 separate RaisePropertyChanged calls.
	/// </summary>
	public void RefreshFromFrame() {
		this.RaisePropertyChanged(string.Empty);
	}
}

#region Undoable Actions

/// <summary>
/// Base class for undoable actions in the TAS editor.
/// </summary>
public abstract partial class UndoableAction {
	/// <summary>Gets a description of this action.</summary>
	public abstract string Description { get; }

	/// <summary>Executes the action.</summary>
	public abstract void Execute();

	/// <summary>Undoes the action.</summary>
	public abstract void Undo();
}

/// <summary>
/// Action that groups multiple undoable actions into a single undo/redo step.
/// </summary>
public sealed partial class BulkUndoableAction : UndoableAction {
	private readonly List<UndoableAction> _actions;
	private readonly string _description;

	public IReadOnlyList<UndoableAction> Actions => _actions;

	public override string Description => _description;

	public BulkUndoableAction(string description, List<UndoableAction> actions) {
		_description = description;
		_actions = actions;
	}

	public override void Execute() {
		foreach (UndoableAction action in _actions) {
			action.Execute();
		}
	}

	public override void Undo() {
		for (int i = _actions.Count - 1; i >= 0; i--) {
			_actions[i].Undo();
		}
	}
}

/// <summary>
/// Action for inserting frames.
/// </summary>
public sealed partial class InsertFramesAction : UndoableAction {
	private readonly MovieData _movie;
	private readonly int _index;
	private readonly List<InputFrame> _frames;

	public int Index => _index;
	public int Count => _frames.Count;

	public override string Description => $"Insert {_frames.Count} frame(s)";

	public InsertFramesAction(MovieData movie, int index, List<InputFrame> frames) {
		_movie = movie;
		_index = index;
		_frames = frames;
	}

	public override void Execute() {
		_movie.InputFrames.InsertRange(_index, _frames);
	}

	public override void Undo() {
		_movie.InputFrames.RemoveRange(_index, _frames.Count);
	}
}

/// <summary>
/// Action for deleting frames.
/// </summary>
public sealed partial class DeleteFramesAction : UndoableAction {
	private readonly MovieData _movie;
	private readonly int _index;
	private readonly List<InputFrame> _deletedFrames;

	public int Index => _index;
	public int Count => _deletedFrames.Count;

	public override string Description => $"Delete {_deletedFrames.Count} frame(s)";

	public DeleteFramesAction(MovieData movie, int index, int count) {
		_movie = movie;
		_index = index;
		_deletedFrames = _movie.InputFrames.GetRange(index, count);
	}

	public override void Execute() {
		_movie.InputFrames.RemoveRange(_index, _deletedFrames.Count);
	}

	public override void Undo() {
		_movie.InputFrames.InsertRange(_index, _deletedFrames);
	}
}

/// <summary>
/// Action for modifying controller input.
/// </summary>
public sealed partial class ModifyInputAction : UndoableAction {
	private readonly InputFrame _frame;
	private readonly int _frameIndex;
	private readonly int _port;
	private readonly ControllerInput _oldInput;
	private readonly ControllerInput _newInput;

	public InputFrame FrameRef => _frame;
	public int FrameIndex => _frameIndex;

	public override string Description => "Modify input";

	public ModifyInputAction(InputFrame frame, int frameIndex, int port, ControllerInput newInput) {
		_frame = frame;
		_frameIndex = frameIndex;
		_port = port;
		_oldInput = frame.Controllers[port].Clone();
		_newInput = newInput;
	}

	public override void Execute() {
		_frame.Controllers[_port] = _newInput;
	}

	public override void Undo() {
		_frame.Controllers[_port] = _oldInput;
	}
}

/// <summary>
/// Action for modifying FrameCommand flags on one or more frames.
/// Used for toggling console switches (Atari 2600 Select/Reset).
/// </summary>
public sealed partial class ModifyCommandAction : UndoableAction {
	private readonly List<(InputFrame Frame, int FrameIndex, FrameCommand OldCommand)> _frames;
	private readonly FrameCommand _flag;
	private readonly bool _newState;

	public IReadOnlyList<int> FrameIndices => _frames.Select(f => f.FrameIndex).ToList();

	public override string Description => $"Toggle {_flag}";

	public ModifyCommandAction(IReadOnlyList<InputFrame> frames, IReadOnlyList<int> frameIndices, FrameCommand flag, bool newState) {
		_flag = flag;
		_newState = newState;
		_frames = new(frames.Count);
		for (int i = 0; i < frames.Count; i++) {
			_frames.Add((frames[i], frameIndices[i], frames[i].Command));
		}
	}

	public override void Execute() {
		foreach (var (frame, _, _) in _frames) {
			if (_newState) {
				frame.Command |= _flag;
			} else {
				frame.Command &= ~_flag;
			}
		}
	}

	public override void Undo() {
		foreach (var (frame, _, oldCommand) in _frames) {
			frame.Command = oldCommand;
		}
	}
}

/// <summary>
/// Action for clearing all controller inputs on a frame.
/// </summary>
public sealed partial class ClearInputAction : UndoableAction {
	private readonly InputFrame _frame;
	private readonly int _frameIndex;
	private readonly ControllerInput[] _oldInputs;

	public int FrameIndex => _frameIndex;

	public override string Description => "Clear input";

	public ClearInputAction(InputFrame frame, int frameIndex) {
		_frame = frame;
		_frameIndex = frameIndex;
		_oldInputs = new ControllerInput[frame.Controllers.Length];
		for (int i = 0; i < frame.Controllers.Length; i++) {
			_oldInputs[i] = frame.Controllers[i].Clone();
		}
	}

	public override void Execute() {
		for (int i = 0; i < _frame.Controllers.Length; i++) {
			_frame.Controllers[i] = new ControllerInput();
		}
	}

	public override void Undo() {
		for (int i = 0; i < _oldInputs.Length && i < _frame.Controllers.Length; i++) {
			_frame.Controllers[i] = _oldInputs[i];
		}
	}
}

/// <summary>
/// Action for batch-painting a button across multiple frames.
/// </summary>
public sealed partial class PaintInputAction : UndoableAction {
	private readonly MovieData _movie;
	private readonly List<int> _frameIndices;
	private readonly int _port;
	private readonly string _button;
	private readonly bool _state;
	private readonly List<ControllerInput> _oldInputs;

	public IReadOnlyList<int> FrameIndices => _frameIndices;

	public override string Description => $"Paint {_frameIndices.Count} frame(s)";

	public PaintInputAction(MovieData movie, List<int> frameIndices, int port, string button, bool state, List<ControllerInput> oldInputs) {
		_movie = movie;
		_frameIndices = frameIndices;
		_port = port;
		_button = button;
		_state = state;
		_oldInputs = oldInputs;
	}

	public override void Execute() {
		for (int i = 0; i < _frameIndices.Count; i++) {
			_movie.InputFrames[_frameIndices[i]].Controllers[_port].SetButton(_button, _state);
		}
	}

	public override void Undo() {
		for (int i = 0; i < _frameIndices.Count; i++) {
			_movie.InputFrames[_frameIndices[i]].Controllers[_port] = _oldInputs[i];
		}
	}
}

/// <summary>
/// Action for modifying a frame's comment/marker text. Undoable.
/// Used by ToggleMarker, SetComment, and UpsertMarkerEntry.
/// </summary>
public sealed partial class ModifyCommentAction : UndoableAction {
	private readonly InputFrame _frame;
	private readonly int _frameIndex;
	private readonly string? _oldComment;
	private readonly string? _newComment;

	public int FrameIndex => _frameIndex;

	public override string Description {
		get {
			if (_newComment is not null && _oldComment is null) return "Add comment";
			if (_newComment is null && _oldComment is not null) return "Remove comment";
			return "Modify comment";
		}
	}

	public ModifyCommentAction(InputFrame frame, int frameIndex, string? newComment) {
		_frame = frame;
		_frameIndex = frameIndex;
		_oldComment = frame.Comment;
		_newComment = newComment;
	}

	public override void Execute() {
		_frame.Comment = _newComment;
	}

	public override void Undo() {
		_frame.Comment = _oldComment;
	}
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
	WonderSwan,

	/// <summary>Lynx - A, B, Option1, Option2, Pause, D-Pad</summary>
	Lynx,

	/// <summary>Atari 2600 - varies by controller type (Joystick, Paddle, Keypad, Driving, BoosterGrip)</summary>
	Atari2600,

	/// <summary>Fairchild Channel F - Right, Left, Back, Forward, Twist CCW, Twist CW, Pull, Push</summary>
	ChannelF
}

/// <summary>
/// Information about a controller button for UI display.
/// </summary>
public sealed partial class ControllerButtonInfo {
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

/// <summary>
/// View model item for the selected-frame controller diagram preview.
/// </summary>
public sealed partial class ControllerButtonPreviewViewModel {
	/// <summary>Gets the button identifier used in ControllerInput.</summary>
	public string ButtonId { get; }

	/// <summary>Gets the display label for the button.</summary>
	public string Label { get; }

	/// <summary>Gets the column position for layout-aware ordering.</summary>
	public int Column { get; }

	/// <summary>Gets the row position for layout-aware ordering.</summary>
	public int Row { get; }

	/// <summary>Gets whether this button is currently pressed.</summary>
	public bool IsPressed { get; }

	/// <summary>Gets the preview background brush based on press state.</summary>
	public IBrush Background => IsPressed ? Brushes.ForestGreen : Brushes.DimGray;

	/// <summary>Gets the preview foreground brush.</summary>
	public IBrush Foreground => Brushes.White;

	public ControllerButtonPreviewViewModel(string buttonId, string label, int column, int row, bool isPressed) {
		ButtonId = buttonId;
		Label = label;
		Column = column;
		Row = row;
		IsPressed = isPressed;
	}
}

#endregion

#region Frame Search Types

/// <summary>
/// Search mode for finding frames in the TAS editor.
/// </summary>
public enum FrameSearchMode {
	/// <summary>Search frame comments for matching text.</summary>
	Comment,

	/// <summary>Search for frames with a specific button pressed.</summary>
	ButtonPressed,

	/// <summary>Search for lag frames.</summary>
	LagFrame,

	/// <summary>Search for frames with markers.</summary>
	Marker
}

/// <summary>
/// Marker/comment type classification for panel filtering.
/// </summary>
public enum MarkerEntryType {
	Comment,
	Marker,
	Todo
}

/// <summary>
/// Filter for marker/comment list panel.
/// </summary>
public enum MarkerEntryFilterType {
	All,
	Comment,
	Marker,
	Todo
}

/// <summary>
/// View model item for marker/comment panel rows.
/// </summary>
public sealed partial class TasMarkerEntryViewModel {
	public int FrameIndex { get; }
	public string Comment { get; }
	public MarkerEntryType Type { get; }

	public TasMarkerEntryViewModel(int frameIndex, string comment, MarkerEntryType type) {
		FrameIndex = frameIndex;
		Comment = comment;
		Type = type;
	}
}

#endregion
