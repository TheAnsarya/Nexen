using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Media;
using Avalonia.Styling;
using Avalonia.Threading;
using Nexen.Config.Shortcuts;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using Nexen.ViewModels;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;
public class PreferencesConfig : BaseConfig<PreferencesConfig> {
	[Reactive] public NexenTheme Theme { get; set; } = NexenTheme.Light;
	[Reactive] public bool AutomaticallyCheckForUpdates { get; set; } = true;
	[Reactive] public bool SingleInstance { get; set; } = true;
	[Reactive] public bool AutoLoadPatches { get; set; } = true;

	[Reactive] public bool PauseWhenInBackground { get; set; } = false;
	[Reactive] public bool PauseWhenInMenusAndConfig { get; set; } = false;
	[Reactive] public bool AllowBackgroundInput { get; set; } = false;
	[Reactive] public bool PauseOnMovieEnd { get; set; } = true;
	[Reactive] public bool ShowMovieIcons { get; set; } = true;
	[Reactive] public bool ShowTurboRewindIcons { get; set; } = true;
	[Reactive] public bool ConfirmExitResetPower { get; set; } = false;

	[Reactive] public bool AssociateSnesRomFiles { get; set; } = false;
	[Reactive] public bool AssociateSnesMusicFiles { get; set; } = false;
	[Reactive] public bool AssociateNesRomFiles { get; set; } = false;
	[Reactive] public bool AssociateNesMusicFiles { get; set; } = false;
	[Reactive] public bool AssociateGbRomFiles { get; set; } = false;
	[Reactive] public bool AssociateGbMusicFiles { get; set; } = false;
	[Reactive] public bool AssociateGbaRomFiles { get; set; } = false;
	[Reactive] public bool AssociatePceRomFiles { get; set; } = false;
	[Reactive] public bool AssociatePceMusicFiles { get; set; } = false;
	[Reactive] public bool AssociateSmsRomFiles { get; set; } = false;
	[Reactive] public bool AssociateGameGearRomFiles { get; set; } = false;
	[Reactive] public bool AssociateSgRomFiles { get; set; } = false;
	[Reactive] public bool AssociateCvRomFiles { get; set; } = false;
	[Reactive] public bool AssociateWsRomFiles { get; set; } = false;

	[Reactive] public bool EnableAutoSaveState { get; set; } = true;
	[Reactive] public UInt32 AutoSaveStateDelay { get; set; } = 20;

	[Reactive] public bool EnableRewind { get; set; } = true;
	[Reactive] public UInt32 RewindBufferSize { get; set; } = 300;

	[Reactive] public bool AlwaysOnTop { get; set; } = false;

	[Reactive] public bool AutoHideMenu { get; set; } = false;

	[Reactive] public bool ShowFps { get; set; } = false;
	[Reactive] public bool ShowFrameCounter { get; set; } = false;
	[Reactive] public bool ShowGameTimer { get; set; } = false;
	[Reactive] public bool ShowLagCounter { get; set; } = false;
	[Reactive] public bool ShowTitleBarInfo { get; set; } = false;
	[Reactive] public bool ShowDebugInfo { get; set; } = false;
	[Reactive] public bool DisableOsd { get; set; } = false;
	[Reactive] public HudDisplaySize HudSize { get; set; } = HudDisplaySize.Fixed;
	[Reactive] public GameSelectionMode GameSelectionScreenMode { get; set; } = GameSelectionMode.ResumeState;

	[Reactive] public FontAntialiasing FontAntialiasing { get; set; } = FontAntialiasing.SubPixelAntialias;
	[Reactive] public FontConfig NexenFont { get; set; } = new FontConfig() { FontFamily = "Microsoft Sans Serif", FontSize = 11 };
	[Reactive] public FontConfig NexenMenuFont { get; set; } = new FontConfig() { FontFamily = "Segoe UI", FontSize = 12 };

	[Reactive] public List<ShortcutKeyInfo> ShortcutKeys { get; set; } = new List<ShortcutKeyInfo>();

	[Reactive] public bool OverrideGameFolder { get; set; } = false;
	[Reactive] public bool OverrideAviFolder { get; set; } = false;
	[Reactive] public bool OverrideMovieFolder { get; set; } = false;
	[Reactive] public bool OverrideSaveDataFolder { get; set; } = false;
	[Reactive] public bool OverrideSaveStateFolder { get; set; } = false;
	[Reactive] public bool OverrideScreenshotFolder { get; set; } = false;
	[Reactive] public bool OverrideWaveFolder { get; set; } = false;

