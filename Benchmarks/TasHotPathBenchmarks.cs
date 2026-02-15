using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;
using Nexen.MovieConverter.Converters;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for TAS/Movie hot paths.
/// Focuses on operations that happen every frame or frequently during TAS editing.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class TasHotPathBenchmarks {
	// Test data
	private MovieData _smallMovie = null!;   // 1,000 frames
	private MovieData _mediumMovie = null!;  // 60,000 frames (~16 min at 60fps)
	private MovieData _largeMovie = null!;   // 300,000 frames (~83 min at 60fps)

	private ControllerInput[] _testInputs = null!;
	private InputFrame _testFrame = null!;

	[GlobalSetup]
	public void Setup() {
		var random = new Random(42); // Deterministic seed

		_smallMovie = GenerateMovie(1_000, random);
		_mediumMovie = GenerateMovie(60_000, random);
		_largeMovie = GenerateMovie(300_000, random);

		_testInputs = new ControllerInput[4];
		for (int i = 0; i < 4; i++) {
			_testInputs[i] = GenerateRandomInput(random);
		}

		_testFrame = new InputFrame(0);
		for (int i = 0; i < _testFrame.Controllers.Length; i++) {
			_testFrame.Controllers[i] = GenerateRandomInput(random);
		}
	}

	private static MovieData GenerateMovie(int frameCount, Random random) {
		var movie = new MovieData {
			Author = "Benchmark Test",
			GameName = "Test Game",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				IsLagFrame = random.NextDouble() < 0.05 // 5% lag frames
			};

			// Generate realistic button patterns
			for (int c = 0; c < frame.Controllers.Length; c++) {
				frame.Controllers[c] = GenerateRandomInput(random);
			}

			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	private static ControllerInput GenerateRandomInput(Random random) {
		return new ControllerInput {
			A = random.NextDouble() < 0.1,
			B = random.NextDouble() < 0.1,
			X = random.NextDouble() < 0.05,
			Y = random.NextDouble() < 0.05,
			L = random.NextDouble() < 0.02,
			R = random.NextDouble() < 0.02,
			Start = random.NextDouble() < 0.001,
			Select = random.NextDouble() < 0.001,
			Up = random.NextDouble() < 0.15,
			Down = random.NextDouble() < 0.15,
			Left = random.NextDouble() < 0.1,
			Right = random.NextDouble() < 0.2
		};
	}

	#region Input Comparison Benchmarks

	/// <summary>
	/// Benchmark comparing button bits - used for input mismatch detection
	/// </summary>
	[Benchmark(Description = "ButtonBits comparison")]
	[BenchmarkCategory("Input")]
	public bool ButtonBitsComparison() {
		return _testInputs[0].ButtonBits == _testInputs[1].ButtonBits;
	}

	/// <summary>
	/// Benchmark for checking if any button is pressed
	/// </summary>
	[Benchmark(Description = "Any button pressed check")]
	[BenchmarkCategory("Input")]
	public bool AnyButtonPressed() {
		return _testInputs[0].ButtonBits != 0;
	}

	/// <summary>
	/// Benchmark input mismatch check for all controllers
	/// </summary>
	[Benchmark(Description = "Input mismatch detection (4 controllers)")]
	[BenchmarkCategory("Input")]
	public bool InputMismatchCheck() {
		var frameInput = _smallMovie.InputFrames[100];
		bool mismatch = false;
		for (int i = 0; i < _testInputs.Length && i < frameInput.Controllers.Length; i++) {
			if (_testInputs[i].ButtonBits != 0 &&
			    _testInputs[i].ButtonBits != frameInput.Controllers[i].ButtonBits) {
				mismatch = true;
				break;
			}
		}
		return mismatch;
	}

	#endregion

	#region Frame Operations

	/// <summary>
	/// Benchmark adding a frame to the movie
	/// </summary>
	[Benchmark(Description = "Add frame to movie")]
	[BenchmarkCategory("FrameOps")]
	public void AddFrame() {
		_smallMovie.InputFrames.Add(_testFrame);
		_smallMovie.InputFrames.RemoveAt(_smallMovie.InputFrames.Count - 1); // Clean up
	}

	/// <summary>
	/// Benchmark inserting a frame at a specific position
	/// </summary>
	[Benchmark(Description = "Insert frame at position 500")]
	[BenchmarkCategory("FrameOps")]
	public void InsertFrame() {
		_smallMovie.InputFrames.Insert(500, _testFrame);
		_smallMovie.InputFrames.RemoveAt(500); // Clean up
	}

	/// <summary>
	/// Benchmark getting frame by index
	/// </summary>
	[Benchmark(Description = "Get frame by index")]
	[BenchmarkCategory("FrameOps")]
	public InputFrame GetFrame() {
		return _mediumMovie.InputFrames[30000];
	}

	#endregion

	#region Serialization Benchmarks

	/// <summary>
	/// Benchmark serializing movie to Nexen format
	/// </summary>
	[Benchmark(Description = "Serialize 1K frames to Nexen")]
	[BenchmarkCategory("Serialize")]
	public void SerializeSmallMovie() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_smallMovie, stream);
	}

	/// <summary>
	/// Benchmark round-trip for small movie
	/// </summary>
	[Benchmark(Description = "Round-trip 1K frames")]
	[BenchmarkCategory("Serialize")]
	public MovieData RoundTripSmallMovie() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_smallMovie, stream);
		stream.Position = 0;
		return converter.Read(stream);
	}

	#endregion

	#region Controller Input Creation

	/// <summary>
	/// Benchmark creating a new InputFrame
	/// </summary>
	[Benchmark(Description = "Create InputFrame")]
	[BenchmarkCategory("Creation")]
	public InputFrame CreateInputFrame() {
		return new InputFrame(0);
	}

	/// <summary>
	/// Benchmark creating a new ControllerInput
	/// </summary>
	[Benchmark(Description = "Create ControllerInput")]
	[BenchmarkCategory("Creation")]
	public ControllerInput CreateControllerInput() {
		return new ControllerInput {
			A = true,
			B = false,
			Right = true,
			Up = false
		};
	}

	#endregion

	#region Lag Frame Detection

	/// <summary>
	/// Benchmark counting lag frames in small movie
	/// </summary>
	[Benchmark(Description = "Count lag frames (1K)")]
	[BenchmarkCategory("LagFrames")]
	public int CountLagFramesSmall() {
		int count = 0;
		foreach (var frame in _smallMovie.InputFrames) {
			if (frame.IsLagFrame) count++;
		}
		return count;
	}

	/// <summary>
	/// Benchmark counting lag frames in medium movie using LINQ
	/// </summary>
	[Benchmark(Description = "Count lag frames (60K) LINQ")]
	[BenchmarkCategory("LagFrames")]
	public int CountLagFramesMediumLinq() {
		return _mediumMovie.InputFrames.Count(f => f.IsLagFrame);
	}

	/// <summary>
	/// Benchmark counting lag frames in medium movie using foreach
	/// </summary>
	[Benchmark(Description = "Count lag frames (60K) foreach")]
	[BenchmarkCategory("LagFrames")]
	public int CountLagFramesMediumForeach() {
		int count = 0;
		foreach (var frame in _mediumMovie.InputFrames) {
			if (frame.IsLagFrame) count++;
		}
		return count;
	}

	#endregion

	#region Movie Lookup Operations

	/// <summary>
	/// Benchmark finding frames with specific input pattern
	/// </summary>
	[Benchmark(Description = "Find frames with A+B (1K)")]
	[BenchmarkCategory("Lookup")]
	public List<int> FindFramesWithPattern() {
		var results = new List<int>();
		for (int i = 0; i < _smallMovie.InputFrames.Count; i++) {
			var input = _smallMovie.InputFrames[i].Controllers[0];
			if (input.A && input.B) {
				results.Add(i);
			}
		}
		return results;
	}

	#endregion

	#region Truncation Benchmarks (Issue #253 fix)

	// Note: These benchmarks demonstrate the O(n) vs O(n²) difference
	// We use copies to avoid modifying the test data

	/// <summary>
	/// Benchmark truncation using RemoveRange - O(n)
	/// </summary>
	[Benchmark(Description = "Truncate 500 frames - RemoveRange (1K movie)")]
	[BenchmarkCategory("Truncate")]
	public int TruncateWithRemoveRange_Small() {
		var copy = new List<InputFrame>(_smallMovie.InputFrames);
		int truncateAt = 500;
		copy.RemoveRange(truncateAt, copy.Count - truncateAt);
		return copy.Count;
	}

	/// <summary>
	/// Benchmark truncation using RemoveAt loop - O(n²) - OLD BAD APPROACH
	/// </summary>
	[Benchmark(Description = "Truncate 500 frames - RemoveAt loop (1K movie)")]
	[BenchmarkCategory("Truncate")]
	public int TruncateWithRemoveAtLoop_Small() {
		var copy = new List<InputFrame>(_smallMovie.InputFrames);
		int truncateAt = 500;
		int removeCount = copy.Count - truncateAt;
		for (int i = 0; i < removeCount; i++) {
			copy.RemoveAt(truncateAt);
		}
		return copy.Count;
	}

	/// <summary>
	/// Benchmark truncation using RemoveRange on medium movie - O(n)
	/// </summary>
	[Benchmark(Description = "Truncate 30K frames - RemoveRange (60K movie)")]
	[BenchmarkCategory("Truncate")]
	public int TruncateWithRemoveRange_Medium() {
		var copy = new List<InputFrame>(_mediumMovie.InputFrames);
		int truncateAt = 30_000;
		copy.RemoveRange(truncateAt, copy.Count - truncateAt);
		return copy.Count;
	}

	/// <summary>
	/// Benchmark truncation using RemoveAt loop on medium movie - O(n²) - OLD BAD APPROACH
	/// WARNING: This is intentionally slow to demonstrate the performance difference
	/// </summary>
	[Benchmark(Description = "Truncate 30K frames - RemoveAt loop (60K movie)")]
	[BenchmarkCategory("Truncate")]
	public int TruncateWithRemoveAtLoop_Medium() {
		var copy = new List<InputFrame>(_mediumMovie.InputFrames);
		int truncateAt = 30_000;
		int removeCount = copy.Count - truncateAt;
		for (int i = 0; i < removeCount; i++) {
			copy.RemoveAt(truncateAt);
		}
		return copy.Count;
	}

	/// <summary>
	/// Benchmark truncation using RemoveRange on large movie - O(n)
	/// </summary>
	[Benchmark(Description = "Truncate 150K frames - RemoveRange (300K movie)")]
	[BenchmarkCategory("Truncate")]
	public int TruncateWithRemoveRange_Large() {
		var copy = new List<InputFrame>(_largeMovie.InputFrames);
		int truncateAt = 150_000;
		copy.RemoveRange(truncateAt, copy.Count - truncateAt);
		return copy.Count;
	}

	// Intentionally NOT including RemoveAt loop for large movie - it would take minutes

	#endregion

	#region ViewModel Update Benchmarks

	// Simulated ViewModel objects for benchmarking
	private List<SimulatedFrameViewModel> _smallVmList = null!;
	private List<SimulatedFrameViewModel> _mediumVmList = null!;
	private List<SimulatedFrameViewModel> _largeVmList = null!;

	[GlobalSetup(Target = nameof(FullRebuild_Small))]
	public void SetupViewModelBenchmarks_Small() => _smallVmList = CreateViewModelList(1_000);

	[GlobalSetup(Target = nameof(FullRebuild_Medium))]
	public void SetupViewModelBenchmarks_Medium() => _mediumVmList = CreateViewModelList(60_000);

	[GlobalSetup(Target = nameof(FullRebuild_Large))]
	public void SetupViewModelBenchmarks_Large() => _largeVmList = CreateViewModelList(300_000);

	[GlobalSetup(Targets = [nameof(SingleFrameRefresh_Small), nameof(IncrementalInsert_Small), nameof(IncrementalDelete_Small)])]
	public void SetupSingleFrame_Small() => _smallVmList = CreateViewModelList(1_000);

	[GlobalSetup(Targets = [nameof(SingleFrameRefresh_Medium), nameof(IncrementalInsert_Medium), nameof(IncrementalDelete_Medium)])]
	public void SetupSingleFrame_Medium() => _mediumVmList = CreateViewModelList(60_000);

	[GlobalSetup(Targets = [nameof(SingleFrameRefresh_Large), nameof(IncrementalInsert_Large), nameof(IncrementalDelete_Large)])]
	public void SetupSingleFrame_Large() => _largeVmList = CreateViewModelList(300_000);

	private List<SimulatedFrameViewModel> CreateViewModelList(int count) {
		var list = new List<SimulatedFrameViewModel>(count);
		for (int i = 0; i < count; i++) {
			list.Add(new SimulatedFrameViewModel(i));
		}
		return list;
	}

	/// <summary>
	/// Benchmark full rebuild of ViewModel collection - O(n) - OLD APPROACH
	/// </summary>
	[Benchmark(Description = "Full rebuild 1K VMs - Clear+Add (OLD)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int FullRebuild_Small() {
		_smallVmList.Clear();
		for (int i = 0; i < 1_000; i++) {
			_smallVmList.Add(new SimulatedFrameViewModel(i));
		}
		return _smallVmList.Count;
	}

	/// <summary>
	/// Benchmark single frame refresh - O(1) - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Single frame refresh (1K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int SingleFrameRefresh_Small() {
		// Simulate RefreshFrameAt(500) - just trigger property changes on one item
		_smallVmList[500].RefreshFromFrame();
		return 1;
	}

	/// <summary>
	/// Benchmark full rebuild of ViewModel collection - O(n) - OLD APPROACH
	/// </summary>
	[Benchmark(Description = "Full rebuild 60K VMs - Clear+Add (OLD)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int FullRebuild_Medium() {
		_mediumVmList.Clear();
		for (int i = 0; i < 60_000; i++) {
			_mediumVmList.Add(new SimulatedFrameViewModel(i));
		}
		return _mediumVmList.Count;
	}

	/// <summary>
	/// Benchmark single frame refresh - O(1) - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Single frame refresh (60K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int SingleFrameRefresh_Medium() {
		_mediumVmList[30_000].RefreshFromFrame();
		return 1;
	}

	/// <summary>
	/// Benchmark full rebuild of ViewModel collection - O(n) - OLD APPROACH
	/// </summary>
	[Benchmark(Description = "Full rebuild 300K VMs - Clear+Add (OLD)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int FullRebuild_Large() {
		_largeVmList.Clear();
		for (int i = 0; i < 300_000; i++) {
			_largeVmList.Add(new SimulatedFrameViewModel(i));
		}
		return _largeVmList.Count;
	}

	/// <summary>
	/// Benchmark single frame refresh - O(1) - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Single frame refresh (300K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int SingleFrameRefresh_Large() {
		_largeVmList[150_000].RefreshFromFrame();
		return 1;
	}

	/// <summary>
	/// Benchmark incremental insert - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental insert (1K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalInsert_Small() {
		int index = 500;
		var vm = new SimulatedFrameViewModel(index);
		_smallVmList.Insert(index, vm);
		for (int i = index + 1; i < _smallVmList.Count; i++) {
			_smallVmList[i].FrameNumber = i + 1;
		}
		// Cleanup for next iteration
		_smallVmList.RemoveAt(index);
		return _smallVmList.Count;
	}

	/// <summary>
	/// Benchmark incremental insert - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental insert (60K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalInsert_Medium() {
		int index = 30_000;
		var vm = new SimulatedFrameViewModel(index);
		_mediumVmList.Insert(index, vm);
		for (int i = index + 1; i < _mediumVmList.Count; i++) {
			_mediumVmList[i].FrameNumber = i + 1;
		}
		_mediumVmList.RemoveAt(index);
		return _mediumVmList.Count;
	}

	/// <summary>
	/// Benchmark incremental insert - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental insert (300K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalInsert_Large() {
		int index = 150_000;
		var vm = new SimulatedFrameViewModel(index);
		_largeVmList.Insert(index, vm);
		for (int i = index + 1; i < _largeVmList.Count; i++) {
			_largeVmList[i].FrameNumber = i + 1;
		}
		_largeVmList.RemoveAt(index);
		return _largeVmList.Count;
	}

	/// <summary>
	/// Benchmark incremental delete - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental delete (1K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalDelete_Small() {
		int index = 500;
		var removed = _smallVmList[index];
		_smallVmList.RemoveAt(index);
		for (int i = index; i < _smallVmList.Count; i++) {
			_smallVmList[i].FrameNumber = i + 1;
		}
		// Restore for next iteration
		_smallVmList.Insert(index, removed);
		return _smallVmList.Count;
	}

	/// <summary>
	/// Benchmark incremental delete - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental delete (60K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalDelete_Medium() {
		int index = 30_000;
		var removed = _mediumVmList[index];
		_mediumVmList.RemoveAt(index);
		for (int i = index; i < _mediumVmList.Count; i++) {
			_mediumVmList[i].FrameNumber = i + 1;
		}
		_mediumVmList.Insert(index, removed);
		return _mediumVmList.Count;
	}

	/// <summary>
	/// Benchmark incremental delete - O(n) for renumber but avoids allocation - NEW APPROACH
	/// </summary>
	[Benchmark(Description = "Incremental delete (300K movie)")]
	[BenchmarkCategory("ViewModelUpdate")]
	public int IncrementalDelete_Large() {
		int index = 150_000;
		var removed = _largeVmList[index];
		_largeVmList.RemoveAt(index);
		for (int i = index; i < _largeVmList.Count; i++) {
			_largeVmList[i].FrameNumber = i + 1;
		}
		_largeVmList.Insert(index, removed);
		return _largeVmList.Count;
	}

	#endregion
}

/// <summary>
/// Simulated FrameViewModel for benchmarking without UI dependencies.
/// </summary>
public sealed class SimulatedFrameViewModel {
	public int FrameNumber { get; set; }
	public bool IsGreenzone { get; set; }
	public string P1Input { get; set; } = "";
	public string P2Input { get; set; } = "";
	public string CommentText { get; set; } = "";

	public SimulatedFrameViewModel(int index) {
		FrameNumber = index + 1;
	}

	public void RefreshFromFrame() {
		// Simulate property change notifications (just touching the properties)
		_ = FrameNumber;
		_ = IsGreenzone;
		_ = P1Input;
		_ = P2Input;
		_ = CommentText;
	}
}
