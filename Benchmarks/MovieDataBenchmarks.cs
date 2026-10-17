using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;
using Nexen.MovieConverter;
using Nexen.MovieConverter.Converters;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for MovieData operations at scale: CRC32 computation,
/// serialization, clone, and truncation on large movies.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class MovieDataBenchmarks {
	private MovieData _small = null!;   // 1,000 frames
	private MovieData _medium = null!;  // 60,000 frames (~16 min)
	private MovieData _large = null!;   // 300,000 frames (~83 min)

	[GlobalSetup]
	public void Setup() {
		var random = new Random(42);
		_small = GenerateMovie(1_000, random);
		_medium = GenerateMovie(60_000, random);
		_large = GenerateMovie(300_000, random);
	}

	private static MovieData GenerateMovie(int frameCount, Random random) {
		var movie = new MovieData {
			Author = "Benchmark",
			GameName = "Test Game",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC,
			ControllerCount = 2
		};

		for (int i = 0; i < frameCount; i++) {
			var frame = new InputFrame(i) {
				IsLagFrame = random.NextDouble() < 0.05
			};
			frame.Controllers[0] = new ControllerInput {
				A = random.NextDouble() < 0.1,
				B = random.NextDouble() < 0.1,
				Up = random.NextDouble() < 0.15,
				Right = random.NextDouble() < 0.2,
				Start = random.NextDouble() < 0.001
			};
			movie.InputFrames.Add(frame);
		}

		return movie;
	}

	#region CRC32

	[Benchmark(Description = "CRC32 1K frames")]
	[BenchmarkCategory("CRC32")]
	public uint Crc32_Small() => _small.CalculateInputCrc32();

	[Benchmark(Description = "CRC32 60K frames")]
	[BenchmarkCategory("CRC32")]
	public uint Crc32_Medium() => _medium.CalculateInputCrc32();

	[Benchmark(Description = "CRC32 300K frames")]
	[BenchmarkCategory("CRC32")]
	public uint Crc32_Large() => _large.CalculateInputCrc32();

	#endregion

	#region Clone

	[Benchmark(Description = "Clone 1K frames")]
	[BenchmarkCategory("Clone")]
	public MovieData Clone_Small() => _small.Clone();

	[Benchmark(Description = "Clone 60K frames")]
	[BenchmarkCategory("Clone")]
	public MovieData Clone_Medium() => _medium.Clone();

	[Benchmark(Description = "Clone 300K frames")]
	[BenchmarkCategory("Clone")]
	public MovieData Clone_Large() => _large.Clone();

	#endregion

	#region Serialization

	[Benchmark(Description = "Serialize 1K frames")]
	[BenchmarkCategory("Serialize")]
	public void Serialize_Small() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_small, stream);
	}

	[Benchmark(Description = "Serialize 60K frames")]
	[BenchmarkCategory("Serialize")]
	public void Serialize_Medium() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_medium, stream);
	}

	[Benchmark(Description = "Round-trip 1K frames")]
	[BenchmarkCategory("RoundTrip")]
	public MovieData RoundTrip_Small() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_small, stream);
		stream.Position = 0;
		return converter.Read(stream);
	}

	[Benchmark(Description = "Round-trip 60K frames")]
	[BenchmarkCategory("RoundTrip")]
	public MovieData RoundTrip_Medium() {
		var converter = new NexenMovieConverter();
		using var stream = new MemoryStream();
		converter.Write(_medium, stream);
		stream.Position = 0;
		return converter.Read(stream);
	}

	#endregion

	#region TruncateAt

	[Benchmark(Description = "TruncateAt middle 60K")]
	[BenchmarkCategory("TruncateAt")]
	public void TruncateAt_Medium() {
		// Clone to avoid destroying benchmark data
		var clone = _medium.Clone();
		clone.TruncateAt(30_000);
	}

	[Benchmark(Description = "TruncateAt near end 60K")]
	[BenchmarkCategory("TruncateAt")]
	public void TruncateAt_NearEnd() {
		var clone = _medium.Clone();
		clone.TruncateAt(59_990);
	}

	#endregion
}
