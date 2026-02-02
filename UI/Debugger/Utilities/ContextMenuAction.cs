using System;
using System.Collections.Generic;
using System.Reactive;
using System.Threading.Tasks;
using System.Windows.Input;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Controls;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.Utilities;
public abstract class BaseMenuAction : ViewModelBase, IDisposable {
	private static Dictionary<ActionType, string?> _iconCache = new();

	public ActionType ActionType;
	public string? CustomText { get; set; }
	public Func<string>? DynamicText { get; set; }
	public Func<string>? DynamicIcon { get; set; }

	static BaseMenuAction() {
		foreach (ActionType value in Enum.GetValues<ActionType>()) {
			_iconCache[value] = value.GetAttribute<IconFileAttribute>()?.Icon;
		}
	}

	public virtual string Name {
		get {
			string label = DynamicText != null
				? DynamicText()
				: ActionType == ActionType.Custom ? CustomText ?? "" : ResourceHelper.GetEnumText(ActionType);
			if (HintText != null) {
				string hint = HintText();
				if (!string.IsNullOrWhiteSpace(hint)) {
					label += " (" + hint + ")";
				}
			}

			if (!label.StartsWith("_")) {
				//Escape underscores to prevent them from getting removed
				//(underscore is used to highlight the next letter when alt is pressed)
				label = label.Replace("_", "__");
			}

			return label;
		}
	}

	private string? _currentIcon = null;

	private string? GetIconFile() {
		string? actionIcon = _iconCache[ActionType];
		if (!string.IsNullOrEmpty(actionIcon)) {
			return actionIcon;
		} else if (DynamicIcon != null) {
			return "Assets/" + DynamicIcon() + ".png";
		} else if (IsSelected?.Invoke() == true) {
			return ConfigManager.ActiveTheme == NexenTheme.Light ? "Assets/MenuItemChecked.png" : "Assets/MenuItemCheckedDark.png";
		}

		return null;
	}

	public List<object>? SubActions {
		get;
		set {
			field = value;

			if (field != null) {
				Func<bool>? isEnabled = IsEnabled;

				IsEnabled = () => {
					if (isEnabled != null && !isEnabled()) {
						return false;
					}

					foreach (object subAction in field) {
						if (subAction is BaseMenuAction act) {
							if (act.IsEnabled == null || act.IsEnabled()) {
								return true;
							}
						}
					}

					return false;
				};
			}

			this.RaiseAndSetIfChanged(ref field, value);
		}
	}

	public Func<string>? HintText { get; set; }
	public Func<bool>? IsEnabled { get; set; }
	public Func<bool>? IsSelected { get; set; }
	public Func<bool>? IsVisible { get; set; }

	public bool AllowedWhenHidden { get; set; }
	public bool AlwaysShowLabel { get; set; }
	public RoutingStrategies RoutingStrategy { get; set; } = RoutingStrategies.Bubble;

	protected abstract string InternalShortcutText { get; }

	[Reactive] public string ShortcutText { get; set; } = "";
	[Reactive] public string ActionName { get; set; } = "";
	[Reactive] public Image? ActionIcon { get; set; }
	[Reactive] public bool Enabled { get; set; }
	[Reactive] public bool Visible { get; set; }

	[Reactive] public string TooltipText { get; set; } = "";

	private static SimpleCommand _emptyCommand = new SimpleCommand(() => { });

	public SimpleCommand? ClickCommand {
		get {
			Update();
			return field ?? ContextMenuAction._emptyCommand;
		}

		private set;
	}

	private Action _onClick = () => { };
	public Action OnClick {
		get => _onClick;
		set {
			_onClick = () => {
				if ((IsVisible == null || AllowedWhenHidden || IsVisible()) && (IsEnabled == null || IsEnabled())) {
					if (ActionType == ActionType.Exit) {
						//When using exit, the command is disposed while the command is running, which causes a crash
						//Run the code in a posted action to prevent the crash
						Dispatcher.UIThread.Post(() => value());
					} else {
						try {
							value();
						} catch (Exception ex) {
							Dispatcher.UIThread.Post(() => NexenMsgBox.ShowException(ex));
						}
					}
				}
			};
			ClickCommand = new SimpleCommand(_onClick);
		}
	}

