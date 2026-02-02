using System;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the configuration window.
/// Lazily loads child ViewModels for each configuration tab and manages save/revert operations.
/// </summary>
public class ConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the audio configuration ViewModel.</summary>
	[Reactive] public AudioConfigViewModel? Audio { get; set; }

	/// <summary>Gets or sets the input configuration ViewModel.</summary>
	[Reactive] public InputConfigViewModel? Input { get; set; }

	/// <summary>Gets or sets the video configuration ViewModel.</summary>
	[Reactive] public VideoConfigViewModel? Video { get; set; }

	/// <summary>Gets or sets the preferences configuration ViewModel.</summary>
	[Reactive] public PreferencesConfigViewModel? Preferences { get; set; }

	/// <summary>Gets or sets the emulation configuration ViewModel.</summary>
	[Reactive] public EmulationConfigViewModel? Emulation { get; set; }

	/// <summary>Gets or sets the SNES configuration ViewModel.</summary>
	[Reactive] public SnesConfigViewModel? Snes { get; set; }

	/// <summary>Gets or sets the NES configuration ViewModel.</summary>
	[Reactive] public NesConfigViewModel? Nes { get; set; }

	/// <summary>Gets or sets the Game Boy configuration ViewModel.</summary>
	[Reactive] public GameboyConfigViewModel? Gameboy { get; set; }

	/// <summary>Gets or sets the GBA configuration ViewModel.</summary>
	[Reactive] public GbaConfigViewModel? Gba { get; set; }

	/// <summary>Gets or sets the PC Engine configuration ViewModel.</summary>
	[Reactive] public PceConfigViewModel? PcEngine { get; set; }

	/// <summary>Gets or sets the Sega Master System configuration ViewModel.</summary>
	[Reactive] public SmsConfigViewModel? Sms { get; set; }

	/// <summary>Gets or sets the WonderSwan configuration ViewModel.</summary>
	[Reactive] public WsConfigViewModel? Ws { get; set; }

	/// <summary>Gets or sets the other consoles configuration ViewModel.</summary>
	[Reactive] public OtherConsolesConfigViewModel? OtherConsoles { get; set; }

	/// <summary>Gets or sets the currently selected tab index.</summary>
	[Reactive] public ConfigWindowTab SelectedIndex { get; set; }

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
		SelectedIndex = selectedTab;

		AddDisposable(this.WhenAnyValue(x => x.SelectedIndex).Subscribe((tab) => this.SelectTab(tab)));
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
				//TODOv2 fix this patch
				Preferences ??= AddDisposable(new PreferencesConfigViewModel());
				Nes ??= AddDisposable(new NesConfigViewModel(Preferences.Config));
				break;

			case ConfigWindowTab.Snes: Snes ??= AddDisposable(new SnesConfigViewModel()); break;
			case ConfigWindowTab.Gameboy: Gameboy ??= AddDisposable(new GameboyConfigViewModel()); break;
			case ConfigWindowTab.Gba: Gba ??= AddDisposable(new GbaConfigViewModel()); break;
			case ConfigWindowTab.PcEngine: PcEngine ??= AddDisposable(new PceConfigViewModel()); break;
			case ConfigWindowTab.Sms: Sms ??= AddDisposable(new SmsConfigViewModel()); break;
			case ConfigWindowTab.Ws: Ws ??= AddDisposable(new WsConfigViewModel()); break;
			case ConfigWindowTab.OtherConsoles: OtherConsoles ??= AddDisposable(new OtherConsolesConfigViewModel()); break;

			case ConfigWindowTab.Preferences: Preferences ??= AddDisposable(new PreferencesConfigViewModel()); break;
		}

		SelectedIndex = tab;
	}

	/// <summary>
	/// Saves all configuration changes.
	/// </summary>
	public void SaveConfig() {
		ConfigManager.Config.ApplyConfig();
		ConfigManager.Config.Save();
		ConfigManager.Config.Preferences.UpdateFileAssociations();
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
		ConfigManager.Config.Cv = OtherConsoles?.CvOriginalConfig ?? ConfigManager.Config.Cv;
		ConfigManager.Config.ApplyConfig();
		ConfigManager.Config.Save();
	}

	/// <summary>
	/// Checks if any configuration has been modified.
	/// </summary>
	/// <returns>True if any configuration differs from original values.</returns>
	public bool IsDirty() {
		return
			Audio?.OriginalConfig.IsIdentical(ConfigManager.Config.Audio) == false ||
			Input?.OriginalConfig.IsIdentical(ConfigManager.Config.Input) == false ||
			Video?.OriginalConfig.IsIdentical(ConfigManager.Config.Video) == false ||
			Preferences?.OriginalConfig.IsIdentical(ConfigManager.Config.Preferences) == false ||
			Emulation?.OriginalConfig.IsIdentical(ConfigManager.Config.Emulation) == false ||
			Nes?.OriginalConfig.IsIdentical(ConfigManager.Config.Nes) == false ||
			Snes?.OriginalConfig.IsIdentical(ConfigManager.Config.Snes) == false ||
			Gameboy?.OriginalConfig.IsIdentical(ConfigManager.Config.Gameboy) == false ||
			Gba?.OriginalConfig.IsIdentical(ConfigManager.Config.Gba) == false ||
			PcEngine?.OriginalConfig.IsIdentical(ConfigManager.Config.PcEngine) == false ||
			Sms?.OriginalConfig.IsIdentical(ConfigManager.Config.Sms) == false ||
			Ws?.OriginalConfig.IsIdentical(ConfigManager.Config.Ws) == false ||
			OtherConsoles?.CvOriginalConfig.IsIdentical(ConfigManager.Config.Cv) == false
		;
	}
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
	/// <summary>Other consoles settings tab.</summary>
	OtherConsoles = 12,
	//separator
	/// <summary>General preferences tab.</summary>
	Preferences = 14
}
