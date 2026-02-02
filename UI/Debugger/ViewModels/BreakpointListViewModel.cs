using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using DataBoxControl;
using Nexen.Config;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for managing and displaying a list of breakpoints in the debugger UI.
/// Provides sorting, selection management, and context menu actions for breakpoint operations.
/// </summary>
/// <remarks>
/// This ViewModel implements the MVVM pattern with ReactiveUI and provides:
/// - Observable collection of breakpoints with automatic UI updates
/// - Multi-select support for bulk operations
/// - Sortable columns (Enabled, Marked, Type, Address, Condition)
/// - Context menu with add, edit, delete, enable/disable, and navigation actions
/// </remarks>
public class BreakpointListViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the observable collection of breakpoint view models displayed in the list.
	/// </summary>
	[Reactive] public NexenList<BreakpointViewModel> Breakpoints { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for tracking selected breakpoints.
	/// Supports multiple selection for bulk operations.
	/// </summary>
	[Reactive] public SelectionModel<BreakpointViewModel?> Selection { get; set; } = new() { SingleSelect = false };

	/// <summary>
	/// Gets or sets the current sort state for column ordering.
	/// </summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>
	/// Gets the column widths from user configuration.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.BreakpointListColumnWidths;

	/// <summary>
	/// Gets the CPU type this breakpoint list is associated with.
	/// </summary>
	public CpuType CpuType { get; }

	/// <summary>
	/// Gets the parent debugger window view model.
	/// </summary>
	public DebuggerWindowViewModel Debugger { get; }

	/// <summary>
	/// Designer-only constructor. Do not use in production code.
	/// </summary>
	[Obsolete("For designer only")]
	public BreakpointListViewModel() : this(CpuType.Snes, new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="BreakpointListViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to filter breakpoints for.</param>
	/// <param name="debugger">The parent debugger window view model.</param>
	public BreakpointListViewModel(CpuType cpuType, DebuggerWindowViewModel debugger) {
		CpuType = cpuType;
		Debugger = debugger;
	}

	/// <summary>
	/// Sorts the breakpoint list based on the current sort state.
	/// </summary>
	/// <param name="param">Sort parameter (unused, for command binding compatibility).</param>
	public void Sort(object? param) {
		UpdateBreakpoints();
	}

	/// <summary>
	/// Dictionary of comparison functions for each sortable column.
	/// </summary>
	private Dictionary<string, Func<BreakpointViewModel, BreakpointViewModel, int>> _comparers = new() {
		{ "Enabled", (a, b) => a.Breakpoint.Enabled.CompareTo(b.Breakpoint.Enabled) },
		{ "Marked", (a, b) => a.Breakpoint.MarkEvent.CompareTo(b.Breakpoint.MarkEvent) },
		{ "Type", (a, b) => string.Compare(a.Breakpoint.ToReadableType(), b.Breakpoint.ToReadableType(), StringComparison.OrdinalIgnoreCase) },
		{ "Address", (a, b) => string.Compare(a.Breakpoint.GetAddressString(true), b.Breakpoint.GetAddressString(true), StringComparison.OrdinalIgnoreCase) },
		{ "Condition", (a, b) => string.Compare(a.Breakpoint.Condition, b.Breakpoint.Condition, StringComparison.OrdinalIgnoreCase) },
	};

	/// <summary>
	/// Updates the breakpoint list from the BreakpointManager and applies current sorting.
	/// Preserves the current selection after the update.
	/// </summary>
	public void UpdateBreakpoints() {
		List<int> selectedIndexes = Selection.SelectedIndexes.ToList();

		List<BreakpointViewModel> sortedBreakpoints = BreakpointManager.GetBreakpoints(CpuType).Select(bp => new BreakpointViewModel(bp)).ToList();

		if (SortState.SortOrder.Count > 0) {
			SortHelper.SortList(sortedBreakpoints, SortState.SortOrder, _comparers, "Address");
		}

		Breakpoints.Replace(sortedBreakpoints);
		Selection.SelectIndexes(selectedIndexes, Breakpoints.Count);
	}

	/// <summary>
	/// Refreshes the display properties of all breakpoints in the list.
	/// Call when breakpoint labels or addresses may have changed.
	/// </summary>
	public void RefreshBreakpointList() {
		foreach (BreakpointViewModel bp in Breakpoints) {
			bp.Refresh();
		}
	}

	/// <summary>
	/// Initializes the context menu for the breakpoint list control.
	/// Adds actions for add, edit, delete, enable/disable, and navigation.
	/// </summary>
	/// <param name="parent">The parent control to attach the context menu to.</param>
	public void InitContextMenu(Control parent) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(parent, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.AddBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_Add),
				OnClick = () => {
					Breakpoint bp = new Breakpoint() { BreakOnRead = true, BreakOnWrite = true, BreakOnExec = true, CpuType = CpuType };
					BreakpointEditWindow.EditBreakpoint(bp, parent);
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.AddForbidBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_AddForbid),
				OnClick = () => {
					Breakpoint bp = new Breakpoint() { Forbid = true, CpuType = CpuType };
					BreakpointEditWindow.EditBreakpoint(bp, parent);
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.Edit,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_Edit),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is BreakpointViewModel vm) {
						BreakpointEditWindow.EditBreakpoint(vm.Breakpoint, parent);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.Delete,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_Delete),
				IsEnabled = () => Selection.SelectedItems.Count > 0,
				OnClick = () => {
					List<Breakpoint> selectedBps = Selection.SelectedItems.Cast<BreakpointViewModel>().Select(vm => vm.Breakpoint).ToList();
					BreakpointManager.RemoveBreakpoints(selectedBps);
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.EnableBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_EnableBreakpoint),
				IsEnabled = () => Selection.SelectedItems.Any(bp => bp?.Breakpoint.Enabled == false),
				OnClick = () => {
					List<Breakpoint> selectedBps = Selection.SelectedItems.Cast<BreakpointViewModel>().Select(vm => vm.Breakpoint).ToList();
					foreach(Breakpoint bp in selectedBps) {
						bp.Enabled = true;
					}

					BreakpointManager.RefreshBreakpoints();
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.DisableBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_DisableBreakpoint),
				IsEnabled = () => Selection.SelectedItems.Any(bp => bp?.Breakpoint.Enabled == true),
				OnClick = () => {
					List<Breakpoint> selectedBps = Selection.SelectedItems.Cast<BreakpointViewModel>().Select(vm => vm.Breakpoint).ToList();
					foreach(Breakpoint bp in selectedBps) {
						bp.Enabled = false;
					}

					BreakpointManager.RefreshBreakpoints();
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_GoToLocation),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is BreakpointViewModel vm && vm.Breakpoint.SupportsExec && vm.Breakpoint.GetRelativeAddress() >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is BreakpointViewModel vm && vm.Breakpoint.SupportsExec) {
						int addr = vm.Breakpoint.GetRelativeAddress();
						if(addr >= 0) {
							Debugger.ScrollToAddress(addr);
						}
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.ViewInMemoryViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.BreakpointList_ViewInMemoryViewer),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is BreakpointViewModel vm) {
						MemoryToolsWindow.ShowInMemoryTools(vm.Breakpoint.MemoryType, (int)vm.Breakpoint.StartAddress);
					}
				}
			}
		}));
	}
}

/// <summary>
/// ViewModel wrapper for a single breakpoint, providing display-friendly properties.
/// Implements <see cref="INotifyPropertyChanged"/> for UI data binding.
/// </summary>
public class BreakpointViewModel : INotifyPropertyChanged {
	/// <summary>
	/// Gets or sets the underlying breakpoint model.
	/// </summary>
	public Breakpoint Breakpoint { get; set; }

	/// <summary>
	/// Gets a human-readable display string for the breakpoint type (e.g., "Read", "Write", "Execute").
	/// </summary>
	public string TypeDisplay => Breakpoint.ToReadableType();

	/// <summary>
	/// Gets a formatted display string for the breakpoint address.
	/// </summary>
	public string AddressDisplay => Breakpoint.GetAddressString(true);

	/// <summary>
	/// Occurs when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Raises property changed notifications for display properties.
	/// Call when the breakpoint's address or type may have changed.
	/// </summary>
	public void Refresh() {
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(TypeDisplay)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(AddressDisplay)));
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="BreakpointViewModel"/> class.
	/// </summary>
	/// <param name="bp">The breakpoint to wrap.</param>
	public BreakpointViewModel(Breakpoint bp) {
		Breakpoint = bp;
	}
}
