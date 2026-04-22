using System;
using System.Collections.Generic;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the configuration window.
/// Lazily loads child ViewModels for each configuration tab and manages save/revert operations.
/// </summary>
public sealed partial class ConfigViewModel : DisposableViewModel {
	private static readonly Dictionary<string, ConfigWindowTab> RouteToTab = new(StringComparer.OrdinalIgnoreCase) {
		[ConfigRouteIds.Audio] = ConfigWindowTab.Audio,
		[ConfigRouteIds.Emulation] = ConfigWindowTab.Emulation,
		[ConfigRouteIds.Input] = ConfigWindowTab.Input,
		[ConfigRouteIds.Video] = ConfigWindowTab.Video,
		[ConfigRouteIds.Nes] = ConfigWindowTab.Nes,
		[ConfigRouteIds.Snes] = ConfigWindowTab.Snes,
		[ConfigRouteIds.Gameboy] = ConfigWindowTab.Gameboy,
		[ConfigRouteIds.Gba] = ConfigWindowTab.Gba,
		[ConfigRouteIds.PcEngine] = ConfigWindowTab.PcEngine,
		[ConfigRouteIds.Sms] = ConfigWindowTab.Sms,
		[ConfigRouteIds.Ws] = ConfigWindowTab.Ws,
		[ConfigRouteIds.Lynx] = ConfigWindowTab.Lynx,
		[ConfigRouteIds.Atari2600] = ConfigWindowTab.Atari2600,
		[ConfigRouteIds.ChannelF] = ConfigWindowTab.ChannelF,
		[ConfigRouteIds.OtherConsoles] = ConfigWindowTab.OtherConsoles,
		[ConfigRouteIds.Preferences] = ConfigWindowTab.Preferences,
	};

	private static readonly Dictionary<ConfigWindowTab, string> TabToRoute = new() {
		[ConfigWindowTab.Audio] = ConfigRouteIds.Audio,
		[ConfigWindowTab.Emulation] = ConfigRouteIds.Emulation,
		[ConfigWindowTab.Input] = ConfigRouteIds.Input,
		[ConfigWindowTab.Video] = ConfigRouteIds.Video,
		[ConfigWindowTab.Nes] = ConfigRouteIds.Nes,
		[ConfigWindowTab.Snes] = ConfigRouteIds.Snes,
		[ConfigWindowTab.Gameboy] = ConfigRouteIds.Gameboy,
		[ConfigWindowTab.Gba] = ConfigRouteIds.Gba,
		[ConfigWindowTab.PcEngine] = ConfigRouteIds.PcEngine,
		[ConfigWindowTab.Sms] = ConfigRouteIds.Sms,
		[ConfigWindowTab.Ws] = ConfigRouteIds.Ws,
		[ConfigWindowTab.Lynx] = ConfigRouteIds.Lynx,
		[ConfigWindowTab.Atari2600] = ConfigRouteIds.Atari2600,
		[ConfigWindowTab.ChannelF] = ConfigRouteIds.ChannelF,
		[ConfigWindowTab.OtherConsoles] = ConfigRouteIds.OtherConsoles,
		[ConfigWindowTab.Preferences] = ConfigRouteIds.Preferences,
	};

	private Configuration _originalConfig = new();

	/// <summary>Gets or sets the audio configuration ViewModel.</summary>
	[Reactive] public partial AudioConfigViewModel? Audio { get; set; }

	/// <summary>Gets or sets the input configuration ViewModel.</summary>
	[Reactive] public partial InputConfigViewModel? Input { get; set; }

	/// <summary>Gets or sets the video configuration ViewModel.</summary>
	[Reactive] public partial VideoConfigViewModel? Video { get; set; }

	/// <summary>Gets or sets the preferences configuration ViewModel.</summary>
	[Reactive] public partial PreferencesConfigViewModel? Preferences { get; set; }

