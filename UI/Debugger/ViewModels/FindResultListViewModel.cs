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
using Avalonia.Threading;
using Nexen.Controls.DataGridExtensions;
using Dock.Model.Core;
using Nexen.Config;
using Nexen.Debugger.Disassembly;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels.DebuggerDock;
using Nexen.Debugger.Views.DebuggerDock;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;

/// <summary>
/// ViewModel for the find results panel in the debugger.
/// </summary>
/// <remarks>
/// <para>
/// Displays search results from operations like "Find All References" or "Find in Disassembly".
/// Results can be sorted by address or content and double-clicked to navigate to the location.
/// </para>
/// <para>
/// Supports multi-selection for batch operations and provides context menu actions
/// for adding watches and toggling breakpoints.
/// </para>
/// </remarks>
public sealed class FindResultListViewModel : DisposableViewModel {
	/// <summary>Gets or sets the list of find results.</summary>
	[Reactive] public NexenList<FindResultViewModel> FindResults { get; private set; } = new();

	/// <summary>Gets or sets the selection model for multi-select support.</summary>
	[Reactive] public SelectionModel<FindResultViewModel?> Selection { get; set; } = new() { SingleSelect = false };

	/// <summary>Gets or sets the current sort state for the results list.</summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>Gets the column widths from configuration.</summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Debugger.FindResultColumnWidths;

	/// <summary>Gets the parent debugger ViewModel.</summary>
	public DebuggerWindowViewModel Debugger { get; }

