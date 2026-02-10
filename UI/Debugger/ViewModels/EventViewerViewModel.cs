using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics.CodeAnalysis;
using System.Reactive.Linq;
using System.Reflection;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.Controls;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the event viewer window that displays debug events graphically.
/// </summary>
/// <remarks>
/// <para>
/// The event viewer provides a visual representation of debug events (register reads/writes,
/// DMA transfers, interrupts, etc.) mapped to their scanline/cycle positions. Events are
/// displayed as colored dots on a bitmap that represents the frame's timing.
/// </para>
/// <para>
/// This ViewModel manages:
/// <list type="bullet">
/// <item>The graphical event bitmap display</item>
/// <item>Console-specific configuration (NES, SNES, GB, PCE, SMS, GBA, WS)</item>
/// <item>Event selection and highlighting</item>
/// <item>Optional list view via <see cref="EventViewerListViewModel"/></item>
/// <item>Menu and toolbar actions</item>
/// </list>
/// </para>
/// </remarks>
public sealed class EventViewerViewModel : DisposableViewModel {
	/// <summary>
	/// Flag value OR'd with DMA channel number to indicate HDMA (horizontal DMA) on SNES.
	/// </summary>
	public const int HdmaChannelFlag = 0x40;

	/// <summary>Gets or sets the CPU type being debugged.</summary>
	[Reactive] public CpuType CpuType { get; set; }

	/// <summary>Gets or sets the bitmap displaying the event viewer visualization.</summary>
	[Reactive] public DynamicBitmap ViewerBitmap { get; private set; }

	/// <summary>Gets or sets the console-specific configuration for the event viewer.</summary>
	[Reactive] public ViewModelBase ConsoleConfig { get; set; }

	/// <summary>Gets or sets the grid highlight position for row/column indication.</summary>
	[Reactive] public GridRowColumn? GridHighlightPoint { get; set; }

	/// <summary>Gets or sets whether the list view is visible.</summary>
	[Reactive] public bool ShowListView { get; set; }

	/// <summary>Gets or sets the minimum height of the list view panel.</summary>
	[Reactive] public double MinListViewHeight { get; set; }

	/// <summary>Gets or sets the current height of the list view panel.</summary>
	[Reactive] public double ListViewHeight { get; set; }

	/// <summary>Tracks the last time the list view was refreshed to throttle updates.</summary>
	private DateTime _lastListRefresh = DateTime.MinValue;

	/// <summary>Gets or sets the currently selected debug event.</summary>
	[Reactive] public DebugEventInfo? SelectedEvent { get; set; }

	/// <summary>Gets or sets the selection rectangle for highlighting the selected event.</summary>
	[Reactive] public Rect SelectionRect { get; set; }

	/// <summary>Gets the list view ViewModel for tabular event display.</summary>
	public EventViewerListViewModel ListView { get; }

	/// <summary>Gets the event viewer configuration.</summary>
	public EventViewerConfig Config { get; }

	/// <summary>Gets or sets the File menu items.</summary>
	[Reactive] public List<object> FileMenuItems { get; private set; } = new();

	/// <summary>Gets or sets the Debug menu items (step actions).</summary>
	[Reactive] public List<ContextMenuAction> DebugMenuItems { get; private set; } = new();

	/// <summary>Gets or sets the View menu items.</summary>
	[Reactive] public List<object> ViewMenuItems { get; private set; } = new();

	/// <summary>Gets or sets the toolbar items.</summary>
	[Reactive] public List<ContextMenuAction> ToolbarItems { get; private set; } = new();

	/// <summary>The picture viewer control for zoom functionality.</summary>
	private PictureViewer _picViewer;

	/// <summary>Flag to prevent redundant refresh operations.</summary>
	/// <summary>Flag to prevent redundant refresh operations.</summary>
	private bool _refreshPending;

