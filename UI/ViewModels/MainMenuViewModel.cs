using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Threading;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Controls;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.ViewModels;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Menus;
using Nexen.Services;
using Nexen.Utilities;
using Nexen.Windows;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the main application menu.
/// Manages all menu items for File, Game, Options, Tools, Debug, and Help menus.
/// </summary>
public class MainMenuViewModel : ViewModelBase {
	/// <summary>Gets or sets the parent main window ViewModel.</summary>
	public MainWindowViewModel MainWindow { get; set; }

	/// <summary>Gets or sets the File menu items.</summary>
	[Reactive] public List<object> FileMenuItems { get; set; } = new();

	/// <summary>Gets or sets the Game menu items.</summary>
	[Reactive] public List<object> GameMenuItems { get; set; } = new();

	/// <summary>Gets or sets the Options menu items.</summary>
	[Reactive] public List<object> OptionsMenuItems { get; set; } = new();

	/// <summary>Gets or sets the Tools menu items.</summary>
	[Reactive] public List<object> ToolsMenuItems { get; set; } = [];

	/// <summary>Gets or sets the Debug menu items.</summary>
	[Reactive] public List<object> DebugMenuItems { get; set; } = [];

	/// <summary>Gets or sets the Help menu items.</summary>
	[Reactive] public List<object> HelpMenuItems { get; set; } = [];

	/// <summary>NetPlay controller selection items.</summary>
	[Reactive] private List<object> _netPlayControllers { get; set; } = new();

	/// <summary>Gets the current ROM information.</summary>
	private RomInfo RomInfo => MainWindow.RomInfo;

	/// <summary>Gets whether a game is currently running.</summary>
	private bool IsGameRunning => RomInfo.Format != RomFormat.Unknown;

	/// <summary>Gets whether the current game is FDS format.</summary>
	private bool IsFdsGame => RomInfo.Format == RomFormat.Fds;

	/// <summary>Gets whether the current game is VS System.</summary>
	private bool IsVsSystemGame => RomInfo.Format is RomFormat.VsSystem or RomFormat.VsDualSystem;

	/// <summary>Gets whether the current game is VS Dual System.</summary>
	private bool IsVsDualSystemGame => RomInfo.Format == RomFormat.VsDualSystem;

	/// <summary>Gets the list of recent files.</summary>
	private List<RecentItem> RecentItems => ConfigManager.Config.RecentFiles.Items;

	/// <summary>Reference to the configuration window if open.</summary>
	private ConfigWindow? _cfgWindow = null;

	/// <summary>Action for controller selection menu.</summary>
	private SimpleMenuAction _selectControllerAction = new();

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public MainMenuViewModel() : this(new MainWindowViewModel()) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="MainMenuViewModel"/> class.
	/// </summary>
	/// <param name="windowModel">The parent main window ViewModel.</param>
	public MainMenuViewModel(MainWindowViewModel windowModel) {
		MainWindow = windowModel;
	}

	/// <summary>
	/// Opens the configuration window to a specific tab.
	/// </summary>
	/// <param name="wnd">The main window.</param>
	/// <param name="tab">The tab to open.</param>
	private void OpenConfig(MainWindow wnd, ConfigWindowTab tab) {
		if (_cfgWindow == null) {
			_cfgWindow = new ConfigWindow(tab);
			_cfgWindow.Closed += cfgWindow_Closed;
			_cfgWindow.ShowCentered((Control)wnd);
		} else {
			(_cfgWindow.DataContext as ConfigViewModel)!.SelectTab(tab);
			_cfgWindow.BringToFront();
		}
	}

	/// <summary>
	/// Handles config window close event.
	/// </summary>
	private void cfgWindow_Closed(object? sender, EventArgs e) {
		_cfgWindow = null;
		if (ConfigManager.Config.Preferences.GameSelectionScreenMode == GameSelectionMode.Disabled && MainWindow.RecentGames.Visible) {
			MainWindow.RecentGames.Visible = false;
		} else if (ConfigManager.Config.Preferences.GameSelectionScreenMode != GameSelectionMode.Disabled && !IsGameRunning) {
			MainWindow.RecentGames.Init(GameScreenMode.RecentGames);
		}
	}

	/// <summary>
	/// Initializes all menus with the main window reference.
	/// </summary>
	/// <param name="wnd">The main window.</param>
	public void Initialize(MainWindow wnd) {
		InitFileMenu(wnd);
		InitGameMenu(wnd);
		InitOptionsMenu(wnd);
		InitToolMenu(wnd);
		InitDebugMenu(wnd);
		InitHelpMenu(wnd);

		// Initialize all menu items to set correct Enabled state on first render
		UpdateAllMenuItems(FileMenuItems);
		UpdateAllMenuItems(GameMenuItems);
		UpdateAllMenuItems(OptionsMenuItems);
		UpdateAllMenuItems(ToolsMenuItems);
		UpdateAllMenuItems(DebugMenuItems);
		UpdateAllMenuItems(HelpMenuItems);
	}

	/// <summary>
	/// Recursively updates all menu items to set their initial Enabled/Visible state.
	/// Supports both old BaseMenuAction and new IMenuAction classes.
	/// </summary>
	private static void UpdateAllMenuItems(IEnumerable<object> items) {
		foreach (object item in items) {
			if (item is Menus.IMenuAction newAction) {
				newAction.Update();
				if (newAction.SubActions != null) {
					UpdateAllMenuItems(newAction.SubActions);
				}
			} else if (item is BaseMenuAction action) {
				action.Update();
				if (action.SubActions != null) {
					UpdateAllMenuItems(action.SubActions);
				}
			}
		}
	}

