using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks for MovieData operations
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class MovieDataBenchmarks {
	private MovieData _movie = null!;
	private InputFrame _frameToAdd = null!;

	[Params(100, 1000, 10000)]
	public int FrameCount { get; set; }

	[GlobalSetup]
	public void Setup() {
		_movie = new MovieData {
			Author = "Benchmark Author",
			Description = "Benchmark movie for performance testing",
			GameName = "Test Game",
			RomFileName = "test.sfc",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC
		};

		// Pre-populate with frames
		for (int i = 0; i < FrameCount; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			_movie.AddFrame(frame);
		}

		_frameToAdd = new InputFrame(FrameCount);
		_frameToAdd.Controllers[0].Start = true;
	}

	[Benchmark(Baseline = true)]
	public int GetTotalFrames() => _movie.TotalFrames;

	[Benchmark]
	public InputFrame? GetFrame() => _movie.GetFrame(FrameCount / 2);

	[Benchmark]
	public void AddFrame() {
		_movie.AddFrame(_frameToAdd);
		// Remove to keep frame count stable
		_movie.InputFrames.RemoveAt(_movie.InputFrames.Count - 1);
	}

	[Benchmark]
	public double GetDuration() => _movie.Duration.TotalSeconds;

	[Benchmark]
	public double GetFrameRate() => _movie.FrameRate;

	[Benchmark]
	public MovieData Clone() => _movie.Clone();
}
