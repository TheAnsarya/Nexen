using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;
using Nexen.TAS;
using Nexen.ViewModels;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for recording undo action creation and execution.
/// Validates overhead of the new undo tracking during recording sessions.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class RecordingUndoBenchmarks {
	private MovieData _movie = null!;
	private List<InputFrame> _recordedFrames = null!;
	private List<InputFrame> _overwrittenFrames = null!;

	[Params(100, 1000, 10000)]
	public int FrameCount { get; set; }

	[GlobalSetup]
	public void Setup() {
		_movie = new MovieData {
			Author = "Benchmark",
			GameName = "Test Game",
			SystemType = SystemType.Nes,
		};

		var random = new Random(42);

		for (int i = 0; i < 60_000; i++) {
			var frame = new InputFrame(i) {
				Controllers = [new ControllerInput { A = random.NextDouble() > 0.5, B = random.NextDouble() > 0.5 }]
			};
			_movie.InputFrames.Add(frame);
		}

		// Pre-generate frames for recording simulation
		_recordedFrames = new List<InputFrame>(FrameCount);
		for (int i = 0; i < FrameCount; i++) {
			_recordedFrames.Add(new InputFrame(100_000 + i) {
				Controllers = [new ControllerInput { A = true }]
			});
		}

		// Pre-generate overwritten frames for overwrite mode
		_overwrittenFrames = new List<InputFrame>(FrameCount);
		for (int i = 0; i < FrameCount && i < _movie.InputFrames.Count; i++) {
			_overwrittenFrames.Add(_movie.InputFrames[i].Clone());
		}
	}

	[Benchmark(Description = "Create RecordingUndoAction (Append)")]
	[BenchmarkCategory("ActionCreation")]
	public RecordingUndoAction CreateAppendAction() {
		return new RecordingUndoAction(_movie, RecordingMode.Append, 60_000, FrameCount);
	}

	[Benchmark(Description = "Create RecordingUndoAction (Overwrite)")]
	[BenchmarkCategory("ActionCreation")]
	public RecordingUndoAction CreateOverwriteAction() {
		return new RecordingUndoAction(_movie, RecordingMode.Overwrite, 0, FrameCount, _overwrittenFrames);
	}

	[Benchmark(Description = "Create RerecordUndoAction")]
	[BenchmarkCategory("ActionCreation")]
	public RerecordUndoAction CreateRerecordAction() {
		var removed = _movie.InputFrames.GetRange(30_000, FrameCount);
		return new RerecordUndoAction(_movie, 30_000, removed);
	}

	[Benchmark(Description = "Append undo+redo roundtrip")]
	[BenchmarkCategory("UndoRedo")]
	public int AppendUndoRedo() {
		// Simulate recording
		int startIndex = _movie.InputFrames.Count;
		_movie.InputFrames.AddRange(_recordedFrames);

		var action = new RecordingUndoAction(_movie, RecordingMode.Append, startIndex, FrameCount);

		// Undo
		action.Undo();
		int afterUndo = _movie.InputFrames.Count;

		return afterUndo;
	}

	[Benchmark(Description = "Rerecord undo+redo roundtrip")]
	[BenchmarkCategory("UndoRedo")]
	public int RerecordUndoRedo() {
		int truncateFrom = _movie.InputFrames.Count - FrameCount;
		if (truncateFrom < 0) truncateFrom = 0;

		var removed = _movie.InputFrames.GetRange(truncateFrom, _movie.InputFrames.Count - truncateFrom).ToList();
		_movie.InputFrames.RemoveRange(truncateFrom, removed.Count);

		var action = new RerecordUndoAction(_movie, truncateFrom, removed);

		// Undo (restore)
		action.Undo();
		int afterUndo = _movie.InputFrames.Count;

		return afterUndo;
	}

	[Benchmark(Baseline = true, Description = "Clone frames for overwrite snapshot")]
	[BenchmarkCategory("SnapshotOverhead")]
	public List<InputFrame> CloneFramesForOverwrite() {
		var cloned = new List<InputFrame>(FrameCount);
		for (int i = 0; i < FrameCount && i < _movie.InputFrames.Count; i++) {
			cloned.Add(_movie.InputFrames[i].Clone());
		}
		return cloned;
	}

	[Benchmark(Description = "GetRange for rerecord snapshot")]
	[BenchmarkCategory("SnapshotOverhead")]
	public List<InputFrame> GetRangeForRerecord() {
		int start = Math.Max(0, _movie.InputFrames.Count - FrameCount);
		return _movie.InputFrames.GetRange(start, _movie.InputFrames.Count - start);
	}

	[Benchmark(Description = "FrameIndices cached vs uncached (ModifyCommandAction)")]
	[BenchmarkCategory("MinorFixes")]
	public int FrameIndicesAccess() {
		var frames = new List<InputFrame>();
		var indices = new List<int>();
		for (int i = 0; i < Math.Min(FrameCount, _movie.InputFrames.Count); i++) {
			frames.Add(_movie.InputFrames[i]);
			indices.Add(i);
		}

		var action = new ModifyCommandAction(frames, indices, FrameCommand.SoftReset, true);

		// Access FrameIndices multiple times (now cached)
		int sum = 0;
		for (int j = 0; j < 10; j++) {
			sum += action.FrameIndices.Count;
		}

		return sum;
	}
}
