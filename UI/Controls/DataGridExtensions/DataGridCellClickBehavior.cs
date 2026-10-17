using System;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Avalonia.VisualTree;

namespace Nexen.Controls.DataGridExtensions;

/// <summary>
/// Event args for DataGrid cell click events, providing the clicked row item and column.
/// </summary>
public sealed class DataGridCellClickEventArgs : EventArgs {
	/// <summary>The data item of the clicked row.</summary>
	public object? RowItem { get; init; }

	/// <summary>The column that was clicked.</summary>
	public DataGridColumn? Column { get; init; }

	/// <summary>The row index.</summary>
	public int RowIndex { get; init; }
}

/// <summary>
/// Attached behavior that adds CellClick and CellDoubleClick events to Avalonia DataGrid,
/// replicating DataBox's cell-level click tracking.
/// </summary>
/// <remarks>
/// CellClick fires when pointer-down and pointer-up occur on the same cell (not a drag).
/// CellDoubleClick fires on double-tap.
///
/// Usage in code-behind:
/// <code>
/// DataGridCellClickBehavior.AddCellClickHandler(grid, OnCellClick);
/// DataGridCellClickBehavior.AddCellDoubleClickHandler(grid, OnCellDoubleClick);
/// </code>
///
/// Or in AXAML with commands:
/// <code>
/// ext:DataGridCellClickBehavior.IsEnabled="True"
/// </code>
/// Then subscribe in code-behind.
/// </remarks>
public static class DataGridCellClickBehavior {
	/// <summary>
	/// Set to true to enable cell click tracking on a DataGrid.
	/// </summary>
	public static readonly AttachedProperty<bool> IsEnabledProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, bool>(
			"IsEnabled", typeof(DataGridCellClickBehavior), defaultValue: false);

	/// <summary>CellClick routed event — fires on pointer release on the same cell as press.</summary>
	public static readonly RoutedEvent<DataGridCellClickRoutedEventArgs> CellClickEvent =
		RoutedEvent.Register<DataGrid, DataGridCellClickRoutedEventArgs>(
			"CellClick", RoutingStrategies.Bubble);

	/// <summary>CellDoubleClick routed event — fires on double-tap.</summary>
	public static readonly RoutedEvent<DataGridCellClickRoutedEventArgs> CellDoubleClickEvent =
		RoutedEvent.Register<DataGrid, DataGridCellClickRoutedEventArgs>(
			"CellDoubleClick", RoutingStrategies.Bubble);

	// Track the cell where pointer-down occurred
	private static readonly AttachedProperty<DataGridCell?> PressedCellProperty =
		AvaloniaProperty.RegisterAttached<DataGrid, DataGridCell?>(
			"PressedCell", typeof(DataGridCellClickBehavior));

	static DataGridCellClickBehavior() {
		IsEnabledProperty.Changed.AddClassHandler<DataGrid>(OnIsEnabledChanged);
	}

	public static bool GetIsEnabled(DataGrid element) => element.GetValue(IsEnabledProperty);
	public static void SetIsEnabled(DataGrid element, bool value) => element.SetValue(IsEnabledProperty, value);

	public static void AddCellClickHandler(DataGrid element, EventHandler<DataGridCellClickRoutedEventArgs> handler) =>
		element.AddHandler(CellClickEvent, handler);

	public static void RemoveCellClickHandler(DataGrid element, EventHandler<DataGridCellClickRoutedEventArgs> handler) =>
		element.RemoveHandler(CellClickEvent, handler);

	public static void AddCellDoubleClickHandler(DataGrid element, EventHandler<DataGridCellClickRoutedEventArgs> handler) =>
		element.AddHandler(CellDoubleClickEvent, handler);

	public static void RemoveCellDoubleClickHandler(DataGrid element, EventHandler<DataGridCellClickRoutedEventArgs> handler) =>
		element.RemoveHandler(CellDoubleClickEvent, handler);