	public void Update() {
		ActionName = Name;

		ShortcutText = InternalShortcutText;
		TooltipText = ShortcutText.Length > 0 ? $"{Name} ({ShortcutText})" : Name;

		string? iconFile = GetIconFile();
		if (_currentIcon != iconFile) {
			ActionIcon = iconFile != null ? ImageUtilities.FromAsset(iconFile) : null;
			_currentIcon = iconFile;
		}

		Enabled = IsEnabled?.Invoke() ?? true;
		Visible = IsVisible?.Invoke() ?? true;
	}

	public virtual void Dispose() {
		_onClick = () => { };
		ClickCommand = null;
		IsSelected = null;
		IsEnabled = null;
		IsVisible = null;
		if (SubActions != null) {
			foreach (object subAction in SubActions) {
				if (subAction is BaseMenuAction action) {
					action.Dispose();
				}
			}
		}
	}
}

public class MainMenuAction : BaseMenuAction {
	public EmulatorShortcut? Shortcut { get; set; }
	public uint ShortcutParam { get; set; }

	public Func<string>? CustomShortcutText { get; set; }

	public MainMenuAction() {
	}

	public MainMenuAction(EmulatorShortcut? shortcut) {
		if (shortcut.HasValue) {
			Shortcut = shortcut.Value;

			IsEnabled = () => EmuApi.IsShortcutAllowed(shortcut.Value, ShortcutParam);

			OnClick = () =>                  //Run outside the UI thread to avoid deadlocks, etc.
				Task.Run(() => EmuApi.ExecuteShortcut(new ExecuteShortcutParams() { Shortcut = shortcut.Value, Param = ShortcutParam }));
		}
	}

	protected override string InternalShortcutText {
		get {
			if (CustomShortcutText != null) {
				return CustomShortcutText();
			} else {
				return Shortcut.HasValue ? Shortcut.Value.GetShortcutKeys()?.ToString() ?? "" : "";
			}
		}
	}

}

public class ContextMenuAction : BaseMenuAction {
	public Func<DbgShortKeys>? Shortcut { get; set; }
	protected override string InternalShortcutText => Shortcut?.Invoke().ToString() ?? "";

	public override void Dispose() {
		base.Dispose();
		Shortcut = null;
	}
}

public class ContextMenuSeparator : ContextMenuAction {
	public override string Name => "-";

	public ContextMenuSeparator() {
		IsEnabled = () => false;
	}
}

public class SimpleCommand : ICommand {
	private Action _commandAction;

#pragma warning disable CS0067 // The event 'SimpleCommand.CanExecuteChanged' is never used
	public event EventHandler? CanExecuteChanged;
#pragma warning restore CS0067 // The event 'SimpleCommand.CanExecuteChanged' is never used

	public SimpleCommand(Action action) {
		this._commandAction = action;
	}

	public bool CanExecute(object? parameter) {
		return true;
	}

	public void Execute(object? parameter) {
		_commandAction();
	}
}

public enum ActionType {
	Custom,

	[IconFile("Copy")]
	Copy,

	[IconFile("Paste")]
	Paste,

	[IconFile("SelectAll")]
	SelectAll,

	[IconFile("Undo")]
	Undo,

	[IconFile("EditLabel")]
	EditLabel,

	[IconFile("EditLabel")]
	EditComment,

	[IconFile("Add")]
	AddWatch,

	[IconFile("BreakpointEnableDisable")]
	EditBreakpoint,

	MarkSelectionAs,

	[IconFile("Accept")]
	MarkAsCode,

	[IconFile("CheatCode")]
	MarkAsData,

	[IconFile("Help")]
	MarkAsUnidentified,

	[IconFile("Add")]
	Add,

	[IconFile("Edit")]
	Edit,

	[IconFile("Close")]
	Delete,

	[IconFile("Breakpoint")]
	AddBreakpoint,
	[IconFile("ForbidBreakpoint")]
	AddForbidBreakpoint,
	[IconFile("ForbidBreakpoint")]
	ToggleForbidBreakpoint,

	[IconFile("Breakpoint")]
	EditTilemapBreakpoint,
	[IconFile("Breakpoint")]
	EditAttributeBreakpoint,

	[IconFile("MoveUp")]
	MoveUp,

