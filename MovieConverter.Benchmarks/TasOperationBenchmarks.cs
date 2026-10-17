using System.Collections.ObjectModel;
using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks comparing TAS editor operation patterns:
/// full collection rebuild vs incremental updates.
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class TasOperationBenchmarks {
	private MovieData _movie = null!;
	private ObservableCollection<FrameVm> _frames = null!;

	[Params(1000, 5000, 10000)]
	public int FrameCount { get; set; }

	[GlobalSetup]
	public void Setup() {
		_movie = new MovieData {
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < FrameCount; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			_movie.AddFrame(frame);
		}

		_frames = new ObservableCollection<FrameVm>();
		for (int i = 0; i < FrameCount; i++) {
			_frames.Add(new FrameVm(_movie.InputFrames[i], i));
		}
	}

	/// <summary>
	/// Full rebuild: Clear + re-add all VMs (the old UpdateFrames pattern).
	/// </summary>
	[Benchmark(Baseline = true)]
	public int FullRebuild() {
		_frames.Clear();
		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			_frames.Add(new FrameVm(_movie.InputFrames[i], i));
		}
		return _frames.Count;
	}

	/// <summary>
	/// Incremental insert: Insert 10 frames at midpoint, renumber tail.
	/// </summary>
	[Benchmark]
	public int IncrementalInsert_10() {
		int insertAt = FrameCount / 2;
		int count = 10;

		// Insert VMs
		for (int i = 0; i < count; i++) {
			_frames.Insert(insertAt + i, new FrameVm(_movie.InputFrames[insertAt], insertAt + i));
		}

		// Renumber from insertAt + count onward
		for (int i = insertAt + count; i < _frames.Count; i++) {
			_frames[i].FrameNumber = i + 1;
		}

		// Cleanup to restore state for next iteration
		for (int i = 0; i < count; i++) {
			_frames.RemoveAt(insertAt);
		}
		for (int i = insertAt; i < _frames.Count; i++) {
			_frames[i].FrameNumber = i + 1;
		}

		return _frames.Count;
	}

	/// <summary>
	/// Incremental remove: Remove 10 frames at midpoint, renumber tail.
	/// </summary>
	[Benchmark]
	public int IncrementalRemove_10() {
		int removeAt = FrameCount / 2;
		int count = 10;

		// Save removed items for restoration
		var removed = new List<FrameVm>(count);
		for (int i = 0; i < count; i++) {
			removed.Add(_frames[removeAt]);
			_frames.RemoveAt(removeAt);
		}

		// Renumber from removeAt onward
		for (int i = removeAt; i < _frames.Count; i++) {
			_frames[i].FrameNumber = i + 1;
		}

		// Cleanup: restore removed items
		for (int i = 0; i < count; i++) {
			_frames.Insert(removeAt + i, removed[i]);
		}
		for (int i = removeAt; i < _frames.Count; i++) {
			_frames[i].FrameNumber = i + 1;
		}

		return _frames.Count;
	}

	/// <summary>
	/// Single frame modify: O(1) refresh (best case for input editing).
	/// </summary>
	[Benchmark]
	public int SingleFrameRefresh() {
		int idx = FrameCount / 2;
		_frames[idx].FrameNumber = idx + 1; // Simulate refresh
		return _frames[idx].FrameNumber;
	}

	/// <summary>
	/// Truncate: Remove from midpoint to end.
	/// </summary>
	[Benchmark]
	public int TruncateHalf() {
		int truncateAt = FrameCount / 2;
		int removeCount = _frames.Count - truncateAt;

		for (int i = _frames.Count - 1; i >= truncateAt; i--) {
			_frames.RemoveAt(i);
		}

		// Restore
		for (int i = truncateAt; i < FrameCount; i++) {
			_frames.Add(new FrameVm(_movie.InputFrames[i], i));
		}

		return _frames.Count;
	}

	/// <summary>
	/// Sync new frames: Append only new frames (recording pattern).
	/// </summary>
	[Benchmark]
	public int SyncNewFrames_100() {
		int newCount = 100;

		// Simulate adding new frames during recording
		for (int i = 0; i < newCount; i++) {
			_frames.Add(new FrameVm(_movie.InputFrames[0], FrameCount + i));
		}

		// Cleanup
		for (int i = 0; i < newCount; i++) {
			_frames.RemoveAt(_frames.Count - 1);
		}

		return _frames.Count;
	}

	/// <summary>
	/// Paint batch: Refresh only painted frames O(k) vs full rebuild O(n).
	/// Simulates painting 50 frames with per-frame refresh.
	/// </summary>
	[Benchmark]
	public int PaintBatch_50() {
		int paintCount = 50;
		int startFrame = FrameCount / 4;

		// Simulate refreshing only the painted frames
		for (int i = 0; i < paintCount; i++) {
			int idx = startFrame + i;
			_frames[idx].FrameNumber = idx + 1; // Simulate RefreshFrameAt
		}

		return paintCount;
	}
}

/// <summary>
/// Lightweight stand-in for TasFrameViewModel (avoids Avalonia dependency).
/// </summary>
public sealed class FrameVm {
	public InputFrame Frame { get; }
	public int FrameNumber { get; set; }

	public FrameVm(InputFrame frame, int index) {
		Frame = frame;
		FrameNumber = index + 1;
	}
}
