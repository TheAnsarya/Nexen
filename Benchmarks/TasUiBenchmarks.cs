using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for TAS UI operations: multi-frame selection, search, clipboard, undo/redo.
/// Exercises the ViewModel-level logic paths that drive the TAS Editor UI.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class TasUiBenchmarks {
	private MovieData _movie = null!;
	private List<InputFrame> _clipboard = null!;
	private List<int> _selectedIndices = null!;

	[Params(1_000, 10_000, 100_000)]
	public int MovieSize { get; set; }

	[GlobalSetup]
	public void Setup() {
		var random = new Random(42);
		_movie = new MovieData {
			Author = "Benchmark",
			GameName = "Test",
			SystemType = SystemType.Nes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < MovieSize; i++) {
			var frame = new InputFrame(i) {
				IsLagFrame = random.NextDouble() < 0.05,
				Comment = i % 100 == 0 ? $"Marker at frame {i}" : null
			};
			frame.Controllers[0].A = random.NextDouble() < 0.3;
			frame.Controllers[0].B = random.NextDouble() < 0.2;
			frame.Controllers[0].Up = random.NextDouble() < 0.4;
			frame.Controllers[0].Down = random.NextDouble() < 0.1;
			frame.Controllers[0].Left = random.NextDouble() < 0.15;
			frame.Controllers[0].Right = random.NextDouble() < 0.15;
			_movie.InputFrames.Add(frame);
		}

		// Prepare clipboard (100 frames)
		_clipboard = new List<InputFrame>(100);
		for (int i = 0; i < 100; i++) {
			_clipboard.Add(_movie.InputFrames[i].Clone());
		}

		// Prepare multi-selection (every 10th frame in first 1000)
		_selectedIndices = new List<int>();
		for (int i = 0; i < Math.Min(1000, MovieSize); i += 10) {
			_selectedIndices.Add(i);
		}
	}

	#region Multi-Frame Selection

	[Benchmark(Description = "Select range (1000 frames contiguous)")]
	[BenchmarkCategory("Selection")]
	public List<int> Selection_ContiguousRange() {
		int start = MovieSize / 4;
		int count = Math.Min(1000, MovieSize - start);
		var selected = new List<int>(count);
		for (int i = start; i < start + count; i++) {
			selected.Add(i);
		}
		return selected;
	}

	[Benchmark(Description = "Select scattered (100 non-contiguous)")]
	[BenchmarkCategory("Selection")]
	public List<int> Selection_Scattered() {
		var selected = new List<int>(100);
		int step = MovieSize / 100;
		for (int i = 0; i < MovieSize; i += step) {
			selected.Add(i);
		}
		return selected;
	}

	#endregion

	#region Frame Search

	[Benchmark(Description = "Search by button state (A pressed)")]
	[BenchmarkCategory("Search")]
	public List<int> Search_ButtonState() {
		var matches = new List<int>();
		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			if (_movie.InputFrames[i].Controllers[0].A) {
				matches.Add(i);
			}
		}
		return matches;
	}

	[Benchmark(Description = "Search by comment text")]
	[BenchmarkCategory("Search")]
	public List<int> Search_CommentText() {
		var matches = new List<int>();
		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			if (_movie.InputFrames[i].Comment is not null
				&& _movie.InputFrames[i].Comment!.Contains("Marker", StringComparison.OrdinalIgnoreCase)) {
				matches.Add(i);
			}
		}
		return matches;
	}

	[Benchmark(Description = "Search lag frames")]
	[BenchmarkCategory("Search")]
	public List<int> Search_LagFrames() {
		var matches = new List<int>();
		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			if (_movie.InputFrames[i].IsLagFrame) {
				matches.Add(i);
			}
		}
		return matches;
	}

	#endregion

	#region Clipboard Operations

	[Benchmark(Description = "Copy 100 frames (clone)")]
	[BenchmarkCategory("Clipboard")]
	public List<InputFrame> Clipboard_Copy100() {
		int start = MovieSize / 2;
		var clip = new List<InputFrame>(100);
		for (int i = start; i < start + 100 && i < _movie.InputFrames.Count; i++) {
			clip.Add(_movie.InputFrames[i].Clone());
		}
		return clip;
	}

	[Benchmark(Description = "Paste 100 frames (insert)")]
	[BenchmarkCategory("Clipboard")]
	public int Clipboard_Paste100() {
		var frames = new List<InputFrame>(_movie.InputFrames);
		int insertAt = frames.Count / 2;
		frames.InsertRange(insertAt, _clipboard);
		return frames.Count;
	}

	[Benchmark(Description = "Copy 1000 frames (clone)")]
	[BenchmarkCategory("Clipboard")]
	public List<InputFrame> Clipboard_Copy1000() {
		int start = 0;
		int count = Math.Min(1000, _movie.InputFrames.Count);
		var clip = new List<InputFrame>(count);
		for (int i = start; i < count; i++) {
			clip.Add(_movie.InputFrames[i].Clone());
		}
		return clip;
	}

	#endregion

	#region Bulk Delete

	[Benchmark(Description = "Bulk delete 100 scattered frames")]
	[BenchmarkCategory("BulkDelete")]
	public int BulkDelete_Scattered() {
		var frames = new List<InputFrame>(_movie.InputFrames);
		// Remove from end to avoid index shifting
		var toRemove = _selectedIndices.Where(i => i < frames.Count).OrderByDescending(i => i).ToList();
		foreach (int idx in toRemove) {
			frames.RemoveAt(idx);
		}
		return frames.Count;
	}

	[Benchmark(Description = "Bulk delete 100 contiguous frames (RemoveRange)")]
	[BenchmarkCategory("BulkDelete")]
	public int BulkDelete_Contiguous() {
		var frames = new List<InputFrame>(_movie.InputFrames);
		int start = frames.Count / 2;
		int count = Math.Min(100, frames.Count - start);
		frames.RemoveRange(start, count);
		return frames.Count;
	}

	#endregion

	#region Undo/Redo Simulation

	[Benchmark(Description = "Undo/Redo 1000 single-frame toggles")]
	[BenchmarkCategory("UndoRedo")]
	public int UndoRedo_1000Toggles() {
		// Simulate undo stack with value toggles
		var undoStack = new Stack<(int frame, bool oldValue)>(1000);
		int count = Math.Min(1000, _movie.InputFrames.Count);

		// Forward: toggle A buttons
		for (int i = 0; i < count; i++) {
			bool old = _movie.InputFrames[i].Controllers[0].A;
			undoStack.Push((i, old));
			_movie.InputFrames[i].Controllers[0].A = !old;
		}

		// Undo all
		while (undoStack.Count > 0) {
			var (frame, oldVal) = undoStack.Pop();
			_movie.InputFrames[frame].Controllers[0].A = oldVal;
		}

		return count;
	}

	#endregion

	#region Auto-Save Simulation

	[Benchmark(Description = "Serialize movie frame data (snapshot)")]
	[BenchmarkCategory("AutoSave")]
	public byte[] AutoSave_Snapshot() {
		// Simulate serializing all frame data to a byte array
		int bytesPerFrame = 4 * 2; // 4 controllers, 2 bytes each (NES)
		var buffer = new byte[_movie.InputFrames.Count * bytesPerFrame + 64]; // +header
		int offset = 64; // Skip header space

		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			var ctrl = _movie.InputFrames[i].Controllers[0];
			byte b = 0;
			if (ctrl.A) b |= 0x01;
			if (ctrl.B) b |= 0x02;
			if (ctrl.Up) b |= 0x10;
			if (ctrl.Down) b |= 0x20;
			if (ctrl.Left) b |= 0x40;
			if (ctrl.Right) b |= 0x80;
			buffer[offset++] = b;
			buffer[offset++] = 0; // Second byte
		}

		return buffer;
	}

	#endregion
}