	private CodeLineData[] _results = [];
	private string _format;

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public FindResultListViewModel() : this(new()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="FindResultListViewModel"/> class.
	/// </summary>
	/// <param name="debugger">The parent debugger window ViewModel.</param>
	public FindResultListViewModel(DebuggerWindowViewModel debugger) {
		Debugger = debugger;

		_format = "X" + debugger.CpuType.GetAddressSize();
		SortState.SetColumnSort("Address", ListSortDirection.Ascending, true);
	}

	/// <summary>
	/// Sorts the results based on the current sort state.
	/// </summary>
	/// <param name="param">Unused parameter.</param>
	public void Sort(object? param) {
		UpdateResults(FindResults);
	}

	/// <summary>
	/// Sets new search results and displays the results panel.
	/// </summary>
	/// <param name="results">The search results to display.</param>
	public void SetResults(IEnumerable<FindResultViewModel> results) {
		Selection.Clear();
		UpdateResults(results);
		Selection.SelectedIndex = 0;
		Dispatcher.UIThread.Post(() =>        //TODOv2 - run this in a post to bypass a bug that might be fixed in the latest Avalonia preview, to re-test after upgrading
			Debugger.OpenTool(Debugger.DockFactory.FindResultListTool));
	}

	private Dictionary<string, Func<FindResultViewModel, FindResultViewModel, int>> _comparers = new() {
		{ "Address", (a, b) => string.Compare(a.Address, b.Address, StringComparison.OrdinalIgnoreCase) },
		{ "Result", (a, b) => string.Compare(a.Text, b.Text, StringComparison.OrdinalIgnoreCase) },
	};

	/// <summary>
	/// Updates and sorts the results list.
	/// </summary>
	private void UpdateResults(IEnumerable<FindResultViewModel> results) {
		List<int> selectedIndexes = Selection.SelectedIndexes.ToList();
		List<FindResultViewModel> sortedResults = results.ToList();

		SortHelper.SortList(sortedResults, SortState.SortOrder, _comparers, "Address");

		FindResults.Replace(sortedResults);
		Selection.SelectIndexes(selectedIndexes, FindResults.Count);
	}

	/// <summary>
	/// Navigates to the location of a search result.
	/// </summary>
	/// <param name="result">The result to navigate to.</param>
	public void GoToResult(FindResultViewModel result) {
		if (result.Location.RelAddress?.Address >= 0) {
			Debugger.ScrollToAddress(result.Location.RelAddress.Value.Address);
		} else if (result.Location.SourceLocation is not null) {
			Debugger.SourceView?.ScrollToLocation(result.Location.SourceLocation.Value);
		}
	}

	/// <summary>
	/// Initializes the context menu with watch and breakpoint actions.
	/// </summary>
	/// <param name="parent">The parent control for the context menu.</param>
	public void InitContextMenu(Control parent) {
		AddDisposables(DebugShortcutManager.CreateContextMenu(parent, new object[] {
			new ContextMenuAction() {
				ActionType = ActionType.AddWatch,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FindResultList_AddWatch),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is FindResultViewModel vm && vm.Location.RelAddress?.Address >= 0,
				OnClick = () => {
					if(Selection.SelectedItem is FindResultViewModel vm && vm.Location.RelAddress?.Address > 0) {
						WatchManager.GetWatchManager(Debugger.CpuType).AddWatch("[$" + vm.Location.RelAddress?.Address.ToString(_format) + "]");
					}
				}
			},
			new ContextMenuAction() {
				ActionType = ActionType.ToggleBreakpoint,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FindResultList_ToggleBreakpoint),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is FindResultViewModel vm && (vm.Location.RelAddress?.Address >= 0 || vm.Location.AbsAddress?.Address >= 0),
				OnClick = () => {
					if(Selection.SelectedItem is FindResultViewModel vm) {
						if(vm.Location.AbsAddress?.Address > 0) {
							BreakpointManager.ToggleBreakpoint(vm.Location.AbsAddress.Value, Debugger.CpuType);
						} else if(vm.Location.RelAddress?.Address > 0) {
							AddressInfo relAddress = vm.Location.RelAddress.Value;
							AddressInfo absAddress = DebugApi.GetAbsoluteAddress(relAddress);
							if(absAddress.Address >= 0) {
								BreakpointManager.ToggleBreakpoint(absAddress, Debugger.CpuType);
							} else if(relAddress.Address >= 0) {
								BreakpointManager.ToggleBreakpoint(relAddress, Debugger.CpuType);
							}
						}
					}
				}
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.GoToLocation,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.FindResultList_GoToLocation),
				IsEnabled = () => Selection.SelectedItems.Count == 1 && Selection.SelectedItem is FindResultViewModel vm,
				OnClick = () => {
					if(Selection.SelectedItem is FindResultViewModel vm) {
						GoToResult(vm);
					}
				}
			}
		}));
	}
}

/// <summary>
/// Represents a single search result with location and display information.
/// </summary>
public sealed class FindResultViewModel {
	/// <summary>Gets the location information for navigation.</summary>
	public LocationInfo Location { get; }

	/// <summary>Gets the formatted address string.</summary>
	public string Address { get; }

	/// <summary>Gets the result text/code.</summary>
	public string Text { get; }

	/// <summary>
	/// Initializes a new instance from location info and display strings.
	/// </summary>
	/// <param name="location">The location information.</param>
	/// <param name="loc">The formatted address string.</param>
	/// <param name="text">The result text.</param>
	public FindResultViewModel(LocationInfo location, string loc, string text) {
		Location = location;
		Address = loc;
		Text = text;
	}

	/// <summary>
	/// Initializes a new instance from a disassembly code line.
	/// </summary>
	/// <param name="line">The code line data.</param>
	public FindResultViewModel(CodeLineData line) {
		Location = new LocationInfo() {
			RelAddress = new AddressInfo() { Address = line.Address, Type = line.CpuType.ToMemoryType() },
			AbsAddress = line.AbsoluteAddress
		};

		string format = "X" + line.CpuType.GetAddressSize();
		Address = "$" + line.Address.ToString(format);
		Text = line.Text;
		if (line.EffectiveAddress >= 0) {
			Text += " " + line.GetEffectiveAddressString(format, out _);
		}
	}
}
