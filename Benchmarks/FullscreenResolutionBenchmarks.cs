using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;
using Nexen.Config;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks comparing the old string-parsing approach to the new dictionary lookup
/// for FullscreenResolution GetWidth/GetHeight.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public sealed class FullscreenResolutionBenchmarks {
	private static readonly FullscreenResolution[] _allResolutions = Enum.GetValues<FullscreenResolution>()
		.Where(r => r != FullscreenResolution.Default)
		.ToArray();

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("Resolution")]
	public int GetWidth_StringParse() {
		int sum = 0;
		foreach (var res in _allResolutions) {
			string name = res.ToString();
			sum += Int32.Parse(name.Substring(1, name.IndexOf("x") - 1));
		}
		return sum;
	}

	[Benchmark]
	[BenchmarkCategory("Resolution")]
	public int GetWidth_DictionaryLookup() {
		int sum = 0;
		foreach (var res in _allResolutions) {
			sum += res.GetWidth();
		}
		return sum;
	}

	[Benchmark]
	[BenchmarkCategory("Resolution")]
	public int GetHeight_StringParse() {
		int sum = 0;
		foreach (var res in _allResolutions) {
			string name = res.ToString();
			sum += Int32.Parse(name.Substring(name.IndexOf("x") + 1));
		}
		return sum;
	}

	[Benchmark]
	[BenchmarkCategory("Resolution")]
	public int GetHeight_DictionaryLookup() {
		int sum = 0;
		foreach (var res in _allResolutions) {
			sum += res.GetHeight();
		}
		return sum;
	}
}
