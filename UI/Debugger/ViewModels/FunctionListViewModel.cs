using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using Avalonia;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Media;
using DataBoxControl;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for displaying and managing discovered functions from CDL data.
/// Shows functions identified during code/data logging analysis.
/// </summary>
/// <remarks>
/// Functions are detected from CDL (Code Data Logger) data which tracks
/// which ROM bytes have been executed. This ViewModel provides:
/// - Sortable list of function entry points
/// - Label editing for unnamed functions
/// - Breakpoint and navigation support
/// </remarks>
public class FunctionListViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the observable collection of function view models.
	/// </summary>
	[Reactive] public NexenList<FunctionViewModel> Functions { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for multi-select support.
	/// </summary>
	[Reactive] public SelectionModel<FunctionViewModel?> Selection { get; set; } = new() { SingleSelect = false };

	/// <summary>
	/// Gets or sets the current sort state for column ordering.
	/// </summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>
	/// Gets the column widths from user configuration.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.FunctionListColumnWidths;

	/// <summary>
	/// Gets the CPU type this function list is associated with.
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
	public FunctionListViewModel() : this(CpuType.Snes, new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="FunctionListViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to get functions for.</param>
	/// <param name="debugger">The parent debugger window view model.</param>
	public FunctionListViewModel(CpuType cpuType, DebuggerWindowViewModel debugger) {
		CpuType = cpuType;
		Debugger = debugger;

		SortState.SetColumnSort("AbsAddr", ListSortDirection.Ascending, true);
	}

	/// <summary>
	/// Sorts the function list based on the current sort state.
	/// </summary>
	/// <param name="param">Sort parameter (unused, for command binding compatibility).</param>
	public void Sort(object? param) {
		UpdateFunctionList();
	}

	/// <summary>
	/// Dictionary of comparison functions for each sortable column.
	/// </summary>
	private Dictionary<string, Func<FunctionViewModel, FunctionViewModel, int>> _comparers = new() {
		{ "Function", (a, b) => string.Compare(a.LabelName, b.LabelName, StringComparison.OrdinalIgnoreCase) },
		{ "RelAddr", (a, b) => a.RelAddress.CompareTo(b.RelAddress) },
		{ "AbsAddr", (a, b) => a.AbsAddress.CompareTo(b.AbsAddress) },
	};

	/// <summary>
	/// Updates the function list from CDL data and applies current sorting.
	/// Preserves the current selection after the update.
	/// </summary>
	public void UpdateFunctionList() {
		List<int> selectedIndexes = Selection.SelectedIndexes.ToList();

		MemoryType prgMemType = CpuType.GetPrgRomMemoryType();
		List<FunctionViewModel> sortedFunctions = DebugApi.GetCdlFunctions(CpuType.GetPrgRomMemoryType()).Select(f => new FunctionViewModel(new AddressInfo() { Address = (int)f, Type = prgMemType }, CpuType)).ToList();

		SortHelper.SortList(sortedFunctions, SortState.SortOrder, _comparers, "AbsAddr");

		Functions.Replace(sortedFunctions);
		Selection.SelectIndexes(selectedIndexes, Functions.Count);
	}

	/// <summary>
	/// Initializes the context menu for the function list control.
	/// Adds actions for label editing, breakpoints, and navigation.
	/// </summary>
	/// <param name="parent">The parent control to attach the context menu to.</param>
	public void InitContextMenu(Control parent) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(parent, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.EditLabel,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FunctionList_EditLabel),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is FunctionViewModel vm) {
						LabelEditWindow.EditLabel(CpuType, parent, vm.Label ?? new CodeLabel(vm.FuncAddr));
					}
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.ToggleBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FunctionList_ToggleBreakpoint),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is FunctionViewModel vm) {
						BreakpointManager.ToggleBreakpoint(vm.FuncAddr, CpuType);
					}
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.FindOccurrences,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FunctionList_FindOccurrences),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is FunctionViewModel vm && vm.RelAddress >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is FunctionViewModel vm && vm.RelAddress >= 0) {
						DisassemblySearchOptions options = new() { MatchCase = true, MatchWholeWord = true };
						Debugger.FindAllOccurrences(vm.Label?.Label ?? vm.RelAddressDisplay, options);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FunctionList_GoToLocation),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is FunctionViewModel vm && vm.RelAddress >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is FunctionViewModel vm) {
						if(vm.RelAddress >= 0) {
							Debugger.ScrollToAddress(vm.RelAddress);
						}
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.ViewInMemoryViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FunctionList_ViewInMemoryViewer),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is FunctionViewModel vm) {
						AddressInfo addr = new AddressInfo() { Address = vm.RelAddress, Type = CpuType.ToMemoryType() };
						if(addr.Address < 0) {
							addr = vm.FuncAddr;
						}

						MemoryToolsWindow.ShowInMemoryTools(addr.Type, addr.Address);
					}
				}
			},
		}));
	}
}

