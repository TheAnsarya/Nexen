using System.IO.Compression;
using System.Text;
using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks for full converter read/write operations
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class ConverterBenchmarks {
	private byte[] _nexenMovieData = null!;
	private byte[] _bk2MovieData = null!;
	private byte[] _fm2MovieData = null!;
	private MovieData _movie = null!;

	private Converters.NexenMovieConverter _nexenConverter = null!;
	private Converters.Bk2MovieConverter _bk2Converter = null!;
	private Converters.Fm2MovieConverter _fm2Converter = null!;

	[Params(100, 1000)]
	public int FrameCount { get; set; }

	[GlobalSetup]
	public void Setup() {
		_nexenConverter = new Converters.NexenMovieConverter();
		_bk2Converter = new Converters.Bk2MovieConverter();
		_fm2Converter = new Converters.Fm2MovieConverter();

		// Create test movie
		_movie = new MovieData {
			Author = "Benchmark Author",
			Description = "Benchmark movie for converter testing",
			GameName = "Test Game",
			RomFileName = "test.sfc",
			SystemType = SystemType.Snes,
			Region = RegionType.NTSC,
			ControllerCount = 2,
			RerecordCount = 12345
		};

		for (int i = 0; i < FrameCount; i++) {
			var frame = new InputFrame(i);
			frame.Controllers[0].A = i % 2 == 0;
			frame.Controllers[0].B = i % 3 == 0;
			frame.Controllers[0].Up = i % 5 == 0;
			frame.Controllers[0].Right = i % 7 == 0;
			frame.Controllers[0].Start = i % 100 == 0;
			_movie.AddFrame(frame);
		}

		// Serialize to different formats
		using (var ms = new MemoryStream()) {
			_nexenConverter.Write(_movie, ms);
			_nexenMovieData = ms.ToArray();
		}

		using (var ms = new MemoryStream()) {
			_bk2Converter.Write(_movie, ms);
			_bk2MovieData = ms.ToArray();
		}

		// Create FM2 data (NES format)
		var fm2Movie = _movie.Clone();
		fm2Movie.SystemType = SystemType.Nes;
		using (var ms = new MemoryStream()) {
			_fm2Converter.Write(fm2Movie, ms);
			_fm2MovieData = ms.ToArray();
		}
	}

	[Benchmark(Baseline = true)]
	public MovieData ReadNexen() {
		using var stream = new MemoryStream(_nexenMovieData);
		return _nexenConverter.Read(stream);
	}

	[Benchmark]
	public void WriteNexen() {
		using var stream = new MemoryStream();
		_nexenConverter.Write(_movie, stream);
	}

	[Benchmark]
	public MovieData ReadBk2() {
		using var stream = new MemoryStream(_bk2MovieData);
		return _bk2Converter.Read(stream);
	}

	[Benchmark]
	public void WriteBk2() {
		using var stream = new MemoryStream();
		_bk2Converter.Write(_movie, stream);
	}

	[Benchmark]
	public MovieData ReadFm2() {
		using var stream = new MemoryStream(_fm2MovieData);
		return _fm2Converter.Read(stream);
	}

	[Benchmark]
	public void WriteFm2() {
		using var stream = new MemoryStream();
		var fm2Movie = _movie.Clone();
		fm2Movie.SystemType = SystemType.Nes;
		_fm2Converter.Write(fm2Movie, stream);
	}
}
