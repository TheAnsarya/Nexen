using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics.CodeAnalysis;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Debugger.RegisterViewer;
using Nexen.Debugger.Utilities;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for the register viewer window.
/// Displays hardware register values for the emulated console, organized into tabs by subsystem.
/// </summary>
public class RegisterViewerWindowViewModel : DisposableViewModel, ICpuTypeModel {
	/// <summary>
	/// Gets or sets the list of register viewer tabs, one per hardware subsystem.
	/// </summary>
	[Reactive] public List<RegisterViewerTab> Tabs { get; set; } = new List<RegisterViewerTab>();

	/// <summary>
	/// Gets the configuration settings for the register viewer.
	/// </summary>
	public RegisterViewerConfig Config { get; }

	/// <summary>
	/// Gets the view model controlling refresh timing behavior.
	/// </summary>
	public RefreshTimingViewModel RefreshTiming { get; }

	/// <summary>
	/// Gets or sets the menu actions for the File menu.
	/// </summary>
	[Reactive] public List<object> FileMenuActions { get; private set; } = new();

	/// <summary>
	/// Gets or sets the menu actions for the View menu.
	/// </summary>
	[Reactive] public List<object> ViewMenuActions { get; private set; } = new();

	/// <summary>Cached console state from the emulator core.</summary>
	private BaseState? _state = null;

	/// <summary>
	/// Gets or sets the CPU type for this register viewer (derived from console type).
	/// </summary>
	public CpuType CpuType {
		get => _romInfo.ConsoleType.GetMainCpuType();
		set { }
	}

	/// <summary>Cached ROM information.</summary>
	private RomInfo _romInfo = new RomInfo();

	/// <summary>SNES interrupt flag register $4210 (NMI enable/flag).</summary>
	private byte _snesReg4210;

	/// <summary>SNES interrupt flag register $4211 (IRQ enable/flag).</summary>
	private byte _snesReg4211;

	/// <summary>SNES status register $4212 (V-blank, H-blank, joypad status).</summary>
	private byte _snesReg4212;

	/// <summary>
	/// Initializes a new instance of the <see cref="RegisterViewerWindowViewModel"/> class.
	/// </summary>
	public RegisterViewerWindowViewModel() {
		Config = ConfigManager.Config.Debug.RegisterViewer.Clone();
		RefreshTiming = new RefreshTimingViewModel(Config.RefreshTiming, CpuType);

		if (Design.IsDesignMode) {
			return;
		}

		UpdateRomInfo();
		RefreshTiming.UpdateMinMaxValues(CpuType);
		RefreshData();
	}

	/// <summary>
	/// Initializes the File and View menu actions and registers keyboard shortcuts.
	/// </summary>
	/// <param name="wnd">The parent window for shortcut registration.</param>
	public void InitMenu(Window wnd) {
		FileMenuActions = AddDisposables(new List<object>() {
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
				IsSelected = () => Config.RefreshTiming.AutoRefresh,
				OnClick = () => Config.RefreshTiming.AutoRefresh = !Config.RefreshTiming.AutoRefresh
			},
			new ContextMenuAction() {
				ActionType = ActionType.RefreshOnBreakPause,
				IsSelected = () => Config.RefreshTiming.RefreshOnBreakPause,
				OnClick = () => Config.RefreshTiming.RefreshOnBreakPause = !Config.RefreshTiming.RefreshOnBreakPause
			}
		});

