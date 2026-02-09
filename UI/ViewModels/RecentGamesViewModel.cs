using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Localization;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the recent games and save state selection screens.
/// Handles displaying recent games, loading save states, and save state management.
/// </summary>
public class RecentGamesViewModel : ViewModelBase {
	/// <summary>Gets or sets whether the recent games screen is visible.</summary>
	[Reactive] public bool Visible { get; set; }

	/// <summary>Gets or sets whether emulation should resume when the screen is closed.</summary>
	[Reactive] public bool NeedResume { get; private set; }

	/// <summary>Gets or sets the screen title text.</summary>
	[Reactive] public string Title { get; private set; } = "";

	/// <summary>Gets or sets the current display mode.</summary>
	[Reactive] public GameScreenMode Mode { get; private set; }

	/// <summary>Gets or sets the list of game entries to display.</summary>
	[Reactive] public List<RecentGameInfo> GameEntries { get; private set; } = new List<RecentGameInfo>();

	/// <summary>
	/// Whether the current mode supports deleting entries (e.g., SaveStatePicker)
	/// </summary>
	public bool CanDeleteEntries => Mode == GameScreenMode.SaveStatePicker;

	/// <summary>
	/// Initializes a new instance of the <see cref="RecentGamesViewModel"/> class.
	/// </summary>
	public RecentGamesViewModel() {
		Visible = ConfigManager.Config.Preferences.GameSelectionScreenMode != GameSelectionMode.Disabled;
	}

	/// <summary>
	/// Initializes the ViewModel for a specific display mode.
	/// </summary>
	/// <param name="mode">The screen mode to display.</param>
	public void Init(GameScreenMode mode) {
		if (mode == GameScreenMode.RecentGames && ConfigManager.Config.Preferences.GameSelectionScreenMode == GameSelectionMode.Disabled) {
			Visible = false;
			GameEntries = new List<RecentGameInfo>();
			return;
		} else if (mode != GameScreenMode.RecentGames && Mode == mode && Visible) {
			Visible = false;
			if (NeedResume) {
				EmuApi.Resume();
			}

			return;
		}

		if (Mode == mode && Visible && GameEntries.Count > 0) {
			//Prevent flickering when closing the config window while no game is running
			//No need to update anything if the game selection screen is already visible
			return;
		}

		Mode = mode;

		List<RecentGameInfo> entries = new();

		if (mode == GameScreenMode.RecentGames) {
			NeedResume = false;
			Title = string.Empty;

			List<string> files = Directory.GetFiles(ConfigManager.RecentGamesFolder, "*.rgd").OrderByDescending((file) => new FileInfo(file).LastWriteTime).ToList();
			for (int i = 0; i < files.Count && entries.Count < 72; i++) {
				entries.Add(new RecentGameInfo() { FileName = files[i], Name = Path.GetFileNameWithoutExtension(files[i]) });
			}
		} else if (mode == GameScreenMode.SaveStatePicker || mode == GameScreenMode.RecentPlayPicker || mode == GameScreenMode.AutoSavePicker) {
			// Timestamped save state picker modes (with optional filtering)
			Log.Info($"[RecentGames] {mode} mode - pausing emulation");
			if (!Visible) {
				NeedResume = Pause();
			}

			// Set title based on mode
			Title = mode switch {
				GameScreenMode.RecentPlayPicker => ResourceHelper.GetMessage("RecentPlaySaves"),
				GameScreenMode.AutoSavePicker => ResourceHelper.GetMessage("AutoSaves"),
				_ => ResourceHelper.GetMessage("LoadStateDialog")
			};

			// Get all timestamped saves for current ROM from API
			Log.Info("[RecentGames] Calling EmuApi.GetSaveStateList()");
			SaveStateInfo[] states = EmuApi.GetSaveStateList();
			Log.Info($"[RecentGames] Got {states.Length} save states from API");

			// Filter by origin if needed
			var filteredStates = mode switch {
				GameScreenMode.RecentPlayPicker => states.Where(s => s.Origin == SaveStateOrigin.Recent),
				GameScreenMode.AutoSavePicker => states.Where(s => s.Origin == SaveStateOrigin.Auto),
				_ => states.AsEnumerable()
			};

			foreach (var state in filteredStates) {
				Log.Info($"[RecentGames]   - {state.RomName} @ {state.Filepath} (Origin: {state.Origin})");
				entries.Add(new RecentGameInfo() {
					FileName = state.Filepath,
					Name = state.RomName,
					SaveStateTimestamp = state.Timestamp,
					FriendlyTimestamp = state.GetFriendlyTimestamp(),
					IsTimestampedSave = true,
					Origin = state.Origin
				});
			}

			// If no timestamped saves exist, show message
			if (entries.Count == 0) {
				// Just close the picker - nothing to show
				Log.Info("[RecentGames] No save states found - closing picker");
				Visible = false;
				if (NeedResume) {
					EmuApi.Resume();
				}

				return;
			}
		} else {
			// Legacy slot-based save/load state modes
			if (!Visible) {
				NeedResume = Pause();
			}

			Title = mode == GameScreenMode.LoadState ? ResourceHelper.GetMessage("LoadStateDialog") : ResourceHelper.GetMessage("SaveStateDialog");

			string romName = EmuApi.GetRomInfo().GetRomName();
			for (int i = 0; i < (mode == GameScreenMode.LoadState ? 11 : 10); i++) {
				// Try Nexen native format first, then legacy Mesen format
				string pathNexen = Path.Combine(ConfigManager.SaveStateFolder, romName + "_" + (i + 1) + "." + FileDialogHelper.NexenSaveStateExt);
				string pathMesen = Path.Combine(ConfigManager.SaveStateFolder, romName + "_" + (i + 1) + "." + FileDialogHelper.MesenSaveStateExt);
				string filePath = File.Exists(pathNexen) ? pathNexen : (File.Exists(pathMesen) ? pathMesen : pathNexen);
				entries.Add(new RecentGameInfo() {
					FileName = filePath,
					StateIndex = i + 1,
					Name = i == 10 ? ResourceHelper.GetMessage("AutoSave") : ResourceHelper.GetMessage("SlotNumber", i + 1),
					SaveMode = mode == GameScreenMode.SaveState
				});
			}

			if (mode == GameScreenMode.LoadState) {
				entries.Add(new RecentGameInfo() {
					FileName = Path.Combine(ConfigManager.RecentGamesFolder, romName + ".rgd"),
					Name = ResourceHelper.GetMessage("LastSession")
				});
			}
		}

		Visible = entries.Count > 0;
		GameEntries = entries;
	}

