using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Styling;
using Nexen.Config;
using Nexen.Config.Shortcuts;
using Nexen.Utilities;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the preferences configuration tab.
/// </summary>
public class PreferencesConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current preferences configuration.</summary>
	[Reactive] public PreferencesConfig Config { get; set; }

	/// <summary>Gets or sets the original preferences configuration for revert.</summary>
	[Reactive] public PreferencesConfig OriginalConfig { get; set; }

	/// <summary>Gets the data storage location path.</summary>
	public string DataStorageLocation { get; }

	/// <summary>Gets whether the current platform is macOS.</summary>
	public bool IsOsx { get; }

	/// <summary>Gets or sets the list of shortcut key bindings.</summary>
	public List<ShortcutKeyInfo> ShortcutKeys { get; set; }

	/// <summary>
	/// Initializes a new instance of the <see cref="PreferencesConfigViewModel"/> class.
	/// </summary>
	public PreferencesConfigViewModel() {
		Config = ConfigManager.Config.Preferences;
		OriginalConfig = Config.Clone();

		IsOsx = OperatingSystem.IsMacOS();
		DataStorageLocation = ConfigManager.HomeFolder;

		EmulatorShortcut[] displayOrder = new EmulatorShortcut[] {
			EmulatorShortcut.FastForward,
			EmulatorShortcut.ToggleFastForward,
			EmulatorShortcut.Rewind,
			EmulatorShortcut.ToggleRewind,
			EmulatorShortcut.RewindTenSecs,
			EmulatorShortcut.RewindOneMin,

			EmulatorShortcut.Pause,
			EmulatorShortcut.Reset,
			EmulatorShortcut.PowerCycle,
			EmulatorShortcut.ReloadRom,
			EmulatorShortcut.PowerOff,
			EmulatorShortcut.Exit,

			EmulatorShortcut.ToggleRecordVideo,
			EmulatorShortcut.ToggleRecordAudio,
			EmulatorShortcut.ToggleRecordMovie,

			EmulatorShortcut.TakeScreenshot,
			EmulatorShortcut.RunSingleFrame,

			EmulatorShortcut.SetScale1x,
			EmulatorShortcut.SetScale2x,
			EmulatorShortcut.SetScale3x,
			EmulatorShortcut.SetScale4x,
			EmulatorShortcut.SetScale5x,
			EmulatorShortcut.SetScale6x,
			EmulatorShortcut.SetScale7x,
			EmulatorShortcut.SetScale8x,
			EmulatorShortcut.SetScale9x,
			EmulatorShortcut.SetScale10x,
			EmulatorShortcut.ToggleFullscreen,

			EmulatorShortcut.ToggleDebugInfo,
			EmulatorShortcut.ToggleFps,
			EmulatorShortcut.ToggleGameTimer,
			EmulatorShortcut.ToggleFrameCounter,
			EmulatorShortcut.ToggleAlwaysOnTop,
			EmulatorShortcut.ToggleCheats,
			EmulatorShortcut.ToggleOsd,

			EmulatorShortcut.ToggleBgLayer1,
			EmulatorShortcut.ToggleBgLayer2,
			EmulatorShortcut.ToggleBgLayer3,
			EmulatorShortcut.ToggleBgLayer4,
			EmulatorShortcut.ToggleSprites1,
			EmulatorShortcut.ToggleSprites2,
			EmulatorShortcut.EnableAllLayers,

			EmulatorShortcut.ToggleLagCounter,
			EmulatorShortcut.ResetLagCounter,

			EmulatorShortcut.ToggleAudio,
			EmulatorShortcut.IncreaseVolume,
			EmulatorShortcut.DecreaseVolume,

			EmulatorShortcut.PreviousTrack,
			EmulatorShortcut.NextTrack,

			EmulatorShortcut.MaxSpeed,
			EmulatorShortcut.IncreaseSpeed,
			EmulatorShortcut.DecreaseSpeed,

			EmulatorShortcut.OpenFile,

			EmulatorShortcut.InputBarcode,
			EmulatorShortcut.LoadTape,
			EmulatorShortcut.RecordTape,
			EmulatorShortcut.StopRecordTape,

			// Infinite save state system (replaces slot-based system)
			EmulatorShortcut.QuickSaveTimestamped,
			EmulatorShortcut.OpenSaveStatePicker,
			EmulatorShortcut.SaveStateToFile,
			EmulatorShortcut.SaveStateDialog,
			EmulatorShortcut.LoadStateFromFile,
			EmulatorShortcut.LoadStateDialog,
			EmulatorShortcut.LoadLastSession
		};

		Dictionary<EmulatorShortcut, ShortcutKeyInfo> shortcuts = new Dictionary<EmulatorShortcut, ShortcutKeyInfo>();
		foreach (ShortcutKeyInfo shortcut in Config.ShortcutKeys) {
			shortcuts[shortcut.Shortcut] = shortcut;
		}

		ShortcutKeys = new List<ShortcutKeyInfo>();
		for (int i = 0; i < displayOrder.Length; i++) {
			if (shortcuts.ContainsKey(displayOrder[i])) {
				ShortcutKeys.Add(shortcuts[displayOrder[i]]);
			}
		}

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => {
			Config.ApplyConfig();
			PreferencesConfig.UpdateTheme();
		}));
	}
}
