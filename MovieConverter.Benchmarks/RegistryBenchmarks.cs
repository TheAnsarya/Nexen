using BenchmarkDotNet.Attributes;

namespace Nexen.MovieConverter.Benchmarks;

/// <summary>
/// Benchmarks for MovieConverterRegistry lookup operations
/// </summary>
[MemoryDiagnoser]
[SimpleJob]
public class RegistryBenchmarks {
	[Benchmark(Baseline = true)]
	public IMovieConverter? GetConverterByFormat_Nexen() =>
		MovieConverterRegistry.GetConverter(MovieFormat.Nexen);

	[Benchmark]
	public IMovieConverter? GetConverterByFormat_Bk2() =>
		MovieConverterRegistry.GetConverter(MovieFormat.Bk2);

	[Benchmark]
	public IMovieConverter? GetConverterByExtension_NexenMovie() =>
		MovieConverterRegistry.GetConverterByExtension(".nexen-movie");

	[Benchmark]
	public IMovieConverter? GetConverterByExtension_Bk2() =>
		MovieConverterRegistry.GetConverterByExtension(".bk2");

	[Benchmark]
	public IMovieConverter? GetConverterByExtension_WithoutDot() =>
		MovieConverterRegistry.GetConverterByExtension("fm2");

	[Benchmark]
	public MovieFormat DetectFormat() =>
		MovieConverterRegistry.DetectFormat("C:\\Movies\\test.lsmv");

	[Benchmark]
	public IReadOnlyList<IMovieConverter> GetAllConverters() =>
		MovieConverterRegistry.Converters;

	[Benchmark]
	public int GetReadableFormats() =>
		MovieConverterRegistry.GetReadableFormats().Count();

	[Benchmark]
	public int GetWritableFormats() =>
		MovieConverterRegistry.GetWritableFormats().Count();

	[Benchmark]
	public string GetOpenFileFilter() =>
		MovieConverterRegistry.GetOpenFileFilter();
}
