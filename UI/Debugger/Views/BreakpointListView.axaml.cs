using System;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Nexen.Controls.DataGridExtensions;
using Nexen.Config;
using Nexen.Debugger;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Utilities;
using Nexen.ViewModels;
using static Nexen.Debugger.ViewModels.BreakpointListViewModel;

namespace Nexen.Debugger.Views;
public partial class BreakpointListView : UserControl {
	public BreakpointListView() {
		InitializeComponent();

		// Subscribe to DataGrid CellClick and CellDoubleClick routed events
		this.AddHandler(DataGridCellClickBehavior.CellClickEvent, OnCellClick);
		this.AddHandler(DataGridCellClickBehavior.CellDoubleClickEvent, OnCellDoubleClick);
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	protected override void OnDataContextChanged(EventArgs e) {
		if (DataContext is BreakpointListViewModel vm) {
			vm.InitContextMenu(this);
		}

		base.OnDataContextChanged(e);
	}

	private void OnCellClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (DataContext is BreakpointListViewModel bpList && e.RowItem is BreakpointViewModel) {
			string? header = e.Column?.Header?.ToString() ?? "";
			if (header is "E" or "M") {
				bool isEnabledColumn = header == "E";
				bool newValue = !bpList.Selection.SelectedItems.Any(bp => (isEnabledColumn ? bp?.Breakpoint.Enabled : bp?.Breakpoint.MarkEvent) == true);

				foreach (BreakpointViewModel? bp in bpList.Selection.SelectedItems) {
					if (bp != null) {
						if (isEnabledColumn) {
							bp.Breakpoint.Enabled = newValue;
						} else {
							if (!bp.Breakpoint.Forbid) {
								bp.Breakpoint.MarkEvent = newValue;
							}
						}
					}
				}

				DebugWorkspaceManager.AutoSave();
				BreakpointManager.RefreshBreakpoints();
			}
		}
	}

	private void OnCellDoubleClick(object? sender, DataGridCellClickRoutedEventArgs e) {
		if (e.RowItem is BreakpointViewModel vm) {
			BreakpointEditWindow.EditBreakpoint(vm.Breakpoint, this);
		}
	}
}
