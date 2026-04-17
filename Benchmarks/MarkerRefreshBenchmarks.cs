using System.Collections.ObjectModel;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;
using Nexen.ViewModels;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks comparing full RefreshMarkerEntries (O(n)) vs incremental RefreshMarkerEntryAt (O(k)).
/// Validates that the incremental approach used by the undo system is faster for single-frame updates.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class MarkerRefreshBenchmarks {
	private MovieData _movie = null!;
	private ObservableCollection<TasMarkerEntryViewModel> _markerEntries = null!;
	private int _targetFrameIndex;

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
			var frame = new InputFrame(i);
			// ~1% of frames have markers, ~2% have comments, ~0.5% have TODOs
			if (i % 100 == 0) {
				frame.Comment = $"[M] Marker at frame {i}";
			} else if (i % 50 == 0) {
				frame.Comment = $"Comment at frame {i}";
			} else if (i % 200 == 0) {
				frame.Comment = $"TODO: check frame {i}";
			}
			_movie.InputFrames.Add(frame);
		}

		// Build initial marker entries (simulates RefreshMarkerEntries)
		_markerEntries = new ObservableCollection<TasMarkerEntryViewModel>();
		RebuildMarkerEntries();

		// Target a frame near the middle for incremental update
		_targetFrameIndex = MovieSize / 2;
	}

	[IterationSetup]
	public void IterationSetup() {
		// Ensure marker entries are in a known state before each iteration
		_markerEntries.Clear();
		RebuildMarkerEntries();
	}

	[Benchmark(Baseline = true, Description = "Full rebuild (O(n))")]
	[BenchmarkCategory("MarkerRefresh")]
	public int FullRebuild() {
		_markerEntries.Clear();
		RebuildMarkerEntries();
		return _markerEntries.Count;
	}

	[Benchmark(Description = "Incremental single-frame (O(k))")]
	[BenchmarkCategory("MarkerRefresh")]
	public int IncrementalSingleFrame() {
		// Simulate modifying one frame's comment, then refreshing just that entry
		var frame = _movie.InputFrames[_targetFrameIndex];
		frame.Comment = "[M] Updated marker";
		RefreshMarkerEntryAt(_targetFrameIndex);

		// Restore original state
		frame.Comment = null;
		RefreshMarkerEntryAt(_targetFrameIndex);
		return _markerEntries.Count;
	}

	[Benchmark(Description = "Incremental add comment (O(k))")]
	[BenchmarkCategory("MarkerRefresh")]
	public int IncrementalAddComment() {
		// Add a comment to a frame that had none
		int frameIdx = _targetFrameIndex + 1; // Likely has no comment
		var frame = _movie.InputFrames[frameIdx];
		string? original = frame.Comment;
		frame.Comment = "New comment";
		RefreshMarkerEntryAt(frameIdx);

		// Restore
		frame.Comment = original;
		RefreshMarkerEntryAt(frameIdx);
		return _markerEntries.Count;
	}

	[Benchmark(Description = "Incremental remove comment (O(k))")]
	[BenchmarkCategory("MarkerRefresh")]
	public int IncrementalRemoveComment() {
		// Remove a comment from a frame that had one
		int frameIdx = (_targetFrameIndex / 100) * 100; // Find a frame with a marker
		var frame = _movie.InputFrames[frameIdx];
		string? original = frame.Comment;
		frame.Comment = null;
		RefreshMarkerEntryAt(frameIdx);

		// Restore
		frame.Comment = original;
		RefreshMarkerEntryAt(frameIdx);
		return _markerEntries.Count;
	}

	/// <summary>
	/// Replicates RefreshMarkerEntries logic — full O(n) rebuild.
	/// </summary>
	private void RebuildMarkerEntries() {
		var entries = new List<TasMarkerEntryViewModel>();
		for (int i = 0; i < _movie.InputFrames.Count; i++) {
			string? comment = _movie.InputFrames[i].Comment;
			if (string.IsNullOrWhiteSpace(comment)) {
				continue;
			}

			MarkerEntryType type = TasEditorViewModel.ClassifyMarkerEntryType(comment);
			entries.Add(new TasMarkerEntryViewModel(i, comment, type));
		}

		foreach (var entry in entries) {
			_markerEntries.Add(entry);
		}
	}

	/// <summary>
	/// Replicates RefreshMarkerEntryAt logic — incremental O(k) update.
	/// </summary>
	private void RefreshMarkerEntryAt(int frameIndex) {
		// Remove existing entry for this frame (if any)
		for (int i = _markerEntries.Count - 1; i >= 0; i--) {
			if (_markerEntries[i].FrameIndex == frameIndex) {
				_markerEntries.RemoveAt(i);
				break;
			}
		}

		// Check if the frame now has a comment
		if (frameIndex >= 0 && frameIndex < _movie.InputFrames.Count) {
			string? comment = _movie.InputFrames[frameIndex].Comment;
			if (!string.IsNullOrWhiteSpace(comment)) {
				MarkerEntryType type = TasEditorViewModel.ClassifyMarkerEntryType(comment);
				var entry = new TasMarkerEntryViewModel(frameIndex, comment, type);
				// Insert in sorted position
				int insertIndex = 0;
				for (int i = 0; i < _markerEntries.Count; i++) {
					if (_markerEntries[i].FrameIndex > frameIndex) {
						break;
					}
					insertIndex = i + 1;
				}
				_markerEntries.Insert(insertIndex, entry);
			}
		}
	}
}
