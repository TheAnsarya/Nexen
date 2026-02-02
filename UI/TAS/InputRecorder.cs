using System;
using System.Collections.Generic;
using System.Linq;
using Nexen.Config;
using Nexen.Interop;
using Nexen.MovieConverter;

namespace Nexen.TAS;

/// <summary>
/// Manages input recording for TAS movie creation.
/// Captures player inputs during gameplay and adds them to the movie.
/// </summary>
public class InputRecorder : IDisposable {
	private readonly GreenzoneManager _greenzone;
	private MovieData? _movie;
	private bool _isRecording;
	private bool _disposed;
	private int _recordStartFrame;
	private RecordingMode _mode = RecordingMode.Append;
	private int _insertPosition;

	/// <summary>Gets or sets the movie being recorded to.</summary>
	public MovieData? Movie {
		get => _movie;
		set {
			if (_isRecording) {
				throw new InvalidOperationException("Cannot change movie while recording.");
			}

			_movie = value;
		}
	}

	/// <summary>Gets whether recording is currently active.</summary>
	public bool IsRecording => _isRecording;

	/// <summary>Gets the current recording mode.</summary>
	public RecordingMode Mode => _mode;

	/// <summary>Gets the frame where recording started.</summary>
	public int RecordStartFrame => _recordStartFrame;

	/// <summary>Gets the number of frames recorded in this session.</summary>
	public int FramesRecorded { get; private set; }

	/// <summary>Gets the total rerecord count since the session started.</summary>
	public int RerecordCount { get; private set; }

	/// <summary>Gets or sets whether to automatically detect lag frames.</summary>
	public bool AutoDetectLagFrames { get; set; } = true;

	/// <summary>Gets or sets whether to capture savestates during recording.</summary>
	public bool CaptureSavestates { get; set; } = true;

	/// <summary>Occurs when recording starts.</summary>
	public event EventHandler<RecordingEventArgs>? RecordingStarted;

	/// <summary>Occurs when recording stops.</summary>
	public event EventHandler<RecordingEventArgs>? RecordingStopped;

	/// <summary>Occurs when a frame is recorded.</summary>
	public event EventHandler<FrameRecordedEventArgs>? FrameRecorded;

	/// <summary>Occurs when rerecording from a frame.</summary>
	public event EventHandler<RerecordEventArgs>? Rerecording;

	/// <summary>
	/// Creates a new input recorder.
	/// </summary>
	/// <param name="greenzone">The greenzone manager for savestate support.</param>
	public InputRecorder(GreenzoneManager greenzone) {
		_greenzone = greenzone;
	}

	/// <summary>
	/// Starts recording input.
	/// </summary>
	/// <param name="mode">The recording mode.</param>
	/// <param name="insertPosition">For Insert mode, the position to insert at.</param>
	public void StartRecording(RecordingMode mode = RecordingMode.Append, int insertPosition = -1) {
		if (_movie == null) {
			throw new InvalidOperationException("No movie loaded.");
		}

		if (_isRecording) {
			return;
		}

		_mode = mode;
		_insertPosition = insertPosition >= 0 ? insertPosition : _movie.InputFrames.Count;
		_recordStartFrame = mode == RecordingMode.Append ? _movie.InputFrames.Count : _insertPosition;
		_isRecording = true;
		FramesRecorded = 0;

		// Resume emulation if paused
		if (EmuApi.IsRunning() && EmuApi.IsPaused()) {
			EmuApi.Resume();
		}

		RecordingStarted?.Invoke(this, new RecordingEventArgs(_recordStartFrame, mode));
	}

	/// <summary>
	/// Stops recording input.
	/// </summary>
	public void StopRecording() {
		if (!_isRecording) {
			return;
		}

		_isRecording = false;

		// Pause emulation
		if (EmuApi.IsRunning() && !EmuApi.IsPaused()) {
			EmuApi.Pause();
		}

		RecordingStopped?.Invoke(this, new RecordingEventArgs(_recordStartFrame, _mode, FramesRecorded));
	}