	[IconFile("MoveDown")]
	MoveDown,

	WatchDecimalDisplay,
	WatchHexDisplay,
	WatchBinaryDisplay,

	RowDisplayFormat,
	RowFormatBinary,
	RowFormatHex8Bits,
	RowFormatHex16Bits,
	RowFormatHex24Bits,
	RowFormatHex32Bits,
	RowFormatSigned8Bits,
	RowFormatSigned16Bits,
	RowFormatSigned24Bits,
	RowFormatSigned32Bits,
	RowFormatUnsigned,

	[IconFile("Close")]
	ClearFormat,

	[IconFile("Import")]
	Import,

	[IconFile("Export")]
	Export,

	ShowConsoleStatus,
	ShowBreakpointList,
	ShowWatchList,
	ShowFunctionList,
	ShowLabelList,
	ShowCallStack,
	ShowControllers,

	ShowSettingsPanel,
	ShowMemoryMappings,

	[IconFile("Settings")]
	Preferences,

	[IconFile("MediaPlay")]
	Continue,

	[IconFile("MediaPause")]
	Break,

	[IconFile("StepInto")]
	StepInto,

	[IconFile("StepOver")]
	StepOver,

	[IconFile("StepOut")]
	StepOut,

	[IconFile("StepBack")]
	StepBack,

	[IconFile("StepBackScanline")]
	StepBackScanline,

	[IconFile("StepBackFrame")]
	StepBackFrame,

	[IconFile("RunCpuCycle")]
	RunCpuCycle,

	[IconFile("RunPpuCycle")]
	RunPpuCycle,

	[IconFile("RunPpuScanline")]
	RunPpuScanline,

	[IconFile("RunPpuFrame")]
	RunPpuFrame,

	[IconFile("StepNmi")]
	RunToNmi,

	[IconFile("StepIrq")]
	RunToIrq,

	BreakIn,

	[IconFile("GoToScanline")]
	BreakOn,

	[IconFile("Refresh")]
	Reset,

	[IconFile("PowerCycle")]
	PowerCycle,

	[IconFile("Reload")]
	ReloadRom,

	[IconFile("Script")]
	NewScript,

	[IconFile("MediaPlay")]
	RunScript,

	[IconFile("MediaStop")]
	StopScript,

	[IconFile("LogWindow")]
	BuiltInScripts,

	[IconFile("Folder")]
	Open,

	[IconFile("SaveFloppy")]
	Save,

	SaveAs,

	[IconFile("Exit")]
	Exit,

	[IconFile("Help")]
	HelpApiReference,

	RecentScripts,

	[IconFile("Refresh")]
	Refresh,
	EnableAutoRefresh,
	RefreshOnBreakPause,

	ZoomIn,
	ZoomOut,

	[IconFile("Expand")]
	GoToLocation,

	[IconFile("Export")]
	ExportToPng,

	[IconFile("VerticalLayout")]
	ViewInTileViewer,

	[IconFile("CheatCode")]
	ViewInMemoryViewer,

	LoadTblFile,
	ResetTblMappings,

	GoTo,
	GoToAddress,
	GoToProgramCounter,
	GoToAll,

	[IconFile("Debugger")]
	OpenDebugger,

	[IconFile("EventViewer")]
	OpenEventViewer,

	[IconFile("CheatCode")]
	OpenMemoryTools,

	[IconFile("RegisterIcon")]
	OpenRegisterViewer,

	[IconFile("LogWindow")]
	OpenTraceLogger,

	[IconFile("Find")]
	OpenMemorySearch,

	[IconFile("VideoOptions")]
	OpenTilemapViewer,

	[IconFile("VerticalLayout")]
	OpenTileViewer,

	[IconFile("PerfTracker")]
	OpenSpriteViewer,

	[IconFile("VideoFilter")]
	OpenPaletteViewer,

	[IconFile("Chip")]
	OpenAssembler,

	[IconFile("LogWindow")]
	OpenDebugLog,

	[IconFile("Speed")]
	OpenProfiler,

	[IconFile("Script")]
	OpenScriptWindow,

	[IconFile("Find")]
	OpenWatchWindow,

	[IconFile("Edit")]
	OpenNesHeaderEditor,