	/// <summary>Gets or sets the emulation configuration ViewModel.</summary>
	[Reactive] public partial EmulationConfigViewModel? Emulation { get; set; }

	/// <summary>Gets or sets the SNES configuration ViewModel.</summary>
	[Reactive] public partial SnesConfigViewModel? Snes { get; set; }

	/// <summary>Gets or sets the NES configuration ViewModel.</summary>
	[Reactive] public partial NesConfigViewModel? Nes { get; set; }

	/// <summary>Gets or sets the Game Boy configuration ViewModel.</summary>
	[Reactive] public partial GameboyConfigViewModel? Gameboy { get; set; }

	/// <summary>Gets or sets the GBA configuration ViewModel.</summary>
	[Reactive] public partial GbaConfigViewModel? Gba { get; set; }

	/// <summary>Gets or sets the PC Engine configuration ViewModel.</summary>
	[Reactive] public partial PceConfigViewModel? PcEngine { get; set; }

	/// <summary>Gets or sets the Sega Master System configuration ViewModel.</summary>
	[Reactive] public partial SmsConfigViewModel? Sms { get; set; }

	/// <summary>Gets or sets the WonderSwan configuration ViewModel.</summary>
	[Reactive] public partial WsConfigViewModel? Ws { get; set; }

	/// <summary>Gets or sets the Lynx configuration ViewModel.</summary>
	[Reactive] public partial LynxConfigViewModel? Lynx { get; set; }

	/// <summary>Gets or sets the Atari 2600 configuration ViewModel.</summary>
	[Reactive] public partial Atari2600ConfigViewModel? Atari2600 { get; set; }

	/// <summary>Gets or sets the Channel F configuration ViewModel.</summary>
	[Reactive] public partial ChannelFConfigViewModel? ChannelF { get; set; }

	/// <summary>Gets or sets the other consoles configuration ViewModel.</summary>
	[Reactive] public partial OtherConsolesConfigViewModel? OtherConsoles { get; set; }

	/// <summary>Gets or sets the currently selected tab index.</summary>
	[Reactive] public partial ConfigWindowTab SelectedIndex { get; set; }

	/// <summary>Gets whether the window should always be on top.</summary>
	public bool AlwaysOnTop { get; }

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public ConfigViewModel() : this(ConfigWindowTab.Audio) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="ConfigViewModel"/> class.
	/// </summary>
	/// <param name="selectedTab">The initial tab to display.</param>
	public ConfigViewModel(ConfigWindowTab selectedTab) {
		AlwaysOnTop = ConfigManager.Config.Preferences.AlwaysOnTop;
		AddDisposable(this.WhenAnyValue(x => x.SelectedIndex).Subscribe((tab) => this.SelectTab(tab)));
		if (selectedTab == ConfigWindowTab.Input) {
			var romInfo = EmuApi.GetRomInfo();
			bool hasLoadedRom = !string.IsNullOrWhiteSpace(romInfo.RomPath);
			ConfigWindowTab landingTab = ResolveInputLandingTab(hasLoadedRom, romInfo.ConsoleType);
			SelectTab(landingTab);
			ApplyInputSubtabSelection(landingTab);
		} else {
			SelectTab(selectedTab);
		}
		CaptureCurrentAsOriginal();
	}

	/// <summary>
	/// Resolves the initial tab to open when the user requests the generic Input settings entry point.
	/// If a ROM is loaded, this prefers the active system's config tab; otherwise it falls back to the generic Input tab.
	/// </summary>
	public static ConfigWindowTab ResolveInputLandingTab(bool hasLoadedRom, ConsoleType consoleType) {
		if (!hasLoadedRom) {
			return ConfigWindowTab.Input;
		}

		return consoleType switch {
			ConsoleType.Nes => ConfigWindowTab.Nes,
			ConsoleType.Snes => ConfigWindowTab.Snes,
			ConsoleType.Gameboy => ConfigWindowTab.Gameboy,
			ConsoleType.Gba => ConfigWindowTab.Gba,
			ConsoleType.PcEngine => ConfigWindowTab.PcEngine,
			ConsoleType.Sms => ConfigWindowTab.Sms,
			ConsoleType.Ws => ConfigWindowTab.Ws,
			ConsoleType.Lynx => ConfigWindowTab.Lynx,
			ConsoleType.Atari2600 => ConfigWindowTab.Atari2600,
			ConsoleType.ChannelF => ConfigWindowTab.ChannelF,
			_ => ConfigWindowTab.Input,
		};
	}

