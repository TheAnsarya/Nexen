using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Threading;
using Avalonia.VisualTree;
using Nexen.Controls.DataGridExtensions;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the profiler window.
/// Displays function call profiling data including call counts, execution times, and percentages
/// for performance analysis of emulated code.
/// </summary>
public sealed class ProfilerWindowViewModel : DisposableViewModel {
	/// <summary>
	/// Gets or sets the list of profiler tabs, one per supported CPU type.
	/// </summary>
	[Reactive] public List<ProfilerTab> ProfilerTabs { get; set; } = [];

	/// <summary>
	/// Gets or sets the currently selected profiler tab.
	/// </summary>
	[Reactive] public ProfilerTab? SelectedTab { get; set; } = null;

	/// <summary>
	/// Gets the menu actions for the File menu.
	/// </summary>
	public List<object> FileMenuActions { get; } = new();

	/// <summary>
	/// Gets the menu actions for the View menu.
	/// </summary>
	public List<object> ViewMenuActions { get; } = new();

	/// <summary>
	/// Gets the profiler configuration settings.
	/// </summary>
	public ProfilerConfig Config { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="ProfilerWindowViewModel"/> class.
	/// </summary>
	/// <param name="wnd">The parent window, used for shortcut registration and menu setup.</param>
	public ProfilerWindowViewModel(Window? wnd) {
		Config = ConfigManager.Config.Debug.Profiler;

		if (Design.IsDesignMode) {
			return;
		}

		UpdateAvailableTabs();

		AddDisposable(this.WhenAnyValue(x => x.SelectedTab).Subscribe(x => {
			if (SelectedTab is not null && EmuApi.IsPaused()) {
				RefreshData();
			}
		}));

		FileMenuActions = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.ResetProfilerData,
				OnClick = () => SelectedTab?.ResetData()
			},
			new ContextMenuAction() {
				ActionType = ActionType.CopyToClipboard,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.Copy),
				OnClick = () => wnd?.GetVisualDescendants().Where(a => a is DataGrid).Cast<DataGrid>().FirstOrDefault()?.CopyToClipboard()
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd?.Close()
			}
		});

		ViewMenuActions = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Refresh,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.Refresh),
				OnClick = () => RefreshData()
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.EnableAutoRefresh,
				IsSelected = () => Config.AutoRefresh,
				OnClick = () => Config.AutoRefresh = !Config.AutoRefresh
			},
			new ContextMenuAction() {
				ActionType = ActionType.RefreshOnBreakPause,
				IsSelected = () => Config.RefreshOnBreakPause,
				OnClick = () => Config.RefreshOnBreakPause = !Config.RefreshOnBreakPause
			}
		});

		if (Design.IsDesignMode || wnd is null) {
			return;
		}

		DebugShortcutManager.RegisterActions(wnd, FileMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuActions);

		LabelManager.OnLabelUpdated += LabelManager_OnLabelUpdated;
	}

	/// <summary>
	/// Disposes resources and unsubscribes from label manager events.
	/// </summary>
	protected override void DisposeView() {
		LabelManager.OnLabelUpdated -= LabelManager_OnLabelUpdated;
	}

	private void LabelManager_OnLabelUpdated(object? sender, EventArgs e) {
		ProfilerTab tab = SelectedTab ?? ProfilerTabs[0];
		Dispatcher.UIThread.Post(() => tab?.RefreshGrid());
	}

	/// <summary>
	/// Updates the available profiler tabs based on the loaded ROM's CPU types.
	/// Creates one tab for each CPU type that supports call stack tracking.
	/// </summary>
	public void UpdateAvailableTabs() {
		List<ProfilerTab> tabs = new();
		foreach (CpuType type in EmuApi.GetRomInfo().CpuTypes) {
			if (type.SupportsCallStack()) {
				tabs.Add(new ProfilerTab() {
					TabName = ResourceHelper.GetEnumText(type),
					CpuType = type
				});
			}
		}

		ProfilerTabs = tabs;
		SelectedTab = tabs[0];
	}

	/// <summary>
	/// Refreshes profiler data from the emulator core and updates the display grid.
	/// </summary>
	public void RefreshData() {
		ProfilerTab tab = SelectedTab ?? ProfilerTabs[0];
		tab.RefreshData();
		Dispatcher.UIThread.Post(() => tab.RefreshGrid());
	}
}

/// <summary>
/// Represents a profiler tab for a specific CPU type.
/// Contains profiler data grid and sorting functionality for that CPU's profiled functions.
/// </summary>
public sealed class ProfilerTab : ReactiveObject {
	/// <summary>
	/// Gets or sets the display name for this tab (typically the CPU type name).
	/// </summary>
	[Reactive] public string TabName { get; set; } = "";

	/// <summary>
	/// Gets or sets the CPU type this tab displays profiling data for.
	/// </summary>
	[Reactive] public CpuType CpuType { get; set; } = CpuType.Snes;

	/// <summary>
	/// Gets or sets the list of profiled function view models for data grid binding.
	/// </summary>
	[Reactive] public NexenList<ProfiledFunctionViewModel> GridData { get; private set; } = new();