	/// <summary>
	/// Delete a save state entry and refresh the list.
	/// Only works in SaveStatePicker mode.
	/// </summary>
	/// <param name="entry">The save state entry to delete.</param>
	/// <returns>True if the entry was deleted successfully.</returns>
	public bool DeleteEntry(RecentGameInfo entry) {
		if (Mode != GameScreenMode.SaveStatePicker || !entry.IsTimestampedSave) {
			return false;
		}

		if (EmuApi.DeleteSaveState(entry.FileName)) {
			// Refresh the list
			var newEntries = GameEntries.Where(e => e.FileName != entry.FileName).ToList();
			GameEntries = newEntries;

			if (GameEntries.Count == 0) {
				Visible = false;
				if (NeedResume) {
					EmuApi.Resume();
				}
			}

			return true;
		}

		return false;
	}

	/// <summary>
	/// Pauses emulation if currently running.
	/// </summary>
	/// <returns>True if emulation was paused; false if already paused.</returns>
	private bool Pause() {
		if (!EmuApi.IsPaused()) {
			EmuApi.Pause();
			return true;
		}

		return false;
	}
}

/// <summary>
/// Specifies the display mode for the game selection screen.
/// </summary>
public enum GameScreenMode {
	/// <summary>Shows recently played games.</summary>
	RecentGames,
	/// <summary>Shows save state slots for loading.</summary>
	LoadState,
	/// <summary>Shows save state slots for saving.</summary>
	SaveState,
	/// <summary>Shows all timestamped saves for browsing.</summary>
	SaveStatePicker,
	/// <summary>Shows only Recent Play saves (5-min auto-saves, max 12).</summary>
	RecentPlayPicker,
	/// <summary>Shows only Auto Saves (20-min interval saves).</summary>
	AutoSavePicker
}

/// <summary>
/// Represents a single entry in the recent games or save state list.
/// </summary>
public class RecentGameInfo {
	/// <summary>Gets or sets the file path to the game or save state.</summary>
	public string FileName { get; set; } = "";

	/// <summary>Gets or sets the save state slot index (1-based), or -1 if not a slot-based save.</summary>
	public int StateIndex { get; set; } = -1;

	/// <summary>Gets or sets the display name for the entry.</summary>
	public string Name { get; set; } = "";

	/// <summary>Gets or sets whether this entry is for saving (true) or loading (false).</summary>
	public bool SaveMode { get; set; } = false;

	/// <summary>
	/// True if this is a timestamped save state (not a slot-based save)
	/// </summary>
	public bool IsTimestampedSave { get; set; } = false;

	/// <summary>
	/// Timestamp for timestamped saves
	/// </summary>
	public DateTime SaveStateTimestamp { get; set; }

	/// <summary>
	/// Friendly formatted timestamp (e.g., "Today 2:30 PM")
	/// </summary>
	public string FriendlyTimestamp { get; set; } = "";

	/// <summary>
	/// Origin category for save states (Auto/Save/Recent/Lua)
	/// </summary>
	public SaveStateOrigin Origin { get; set; } = SaveStateOrigin.Save;

	/// <summary>
	/// Checks if the entry is enabled (file exists for loading, or save mode).
	/// </summary>
	/// <returns>True if the entry can be selected.</returns>
	public bool IsEnabled() {
		return SaveMode || File.Exists(FileName);
	}

	/// <summary>
	/// Loads or saves the game/state based on the entry type.
	/// </summary>
	public void Load() {
		if (IsTimestampedSave) {
			// Load timestamped save by filepath
			Task.Run(() => {
				EmuApi.LoadStateFile(FileName);
				EmuApi.Resume();
			});
		} else if (StateIndex > 0) {
			Task.Run(() => {
				//Run in another thread to prevent deadlocks etc. when emulator notifications are processed UI-side
				if (SaveMode) {
					EmuApi.SaveState((uint)StateIndex);
				} else {
					EmuApi.LoadState((uint)StateIndex);
				}

				EmuApi.Resume();
			});
		} else {
			LoadRomHelper.LoadRecentGame(FileName, false);
			EmuApi.Resume();
		}
	}
}
