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
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;
public sealed partial class PreferencesConfig : BaseConfig<PreferencesConfig> {
	[Reactive] public partial PrimaryUsageProfile PrimaryUsageProfile { get; set; } = PrimaryUsageProfile.Playing;
	[Reactive] public partial NexenTheme Theme { get; set; } = NexenTheme.Light;
	[Reactive] public partial bool AutomaticallyCheckForUpdates { get; set; } = true;
	[Reactive] public partial bool SingleInstance { get; set; } = true;
	[Reactive] public partial bool AutoLoadPatches { get; set; } = true;

	[Reactive] public partial bool PauseWhenInBackground { get; set; } = false;
	[Reactive] public partial bool PauseWhenInMenusAndConfig { get; set; } = false;
	[Reactive] public partial bool AllowBackgroundInput { get; set; } = false;
	[Reactive] public partial bool PauseOnMovieEnd { get; set; } = true;
	[Reactive] public partial bool ShowMovieIcons { get; set; } = true;
	[Reactive] public partial bool ShowTurboRewindIcons { get; set; } = true;
	[Reactive] public partial bool ConfirmExitResetPower { get; set; } = false;

	[Reactive] public partial bool AssociateSnesRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSnesMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateNesRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateNesMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGbaRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociatePceRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociatePceMusicFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSmsRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateGameGearRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateSgRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateCvRomFiles { get; set; } = false;
	[Reactive] public partial bool AssociateWsRomFiles { get; set; } = false;

	[Reactive] public partial bool EnableAutoSaveState { get; set; } = true;
	[Reactive] public partial UInt32 AutoSaveStateDelay { get; set; } = 20;
	[Reactive] public partial bool ShowAutoSaveNotifications { get; set; } = true;
	[Reactive] public partial bool ShowRecentPlayNotifications { get; set; } = false;

	[Reactive] public partial bool EnableRewind { get; set; } = true;
	[Reactive] public partial UInt32 RewindBufferSize { get; set; } = 300;

	[Reactive] public partial bool AlwaysOnTop { get; set; } = false;

	[Reactive] public partial bool AutoHideMenu { get; set; } = false;

	[Reactive] public partial bool ShowFps { get; set; } = false;
	[Reactive] public partial bool ShowFrameCounter { get; set; } = false;
	[Reactive] public partial bool ShowGameTimer { get; set; } = false;
	[Reactive] public partial bool ShowLagCounter { get; set; } = false;
	[Reactive] public partial bool ShowRerecordCounter { get; set; } = false;
	[Reactive] public partial bool ShowLagFrameIndicator { get; set; } = false;
	[Reactive] public partial bool ShowTitleBarInfo { get; set; } = false;
	[Reactive] public partial bool ShowDebugInfo { get; set; } = false;
	[Reactive] public partial bool DisableOsd { get; set; } = false;
	[Reactive] public partial HudDisplaySize HudSize { get; set; } = HudDisplaySize.Fixed;
	[Reactive] public partial GameSelectionMode GameSelectionScreenMode { get; set; } = GameSelectionMode.ResumeState;

	[Reactive] public partial FontAntialiasing FontAntialiasing { get; set; } = FontAntialiasing.SubPixelAntialias;
	[Reactive] public partial FontConfig NexenFont { get; set; } = new FontConfig() { FontFamily = "Microsoft Sans Serif", FontSize = 11 };
	[Reactive] public partial FontConfig NexenMenuFont { get; set; } = new FontConfig() { FontFamily = "Segoe UI", FontSize = 12 };

	[Reactive] public partial List<ShortcutKeyInfo> ShortcutKeys { get; set; } = [];

	[Reactive] public partial bool OverrideGameFolder { get; set; } = false;
	[Reactive] public partial bool OverrideAviFolder { get; set; } = false;
	[Reactive] public partial bool OverrideMovieFolder { get; set; } = false;
	[Reactive] public partial bool OverrideSaveDataFolder { get; set; } = false;
	[Reactive] public partial bool OverrideSaveStateFolder { get; set; } = false;
	[Reactive] public partial bool OverrideScreenshotFolder { get; set; } = false;
	[Reactive] public partial bool OverrideWaveFolder { get; set; } = false;