	[Reactive] public string GameFolder { get; set; } = "";
	[Reactive] public string AviFolder { get; set; } = "";
	[Reactive] public string MovieFolder { get; set; } = "";
	[Reactive] public string SaveDataFolder { get; set; } = "";
	[Reactive] public string SaveStateFolder { get; set; } = "";
	[Reactive] public string ScreenshotFolder { get; set; } = "";
	[Reactive] public string WaveFolder { get; set; } = "";

	public PreferencesConfig() {
	}

	private void AddShortcut(ShortcutKeyInfo shortcut) {
		if (!ShortcutKeys.Exists(a => a.Shortcut == shortcut.Shortcut)) {
			ShortcutKeys.Add(shortcut);
		}
	}

	public void InitializeDefaultShortcuts() {
		UInt16 ctrl = InputApi.GetKeyCode("Left Ctrl");
		UInt16 alt = InputApi.GetKeyCode("Left Alt");
		UInt16 shift = InputApi.GetKeyCode("Left Shift");

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.FastForward, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Tab") }, KeyCombination2 = new KeyCombination() { Key1 = InputApi.GetKeyCode("Pad1 R2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Rewind, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Backspace") }, KeyCombination2 = new KeyCombination() { Key1 = InputApi.GetKeyCode("Pad1 L2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.IncreaseSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("=") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.DecreaseSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("-") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.MaxSpeed, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F9") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.IncreaseVolume, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("=") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.DecreaseVolume, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("-") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ToggleFps, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F10") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ToggleFullscreen, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F11") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.TakeScreenshot, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F12") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Reset, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("R") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.PowerCycle, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("T") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.ReloadRom, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = shift, Key3 = InputApi.GetKeyCode("R") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.Pause, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("Esc") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.RunSingleFrame, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("`") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale1x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale2x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale3x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale4x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("4") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale5x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("5") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale6x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("6") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale7x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("7") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale8x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("8") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale9x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("9") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SetScale10x, KeyCombination = new KeyCombination() { Key1 = alt, Key2 = InputApi.GetKeyCode("0") } });

		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.OpenFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("O") } });

		// Infinite save states - F1 opens picker, Shift+F1/Ctrl+S creates new save
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.OpenSaveStatePicker, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("S") } });

		// Save/Load state to/from file dialog
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveStateToFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = shift, Key3 = InputApi.GetKeyCode("S") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadStateFromFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("L") } });

		foreach (EmulatorShortcut value in Enum.GetValues<EmulatorShortcut>()) {
			if (value < EmulatorShortcut.LastValidValue) {
				AddShortcut(new ShortcutKeyInfo { Shortcut = value });
			}
		}
	}

	public void UpdateFileAssociations() {
		FileAssociationHelper.UpdateFileAssociations();
	}

	public void ApplyFontOptions() {
		UpdateFonts();
	}

	private void UpdateFonts() {
		if (Application.Current != null) {
			string nexenFontFamily = Configuration.GetValidFontFamily(NexenFont.FontFamily, false);
			string menuFont = Configuration.GetValidFontFamily(NexenMenuFont.FontFamily, false);

			if (Application.Current.Resources["NexenFont"] is FontFamily curNexenFont && curNexenFont.Name != nexenFontFamily) {
				Application.Current.Resources["NexenFont"] = new FontFamily(nexenFontFamily);
			}

			if (Application.Current.Resources["NexenMenuFont"] is FontFamily curNexenMenuFont && curNexenMenuFont.Name != menuFont) {
				Application.Current.Resources["NexenMenuFont"] = new FontFamily(menuFont);
			}

			if (Application.Current.Resources["NexenFontSize"] is double curNexenFontSize && curNexenFontSize != NexenFont.FontSize) {
				Application.Current.Resources["NexenFontSize"] = (double)NexenFont.FontSize;
			}

			if (Application.Current.Resources["NexenMenuFontSize"] is double curNexenMenuFontSize && curNexenMenuFontSize != NexenMenuFont.FontSize) {
				Application.Current.Resources["NexenMenuFontSize"] = (double)NexenMenuFont.FontSize;
			}
		}
	}

	public void InitializeFontDefaults() {
		NexenFont = Configuration.GetDefaultFont();
		NexenMenuFont = Configuration.GetDefaultMenuFont();
		ApplyFontOptions();
	}

	public static void UpdateTheme() {
		if (Application.Current != null) {
			ThemeVariant newTheme = ConfigManager.Config.Preferences.Theme == NexenTheme.Dark ? ThemeVariant.Dark : ThemeVariant.Light;
			if (Application.Current.RequestedThemeVariant != newTheme) {
				ConfigManager.ActiveTheme = ConfigManager.Config.Preferences.Theme;
				Application.Current.RequestedThemeVariant = newTheme;
			}
		}
	}

	public void ApplyConfig() {
		UpdateFonts();

		List<InteropShortcutKeyInfo> shortcutKeys = new List<InteropShortcutKeyInfo>();
		foreach (ShortcutKeyInfo shortcutInfo in ShortcutKeys) {
			if (!shortcutInfo.KeyCombination.IsEmpty) {
				shortcutKeys.Add(new InteropShortcutKeyInfo(shortcutInfo.Shortcut, shortcutInfo.KeyCombination.ToInterop()));
			}

			if (!shortcutInfo.KeyCombination2.IsEmpty) {
				shortcutKeys.Add(new InteropShortcutKeyInfo(shortcutInfo.Shortcut, shortcutInfo.KeyCombination2.ToInterop()));
			}
		}

		ConfigApi.SetShortcutKeys(shortcutKeys.ToArray(), (UInt32)shortcutKeys.Count);

		ConfigApi.SetPreferences(new InteropPreferencesConfig() {
			ShowFps = ShowFps,
			ShowFrameCounter = ShowFrameCounter,
			ShowGameTimer = ShowGameTimer,
			ShowDebugInfo = ShowDebugInfo,
			ShowLagCounter = ShowLagCounter,
			DisableOsd = DisableOsd,
			AllowBackgroundInput = AllowBackgroundInput,
			PauseOnMovieEnd = PauseOnMovieEnd,
			ShowMovieIcons = ShowMovieIcons,
			ShowTurboRewindIcons = ShowTurboRewindIcons,
			DisableGameSelectionScreen = GameSelectionScreenMode == GameSelectionMode.Disabled,
			HudSize = HudSize,
			SaveFolderOverride = OverrideSaveDataFolder ? SaveDataFolder : "",
			SaveStateFolderOverride = OverrideSaveStateFolder ? SaveStateFolder : "",
			ScreenshotFolderOverride = OverrideScreenshotFolder ? ScreenshotFolder : "",
			RewindBufferSize = EnableRewind ? RewindBufferSize : 0,
			AutoSaveStateDelay = EnableAutoSaveState ? AutoSaveStateDelay : 0
		});
	}
}