	/// <summary>
	/// Initializes a new instance for the designer.
	/// </summary>
	[Obsolete("For designer only")]
	public EventViewerViewModel() : this(CpuType.Nes, new PictureViewer(), null!, null) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="EventViewerViewModel"/> class.
	/// </summary>
	/// <param name="cpuType">The CPU type to display events for.</param>
	/// <param name="picViewer">The picture viewer control for zoom operations.</param>
	/// <param name="listView">The DataGrid control for the list view.</param>
	/// <param name="wnd">The parent window for menu registration.</param>
	public EventViewerViewModel(CpuType cpuType, PictureViewer picViewer, DataGrid listView, Window? wnd) {
		CpuType = cpuType;
		ListView = new EventViewerListViewModel(this);

		ListView.Selection.SelectionChanged += Selection_SelectionChanged;

		_picViewer = picViewer;
		Config = ConfigManager.Config.Debug.EventViewer;
		ShowListView = Config.ShowListView;

		InitBitmap(new FrameInfo() { Width = 1, Height = 1 });
		InitForCpuType();

		FileMenuItems = AddDisposables(new List<object>() {
			new ContextMenuAction() {
				ActionType = ActionType.Exit,
				OnClick = () => wnd?.Close()
			}
		});

		ViewMenuItems = AddDisposables(new List<object>() {
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
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.ShowSettingsPanel,
				Shortcut =  () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ToggleSettingsPanel),
				IsSelected = () => Config.ShowSettingsPanel,
				OnClick = () => Config.ShowSettingsPanel = !Config.ShowSettingsPanel
			},
			new ContextMenuAction() {
				ActionType = ActionType.ShowListView,
				IsSelected = () => Config.ShowListView,
				OnClick = () => Config.ShowListView = !Config.ShowListView
			},
			new ContextMenuAction() {
				ActionType = ActionType.ShowToolbar,
				IsSelected = () => Config.ShowToolbar,
				OnClick = () => Config.ShowToolbar = !Config.ShowToolbar
			},
			new ContextMenuSeparator(),
			new ContextMenuAction() {
				ActionType = ActionType.ZoomIn,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomIn),
				OnClick = () => _picViewer.ZoomIn()
			},
			new ContextMenuAction() {
				ActionType = ActionType.ZoomOut,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.ZoomOut),
				OnClick = () => _picViewer.ZoomOut()
			},
		});

		if (Design.IsDesignMode || wnd is null) {
			return;
		}

		AddDisposables(DebugShortcutManager.CreateContextMenu(_picViewer, GetContextMenuActions()));
		AddDisposables(DebugShortcutManager.CreateContextMenu(listView, GetContextMenuActions()));

		UpdateConfig();
		RefreshData();

		DebugMenuItems = AddDisposables(DebugSharedActions.GetStepActions(wnd, () => CpuType));
		ToolbarItems = AddDisposables(DebugSharedActions.GetStepActions(wnd, () => CpuType));

		AddDisposable(this.WhenAnyValue(x => x.CpuType).Subscribe(_ => {
			InitForCpuType();
			UpdateConfig();
			RefreshData();
		}));

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => {
			UpdateConfig();
			RefreshUi(false);
		}));

		AddDisposable(this.WhenAnyValue(x => x.ShowListView).Subscribe(showListView => {
			Config.ShowListView = showListView;
			ListViewHeight = showListView ? Config.ListViewHeight : 0;
			MinListViewHeight = showListView ? 100 : 0;
			RefreshUi(false);
		}));

		AddDisposable(this.WhenAnyValue(x => x.ListViewHeight).Subscribe(height => {
			if (ShowListView) {
				Config.ListViewHeight = height;
			} else {
				ListViewHeight = 0;
			}
		}));

		DebugShortcutManager.RegisterActions(wnd, FileMenuItems);
		DebugShortcutManager.RegisterActions(wnd, DebugMenuItems);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuItems);
	}

	/// <summary>
	/// Creates the context menu actions for the event viewer.
	/// </summary>
	/// <returns>List of context menu actions for viewing in debugger and toggling breakpoints.</returns>
	private List<ContextMenuAction> GetContextMenuActions() {
		return new List<ContextMenuAction> {
			new ContextMenuAction() {
				ActionType = ActionType.ViewInDebugger,
				IsEnabled = () => SelectedEvent is not null,
				HintText = () => SelectedEvent is not null ? $"PC -${SelectedEvent.Value.ProgramCounter:X4}" : "",
				OnClick = () => {
					if(SelectedEvent is not null) {
						DebuggerWindow.OpenWindowAtAddress(CpuType, (int)SelectedEvent.Value.ProgramCounter);
					}
				}
			},
			new ContextMenuAction() {
				ActionType = ActionType.ToggleBreakpoint,
				IsEnabled = () => SelectedEvent is not null,
				HintText = () => SelectedEvent is not null ? $"PC - ${SelectedEvent.Value.ProgramCounter:X4}" : "",
				OnClick = () => {
					if(SelectedEvent is not null) {
						int addr = (int)SelectedEvent.Value.ProgramCounter;
						BreakpointManager.ToggleBreakpoint(new AddressInfo() { Address = addr, Type = CpuType.ToMemoryType() }, CpuType);
					}
				}
			},
			new ContextMenuAction() {
				ActionType = ActionType.ToggleBreakpoint,
				IsEnabled = () => SelectedEvent?.Flags.HasFlag(EventFlags.ReadWriteOp) == true,
				HintText = () => SelectedEvent?.Flags.HasFlag(EventFlags.ReadWriteOp) == true ? $"Address - ${SelectedEvent.Value.Operation.Address:X4}" : "",
				OnClick = () => {
					if(SelectedEvent?.Flags.HasFlag(EventFlags.ReadWriteOp) == true) {
						int addr = (int)SelectedEvent.Value.Operation.Address;
						BreakpointManager.ToggleBreakpoint(new AddressInfo() { Address = addr, Type = CpuType.ToMemoryType() }, CpuType);
					}
				}
			},
		};
	}

	/// <summary>
	/// Handles selection changes in the list view.
	/// </summary>
	/// <param name="sender">The selection model.</param>
	/// <param name="e">Selection change event arguments.</param>
	private void Selection_SelectionChanged(object? sender, Avalonia.Controls.Selection.SelectionModelSelectionChangedEventArgs<DebugEventViewModel?> e) {
		if (e.SelectedItems.Count > 0 && e.SelectedItems[0] is DebugEventViewModel evt) {
			UpdateSelectedEvent(evt.RawEvent);
		} else {
			SelectionRect = default;
		}
	}

	/// <summary>
	/// Updates the selected event and positions the selection rectangle.
	/// </summary>
	/// <param name="evt">The debug event to select, or null to clear selection.</param>
	public void UpdateSelectedEvent(DebugEventInfo? evt) {
		if (evt is not null) {
			PixelPoint p = GetEventLocation(evt.Value);
			SelectedEvent = evt;
			SelectionRect = new Rect(p.X - 2, p.Y - 2, 6, 6);
		} else {
			SelectedEvent = null;
			SelectionRect = default;
		}
	}

	/// <summary>
	/// Initializes the console-specific configuration based on CPU type.
	/// </summary>
	[MemberNotNull(nameof(EventViewerViewModel.ConsoleConfig))]
	private void InitForCpuType() {
		ConsoleConfig = CpuType switch {
			CpuType.Snes => Config.SnesConfig,
			CpuType.Nes => Config.NesConfig,
			CpuType.Gameboy => Config.GbConfig,
			CpuType.Pce => Config.PceConfig,
			CpuType.Sms => Config.SmsConfig,
			CpuType.Gba => Config.GbaConfig,
			CpuType.Ws => Config.WsConfig,
			_ => throw new Exception("Invalid cpu type")
		};
	}

	/// <summary>
	/// Initializes the bitmap using the current CPU type's display size.
	/// </summary>
	private void InitBitmap() {
		InitBitmap(DebugApi.GetEventViewerDisplaySize(CpuType));
	}

	/// <summary>
	/// Initializes or resizes the viewer bitmap to match the specified frame size.
	/// </summary>
	/// <param name="size">The frame dimensions for the bitmap.</param>
	[MemberNotNull(nameof(ViewerBitmap))]
	private void InitBitmap(FrameInfo size) {
		if (ViewerBitmap is null || ViewerBitmap.Size.Width != size.Width || ViewerBitmap.Size.Height != size.Height) {
			ViewerBitmap?.Dispose();
			ViewerBitmap = new DynamicBitmap(new PixelSize((int)size.Width, (int)size.Height), new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);
		}
	}

	/// <summary>
	/// Refreshes the event data by taking a new snapshot from the emulator.
	/// </summary>
	/// <param name="forAutoRefresh">Whether this is an auto-refresh (throttles list updates).</param>
	public void RefreshData(bool forAutoRefresh = false) {
		DebugApi.TakeEventSnapshot(CpuType, forAutoRefresh);
		Dispatcher.UIThread.Post(() => {
			SelectionRect = default;
			SelectedEvent = null;
		});

		RefreshUi(forAutoRefresh);
	}

	/// <summary>
	/// Queues a UI refresh on the dispatcher thread.
	/// </summary>
	/// <param name="forAutoRefresh">Whether this is an auto-refresh.</param>
	/// <remarks>
	/// Prevents redundant refresh operations by checking <see cref="_refreshPending"/>.
	/// </remarks>
	public void RefreshUi(bool forAutoRefresh) {
		if (_refreshPending) {
			return;
		}

		_refreshPending = true;
		Dispatcher.UIThread.Post(() => {
			InternalRefreshUi(forAutoRefresh);
			_refreshPending = false;
		});
	}

	/// <summary>
	/// Performs the actual UI refresh - updates the bitmap and optionally the list view.
	/// </summary>
	/// <param name="forAutoRefresh">Whether this is an auto-refresh (throttles list updates).</param>
	private void InternalRefreshUi(bool forAutoRefresh) {
		if (Disposed) {
			return;
		}

		InitBitmap();
		using (var bitmapLock = ViewerBitmap.Lock()) {
			DebugApi.GetEventViewerOutput(CpuType, bitmapLock.FrameBuffer.Address, (uint)(ViewerBitmap.Size.Width * ViewerBitmap.Size.Height * sizeof(UInt32)));
		}

		if (ShowListView) {
			DateTime now = DateTime.Now;
			if (!forAutoRefresh || (now - _lastListRefresh).TotalMilliseconds >= 66) {
				_lastListRefresh = now;
				ListView.RefreshList();
			}
		}
	}

	/// <summary>
	/// Calculates the pixel location for an event based on its scanline/cycle position.
	/// </summary>
	/// <param name="evt">The debug event.</param>
	/// <returns>The pixel position in the event viewer bitmap.</returns>
	/// <remarks>
	/// Different consoles have different timing characteristics, so the mapping
	/// from cycle/scanline to pixel coordinates varies by CPU type.
	/// </remarks>
	private PixelPoint GetEventLocation(DebugEventInfo evt) {
		return CpuType switch {
			CpuType.Snes => new PixelPoint(evt.Cycle / 2, evt.Scanline * 2),
			CpuType.Nes => new PixelPoint(evt.Cycle * 2, (evt.Scanline + 1) * 2),
			CpuType.Gameboy => new PixelPoint(evt.Cycle * 2, evt.Scanline * 2),
			CpuType.Pce => new PixelPoint(evt.Cycle, evt.Scanline * 2),
			CpuType.Sms => new PixelPoint(evt.Cycle * 2, evt.Scanline * 2),
			CpuType.Gba => new PixelPoint(evt.Cycle, evt.Scanline * 4),
			CpuType.Ws => new PixelPoint(evt.Cycle * 2, evt.Scanline * 2),
			_ => throw new Exception("Invalid cpu type")
		};
	}

	/// <summary>
	/// Updates the grid highlight point for row/column indication on mouse hover.
	/// </summary>
	/// <param name="p">The pixel position of the mouse.</param>
	/// <param name="eventInfo">Optional event to snap the highlight to.</param>
	public void UpdateHighlightPoint(PixelPoint p, DebugEventInfo? eventInfo) {
		if (eventInfo is not null) {
			//Snap the row/column highlight to the selected event
			p = GetEventLocation(eventInfo.Value);
		}

		GridRowColumn result = new GridRowColumn() {
			Y = p.Y / 2 * 2,
			Width = 2,
			Height = 2
		};

		int xPos;
		int yPos;
		switch (CpuType) {
			case CpuType.Snes:
				result.X = p.X;
				xPos = result.X * 2;
				yPos = result.Y / 2;
				break;

			case CpuType.Nes:
				result.X = p.X / 2 * 2;
				xPos = result.X / 2;
				yPos = (result.Y / 2) - 1;
				break;

			case CpuType.Gameboy:
				result.X = p.X / 2 * 2;
				xPos = result.X / 2;
				yPos = result.Y / 2;
				break;

			case CpuType.Pce:
				result.X = p.X;
				xPos = result.X;
				yPos = result.Y / 2;
				break;

			case CpuType.Sms:
				result.X = p.X / 2 * 2;
				xPos = result.X / 2;
				yPos = result.Y / 2;
				break;

			case CpuType.Gba:
				result.X = p.X;
				xPos = result.X;
				yPos = result.Y / 4;
				break;

			case CpuType.Ws:
				result.X = p.X / 2 * 2;
				xPos = result.X / 2;
				yPos = result.Y / 2;
				break;

			default:
				throw new Exception("Invalid cpu type");
		}

		result.DisplayValue = $"X: {xPos}\nY: {yPos}";

		GridHighlightPoint = result;
	}

	/// <summary>
	/// Updates the emulator's event viewer configuration based on the current console config.
	/// </summary>
	public void UpdateConfig() {
		if (ConsoleConfig is SnesEventViewerConfig snesCfg) {
			DebugApi.SetEventViewerConfig(CpuType, snesCfg.ToInterop());
		} else if (ConsoleConfig is NesEventViewerConfig nesCfg) {
			DebugApi.SetEventViewerConfig(CpuType, nesCfg.ToInterop());
		} else if (ConsoleConfig is GbEventViewerConfig gbCfg) {
			DebugApi.SetEventViewerConfig(CpuType, gbCfg.ToInterop());
		} else if (ConsoleConfig is GbaEventViewerConfig gbaCfg) {
			DebugApi.SetEventViewerConfig(CpuType, gbaCfg.ToInterop());
		} else if (ConsoleConfig is PceEventViewerConfig pceCfg) {
			DebugApi.SetEventViewerConfig(CpuType, pceCfg.ToInterop());
		} else if (ConsoleConfig is SmsEventViewerConfig smsCfg) {
			DebugApi.SetEventViewerConfig(CpuType, smsCfg.ToInterop());
		} else if (ConsoleConfig is WsEventViewerConfig wsCfg) {
			DebugApi.SetEventViewerConfig(CpuType, wsCfg.ToInterop());
		}
	}

	/// <summary>
	/// Enables all event type categories via reflection.
	/// </summary>
	[RequiresUnreferencedCode("Uses reflection to access properties dynamically")]
	public void EnableAllEventTypes() {
		foreach (PropertyInfo prop in ConsoleConfig.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
			if (prop.PropertyType == typeof(EventViewerCategoryCfg)) {
				((EventViewerCategoryCfg)prop.GetValue(ConsoleConfig)!).Visible = true;
			}
		}
	}

	/// <summary>
	/// Disables all event type categories via reflection.
	/// </summary>
	[RequiresUnreferencedCode("Uses reflection to access properties dynamically")]
	public void DisableAllEventTypes() {
		foreach (PropertyInfo prop in ConsoleConfig.GetType().GetProperties(BindingFlags.Public | BindingFlags.Instance)) {
			if (prop.PropertyType == typeof(EventViewerCategoryCfg)) {
				((EventViewerCategoryCfg)prop.GetValue(ConsoleConfig)!).Visible = false;
			}
		}
	}

	/// <summary>
	/// Specifies the active tab in the event viewer.
	/// </summary>
	public enum EventViewerTab {
		/// <summary>The graphical PPU timing view.</summary>
		PpuView,
		/// <summary>The tabular list view.</summary>
		ListView,
	}

	/// <summary>
	/// Gets formatted details for a debug event.
	/// </summary>
	/// <param name="cpuType">The CPU type for context-specific formatting.</param>
	/// <param name="evt">The debug event to describe.</param>
	/// <param name="singleLine">Whether to format as single line (for columns) or multi-line (for tooltips).</param>
	/// <returns>Formatted string describing the event's details.</returns>
	/// <remarks>
	/// Includes information such as:
	/// <list type="bullet">
	/// <item>Previous frame indicator</item>
	/// <item>Register write state (first/second write for double-write registers)</item>
	/// <item>Target memory address</item>
	/// <item>Breakpoint details if triggered by breakpoint</item>
	/// <item>DMA/HDMA channel info for SNES</item>
	/// </list>
	/// </remarks>
	public static string GetEventDetails(CpuType cpuType, DebugEventInfo evt, bool singleLine) {
		bool isDma = evt.DmaChannel >= 0 && (evt.Operation.Type == MemoryOperationType.DmaWrite || evt.Operation.Type == MemoryOperationType.DmaRead);

		List<string> details = [];
		if (evt.Flags.HasFlag(EventFlags.PreviousFrame)) {
			details.Add("Previous frame");
		}

		if (evt.Flags.HasFlag(EventFlags.RegSecondWrite)) {
			details.Add("Second register write");
		} else if (evt.Flags.HasFlag(EventFlags.RegFirstWrite)) {
			details.Add("First register write");
		}

		if (evt.Flags.HasFlag(EventFlags.HasTargetMemory)) {
			details.Add("Target: " + evt.TargetMemory.MemType.GetShortName() + " $" + evt.TargetMemory.Address.ToString(evt.TargetMemory.MemType.GetFormatString()));
		}

		if (evt.Type == DebugEventType.Breakpoint && evt.BreakpointId >= 0) {
			Breakpoint? bp = BreakpointManager.GetBreakpointById(evt.BreakpointId);
			if (bp is not null) {
				string bpInfo = "Breakpoint - ";
				bpInfo += "CPU: " + ResourceHelper.GetEnumText(bp.CpuType);
				bpInfo += singleLine ? " - " : Environment.NewLine;
				bpInfo += "Type: " + bp.ToReadableType();
				bpInfo += singleLine ? " - " : Environment.NewLine;
				bpInfo += "Addresses: " + bp.GetAddressString(true);
				if (bp.Condition.Length > 0) {
					bpInfo += singleLine ? " - " : Environment.NewLine;
					bpInfo += "Condition: " + bp.Condition;
				}

				details.Add(bpInfo);
			}
		}

		if (isDma) {
			string dmaInfo = "";
			if (cpuType == CpuType.Snes) {
				bool indirectHdma = false;
				bool isHdma = (evt.DmaChannel & EventViewerViewModel.HdmaChannelFlag) != 0;
				if (isHdma) {
					indirectHdma = evt.DmaChannelInfo.HdmaIndirectAddressing;
					dmaInfo += "HDMA #" + (evt.DmaChannel & 0x07);
					dmaInfo += indirectHdma ? " (indirect)" : "";
					dmaInfo += singleLine ? " - " : Environment.NewLine;
					dmaInfo += "Line Counter: $" + evt.DmaChannelInfo.HdmaLineCounterAndRepeat.ToString("X2");
				} else {
					dmaInfo += "DMA #" + (evt.DmaChannel & 0x07);
				}

				dmaInfo += singleLine ? " - " : Environment.NewLine;
				dmaInfo += "Mode: " + evt.DmaChannelInfo.TransferMode;
				dmaInfo += singleLine ? " - " : Environment.NewLine;

				int aBusAddress = indirectHdma
					? (evt.DmaChannelInfo.HdmaBank << 16) | evt.DmaChannelInfo.TransferSize
					: isHdma
						? (evt.DmaChannelInfo.SrcBank << 16) | evt.DmaChannelInfo.HdmaTableAddress
						: (evt.DmaChannelInfo.SrcBank << 16) | evt.DmaChannelInfo.SrcAddress;
				if (!evt.DmaChannelInfo.InvertDirection) {
					dmaInfo += "$" + aBusAddress.ToString("X4") + " -> $" + (0x2100 | evt.DmaChannelInfo.DestAddress).ToString("X4");
				} else {
					dmaInfo += "$" + aBusAddress.ToString("X4") + " <- $" + (0x2100 | evt.DmaChannelInfo.DestAddress).ToString("X4");
				}
			} else {
				dmaInfo = "DMA Channel #" + evt.DmaChannel;
			}

			details.Add(dmaInfo.Trim());
		}

		if (singleLine) {
			if (details.Count > 1) {
				return "[" + string.Join("] [", details) + "]";
			} else if (details.Count == 1) {
				return details[0];
			} else {
				return "";
			}
		} else {
			return string.Join(Environment.NewLine, details);
		}
	}
}