	[Reactive] public partial string GameFolder { get; set; } = "";
	[Reactive] public partial string AviFolder { get; set; } = "";
	[Reactive] public partial string MovieFolder { get; set; } = "";
	[Reactive] public partial string SaveDataFolder { get; set; } = "";
	[Reactive] public partial string SaveStateFolder { get; set; } = "";
	[Reactive] public partial string ScreenshotFolder { get; set; } = "";
	[Reactive] public partial string WaveFolder { get; set; } = "";

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

		// Infinite save states - F1/Ctrl+S creates new save, Shift+F1 opens picker
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.OpenSaveStatePicker, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F1") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.QuickSaveTimestamped, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("S") } });

		// Save/Load state to/from file dialog
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveStateToFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = shift, Key3 = InputApi.GetKeyCode("S") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadStateFromFile, KeyCombination = new KeyCombination() { Key1 = ctrl, Key2 = InputApi.GetKeyCode("L") } });

		// Designated slots - F2-F4 for 3 quick save/load slots (F = save, Shift+F = load, matching F1 pattern)
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F2") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot2, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot2, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F3") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.SaveDesignatedSlot3, KeyCombination = new KeyCombination() { Key1 = InputApi.GetKeyCode("F4") } });
		AddShortcut(new ShortcutKeyInfo { Shortcut = EmulatorShortcut.LoadDesignatedSlot3, KeyCombination = new KeyCombination() { Key1 = shift, Key2 = InputApi.GetKeyCode("F4") } });

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
		if (Application.Current is not null) {
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
		if (Application.Current is not null) {
			ThemeVariant newTheme = ConfigManager.Config.Preferences.Theme == NexenTheme.Dark ? ThemeVariant.Dark : ThemeVariant.Light;
			if (Application.Current.RequestedThemeVariant != newTheme) {
				ConfigManager.ActiveTheme = ConfigManager.Config.Preferences.Theme;
				Application.Current.RequestedThemeVariant = newTheme;
			}
		}
	}

	public void ApplyConfig() {
		UpdateFonts();

		List<InteropShortcutKeyInfo> shortcutKeys = [];
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
			ShowRerecordCounter = ShowRerecordCounter,
			ShowLagFrameIndicator = ShowLagFrameIndicator,
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
			AutoSaveStateDelay = EnableAutoSaveState ? AutoSaveStateDelay : 0,
			ShowAutoSaveNotifications = ShowAutoSaveNotifications,
			ShowRecentPlayNotifications = ShowRecentPlayNotifications
		});
	}
}

public enum NexenTheme {
	Light = 0,
	Dark = 1
}

public enum PrimaryUsageProfile {
	Playing = 0,
	Debugging = 1
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
	[MarshalAs(UnmanagedType.I1)] public bool ShowRerecordCounter;
	[MarshalAs(UnmanagedType.I1)] public bool ShowLagFrameIndicator;
	[MarshalAs(UnmanagedType.I1)] public bool ShowDebugInfo;
	[MarshalAs(UnmanagedType.I1)] public bool DisableOsd;
	[MarshalAs(UnmanagedType.I1)] public bool AllowBackgroundInput;
	[MarshalAs(UnmanagedType.I1)] public bool PauseOnMovieEnd;
	[MarshalAs(UnmanagedType.I1)] public bool ShowMovieIcons;
	[MarshalAs(UnmanagedType.I1)] public bool ShowTurboRewindIcons;
	[MarshalAs(UnmanagedType.I1)] public bool DisableGameSelectionScreen;

	public HudDisplaySize HudSize;

	public UInt32 AutoSaveStateDelay;
	[MarshalAs(UnmanagedType.I1)] public bool ShowAutoSaveNotifications;
	[MarshalAs(UnmanagedType.I1)] public bool ShowRecentPlayNotifications;
	public UInt32 RewindBufferSize;

	public string SaveFolderOverride;
	public string SaveStateFolderOverride;
	public string ScreenshotFolderOverride;
}