	/// <summary>
	/// Records a single frame of input.
	/// Call this from the emulation frame callback.
	/// </summary>
	/// <param name="input">The controller input for this frame.</param>
	/// <param name="isLagFrame">Whether this is a lag frame.</param>
	public void RecordFrame(ControllerInput[] input, bool isLagFrame = false) {
		if (!_isRecording || _movie == null) {
			return;
		}

		int frameIndex = _mode switch {
			RecordingMode.Append => _movie.InputFrames.Count,
			RecordingMode.Insert => _insertPosition + FramesRecorded,
			RecordingMode.Overwrite => _insertPosition + FramesRecorded,
			_ => _movie.InputFrames.Count
		};

		var frame = new InputFrame(frameIndex) {
			IsLagFrame = isLagFrame
		};

		// Copy controller inputs
		for (int i = 0; i < input.Length && i < frame.Controllers.Length; i++) {
			frame.Controllers[i] = CloneInput(input[i]);
		}

		// Add/insert/overwrite based on mode
		switch (_mode) {
			case RecordingMode.Append:
				_movie.InputFrames.Add(frame);
				break;

			case RecordingMode.Insert:
				if (_insertPosition + FramesRecorded <= _movie.InputFrames.Count) {
					_movie.InputFrames.Insert(_insertPosition + FramesRecorded, frame);
				} else {
					_movie.InputFrames.Add(frame);
				}

				break;

			case RecordingMode.Overwrite:
				if (_insertPosition + FramesRecorded < _movie.InputFrames.Count) {
					_movie.InputFrames[_insertPosition + FramesRecorded] = frame;
				} else {
					_movie.InputFrames.Add(frame);
				}

				break;
		}

		FramesRecorded++;

		// Capture savestate at intervals
		if (CaptureSavestates) {
			_greenzone.CaptureCurrentState(frameIndex);
		}

		FrameRecorded?.Invoke(this, new FrameRecordedEventArgs(frameIndex, frame, isLagFrame));
	}

	/// <summary>
	/// Starts rerecording from a specific frame.
	/// Loads the savestate and truncates/prepares the movie for recording.
	/// </summary>
	/// <param name="frame">The frame to rerecord from.</param>
	/// <param name="truncate">If true, removes all frames after this point.</param>
	/// <returns>True if rerecording was successfully started.</returns>
	public bool RerecordFrom(int frame, bool truncate = true) {
		if (_movie == null) {
			return false;
		}

		// Stop any current recording
		StopRecording();

		// Load the savestate for this frame
		if (!_greenzone.LoadState(frame)) {
			// Try to seek to the frame if no exact state
			var nearestFrame = _greenzone.GetNearestStateFrame(frame);

			if (nearestFrame < 0) {
				return false;
			}

			_greenzone.LoadState(nearestFrame);
			// Would need to advance frames here...
		}

		// Truncate movie if requested
		if (truncate && frame < _movie.InputFrames.Count) {
			int removeCount = _movie.InputFrames.Count - frame;

			for (int i = 0; i < removeCount; i++) {
				_movie.InputFrames.RemoveAt(frame);
			}

			// Invalidate greenzone from this point
			_greenzone.InvalidateFrom(frame);
		}

		// Update rerecord count
		RerecordCount++;
		_movie.RerecordCount = (ulong)RerecordCount;

		Rerecording?.Invoke(this, new RerecordEventArgs(frame, truncate, RerecordCount));

		// Start recording in append mode from this point
		StartRecording(RecordingMode.Append);

		return true;
	}

	/// <summary>
	/// Creates a new branch from the current state.
	/// </summary>
	/// <param name="name">Optional name for the branch.</param>
	/// <returns>The branch data.</returns>
	public BranchData CreateBranch(string? name = null) {
		if (_movie == null) {
			throw new InvalidOperationException("No movie loaded.");
		}

		var branch = new BranchData {
			Name = name ?? $"Branch {DateTime.Now:HH:mm:ss}",
			CreatedAt = DateTime.UtcNow,
			FrameCount = _movie.InputFrames.Count,
			RerecordCount = RerecordCount,
			Frames = _movie.InputFrames.Select(CloneFrame).ToList()
		};

		return branch;
	}