/// <summary>
/// ViewModel representing a single debug event in the event list.
/// </summary>
/// <remarks>
/// <para>
/// Wraps a <see cref="DebugEventInfo"/> with lazily-formatted display properties.
/// Implements <see cref="INotifyPropertyChanged"/> for efficient binding updates
/// when the underlying event data changes.
/// </para>
/// <para>
/// The fields are updated lazily when <see cref="Color"/> is accessed, which
/// triggers <see cref="UpdateFields"/> to format all display strings.
/// </para>
/// </remarks>
public sealed class DebugEventViewModel : INotifyPropertyChanged {
	/// <summary>The array of events this ViewModel draws from.</summary>
	private DebugEventInfo[] _events = [];

	/// <summary>The index of this event in the events array.</summary>
	private int _index;

	/// <summary>The CPU type for formatting context.</summary>
	private CpuType _cpuType;

	/// <summary>Occurs when a property value changes.</summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>Gets the formatted program counter address.</summary>
	public string ProgramCounter { get; set; } = "";

	/// <summary>Gets the scanline number as a string.</summary>
	public string Scanline { get; set; } = "";

	/// <summary>Gets the cycle number as a string.</summary>
	public string Cycle { get; set; } = "";

	/// <summary>Gets the event type description.</summary>
	public string Type { get; set; } = "";

