using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Data;

namespace Nexen.Controls.DataGridExtensions;

/// <summary>
/// Attached behavior for persisting DataGrid column widths.
/// Column widths are stored positionally in a <see cref="List{T}"/> of <see cref="int"/>
/// and synced back to the bound ViewModel property on resize.
/// </summary>
/// <remarks>
/// Usage in AXAML:
/// <code>
/// xmlns:ext="using:Nexen.Controls.DataGridExtensions"
/// ext:DataGridColumnWidthBehavior.ColumnWidths="{Binding ColumnWidths}"
/// </code>
/// </remarks>
public static class DataGridColumnWidthBehavior {
	/// <summary>
	/// Positional list of column widths (int). Bound two-way to ViewModel.
	/// </summary>
	public static readonly AttachedProperty<List<int>> ColumnWidthsProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, List<int>>(
			"ColumnWidths", typeof(DataGridColumnWidthBehavior), defaultBindingMode: BindingMode.TwoWay);

	static DataGridColumnWidthBehavior() {
		ColumnWidthsProperty.Changed.AddClassHandler<DataGrid>(OnColumnWidthsChanged);
	}

	public static List<int> GetColumnWidths(DataGrid element) => element.GetValue(ColumnWidthsProperty);
	public static void SetColumnWidths(DataGrid element, List<int> value) => element.SetValue(ColumnWidthsProperty, value);

	private static void OnColumnWidthsChanged(DataGrid grid, AvaloniaPropertyChangedEventArgs e) {
		if (e.NewValue is List<int> widths) {
			ApplyWidths(grid, widths);
			grid.ColumnDisplayIndexChanged -= OnColumnResized;
			grid.ColumnDisplayIndexChanged += OnColumnResized;
			// Avalonia DataGrid doesn't have a direct ColumnWidthChanged event,
			// so we hook into LayoutUpdated to detect resize changes
			grid.LayoutUpdated -= OnLayoutUpdated;
			grid.LayoutUpdated += OnLayoutUpdated;
		}
	}

	private static void ApplyWidths(DataGrid grid, List<int> widths) {
		// Ensure list has entries for all columns, filling with defaults
		while (widths.Count < grid.Columns.Count) {
			var col = grid.Columns[widths.Count];
			widths.Add((int)col.Width.Value);
		}

		for (int i = 0; i < grid.Columns.Count && i < widths.Count; i++) {
			var col = grid.Columns[i];
			if (widths[i] > 0) {
				col.Width = new DataGridLength(widths[i]);
			}
		}
	}

	private static void OnColumnResized(object? sender, DataGridColumnEventArgs e) {
		if (sender is not DataGrid grid) return;
		SyncWidthsBack(grid);
	}

	private static void OnLayoutUpdated(object? sender, EventArgs e) {
		if (sender is not DataGrid grid) return;
		SyncWidthsBack(grid);
	}

	private static void SyncWidthsBack(DataGrid grid) {
		var widths = GetColumnWidths(grid);
		if (widths is null) return;

		bool changed = false;
		for (int i = 0; i < grid.Columns.Count && i < widths.Count; i++) {
			int actual = (int)grid.Columns[i].ActualWidth;
			if (actual > 0 && widths[i] != actual) {
				widths[i] = actual;
				changed = true;
			}
		}

		// The list is mutated in-place (matching DataBox behavior)
		// so the ViewModel sees the changes automatically
		_ = changed; // suppress unused warning — mutation is in-place
	}
}