	/// <summary>
	/// Loads a branch, replacing the current movie data.
	/// </summary>
	/// <param name="branch">The branch to load.</param>
	public void LoadBranch(BranchData branch) {
		if (_movie == null) {
			throw new InvalidOperationException("No movie loaded.");
		}

		if (_isRecording) {
			StopRecording();
		}

		// Replace movie frames with branch frames
		_movie.InputFrames.Clear();

		foreach (var frame in branch.Frames) {
			_movie.InputFrames.Add(CloneFrame(frame));
		}

		// Clear greenzone since movie changed
		_greenzone.Clear();
	}

	/// <summary>
	/// Gets the current input state from the emulator.
	/// </summary>
	/// <returns>Array of controller inputs.</returns>
	public static ControllerInput[] GetCurrentInput() {
		// TODO: Hook into InputApi to get actual controller state
		// For now, return empty inputs
		return new ControllerInput[4];
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

	private static InputFrame CloneFrame(InputFrame original) {
		var clone = new InputFrame(original.FrameNumber) {
			IsLagFrame = original.IsLagFrame,
			Comment = original.Comment
		};

		for (int i = 0; i < original.Controllers.Length && i < clone.Controllers.Length; i++) {
			clone.Controllers[i] = CloneInput(original.Controllers[i]);
		}

		return clone;
	}

	public void Dispose() {
		if (_disposed) {
			return;
		}

		_disposed = true;

		if (_isRecording) {
			StopRecording();
		}
	}
}

/// <summary>
/// Recording modes for the input recorder.
/// </summary>
public enum RecordingMode {
	/// <summary>Append new frames to the end of the movie.</summary>
	Append,

	/// <summary>Insert frames at a specific position, shifting existing frames.</summary>
	Insert,

	/// <summary>Overwrite frames starting at a specific position.</summary>
	Overwrite
}

/// <summary>
/// Event arguments for recording events.
/// </summary>
public class RecordingEventArgs : EventArgs {
	/// <summary>Gets the starting frame.</summary>
	public int StartFrame { get; }

	/// <summary>Gets the recording mode.</summary>
	public RecordingMode Mode { get; }

	/// <summary>Gets the number of frames recorded (for stop events).</summary>
	public int FramesRecorded { get; }

	public RecordingEventArgs(int startFrame, RecordingMode mode, int framesRecorded = 0) {
		StartFrame = startFrame;
		Mode = mode;
		FramesRecorded = framesRecorded;
	}
}

/// <summary>
/// Event arguments for frame recorded events.
/// </summary>
public class FrameRecordedEventArgs : EventArgs {
	/// <summary>Gets the frame index.</summary>
	public int FrameIndex { get; }

	/// <summary>Gets the recorded frame data.</summary>
	public InputFrame Frame { get; }

	/// <summary>Gets whether this was a lag frame.</summary>
	public bool IsLagFrame { get; }

	public FrameRecordedEventArgs(int frameIndex, InputFrame frame, bool isLagFrame) {
		FrameIndex = frameIndex;
		Frame = frame;
		IsLagFrame = isLagFrame;
	}
}

/// <summary>
/// Event arguments for rerecord events.
/// </summary>
public class RerecordEventArgs : EventArgs {
	/// <summary>Gets the frame being rerecorded from.</summary>
	public int Frame { get; }

	/// <summary>Gets whether the movie was truncated.</summary>
	public bool Truncated { get; }

	/// <summary>Gets the total rerecord count.</summary>
	public int RerecordCount { get; }

	public RerecordEventArgs(int frame, bool truncated, int rerecordCount) {
		Frame = frame;
		Truncated = truncated;
		RerecordCount = rerecordCount;
	}
}

/// <summary>
/// Represents a branch (saved state of the movie for alternative attempts).
/// </summary>
public class BranchData {
	/// <summary>Gets or sets the branch name.</summary>
	public string Name { get; set; } = "";

	/// <summary>Gets or sets when the branch was created.</summary>
	public DateTime CreatedAt { get; set; }

	/// <summary>Gets or sets the frame count at creation.</summary>
	public int FrameCount { get; set; }

	/// <summary>Gets or sets the rerecord count at creation.</summary>
	public int RerecordCount { get; set; }

	/// <summary>Gets or sets the saved frames.</summary>
	public List<InputFrame> Frames { get; set; } = new();
}
