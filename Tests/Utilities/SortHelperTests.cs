using Xunit;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using Nexen.Utilities;

namespace Nexen.Tests.Utilities;

/// <summary>
/// Unit tests for <see cref="SortHelper"/> multi-column sorting logic.
/// Tests correct sort ordering, direction, and fallback behavior.
/// </summary>
public class SortHelperTests
{
	// Test record with multiple sortable fields
	private record TestItem(string Name, int Age, double Score);

	private static Dictionary<string, Func<TestItem, TestItem, int>> CreateComparers()
	{
		return new Dictionary<string, Func<TestItem, TestItem, int>> {
			["Name"] = (a, b) => string.Compare(a.Name, b.Name, StringComparison.Ordinal),
			["Age"] = (a, b) => a.Age.CompareTo(b.Age),
			["Score"] = (a, b) => a.Score.CompareTo(b.Score)
		};
	}

	#region SortArray Tests

	[Fact]
	public void SortArray_SingleColumnAscending_SortsByName()
	{
		var items = new[] {
			new TestItem("Charlie", 30, 85.0),
			new TestItem("Alice", 25, 90.0),
			new TestItem("Bob", 35, 80.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal("Alice", items[0].Name);
		Assert.Equal("Bob", items[1].Name);
		Assert.Equal("Charlie", items[2].Name);
	}

	[Fact]
	public void SortArray_SingleColumnDescending_SortsByNameReversed()
	{
		var items = new[] {
			new TestItem("Alice", 25, 90.0),
			new TestItem("Charlie", 30, 85.0),
			new TestItem("Bob", 35, 80.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Descending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal("Charlie", items[0].Name);
		Assert.Equal("Bob", items[1].Name);
		Assert.Equal("Alice", items[2].Name);
	}

	[Fact]
	public void SortArray_TwoColumns_PrimarySortThenSecondary()
	{
		var items = new[] {
			new TestItem("Alice", 30, 90.0),
			new TestItem("Bob", 25, 80.0),
			new TestItem("Alice", 25, 85.0),
			new TestItem("Bob", 30, 95.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending),
			Tuple.Create("Age", ListSortDirection.Ascending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal("Alice", items[0].Name);
		Assert.Equal(25, items[0].Age);
		Assert.Equal("Alice", items[1].Name);
		Assert.Equal(30, items[1].Age);
		Assert.Equal("Bob", items[2].Name);
		Assert.Equal(25, items[2].Age);
		Assert.Equal("Bob", items[3].Name);
		Assert.Equal(30, items[3].Age);
	}

	[Fact]
	public void SortArray_ThreeColumns_AllLevelsUsed()
	{
		var items = new[] {
			new TestItem("Alice", 25, 90.0),
			new TestItem("Alice", 25, 80.0),
			new TestItem("Alice", 25, 85.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending),
			Tuple.Create("Age", ListSortDirection.Ascending),
			Tuple.Create("Score", ListSortDirection.Descending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		// Same name and age → sort by score descending
		Assert.Equal(90.0, items[0].Score);
		Assert.Equal(85.0, items[1].Score);
		Assert.Equal(80.0, items[2].Score);
	}

	[Fact]
	public void SortArray_DefaultFallback_UsesDefaultColumnWhenTied()
	{
		var items = new[] {
			new TestItem("Same", 25, 90.0),
			new TestItem("Same", 25, 80.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending)
		};

		// Default column is "Score" — should break ties
		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Score");

		Assert.Equal(80.0, items[0].Score);
		Assert.Equal(90.0, items[1].Score);
	}

	[Fact]
	public void SortArray_EmptyArray_DoesNotThrow()
	{
		var items = Array.Empty<TestItem>();
		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		Assert.Empty(items);
	}

	[Fact]
	public void SortArray_SingleElement_RemainsUnchanged()
	{
		var items = new[] { new TestItem("Only", 42, 100.0) };
		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending)
		};

		SortHelper.SortArray(items, sortOrder, CreateComparers(), "Name");

		Assert.Single(items);
		Assert.Equal("Only", items[0].Name);
	}

	#endregion

	#region SortList Tests

	[Fact]
	public void SortList_SingleColumn_SortsByAge()
	{
		var items = new List<TestItem> {
			new("Charlie", 30, 85.0),
			new("Alice", 25, 90.0),
			new("Bob", 35, 80.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Age", ListSortDirection.Ascending)
		};

		SortHelper.SortList(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal(25, items[0].Age);
		Assert.Equal(30, items[1].Age);
		Assert.Equal(35, items[2].Age);
	}

	[Fact]
	public void SortList_MixedDirections_CorrectOrdering()
	{
		var items = new List<TestItem> {
			new("Alice", 30, 90.0),
			new("Bob", 25, 80.0),
			new("Charlie", 35, 85.0)
		};

		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Score", ListSortDirection.Descending)
		};

		SortHelper.SortList(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal(90.0, items[0].Score);
		Assert.Equal(85.0, items[1].Score);
		Assert.Equal(80.0, items[2].Score);
	}

	[Fact]
	public void SortList_FourOrMoreColumns_UsesGenericPath()
	{
		var items = new List<TestItem> {
			new("B", 2, 2.0),
			new("A", 1, 1.0),
			new("C", 3, 3.0)
		};

		// 4+ columns triggers the default/generic code path
		var sortOrder = new List<Tuple<string, ListSortDirection>> {
			Tuple.Create("Name", ListSortDirection.Ascending),
			Tuple.Create("Age", ListSortDirection.Ascending),
			Tuple.Create("Score", ListSortDirection.Ascending),
			Tuple.Create("Name", ListSortDirection.Descending)  // duplicated column for testing
		};

		SortHelper.SortList(items, sortOrder, CreateComparers(), "Name");

		Assert.Equal("A", items[0].Name);
		Assert.Equal("B", items[1].Name);
		Assert.Equal("C", items[2].Name);
	}

	#endregion
}