	private void InitFileMenu(MainWindow wnd) {
		FileMenuItems = [
			// Open - uses modern async file dialog directly
			new SimpleMenuAction(ActionType.Open) {
				AsyncOnClick = async () => {
					string? initialFolder = ConfigManager.Config.Preferences.OverrideGameFolder && Directory.Exists(ConfigManager.Config.Preferences.GameFolder)
						? ConfigManager.Config.Preferences.GameFolder
						: ConfigManager.Config.RecentFiles.Items.Count > 0 ? ConfigManager.Config.RecentFiles.Items[0].RomFile.Folder : null;
					string? filename = await FileDialogHelper.OpenFile(initialFolder, wnd, FileDialogHelper.RomExt);
					if (filename != null) {
						LoadRomHelper.LoadFile(filename);
					}
				}
			},

			new MenuSeparator(),

			// Save State submenu - requires ROM
			new SimpleMenuAction(ActionType.SaveState) {
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				SubActions = [
					CreateSaveStateMenuItem(1, true),
					CreateSaveStateMenuItem(2, true),
					CreateSaveStateMenuItem(3, true),
					CreateSaveStateMenuItem(4, true),
					CreateSaveStateMenuItem(5, true),
					CreateSaveStateMenuItem(6, true),
					CreateSaveStateMenuItem(7, true),
					CreateSaveStateMenuItem(8, true),
					CreateSaveStateMenuItem(9, true),
					CreateSaveStateMenuItem(10, true),
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.SaveStateDialog, EnableCategory.RequiresRom) {
						ActionType = ActionType.SaveStateDialog
					},
					new ShortcutMenuAction(EmulatorShortcut.SaveStateToFile, EnableCategory.RequiresRom) {
						ActionType = ActionType.SaveStateToFile
					},
				]
			},

			// Load State submenu - requires ROM
			new SimpleMenuAction(ActionType.LoadState) {
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				SubActions = [
					CreateSaveStateMenuItem(1, false),
					CreateSaveStateMenuItem(2, false),
					CreateSaveStateMenuItem(3, false),
					CreateSaveStateMenuItem(4, false),
					CreateSaveStateMenuItem(5, false),
					CreateSaveStateMenuItem(6, false),
					CreateSaveStateMenuItem(7, false),
					CreateSaveStateMenuItem(8, false),
					CreateSaveStateMenuItem(9, false),
					CreateSaveStateMenuItem(10, false),
					new MenuSeparator(),
					CreateSaveStateMenuItem(11, false), // Auto-save slot
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.LoadStateDialog, EnableCategory.RequiresRom) {
						ActionType = ActionType.LoadStateDialog
					},
					new ShortcutMenuAction(EmulatorShortcut.LoadStateFromFile, EnableCategory.RequiresRom) {
						ActionType = ActionType.LoadStateFromFile
					},
				]
			},

			// Load Last Session - requires ROM and session file
			new ShortcutMenuAction(EmulatorShortcut.LoadLastSession) {
				ActionType = ActionType.LoadLastSession,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded &&
					File.Exists(Path.Combine(ConfigManager.RecentGamesFolder, MainWindow.RomInfo.GetRomName() + ".rgd"))
			},

			new MenuSeparator(),

			// Quick Save Timestamped - requires ROM
			new ShortcutMenuAction(EmulatorShortcut.QuickSaveTimestamped, EnableCategory.RequiresRom) {
				ActionType = ActionType.QuickSaveTimestamped
			},

			// Open Save State Picker - requires ROM and existing saves
			new ShortcutMenuAction(EmulatorShortcut.OpenSaveStatePicker) {
				ActionType = ActionType.OpenSaveStatePicker,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded && EmuApi.GetSaveStateCount() > 0
			},

			new MenuSeparator(),

			// Recent Files submenu - always visible, enabled when there are recent files
			new SimpleMenuAction(ActionType.RecentFiles) {
				IsEnabled = () => ConfigManager.Config.RecentFiles.Items.Count > 0,
				SubActions = [
					CreateRecentMenuItem(0),
					CreateRecentMenuItem(1),
					CreateRecentMenuItem(2),
					CreateRecentMenuItem(3),
					CreateRecentMenuItem(4),
					CreateRecentMenuItem(5),
					CreateRecentMenuItem(6),
					CreateRecentMenuItem(7),
					CreateRecentMenuItem(8),
					CreateRecentMenuItem(9)
				]
			},

			new MenuSeparator(),

