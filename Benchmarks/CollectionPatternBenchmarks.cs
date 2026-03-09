using System.Collections.Frozen;
using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks comparing collection and string patterns used in recent
/// performance optimizations (#590-#593). Validates that FrozenDictionary,
/// TryGetValue, TryAdd, and StringBuilder patterns are faster than the
/// original code they replaced.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public class CollectionPatternBenchmarks {
	// --- Frozen vs Mutable data ---
	private Dictionary<int, string> _mutableDict = null!;
	private FrozenDictionary<int, string> _frozenDict = null!;
	private HashSet<int> _mutableSet = null!;
	private FrozenSet<int> _frozenSet = null!;
	private int[] _lookupKeys = null!;

	// --- TryGetValue vs ContainsKey+indexer data ---
	private Dictionary<string, int> _stringDict = null!;
	private string[] _existingKeys = null!;
	private string[] _mixedKeys = null!;

	// --- TryAdd vs ContainsKey+Add data ---
	private (string Key, int Value)[] _addEntries = null!;

	// --- StringBuilder vs concat data ---
	private short[] _byteCodeData = null!;

	[GlobalSetup]
	public void Setup() {
		// Build dictionaries with 200 entries (typical for platform ID maps, icon caches)
		var dict = new Dictionary<int, string>(200);
		var set = new HashSet<int>(200);
		for (int i = 0; i < 200; i++) {
			dict[i] = $"Value_{i}";
			set.Add(i);
		}
		_mutableDict = dict;
		_frozenDict = dict.ToFrozenDictionary();
		_mutableSet = set;
		_frozenSet = set.ToFrozenSet();

		// Keys: 80% hit, 20% miss (typical real-world pattern)
		var rng = new Random(42);
		_lookupKeys = new int[10000];
		for (int i = 0; i < 10000; i++) {
			_lookupKeys[i] = rng.Next(0, 250);
		}

		// String dictionary for TryGetValue benchmarks (typical label manager)
		_stringDict = new Dictionary<string, int>(500);
		_existingKeys = new string[500];
		for (int i = 0; i < 500; i++) {
			string key = $"Label_{i:D4}";
			_stringDict[key] = i;
			_existingKeys[i] = key;
		}
		_mixedKeys = new string[1000];
		for (int i = 0; i < 1000; i++) {
			_mixedKeys[i] = i < 500 ? _existingKeys[i] : $"Missing_{i:D4}";
		}

		// TryAdd entries: half duplicates
		_addEntries = new (string, int)[200];
		for (int i = 0; i < 200; i++) {
			_addEntries[i] = ($"Key_{i % 100:D3}", i);
		}

		// Byte code data for StringBuilder benchmark (simulates assembler output)
		_byteCodeData = new short[500];
		for (int i = 0; i < 500; i++) {
			if (i % 20 == 0) {
				_byteCodeData[i] = -2; // EndOfLine
			} else {
				_byteCodeData[i] = (short)(i % 256);
			}
		}
	}

	// ===== Frozen vs Mutable Dictionary =====

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("FrozenDict")]
	public int Dictionary_TryGetValue() {
		int found = 0;
		var dict = _mutableDict;
		foreach (int key in _lookupKeys) {
			if (dict.TryGetValue(key, out _))
				found++;
		}
		return found;
	}

	[Benchmark]
	[BenchmarkCategory("FrozenDict")]
	public int FrozenDictionary_TryGetValue() {
		int found = 0;
		var dict = _frozenDict;
		foreach (int key in _lookupKeys) {
			if (dict.TryGetValue(key, out _))
				found++;
		}
		return found;
	}

	// ===== Frozen vs Mutable Set =====

	[Benchmark]
	[BenchmarkCategory("FrozenSet")]
	public int HashSet_Contains() {
		int found = 0;
		var set = _mutableSet;
		foreach (int key in _lookupKeys) {
			if (set.Contains(key))
				found++;
		}
		return found;
	}

	[Benchmark]
	[BenchmarkCategory("FrozenSet")]
	public int FrozenSet_Contains() {
		int found = 0;
		var set = _frozenSet;
		foreach (int key in _lookupKeys) {
			if (set.Contains(key))
				found++;
		}
		return found;
	}

	// ===== TryGetValue vs ContainsKey+Indexer =====

	[Benchmark]
	[BenchmarkCategory("TryGetValue")]
	public int ContainsKey_Then_Indexer() {
		int sum = 0;
		var dict = _stringDict;
		foreach (string key in _mixedKeys) {
			if (dict.ContainsKey(key)) {
				sum += dict[key];
			}
		}
		return sum;
	}

	[Benchmark]
	[BenchmarkCategory("TryGetValue")]
	public int TryGetValue_SingleLookup() {
		int sum = 0;
		var dict = _stringDict;
		foreach (string key in _mixedKeys) {
			if (dict.TryGetValue(key, out int value)) {
				sum += value;
			}
		}
		return sum;
	}

	// ===== TryAdd vs ContainsKey+Add =====

	[Benchmark]
	[BenchmarkCategory("TryAdd")]
	public int ContainsKey_Then_Add() {
		var dict = new Dictionary<string, int>(128);
		int added = 0;
		foreach (var (key, value) in _addEntries) {
			if (!dict.ContainsKey(key)) {
				dict.Add(key, value);
				added++;
			}
		}
		return added;
	}

	[Benchmark]
	[BenchmarkCategory("TryAdd")]
	public int TryAdd_SingleCall() {
		var dict = new Dictionary<string, int>(128);
		int added = 0;
		foreach (var (key, value) in _addEntries) {
			if (dict.TryAdd(key, value))
				added++;
		}
		return added;
	}

	// ===== StringBuilder vs String Concat =====

	[Benchmark]
	[BenchmarkCategory("StringBuilder")]
	public string StringConcat_InLoop() {
		string result = "";
		foreach (short s in _byteCodeData) {
			if (s >= 0) {
				result += s.ToString("X2") + " ";
			} else if (s == -2) {
				result += Environment.NewLine;
			}
		}
		return result;
	}

	[Benchmark]
	[BenchmarkCategory("StringBuilder")]
	public string StringBuilder_InLoop() {
		var sb = new StringBuilder();
		foreach (short s in _byteCodeData) {
			if (s >= 0) {
				sb.Append(s.ToString("X2")).Append(' ');
			} else if (s == -2) {
				sb.Append(Environment.NewLine);
			}
		}
		return sb.ToString();
	}
}