	/// <summary>
	/// Gets or sets the selection model for the data grid.
	/// </summary>
	[Reactive] public SelectionModel<ProfiledFunctionViewModel> Selection { get; set; } = new();

	/// <summary>
	/// Gets or sets the current sort state for the data grid.
	/// </summary>
	[Reactive] public SortState SortState { get; set; } = new();

	/// <summary>
	/// Gets the profiler configuration settings.
	/// </summary>
	public ProfilerConfig Config => ConfigManager.Config.Debug.Profiler;

	/// <summary>
	/// Gets the list of column widths for grid persistence.
	/// </summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.Profiler.ColumnWidths;

	/// <summary>Lock object for thread-safe data updates.</summary>
	private object _updateLock = new();

	/// <summary>Number of entries in the current profiler data.</summary>
	private int _dataSize = 0;

	/// <summary>Buffer for profiler data from the emulator core.</summary>
	private ProfiledFunction[] _coreProfilerData = new ProfiledFunction[100000];

	/// <summary>Working copy of profiler data for display.</summary>
	private ProfiledFunction[] _profilerData = [];

	/// <summary>Total CPU cycles across all profiled functions (for percentage calculations).</summary>
	private UInt64 _totalCycles;

	/// <summary>
	/// Initializes a new instance of the <see cref="ProfilerTab"/> class.
	/// </summary>
	public ProfilerTab() {
		SortState.SetColumnSort("InclusiveTime", ListSortDirection.Descending, false);
	}

	/// <summary>
	/// Gets the raw profiled function data at the specified index.
	/// </summary>
	/// <param name="index">The index into the profiler data array.</param>
	/// <returns>The profiled function data, or null if the index is out of range.</returns>
	public ProfiledFunction? GetRawData(int index) {
		ProfiledFunction[] data = _profilerData;
		if (index < data.Length) {
			return data[index];
		}

		return null;
	}

	/// <summary>
	/// Resets all profiler data to zero and refreshes the display.
	/// </summary>
	public void ResetData() {
		DebugApi.ResetProfiler(CpuType);
		GridData.Clear();
		RefreshData();
		RefreshGrid();
	}

	/// <summary>
	/// Fetches new profiler data from the emulator core.
	/// Thread-safe; data is stored in internal buffers.
	/// </summary>
	public void RefreshData() {
		lock (_updateLock) {
			_dataSize = DebugApi.GetProfilerData(CpuType, ref _coreProfilerData);
		}
	}

	/// <summary>
	/// Updates the grid display with the latest profiler data.
	/// Copies data from core buffer, sorts it, and updates view model instances.
	/// </summary>
	public void RefreshGrid() {
		lock (_updateLock) {
			Array.Resize(ref _profilerData, _dataSize);
			Array.Copy(_coreProfilerData, _profilerData, _dataSize);
		}

		Sort();

		UInt64 totalCycles = 0;
		ProfiledFunction[] profilerData = _profilerData;
		foreach (ProfiledFunction f in profilerData) {
			totalCycles += f.ExclusiveCycles;
		}

		_totalCycles = totalCycles;

		while (GridData.Count < profilerData.Length) {
			GridData.Add(new ProfiledFunctionViewModel());
		}

		for (int i = 0; i < profilerData.Length; i++) {
			GridData[i].Update(profilerData[i], CpuType, _totalCycles);
		}
	}

	/// <summary>
	/// Command handler for data grid sort operations.
	/// </summary>
	/// <param name="param">Command parameter (unused).</param>
	public void SortCommand(object? param) {
		RefreshGrid();
	}

	/// <summary>
	/// Sorts the profiler data array according to the current sort state.
	/// </summary>
	public void Sort() {
		CpuType cpuType = CpuType;

		Dictionary<string, Func<ProfiledFunction, ProfiledFunction, int>> comparers = new() {
			{ "FunctionName", (a, b) => string.Compare(a.GetFunctionName(cpuType), b.GetFunctionName(cpuType), StringComparison.OrdinalIgnoreCase) },
			{ "CallCount", (a, b) => a.CallCount.CompareTo(b.CallCount) },
			{ "InclusiveTime", (a, b) => a.InclusiveCycles.CompareTo(b.InclusiveCycles) },
			{ "InclusiveTimePercent", (a, b) => a.InclusiveCycles.CompareTo(b.InclusiveCycles) },
			{ "ExclusiveTime", (a, b) => a.ExclusiveCycles.CompareTo(b.ExclusiveCycles) },
			{ "ExclusiveTimePercent", (a, b) => a.ExclusiveCycles.CompareTo(b.ExclusiveCycles) },
			{ "AvgCycles", (a, b) => a.GetAvgCycles().CompareTo(b.GetAvgCycles()) },
			{ "MinCycles", (a, b) => a.MinCycles.CompareTo(b.MinCycles) },
			{ "MaxCycles", (a, b) => a.MaxCycles.CompareTo(b.MaxCycles) },
		};

		SortHelper.SortArray(_profilerData, SortState.SortOrder, comparers, "InclusiveTime");
	}
}

