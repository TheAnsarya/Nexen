using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia.Collections;
using Avalonia.Controls.Selection;
using Nexen.Controls.DataGridExtensions;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.ViewModels; 
/// <summary>
/// ViewModel for the event viewer list panel that shows debug events.
/// </summary>
/// <remarks>
/// <para>
/// Displays a sortable list of debug events (register reads/writes, DMA, IRQ, etc.)
/// captured during emulation. Events are grouped by scanline and cycle position.
/// </para>
/// <para>
/// Works in conjunction with <see cref="EventViewerViewModel"/> which provides the
/// graphical event viewer display.
/// </para>
/// </remarks>
public sealed partial class EventViewerListViewModel : DisposableViewModel {
	/// <summary>Gets the raw debug event data from the emulator.</summary>
	public DebugEventInfo[] RawDebugEvents { get; private set; } = [];

	/// <summary>Gets the list of debug event ViewModels for display.</summary>
	public NexenList<DebugEventViewModel> DebugEvents { get; }

	/// <summary>Gets or sets the selection model for the event list.</summary>
	public SelectionModel<DebugEventViewModel?> Selection { get; set; } = new();

	/// <summary>Gets the parent event viewer ViewModel.</summary>
	public EventViewerViewModel EventViewer { get; }

	/// <summary>Gets or sets the current sort state.</summary>
	[Reactive] public partial SortState SortState { get; set; } = new();

	/// <summary>Gets the column widths from configuration.</summary>
	public List<int> ColumnWidths { get; } = ConfigManager.Config.Debug.EventViewer.ColumnWidths;

	/// <summary>Gets the command to trigger sorting.</summary>
	public ICommand SortCommand { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="EventViewerListViewModel"/> class.
	/// </summary>
	/// <param name="eventViewer">The parent event viewer ViewModel.</param>
	public EventViewerListViewModel(EventViewerViewModel eventViewer) {
		EventViewer = eventViewer;
		DebugEvents = new();

		// Default sort by scanline, then cycle
		SortState.SetColumnSort("Scanline", ListSortDirection.Ascending, false);
		SortState.SetColumnSort("Cycle", ListSortDirection.Ascending, false);

		SortCommand = ReactiveCommand.Create<string?>(sortMemberPath => RefreshList());
	}

	private Dictionary<string, Func<DebugEventInfo, DebugEventInfo, int>> _comparers = new() {
		{ "Color", (a, b) => a.Color.CompareTo(b.Color) },
		{ "ProgramCounter", (a, b) => a.ProgramCounter.CompareTo(b.ProgramCounter) },
		{ "Scanline", (a, b) => a.Scanline.CompareTo(b.Scanline) },
		{ "Cycle", (a, b) => a.Cycle.CompareTo(b.Cycle) },
		{ "Type", (a, b) => a.Type.CompareTo(b.Type) },
		{ "Address", (a, b) => {
			int result = a.Operation.Address.CompareTo(b.Operation.Address);
			return result != 0 ? result : a.RegisterId.CompareTo(b.RegisterId);
		} },
		{ "Value", (a, b) => a.Operation.Value.CompareTo(b.Operation.Value)},
		{ "Default", (a, b) => {
			int result = a.Scanline.CompareTo(b.Scanline);
			return result != 0 ? result : a.Cycle.CompareTo(b.Cycle);
		} }
	};

	/// <summary>
	/// Refreshes the event list from the emulator and applies sorting.
	/// </summary>
	/// <remarks>
	/// Efficiently updates existing ViewModels when possible to minimize allocations.
	/// </remarks>
	public void RefreshList() {
		RawDebugEvents = DebugApi.GetDebugEvents(EventViewer.CpuType);

		SortHelper.SortArray(RawDebugEvents, SortState.SortOrder, _comparers, "Default");

		// Efficient update: reuse existing ViewModels where possible
		if (DebugEvents.Count < RawDebugEvents.Length) {
			for (int i = 0; i < DebugEvents.Count; i++) {
				DebugEvents[i].Update(RawDebugEvents, i, EventViewer.CpuType);
			}

			DebugEvents.AddRange(Enumerable.Range(DebugEvents.Count, RawDebugEvents.Length - DebugEvents.Count).Select(i => new DebugEventViewModel(RawDebugEvents, i, EventViewer.CpuType)));
		} else if (DebugEvents.Count > RawDebugEvents.Length) {
			for (int i = 0; i < RawDebugEvents.Length; i++) {
				DebugEvents[i].Update(RawDebugEvents, i, EventViewer.CpuType);
			}

			DebugEvents.RemoveRange(RawDebugEvents.Length, DebugEvents.Count - RawDebugEvents.Length);
		} else {
			for (int i = 0; i < DebugEvents.Count; i++) {
				DebugEvents[i].Update(RawDebugEvents, i, EventViewer.CpuType);
			}
		}
	}
}