	/// <summary>Gets the formatted address with label if available.</summary>
	public string Address { get; set; } = "";

	/// <summary>Gets the formatted value for read/write operations.</summary>
	public string Value { get; set; } = "";

	/// <summary>Gets additional event details (DMA info, breakpoint info, etc.).</summary>
	public string Details { get; set; } = "";

	/// <summary>
	/// Gets the event color for display.
	/// </summary>
	/// <remarks>
	/// Accessing this property triggers lazy field updates via <see cref="UpdateFields"/>.
	/// </remarks>
	public UInt32 Color {
		get {
			UpdateFields();
			return field;
		}

		private set;
	} = 0;

	/// <summary>Gets the raw debug event info from the emulator.</summary>
	public DebugEventInfo RawEvent => _events[_index];

	/// <summary>
	/// Initializes a new instance of the <see cref="DebugEventViewModel"/> class.
	/// </summary>
	/// <param name="events">The events array.</param>
	/// <param name="index">The index of this event in the array.</param>
	/// <param name="cpuType">The CPU type for formatting context.</param>
	public DebugEventViewModel(DebugEventInfo[] events, int index, CpuType cpuType) {
		Update(events, index, cpuType);
	}

	/// <summary>
	/// Updates the ViewModel to reference a different event.
	/// </summary>
	/// <param name="events">The events array.</param>
	/// <param name="index">The new index in the array.</param>
	/// <param name="cpuType">The CPU type for formatting context.</param>
	/// <remarks>
	/// Raises <see cref="PropertyChanged"/> for all properties to trigger UI updates.
	/// The actual field values are updated lazily when accessed.
	/// </remarks>
	public void Update(DebugEventInfo[] events, int index, CpuType cpuType) {
		_cpuType = cpuType;
		_events = events;
		_index = index;

		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Color"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("ProgramCounter"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Scanline"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Cycle"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Type"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Address"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Value"));
		PropertyChanged?.Invoke(this, new PropertyChangedEventArgs("Details"));
	}

	/// <summary>
	/// Updates all display fields from the current event data.
	/// </summary>
	/// <remarks>
	/// Called lazily when <see cref="Color"/> is accessed. Formats:
	/// <list type="bullet">
	/// <item>Program counter as hex address with $ prefix</item>
	/// <item>Scanline and cycle as decimal strings</item>
	/// <item>Address with label lookup if available</item>
	/// <item>Register name and ID for register operations</item>
	/// <item>Value as hex for read/write operations</item>
	/// <item>Event type with read/write indicator</item>
	/// <item>Detailed information via <see cref="EventViewerViewModel.GetEventDetails"/></item>
	/// </list>
	/// </remarks>
	private void UpdateFields() {
		DebugEventInfo evt = _events[_index];
		Color = evt.Color;
		ProgramCounter = "$" + evt.ProgramCounter.ToString("X4");
		Scanline = evt.Scanline.ToString();
		Cycle = evt.Cycle.ToString();
		string address = "";
		bool isReadWriteOp = evt.Flags.HasFlag(EventFlags.ReadWriteOp);
		if (isReadWriteOp) {
			address += "$" + evt.Operation.Address.ToString("X4");

			CodeLabel? label = LabelManager.GetLabel(new AddressInfo() { Address = (int)evt.Operation.Address, Type = evt.Operation.MemType });
			if (label is not null) {
				address = label.Label + " (" + address + ")";
			}

			if (evt.RegisterId >= 0) {
				string regName = evt.GetRegisterName();
				if (string.IsNullOrEmpty(regName)) {
					address += $" (${evt.RegisterId:X2})";
				} else {
					address += $" ({regName} - ${evt.RegisterId:X2})";
				}
			}
		}

		Address = address;
		Value = isReadWriteOp ? "$" + evt.Operation.Value.ToString("X2") : "";
		Type = ResourceHelper.GetEnumText(evt.Type);
		if (isReadWriteOp) {
			Type += evt.Operation.Type.IsWrite() ? " (W)" : " (R)";
		}

		Details = EventViewerViewModel.GetEventDetails(_cpuType, evt, true);
	}
}
