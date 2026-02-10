using System;
using System.Diagnostics;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;

namespace Nexen.Controls.DataGridExtensions;

/// <summary>
/// Attached behavior that adds type-ahead keyboard search to Avalonia DataGrid,
/// replicating DataBox's search behavior including hex prefix ($) matching.
/// </summary>
/// <remarks>
/// When enabled, typing characters while the DataGrid is focused will search through
/// visible rows for a match. The search:
/// - Checks sorted columns first (priority)
/// - Auto-prepends '$' for hex matching (typing "FF" matches both "FF" and "$FF")
/// - Resets after 1 second of inactivity
/// - Can be disabled per-grid with DisableTypeAhead
///
/// Usage in AXAML:
/// <code>
/// ext:DataGridTypeAheadBehavior.IsEnabled="True"
/// </code>
/// </remarks>
public static class DataGridTypeAheadBehavior {
	/// <summary>Set to true to enable type-ahead search on the DataGrid.</summary>
	public static readonly AttachedProperty<bool> IsEnabledProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, bool>(
			"IsEnabled", typeof(DataGridTypeAheadBehavior), defaultValue: false);

	// Internal state stored per-grid via attached properties
	private static readonly AttachedProperty<string> SearchStringProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, string>(
			"SearchString", typeof(DataGridTypeAheadBehavior), defaultValue: "");

	private static readonly AttachedProperty<Stopwatch?> ResetTimerProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, Stopwatch?>(
			"ResetTimer", typeof(DataGridTypeAheadBehavior));

	static DataGridTypeAheadBehavior() {
		IsEnabledProperty.Changed.AddClassHandler<DataGrid>(OnIsEnabledChanged);
	}

	public static bool GetIsEnabled(DataGrid element) => element.GetValue(IsEnabledProperty);
	public static void SetIsEnabled(DataGrid element, bool value) => element.SetValue(IsEnabledProperty, value);

	private static void OnIsEnabledChanged(DataGrid grid, AvaloniaPropertyChangedEventArgs e) {
		if (e.NewValue is true) {
			grid.SetValue(ResetTimerProperty, Stopwatch.StartNew());
			grid.AddHandler(InputElement.TextInputEvent, OnTextInput, RoutingStrategies.Tunnel);
			grid.AddHandler(InputElement.KeyDownEvent, OnKeyDown, RoutingStrategies.Tunnel);
			grid.AddHandler(InputElement.PointerPressedEvent, OnPointerPressed, RoutingStrategies.Tunnel);
		} else {
			grid.RemoveHandler(InputElement.TextInputEvent, OnTextInput);
			grid.RemoveHandler(InputElement.KeyDownEvent, OnKeyDown);
			grid.RemoveHandler(InputElement.PointerPressedEvent, OnPointerPressed);
		}
	}

	private static void OnTextInput(object? sender, TextInputEventArgs e) {
		if (sender is not DataGrid grid || e.Text is null) return;
		ProcessKeyPress(grid, e.Text);
	}

	private static void OnKeyDown(object? sender, KeyEventArgs e) {
		if (sender is not DataGrid grid) return;
		if (e.Key == Key.Space) {
			ProcessKeyPress(grid, " ");
			e.Handled = true;
		}
	}

	private static void OnPointerPressed(object? sender, PointerPressedEventArgs e) {
		if (sender is not DataGrid grid) return;
		grid.SetValue(SearchStringProperty, "");
		grid.GetValue(ResetTimerProperty)?.Restart();
	}

	private static void ProcessKeyPress(DataGrid grid, string keyText) {
		if (grid.ItemsSource is null) return;

		var timer = grid.GetValue(ResetTimerProperty);
		if (timer is null) return;

		// Reset search string after 1 second of inactivity
		if (timer.ElapsedMilliseconds > 1000) {
			grid.SetValue(SearchStringProperty, "");
		}

		string searchString = grid.GetValue(SearchStringProperty) + keyText;
		grid.SetValue(SearchStringProperty, searchString);
		timer.Restart();

		string searchStringHex = "$" + searchString; // Auto-prepend $ for hex match

		// Get sort state to prioritize searching sorted columns first
		var sortState = DataGridSortBehavior.GetSortState(grid);

		// Search sorted columns first
		if (sortState?.SortOrder.Count > 0) {
			foreach (var sort in sortState.SortOrder) {
				var col = grid.Columns.FirstOrDefault(c =>
					(c.SortMemberPath ?? c.Header?.ToString() ?? "") == sort.Item1);
				if (col is not null && SearchColumn(grid, col, searchString, searchStringHex)) {
					return;
				}
			}
		}

		// Then search all columns
		foreach (var col in grid.Columns) {
			if (SearchColumn(grid, col, searchString, searchStringHex)) {
				return;
			}
		}
	}

	/// <summary>
	/// Searches a single column for a value matching the search string.
	/// Returns true if a match was found and selection was updated.
	/// </summary>
	private static bool SearchColumn(DataGrid grid, DataGridColumn column, string searchString, string searchStringHex) {
		if (grid.ItemsSource is null) return false;

		// Get the binding path for this column
		string? path = null;
		if (column is DataGridBoundColumn boundCol && boundCol.Binding is Avalonia.Data.Binding binding) {
			path = binding.Path;
		}
		if (string.IsNullOrEmpty(path)) return false;

		int index = 0;
		foreach (var item in grid.ItemsSource) {
			try {
				var prop = item.GetType().GetProperty(path);
				string value = prop?.GetValue(item)?.ToString() ?? "";

				if (value.StartsWith(searchString, StringComparison.OrdinalIgnoreCase)
					|| value.StartsWith(searchStringHex, StringComparison.OrdinalIgnoreCase)) {
					grid.SelectedIndex = index;
					grid.ScrollIntoView(item, null);
					return true;
				}
			} catch {
				// Skip items where property lookup fails
			}
			index++;
		}

		return false;
	}
}
