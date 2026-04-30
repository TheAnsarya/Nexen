using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Input.Platform;
using Avalonia.Interactivity;
using Avalonia.Layout;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Avalonia.VisualTree;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Debugger.Labels;
using Nexen.Debugger.Utilities;
using Nexen.Debugger.Windows;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using Nexen.Views;

namespace Nexen.Windows;
public partial class MainWindow : NexenWindow {
	private DispatcherTimer _timerBackgroundFlag = new DispatcherTimer();
	private MainWindowViewModel _model = null!;

	private NotificationListener? _listener = null;
	private ShortcutHandler _shortcutHandler;

	private MouseManager? _mouseManager = null;
	private ContentControl _audioPlayer;
	private MainMenuView _mainMenu;
	private CommandLineHelper? _cmdLine;

	private bool _testModeEnabled;
	private bool _needResume = false;
	private bool _needCloseValidation = true;
	private bool _isClosing = false;

	private bool _preventFullscreenToggle = false;

	private Panel _rendererPanel;
	private NativeRenderer _renderer;
	private SoftwareRendererView _softwareRenderer;
	private Size _rendererSize;
	private bool _usesSoftwareRenderer;

	private FrameInfo _prevScreenSize;

	private Size _originalSize;
	private PixelPoint _originalPos;
	private WindowState _prevWindowState;

	//Used to suppress key-repeat keyup events on Linux
	private Dictionary<Key, IDisposable> _pendingKeyUpEvents = new();
	private bool _isLinux = false;

	private Stopwatch _stopWatch = Stopwatch.StartNew();
	private Dictionary<Key, long> _keyPressedStamp = new();
	private bool _focusInMenu;
	private static long _notificationTraceSequence;
	private long _ppuFrameDoneCount;

	static MainWindow() {
		WindowStateProperty.Changed.AddClassHandler<MainWindow>((x, e) => x.OnWindowStateChanged());
		IsActiveProperty.Changed.AddClassHandler<MainWindow>((x, e) => x.OnActiveChanged());
	}

	public MainWindow() {
		_testModeEnabled = System.Diagnostics.Debugger.IsAttached;
		_isLinux = OperatingSystem.IsLinux();
		_usesSoftwareRenderer = ConfigManager.Config.Video.UseSoftwareRenderer || OperatingSystem.IsMacOS();

		_model = new MainWindowViewModel();
		DataContext = _model;
		InitGlobalShortcuts();

		try {
			EmuApi.InitDll();
		} catch (Exception ex) {
			Utilities.Log.Fatal(ex, "[MainWindow] EmuApi.InitDll() failed — NexenCore.dll may be missing, corrupted, or incompatible");
			throw;
		}

		Directory.CreateDirectory(ConfigManager.HomeFolder);
		Directory.SetCurrentDirectory(ConfigManager.HomeFolder);

		InitializeComponent();

		_shortcutHandler = new ShortcutHandler(this);

		AddHandler(DragDrop.DropEvent, OnDrop);

		//Allows us to catch LeftAlt/RightAlt key presses
		AddHandler(InputElement.KeyDownEvent, OnPreviewKeyDown, RoutingStrategies.Tunnel, true);
		AddHandler(InputElement.KeyUpEvent, OnPreviewKeyUp, RoutingStrategies.Tunnel, true);

		_rendererPanel = this.GetControl<Panel>("RendererPanel");
		_rendererPanel.LayoutUpdated += RendererPanel_LayoutUpdated;

		_renderer = this.GetControl<NativeRenderer>("Renderer");
		_softwareRenderer = this.GetControl<SoftwareRendererView>("SoftwareRenderer");
		_audioPlayer = this.GetControl<ContentControl>("AudioPlayer");
		_mainMenu = this.GetControl<MainMenuView>("MainMenu");
		ConfigManager.Config.MainWindow.LoadWindowSettings(this);

		Console.CancelKeyPress += Console_CancelKeyPress;

#if DEBUG
		this.AttachDeveloperTools();
#endif
	}

	private static void InitGlobalShortcuts() {
		if (Application.Current?.PlatformSettings == null) {
			return;
		}

		PlatformHotkeyConfiguration hotkeyConfig = Application.Current.PlatformSettings.HotkeyConfiguration;
		List<KeyGesture> gestures = hotkeyConfig.OpenContextMenu;
		for (int i = gestures.Count - 1; i >= 0; i--) {
			if (gestures[i].Key == Key.F10 && gestures[i].KeyModifiers == KeyModifiers.Shift) {
				//Disable Shift-F10 shortcut to open context menu - interferes with default shortcut for step back
				gestures.RemoveAt(i);
			}
		}

		hotkeyConfig.Copy.Add(new KeyGesture(Key.Insert, KeyModifiers.Control));
		hotkeyConfig.Paste.Add(new KeyGesture(Key.Insert, KeyModifiers.Shift));
		hotkeyConfig.Cut.Add(new KeyGesture(Key.Delete, KeyModifiers.Shift));
	}