	[IconFile("Settings")]
	OpenDebugSettings,

	[IconFile("SpcDebugger")]
	OpenSpcDebugger,
	[IconFile("Cx4Debugger")]
	OpenCx4Debugger,
	[IconFile("NecDspDebugger")]
	OpenNecDspDebugger,
	[IconFile("GsuDebugger")]
	OpenGsuDebugger,
	[IconFile("Sa1Debugger")]
	OpenSa1Debugger,
	[IconFile("St018Debugger")]
	OpenSt018Debugger,
	[IconFile("GameboyDebugger")]
	OpenGameboyDebugger,

	[IconFile("CheatCode")]
	Cheats,
	[IconFile("HistoryViewer")]
	HistoryViewer,
	[IconFile("Edit")]
	TasEditor,
	[IconFile("Movie")]
	Movies,
	[IconFile("MediaPlay")]
	Play,
	[IconFile("Record")]
	Record,
	[IconFile("MediaStop")]
	Stop,

	[IconFile("Network")]
	NetPlay,
	Connect,
	Disconnect,
	StartServer,
	StopServer,
	SelectController,

	[IconFile("Microphone")]
	SoundRecorder,
	[IconFile("VideoRecorder")]
	VideoRecorder,

	[IconFile("HdPack")]
	HdPacks,
	[IconFile("Import")]
	InstallHdPack,
	[IconFile("HdPack")]
	HdPackBuilder,

	[IconFile("LogWindow")]
	LogWindow,

	[IconFile("Camera")]
	TakeScreenshot,

	[IconFile("Help")]
	OnlineHelp,
	[IconFile("CommandLine")]
	CommandLineHelp,
	[IconFile("Update")]
	CheckForUpdates,
	[IconFile("Exclamation")]
	About,
	[IconFile("Comment")]
	ReportBug,

	[IconFile("Speed")]
	Speed,
	NormalSpeed,
	DoubleSpeed,
	TripleSpeed,
	MaximumSpeed,
	HalfSpeed,
	QuarterSpeed,
	IncreaseSpeed,
	DecreaseSpeed,
	ShowFps,

	[IconFile("Fullscreen")]
	VideoScale,
	Fullscreen,

	[IconFile("AspectRatio")]
	AspectRatio,

	[IconFile("VideoFilter")]
	VideoFilter,
	[IconFile("WebBrowser")]
	Region,

	[IconFile("Audio")]
	Audio,
	[IconFile("HammerScrewdriver")]
	Emulation,
	[IconFile("Controller")]
	Input,
	[IconFile("VideoOptions")]
	Video,
	[IconFile("NesIcon")]
	Nes,
	[IconFile("SnesIcon")]
	Snes,
	[IconFile("GameboyIcon")]
	Gameboy,
	[IconFile("GbaIcon")]
	Gba,
	[IconFile("PceIcon")]
	PcEngine,
	[IconFile("SmsIcon")]
	Sms,
	[IconFile("WsIcon")]
	Ws,
	[IconFile("Drive")]
	OtherConsoles,
	[IconFile("MediaPause")]
	Pause,
	[IconFile("MediaStop")]
	PowerOff,
	[IconFile("MediaPlay")]
	Resume,

	[IconFile("SaveFloppy")]
	SelectDisk,
	[IconFile("MediaEject")]
	EjectDisk,

	[IconFile("Coins")]
	InsertCoin1,
	[IconFile("Coins")]
	InsertCoin2,
	[IconFile("Coins")]
	InsertCoin3,
	[IconFile("Coins")]
	InsertCoin4,

	SaveState,
	LoadState,
	[IconFile("SplitView")]
	SaveStateDialog,
	[IconFile("SaveFloppy")]
	SaveStateToFile,
	[IconFile("SplitView")]
	LoadStateDialog,
	[IconFile("Folder")]
	LoadStateFromFile,

	RecentFiles,
	LoadLastSession,

	/// <summary>
	/// Quick save to a timestamped save state file
	/// </summary>
	[IconFile("SaveFloppy")]
	QuickSaveTimestamped,

	/// <summary>
	/// Open picker to browse all timestamped saves
	/// </summary>
	[IconFile("SplitView")]
	OpenSaveStatePicker,