/// <summary>
/// ViewModel wrapper for a single function entry point.
/// Implements <see cref="INotifyPropertyChanged"/> for UI data binding.
/// </summary>
public class FunctionViewModel : INotifyPropertyChanged {
	/// <summary>
	/// Format string for address display based on CPU address size.
	/// </summary>
	private string _format;

	/// <summary>
	/// Gets the absolute address info for this function.
	/// </summary>
	public AddressInfo FuncAddr { get; private set; }

	/// <summary>
	/// The CPU type for address conversion.
	/// </summary>
	public CpuType _cpuType;

	/// <summary>
	/// Gets the formatted absolute address string.
	/// </summary>
	public string AbsAddressDisplay { get; }

	/// <summary>
	/// Gets the absolute address as an integer.
	/// </summary>
	public int AbsAddress => FuncAddr.Address;

	/// <summary>
	/// Gets the relative (CPU-visible) address, or -1 if unmapped.
	/// </summary>
	public int RelAddress { get; private set; }

	/// <summary>
	/// Gets the formatted relative address string, or "unavailable" if unmapped.
	/// </summary>
	public string RelAddressDisplay => RelAddress >= 0 ? ("$" + RelAddress.ToString(_format)) : "<unavailable>";

	/// <summary>
	/// Gets the brush for row highlighting (gray for unavailable addresses).
	/// </summary>
	public object RowBrush => RelAddress >= 0 ? AvaloniaProperty.UnsetValue : Brushes.Gray;

	/// <summary>
	/// Gets the font style (italic for unavailable addresses).
	/// </summary>
	public FontStyle RowStyle => RelAddress >= 0 ? FontStyle.Normal : FontStyle.Italic;

	/// <summary>
	/// Gets the label associated with this function address, if any.
	/// </summary>
	public CodeLabel? Label => LabelManager.GetLabel(FuncAddr);

	/// <summary>
	/// Gets the label name or "no label" placeholder text.
	/// </summary>
	public string LabelName => Label?.Label ?? "<no label>";

	/// <summary>
	/// Occurs when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Refreshes the relative address and label name if changed.
	/// </summary>
	public void Refresh() {
		int addr = DebugApi.GetRelativeAddress(FuncAddr, _cpuType).Address;
		if (addr != RelAddress) {
			RelAddress = addr;
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RowBrush)));
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RowStyle)));
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RelAddressDisplay)));
		}

		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(LabelName)));
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="FunctionViewModel"/> class.
	/// </summary>
	/// <param name="funcAddr">The absolute address of the function entry point.</param>
	/// <param name="cpuType">The CPU type for address conversion.</param>
	public FunctionViewModel(AddressInfo funcAddr, CpuType cpuType) {
		FuncAddr = funcAddr;
		_cpuType = cpuType;
		RelAddress = DebugApi.GetRelativeAddress(FuncAddr, _cpuType).Address;
		_format = "X" + cpuType.GetAddressSize();

		AbsAddressDisplay = "$" + FuncAddr.Address.ToString(_format);
	}
}