	public static bool TryResolveRoute(string routeId, out ConfigWindowTab tab) {
		return RouteToTab.TryGetValue(routeId, out tab);
	}

	public static string GetRouteId(ConfigWindowTab tab) {
		return TabToRoute.TryGetValue(tab, out string? routeId) ? routeId : ConfigRouteIds.Audio;
	}

	public static ConfigWindowTab ResolveRoute(string routeId, bool hasLoadedRom, ConsoleType consoleType) {
		if (string.Equals(routeId, ConfigRouteIds.InputActiveSystem, StringComparison.OrdinalIgnoreCase)) {
			return ResolveInputLandingTab(hasLoadedRom, consoleType);
		}

		return TryResolveRoute(routeId, out ConfigWindowTab tab) ? tab : ConfigWindowTab.Audio;
	}

	private void CaptureCurrentAsOriginal() {
		_originalConfig = JsonHelper.Clone<Configuration>(ConfigManager.Config);
	}

	/// <summary>
	/// Selects a configuration tab and lazily creates its ViewModel.
	/// </summary>
	/// <param name="tab">The tab to select.</param>
	public void SelectTab(ConfigWindowTab tab) {
		//Create each view model when the corresponding tab is clicked, for performance
		switch (tab) {
			case ConfigWindowTab.Audio: Audio ??= AddDisposable(new AudioConfigViewModel()); break;
			case ConfigWindowTab.Emulation: Emulation ??= AddDisposable(new EmulationConfigViewModel()); break;
			case ConfigWindowTab.Input: Input ??= AddDisposable(new InputConfigViewModel()); break;
			case ConfigWindowTab.Video: Video ??= AddDisposable(new VideoConfigViewModel()); break;

			case ConfigWindowTab.Nes:
				//TODO(#1281) fix this patch
				Preferences ??= AddDisposable(new PreferencesConfigViewModel());
				Nes ??= AddDisposable(new NesConfigViewModel(Preferences.Config));
				break;

			case ConfigWindowTab.Snes: Snes ??= AddDisposable(new SnesConfigViewModel()); break;
			case ConfigWindowTab.Gameboy: Gameboy ??= AddDisposable(new GameboyConfigViewModel()); break;
			case ConfigWindowTab.Gba: Gba ??= AddDisposable(new GbaConfigViewModel()); break;
			case ConfigWindowTab.PcEngine: PcEngine ??= AddDisposable(new PceConfigViewModel()); break;
			case ConfigWindowTab.Sms: Sms ??= AddDisposable(new SmsConfigViewModel()); break;
			case ConfigWindowTab.Ws: Ws ??= AddDisposable(new WsConfigViewModel()); break;
			case ConfigWindowTab.Lynx: Lynx ??= AddDisposable(new LynxConfigViewModel()); break;
			case ConfigWindowTab.Atari2600: Atari2600 ??= AddDisposable(new Atari2600ConfigViewModel()); break;
			case ConfigWindowTab.ChannelF: ChannelF ??= AddDisposable(new ChannelFConfigViewModel()); break;
			case ConfigWindowTab.OtherConsoles: OtherConsoles ??= AddDisposable(new OtherConsolesConfigViewModel()); break;

			case ConfigWindowTab.Preferences: Preferences ??= AddDisposable(new PreferencesConfigViewModel()); break;
		}

		SelectedIndex = tab;
	}

