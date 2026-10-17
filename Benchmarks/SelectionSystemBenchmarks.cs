using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Configs;
using BenchmarkDotNet.Order;

namespace Nexen.Benchmarks;

/// <summary>
/// Benchmarks comparing List&lt;int&gt; vs SortedSet&lt;int&gt; for frame selection.
/// Validates that the SortedSet refactor eliminates redundant Distinct/OrderBy overhead.
/// </summary>
[MemoryDiagnoser]
[Orderer(SummaryOrderPolicy.FastestToSlowest)]
[GroupBenchmarksBy(BenchmarkLogicalGroupRule.ByCategory)]
public class SelectionSystemBenchmarks {
	private List<int> _listSelection = null!;
	private SortedSet<int> _sortedSetSelection = null!;
	private int[] _rawIndices = null!;

	[Params(100, 1000, 10000)]
	public int SelectionSize { get; set; }

	[GlobalSetup]
	public void Setup() {
		// Simulate a selection with some duplicates and unsorted order
		var rng = new Random(42);
		_rawIndices = new int[SelectionSize + SelectionSize / 10];
		for (int i = 0; i < _rawIndices.Length; i++) {
			_rawIndices[i] = rng.Next(0, SelectionSize * 2);
		}

		_listSelection = new List<int>(_rawIndices);
		_sortedSetSelection = new SortedSet<int>(_rawIndices);
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("BuildSorted")]
	public List<int> List_DistinctOrderBy() {
		return _listSelection
			.Where(i => i >= 0 && i < SelectionSize * 2)
			.Distinct()
			.OrderBy(i => i)
			.ToList();
	}

	[Benchmark]
	[BenchmarkCategory("BuildSorted")]
	public List<int> SortedSet_WhereToList() {
		return _sortedSetSelection
			.Where(i => i >= 0 && i < SelectionSize * 2)
			.ToList();
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("BuildDescending")]
	public List<int> List_DistinctOrderByDescending() {
		return _listSelection
			.Where(i => i >= 0 && i < SelectionSize * 2)
			.Distinct()
			.OrderByDescending(i => i)
			.ToList();
	}

	[Benchmark]
	[BenchmarkCategory("BuildDescending")]
	public List<int> SortedSet_ReverseToList() {
		return _sortedSetSelection
			.Where(i => i >= 0 && i < SelectionSize * 2)
			.Reverse()
			.ToList();
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("Contains")]
	public int List_Contains() {
		int count = 0;
		for (int i = 0; i < 1000; i++) {
			if (_listSelection.Contains(i)) count++;
		}
		return count;
	}

	[Benchmark]
	[BenchmarkCategory("Contains")]
	public int SortedSet_Contains() {
		int count = 0;
		for (int i = 0; i < 1000; i++) {
			if (_sortedSetSelection.Contains(i)) count++;
		}
		return count;
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("MinMax")]
	public int List_MinMax_Linq() {
		return _listSelection.Min() + _listSelection.Max();
	}

	[Benchmark]
	[BenchmarkCategory("MinMax")]
	public int SortedSet_MinMax_Property() {
		return _sortedSetSelection.Min + _sortedSetSelection.Max;
	}

	[Benchmark(Baseline = true)]
	[BenchmarkCategory("Create")]
	public List<int> List_FromRange() {
		return Enumerable.Range(0, SelectionSize).ToList();
	}

	[Benchmark]
	[BenchmarkCategory("Create")]
	public SortedSet<int> SortedSet_FromRange() {
		return new SortedSet<int>(Enumerable.Range(0, SelectionSize));
	}
}
