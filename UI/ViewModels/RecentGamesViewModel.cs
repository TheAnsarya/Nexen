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
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the recent games and save state selection screens.
/// Handles displaying recent games, loading save states, and save state management.
/// </summary>
public sealed partial class RecentGamesViewModel : ViewModelBase {
	/// <summary>Gets or sets whether the recent games screen is visible.</summary>
	[Reactive] public partial bool Visible { get; set; }

	/// <summary>Gets or sets whether emulation should resume when the screen is closed.</summary>
	[Reactive] public partial bool NeedResume { get; set; }

	/// <summary>Gets or sets the screen title text.</summary>
	[Reactive] public partial string Title { get; private set; } = "";

	/// <summary>Gets or sets the current display mode.</summary>
	[Reactive] public partial GameScreenMode Mode { get; private set; }

	/// <summary>Gets or sets the list of game entries to display.</summary>
	[Reactive] public partial List<RecentGameInfo> GameEntries { get; private set; } = [];

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
			GameEntries = [];
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

			// Load games with .rgd session data
			List<string> files = Directory.GetFiles(ConfigManager.RecentGamesFolder, "*.rgd").OrderByDescending((file) => new FileInfo(file).LastWriteTime).ToList();
			HashSet<string> rgdNames = new(StringComparer.OrdinalIgnoreCase);
			for (int i = 0; i < files.Count && entries.Count < 72; i++) {
				string name = Path.GetFileNameWithoutExtension(files[i]);
				entries.Add(new RecentGameInfo() { FileName = files[i], Name = name });
				rgdNames.Add(name);
			}

			// Also include recent ROMs that don't have .rgd files yet
			foreach (RecentItem item in ConfigManager.Config.RecentFiles.Items) {
				if (entries.Count >= 72) {
					break;
				}

				string romName = Path.GetFileNameWithoutExtension(item.RomFile.ReadablePath);
				if (!rgdNames.Contains(romName) && File.Exists(item.RomFile)) {
					entries.Add(new RecentGameInfo() {
						FileName = item.RomFile,
						Name = romName,
						IsRecentRomFile = true
					});
					rgdNames.Add(romName);
				}
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
					Origin = state.Origin,
					IsPaused = state.IsPaused,
					SlotNumber = state.SlotNumber
				});
			}

			// Mark the most recent save per designated slot as "current"
			var slotGroups = entries
				.Where(e => e.Origin == SaveStateOrigin.Designated && e.SlotNumber > 0)
				.GroupBy(e => e.SlotNumber);
			foreach (var group in slotGroups) {
				var newest = group.OrderByDescending(e => e.SaveStateTimestamp).FirstOrDefault();
				if (newest != null) {
					newest.IsCurrentSlot = true;
				}
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
public sealed partial class RecentGameInfo {
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
	/// Whether the emulator was paused when this state was saved
	/// </summary>
	public bool IsPaused { get; set; } = false;

	/// <summary>
	/// Slot number for Designated saves (1-3), 0 for non-slot saves
	/// </summary>
	public int SlotNumber { get; set; } = 0;

	/// <summary>
	/// Whether this is the most recent (current) save for its designated slot.
	/// Only meaningful when Origin == Designated and SlotNumber > 0.
	/// </summary>
	public bool IsCurrentSlot { get; set; } = false;

	/// <summary>
	/// Whether this entry is a recent ROM file (not a .rgd session save).
	/// </summary>
	public bool IsRecentRomFile { get; set; } = false;

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
			// Note: SaveStateManager::LoadState() restores pause state from the save data
			Task.Run(() => {
				EmuApi.LoadStateFile(FileName);
			});
		} else if (IsRecentRomFile) {
			// Load ROM file directly (no .rgd session data)
			_ = LoadRomHelper.LoadRom(FileName);
		} else if (StateIndex > 0) {
			Task.Run(() => {
				//Run in another thread to prevent deadlocks etc. when emulator notifications are processed UI-side
				if (SaveMode) {
					EmuApi.SaveState((uint)StateIndex);
					// OnCloseClick handles resuming when the dialog closes via NeedResume
				} else {
					// SaveStateManager::LoadState() restores pause state from the save data
					EmuApi.LoadState((uint)StateIndex);
				}
			});
		} else {
			// LoadRecentGame loads the ROM + state; pause state is restored by the C++ layer
			LoadRomHelper.LoadRecentGame(FileName, false);
		}
	}
}
