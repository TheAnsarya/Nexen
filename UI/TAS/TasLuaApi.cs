using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Nexen.Interop;
using Nexen.ViewModels;
using Nexen.MovieConverter;

namespace Nexen.TAS;

/// <summary>
/// Provides Lua scripting API for TAS operations.
/// Exposes movie manipulation, input search, and automation functions to Lua scripts.
/// </summary>
public class TasLuaApi {
	private readonly TasEditorViewModel _viewModel;
	private readonly GreenzoneManager _greenzone;
	private readonly InputRecorder _recorder;

	private bool _isSearching;
	private int _searchStartFrame;
	private int _searchBestFrame;
	private Dictionary<string, object> _searchState = new();

	/// <summary>
	/// Initializes TAS Lua API with references to core TAS components.
	/// </summary>
	public TasLuaApi(TasEditorViewModel viewModel) {
		_viewModel = viewModel;
		_greenzone = viewModel.Greenzone;
		_recorder = viewModel.Recorder;
	}

	#region Movie Information

	/// <summary>
	/// Gets the current frame number.
	/// </summary>
	public int GetCurrentFrame() => _viewModel.PlaybackFrame;

	/// <summary>
	/// Gets the total number of frames in the movie.
	/// </summary>
	public int GetTotalFrames() => _viewModel.Movie?.InputFrames.Count ?? 0;

	/// <summary>
	/// Gets the current rerecord count.
	/// </summary>
	public int GetRerecordCount() => _viewModel.RerecordCount;

	/// <summary>
	/// Gets whether the movie has been modified.
	/// </summary>
	public bool IsModified() => _viewModel.HasUnsavedChanges;

	/// <summary>
	/// Gets the movie's system type (NES, SNES, etc.).
	/// </summary>
	public string GetSystemType() => _viewModel.Movie?.SystemType.ToString() ?? "Unknown";

	/// <summary>
	/// Gets the movie's author.
	/// </summary>
	public string GetAuthor() => _viewModel.Movie?.Author ?? "";

	/// <summary>
	/// Gets movie metadata as a table.
	/// </summary>
	public Dictionary<string, object> GetMovieInfo() {
		var movie = _viewModel.Movie;
		if (movie == null) {
			return new Dictionary<string, object>();
		}

		return new Dictionary<string, object> {
			["frameCount"] = movie.InputFrames.Count,
			["rerecordCount"] = movie.RerecordCount,
			["author"] = movie.Author ?? "",
			["systemType"] = movie.SystemType.ToString(),
			["romName"] = movie.RomFileName ?? "",
			["romChecksum"] = movie.Sha1Hash ?? "",
			["startType"] = movie.StartType.ToString(),
			["description"] = movie.Description ?? ""
		};
	}

	#endregion

	#region Frame Navigation

	/// <summary>
	/// Seeks to a specific frame using greenzone if available.
	/// </summary>
	public async Task<bool> SeekToFrame(int frame) {
		int totalFrames = GetTotalFrames();
		if (frame < 0 || frame > totalFrames) {
			return false;
		}

		int actualFrame = await _greenzone.SeekToFrameAsync(frame);
		return actualFrame >= 0;
	}

	/// <summary>
	/// Advances one frame.
	/// </summary>
	public void FrameAdvance() {
		EmuApi.ExecuteShortcut(new ExecuteShortcutParams { Shortcut = Nexen.Config.Shortcuts.EmulatorShortcut.RunSingleFrame });
	}

	/// <summary>
	/// Rewinds one frame.
	/// </summary>
	public void FrameRewind() {
		EmuApi.ExecuteShortcut(new ExecuteShortcutParams { Shortcut = Nexen.Config.Shortcuts.EmulatorShortcut.Rewind });
	}

	/// <summary>
	/// Pauses emulation.
	/// </summary>
	public void Pause() {
		EmuApi.Pause();
	}

	/// <summary>
	/// Resumes emulation.
	/// </summary>
	public void Resume() {
		EmuApi.Resume();
	}

	#endregion

	#region Input Manipulation

	/// <summary>
	/// Gets input for a specific frame.
	/// </summary>
	public Dictionary<string, bool>? GetFrameInput(int frame, int controller = 0) {
		var movie = _viewModel.Movie;
		if (movie == null || frame < 0 || frame >= movie.InputFrames.Count) {
			return null;
		}

		var frameData = movie.InputFrames[frame];
		if (controller >= frameData.Controllers.Length) {
			return null;
		}

		var input = frameData.Controllers[controller];
		return new Dictionary<string, bool> {
			["a"] = input.A,
			["b"] = input.B,
			["x"] = input.X,
			["y"] = input.Y,
			["l"] = input.L,
			["r"] = input.R,
			["select"] = input.Select,
			["start"] = input.Start,
			["up"] = input.Up,
			["down"] = input.Down,
			["left"] = input.Left,
			["right"] = input.Right
		};
	}

