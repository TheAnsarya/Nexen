using System.Runtime.InteropServices;
using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;
using Nexen.Utilities;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for Utf8Utilities string conversion operations.
/// These are called frequently during interop with the native C++ core,
/// especially for debugger label resolution and log display.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public sealed class Utf8UtilitiesBenchmarks
{
	private byte[][] _shortStrings = null!;
	private byte[][] _longStrings = null!;
	private byte[][] _nullTerminated = null!;

	[GlobalSetup]
	public void Setup()
	{
		// Short strings (typical label names: 5-20 chars)
		_shortStrings = new byte[1000][];
		for (int i = 0; i < 1000; i++) {
			string s = $"Label_{i:D4}";
			_shortStrings[i] = Encoding.UTF8.GetBytes(s);
		}

		// Long strings (disassembly lines: 50-100 chars)
		_longStrings = new byte[1000][];
		for (int i = 0; i < 1000; i++) {
			string s = $"LDA $2000,X ; Load accumulator with value from PPU address table index {i}";
			_longStrings[i] = Encoding.UTF8.GetBytes(s);
		}

		// Null-terminated (typical from native interop)
		_nullTerminated = new byte[1000][];
		for (int i = 0; i < 1000; i++) {
			string s = $"NativeStr_{i:D4}";
			var bytes = Encoding.UTF8.GetBytes(s);
			_nullTerminated[i] = new byte[bytes.Length + 16]; // Extra padding after null
			Array.Copy(bytes, _nullTerminated[i], bytes.Length);
			// Rest is zeros (null terminated)
		}
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("GetString")]
	public string[] GetStringFromArray_ShortStrings()
	{
		var results = new string[1000];
		for (int i = 0; i < 1000; i++) {
			results[i] = Utf8Utilities.GetStringFromArray(_shortStrings[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("GetString")]
	public string[] GetStringFromArray_LongStrings()
	{
		var results = new string[1000];
		for (int i = 0; i < 1000; i++) {
			results[i] = Utf8Utilities.GetStringFromArray(_longStrings[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("GetString")]
	public string[] GetStringFromArray_NullTerminated()
	{
		var results = new string[1000];
		for (int i = 0; i < 1000; i++) {
			results[i] = Utf8Utilities.GetStringFromArray(_nullTerminated[i]);
		}
		return results;
	}

	[Benchmark]
	[BenchmarkCategory("GetString")]
	public string GetStringFromArray_VsEncodingGetString_Short()
	{
		// Compare against direct Encoding.UTF8.GetString for non-null-terminated case
		return Encoding.UTF8.GetString(_shortStrings[0]);
	}

	[Benchmark]
	[BenchmarkCategory("GetString")]
	public string GetStringFromArray_Custom_Short()
	{
		return Utf8Utilities.GetStringFromArray(_shortStrings[0]);
	}
}