			// Exit - always enabled
			new ShortcutMenuAction(EmulatorShortcut.Exit, EnableCategory.AlwaysEnabled) {
				ActionType = ActionType.Exit
			},
		];
	}

	/// <summary>
	/// Creates a menu item for a recent file entry.
	/// </summary>
	private SimpleMenuAction CreateRecentMenuItem(int index) {
		return new SimpleMenuAction {
			ActionType = ActionType.Custom,
			DynamicText = () => index < RecentItems.Count ? RecentItems[index].DisplayText : "",
			CustomShortcutText = () => index < RecentItems.Count ? RecentItems[index].ShortenedFolder : "",
			IsVisible = () => index < RecentItems.Count,
			OnClick = () => {
				if (index < RecentItems.Count) {
					LoadRomHelper.LoadRom(RecentItems[index].RomFile, RecentItems[index].PatchFile);
				}
			}
		};
	}

	/// <summary>
	/// Creates a menu item for a save state slot.
	/// </summary>
	private ShortcutMenuAction CreateSaveStateMenuItem(int slot, bool forSave) {
		EmulatorShortcut shortcut = forSave
			? (EmulatorShortcut)((int)EmulatorShortcut.SaveStateSlot1 + slot - 1)
			: (EmulatorShortcut)((int)EmulatorShortcut.LoadStateSlot1 + slot - 1);
		bool isAutoSaveSlot = slot == 11;

		return new ShortcutMenuAction(shortcut, EnableCategory.RequiresRom) {
			ActionType = ActionType.Custom,
			DynamicText = () => {
				// Check for Nexen native format first, then legacy Mesen format
				string statePathNexen = Path.Combine(ConfigManager.SaveStateFolder, EmuApi.GetRomInfo().GetRomName() + "_" + slot + "." + FileDialogHelper.NexenSaveStateExt);
				string statePathMesen = Path.Combine(ConfigManager.SaveStateFolder, EmuApi.GetRomInfo().GetRomName() + "_" + slot + "." + FileDialogHelper.MesenSaveStateExt);
				string statePath = File.Exists(statePathNexen) ? statePathNexen : (File.Exists(statePathMesen) ? statePathMesen : statePathNexen);
				string slotName = isAutoSaveSlot ? "Auto" : slot.ToString();

				string header;
				if (!File.Exists(statePath)) {
					header = slotName + ". " + ResourceHelper.GetMessage("EmptyState");
				} else {
					DateTime dateTime = new FileInfo(statePath).LastWriteTime;
					header = slotName + ". " + dateTime.ToShortDateString() + " " + dateTime.ToShortTimeString();
				}

				return header;
			},
			OnClick = () => {
				if (forSave) {
					EmuApi.SaveState((uint)slot);
				} else {
					EmuApi.LoadState((uint)slot);
				}
			}
		};
	}

	private void InitGameMenu(MainWindow wnd) {
		GameMenuItems = [
			// Pause (shown when running)
			new ShortcutMenuAction(EmulatorShortcut.Pause, EnableCategory.RequiresRom) {
				ActionType = ActionType.Pause,
				IsVisible = () => !EmuApi.IsPaused() && (!ConfigManager.Config.Preferences.PauseWhenInMenusAndConfig || !EmuApi.IsRunning())
			},
			// Resume (shown when paused)
			new ShortcutMenuAction(EmulatorShortcut.Pause, EnableCategory.RequiresRom) {
				ActionType = ActionType.Resume,
				IsVisible = () => EmuApi.IsPaused() || (ConfigManager.Config.Preferences.PauseWhenInMenusAndConfig && EmuApi.IsRunning())
			},

			new MenuSeparator(),

			new ShortcutMenuAction(EmulatorShortcut.Reset, EnableCategory.RequiresRom) {
				ActionType = ActionType.Reset
			},
			new ShortcutMenuAction(EmulatorShortcut.PowerCycle, EnableCategory.RequiresRom) {
				ActionType = ActionType.PowerCycle
			},
			new ShortcutMenuAction(EmulatorShortcut.ReloadRom, EnableCategory.RequiresRom) {
				ActionType = ActionType.ReloadRom
			},

			new MenuSeparator(),

			new ShortcutMenuAction(EmulatorShortcut.PowerOff, EnableCategory.RequiresRom) {
				ActionType = ActionType.PowerOff
			},

			// Game Config - shown for consoles that support it
			new MenuSeparator {
				IsVisible = () => IsGameRunning && RomInfo.ConsoleType != ConsoleType.Gameboy && RomInfo.Format != RomFormat.GameGear && RomInfo.ConsoleType != ConsoleType.Gba
			},
			new SimpleMenuAction(ActionType.GameConfig) {
				IsVisible = () => IsGameRunning && RomInfo.ConsoleType != ConsoleType.Gameboy && RomInfo.Format != RomFormat.GameGear && RomInfo.ConsoleType != ConsoleType.Gba,
				IsEnabled = () => IsGameRunning,
				OnClick = () => new GameConfigWindow().ShowCenteredDialog((Control)wnd)
			},

			// FDS disk selection
			new MenuSeparator { IsVisible = () => IsFdsGame },
			new SimpleMenuAction(ActionType.SelectDisk) {
				IsVisible = () => IsFdsGame,
				SubActions = [
					CreateFdsInsertDiskItem(0),
					CreateFdsInsertDiskItem(1),
					CreateFdsInsertDiskItem(2),
					CreateFdsInsertDiskItem(3),
					CreateFdsInsertDiskItem(4),
					CreateFdsInsertDiskItem(5),
					CreateFdsInsertDiskItem(6),
					CreateFdsInsertDiskItem(7),
				]
			},
			new ShortcutMenuAction(EmulatorShortcut.FdsEjectDisk) {
				ActionType = ActionType.EjectDisk,
				IsVisible = () => IsFdsGame,
			},

			// VS System coin insertion
			new MenuSeparator { IsVisible = () => IsVsSystemGame },
			new ShortcutMenuAction(EmulatorShortcut.VsInsertCoin1) {
				ActionType = ActionType.InsertCoin1,
				IsVisible = () => IsVsSystemGame
			},
			new ShortcutMenuAction(EmulatorShortcut.VsInsertCoin2) {
				ActionType = ActionType.InsertCoin2,
				IsVisible = () => IsVsSystemGame
			},
			new ShortcutMenuAction(EmulatorShortcut.VsInsertCoin3) {
				ActionType = ActionType.InsertCoin3,
				IsVisible = () => IsVsDualSystemGame
			},
			new ShortcutMenuAction(EmulatorShortcut.VsInsertCoin4) {
				ActionType = ActionType.InsertCoin4,
				IsVisible = () => IsVsDualSystemGame
			},

			// Barcode input and tape recorder
			new MenuSeparator {
				IsVisible = () => EmuApi.IsShortcutAllowed(EmulatorShortcut.InputBarcode) ||
				                  EmuApi.IsShortcutAllowed(EmulatorShortcut.RecordTape) ||
				                  EmuApi.IsShortcutAllowed(EmulatorShortcut.StopRecordTape)
			},
			new ShortcutMenuAction(EmulatorShortcut.InputBarcode) {
				ActionType = ActionType.InputBarcode,
				IsVisible = () => EmuApi.IsShortcutAllowed(EmulatorShortcut.InputBarcode)
			},
			new SimpleMenuAction(ActionType.TapeRecorder) {
				IsVisible = () => EmuApi.IsShortcutAllowed(EmulatorShortcut.RecordTape) ||
				                  EmuApi.IsShortcutAllowed(EmulatorShortcut.StopRecordTape),
				SubActions = [
					new ShortcutMenuAction(EmulatorShortcut.LoadTape) {
						ActionType = ActionType.Play,
					},
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.RecordTape) {
						ActionType = ActionType.Record,
					},
					new ShortcutMenuAction(EmulatorShortcut.StopRecordTape) {
						ActionType = ActionType.Stop,
					},
				]
			},
		];
	}

	/// <summary>
	/// Creates a menu item for FDS disk side selection.
	/// </summary>
	private ShortcutMenuAction CreateFdsInsertDiskItem(int diskSide) {
		return new ShortcutMenuAction(EmulatorShortcut.FdsInsertDiskNumber) {
			ActionType = ActionType.Custom,
			CustomText = "Disk " + ((diskSide / 2) + 1) + " Side " + ((diskSide % 2 == 0) ? "A" : "B"),
			ShortcutParam = (uint)diskSide,
			IsVisible = () => EmuApi.IsShortcutAllowed(EmulatorShortcut.FdsInsertDiskNumber, (uint)diskSide)
		};
	}

	private void InitOptionsMenu(MainWindow wnd) {
		OptionsMenuItems = [
			new SimpleMenuAction() {
				ActionType = ActionType.Speed,
				SubActions = [
					GetSpeedMenuItem(ActionType.NormalSpeed, 100),
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.IncreaseSpeed) {
						ActionType = ActionType.IncreaseSpeed
					},
					new ShortcutMenuAction(EmulatorShortcut.DecreaseSpeed) {
						ActionType = ActionType.DecreaseSpeed
					},
					GetSpeedMenuItem(ActionType.MaximumSpeed, 0, EmulatorShortcut.MaxSpeed),
					new MenuSeparator(),
					GetSpeedMenuItem(ActionType.TripleSpeed, 300),
					GetSpeedMenuItem(ActionType.DoubleSpeed, 200),
					GetSpeedMenuItem(ActionType.HalfSpeed, 50),
					GetSpeedMenuItem(ActionType.QuarterSpeed, 25),
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.ToggleFps) {
						ActionType = ActionType.ShowFps,
						IsSelected = () => ConfigManager.Config.Preferences.ShowFps
					}
				]
			},

			new SimpleMenuAction() {
				ActionType = ActionType.VideoScale,
				SubActions = [
					GetScaleMenuItem(1, EmulatorShortcut.SetScale1x),
					GetScaleMenuItem(2, EmulatorShortcut.SetScale2x),
					GetScaleMenuItem(3, EmulatorShortcut.SetScale3x),
					GetScaleMenuItem(4, EmulatorShortcut.SetScale4x),
					GetScaleMenuItem(5, EmulatorShortcut.SetScale5x),
					GetScaleMenuItem(6, EmulatorShortcut.SetScale6x),
					GetScaleMenuItem(7, EmulatorShortcut.SetScale7x),
					GetScaleMenuItem(8, EmulatorShortcut.SetScale8x),
					GetScaleMenuItem(9, EmulatorShortcut.SetScale9x),
					GetScaleMenuItem(10, EmulatorShortcut.SetScale10x),
					new MenuSeparator(),
					new ShortcutMenuAction(EmulatorShortcut.ToggleFullscreen) {
						ActionType = ActionType.Fullscreen
					},
				]
			},

			new SimpleMenuAction() {
				ActionType = ActionType.VideoFilter,
				SubActions = [
					GetVideoFilterMenuItem(VideoFilterType.None),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.NtscBlargg),
					GetVideoFilterMenuItem(VideoFilterType.NtscBisqwit),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.LcdGrid),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.xBRZ2x),
					GetVideoFilterMenuItem(VideoFilterType.xBRZ3x),
					GetVideoFilterMenuItem(VideoFilterType.xBRZ4x),
					GetVideoFilterMenuItem(VideoFilterType.xBRZ5x),
					GetVideoFilterMenuItem(VideoFilterType.xBRZ6x),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.HQ2x),
					GetVideoFilterMenuItem(VideoFilterType.HQ3x),
					GetVideoFilterMenuItem(VideoFilterType.HQ4x),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.Scale2x),
					GetVideoFilterMenuItem(VideoFilterType.Scale3x),
					GetVideoFilterMenuItem(VideoFilterType.Scale4x),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType._2xSai),
					GetVideoFilterMenuItem(VideoFilterType.Super2xSai),
					GetVideoFilterMenuItem(VideoFilterType.SuperEagle),
					new MenuSeparator(),
					GetVideoFilterMenuItem(VideoFilterType.Prescale2x),
					GetVideoFilterMenuItem(VideoFilterType.Prescale3x),
					GetVideoFilterMenuItem(VideoFilterType.Prescale4x),
					GetVideoFilterMenuItem(VideoFilterType.Prescale6x),
					GetVideoFilterMenuItem(VideoFilterType.Prescale8x),
					GetVideoFilterMenuItem(VideoFilterType.Prescale10x),
					new MenuSeparator(),
					new SimpleMenuAction() {
						ActionType = ActionType.ToggleBilinearInterpolation,
						IsSelected = () => ConfigManager.Config.Video.UseBilinearInterpolation,
						OnClick = () => {
							ConfigManager.Config.Video.UseBilinearInterpolation = !ConfigManager.Config.Video.UseBilinearInterpolation;
							ConfigManager.Config.Video.ApplyConfig();
						}
					}
				]
			},

			new SimpleMenuAction() {
				ActionType = ActionType.AspectRatio,
				SubActions = [
					GetAspectRatioMenuItem(VideoAspectRatio.NoStretching),
					new MenuSeparator(),
					GetAspectRatioMenuItem(VideoAspectRatio.Auto),
					new MenuSeparator(),
					GetAspectRatioMenuItem(VideoAspectRatio.NTSC),
					GetAspectRatioMenuItem(VideoAspectRatio.PAL),
					GetAspectRatioMenuItem(VideoAspectRatio.Standard),
					GetAspectRatioMenuItem(VideoAspectRatio.Widescreen),
					new MenuSeparator(),
					GetAspectRatioMenuItem(VideoAspectRatio.Custom),
				]
			},

			new SimpleMenuAction() {
				ActionType = ActionType.Region,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				IsVisible = () =>
					!EmulatorState.Instance.IsRomLoaded ||
					(MainWindow.RomInfo.ConsoleType != ConsoleType.Gameboy && MainWindow.RomInfo.ConsoleType != ConsoleType.Gba)
				,
				SubActions = [
					GetRegionMenuItem(ConsoleRegion.Auto),
					GetPcEngineModelMenuItem(PceConsoleType.Auto),
					GetWsModelMenuItem(WsModel.Auto),
					new MenuSeparator(),
					GetRegionMenuItem(ConsoleRegion.Ntsc),
					GetRegionMenuItem(ConsoleRegion.NtscJapan),
					GetRegionMenuItem(ConsoleRegion.Pal),
					GetRegionMenuItem(ConsoleRegion.Dendy),
					GetPcEngineModelMenuItem(PceConsoleType.PcEngine),
					GetPcEngineModelMenuItem(PceConsoleType.SuperGrafx),
					GetPcEngineModelMenuItem(PceConsoleType.TurboGrafx),
					GetWsModelMenuItem(WsModel.Monochrome),
					GetWsModelMenuItem(WsModel.Color),
					GetWsModelMenuItem(WsModel.SwanCrystal),
				]
			},

			new SimpleMenuAction() {
				ActionType = ActionType.Region,
				DynamicText = () => ResourceHelper.GetEnumText(ActionType.Region) + (MainWindow.RomInfo.ConsoleType != ConsoleType.Gameboy ? " (GB)" : ""),
				IsVisible = () => EmulatorState.Instance.IsRomLoaded && MainWindow.RomInfo.CpuTypes.Contains(CpuType.Gameboy),
				SubActions = [
					GetGameboyModelMenuItem(GameboyModel.AutoFavorGbc),
					GetGameboyModelMenuItem(GameboyModel.AutoFavorSgb),
					GetGameboyModelMenuItem(GameboyModel.AutoFavorGb),
					new MenuSeparator(),
					GetGameboyModelMenuItem(GameboyModel.Gameboy),
					GetGameboyModelMenuItem(GameboyModel.GameboyColor),
					GetGameboyModelMenuItem(GameboyModel.SuperGameboy),
				]
			},

			new MenuSeparator(),

			new SimpleMenuAction() {
				ActionType = ActionType.Audio,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Audio)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Emulation,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Emulation)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Input,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Input)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Video,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Video)
			},

			new MenuSeparator(),

			new SimpleMenuAction() {
				ActionType = ActionType.Nes,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Nes)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Snes,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Snes)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Gameboy,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Gameboy)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Gba,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Gba)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.PcEngine,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.PcEngine)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Sms,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Sms)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.Ws,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Ws)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.OtherConsoles,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.OtherConsoles)
			},
			new MenuSeparator(),

			new SimpleMenuAction() {
				ActionType = ActionType.Preferences,
				OnClick = () => OpenConfig(wnd, ConfigWindowTab.Preferences)
			}
		];
	}

	private SimpleMenuAction GetAspectRatioMenuItem(VideoAspectRatio aspectRatio) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(aspectRatio),
			IsSelected = () => {
				ConsoleOverrideConfig? overrides = ConsoleOverrideConfig.GetActiveOverride();
				if (overrides?.OverrideAspectRatio == true) {
					return aspectRatio == overrides.AspectRatio;
				}

				return aspectRatio == ConfigManager.Config.Video.AspectRatio;
			},
			OnClick = () => {
				ConsoleOverrideConfig? overrides = ConsoleOverrideConfig.GetActiveOverride();
				if (overrides?.OverrideAspectRatio == true) {
					overrides.AspectRatio = aspectRatio;
				} else {
					ConfigManager.Config.Video.AspectRatio = aspectRatio;
				}

				ConfigManager.Config.Video.ApplyConfig();
			}
		};
	}

	private SimpleMenuAction GetRegionMenuItem(ConsoleRegion region) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			DynamicText = () => {
				if (region == ConsoleRegion.Pal && MainWindow.RomInfo.Format == RomFormat.GameGear) {
					return "PAL (60 FPS)"; //GG is 60fps even when region is PAL
				}

				return ResourceHelper.GetEnumText(region);
			},
			IsVisible = () => {
				if (MainWindow.RomInfo.ConsoleType is ConsoleType.PcEngine or ConsoleType.Gameboy or ConsoleType.Ws) {
					return false;
				}

				return region switch {
					ConsoleRegion.Ntsc => true,
					ConsoleRegion.NtscJapan => MainWindow.RomInfo.Format == RomFormat.GameGear,
					ConsoleRegion.Pal => true,
					ConsoleRegion.Dendy => MainWindow.RomInfo.ConsoleType == ConsoleType.Nes,
					ConsoleRegion.Auto or _ => true
				};
			},
			IsSelected = () => MainWindow.RomInfo.ConsoleType switch {
				ConsoleType.Snes => ConfigManager.Config.Snes.Region == region,
				ConsoleType.Nes => ConfigManager.Config.Nes.Region == region,
				ConsoleType.Sms =>
					MainWindow.RomInfo.Format switch {
						RomFormat.ColecoVision => ConfigManager.Config.Cv.Region,
						RomFormat.GameGear => ConfigManager.Config.Sms.GameGearRegion,
						_ => ConfigManager.Config.Sms.Region
					} == region
				,
				_ => region == ConsoleRegion.Auto
			},
			OnClick = () => {
				switch (MainWindow.RomInfo.ConsoleType) {
					case ConsoleType.Snes:
						ConfigManager.Config.Snes.Region = region;
						ConfigManager.Config.Snes.ApplyConfig();
						break;

					case ConsoleType.Nes:
						ConfigManager.Config.Nes.Region = region;
						ConfigManager.Config.Nes.ApplyConfig();
						break;

					case ConsoleType.Sms:
						switch (MainWindow.RomInfo.Format) {
							default: case RomFormat.Sms: ConfigManager.Config.Sms.Region = region; break;
							case RomFormat.GameGear: ConfigManager.Config.Sms.GameGearRegion = region; break;
							case RomFormat.ColecoVision: ConfigManager.Config.Cv.Region = region; break;
						}

						ConfigManager.Config.Sms.ApplyConfig();
						ConfigManager.Config.Cv.ApplyConfig();
						break;

					default:
						break;
				}
			}
		};
	}

	private SimpleMenuAction GetPcEngineModelMenuItem(PceConsoleType model) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(model),
			IsVisible = () => MainWindow.RomInfo.ConsoleType == ConsoleType.PcEngine,
			IsSelected = () => ConfigManager.Config.PcEngine.ConsoleType == model,
			OnClick = () => {
				ConfigManager.Config.PcEngine.ConsoleType = model;
				ConfigManager.Config.PcEngine.ApplyConfig();
			}
		};
	}

	private SimpleMenuAction GetWsModelMenuItem(WsModel model) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(model),
			IsVisible = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Ws,
			IsSelected = () => ConfigManager.Config.Ws.Model == model,
			OnClick = () => {
				ConfigManager.Config.Ws.Model = model;
				ConfigManager.Config.Ws.ApplyConfig();
			}
		};
	}

	private SimpleMenuAction GetGameboyModelMenuItem(GameboyModel model) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(model),
			IsSelected = () => ConfigManager.Config.Gameboy.Model == model,
			OnClick = () => {
				ConfigManager.Config.Gameboy.Model = model;
				ConfigManager.Config.Gameboy.ApplyConfig();
			}
		};
	}

	private bool AllowFilterType(VideoFilterType filter) {
		switch (filter) {
			case VideoFilterType.NtscBisqwit: return MainWindow.RomInfo.ConsoleType == ConsoleType.Nes;
			default: return true;
		}
	}

	private SimpleMenuAction GetVideoFilterMenuItem(VideoFilterType filter) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(filter),
			IsEnabled = () => AllowFilterType(filter),
			IsSelected = () => {
				ConsoleOverrideConfig? overrides = ConsoleOverrideConfig.GetActiveOverride();
				if (overrides?.OverrideVideoFilter == true) {
					return filter == overrides.VideoFilter;
				}

				return filter == ConfigManager.Config.Video.VideoFilter;
			},
			OnClick = () => {
				ConsoleOverrideConfig? overrides = ConsoleOverrideConfig.GetActiveOverride();
				if (overrides?.OverrideVideoFilter == true) {
					overrides.VideoFilter = filter;
				} else {
					ConfigManager.Config.Video.VideoFilter = filter;
				}

				ConfigManager.Config.Video.ApplyConfig();
			}
		};
	}

	private ShortcutMenuAction GetScaleMenuItem(int scale, EmulatorShortcut shortcut) {
		return new ShortcutMenuAction(shortcut) {
			ActionType = ActionType.Custom,
			CustomText = scale + "x",
			IsSelected = () => (int)((double)MainWindow.RendererSize.Height / EmuApi.GetBaseScreenSize().Height) == scale
		};
	}

	private MenuActionBase GetSpeedMenuItem(ActionType action, int speed, EmulatorShortcut? shortcut = null) {
		if (shortcut != null) {
			return new ShortcutMenuAction(shortcut.Value) {
				ActionType = action,
				IsSelected = () => ConfigManager.Config.Emulation.EmulationSpeed == speed,
			};
		}

		return new SimpleMenuAction() {
			ActionType = action,
			IsSelected = () => ConfigManager.Config.Emulation.EmulationSpeed == speed,
			OnClick = () => {
				ConfigManager.Config.Emulation.EmulationSpeed = (uint)speed;
				ConfigManager.Config.Emulation.ApplyConfig();
			}
		};
	}

	private SimpleMenuAction GetMoviesMenu(MainWindow wnd) {
		return new SimpleMenuAction() {
			ActionType = ActionType.Movies,
			SubActions = [
				new SimpleMenuAction() {
					ActionType = ActionType.Play,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && !RecordApi.MovieRecording() && !RecordApi.MoviePlaying(),
					OnClick = async () => {
						string? filename = await FileDialogHelper.OpenFile(ConfigManager.MovieFolder, wnd, FileDialogHelper.NexenMovieExt, FileDialogHelper.MesenMovieExt);
						if(filename != null) {
							RecordApi.MoviePlay(filename);
						}
					}
				},
				new SimpleMenuAction() {
					ActionType = ActionType.Record,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && !RecordApi.MovieRecording() && !RecordApi.MoviePlaying(),
					OnClick = () => new MovieRecordWindow() {
							DataContext = new MovieRecordConfigViewModel()
						}.ShowCenteredDialog((Control)wnd)                },
				new SimpleMenuAction() {
					ActionType = ActionType.Stop,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && (RecordApi.MovieRecording() || RecordApi.MoviePlaying()),
					OnClick = () => RecordApi.MovieStop()                  }
			]
		};
	}

	private void InitToolMenu(MainWindow wnd) {
		ToolsMenuItems = [
			new SimpleMenuAction() {
				ActionType = ActionType.Cheats,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded && MainWindow.RomInfo.ConsoleType.SupportsCheats(),
				OnClick = () => ApplicationHelper.GetOrCreateUniqueWindow(wnd, () => new CheatListWindow())          },

			new SimpleMenuAction() {
				ActionType = ActionType.HistoryViewer,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded && HistoryApi.HistoryViewerEnabled(),
				OnClick = () => ApplicationHelper.GetOrCreateUniqueWindow(null, () => new HistoryViewerWindow())                },

			new SimpleMenuAction() {
				ActionType = ActionType.TasEditor,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => ApplicationHelper.GetOrCreateUniqueWindow(wnd, () => new TasEditorWindow())                },

			GetMoviesMenu(wnd),
			GetNetPlayMenu(wnd),

			new MenuSeparator(),

			GetSoundRecorderMenu(wnd),
			GetVideoRecorderMenu(wnd),

			new MenuSeparator() {
				IsVisible = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Nes
			},

			new SimpleMenuAction() {
				ActionType = ActionType.HdPacks,
				IsVisible = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Nes,
				SubActions = [
					new SimpleMenuAction() {
						ActionType = ActionType.InstallHdPack,
						OnClick = () => InstallHdPack(wnd)
					},
					new SimpleMenuAction() {
						ActionType = ActionType.HdPackBuilder,
						OnClick = () => ApplicationHelper.GetOrCreateUniqueWindow(wnd, () => new HdPackBuilderWindow())                  }
				]
			},

			new MenuSeparator(),

			new SimpleMenuAction() {
				ActionType = ActionType.LogWindow,
				OnClick = () => ApplicationHelper.GetOrCreateUniqueWindow(wnd, () => new LogWindow())              },

			new ShortcutMenuAction(EmulatorShortcut.TakeScreenshot) {
				ActionType = ActionType.TakeScreenshot,
			},
		];
	}

	private SimpleMenuAction GetVideoRecorderMenu(MainWindow wnd) {
		return new SimpleMenuAction() {
			ActionType = ActionType.VideoRecorder,
			SubActions = [
				new SimpleMenuAction() {
					ActionType = ActionType.Record,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && !RecordApi.AviIsRecording(),
					OnClick = () => new VideoRecordWindow() {
							DataContext = new VideoRecordConfigViewModel()
						}.ShowCenteredDialog((Control)wnd)                },
				new SimpleMenuAction() {
					ActionType = ActionType.Stop,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && RecordApi.AviIsRecording(),
					OnClick = () => RecordApi.AviStop()              }
			]
		};
	}

	private SimpleMenuAction GetSoundRecorderMenu(MainWindow wnd) {
		return new SimpleMenuAction() {
			ActionType = ActionType.SoundRecorder,
			SubActions = [
				new SimpleMenuAction() {
					ActionType = ActionType.Record,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && !RecordApi.WaveIsRecording(),
					OnClick = async () => {
						string? filename = await FileDialogHelper.SaveFile(ConfigManager.WaveFolder, EmuApi.GetRomInfo().GetRomName() + ".wav", wnd, FileDialogHelper.WaveExt);
						if(filename != null) {
							RecordApi.WaveRecord(filename);
						}
					}
				},
				new SimpleMenuAction() {
					ActionType = ActionType.Stop,
					IsEnabled = () => EmulatorState.Instance.IsRomLoaded && RecordApi.WaveIsRecording(),
					OnClick = () => RecordApi.WaveStop()                    }
			]
		};
	}

	private SimpleMenuAction GetNetPlayMenu(MainWindow wnd) {
		_selectControllerAction = new SimpleMenuAction() {
			ActionType = ActionType.SelectController,
			IsEnabled = () => NetplayApi.IsConnected() || NetplayApi.IsServerRunning(),
			SubActions = _netPlayControllers
		};

		return new SimpleMenuAction() {
			ActionType = ActionType.NetPlay,
			SubActions = [
				new SimpleMenuAction() {
					ActionType = ActionType.Connect,
					IsEnabled = () => !NetplayApi.IsConnected() && !NetplayApi.IsServerRunning(),
					OnClick = () => new NetplayConnectWindow() {
							DataContext = ConfigManager.Config.Netplay.Clone()
						}.ShowCenteredDialog((Control)wnd)                },

				new SimpleMenuAction() {
					ActionType = ActionType.Disconnect,
					IsEnabled = () => NetplayApi.IsConnected(),
					OnClick = () => NetplayApi.Disconnect()              },

				new MenuSeparator(),

				new SimpleMenuAction() {
					ActionType = ActionType.StartServer,
					IsEnabled = () => !NetplayApi.IsConnected() && !NetplayApi.IsServerRunning(),
					OnClick = () => new NetplayStartServerWindow() {
							DataContext = ConfigManager.Config.Netplay.Clone()
						}.ShowCenteredDialog((Control)wnd)                },

				new SimpleMenuAction() {
					ActionType = ActionType.StopServer,
					IsEnabled = () => NetplayApi.IsServerRunning(),
					OnClick = () => NetplayApi.StopServer()              },

				new MenuSeparator(),

				_selectControllerAction
			]
		};
	}

	private void InitDebugMenu(Window wnd) {
		Func<bool> isSuperGameBoy = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Snes && MainWindow.RomInfo.Format == RomFormat.Gb;
		Func<bool> isNesFormat = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Nes && (MainWindow.RomInfo.Format == RomFormat.iNes || MainWindow.RomInfo.Format == RomFormat.VsSystem || MainWindow.RomInfo.Format == RomFormat.VsDualSystem);

		DebugMenuItems = [
			new DebugMenuAction() {
				ActionType = ActionType.OpenDebugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenDebugger),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebuggerWindow.GetOrOpenWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenSpcDebugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenSpcDebugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.Spc),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.Spc)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenCx4Debugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenCx4Debugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.Cx4),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.Cx4)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenNecDspDebugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenNecDspDebugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.NecDsp),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.NecDsp)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenGsuDebugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenGsuDebugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.Gsu),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.Gsu)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenSa1Debugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenSa1Debugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.Sa1),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.Sa1)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenSt018Debugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenSt018Debugger),
				IsVisible = () => MainWindow.RomInfo.CpuTypes.Contains(CpuType.St018),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.St018)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenGameboyDebugger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenGameboyDebugger),
				IsVisible = () => MainWindow.RomInfo.ConsoleType == ConsoleType.Snes && MainWindow.RomInfo.CpuTypes.Contains(CpuType.Gameboy),
				OnClick = () => DebuggerWindow.GetOrOpenWindow(CpuType.Gameboy)
			},
			new DebugMenuSeparator(),
			new DebugMenuAction() {
				ActionType = ActionType.OpenEventViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenEventViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => EventViewerWindow.GetOrOpenWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenMemoryTools,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenMemoryTools),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new MemoryToolsWindow())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenRegisterViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenRegisterViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new RegisterViewerWindow(new RegisterViewerWindowViewModel()))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenTraceLogger,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenTraceLogger),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.GetOrOpenDebugWindow(() => new TraceLoggerWindow(new TraceLoggerViewModel()))
			},
			new DebugMenuSeparator(),
			new DebugMenuAction() {
				ActionType = ActionType.OpenTilemapViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenTilemapViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new TilemapViewerWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType()))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenTileViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenTileViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new TileViewerWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType()))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenSpriteViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenSpriteViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new SpriteViewerWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType()))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenPaletteViewer,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenPaletteViewer),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new PaletteViewerWindow(MainWindow.RomInfo.ConsoleType.GetMainCpuType()))
			},
			new DebugMenuSeparator(),
			new DebugMenuAction() {
				ActionType = ActionType.OpenAssembler,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenAssembler),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded && MainWindow.RomInfo.ConsoleType.GetMainCpuType().SupportsAssembler(),
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new AssemblerWindow(new AssemblerWindowViewModel(MainWindow.RomInfo.ConsoleType.GetMainCpuType())))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenDebugLog,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenDebugLog),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.GetOrOpenDebugWindow(() => new DebugLogWindow())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenMemorySearch,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenMemorySearch),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.GetOrOpenDebugWindow(() => new MemorySearchWindow())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenProfiler,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenProfiler),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.GetOrOpenDebugWindow(() => new ProfilerWindow())
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenScriptWindow,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenScriptWindow),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new ScriptWindow(new ScriptWindowViewModel(null)))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenWatchWindow,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenWatchWindow),
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.GetOrOpenDebugWindow(() => new WatchWindow(new WatchWindowViewModel()))
			},
			new DebugMenuSeparator() { IsVisible = isSuperGameBoy },
			new DebugMenuAction() {
				ActionType = ActionType.OpenTilemapViewer,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new TilemapViewerWindow(CpuType.Gameboy))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenTileViewer,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new TileViewerWindow(CpuType.Gameboy))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenSpriteViewer,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new SpriteViewerWindow(CpuType.Gameboy))
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenPaletteViewer,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new PaletteViewerWindow(CpuType.Gameboy))
			},

			new DebugMenuSeparator() { IsVisible = isSuperGameBoy },
			new DebugMenuAction() {
				ActionType = ActionType.OpenEventViewer,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => EventViewerWindow.GetOrOpenWindow(CpuType.Gameboy)
			},
			new DebugMenuAction() {
				ActionType = ActionType.OpenAssembler,
				HintText = () => "GB",
				IsVisible = isSuperGameBoy,
				IsEnabled = () => EmulatorState.Instance.IsRomLoaded,
				OnClick = () => DebugWindowManager.OpenDebugWindow(() => new AssemblerWindow(new AssemblerWindowViewModel(CpuType.Gameboy)))
			},

			new DebugMenuSeparator() { IsVisible = isNesFormat },
			new DebugMenuAction() {
				ActionType = ActionType.OpenNesHeaderEditor,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenNesHeaderEditor),
				IsVisible = isNesFormat,
				OnClick = () => new NesHeaderEditWindow().ShowCenteredDialog((Control)wnd)
			},

			new DebugMenuSeparator(),
			new DebugMenuAction() {
				ActionType = ActionType.OpenDebugSettings,
				Shortcut = () => ConfigManager.Config.Debug.Shortcuts.Get(DebuggerShortcut.OpenDebugSettings),
				OnClick = () => DebuggerConfigWindow.Open(DebugConfigWindowTab.Debugger, wnd)
			}
		];

		DebugShortcutManager.RegisterActions(wnd, DebugMenuItems);
	}

	private void InitHelpMenu(Window wnd) {
		HelpMenuItems = [
			new SimpleMenuAction() {
				ActionType = ActionType.OnlineHelp,
				IsVisible = () => false,
				OnClick = () => ApplicationHelper.OpenBrowser("https://www.mesen.ca/documentation/")
			},
			new SimpleMenuAction() {
				ActionType = ActionType.CommandLineHelp,
				OnClick = () => new CommandLineHelpWindow().ShowCenteredDialog((Control)wnd)                },
			new SimpleMenuAction() {
				ActionType = ActionType.CheckForUpdates,
				OnClick = () => CheckForUpdate(wnd, false)
			},
			new SimpleMenuAction() {
				ActionType = ActionType.ReportBug,
				IsVisible = () => false,
				OnClick = () => ApplicationHelper.OpenBrowser("https://www.mesen.ca/reportbug/")
			},
			new MenuSeparator(),
			new SimpleMenuAction() {
				ActionType = ActionType.About,
				OnClick = () => new AboutWindow().ShowCenteredDialog((Control)wnd)            },
		];
	}

	public void CheckForUpdate(Window mainWindow, bool silent) {
		Task.Run(async () => {
			UpdatePromptViewModel? updateInfo = await UpdatePromptViewModel.GetUpdateInformation(silent);
			if (updateInfo == null) {
				if (!silent) {
					Dispatcher.UIThread.Post(() => NexenMsgBox.Show(null, "UpdateDownloadFailed", MessageBoxButtons.OK, MessageBoxIcon.Info));
				}

				return;
			}

			if (updateInfo.LatestVersion > updateInfo.InstalledVersion) {
				Dispatcher.UIThread.Post(async () => {
					UpdatePromptWindow updatePrompt = new UpdatePromptWindow(updateInfo);
					if (await updatePrompt.ShowCenteredDialog<bool>(mainWindow) == true) {
						mainWindow.Close();
					}
				});
			} else if (!silent) {
				Dispatcher.UIThread.Post(() => NexenMsgBox.Show(null, "NexenUpToDate", MessageBoxButtons.OK, MessageBoxIcon.Info));
			}
		});
	}

	private async void InstallHdPack(Window wnd) {
		string? filename = await FileDialogHelper.OpenFile(null, wnd, FileDialogHelper.ZipExt);
		if (filename == null) {
			return;
		}

		try {
			using (FileStream? stream = FileHelper.OpenRead(filename)) {
				if (stream == null) {
					return;
				}

				ZipArchive zip = new ZipArchive(stream);

				//Find the hires.txt file
				ZipArchiveEntry? hiresEntry = null;

				//Find the most top-level hires.txt file in the zip
				int minDepth = int.MaxValue;
				foreach (ZipArchiveEntry entry in zip.Entries) {
					if (entry.Name == "hires.txt") {
						string? folder = Path.GetDirectoryName(entry.FullName);
						int depth = 0;
						if (folder != null) {
							do {
								depth++;
							} while ((folder = Path.GetDirectoryName(folder)) != null);
						}

						if (depth < minDepth) {
							minDepth = depth;
							hiresEntry = entry;
							if (depth == 0) {
								break;
							}
						}
					}
				}

				if (hiresEntry == null) {
					await NexenMsgBox.Show(wnd, "InstallHdPackInvalidPack", MessageBoxButtons.OK, MessageBoxIcon.Error);
					return;
				}

				using Stream entryStream = hiresEntry.Open();
				using StreamReader reader = new StreamReader(entryStream);
				string hiresData = reader.ReadToEnd();
				RomInfo romInfo = EmuApi.GetRomInfo();

				//If there's a "supportedRom" tag, check if it matches the current ROM
				Regex supportedRomRegex = new Regex("<supportedRom>([^\\n]*)");
				Match match = supportedRomRegex.Match(hiresData);
				if (match.Success) {
					if (!match.Groups[1].Value.ToUpper().Contains(EmuApi.GetRomHash(HashType.Sha1).ToUpper())) {
						await NexenMsgBox.Show(wnd, "InstallHdPackWrongRom", MessageBoxButtons.OK, MessageBoxIcon.Error);
						return;
					}
				}

				//Extract HD pack
				try {
					string targetFolder = Path.Combine(ConfigManager.HdPackFolder, romInfo.GetRomName());
					if (Directory.Exists(targetFolder)) {
						//Warn if the folder already exists
						if (await NexenMsgBox.Show(wnd, "InstallHdPackConfirmOverwrite", MessageBoxButtons.OKCancel, MessageBoxIcon.Question, targetFolder) != DialogResult.OK) {
							return;
						}
					} else {
						Directory.CreateDirectory(targetFolder);
					}

					string hiresFileFolder = hiresEntry.FullName.Substring(0, hiresEntry.FullName.Length - "hires.txt".Length);
					foreach (ZipArchiveEntry entry in zip.Entries) {
						//Extract only the files in the same subfolder as the hires.txt file (and only if they have a name & size > 0)
						if (!string.IsNullOrWhiteSpace(entry.Name) && entry.Length > 0 && entry.FullName.StartsWith(hiresFileFolder)) {
							string filePath = Path.Combine(targetFolder, entry.FullName.Substring(hiresFileFolder.Length));
							string? fileFolder = Path.GetDirectoryName(filePath);
							if (fileFolder != null) {
								Directory.CreateDirectory(fileFolder);
							}

							entry.ExtractToFile(filePath, true);
						}
					}
				} catch (Exception ex) {
					await NexenMsgBox.Show(wnd, "InstallHdPackError", MessageBoxButtons.OK, MessageBoxIcon.Error, ex.ToString());
					return;
				}

				//Turn on HD Pack support automatically after installation succeeds
				if (!ConfigManager.Config.Nes.EnableHdPacks) {
					ConfigManager.Config.Nes.EnableHdPacks = true;
					ConfigManager.Config.Nes.ApplyConfig();
				}

				if (await NexenMsgBox.Show(wnd, "InstallHdPackConfirmReset", MessageBoxButtons.OKCancel, MessageBoxIcon.Question) == DialogResult.OK) {
					//Power cycle game if the user agrees
					LoadRomHelper.PowerCycle();
				}
			}
		} catch {
			//Invalid file (file missing, not a zip file, etc.)
			await NexenMsgBox.Show(wnd, "InstallHdPackInvalidZipFile", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}
	}

	public bool UpdateNetplayMenu() {
		if (!NetplayApi.IsServerRunning() && !NetplayApi.IsConnected()) {
			return false;
		}

		List<object> controllerActions = new();

		NetplayControllerInfo clientPort = NetplayApi.NetPlayGetControllerPort();

		int playerIndex = 1;
		NetplayControllerUsageInfo[] controllers = NetplayApi.NetPlayGetControllerList();

		for (int i = 0; i < controllers.Length; i++) {
			NetplayControllerUsageInfo controller = controllers[i];
			if (controller.Type == ControllerType.None) {
				//Skip this controller/port (when type is none, the "port" for hardware buttons, etc.)
				continue;
			}

			SimpleMenuAction action = new() {
				ActionType = ActionType.Custom,
				CustomText = ResourceHelper.GetMessage("Player") + " " + playerIndex + " (" + ResourceHelper.GetEnumText(controller.Type) + ")",
				IsSelected = () => controller.Port.Port == clientPort.Port && controller.Port.SubPort == clientPort.SubPort,
				IsEnabled = () => !controller.InUse,
				OnClick = () => NetplayApi.NetPlaySelectController(controller.Port)
			};
			controllerActions.Add(action);
			playerIndex++;
		}

		if (controllerActions.Count > 0) {
			controllerActions.Add(new MenuSeparator());
		}

		controllerActions.Add(new SimpleMenuAction() {
			ActionType = ActionType.Custom,
			CustomText = ResourceHelper.GetEnumText(ControllerType.None),
			IsSelected = () => clientPort.Port == 0xFF,
			OnClick = () => NetplayApi.NetPlaySelectController(new NetplayControllerInfo() { Port = 0xFF })
		});

		_selectControllerAction.SubActions = controllerActions;

		return true;
	}
}
