using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for WatchManager formatting operations.
/// Compares string concatenation vs interpolation for hex value formatting.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public sealed class WatchFormatBenchmarks {
	private static readonly Int64[] _testValues = [0, 1, 0x0f, 0xff, 0x00ff, 0xffff, 0x00ffffff, 0x7fffffff];
	private static readonly int[] _byteLengths = [1, 2, 3, 4];

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("HexFormat")]
	public string[] FormatHex_StringConcat() {
		var results = new string[_testValues.Length * _byteLengths.Length];
		int idx = 0;
		foreach (var value in _testValues) {
			foreach (var byteLen in _byteLengths) {
				results[idx++] = "$" + value.ToString("X" + (byteLen * 2));
			}
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("HexFormat")]
	public string[] FormatHex_Interpolation() {
		var results = new string[_testValues.Length * _byteLengths.Length];
		int idx = 0;
		foreach (var value in _testValues) {
			foreach (var byteLen in _byteLengths) {
				results[idx++] = $"${value.ToString("x" + (byteLen * 2))}";
			}
		}
		return results;
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("RemoveWatch")]
	public List<string> RemoveWatch_LinqWhere() {
		var entries = new List<string>(Enumerable.Range(0, 100).Select(i => $"Watch{i}"));
		var indexes = new HashSet<int>([5, 15, 25, 50, 75, 99]);
		return entries.Where((el, index) => !indexes.Contains(index)).ToList();
	}

	[Benchmark]
	[BenchmarkCategory("RemoveWatch")]
	public List<string> RemoveWatch_InPlace() {
		var entries = new List<string>(Enumerable.Range(0, 100).Select(i => $"Watch{i}"));
		var indexes = new HashSet<int>([5, 15, 25, 50, 75, 99]);
		for (int i = entries.Count - 1; i >= 0; i--) {
			if (indexes.Contains(i)) {
				entries.RemoveAt(i);
			}
		}
		return entries;
	}
}
