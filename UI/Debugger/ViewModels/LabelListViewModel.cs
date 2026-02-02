using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
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
/// ViewModel for managing and displaying a list of code labels in the debugger.
/// Provides sorting, filtering, and CRUD operations for labels.
/// </summary>
/// <remarks>
/// Labels are symbolic names assigned to memory addresses for easier debugging.
/// This ViewModel supports:
/// - Sortable columns (Label, RelAddr, AbsAddr, Comment)
/// - Multi-select for bulk delete operations
/// - Context menu with add, edit, delete, and navigation actions
/// - Integration with breakpoints and watch expressions
/// </remarks>
public class LabelListViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the observable collection of label view models.
	/// </summary>
	[Reactive] public NexenList<LabelViewModel> Labels { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for multi-select support.
	/// </summary>
	[Reactive] public SelectionModel<LabelViewModel?> Selection { get; set; } = new() { SingleSelect = false };

	/// <summary>
	/// Gets or sets the current sort state for column ordering.
	/// </summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>
	/// Gets the column widths from user configuration.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.LabelListColumnWidths;

	/// <summary>
	/// Gets the CPU type this label list is associated with.
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
	public LabelListViewModel() : this(CpuType.Snes, new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="LabelListViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to filter labels for.</param>
	/// <param name="debugger">The parent debugger window view model.</param>
	public LabelListViewModel(CpuType cpuType, DebuggerWindowViewModel debugger) {
		CpuType = cpuType;
		Debugger = debugger;

		SortState.SetColumnSort("Label", ListSortDirection.Ascending, true);
	}

	/// <summary>
	/// Sorts the label list based on the current sort state.
	/// </summary>
	/// <param name="param">Sort parameter (unused, for command binding compatibility).</param>
	public void Sort(object? param) {
		UpdateLabelList();
	}

	/// <summary>
	/// Dictionary of comparison functions for each sortable column.
	/// </summary>
	private Dictionary<string, Func<LabelViewModel, LabelViewModel, int>> _comparers = new() {
		{ "Label", (a, b) => string.Compare(a.Label.Label, b.Label.Label, StringComparison.OrdinalIgnoreCase) },
		{ "RelAddr", (a, b) => a.RelAddress.CompareTo(b.RelAddress) },
		{ "AbsAddr", (a, b) => a.Label.Address.CompareTo(b.Label.Address) },
		{ "Comment", (a, b) => string.Compare(a.Label.Comment, b.Label.Comment, StringComparison.OrdinalIgnoreCase) },
	};

	/// <summary>
	/// Updates the label list from the LabelManager and applies current sorting.
	/// Preserves the current selection after the update.
	/// </summary>
	public void UpdateLabelList() {
		List<int> selectedIndexes = Selection.SelectedIndexes.ToList();

		List<LabelViewModel> sortedLabels = LabelManager.GetLabels(CpuType).Select(l => new LabelViewModel(l, CpuType)).ToList();

		SortHelper.SortList(sortedLabels, SortState.SortOrder, _comparers, "Label");

		Labels.Replace(sortedLabels);

		Selection.SelectIndexes(selectedIndexes, Labels.Count);
	}

	/// <summary>
	/// Refreshes the display properties of all labels in the list.
	/// Call when label addresses may have changed due to memory mapping updates.
	/// </summary>
	public void RefreshLabelList() {
		foreach (LabelViewModel label in Labels) {
			label.Refresh();
		}
	}

	/// <summary>
	/// Initializes the context menu for the label list control.
	/// Adds actions for add, edit, delete, breakpoints, watches, and navigation.
	/// </summary>
	/// <param name="parent">The parent control to attach the context menu to.</param>
	public void InitContextMenu(Control parent) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(parent, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.Add,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_Add),
				OnClick = () => LabelEditWindow.EditLabel(CpuType, parent, new CodeLabel())
			},

			new ContextMenuAction() {
				ActionType = ActionType.Edit,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_Edit),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						LabelEditWindow.EditLabel(CpuType, parent, vm.Label);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.Delete,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_Delete),
				IsEnabled = () => Selection.SelectedItems.Count > 0,
				OnClick = () => {
					IEnumerable<CodeLabel> labels = Selection.SelectedItems.Cast<LabelViewModel>().Select(vm => vm.Label).ToList();
					LabelManager.DeleteLabels(labels);
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.ToggleBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_ToggleBreakpoint),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						AddressInfo addr = vm.Label.GetAbsoluteAddress();
						BreakpointManager.ToggleBreakpoint(addr, CpuType);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.AddWatch,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_AddToWatch),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is LabelViewModel vm && vm.RelAddress >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						if(vm.Label?.Label.Length > 0) {
							WatchManager.GetWatchManager(CpuType).AddWatch("[" + vm.Label.Label + "]");
						} else {
							WatchManager.GetWatchManager(CpuType).AddWatch("[" + vm.RelAddressDisplay + "]");
						}
					}
				}
			},

			new ContextMenuSeparator(),

			new ContextMenuAction() {
				ActionType = ActionType.FindOccurrences,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_FindOccurrences),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is LabelViewModel vm && vm.Label.GetRelativeAddress(CpuType).Address >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						DisassemblySearchOptions options = new() { MatchCase = true, MatchWholeWord = true };
						Debugger.FindAllOccurrences(vm.Label.Label, options);
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_GoToLocation),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is LabelViewModel vm && vm.Label.GetRelativeAddress(CpuType).Address >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						AddressInfo addr = vm.Label.GetRelativeAddress(CpuType);
						if(addr.Address >= 0) {
							Debugger.ScrollToAddress(addr.Address);
						}
					}
				}
			},

			new ContextMenuAction() {
				ActionType = ActionType.ViewInMemoryViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.LabelList_ViewInMemoryViewer),
				IsEnabled = () => Selection.SelectedItems.Count == 1,
				OnClick = () => {
					if(Selection.SelectedItem is LabelViewModel vm) {
						AddressInfo addr = vm.Label.GetRelativeAddress(CpuType);
						if(addr.Address < 0) {
							addr = vm.Label.GetAbsoluteAddress();
						}

						MemoryToolsWindow.ShowInMemoryTools(addr.Type, addr.Address);
					}
				}
			},
		}));
	}
}

