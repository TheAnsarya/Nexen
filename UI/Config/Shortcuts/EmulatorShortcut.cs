using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Nexen.Config.Shortcuts;
public enum EmulatorShortcut {
	FastForward,
	Rewind,
	RewindTenSecs,
	RewindOneMin,

	// Legacy slot system - kept for binary compatibility, no longer functional
	[Obsolete("Slot-based saves removed. Use QuickSaveTimestamped/OpenSaveStatePicker instead.")]
	SelectSaveSlot1,
	[Obsolete] SelectSaveSlot2,
	[Obsolete] SelectSaveSlot3,
	[Obsolete] SelectSaveSlot4,
	[Obsolete] SelectSaveSlot5,
	[Obsolete] SelectSaveSlot6,
	[Obsolete] SelectSaveSlot7,
	[Obsolete] SelectSaveSlot8,
	[Obsolete] SelectSaveSlot9,
	[Obsolete] SelectSaveSlot10,
	[Obsolete] MoveToNextStateSlot,
	[Obsolete] MoveToPreviousStateSlot,
	[Obsolete] SaveState,
	[Obsolete] LoadState,

	ToggleCheats,
	ToggleFastForward,
	ToggleRewind,

	RunSingleFrame,

	TakeScreenshot,

	ToggleRecordVideo,
	ToggleRecordAudio,
	ToggleRecordMovie,

	IncreaseSpeed,
	DecreaseSpeed,
	MaxSpeed,

	Pause,
	Reset,
	PowerCycle,
	ReloadRom,
	PowerOff,
	Exit,

	ExecReset,
	ExecPowerCycle,
	ExecReloadRom,
	ExecPowerOff,

	SetScale1x,
	SetScale2x,
	SetScale3x,
	SetScale4x,
	SetScale5x,
	SetScale6x,
	SetScale7x,
	SetScale8x,
	SetScale9x,
	SetScale10x,
	ToggleFullscreen,
	ToggleFps,
	ToggleGameTimer,
	ToggleFrameCounter,
	ToggleLagCounter,
	ToggleOsd,
	ToggleAlwaysOnTop,
	ToggleDebugInfo,

	ToggleAudio,
	IncreaseVolume,
	DecreaseVolume,

	PreviousTrack,
	NextTrack,

	ToggleBgLayer1,
	ToggleBgLayer2,
	ToggleBgLayer3,
	ToggleBgLayer4,
	ToggleSprites1,
	ToggleSprites2,
	EnableAllLayers,

	ResetLagCounter,

	// Legacy slot system - kept for binary compatibility, no longer functional
	[Obsolete("Slot-based saves removed. Use QuickSaveTimestamped/OpenSaveStatePicker instead.")]
	SaveStateSlot1,
	[Obsolete] SaveStateSlot2,
	[Obsolete] SaveStateSlot3,
	[Obsolete] SaveStateSlot4,
	[Obsolete] SaveStateSlot5,
	[Obsolete] SaveStateSlot6,
	[Obsolete] SaveStateSlot7,
	[Obsolete] SaveStateSlot8,
	[Obsolete] SaveStateSlot9,
	[Obsolete] SaveStateSlot10,
	SaveStateToFile,
	SaveStateDialog,

	[Obsolete("Slot-based saves removed. Use QuickSaveTimestamped/OpenSaveStatePicker instead.")]
	LoadStateSlot1,
	[Obsolete] LoadStateSlot2,
	[Obsolete] LoadStateSlot3,
	[Obsolete] LoadStateSlot4,
	[Obsolete] LoadStateSlot5,
	[Obsolete] LoadStateSlot6,
	[Obsolete] LoadStateSlot7,
	[Obsolete] LoadStateSlot8,
	[Obsolete] LoadStateSlot9,
	[Obsolete] LoadStateSlot10,
	[Obsolete] LoadStateSlotAuto,
	LoadStateFromFile,
	LoadStateDialog,
	LoadLastSession,

	/// <summary>
	/// Quick save to a timestamped save state file (infinite saves)
	/// </summary>
	QuickSaveTimestamped,

	/// <summary>
	/// Open the save state picker to browse all timestamped saves
	/// </summary>
	OpenSaveStatePicker,

	/// <summary>
	/// Save to the single designated slot (F4 quick access)
	/// </summary>
	SaveDesignatedSlot,

	/// <summary>
	/// Load from the single designated slot (Shift-F4 quick access)
	/// </summary>
	LoadDesignatedSlot,

	OpenFile,

	InputBarcode,
	LoadTape,
	RecordTape,
	StopRecordTape,

	//NES
	FdsSwitchDiskSide,
	FdsEjectDisk,
	FdsInsertDiskNumber,
	FdsInsertNextDisk,
	VsServiceButton,
	VsServiceButton2,
	VsInsertCoin1,
	VsInsertCoin2,
	VsInsertCoin3,
	VsInsertCoin4,
	StartRecordHdPack,
	StopRecordHdPack,

	LastValidValue,
	[Obsolete] LoadRandomGame,
}

public static class EmulatorShortcutExtensions {
	public static KeyCombination? GetShortcutKeys(this EmulatorShortcut shortcut) {
		PreferencesConfig cfg = ConfigManager.Config.Preferences;
		int keyIndex = cfg.ShortcutKeys.FindIndex((ShortcutKeyInfo shortcutInfo) => shortcutInfo.Shortcut == shortcut);
		if (keyIndex >= 0) {
			if (!cfg.ShortcutKeys[keyIndex].KeyCombination.IsEmpty) {
				return cfg.ShortcutKeys[keyIndex].KeyCombination;
			} else if (!cfg.ShortcutKeys[keyIndex].KeyCombination2.IsEmpty) {
				return cfg.ShortcutKeys[keyIndex].KeyCombination2;
			}
		}

		return null;
	}
}