		DebugShortcutManager.RegisterActions(wnd, FileMenuActions);
		DebugShortcutManager.RegisterActions(wnd, ViewMenuActions);
	}

	/// <summary>
	/// Updates the cached ROM information from the emulator.
	/// </summary>
	public void UpdateRomInfo() {
		_romInfo = EmuApi.GetRomInfo();
	}

	/// <summary>
	/// Refreshes register data from the emulator core.
	/// Fetches console state appropriate for the loaded system type.
	/// </summary>
	public void RefreshData() {
		if (_romInfo.ConsoleType == ConsoleType.Snes) {
			_snesReg4210 = DebugApi.GetMemoryValue(MemoryType.SnesMemory, 0x4210);
			_snesReg4211 = DebugApi.GetMemoryValue(MemoryType.SnesMemory, 0x4211);
			_snesReg4212 = DebugApi.GetMemoryValue(MemoryType.SnesMemory, 0x4212);
			_state = DebugApi.GetConsoleState<SnesState>(ConsoleType.Snes);
		} else if (_romInfo.ConsoleType == ConsoleType.Nes) {
			_state = DebugApi.GetConsoleState<NesState>(ConsoleType.Nes);
		} else if (_romInfo.ConsoleType == ConsoleType.Gameboy) {
			_state = DebugApi.GetConsoleState<GbState>(ConsoleType.Gameboy);
		} else if (_romInfo.ConsoleType == ConsoleType.PcEngine) {
			_state = DebugApi.GetConsoleState<PceState>(ConsoleType.PcEngine);
		} else if (_romInfo.ConsoleType == ConsoleType.Sms) {
			_state = DebugApi.GetConsoleState<SmsState>(ConsoleType.Sms);
		} else if (_romInfo.ConsoleType == ConsoleType.Gba) {
			_state = DebugApi.GetConsoleState<GbaState>(ConsoleType.Gba);
		} else if (_romInfo.ConsoleType == ConsoleType.Ws) {
			_state = DebugApi.GetConsoleState<WsState>(ConsoleType.Ws);
		}

		Dispatcher.UIThread.Post(() => RefreshTabs());
	}

	/// <summary>
	/// Updates the register viewer tabs with the latest data from the console state.
	/// Creates or updates tabs using the appropriate platform-specific register viewer.
	/// </summary>
	public void RefreshTabs() {
		if (_state == null) {
			return;
		}

		List<RegisterViewerTab> tabs = new List<RegisterViewerTab>();
		BaseState lastState = _state;

		if (lastState is SnesState snesState) {
			tabs = SnesRegisterViewer.GetTabs(ref snesState, _romInfo.CpuTypes, _snesReg4210, _snesReg4211, _snesReg4212);
		} else if (lastState is NesState nesState) {
			tabs = NesRegisterViewer.GetTabs(ref nesState);
		} else if (lastState is GbState gbState) {
			tabs = GbRegisterViewer.GetTabs(ref gbState);
		} else if (lastState is PceState pceState) {
			tabs = PceRegisterViewer.GetTabs(ref pceState);
		} else if (lastState is SmsState smsState) {
			tabs = SmsRegisterViewer.GetTabs(ref smsState, _romInfo.Format);
		} else if (lastState is GbaState gbaState) {
			tabs = GbaRegisterViewer.GetTabs(ref gbaState);
		} else if (lastState is WsState wsState) {
			tabs = WsRegisterViewer.GetTabs(ref wsState);
		}

		foreach (RegisterViewerTab tab in tabs) {
			// Column widths are now managed directly by the DataGrid control
		}

		if (Tabs.Count != tabs.Count) {
			Tabs = tabs;
		} else {
			for (int i = 0; i < Tabs.Count; i++) {
				Tabs[i].SetData(tabs[i].Data);
				Tabs[i].TabName = tabs[i].TabName;
			}
		}
	}

	/// <summary>
	/// Called when a game is loaded. Updates ROM info and refreshes register data.
	/// </summary>
	public void OnGameLoaded() {
		UpdateRomInfo();
		RefreshData();
	}
}

/// <summary>
/// Represents a tab in the register viewer containing registers for a specific hardware subsystem.
/// </summary>
public class RegisterViewerTab : ReactiveObject {
	/// <summary>Backing field for <see cref="TabName"/>.</summary>
	private string _name;

	/// <summary>Backing field for <see cref="Data"/>.</summary>
	private List<RegEntry> _data;

	/// <summary>
	/// Gets or sets the display name for this tab.
	/// </summary>
	public string TabName { get => _name; set => this.RaiseAndSetIfChanged(ref _name, value); }

	/// <summary>
	/// Gets or sets the list of register entries displayed in this tab.
	/// </summary>
	public List<RegEntry> Data { get => _data; set => this.RaiseAndSetIfChanged(ref _data, value); }

	/// <summary>Backing field for <see cref="SelectedItem"/>.</summary>
	private RegEntry? _selectedItem;

	/// <summary>
	/// Gets or sets the currently selected register entry in the data grid.
	/// </summary>
	public RegEntry? SelectedItem { get => _selectedItem; set => this.RaiseAndSetIfChanged(ref _selectedItem, value); }

	/// <summary>
	/// Gets the CPU type associated with this tab's registers, if applicable.
	/// </summary>
	public CpuType? CpuType { get; }

	/// <summary>
	/// Gets the memory type associated with this tab's registers, if applicable.
	/// </summary>
	public MemoryType? MemoryType { get; }

