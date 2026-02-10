using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;

namespace Nexen.Controls.DataGridExtensions;

/// <summary>
/// Multi-column sort state tracker for DataGrid, replicating DataBox SortState behavior.
/// Supports single-click for single-column sort, Shift+click for multi-column,
/// and Ctrl+click to remove a column from sort.
/// </summary>
public sealed class SortState {
	/// <summary>
	/// Ordered list of (ColumnName, Direction) tuples defining the current multi-column sort.
	/// </summary>
	public List<Tuple<string, ListSortDirection>> SortOrder { get; private set; } = [];

	/// <summary>Clears all sort state.</summary>
	public void Reset() {
		SortOrder.Clear();
	}

	/// <summary>Removes a specific column from the sort order.</summary>
	public void Remove(string column) {
		Tuple<string, ListSortDirection>? columnInfo = SortOrder.Find(x => x.Item1 == column);
		if (columnInfo is not null) {
			SortOrder.Remove(columnInfo);
		}
	}

	/// <summary>
	/// Toggles the sort direction on a column. If <paramref name="reset"/> is true,
	/// clears all other columns first (single-column sort).
	/// </summary>
	public void ToggleSortOrder(string column, bool reset) {
		int i = SortOrder.FindIndex(x => x.Item1 == column);
		ListSortDirection order = i < 0
			? ListSortDirection.Ascending
			: SortOrder[i].Item2 == ListSortDirection.Descending
				? ListSortDirection.Ascending
				: ListSortDirection.Descending;
		SetColumnSort(column, order, reset);
	}

	/// <summary>Sets a specific sort direction on a column.</summary>
	public void SetColumnSort(string column, ListSortDirection order, bool reset) {
		Tuple<string, ListSortDirection> columnInfo = new(column, order);
		int i = SortOrder.FindIndex(x => x.Item1 == column);

		if (reset) {
			Reset();
			SortOrder.Add(columnInfo);
		} else if (i < 0) {
			SortOrder.Add(columnInfo);
		} else {
			SortOrder[i] = columnInfo;
		}
	}

	/// <summary>Gets the sort direction for a column, or null if not sorted.</summary>
	public ListSortDirection? GetSortDirection(string column) {
		Tuple<string, ListSortDirection>? columnInfo = SortOrder.Find(x => x.Item1 == column);
		return columnInfo?.Item2;
	}

	/// <summary>Gets the 1-based sort order number for display, or "" if not in multi-sort.</summary>
	public string GetSortNumber(string column) {
		if (SortOrder.Count <= 1) return "";
		int i = SortOrder.FindIndex(x => x.Item1 == column);
		return i >= 0 ? (i + 1).ToString() : "";
	}
}

/// <summary>
/// Sort mode matching DataBox behavior.
/// </summary>
public enum SortMode {
	/// <summary>No sorting allowed.</summary>
	None,
	/// <summary>Only one column can be sorted at a time.</summary>
	Single,
	/// <summary>Multiple columns can be sorted simultaneously.</summary>
	Multiple
}

/// <summary>
/// Attached behavior that adds multi-column sort support to Avalonia DataGrid,
/// replicating the DataBox SortState/SortCommand/SortMode pattern.
/// </summary>
/// <remarks>
/// Usage in AXAML:
/// <code>
/// xmlns:ext="using:Nexen.Controls.DataGridExtensions"
/// ext:DataGridSortBehavior.SortState="{Binding SortState}"
/// ext:DataGridSortBehavior.SortCommand="{Binding SortCommand}"
/// ext:DataGridSortBehavior.SortMode="Multiple"
/// </code>
/// </remarks>
public static class DataGridSortBehavior {
	public static readonly AttachedProperty<SortState> SortStateProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, SortState>(
			"SortState", typeof(DataGridSortBehavior), defaultBindingMode: BindingMode.TwoWay);

	public static readonly AttachedProperty<ICommand?> SortCommandProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, ICommand?>(
			"SortCommand", typeof(DataGridSortBehavior));

