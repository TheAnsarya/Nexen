using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;
using System.Collections.Concurrent;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks proving that our optimizations for issues #261, #262, #265 are faster.
/// Compares old O(n*k) approaches vs new O(n) bulk operations and O(log n) lookups.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class TasOptimizationBenchmarks {
	private List<InputFrame> _frames = null!;
	private List<InputFrame> _insertFrames = null!;
	private ConcurrentDictionary<int, byte[]> _savestates = null!;
	private SortedSet<int> _frameIndex = null!;

	[Params(1_000, 10_000, 100_000)]
	public int MovieSize { get; set; }

	[GlobalSetup]
	public void Setup() {
		var random = new Random(42);
		_frames = new List<InputFrame>(MovieSize);
		for (int i = 0; i < MovieSize; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = random.Next(2) == 1;
			frame.Controllers[0].B = random.Next(2) == 1;
			frame.Controllers[0].Up = random.Next(2) == 1;
			frame.IsLagFrame = random.Next(7) == 0;
			_frames.Add(frame);
		}

		// Frames to insert (100 frames)
		_insertFrames = new List<InputFrame>(100);
		for (int i = 0; i < 100; i++) {
			_insertFrames.Add(new InputFrame(i));
		}

		// Greenzone savestates: one every 60 frames
		_savestates = new ConcurrentDictionary<int, byte[]>();
		_frameIndex = new SortedSet<int>();
		for (int i = 0; i < MovieSize; i += 60) {
			_savestates[i] = new byte[1024];
			_frameIndex.Add(i);
		}
	}

	#region Issue #261: RemoveRange vs RemoveAt Loop

	[Benchmark(Baseline = true, Description = "OLD: RemoveAt loop (100 frames)")]
	[BenchmarkCategory("Delete")]
	public int Delete_RemoveAtLoop() {
		var frames = new List<InputFrame>(_frames); // Copy
		int index = frames.Count / 2;
		int count = Math.Min(100, frames.Count - index);

		for (int i = 0; i < count; i++) {
			frames.RemoveAt(index);
		}

		return frames.Count;
	}

	[Benchmark(Description = "NEW: RemoveRange (100 frames)")]
	[BenchmarkCategory("Delete")]
	public int Delete_RemoveRange() {
		var frames = new List<InputFrame>(_frames); // Copy
		int index = frames.Count / 2;
		int count = Math.Min(100, frames.Count - index);

		frames.RemoveRange(index, count);

		return frames.Count;
	}

	#endregion

	#region Issue #261: InsertRange vs Insert Loop

	[Benchmark(Baseline = true, Description = "OLD: Insert loop (100 frames)")]
	[BenchmarkCategory("Insert")]
	public int Insert_InsertLoop() {
		var frames = new List<InputFrame>(_frames); // Copy
		int index = frames.Count / 2;

		for (int i = 0; i < _insertFrames.Count; i++) {
			frames.Insert(index + i, _insertFrames[i]);
		}

		return frames.Count;
	}

	[Benchmark(Description = "NEW: InsertRange (100 frames)")]
	[BenchmarkCategory("Insert")]
	public int Insert_InsertRange() {
		var frames = new List<InputFrame>(_frames); // Copy
		int index = frames.Count / 2;

		frames.InsertRange(index, _insertFrames);

		return frames.Count;
	}

	#endregion

	#region Issue #261: GetRange vs Skip/Take

	[Benchmark(Baseline = true, Description = "OLD: Skip().Take().ToList()")]
	[BenchmarkCategory("Slice")]
	public int Slice_SkipTake() {
		int index = _frames.Count / 4;
		int count = _frames.Count / 2;

		var result = _frames.Skip(index).Take(count).ToList();
		return result.Count;
	}

	[Benchmark(Description = "NEW: GetRange()")]
	[BenchmarkCategory("Slice")]
	public int Slice_GetRange() {
		int index = _frames.Count / 4;
		int count = _frames.Count / 2;

		var result = _frames.GetRange(index, count);
		return result.Count;
	}

	#endregion

	#region Issue #261: Fork Truncation

	[Benchmark(Baseline = true, Description = "OLD: While RemoveAt(Count-1)")]
	[BenchmarkCategory("Truncate")]
	public int Truncate_WhileLoop() {
		var frames = new List<InputFrame>(_frames);
		int keepCount = frames.Count / 2;

		while (frames.Count > keepCount) {
			frames.RemoveAt(frames.Count - 1);
		}

		return frames.Count;
	}

	[Benchmark(Description = "NEW: RemoveRange")]
	[BenchmarkCategory("Truncate")]
	public int Truncate_RemoveRange() {
		var frames = new List<InputFrame>(_frames);
		int keepCount = frames.Count / 2;

		frames.RemoveRange(keepCount, frames.Count - keepCount);

		return frames.Count;
	}

	#endregion

	#region Issue #262: GetNearestStateFrame - LINQ vs SortedSet

	[Benchmark(Baseline = true, Description = "OLD: LINQ Where+Max")]
	[BenchmarkCategory("NearestFrame")]
	public int NearestFrame_LinqWhereMax() {
		int targetFrame = MovieSize * 3 / 4; // Search at 75% point

		return _savestates.Keys
			.Where(f => f <= targetFrame)
			.DefaultIfEmpty(-1)
			.Max();
	}

	[Benchmark(Description = "NEW: SortedSet.GetViewBetween")]
	[BenchmarkCategory("NearestFrame")]
	public int NearestFrame_SortedSet() {
		int targetFrame = MovieSize * 3 / 4;

		if (_frameIndex.Count == 0) return -1;
		int minFrame = _frameIndex.Min;
		if (minFrame > targetFrame) return -1;
		return _frameIndex.GetViewBetween(minFrame, targetFrame).Max;
	}

	#endregion

	#region Issue #262: LatestFrame - LINQ vs SortedSet

	[Benchmark(Baseline = true, Description = "OLD: Keys.DefaultIfEmpty.Max()")]
	[BenchmarkCategory("LatestFrame")]
	public int LatestFrame_Linq() {
		return _savestates.Keys.DefaultIfEmpty(-1).Max();
	}

	[Benchmark(Description = "NEW: SortedSet.Max")]
	[BenchmarkCategory("LatestFrame")]
	public int LatestFrame_SortedSet() {
		return _frameIndex.Count > 0 ? _frameIndex.Max : -1;
	}

	#endregion

	#region Issue #262: TotalMemoryUsage - LINQ Sum vs Tracked

	private long _trackedMemoryUsage;

	[IterationSetup(Target = nameof(TotalMemory_Tracked))]
	public void SetupTrackedMemory() {
		_trackedMemoryUsage = _savestates.Values.Sum(s => (long)s.Length);
	}

	[Benchmark(Baseline = true, Description = "OLD: Values.Sum(lambda)")]
	[BenchmarkCategory("MemoryUsage")]
	public long TotalMemory_LinqSum() {
		return _savestates.Values.Sum(s => (long)s.Length);
	}

	[Benchmark(Description = "NEW: Interlocked.Read (tracked)")]
	[BenchmarkCategory("MemoryUsage")]
	public long TotalMemory_Tracked() {
		return Interlocked.Read(ref _trackedMemoryUsage);
	}

	#endregion

	#region Issue #265: Clone vs Manual CloneFrame

	[Benchmark(Baseline = true, Description = "OLD: Manual CloneFrame (misses fields)")]
	[BenchmarkCategory("Clone")]
	public InputFrame Clone_ManualCloneFrame() {
		var original = _frames[0];
		var clone = new InputFrame(original.FrameNumber) {
			IsLagFrame = original.IsLagFrame,
			Comment = original.Comment
		};

		// Old CloneInput only copied 12 button fields
		for (int i = 0; i < original.Controllers.Length && i < clone.Controllers.Length; i++) {
			clone.Controllers[i] = new ControllerInput {
				A = original.Controllers[i].A,
				B = original.Controllers[i].B,
				X = original.Controllers[i].X,
				Y = original.Controllers[i].Y,
				L = original.Controllers[i].L,
				R = original.Controllers[i].R,
				Up = original.Controllers[i].Up,
				Down = original.Controllers[i].Down,
				Left = original.Controllers[i].Left,
				Right = original.Controllers[i].Right,
				Start = original.Controllers[i].Start,
				Select = original.Controllers[i].Select
			};
		}

		return clone;
	}

	[Benchmark(Description = "NEW: InputFrame.Clone() (all fields)")]
	[BenchmarkCategory("Clone")]
	public InputFrame Clone_Canonical() {
		return _frames[0].Clone();
	}

	#endregion
}
