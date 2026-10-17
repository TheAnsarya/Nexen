using System.Buffers;
using Avalonia.Media;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Order;
using Nexen.Utilities;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks for UI performance optimizations.
/// Measures the impact of caching strategies for ColorHelper and collection allocations.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
public sealed class UiPerformanceBenchmarks {
	private Color[] _testColors = null!;
	private readonly Random _random = new(42);

	[GlobalSetup]
	public void Setup() {
		// Generate a set of test colors similar to what would be used in a hex editor
		_testColors = new Color[256];
		for (int i = 0; i < _testColors.Length; i++) {
			_testColors[i] = Color.FromRgb(
				(byte)_random.Next(256),
				(byte)_random.Next(256),
				(byte)_random.Next(256)
			);
		}
	}

	[Benchmark]
	[BenchmarkCategory("ColorHelper")]
	public SolidColorBrush[] GetBrushUncached() {
		// Simulate what the old code did - create new brushes every time
		var brushes = new SolidColorBrush[_testColors.Length];
		for (int i = 0; i < _testColors.Length; i++) {
			brushes[i] = new SolidColorBrush(_testColors[i]);
		}
		return brushes;
	}

	[Benchmark]
	[BenchmarkCategory("ColorHelper")]
	public SolidColorBrush[] GetBrushCached() {
		// Use the cached version
		var brushes = new SolidColorBrush[_testColors.Length];
		for (int i = 0; i < _testColors.Length; i++) {
			brushes[i] = ColorHelper.GetBrush(_testColors[i]);
		}
		return brushes;
	}

	[Benchmark]
	[BenchmarkCategory("ColorHelper")]
	public SolidColorBrush[] GetBrushCachedRepeated() {
		// Test with repeated colors (simulates actual usage where same colors appear multiple times)
		var brushes = new SolidColorBrush[1000];
		for (int i = 0; i < 1000; i++) {
			// Use only first 16 colors to simulate repeated patterns
			brushes[i] = ColorHelper.GetBrush(_testColors[i % 16]);
		}
		return brushes;
	}

	[Benchmark]
	[BenchmarkCategory("Collections")]
	public List<int> AllocateListEveryFrame() {
		// Simulate allocating a new list every frame
		var list = new List<int>(512);
		for (int i = 0; i < 512; i++) {
			list.Add(i);
		}
		return list;
	}

	// Reusable list for benchmark
	private readonly List<int> _reusableList = new(512);

	[Benchmark]
	[BenchmarkCategory("Collections")]
	public List<int> ReuseListAcrossFrames() {
		// Simulate reusing a list across frames
		_reusableList.Clear();
		for (int i = 0; i < 512; i++) {
			_reusableList.Add(i);
		}
		return _reusableList;
	}

	[Benchmark]
	[BenchmarkCategory("HashSet")]
	public HashSet<Color> AllocateHashSetEveryFrame() {
		// Simulate allocating a new HashSet every frame (like old HexEditor)
		var set = new HashSet<Color>(32);
		for (int i = 0; i < 256; i++) {
			set.Add(_testColors[i % 32]); // Only 32 unique colors
		}
		return set;
	}

	private readonly HashSet<Color> _reusableHashSet = new(32);

	[Benchmark]
	[BenchmarkCategory("HashSet")]
	public HashSet<Color> ReuseHashSetAcrossFrames() {
		// Simulate reusing a HashSet across frames
		_reusableHashSet.Clear();
		for (int i = 0; i < 256; i++) {
			_reusableHashSet.Add(_testColors[i % 32]);
		}
		return _reusableHashSet;
	}

	[Benchmark]
	[BenchmarkCategory("Dictionary")]
	public Dictionary<int, List<int>> AllocateDictionaryWithLists() {
		// Simulate the old _textFragments pattern - new dictionary and lists every frame
		var dict = new Dictionary<int, List<int>>(64);
		for (int i = 0; i < 64; i++) {
			dict[i] = new List<int>(16);
			for (int j = 0; j < 16; j++) {
				dict[i].Add(j);
			}
		}
		return dict;
	}

	private readonly Dictionary<int, List<int>> _reusableDict = new(64);
	private readonly List<List<int>> _listPool = new(64);

	[Benchmark]
	[BenchmarkCategory("Dictionary")]
	public Dictionary<int, List<int>> ReuseDictionaryWithPooledLists() {
		// Return lists to pool
		foreach (var kvp in _reusableDict) {
			kvp.Value.Clear();
			_listPool.Add(kvp.Value);
		}
		_reusableDict.Clear();

		// Reuse from pool
		for (int i = 0; i < 64; i++) {
			List<int> list;
			if (_listPool.Count > 0) {
				list = _listPool[^1];
				_listPool.RemoveAt(_listPool.Count - 1);
			} else {
				list = new List<int>(16);
			}
			_reusableDict[i] = list;
			for (int j = 0; j < 16; j++) {
				list.Add(j);
			}
		}
		return _reusableDict;
	}
}