/// <summary>
/// Extension methods for <see cref="ProfiledFunction"/> to provide display formatting.
/// </summary>
public static class ProfiledFunctionExtensions {
	/// <summary>
	/// Gets a display-friendly name for a profiled function, including its label, address, and flags.
	/// </summary>
	/// <param name="func">The profiled function data.</param>
	/// <param name="cpuType">The CPU type for address formatting.</param>
	/// <returns>A formatted function name string (e.g., "MyLabel ($1234)" or "[irq] $8000").</returns>
	public static string GetFunctionName(this ProfiledFunction func, CpuType cpuType) {
		string functionName;

		if (func.Address.Address == -1) {
			functionName = "[Reset]";
		} else {
			CodeLabel? label = LabelManager.GetLabel((UInt32)func.Address.Address, func.Address.Type);

			int hexCount = cpuType.GetAddressSize();
			functionName = func.Address.Type.GetShortName() + ": $" + func.Address.Address.ToString("X" + hexCount.ToString());
			if (label is not null) {
				functionName = label.Label + " (" + functionName + ")";
			}
		}

		if (func.Flags.HasFlag(StackFrameFlags.Irq)) {
			functionName = "[irq] " + functionName;
		} else if (func.Flags.HasFlag(StackFrameFlags.Nmi)) {
			functionName = "[nmi] " + functionName;
		}

		return functionName;
	}
}

/// <summary>
/// ViewModel for a single profiled function row in the profiler data grid.
/// Provides formatted display properties for function statistics.
/// </summary>
public sealed class ProfiledFunctionViewModel : INotifyPropertyChanged {
	/// <summary>
	/// Event raised when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Gets the display name of the profiled function.
	/// </summary>
	public string FunctionName {
		get {
			UpdateFields();
			return field;
		}

		private set;
	} = "";

	/// <summary>
	/// Gets the exclusive (self) CPU cycles formatted as a string.
	/// </summary>
	public string ExclusiveCycles { get; set; } = "";

	/// <summary>
	/// Gets the inclusive (self + called functions) CPU cycles formatted as a string.
	/// </summary>
	public string InclusiveCycles { get; set; } = "";

	/// <summary>
	/// Gets the number of times this function was called.
	/// </summary>
	public string CallCount { get; set; } = "";

	/// <summary>
	/// Gets the minimum cycles for a single call to this function.
	/// </summary>
	public string MinCycles { get; set; } = "";

	/// <summary>
	/// Gets the maximum cycles for a single call to this function.
	/// </summary>
	public string MaxCycles { get; set; } = "";

	/// <summary>
	/// Gets the exclusive time as a percentage of total execution time.
	/// </summary>
	public string ExclusivePercent { get; set; } = "";

	/// <summary>
	/// Gets the inclusive time as a percentage of total execution time.
	/// </summary>
	public string InclusivePercent { get; set; } = "";

	/// <summary>
	/// Gets the average cycles per call to this function.
	/// </summary>
	public string AvgCycles { get; set; } = "";

	/// <summary>Cached profiled function data from the emulator core.</summary>
	private ProfiledFunction _funcData;

	/// <summary>CPU type for address formatting.</summary>
	private CpuType _cpuType;

	/// <summary>Total cycles across all functions for percentage calculations.</summary>
	private UInt64 _totalCycles;

	/// <summary>
	/// Updates this view model with new profiled function data.
	/// </summary>
	/// <param name="func">The profiled function data from the emulator core.</param>
	/// <param name="cpuType">The CPU type for address formatting.</param>
	/// <param name="totalCycles">Total cycles across all functions for percentage calculations.</param>
	public void Update(ProfiledFunction func, CpuType cpuType, UInt64 totalCycles) {
		_funcData = func;
		_cpuType = cpuType;
		_totalCycles = totalCycles;

		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.FunctionName)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.ExclusiveCycles)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.InclusiveCycles)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.CallCount)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.MinCycles)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.MaxCycles)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.ExclusivePercent)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.InclusivePercent)));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ProfiledFunctionViewModel.AvgCycles)));
	}

	/// <summary>
	/// Recalculates all display field values from the cached profiled function data.
	/// </summary>
	private void UpdateFields() {
		FunctionName = _funcData.GetFunctionName(_cpuType);
		ExclusiveCycles = _funcData.ExclusiveCycles.ToString();
		InclusiveCycles = _funcData.InclusiveCycles.ToString();
		CallCount = _funcData.CallCount.ToString();
		MinCycles = _funcData.MinCycles == UInt64.MaxValue ? "n/a" : _funcData.MinCycles.ToString();
		MaxCycles = _funcData.MaxCycles == 0 ? "n/a" : _funcData.MaxCycles.ToString();

		AvgCycles = (_funcData.CallCount == 0 ? 0 : (_funcData.InclusiveCycles / _funcData.CallCount)).ToString();
		ExclusivePercent = ((double)_funcData.ExclusiveCycles / _totalCycles * 100).ToString("0.00");
		InclusivePercent = ((double)_funcData.InclusiveCycles / _totalCycles * 100).ToString("0.00");
	}
}