/// <summary>
/// ViewModel wrapper for a single code label, providing display-friendly properties.
/// Implements <see cref="INotifyPropertyChanged"/> for UI data binding.
/// </summary>
public class LabelViewModel : INotifyPropertyChanged {
	/// <summary>
	/// Indicates whether the label's memory type is an unmapped type (e.g., PRG ROM).
	/// </summary>
	private bool _isUnmappedType;

	/// <summary>
	/// Gets or sets the underlying code label model.
	/// </summary>
	public CodeLabel Label { get; set; }

	/// <summary>
	/// Gets the CPU type for address formatting.
	/// </summary>
	public CpuType CpuType { get; }

	/// <summary>
	/// Gets the formatted absolute address string with memory type.
	/// </summary>
	public string AbsAddressDisplay { get; }

	/// <summary>
	/// Gets the label name text.
	/// </summary>
	public string LabelText { get; private set; }

	/// <summary>
	/// Gets the label comment text.
	/// </summary>
	public string LabelComment { get; private set; }

	/// <summary>
	/// Gets the relative (CPU-visible) address, or -1 if unmapped.
	/// </summary>
	public int RelAddress { get; private set; }

	/// <summary>
	/// Gets the formatted relative address string, or "unavailable" if unmapped.
	/// </summary>
	public string RelAddressDisplay => RelAddress >= 0 ? ("$" + RelAddress.ToString(field)) : (_isUnmappedType ? "" : "<unavailable>");

	/// <summary>
	/// Gets the brush for row highlighting (gray for unavailable addresses).
	/// </summary>
	public object RowBrush => RelAddress >= 0 || _isUnmappedType ? AvaloniaProperty.UnsetValue : Brushes.Gray;

	/// <summary>
	/// Gets the font style (italic for unavailable addresses).
	/// </summary>
	public FontStyle RowStyle => RelAddress >= 0 || _isUnmappedType ? FontStyle.Normal : FontStyle.Italic;

	/// <summary>
	/// Occurs when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Refreshes the relative address and updates display properties if changed.
	/// </summary>
	public void Refresh() {
		int addr = Label.GetRelativeAddress(CpuType).Address;
		if (addr != RelAddress) {
			RelAddress = addr;
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RowBrush)));
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RowStyle)));
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(RelAddressDisplay)));
		}
	}

	/// <summary>
	/// Initializes a new instance of the <see cref="LabelViewModel"/> class.
	/// </summary>
	/// <param name="label">The code label to wrap.</param>
	/// <param name="cpuType">The CPU type for address formatting.</param>
	public LabelViewModel(CodeLabel label, CpuType cpuType) {
		Label = label;
		LabelText = label.Label;
		LabelComment = label.Comment;
		CpuType = cpuType;
		RelAddress = Label.GetRelativeAddress(CpuType).Address;
		RelAddressDisplay = "X" + cpuType.GetAddressSize();
		_isUnmappedType = Label.MemoryType.IsUnmapped();

		AbsAddressDisplay = Label.MemoryType.IsRelativeMemory()
			? ""
			: "$" + label.Address.ToString(label.MemoryType.GetFormatString()) + " [" + label.MemoryType.GetShortName() + "]";
	}
}