	private void ApplyInputSubtabSelection(ConfigWindowTab tab) {
		switch (tab) {
			case ConfigWindowTab.Nes:
				if (Nes != null) {
					Nes.SelectedTab = NesConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Snes:
				if (Snes != null) {
					Snes.SelectedTab = SnesConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Gameboy:
				if (Gameboy != null) {
					Gameboy.SelectedTab = GameboyConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Gba:
				if (Gba != null) {
					Gba.SelectedTab = GbaConfigTab.Input;
				}
				break;
			case ConfigWindowTab.PcEngine:
				if (PcEngine != null) {
					PcEngine.SelectedTab = PceConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Sms:
				if (Sms != null) {
					Sms.SelectedTab = SmsConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Ws:
				if (Ws != null) {
					Ws.SelectedTab = WsConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Lynx:
				if (Lynx != null) {
					Lynx.SelectedTab = LynxConfigTab.Input;
				}
				break;
			case ConfigWindowTab.Atari2600:
				if (Atari2600 != null) {
					Atari2600.SelectedTab = Atari2600ConfigTab.Input;
				}
				break;
			case ConfigWindowTab.ChannelF:
				if (ChannelF != null) {
					ChannelF.SelectedTab = ChannelFConfigTab.Input;
				}
				break;
		}
	}

	/// <summary>
	/// Saves all configuration changes.
	/// </summary>
	public void SaveConfig() {
		ConfigManager.Config.ApplyConfig();
		ConfigManager.Config.Save();
		ConfigManager.Config.Preferences.UpdateFileAssociations();
		CaptureCurrentAsOriginal();
	}

	/// <summary>
	/// Reverts all configuration changes to original values.
	/// </summary>
	public void RevertConfig() {
		ConfigManager.Config.Audio = Audio?.OriginalConfig ?? ConfigManager.Config.Audio;
		ConfigManager.Config.Input = Input?.OriginalConfig ?? ConfigManager.Config.Input;
		ConfigManager.Config.Video = Video?.OriginalConfig ?? ConfigManager.Config.Video;
		ConfigManager.Config.Preferences = Preferences?.OriginalConfig ?? ConfigManager.Config.Preferences;
		ConfigManager.Config.Emulation = Emulation?.OriginalConfig ?? ConfigManager.Config.Emulation;
		ConfigManager.Config.Nes = Nes?.OriginalConfig ?? ConfigManager.Config.Nes;
		ConfigManager.Config.Snes = Snes?.OriginalConfig ?? ConfigManager.Config.Snes;
		ConfigManager.Config.Gameboy = Gameboy?.OriginalConfig ?? ConfigManager.Config.Gameboy;
		ConfigManager.Config.Gba = Gba?.OriginalConfig ?? ConfigManager.Config.Gba;
		ConfigManager.Config.PcEngine = PcEngine?.OriginalConfig ?? ConfigManager.Config.PcEngine;
		ConfigManager.Config.Sms = Sms?.OriginalConfig ?? ConfigManager.Config.Sms;
		ConfigManager.Config.Ws = Ws?.OriginalConfig ?? ConfigManager.Config.Ws;
		ConfigManager.Config.Lynx = Lynx?.OriginalConfig ?? ConfigManager.Config.Lynx;
		ConfigManager.Config.Atari2600 = Atari2600?.OriginalConfig ?? ConfigManager.Config.Atari2600;
		ConfigManager.Config.ChannelF = ChannelF?.OriginalConfig ?? ConfigManager.Config.ChannelF;
		ConfigManager.Config.Cv = OtherConsoles?.CvOriginalConfig ?? ConfigManager.Config.Cv;
		ConfigManager.Config.ApplyConfig();
		ConfigManager.Config.Save();
		CaptureCurrentAsOriginal();
	}

	/// <summary>
	/// Checks if any configuration has been modified.
	/// </summary>
	/// <returns>True if any configuration differs from original values.</returns>
	public bool IsDirty() {
		return
			!_originalConfig.Audio.IsIdentical(ConfigManager.Config.Audio) ||
			!_originalConfig.Input.IsIdentical(ConfigManager.Config.Input) ||
			!_originalConfig.Video.IsIdentical(ConfigManager.Config.Video) ||
			!_originalConfig.Preferences.IsIdentical(ConfigManager.Config.Preferences) ||
			!_originalConfig.Emulation.IsIdentical(ConfigManager.Config.Emulation) ||
			!_originalConfig.Nes.IsIdentical(ConfigManager.Config.Nes) ||
			!_originalConfig.Snes.IsIdentical(ConfigManager.Config.Snes) ||
			!_originalConfig.Gameboy.IsIdentical(ConfigManager.Config.Gameboy) ||
			!_originalConfig.Gba.IsIdentical(ConfigManager.Config.Gba) ||
			!_originalConfig.PcEngine.IsIdentical(ConfigManager.Config.PcEngine) ||
			!_originalConfig.Sms.IsIdentical(ConfigManager.Config.Sms) ||
			!_originalConfig.Ws.IsIdentical(ConfigManager.Config.Ws) ||
			!_originalConfig.Lynx.IsIdentical(ConfigManager.Config.Lynx) ||
			!_originalConfig.Atari2600.IsIdentical(ConfigManager.Config.Atari2600) ||
			!_originalConfig.ChannelF.IsIdentical(ConfigManager.Config.ChannelF) ||
			!_originalConfig.Cv.IsIdentical(ConfigManager.Config.Cv)
		;
	}
}

public static class ConfigRouteIds {
	public const string Audio = "settings.audio";
	public const string Emulation = "settings.emulation";
	public const string Input = "settings.input";
	public const string InputActiveSystem = "settings.input.active-system";
	public const string Video = "settings.video";
	public const string Nes = "settings.system.nes";
	public const string Snes = "settings.system.snes";
	public const string Gameboy = "settings.system.gameboy";
	public const string Gba = "settings.system.gba";
	public const string PcEngine = "settings.system.pce";
	public const string Sms = "settings.system.sms";
	public const string Ws = "settings.system.ws";
	public const string Lynx = "settings.system.lynx";
	public const string Atari2600 = "settings.system.atari2600";
	public const string ChannelF = "settings.system.channelf";
	public const string OtherConsoles = "settings.system.other";
	public const string Preferences = "settings.preferences";
}

/// <summary>
/// Specifies the tabs in the configuration window.
/// </summary>
public enum ConfigWindowTab {
	/// <summary>Audio settings tab.</summary>
	Audio = 0,
	/// <summary>Emulation settings tab.</summary>
	Emulation = 1,
	/// <summary>Input/controller settings tab.</summary>
	Input = 2,
	/// <summary>Video/display settings tab.</summary>
	Video = 3,
	//separator
	/// <summary>NES-specific settings tab.</summary>
	Nes = 5,
	/// <summary>SNES-specific settings tab.</summary>
	Snes = 6,
	/// <summary>Game Boy-specific settings tab.</summary>
	Gameboy = 7,
	/// <summary>GBA-specific settings tab.</summary>
	Gba = 8,
	/// <summary>PC Engine-specific settings tab.</summary>
	PcEngine = 9,
	/// <summary>Sega Master System-specific settings tab.</summary>
	Sms = 10,
	/// <summary>WonderSwan-specific settings tab.</summary>
	Ws = 11,
	/// <summary>Lynx-specific settings tab.</summary>
	Lynx = 12,
	/// <summary>Atari 2600-specific settings tab.</summary>
	Atari2600 = 13,
	/// <summary>Channel F-specific settings tab.</summary>
	ChannelF = 14,
	/// <summary>Other consoles settings tab.</summary>
	OtherConsoles = 15,
	//separator
	/// <summary>General preferences tab.</summary>
	Preferences = 17
}
