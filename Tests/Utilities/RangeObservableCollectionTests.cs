using System.Collections.Specialized;
using Nexen.Utilities;
using Xunit;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Tests for <see cref="RangeObservableCollection{T}"/>.
/// Verifies that batch operations produce correct contents and fire
/// exactly one <see cref="NotifyCollectionChangedAction.Reset"/> notification.
/// </summary>
public class RangeObservableCollectionTests {
	// ===== AddRange =====

	[Fact]
	public void AddRange_AppendsAllItems() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		Assert.Equal([1, 2, 3], col);
	}

	[Fact]
	public void AddRange_AppendsToExistingItems() {
		var col = new RangeObservableCollection<int>();
		col.Add(0);
		col.AddRange([1, 2, 3]);
		Assert.Equal([0, 1, 2, 3], col);
	}

	[Fact]
	public void AddRange_FiresSingleResetNotification() {
		var col = new RangeObservableCollection<int>();
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Reset, e.Action);
		};
		col.AddRange([1, 2, 3, 4, 5]);
		Assert.Equal(1, eventCount);
	}

	[Fact]
	public void AddRange_EmptyEnumerable_FiresSingleEvent() {
		var col = new RangeObservableCollection<int>();
		int eventCount = 0;
		col.CollectionChanged += (_, _) => eventCount++;
		col.AddRange([]);
		Assert.Equal(1, eventCount);
		Assert.Empty(col);
	}

	// ===== InsertRange =====

	[Fact]
	public void InsertRange_InsertsAtCorrectPosition() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 4, 5]);
		col.InsertRange(1, [2, 3]);
		Assert.Equal([1, 2, 3, 4, 5], col);
	}

	[Fact]
	public void InsertRange_AtStart_PrependsItems() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([3, 4]);
		col.InsertRange(0, [1, 2]);
		Assert.Equal([1, 2, 3, 4], col);
	}

	[Fact]
	public void InsertRange_AtEnd_AppendsItems() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2]);
		col.InsertRange(2, [3, 4]);
		Assert.Equal([1, 2, 3, 4], col);
	}

	[Fact]
	public void InsertRange_FiresSingleResetNotification() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 5]);
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Reset, e.Action);
		};
		col.InsertRange(1, [2, 3, 4]);
		Assert.Equal(1, eventCount);
	}

	[Fact]
	public void InsertRange_InvalidIndex_Throws() {
		var col = new RangeObservableCollection<int> { 1 };
		Assert.Throws<ArgumentOutOfRangeException>(() => col.InsertRange(-1, [2]));
		Assert.Throws<ArgumentOutOfRangeException>(() => col.InsertRange(5, [2]));
	}

	// ===== RemoveRange =====

	[Fact]
	public void RemoveRange_RemovesCorrectItems() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3, 4, 5]);
		col.RemoveRange(1, 3);
		Assert.Equal([1, 5], col);
	}

	[Fact]
	public void RemoveRange_FromStart() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3, 4]);
		col.RemoveRange(0, 2);
		Assert.Equal([3, 4], col);
	}

	[Fact]
	public void RemoveRange_FromEnd() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3, 4]);
		col.RemoveRange(2, 2);
		Assert.Equal([1, 2], col);
	}

	[Fact]
	public void RemoveRange_AllItems() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		col.RemoveRange(0, 3);
		Assert.Empty(col);
	}

	[Fact]
	public void RemoveRange_ZeroCount_NoOp() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		int eventCount = 0;
		col.CollectionChanged += (_, _) => eventCount++;
		col.RemoveRange(1, 0);
		Assert.Equal(0, eventCount);
		Assert.Equal([1, 2, 3], col);
	}

	[Fact]
	public void RemoveRange_FiresSingleResetNotification() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3, 4, 5]);
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Reset, e.Action);
		};
		col.RemoveRange(1, 3);
		Assert.Equal(1, eventCount);
	}

	[Fact]
	public void RemoveRange_InvalidArgs_Throws() {
		var col = new RangeObservableCollection<int> { 1, 2, 3 };
		Assert.Throws<ArgumentOutOfRangeException>(() => col.RemoveRange(-1, 1));
		Assert.Throws<ArgumentOutOfRangeException>(() => col.RemoveRange(0, 5));
	}

	// ===== ReplaceAll =====

	[Fact]
	public void ReplaceAll_ReplacesEntireCollection() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		col.ReplaceAll([4, 5, 6, 7]);
		Assert.Equal([4, 5, 6, 7], col);
	}

	[Fact]
	public void ReplaceAll_EmptyToPopulated() {
		var col = new RangeObservableCollection<int>();
		col.ReplaceAll([1, 2, 3]);
		Assert.Equal([1, 2, 3], col);
	}

	[Fact]
	public void ReplaceAll_PopulatedToEmpty() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		col.ReplaceAll([]);
		Assert.Empty(col);
	}

	[Fact]
	public void ReplaceAll_FiresSingleResetNotification() {
		var col = new RangeObservableCollection<int>();
		col.AddRange([1, 2, 3]);
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Reset, e.Action);
		};
		col.ReplaceAll([4, 5]);
		Assert.Equal(1, eventCount);
	}

	// ===== Single-item operations still work =====

	[Fact]
	public void SingleAdd_StillFiresNormalEvent() {
		var col = new RangeObservableCollection<int>();
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Add, e.Action);
		};
		col.Add(1);
		Assert.Equal(1, eventCount);
	}

	[Fact]
	public void SingleRemoveAt_StillFiresNormalEvent() {
		var col = new RangeObservableCollection<int> { 1, 2, 3 };
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Remove, e.Action);
		};
		col.RemoveAt(1);
		Assert.Equal(1, eventCount);
		Assert.Equal([1, 3], col);
	}

	[Fact]
	public void Clear_StillFiresResetEvent() {
		var col = new RangeObservableCollection<int> { 1, 2, 3 };
		int eventCount = 0;
		col.CollectionChanged += (_, e) => {
			eventCount++;
			Assert.Equal(NotifyCollectionChangedAction.Reset, e.Action);
		};
		col.Clear();
		Assert.Equal(1, eventCount);
		Assert.Empty(col);
	}

	// ===== Large batch =====

	[Fact]
	public void AddRange_LargeBatch_FiresSingleEvent() {
		var col = new RangeObservableCollection<int>();
		int eventCount = 0;
		col.CollectionChanged += (_, _) => eventCount++;

		var items = Enumerable.Range(0, 10_000).ToList();
		col.AddRange(items);

		Assert.Equal(1, eventCount);
		Assert.Equal(10_000, col.Count);
		Assert.Equal(0, col[0]);
		Assert.Equal(9_999, col[^1]);
	}

	[Fact]
	public void ReplaceAll_LargeBatch_FiresSingleEvent() {
		var col = new RangeObservableCollection<int>();
		col.AddRange(Enumerable.Range(0, 5_000));
		int eventCount = 0;
		col.CollectionChanged += (_, _) => eventCount++;

		col.ReplaceAll(Enumerable.Range(5_000, 10_000));

		Assert.Equal(1, eventCount);
		Assert.Equal(10_000, col.Count);
		Assert.Equal(5_000, col[0]);
	}
}