	private static void OnIsEnabledChanged(DataGrid grid, AvaloniaPropertyChangedEventArgs e) {
		if (e.NewValue is true) {
			grid.AddHandler(InputElement.PointerPressedEvent, OnPointerPressed, RoutingStrategies.Tunnel);
			grid.AddHandler(InputElement.PointerReleasedEvent, OnPointerReleased, RoutingStrategies.Tunnel);
			grid.AddHandler(InputElement.DoubleTappedEvent, OnDoubleTapped, RoutingStrategies.Tunnel);
		} else {
			grid.RemoveHandler(InputElement.PointerPressedEvent, OnPointerPressed);
			grid.RemoveHandler(InputElement.PointerReleasedEvent, OnPointerReleased);
			grid.RemoveHandler(InputElement.DoubleTappedEvent, OnDoubleTapped);
		}
	}

	private static void OnPointerPressed(object? sender, PointerPressedEventArgs e) {
		if (sender is not DataGrid grid) return;
		var cell = FindCellFromVisual(e.Source as Visual);
		grid.SetValue(PressedCellProperty, cell);
	}

	private static void OnPointerReleased(object? sender, PointerReleasedEventArgs e) {
		if (sender is not DataGrid grid) return;
		var pressedCell = grid.GetValue(PressedCellProperty);
		var releasedCell = FindCellFromVisual(e.Source as Visual);

		grid.SetValue(PressedCellProperty, null);

		if (pressedCell is not null && pressedCell == releasedCell) {
			var args = CreateEventArgs(grid, pressedCell);
			Dispatcher.UIThread.Post(() => grid.RaiseEvent(
				new DataGridCellClickRoutedEventArgs(CellClickEvent) {
					RowItem = args.RowItem,
					Column = args.Column,
					RowIndex = args.RowIndex
				}
			));
		}
	}

	private static void OnDoubleTapped(object? sender, TappedEventArgs e) {
		if (sender is not DataGrid grid) return;
		var cell = FindCellFromVisual(e.Source as Visual);
		if (cell is null) return;

		var args = CreateEventArgs(grid, cell);
		Dispatcher.UIThread.Post(() => grid.RaiseEvent(
			new DataGridCellClickRoutedEventArgs(CellDoubleClickEvent) {
				RowItem = args.RowItem,
				Column = args.Column,
				RowIndex = args.RowIndex
			}
		));
	}

	/// <summary>
	/// Walks up the visual tree to find the containing DataGridCell.
	/// </summary>
	private static DataGridCell? FindCellFromVisual(Visual? visual) {
		while (visual is not null) {
			if (visual is DataGridCell cell) return cell;
			visual = visual.GetVisualParent() as Visual;
		}
		return null;
	}

	private static PropertyInfo? _owningColumnProperty;

	private static DataGridCellClickEventArgs CreateEventArgs(DataGrid grid, DataGridCell cell) {
		var row = cell.FindAncestorOfType<DataGridRow>();

		// Access internal OwningColumn via reflection
		_owningColumnProperty ??= typeof(DataGridCell).GetProperty(
			"OwningColumn", BindingFlags.NonPublic | BindingFlags.Instance);
		var column = _owningColumnProperty?.GetValue(cell) as DataGridColumn;

		return new DataGridCellClickEventArgs {
			RowItem = row?.DataContext,
			Column = column,
			RowIndex = row?.Index ?? -1
		};
	}
}

/// <summary>
/// Routed event args for cell click events on DataGrid.
/// </summary>
public sealed class DataGridCellClickRoutedEventArgs : RoutedEventArgs {
	public DataGridCellClickRoutedEventArgs(RoutedEvent routedEvent) : base(routedEvent) { }

	/// <summary>The data item of the clicked row.</summary>
	public object? RowItem { get; init; }

	/// <summary>The column that was clicked.</summary>
	public DataGridColumn? Column { get; init; }

	/// <summary>The row index.</summary>
	public int RowIndex { get; init; }
}