	/// <summary>
	/// Sets input for a specific frame.
	/// </summary>
	public bool SetFrameInput(int frame, Dictionary<string, bool> input, int controller = 0) {
		var movie = _viewModel.Movie;
		if (movie == null || frame < 0 || frame >= movie.InputFrames.Count) {
			return false;
		}

		var frameData = movie.InputFrames[frame];
		if (controller >= frameData.Controllers.Length) {
			return false;
		}

		var controllerInput = frameData.Controllers[controller];

		if (input.TryGetValue("a", out bool a)) controllerInput.A = a;
		if (input.TryGetValue("b", out bool b)) controllerInput.B = b;
		if (input.TryGetValue("x", out bool x)) controllerInput.X = x;
		if (input.TryGetValue("y", out bool y)) controllerInput.Y = y;
		if (input.TryGetValue("l", out bool l)) controllerInput.L = l;
		if (input.TryGetValue("r", out bool r)) controllerInput.R = r;
		if (input.TryGetValue("select", out bool select)) controllerInput.Select = select;
		if (input.TryGetValue("start", out bool start)) controllerInput.Start = start;
		if (input.TryGetValue("up", out bool up)) controllerInput.Up = up;
		if (input.TryGetValue("down", out bool down)) controllerInput.Down = down;
		if (input.TryGetValue("left", out bool left)) controllerInput.Left = left;
		if (input.TryGetValue("right", out bool right)) controllerInput.Right = right;

		_viewModel.HasUnsavedChanges = true;
		_greenzone.InvalidateFrom(frame);

		return true;
	}

	/// <summary>
	/// Clears all input for a frame.
	/// </summary>
	public bool ClearFrameInput(int frame, int controller = 0) {
		var movie = _viewModel.Movie;
		if (movie == null || frame < 0 || frame >= movie.InputFrames.Count) {
			return false;
		}

		var frameData = movie.InputFrames[frame];
		if (controller >= frameData.Controllers.Length) {
			return false;
		}

		var input = frameData.Controllers[controller];
		input.A = input.B = input.X = input.Y = false;
		input.L = input.R = input.Select = input.Start = false;
		input.Up = input.Down = input.Left = input.Right = false;

		_viewModel.HasUnsavedChanges = true;
		_greenzone.InvalidateFrom(frame);

		return true;
	}

	/// <summary>
	/// Inserts empty frames at a position.
	/// </summary>
	public bool InsertFrames(int position, int count) {
		if (count <= 0 || position < 0) {
			return false;
		}

		// TODO: Implement when InsertFrameAt is available
		return false;
	}

	/// <summary>
	/// Deletes frames from a position.
	/// </summary>
	public bool DeleteFrames(int position, int count) {
		if (count <= 0 || position < 0) {
			return false;
		}

		// TODO: Implement when DeleteFrameAt is available
		return false;
	}

	#endregion

	#region Greenzone Operations

	/// <summary>
	/// Gets greenzone statistics.
	/// </summary>
	public Dictionary<string, object> GetGreenzoneInfo() {
		return new Dictionary<string, object> {
			["savestateCount"] = _greenzone.SavestateCount,
			["memoryUsageMB"] = _greenzone.TotalMemoryUsage / (1024.0 * 1024.0),
			["captureInterval"] = _greenzone.CaptureInterval,
			["maxSavestates"] = _greenzone.MaxSavestates,
			["compressionEnabled"] = _greenzone.CompressionEnabled
		};
	}

	/// <summary>
	/// Captures a savestate at the current frame.
	/// </summary>
	public void CaptureGreenzone() {
		_greenzone.CaptureCurrentState(_viewModel.PlaybackFrame);
	}

	/// <summary>
	/// Clears all greenzone savestates.
	/// </summary>
	public void ClearGreenzone() {
		_greenzone.Clear();
	}

	/// <summary>
	/// Checks if a frame has a greenzone savestate.
	/// </summary>
	public bool HasSavestate(int frame) {
		return _greenzone.SavestateCount > 0; // Simplified check
	}

	#endregion

	#region Recording Operations

	/// <summary>
	/// Starts recording in the specified mode.
	/// </summary>
	public void StartRecording(string mode = "append") {
		var recordMode = mode.ToLowerInvariant() switch {
			"insert" => RecordingMode.Insert,
			"overwrite" => RecordingMode.Overwrite,
			_ => RecordingMode.Append
		};

		_recorder.StartRecording(recordMode);
	}

	/// <summary>
	/// Stops recording.
	/// </summary>
	public void StopRecording() {
		_recorder.StopRecording();
	}

	/// <summary>
	/// Gets whether recording is active.
	/// </summary>
	public bool IsRecording() => _recorder.IsRecording;

	/// <summary>
	/// Rerecords from a specific frame.
	/// </summary>
	public bool RerecordFrom(int frame) {
		return _recorder.RerecordFrom(frame);
	}

	#endregion

	#region Branch Operations

	/// <summary>
	/// Creates a new branch with the current movie state.
	/// </summary>
	public string CreateBranch(string name = "") {
		var branch = _recorder.CreateBranch(name);
		return branch?.Name ?? "";
	}

	/// <summary>
	/// Gets list of branch names.
	/// </summary>
	public List<string> GetBranches() {
		var branches = new List<string>();
		foreach (var branch in _viewModel.Branches) {
			branches.Add(branch.Name);
		}
		return branches;
	}