	public static readonly AttachedProperty<SortMode> SortModeProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, SortMode>(
			"SortMode", typeof(DataGridSortBehavior), defaultValue: SortMode.None);

	static DataGridSortBehavior() {
		SortStateProperty.Changed.AddClassHandler<DataGrid>(OnSortStateChanged);
	}

	public static SortState GetSortState(DataGrid element) => element.GetValue(SortStateProperty);
	public static void SetSortState(DataGrid element, SortState value) => element.SetValue(SortStateProperty, value);

	public static ICommand? GetSortCommand(DataGrid element) => element.GetValue(SortCommandProperty);
	public static void SetSortCommand(DataGrid element, ICommand? value) => element.SetValue(SortCommandProperty, value);

	public static SortMode GetSortMode(DataGrid element) => element.GetValue(SortModeProperty);
	public static void SetSortMode(DataGrid element, SortMode value) => element.SetValue(SortModeProperty, value);

	private static void OnSortStateChanged(DataGrid grid, AvaloniaPropertyChangedEventArgs e) {
		// Override DataGrid's built-in sorting with our custom multi-column sort
		grid.Sorting -= OnDataGridSorting;
		grid.Sorting += OnDataGridSorting;
	}

	/// <summary>
	/// Intercepts Avalonia DataGrid's built-in sort and replaces it with our multi-column SortState logic.
	/// </summary>
	private static void OnDataGridSorting(object? sender, DataGridColumnEventArgs e) {
		if (sender is not DataGrid grid) return;

		SortMode mode = GetSortMode(grid);
		if (mode == SortMode.None) return;

		SortState state = GetSortState(grid);
		ICommand? command = GetSortCommand(grid);
		if (state is null || command is null) return;

		// Get the column name — use SortMemberPath or Header as fallback
		string columnName = e.Column.SortMemberPath
			?? e.Column.Header?.ToString()
			?? "";

		if (string.IsNullOrEmpty(columnName)) return;

		// Determine modifier keys for multi-sort behavior
		// Avalonia DataGrid handles Shift internally, but we take full control
		// Plain click = single-column sort (reset others)
		// For multi-column, the SortCommand handler in the ViewModel manages it
		state.ToggleSortOrder(columnName, mode != SortMode.Multiple);

		// Fire the ViewModel's sort command
		if (command.CanExecute(null)) {
			command.Execute(null);
		}

		// Update column header sort indicators via reflection
		// (DataGrid's SortDirection/CurrentSortingState is internal)
		UpdateSortIndicators(grid, state);
	}

	/// <summary>
	/// Updates visual sort indicators on column headers using reflection
	/// to access internal Avalonia DataGrid APIs.
	/// </summary>
	private static void UpdateSortIndicators(DataGrid grid, SortState state) {
		foreach (var col in grid.Columns) {
			string colName = col.SortMemberPath ?? col.Header?.ToString() ?? "";
			var direction = state.GetSortDirection(colName);
			SetColumnSortDirection(col, direction);
		}
	}

	private static PropertyInfo? _headerCellProperty;
	private static PropertyInfo? _sortingStateProperty;

	/// <summary>
	/// Sets the visual sort indicator on a column header via reflection.
	/// Accesses internal HeaderCell.CurrentSortingState property.
	/// </summary>
	private static void SetColumnSortDirection(DataGridColumn column, ListSortDirection? direction) {
		try {
			_headerCellProperty ??= typeof(DataGridColumn).GetProperty(
				"HeaderCell", BindingFlags.NonPublic | BindingFlags.Instance);

			var headerCell = _headerCellProperty?.GetValue(column);
			if (headerCell is null) return;

			_sortingStateProperty ??= headerCell.GetType().GetProperty(
				"CurrentSortingState", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Public);

			_sortingStateProperty?.SetValue(headerCell, direction);
		} catch {
			// Silently fail if Avalonia internals change — sort still works, just no visual indicator
		}
	}
}
