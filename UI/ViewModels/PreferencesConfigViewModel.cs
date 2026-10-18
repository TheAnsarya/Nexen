using Avalonia;
using Avalonia.Controls;
using Avalonia.Styling;
using Mesen.Config;
using Mesen.Config.Shortcuts;
using Mesen.Utilities;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections.Generic;

namespace Mesen.ViewModels
{
	public class PreferencesConfigViewModel : DisposableViewModel
	{
		[Reactive] public PreferencesConfig Config { get; set; }
		[Reactive] public PreferencesConfig OriginalConfig { get; set; }

		public string DataStorageLocation { get; }
		public bool IsOsx { get; }

		public List<ShortcutKeyInfo> ShortcutKeys { get; set; }

		public PreferencesConfigViewModel()
		{
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

				EmulatorShortcut.QuickSaveTimestamped,
				EmulatorShortcut.OpenSaveStatePicker,
				EmulatorShortcut.SaveState,
				EmulatorShortcut.LoadState,

				EmulatorShortcut.SaveStateToFile,
				EmulatorShortcut.SaveStateDialog,

				EmulatorShortcut.LoadStateFromFile,
				EmulatorShortcut.LoadStateDialog,
				EmulatorShortcut.LoadLastSession,
			};

			Dictionary<EmulatorShortcut, ShortcutKeyInfo> shortcuts = new Dictionary<EmulatorShortcut, ShortcutKeyInfo>();
			foreach(ShortcutKeyInfo shortcut in Config.ShortcutKeys) {
				shortcuts[shortcut.Shortcut] = shortcut;
			}

			ShortcutKeys = new List<ShortcutKeyInfo>();
			for(int i = 0; i < displayOrder.Length; i++) {
				if(shortcuts.ContainsKey(displayOrder[i])) {
					ShortcutKeys.Add(shortcuts[displayOrder[i]]);
				}
			}

			if(Design.IsDesignMode) {
				return;
			}

			AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => { 
				Config.ApplyConfig();
				PreferencesConfig.UpdateTheme();
			}));
		}
   }
}