public enum NexenTheme {
	Light = 0,
	Dark = 1
}

public enum FontAntialiasing {
	Disabled,
	Antialias,
	SubPixelAntialias
}

public enum GameSelectionMode {
	Disabled,
	ResumeState,
	PowerOn
}

public enum HudDisplaySize {
	Fixed,
	Scaled,
}

public struct InteropPreferencesConfig {
	[MarshalAs(UnmanagedType.I1)] public bool ShowFps;
	[MarshalAs(UnmanagedType.I1)] public bool ShowFrameCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowGameTimer;
	[MarshalAs(UnmanagedType.I1)] public bool ShowLagCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowDebugInfo;
	[MarshalAs(UnmanagedType.I1)] public bool DisableOsd;
	[MarshalAs(UnmanagedType.I1)] public bool AllowBackgroundInput;
	[MarshalAs(UnmanagedType.I1)] public bool PauseOnMovieEnd;
	[MarshalAs(UnmanagedType.I1)] public bool ShowMovieIcons;
	[MarshalAs(UnmanagedType.I1)] public bool ShowTurboRewindIcons;
	[MarshalAs(UnmanagedType.I1)] public bool DisableGameSelectionScreen;

	public HudDisplaySize HudSize;

	public UInt32 AutoSaveStateDelay;
	public UInt32 RewindBufferSize;

	public string SaveFolderOverride;
	public string SaveStateFolderOverride;
	public string ScreenshotFolderOverride;
}