	/// <summary>
	/// Initializes a new instance of the <see cref="RegisterViewerTab"/> class.
	/// </summary>
	/// <param name="name">The display name for this tab.</param>
	/// <param name="data">The list of register entries.</param>
	/// <param name="cpuType">Optional CPU type for register context.</param>
	/// <param name="memoryType">Optional memory type for viewing in memory editor.</param>
	public RegisterViewerTab(string name, List<RegEntry> data, CpuType? cpuType = null, MemoryType? memoryType = null) {
		_name = name;
		_data = data;
		CpuType = cpuType;
		MemoryType = memoryType;
	}

	/// <summary>
	/// Updates the register data with new values, reusing existing entries when counts match.
	/// </summary>
	/// <param name="rows">The new register entry data.</param>
	public void SetData(List<RegEntry> rows) {
		if (Data.Count != rows.Count) {
			Data = rows;
		} else {
			for (int i = 0; i < rows.Count; i++) {
				Data[i].Value = rows[i].Value;
				Data[i].ValueHex = rows[i].ValueHex;
			}
		}
	}
}

/// <summary>
/// Represents a single register entry in the register viewer.
/// Contains address, name, value, and hex value with support for headers and various value formats.
/// </summary>
public class RegEntry : INotifyPropertyChanged {
	/// <summary>Background brush used for header rows.</summary>
	private static ISolidColorBrush HeaderBgBrush = new SolidColorBrush(0x40B0B0B0);

	/// <summary>
	/// Gets the address or address range for this register (e.g., "$4200" or "$4200-$4203").
	/// </summary>
	public string Address { get; private set; }

	/// <summary>
	/// Gets the display name for this register.
	/// </summary>
	public string Name { get; private set; }

	/// <summary>
	/// Gets whether this entry is a data row (true) or a header row (false).
	/// </summary>
	public bool IsEnabled { get; private set; }

	/// <summary>
	/// Gets the background brush for this row (different for headers vs data rows).
	/// </summary>
	public IBrush Background { get; private set; }

	/// <summary>Backing field for <see cref="Value"/>.</summary>
	private string _value;

	/// <summary>Backing field for <see cref="ValueHex"/>.</summary>
	private string _valueHex;

	/// <summary>
	/// Event raised when a property value changes.
	/// </summary>
	public event PropertyChangedEventHandler? PropertyChanged;