	protected override void OnClosing(WindowClosingEventArgs e) {
		base.OnClosing(e);
		if (_needCloseValidation) {
			e.Cancel = true;
			ValidateExit();
		} else {
			if (!CloseEmu(false)) {
				e.Cancel = true;
			}
		}
	}

	private bool CloseEmu(bool force) {
		//Close all other windows first
		DebugWindowManager.CloseAllWindows();
		foreach (Window wnd in ApplicationHelper.GetOpenedWindows()) {
			if (wnd != this) {
				wnd.Close();
			}
		}

		if (!force && ApplicationHelper.GetOpenedWindows().Count > 1) {
			return false;
		}

		_timerBackgroundFlag.Stop();
		EmuApi.Stop();
		_listener?.Dispose();
		EmuApi.Release();
		ConfigManager.Config.MainWindow.SaveWindowSettings(this);
		ConfigManager.Config.Save();
		_isClosing = true;

		return true;
	}

	private async void ValidateExit() {
		if (!ConfigManager.Config.Preferences.ConfirmExitResetPower || await NexenMsgBox.Show(null, "ConfirmExit", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes) {
			_needCloseValidation = false;
			Close();
		}
	}

	protected override void OnClosed(EventArgs e) {
		base.OnClosed(e);
		_mouseManager?.Dispose();
	}

	private void Console_CancelKeyPress(object? sender, ConsoleCancelEventArgs e) {
		_needCloseValidation = false;
		Dispatcher.UIThread.Post(() => CloseEmu(true));
	}

	private void OnDrop(object? sender, DragEventArgs e) {
		string? filename = e.DataTransfer.TryGetFiles()?.FirstOrDefault()?.Path.LocalPath;
		if (filename != null) {
			if (File.Exists(filename)) {
				LoadRomHelper.LoadFile(filename);
				Activate();
			} else {
				DisplayMessageHelper.DisplayMessage("Error", ResourceHelper.GetMessage("FileNotFound", filename));
			}
		}
	}

	protected override void OnOpened(EventArgs e) {
		base.OnOpened(e);

		if (Design.IsDesignMode) {
			return;
		}

		_mouseManager = new MouseManager(this, _usesSoftwareRenderer ? _softwareRenderer : _renderer, _mainMenu, _usesSoftwareRenderer);

		ConfigManager.Config.InitializeFontDefaults();
		ConfigManager.Config.Preferences.ApplyFontOptions();
		ConfigManager.Config.Debug.Fonts.ApplyConfig();

		_timerBackgroundFlag.Interval = TimeSpan.FromMilliseconds(100);
		_timerBackgroundFlag.Tick += timerUpdateBackgroundFlag;
		_timerBackgroundFlag.Start();

		//Give focus to panel to avoid menu being given focus by default
		this.GetControl<Panel>("RendererPanel").Focus();

		//Focus on the recent games dialog if it's visible
		//This also enables keyboard/gamepad navigation on the selection screen without having to click it first
		this.FindDescendantOfType<StateGrid>()?.Focus();

		Task.Run(() => {
			try {
				Log.Info("[MainWindow] Startup background init begin");
				CommandLineHelper cmdLine = new CommandLineHelper(Program.CommandLineArgs, true);
				Log.Info($"[MainWindow] Parsed command line: files={cmdLine.FilesToLoad.Count}, lua={cmdLine.LuaScriptsToLoad.Count}, noVideo={cmdLine.NoVideo}, noAudio={cmdLine.NoAudio}, noInput={cmdLine.NoInput}");
				_cmdLine = cmdLine;

				Log.Info("[MainWindow] Calling EmuApi.InitializeEmu");
				EmuApi.InitializeEmu(
					ConfigManager.HomeFolder,
					TryGetPlatformHandle()?.Handle ?? IntPtr.Zero,
					_renderer.Handle,
					_usesSoftwareRenderer,
					cmdLine.NoAudio,
					cmdLine.NoVideo,
					cmdLine.NoInput
				);
				Log.Info("[MainWindow] EmuApi.InitializeEmu completed");

				ConfigManager.Config.RemoveObsoleteConfig();

				//InitializeDefaults must be after InitializeEmu, otherwise keybindings will be empty
				ConfigManager.Config.InitializeDefaults();
				ConfigManager.Config.UpgradeConfig();

				_listener = new NotificationListener();
				_listener.OnNotification += OnNotification;
				Log.Info("[MainWindow] Notification listener initialized");

				// Apply config and add known folders (can be done on background thread)
				// Wrap in try-catch so a missing native export (e.g. stale NexenCore.dll)
				// doesn't prevent menu initialization from running.
				try {
					ConfigManager.Config.ApplyConfig();
					Log.Info("[MainWindow] ApplyConfig completed");
				} catch (Exception ex) {
					Utilities.Log.Error(ex, "[MainWindow] ApplyConfig failed during startup (stale NexenCore.dll?) — continuing initialization");
				}

				if (ConfigManager.Config.Preferences.OverrideGameFolder && Directory.Exists(ConfigManager.Config.Preferences.GameFolder)) {
					EmuApi.AddKnownGameFolder(ConfigManager.Config.Preferences.GameFolder);
				}

				foreach (RecentItem recentItem in ConfigManager.Config.RecentFiles.Items) {
					EmuApi.AddKnownGameFolder(recentItem.RomFile.Folder);
				}

				ConfigManager.Config.Preferences.UpdateFileAssociations();
				SingleInstance.Instance.ArgumentsReceived += Instance_ArgumentsReceived;
				Log.Info("[MainWindow] Posting UI startup continuation");

				// Menu initialization creates UI controls (Image), must be on UI thread
				Dispatcher.UIThread.Post(() => {
					try {
						Log.Info("[MainWindow] UI startup continuation begin");
						_model.Init(this);
						Log.Info("[MainWindow] Main model initialized");

						cmdLine.LoadFiles();
						Log.Info("[MainWindow] Command-line files loaded");
						cmdLine.OnAfterInit(this);
						Log.Info("[MainWindow] Post-load command switches processed");

						if (ConfigManager.Config.Preferences.AutomaticallyCheckForUpdates) {
							_model.MainMenu.CheckForUpdate(this, true);
						}
						Log.Info("[MainWindow] UI startup continuation completed");
					} catch (Exception ex) {
						Log.Error(ex, "[MainWindow] UI startup continuation failed");
					}
				});
			} catch (Exception ex) {
				Log.Error(ex, "[MainWindow] Startup background init failed");
			}
		});
	}

	private void Instance_ArgumentsReceived(object? sender, ArgumentsReceivedEventArgs e) {
		Dispatcher.UIThread.Post(() => {
			CommandLineHelper cmdLine = new(e.Args, false);

			//Set _cmdLine to allow Lua scripts to be loaded once/if a game is loaded
			_cmdLine = cmdLine;

			ConfigManager.Config.ApplyConfig();
			cmdLine.LoadFiles();
		});
	}

	private void OnNotification(NotificationEventArgs e) {
		bool traceLifecycleNotification = e.NotificationType is ConsoleNotificationType.BeforeGameLoad
			or ConsoleNotificationType.GameLoaded
			or ConsoleNotificationType.GameLoadFailed
			or ConsoleNotificationType.BeforeEmulationStop
			or ConsoleNotificationType.EmulationStopped;

		long traceId = 0;
		long traceStart = 0;
		if (traceLifecycleNotification) {
			traceId = Interlocked.Increment(ref _notificationTraceSequence);
			traceStart = Stopwatch.GetTimestamp();
			Log.Info($"[MainWindow] Notification start #{traceId}: {e.NotificationType} (thread: {Environment.CurrentManagedThreadId}, param: 0x{e.Parameter.ToInt64():x})");
		}

		DebugWindowManager.ProcessNotification(e);

		try {
			switch (e.NotificationType) {
			case ConsoleNotificationType.PpuFrameDone:
				long frameCount = Interlocked.Increment(ref _ppuFrameDoneCount);
				bool traceFrame = (frameCount <= 60) || (frameCount >= 2050 && frameCount <= 2450) || ((frameCount % 120) == 0);
				if (traceFrame) {
					Log.Info($"[MainWindow] PpuFrameDone count={frameCount}");

					RomInfo activeRom = EmuApi.GetRomInfo();
					if (activeRom.ConsoleType == ConsoleType.Genesis && (frameCount <= 240 || (frameCount >= 2050 && frameCount <= 2450) || (frameCount % 600) == 0)) {
						GenesisState genesisState = DebugApi.GetConsoleState<GenesisState>(ConsoleType.Genesis);
						byte reg01 = genesisState.Vdp.Registers is { Length: > 1 } ? genesisState.Vdp.Registers[1] : (byte)0;
						byte reg00 = genesisState.Vdp.Registers is { Length: > 0 } ? genesisState.Vdp.Registers[0] : (byte)0;
						byte reg0f = genesisState.Vdp.Registers is { Length: > 15 } ? genesisState.Vdp.Registers[15] : (byte)0;
						byte reg13 = genesisState.Vdp.Registers is { Length: > 19 } ? genesisState.Vdp.Registers[19] : (byte)0;
						byte reg14 = genesisState.Vdp.Registers is { Length: > 20 } ? genesisState.Vdp.Registers[20] : (byte)0;
						byte reg15 = genesisState.Vdp.Registers is { Length: > 21 } ? genesisState.Vdp.Registers[21] : (byte)0;
						byte reg16 = genesisState.Vdp.Registers is { Length: > 22 } ? genesisState.Vdp.Registers[22] : (byte)0;
						byte reg17 = genesisState.Vdp.Registers is { Length: > 23 } ? genesisState.Vdp.Registers[23] : (byte)0;
						bool displayEnabled = (reg01 & 0x40) != 0;
						uint pc = genesisState.Cpu.PC & 0xFFFFFF;
						byte opHi = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, pc);
						byte opLo = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, (pc + 1) & 0xFFFFFF);
						byte ext0 = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, (pc + 2) & 0xFFFFFF);
						byte ext1 = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, (pc + 3) & 0xFFFFFF);
						byte ext2 = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, (pc + 4) & 0xFFFFFF);
						byte ext3 = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, (pc + 5) & 0xFFFFFF);
						uint opExtLong = ((uint)ext0 << 24) | ((uint)ext1 << 16) | ((uint)ext2 << 8) | ext3;
						uint opExtAddr24 = opExtLong & 0x00FFFFFF;
						byte opExtAddrValue = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, opExtAddr24);
						byte ff001e = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, 0xFF001E);
						byte ff001f = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, 0xFF001F);
						ushort sr = genesisState.Cpu.SR;
						ushort vdpStatus = genesisState.Vdp.StatusRegister;
						byte vdpStatusLo = (byte)(vdpStatus & 0xFF);
						bool vblankBit3 = (vdpStatusLo & 0x08) != 0;
						byte statusByteC00005 = DebugApi.GetMemoryValue(MemoryType.GenesisMemory, 0xC00005);
						int intMask = (sr >> 8) & 0x07;
						bool vIntEnabled = (reg01 & 0x20) != 0;
						bool hIntEnabled = (reg00 & 0x10) != 0;
						bool vIntPending = (genesisState.Vdp.StatusRegister & 0x0080) != 0;
						ushort cram1 = genesisState.Vdp.Cram is { Length: > 1 } ? genesisState.Vdp.Cram[1] : (ushort)0;
						ushort cram2 = genesisState.Vdp.Cram is { Length: > 2 } ? genesisState.Vdp.Cram[2] : (ushort)0;
						Log.Info($"[MainWindow] GenesisDiag frame={frameCount} pc=${pc:x6} op=${opHi:x2}{opLo:x2} ext=${ext0:x2}{ext1:x2}{ext2:x2}{ext3:x2} extAddr=${opExtAddr24:x6} extVal=${opExtAddrValue:x2} sr=${sr:x4} imask={intMask} r0=${reg00:x2} r1=${reg01:x2} hIntEn={hIntEnabled} vIntEn={vIntEnabled} vIntPending={vIntPending} vblankBit3={vblankBit3} statusLo=${vdpStatusLo:x2} c00005=${statusByteC00005:x2} displayEnabled={displayEnabled} ff001e=${ff001e:x2} ff001f=${ff001f:x2} vdpAddr=${genesisState.Vdp.AddressRegister:x4} vdpCode=${genesisState.Vdp.CodeRegister:x2} dmaActive={genesisState.Vdp.DmaActive} dmaMode={genesisState.Vdp.DmaMode} dmaLen=${reg14:x2}{reg13:x2} dmaSrc=${reg17:x2}{reg16:x2}{reg15:x2} autoInc=${reg0f:x2} cram1=${cram1:x4} cram2=${cram2:x4} tmssEnabled={genesisState.Io.TmssEnabled} tmssUnlocked={genesisState.Io.TmssUnlocked} vdpFrame={genesisState.Vdp.FrameCount}");
					}
				}
				break;

			case ConsoleNotificationType.GameLoaded:
				CheatCodes.ApplyCheats();
				RomInfo romInfo = EmuApi.GetRomInfo();

				// Set per-ROM save state directory for C++ core
				string saveStatesPath = GameDataManager.GetSaveStatesFolder(romInfo);
				EmuApi.SetPerRomSaveStateDirectory(saveStatesPath);

				// Set per-ROM battery save directory for C++ core
				string savesPath = GameDataManager.GetSavesFolder(romInfo);
				EmuApi.SetPerRomSaveDirectory(savesPath);

				int pendingMigrationCount = GameDataManager.GetPendingMigrationCount(romInfo);
				if (pendingMigrationCount > 0) {
					Dispatcher.UIThread.Post(async () => {
						DialogResult result = await NexenMsgBox.Show(
							this,
							"ConfirmGameDataMigration",
							MessageBoxButtons.YesNo,
							MessageBoxIcon.Question,
							pendingMigrationCount.ToString()
						);

						if (result != DialogResult.Yes)
							return;

						// Copy-only migration keeps legacy files in place as fallback.
						int migratedStates = GameDataManager.MigrateSaveStates(romInfo);
						int migratedSaves = GameDataManager.MigrateBatterySaves(romInfo);

						int totalMigrated = migratedStates + migratedSaves;
						if (totalMigrated > 0) {
							DisplayMessageHelper.DisplayMessage("Migration", $"Migrated {totalMigrated} file(s) to per-game folder");
						}
					});
				}

				// Update global emulator state for menu enable/disable
				Dispatcher.UIThread.Post(() => Services.EmulatorState.Instance.OnRomChanged(romInfo));

				Dispatcher.UIThread.Post(() => {
					bool wasAudioFile = _model.AudioPlayer != null;
					bool updateConfig = _model.RomInfo.Format != romInfo.Format;
					_model.RomInfo = romInfo;

					if (updateConfig) {
						//Make sure any config overrides (video filter/aspect ratio) are applied when loading a different file
						ConfigManager.Config.Video.ApplyConfig();
					}

					bool isAudioFile = _model.AudioPlayer != null;
					if (wasAudioFile != isAudioFile) {
						//Force window size update when switching between an audio file and a regular rom
						Dispatcher.UIThread.Post(() => ProcessResolutionChange());
					}
				});

				GameConfig.LoadGameConfig(romInfo).ApplyConfig();

				GameLoadedEventParams evtParams = Marshal.PtrToStructure<GameLoadedEventParams>(e.Parameter);
				if (!evtParams.IsPowerCycle) {
					Dispatcher.UIThread.Post(() => {
						_model.RecentGames.Visible = false;
						if (IsKeyboardFocusWithin || IsActive || ApplicationHelper.GetActiveOrMainWindow() == this) {
							this.GetControl<Panel>("RendererPanel").Focus();
						}

						DispatcherTimer.RunOnce(() => {
							if (_cmdLine != null) {
								_cmdLine?.ProcessPostLoadCommandSwitches(this);
								_cmdLine = null;
							}

							if (WindowState is WindowState.FullScreen or WindowState.Maximized) {
								//Force resize of renderer when loading a game while in fullscreen
								//Prevents some issues when fullscreen was turned on before loading a game, etc.
								_rendererSize = new Size();
								ResizeRenderer();
							}
						}, TimeSpan.FromMilliseconds(50));
					});
				}

				Dispatcher.UIThread.Post(() => ApplicationHelper.GetExistingWindow<HdPackBuilderWindow>()?.Close());

				// Start background CDL recording if enabled
				Dispatcher.UIThread.Post(() => BackgroundPansyExporter.OnRomLoaded(romInfo));

				string? autoOpenDebugger = Environment.GetEnvironmentVariable("NEXEN_AUTO_OPEN_DEBUGGER");
				if (string.Equals(autoOpenDebugger, "1", StringComparison.OrdinalIgnoreCase)) {
					Dispatcher.UIThread.Post(() => {
						try {
							CpuType cpuType = romInfo.ConsoleType.GetMainCpuType();
							Log.Info($"[MainWindow] NEXEN_AUTO_OPEN_DEBUGGER=1, opening debugger for cpu={cpuType}");
							DebuggerWindow.GetOrOpenWindow(cpuType);
						} catch (Exception ex) {
							Log.Error(ex, "[MainWindow] Auto-open debugger failed");
						}
					});
				}

				// Reset recent play timer so the first auto-save happens 5 minutes after load,
				// not immediately (constructor initializes _lastRecentPlayTime to epoch 0)
				EmuApi.ResetRecentPlayTimer();

				LoadRomHelper.ResetReloadCounter();
				break;

			case ConsoleNotificationType.GameLoadFailed:
				LoadRomHelper.ResetReloadCounter();
				break;

			case ConsoleNotificationType.DebuggerResumed:
			case ConsoleNotificationType.GameResumed:
				Dispatcher.UIThread.Post(() => {
					Services.EmulatorState.Instance.OnPauseChanged();
					_model.RecentGames.Visible = false;
					if (IsKeyboardFocusWithin) {
						this.GetControl<Panel>("RendererPanel").Focus();
					}
				});
				break;

			case ConsoleNotificationType.RequestConfigChange:
				Dispatcher.UIThread.Post(() => UpdateInputConfiguration());
				break;

			case ConsoleNotificationType.BeforeEmulationStop:
				// Save final Pansy file while console is still alive
				BackgroundPansyExporter.OnRomUnloaded();
				break;

			case ConsoleNotificationType.EmulationStopped:
				// Update global emulator state (console is destroyed at this point)
				Dispatcher.UIThread.Post(() => Services.EmulatorState.Instance.OnRomChanged(new RomInfo()));

				Dispatcher.UIThread.Post(() => {
					_model.RomInfo = new RomInfo();
					_model.RecentGames.Init(GameScreenMode.RecentGames);
				});
				break;

			case ConsoleNotificationType.ResolutionChanged:
				Dispatcher.UIThread.Post(() => ProcessResolutionChange());
				break;

			case ConsoleNotificationType.ExecuteShortcut: {
				ExecuteShortcutParams p = Marshal.PtrToStructure<ExecuteShortcutParams>(e.Parameter);
				Dispatcher.UIThread.Post(() => {
					_shortcutHandler.ExecuteShortcut(p.Shortcut);
				});
				break;
			}

			case ConsoleNotificationType.MissingFirmware: {
					MissingFirmwareMessage msg = Marshal.PtrToStructure<MissingFirmwareMessage>(e.Parameter);
					TaskCompletionSource tcs = new TaskCompletionSource();
					Dispatcher.UIThread.Post(async () => {
						await FirmwareHelper.RequestFirmwareFile(msg);
						tcs.SetResult();
					});
					tcs.Task.Wait();
					break;
				}

			case ConsoleNotificationType.SufamiTurboFilePrompt: {
					SufamiTurboFilePromptMessage msg = Marshal.PtrToStructure<SufamiTurboFilePromptMessage>(e.Parameter);
					TaskCompletionSource tcs = new TaskCompletionSource();
					Dispatcher.UIThread.Post(async () => {
						if (await NexenMsgBox.Show(this, "PromptLoadSufamiTurbo", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes) {
							string? selectedFile = await FileDialogHelper.OpenFile(null, this, FileDialogHelper.SufamiTurboExt);
							if (selectedFile != null) {
								byte[] file = Encoding.UTF8.GetBytes(selectedFile);
								Array.Copy(file, msg.Filename, file.Length);
								Marshal.StructureToPtr<SufamiTurboFilePromptMessage>(msg, e.Parameter, false);
							}
						}

						tcs.SetResult();
					});
					tcs.Task.Wait();
					break;
				}

			case ConsoleNotificationType.BeforeGameLoad:
				Dispatcher.UIThread.Post(() => ApplicationHelper.GetExistingWindow<HdPackBuilderWindow>()?.Close());
				break;

			case ConsoleNotificationType.RefreshSoftwareRenderer:
				SoftwareRendererFrame frame = Marshal.PtrToStructure<SoftwareRendererFrame>(e.Parameter);
				_softwareRenderer.UpdateSoftwareRenderer(frame);
				break;
			}
		} finally {
			if (traceLifecycleNotification) {
				double elapsedMs = (Stopwatch.GetTimestamp() - traceStart) * 1000.0 / Stopwatch.Frequency;
				Log.Info($"[MainWindow] Notification end #{traceId}: {e.NotificationType} (elapsed: {elapsedMs:0.000}ms)");
			}
		}
	}

	private static void UpdateInputConfiguration() {
		//Used to update input devices when the core requests changes (NES-only for now)
		ConfigManager.Config.Nes.UpdateInputFromCoreConfig();
	}

	private void InitializeComponent() {
		AvaloniaXamlLoader.Load(this);
	}

	private void ProcessResolutionChange() {
		double dpiScale = LayoutHelper.GetLayoutScale(this);
		FrameInfo baseScreenSize = EmuApi.GetBaseScreenSize();
		if (WindowState == WindowState.Normal) {
			double menuHeight = ConfigManager.Config.Preferences.AutoHideMenu ? 0 : _mainMenu.Bounds.Height;
			double height = ClientSize.Height - menuHeight - _audioPlayer.Bounds.Height;
			if (baseScreenSize.Width == _prevScreenSize.Height && baseScreenSize.Height == _prevScreenSize.Width) {
				//Rotation, swap sizes without changing scale
				double xScale = ClientSize.Width * dpiScale / _prevScreenSize.Width;
				double yScale = height * dpiScale / _prevScreenSize.Height;
				SetScale(Math.Min(Math.Round(xScale), Math.Round(yScale)));
			} else {
				double xScale = ClientSize.Width * dpiScale / baseScreenSize.Width;
				double yScale = height * dpiScale / baseScreenSize.Height;
				SetScale(Math.Min(Math.Round(xScale), Math.Round(yScale)));
			}
		} else if (WindowState is WindowState.Maximized or WindowState.FullScreen) {
			if (_rendererSize == default) {
				ResizeRenderer();
			} else {
				double xScale = _rendererSize.Width * dpiScale / baseScreenSize.Width;
				double yScale = _rendererSize.Height * dpiScale / baseScreenSize.Height;
				SetScale(Math.Min(Math.Round(xScale, 2), Math.Round(yScale, 2)));
			}
		}

		_prevScreenSize = baseScreenSize;
	}

	public void SetScale(double scale) {
		if (scale < 1) {
			scale = 1;
		}

		//TODO(#1281) - Calling this twice seems to fix what might be an issue in Avalonia?
		//On the first call, when DPI > 100%, sometimes _rendererPanel's bounds are incorrect
		InternalSetScale(scale);
		InternalSetScale(scale);
	}

	private void InternalSetScale(double scale) {
		double dpiScale = LayoutHelper.GetLayoutScale(this);
		double aspectRatio = EmuApi.GetAspectRatio();

		FrameInfo screenSize = EmuApi.GetBaseScreenSize();
		if (WindowState == WindowState.Normal) {
			_rendererSize = new Size();

			//When menu is set to auto-hide, don't count its height when calculating the window's final size
			double menuHeight = ConfigManager.Config.Preferences.AutoHideMenu ? 0 : _mainMenu.Bounds.Height;

			double width = Math.Max(MinWidth, Math.Round(screenSize.Height * aspectRatio * scale) / dpiScale);
			double height = Math.Max(MinHeight, screenSize.Height * scale / dpiScale);
			Width = width;
			Height = height + menuHeight + _audioPlayer.Bounds.Height;
			ResizeRenderer();
		} else if (WindowState is WindowState.Maximized or WindowState.FullScreen) {
			_rendererSize = new Size(Math.Round(screenSize.Width * scale * aspectRatio) / dpiScale, Math.Round(screenSize.Height * scale) / dpiScale);
			ResizeRenderer();
		}
	}

	private void ResizeRenderer() {
		_rendererPanel.InvalidateMeasure();
		_rendererPanel.InvalidateArrange();
	}

	private void RendererPanel_LayoutUpdated(object? sender, EventArgs e) {
		double aspectRatio = EmuApi.GetAspectRatio();
		double dpiScale = LayoutHelper.GetLayoutScale(this);

		Size finalSize = _rendererSize == default ? _rendererPanel.Bounds.Size : _rendererSize;
		double height = finalSize.Height;
		double width = finalSize.Height * aspectRatio;
		if (Math.Round(width) > Math.Round(finalSize.Width)) {
			//Use renderer width to calculate the height instead of the opposite
			//when current window dimensions would cause cropping horizontally
			//if the screen width was calculated based on the height.
			width = finalSize.Width;
			height = width / aspectRatio;
		}

		if (ConfigManager.Config.Video.FullscreenForceIntegerScale && VisualRoot is Window wnd && (wnd.WindowState == WindowState.FullScreen || wnd.WindowState == WindowState.Maximized)) {
			FrameInfo baseSize = EmuApi.GetBaseScreenSize();
			double scale = height * dpiScale / baseSize.Height;
			if (scale != Math.Floor(scale)) {
				height = baseSize.Height * Math.Max(1, Math.Floor(scale / dpiScale));
				width = height * aspectRatio;
			}
		}

		uint realWidth = (uint)Math.Round(width * dpiScale);
		uint realHeight = (uint)Math.Round(height * dpiScale);
		EmuApi.SetRendererSize(realWidth, realHeight);
		_model.RendererSize = new Size(realWidth, realHeight);

		_renderer.Width = width;
		_renderer.Height = height;
		_model.SoftwareRenderer.Width = width;
		_model.SoftwareRenderer.Height = height;
	}

	private void OnWindowStateChanged() {
		_rendererSize = new Size();
		ResizeRenderer();
	}

	public void ToggleFullscreen() {
		if (_preventFullscreenToggle) {
			return;
		}

		_preventFullscreenToggle = true;
		if (WindowState == WindowState.FullScreen) {
			Task.Run(() => {
				if (ConfigManager.Config.Video.UseExclusiveFullscreen) {
					EmuApi.SetExclusiveFullscreenMode(false, _renderer.Handle);
				}

				Dispatcher.UIThread.Post(() => {
					WindowState = _prevWindowState;
					if (_prevWindowState == WindowState.Normal) {
						Width = _originalSize.Width;
						Height = _originalSize.Height;
						Position = _originalPos;
					}

					_preventFullscreenToggle = false;
				});
			});
		} else {
			_originalSize = ClientSize;
			_originalPos = Position;
			_prevWindowState = WindowState;

			if (ConfigManager.Config.Video.UseExclusiveFullscreen) {
				if (!EmuApi.IsRunning()) {
					//Prevent entering fullscreen mode until a game is loaded
					_preventFullscreenToggle = false;
					return;
				}

				Task.Run(() => {
					EmuApi.SetExclusiveFullscreenMode(true, TryGetPlatformHandle()?.Handle ?? IntPtr.Zero);
					_preventFullscreenToggle = false;

					Dispatcher.UIThread.Post(() => WindowState = WindowState.FullScreen);
				});
			} else {
				WindowState = WindowState.FullScreen;
				_preventFullscreenToggle = false;
			}
		}
	}

	protected override void OnLostFocus(FocusChangedEventArgs e) {
		base.OnLostFocus(e);
		if (WindowState == WindowState.FullScreen && ConfigManager.Config.Video.UseExclusiveFullscreen) {
			ToggleFullscreen();
		}
	}

	private bool ProcessTestModeShortcuts(Key key) {
		if (key == Key.F1) {
			if (TestApi.RomTestRecording()) {
				TestApi.RomTestStop();
			} else {
				RomTestHelper.RecordTest();
			}

			return true;
		} else if (key == Key.F2) {
			_ = RomTestHelper.RunTest();
			return true;
		} else if (key == Key.F3) {
			RomTestHelper.RunAllTests();
			return true;
		} else if (key == Key.F7) {
			RomTestHelper.RunGbMicroTests();
			return true;
		} else if (key == Key.F8) {
			RomTestHelper.RunGambatteTests();
			return true;
		} else if (key == Key.F6) {
			//For testing purposes (to test for memory leaks)
			Task.Run(() => {
				for (int i = 0; i < 50; i++) {
					GC.Collect();
					GC.WaitForPendingFinalizers();
					Thread.Sleep(10);
				}
			});
			return true;
		}

		return false;
	}

	private void OnPreviewKeyDown(object? sender, KeyEventArgs e) {
		if (_testModeEnabled && e.KeyModifiers == KeyModifiers.Alt && ProcessTestModeShortcuts(e.Key)) {
			return;
		}

		if (OperatingSystem.IsMacOS()) {
			//Keyhandler handles key internally on macOS
			return;
		}

		if (_focusInMenu) {
			return;
		}

		if (e.Key != Key.None) {
			_keyPressedStamp[e.Key] = _stopWatch.ElapsedTicks;

			if (_isLinux && _pendingKeyUpEvents.TryGetValue(e.Key, out IDisposable? cancelTimer)) {
				//Cancel any pending key up event
				cancelTimer.Dispose();
				_pendingKeyUpEvents.Remove(e.Key);
			}

			InputApi.SetKeyState((UInt16)e.Key, true);
		}

		if (e.Key is Key.Tab or Key.F10) {
			//Prevent menu/window from handling these keys to avoid issue with custom shortcuts
			e.Handled = true;
		}
	}

	private void OnPreviewKeyUp(object? sender, KeyEventArgs e) {
		if (OperatingSystem.IsMacOS()) {
			//Keyhandler handles key internally on macOS
			return;
		}

		if (e.Key != Key.None) {
			if (e.Key.IsSpecialKey() && (!_keyPressedStamp.TryGetValue(e.Key, out long stamp) || ((_stopWatch.ElapsedTicks - stamp) * 1000 / Stopwatch.Frequency) < 10)) {
				//Key up received without key down, or key pressed for less than 10 ms, pretend the key was pressed for 50ms
				//Some special keys can behave this way (e.g printscreen)
				InputApi.SetKeyState((UInt16)e.Key, true);
				DispatcherTimer.RunOnce(() => InputApi.SetKeyState((UInt16)e.Key, false), TimeSpan.FromMilliseconds(50), DispatcherPriority.MaxValue);
				_keyPressedStamp.Remove(e.Key);
				return;
			}

			_keyPressedStamp.Remove(e.Key);

			if (_isLinux) {
				//Process keyup events after 1ms on Linux to prevent key repeat from triggering key up/down repeatedly
				IDisposable cancelTimer = DispatcherTimer.RunOnce(() => InputApi.SetKeyState((UInt16)e.Key, false), TimeSpan.FromMilliseconds(1), DispatcherPriority.MaxValue);
				_pendingKeyUpEvents[e.Key] = cancelTimer;
			} else {
				InputApi.SetKeyState((UInt16)e.Key, false);
			}
		}
	}

	private void OnActiveChanged() {
		if (!_isClosing) {
			ConfigApi.SetEmulationFlag(EmulationFlags.InBackground, !IsActive);
			InputApi.ResetKeyState();
		}
	}

	private void timerUpdateBackgroundFlag(object? sender, EventArgs e) {
		Window? activeWindow = ApplicationHelper.GetActiveWindow();

		PreferencesConfig cfg = ConfigManager.Config.Preferences;
		bool isGameRunning = EmuApi.IsRunning();

		bool focusInMenu = MenuHelper.IsFocusInMenu(_mainMenu.MainMenu);
		if (focusInMenu && !_focusInMenu) {
			InputApi.ResetKeyState();
		}

		_focusInMenu = focusInMenu;

		bool needPause = activeWindow == null && cfg.PauseWhenInBackground;
		if (activeWindow != null) {
			bool isConfigWindow = (activeWindow != this) && !DebugWindowManager.IsDebugWindow(activeWindow);
			needPause |= ShouldPauseInMenusAndConfig(cfg.PauseWhenInMenusAndConfig, isGameRunning, LoadRomHelper.IsRomLoadInProgress, isConfigWindow, _mainMenu.MainMenu.IsOpen);
		}

		if (needPause) {
			if (!EmuApi.IsPaused()) {
				_needResume = true;

				DebuggerWindow? wnd = DebugWindowManager.GetDebugWindow<DebuggerWindow>(x => x.CpuType == _model.RomInfo.ConsoleType.GetMainCpuType());
				if (wnd != null) {
					//If the debugger window for the main cpu is opened, suppress the "bring to front on break" behavior
					wnd.SuppressBringToFront();
				}

				EmuApi.Pause();
			}
		} else if (_needResume) {
			//Don't resume if the load/save state dialog is opened
			if (!_model.RecentGames.Visible) {
				EmuApi.Resume();
				_needResume = false;
			}
		}

		// Check if we should save a Recent Play state (every 5 minutes)
		if (EmuApi.IsRunning() && EmuApi.ShouldSaveRecentPlay()) {
			EmuApi.SaveRecentPlayState();
		}
	}

	internal static bool ShouldPauseInMenusAndConfig(bool pauseWhenInMenusAndConfig, bool isGameRunning, bool isRomLoadInProgress, bool isConfigWindow, bool isMainMenuOpen) {
		if (!pauseWhenInMenusAndConfig || !isGameRunning || isRomLoadInProgress) {
			return false;
		}

		// Pause only while an active game is running and either a config dialog is opened
		// or the main menu is currently opened in the main window.
		return isConfigWindow || isMainMenuOpen;
	}
}
