using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using Avalonia.Collections;
using Avalonia.Controls;
using Avalonia.Controls.Selection;
using Avalonia.Threading;
using DataBoxControl;
using Nexen.Config;
using Nexen.Debugger.Labels;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the watch expression window.
/// Manages watch tabs for each CPU type to monitor expressions and variables during debugging.
/// </summary>
public class WatchWindowViewModel : ViewModelBase {
	/// <summary>
	/// Gets or sets the list of watch tabs, one per available CPU type.
	/// </summary>
	[Reactive] public List<WatchTab> WatchTabs { get; set; } = new List<WatchTab>();

	/// <summary>
	/// Gets or sets the currently selected watch tab.
	/// </summary>
	[Reactive] public WatchTab SelectedTab { get; set; } = null!;

	/// <summary>
	/// Gets the watch window configuration settings.
	/// </summary>
	public WatchWindowConfig Config { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="WatchWindowViewModel"/> class.
	/// </summary>
	public WatchWindowViewModel() {
		Config = ConfigManager.Config.Debug.WatchWindow;

		if (Design.IsDesignMode) {
			return;
		}

		UpdateAvailableTabs();
	}

	/// <summary>
	/// Updates the available watch tabs based on loaded ROM's CPU types.
	/// Disposes existing tabs and creates new ones for each CPU.
	/// </summary>
	public void UpdateAvailableTabs() {
		foreach (WatchTab tab in WatchTabs) {
			tab.Dispose();
		}

		List<WatchTab> tabs = new();
		foreach (CpuType type in EmuApi.GetRomInfo().CpuTypes) {
			tabs.Add(new WatchTab(type));
		}

		foreach (WatchTab tab in tabs) {
			tab.WatchList.UpdateWatch();
		}

		WatchTabs = tabs;
		SelectedTab = tabs[0];
	}

	/// <summary>
	/// Refreshes the watch data for the currently selected tab.
	/// </summary>
	public void RefreshData() {
		Dispatcher.UIThread.Post(() => {
			WatchTab tab = SelectedTab ?? WatchTabs[0];
			tab.WatchList.UpdateWatch();
		});
	}
}

/// <summary>
/// Represents a single watch tab for a specific CPU type.
/// Contains the watch list for monitoring expressions.
/// </summary>
public class WatchTab : DisposableViewModel {
	/// <summary>
	/// Gets the display name of the tab (CPU type name).
	/// </summary>
	public string TabName { get; }

	/// <summary>
	/// Gets the CPU type this tab monitors.
	/// </summary>
	public CpuType CpuType { get; }

	/// <summary>
	/// Gets the watch list ViewModel for managing watch expressions.
	/// </summary>
	public WatchListViewModel WatchList { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="WatchTab"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to watch.</param>
	public WatchTab(CpuType cpuType) {
		CpuType = cpuType;
		TabName = ResourceHelper.GetEnumText(cpuType);
		WatchList = new WatchListViewModel(cpuType);
	}

	/// <summary>
	/// Disposes the watch list when the tab is disposed.
	/// </summary>
	protected override void DisposeView() {
		WatchList.Dispose();
	}
}