	/// <summary>
	/// Gets or sets the formatted display value for this register.
	/// </summary>
	public string Value {
		get => _value;
		set {
			if (_value != value) {
				_value = value;
				PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Value)));
			}
		}
	}

	/// <summary>
	/// Gets or sets the hexadecimal representation of this register's value.
	/// </summary>
	public string ValueHex {
		get => _valueHex;
		set {
			if (_valueHex != value) {
				_valueHex = value;
				PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(ValueHex)));
			}
		}
	}

	/// <summary>
	/// Initializes a new register entry with a formattable value.
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The register name.</param>
	/// <param name="value">The register value.</param>
	/// <param name="format">The hex format for display.</param>
	public RegEntry(string reg, string name, ISpanFormattable? value, Format format = Format.None) {
		Init(reg, name, value, format);
	}

	/// <summary>
	/// Initializes a new register entry with a boolean value.
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The register name.</param>
	/// <param name="value">The boolean value.</param>
	public RegEntry(string reg, string name, bool value) {
		Init(reg, name, value, Format.None);
	}

	/// <summary>
	/// Initializes a new register entry with an enum value.
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The register name.</param>
	/// <param name="value">The enum value.</param>
	public RegEntry(string reg, string name, Enum value) {
		Init(reg, name, value, Format.X8);
	}

	/// <summary>
	/// Initializes a new header entry (no value).
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The header name.</param>
	public RegEntry(string reg, string name) {
		Init(reg, name, null, Format.None);
	}

	/// <summary>
	/// Initializes a new register entry with a text value and optional raw value for hex display.
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The register name.</param>
	/// <param name="textValue">The text representation of the value.</param>
	/// <param name="rawValue">Optional raw value for hex display.</param>
	public RegEntry(string reg, string name, string textValue, IConvertible? rawValue) {
		Init(reg, name, textValue, Format.None);
		if (rawValue != null) {
			_valueHex = GetHexValue(rawValue, Format.X8);
		}
	}

	/// <summary>
	/// Initializes the register entry with the specified values.
	/// </summary>
	/// <param name="reg">The register address string.</param>
	/// <param name="name">The register name.</param>
	/// <param name="value">The register value (null for headers).</param>
	/// <param name="format">The hex format for display.</param>
	[MemberNotNull(nameof(Address), nameof(Name), nameof(Background), nameof(_value), nameof(_valueHex))]
	private void Init(string reg, string name, object? value, Format format) {
		if (format == Format.None && value is not bool) {
			//Display hex values for everything except booleans
			format = Format.X8;
		}

		Address = reg;
		Name = name;

		_value = GetValue(value);

		_valueHex = value is Enum ? GetHexValue(Convert.ToInt64(value), Format.X8) : GetHexValue(value, format);

		Background = value == null ? RegEntry.HeaderBgBrush : Brushes.Transparent;
		IsEnabled = value != null;
	}

	/// <summary>
	/// Converts a value to its display string representation.
	/// </summary>
	/// <param name="value">The value to convert.</param>
	/// <returns>The formatted display string.</returns>
	private string GetValue(object? value) {
		if (value is string str) {
			return str;
		} else if (value is bool boolValue) {
			return boolValue ? "☑ true" : "☐ false";
		} else if (value is IFormattable formattable) {
			return formattable.ToString() ?? "";
		} else if (value == null) {
			return "";
		}

		throw new Exception("Unsupported type");
	}

	/// <summary>
	/// Converts a value to its hexadecimal string representation.
	/// </summary>
	/// <param name="value">The value to convert.</param>
	/// <param name="format">The hex format specifying width.</param>
	/// <returns>The formatted hex string (e.g., "$FF" or "$FFFF").</returns>
	private string GetHexValue(object? value, Format format) {
		if (value is null or string) {
			return "";
		}

		if (value is bool boolValue && format != Format.None) {
			return boolValue ? "$01" : "$00";
		}

		switch (format) {
			default: return "";
			case Format.X8: return "$" + ((IFormattable)value).ToString("X2", null);
			case Format.X16: return "$" + ((IFormattable)value).ToString("X4", null);

			case Format.X24: {
					string str = ((IFormattable)value).ToString("X6", null);
					return "$" + (str.Length > 7 ? str.Substring(str.Length - 6) : str);
				}

			case Format.X28: {
					string str = ((IFormattable)value).ToString("X7", null);
					return "$" + (str.Length > 7 ? str.Substring(str.Length - 7) : str);
				}
			case Format.X32: return "$" + ((IFormattable)value).ToString("X8", null);
		}
	}

	/// <summary>
	/// Gets the starting address parsed from the Address string.
	/// </summary>
	public int StartAddress {
		get {
			string addr = Address;
			if (addr.StartsWith("$")) {
				addr = addr.Substring(1);
			}

			int separator = addr.IndexOfAny(new char[] { '.', '-', '/' });
			if (separator >= 0) {
				addr = addr.Substring(0, separator);
			}

			if (Int32.TryParse(addr.Trim(), System.Globalization.NumberStyles.HexNumber, null, out int startAddress)) {
				return startAddress;
			}

			return -1;
		}
	}

	/// <summary>
	/// Gets the ending address parsed from the Address string (same as StartAddress for single registers).
	/// </summary>
	public int EndAddress {
		get {
			string addr = Address;
			int separator = addr.IndexOfAny(new char[] { '.', '-', '/' });
			if (separator >= 0) {
				if (addr[separator] == '.') {
					//First separator is a dot for bits, assume there is no end address
					return StartAddress;
				}

				addr = addr.Substring(separator + 1).Trim();
			} else {
				return StartAddress;
			}

			int lastRangeAddr = addr.LastIndexOf('/');
			if (lastRangeAddr >= 0) {
				addr = addr.Substring(lastRangeAddr + 1);
			}

			if (addr.StartsWith("$")) {
				addr = addr.Substring(1);
			}

			separator = addr.IndexOfAny(new char[] { '.', '-', '/' });
			if (separator >= 0) {
				addr = addr.Substring(0, separator);
			}

			if (Int32.TryParse(addr.Trim(), System.Globalization.NumberStyles.HexNumber, null, out int endAddress)) {
				if (addr.Length == 1) {
					return (StartAddress & ~0xF) | endAddress;
				} else {
					return endAddress;
				}
			}

			return StartAddress;
		}
	}

	/// <summary>
	/// Specifies the hexadecimal format width for register value display.
	/// </summary>
	public enum Format {
		/// <summary>No hex format (for text values).</summary>
		None,
		/// <summary>8-bit hex format (2 digits, e.g., "$FF").</summary>
		X8,
		/// <summary>16-bit hex format (4 digits, e.g., "$FFFF").</summary>
		X16,
		/// <summary>24-bit hex format (6 digits, e.g., "$FFFFFF").</summary>
		X24,
		/// <summary>28-bit hex format (7 digits).</summary>
		X28,
		/// <summary>32-bit hex format (8 digits, e.g., "$FFFFFFFF").</summary>
		X32
	}
}