	[IconFile("SaveFloppy")]
	SaveRom,
	SaveRomAs,
	[IconFile("CheatCode")]
	SaveEditsAsIps,

	[IconFile("ResetSettings")]
	ResetLayout,

	[IconFile("Breakpoint")]
	ToggleBreakpoint,

	[IconFile("Edit")]
	EditSelectedCode,

	[IconFile("Debugger")]
	ViewInDebugger,

	[IconFile("Breakpoint")]
	SetBreakpoint,

	[IconFile("Close")]
	RemoveBreakpoint,

	[IconFile("Breakpoint")]
	EnableBreakpoint,

	[IconFile("BreakpointDisabled")]
	DisableBreakpoint,

	[IconFile("Edit")]
	CodeWindowEditBreakpoint,

	CodeDataLogger,
	[IconFile("ResetSettings")]
	ResetCdl,
	[IconFile("Folder")]
	LoadCdl,
	[IconFile("SaveFloppy")]
	SaveCdl,

	MoveProgramCounter,
	RunToLocation,

	ShowToolbar,
	ShowListView,

	[IconFile("ResetSettings")]
	ResetWorkspace,
	[IconFile("TabContent")]
	Workspace,

	[IconFile("Import")]
	ImportLabels,
	[IconFile("Export")]
	ExportLabels,
	[IconFile("Export")]
	ExportPansy,

	[IconFile("Import")]
	ImportWatchEntries,
	[IconFile("Export")]
	ExportWatchEntries,

	[IconFile("Barcode")]
	InputBarcode,

	[IconFile("Tape")]
	TapeRecorder,

	GenerateRom,
	CdlRomStripUnused,
	CdlRomStripUsed,

	[IconFile("Find")]
	Find,
	[IconFile("NextArrow")]
	FindNext,
	[IconFile("PreviousArrow")]
	FindPrev,
	[IconFile("Find")]
	FindOccurrences,

	[IconFile("Edit")]
	EditTile,
	[IconFile("Edit")]
	EditTiles,
	[IconFile("Edit")]
	EditSprite,
	[IconFile("Edit")]
	EditColor,

	NavigateTo,

	[IconFile("PrevCode")]
	GoToPrevCode,
	[IconFile("NextCode")]
	GoToNextCode,

	[IconFile("PrevData")]
	GoToPrevData,
	[IconFile("NextData")]
	GoToNextData,

	[IconFile("PrevUnknown")]
	GoToPrevUnknown,
	[IconFile("NextUnknown")]
	GoToNextUnknown,

	[IconFile("PrevRead")]
	GoToPrevRead,
	[IconFile("NextRead")]
	GoToNextRead,

	[IconFile("PrevWrite")]
	GoToPrevWrite,
	[IconFile("NextWrite")]
	GoToNextWrite,

	[IconFile("PrevExec")]
	GoToPrevExec,
	[IconFile("NextExec")]
	GoToNextExec,

	[IconFile("Export")]
	ExportMovie,
	[IconFile("SaveFloppy")]
	CreateSaveState,
	[IconFile("MediaPlay")]
	ResumeGameplay,

	[IconFile("Settings")]
	GameConfig,

	[IconFile("MediaStop")]
	FreezeMemory,
	[IconFile("MediaPlay")]
	UnfreezeMemory,

	[IconFile("ResetSettings")]
	ResetAccessCounters,

	[IconFile("HdPack")]
	CopyToHdPackFormat,

	[IconFile("Find")]
	CheatDatabase,

	ToggleBilinearInterpolation,

	[IconFile("RotateLeft")]
	RotateLeft,
	[IconFile("RotateRight")]
	RotateRight,
	[IconFile("FlipHorizontal")]
	FlipHorizontal,
	[IconFile("FlipVertical")]
	FlipVertical,

	[IconFile("TranslateLeft")]
	TranslateLeft,
	[IconFile("TranslateRight")]
	TranslateRight,
	[IconFile("TranslateUp")]
	TranslateUp,
	[IconFile("TranslateDown")]
	TranslateDown,

	[IconFile("PreviousArrow")]
	NavigateBack,
	[IconFile("NextArrow")]
	NavigateForward,

	[IconFile("Close")]
	ResetProfilerData,
	[IconFile("Copy")]
	CopyToClipboard,
}