	/// <summary>
	/// Loads a branch by name.
	/// </summary>
	public bool LoadBranch(string name) {
		foreach (var branch in _viewModel.Branches) {
			if (branch.Name == name) {
				_recorder.LoadBranch(branch);
				return true;
			}
		}
		return false;
	}

	/// <summary>
	/// Gets branch info by name.
	/// </summary>
	public Dictionary<string, object>? GetBranchInfo(string name) {
		foreach (var branch in _viewModel.Branches) {
			if (branch.Name == name) {
				return new Dictionary<string, object> {
					["name"] = branch.Name,
					["createdAt"] = branch.CreatedAt.ToString("yyyy-MM-dd HH:mm:ss"),
					["frameCount"] = branch.FrameCount
				};
			}
		}
		return null;
	}

	#endregion

	#region Input Search / Brute Force

	/// <summary>
	/// Begins an input search from the current frame.
	/// Call TestInput() to try combinations and FinishSearch() when done.
	/// </summary>
	public void BeginSearch() {
		_isSearching = true;
		_searchStartFrame = _viewModel.PlaybackFrame;
		_searchBestFrame = _searchStartFrame;
		_searchState.Clear();

		// Create a branch to preserve current state
		_recorder.CreateBranch($"Search_{DateTime.Now:HHmmss}");
	}

	/// <summary>
	/// Tests a specific input combination and advances frames.
	/// Returns the resulting frame number.
	/// </summary>
	public async Task<int> TestInput(Dictionary<string, bool> input, int framesToRun = 1) {
		if (!_isSearching) {
			return -1;
		}

		// Seek back to start frame
		await SeekToFrame(_searchStartFrame);

		// Set the input
		SetFrameInput(_searchStartFrame, input);

		// Run specified frames
		for (int i = 0; i < framesToRun; i++) {
			FrameAdvance();
		}

		return _viewModel.PlaybackFrame;
	}

	/// <summary>
	/// Sets a value in the search state (for tracking best results).
	/// </summary>
	public void SetSearchState(string key, object value) {
		_searchState[key] = value;
	}

	/// <summary>
	/// Gets a value from the search state.
	/// </summary>
	public object? GetSearchState(string key) {
		return _searchState.TryGetValue(key, out var value) ? value : null;
	}

	/// <summary>
	/// Marks the current state as the best found so far.
	/// </summary>
	public void MarkBestResult() {
		_searchBestFrame = _viewModel.PlaybackFrame;
		_recorder.CreateBranch($"Best_{DateTime.Now:HHmmss}");
	}

	/// <summary>
	/// Ends the search and optionally loads the best result.
	/// </summary>
	public void FinishSearch(bool loadBest = true) {
		_isSearching = false;

		if (loadBest && _viewModel.Branches.Count > 0) {
			// Find the "Best_" branch
			foreach (var branch in _viewModel.Branches) {
				if (branch.Name.StartsWith("Best_")) {
					_recorder.LoadBranch(branch);
					break;
				}
			}
		}
	}

	/// <summary>
	/// Gets whether a search is in progress.
	/// </summary>
	public bool IsSearching() => _isSearching;

	#endregion

	#region Utility Functions

	/// <summary>
	/// Generates all button combinations for a controller.
	/// Useful for brute force searching.
	/// </summary>
	public List<Dictionary<string, bool>> GenerateInputCombinations(List<string>? buttons = null) {
		buttons ??= new List<string> { "a", "b", "up", "down", "left", "right" };

		var combinations = new List<Dictionary<string, bool>>();
		int totalCombinations = 1 << buttons.Count; // 2^n combinations

		for (int i = 0; i < totalCombinations; i++) {
			var combo = new Dictionary<string, bool>();
			for (int j = 0; j < buttons.Count; j++) {
				combo[buttons[j]] = (i & (1 << j)) != 0;
			}
			combinations.Add(combo);
		}

		return combinations;
	}

	/// <summary>
	/// Reads a memory address (wraps debugger API).
	/// </summary>
	public int ReadMemory(int address, int memType = 0) {
		return (int)DebugApi.GetMemoryValue((MemoryType)memType, (uint)address);
	}

	/// <summary>
	/// Writes to a memory address (wraps debugger API).
	/// </summary>
	public void WriteMemory(int address, int value, int memType = 0) {
		DebugApi.SetMemoryValue((MemoryType)memType, (uint)address, (byte)value);
	}

	/// <summary>
	/// Reads a 16-bit value from memory.
	/// </summary>
	public int ReadMemory16(int address, int memType = 0) {
		int low = ReadMemory(address, memType);
		int high = ReadMemory(address + 1, memType);
		return low | (high << 8);
	}

	/// <summary>
	/// Logs a message to the script output.
	/// </summary>
	public void Log(string message) {
		// Use DisplayMessage for now as there's no dedicated log API
		EmuApi.DisplayMessage("TAS Script", message);
	}

	/// <summary>
	/// Displays an on-screen message.
	/// </summary>
	public void DisplayMessage(string message) {
		EmuApi.DisplayMessage("TAS", message);
	}

	#endregion
}
