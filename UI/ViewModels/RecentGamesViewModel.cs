using Mesen.Config;
using Mesen.Interop;
using Mesen.Localization;
using Mesen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Mesen.ViewModels
{
	public class RecentGamesViewModel : ViewModelBase
	{
		[Reactive] public bool Visible { get; set; }
		[Reactive] public bool NeedResume { get; private set; }
		[Reactive] public bool IsPaused { get; private set; }
		[Reactive] public bool RestartGame { get; set; }
		[Reactive] public string Title { get; private set; } = "";
		[Reactive] public GameScreenMode Mode { get; private set; }
		[Reactive] public List<RecentGameInfo> GameEntries { get; private set; } = new List<RecentGameInfo>();
		
		public RecentGamesViewModel()
		{
			Visible = ConfigManager.Config.Preferences.GameSelectionScreenMode != GameSelectionMode.Disabled;
		}

		public void Init(GameScreenMode mode)
		{
			if(mode == GameScreenMode.RecentGames && ConfigManager.Config.Preferences.GameSelectionScreenMode == GameSelectionMode.Disabled) {
				Visible = false;
				GameEntries = new List<RecentGameInfo>();
				return;
			} else if(mode != GameScreenMode.RecentGames && Mode == mode && Visible) {
				Visible = false;
				ResumeIfNeeded();
				return;
			}

			if(Mode == mode && Visible && GameEntries.Count > 0) {
				//Prevent flickering when closing the config window while no game is running
				//No need to update anything if the game selection screen is already visible
				return;
			}

			Mode = mode;

			if(mode == GameScreenMode.RecentGames) {
				NeedResume = false;
				IsPaused = false;
				RestartGame = false;
				Title = ResourceHelper.GetMessage("GameSelectTitle");
				ShowEntries(GetRecentGameEntries());
			} else {
				if(!Visible) {
					NeedResume = Pause();
				}

				Title = ResourceHelper.GetMessage("SaveStatePickerTitle");
				List<RecentGameInfo> entries = GetTimestampedEntries();
				ShowEntries(entries);
				if(entries.Count == 0) {
					EmuApi.DisplayMessage("SaveStates", "NoSaveStates");
				}
			}
		}

		public void ShowGameSelect()
		{
			if(Visible && Mode == GameScreenMode.RecentGames) {
				if(EmuApi.IsRunning()) {
					Close();
				}
				return;
			}

			List<RecentGameInfo> entries = GetRecentGameEntries();
			if(entries.Count == 0) {
				EmuApi.DisplayMessage("SaveStates", "NoRecentGames");
				return;
			}

			if(!EmuApi.IsRunning()) {
				NeedResume = false;
			} else if(!Visible) {
				NeedResume = Pause();
			}

			Mode = GameScreenMode.RecentGames;
			RestartGame = false;
			Title = ResourceHelper.GetMessage("GameSelectTitle");
			ShowEntries(entries);
		}

		public void Close()
		{
			Visible = false;
			RestartGame = false;
			ResumeIfNeeded();
		}

		private void ShowEntries(List<RecentGameInfo> entries)
		{
			Visible = entries.Count > 0;
			if(!Visible) {
				ResumeIfNeeded();
			} else {
				IsPaused = EmuApi.IsPaused();
			}
			GameEntries = entries;
		}

		private List<RecentGameInfo> GetRecentGameEntries()
		{
			List<RecentGameInfo> entries = new();
			List<string> files = Directory.GetFiles(ConfigManager.RecentGamesFolder, "*.rgd").OrderByDescending((file) => new FileInfo(file).LastWriteTime).ToList();
			for(int i = 0; i < files.Count && entries.Count < 72; i++) {
				entries.Add(new RecentGameInfo() { FileName = files[i], Name = Path.GetFileNameWithoutExtension(files[i]), ForceRestart = () => RestartGame });
			}
			return entries;
		}

		private List<RecentGameInfo> GetTimestampedEntries()
		{
			List<RecentGameInfo> entries = new();
			foreach(SaveStateInfo state in EmuApi.GetSaveStateList()) {
				(string label, string color) = state.Origin switch {
					SaveStateOrigin.Auto => (ResourceHelper.GetMessage("AutoSave"), "#2E9E5B"),
					SaveStateOrigin.RecentPlay => (ResourceHelper.GetMessage("RecentPlaySave"), "#D9822B"),
					_ => (ResourceHelper.GetMessage("ManualSave"), "#1E6FD9")
				};

				entries.Add(new RecentGameInfo() {
					FileName = state.FilePath,
					Name = state.Timestamp.ToString("g"),
					BadgeText = label,
					BadgeColor = color,
					Timestamp = state.Timestamp,
					OnDeleted = RefreshPicker
				});
			}
			return entries;
		}

		private void RefreshPicker()
		{
			if(Mode != GameScreenMode.SaveStatePicker) {
				return;
			}

			List<RecentGameInfo> entries = GetTimestampedEntries();
			GameEntries = entries;
			if(entries.Count == 0) {
				Close();
				EmuApi.DisplayMessage("SaveStates", "NoSaveStates");
			}
		}

		private void ResumeIfNeeded()
		{
			if(NeedResume) {
				EmuApi.Resume();
				NeedResume = false;
			}
			IsPaused = EmuApi.IsPaused();
		}

		private bool Pause()
		{
			if(!EmuApi.IsPaused()) {
				EmuApi.Pause();
				IsPaused = EmuApi.IsPaused();
				return true;
			}
			IsPaused = true;
			return false;
		}
	}

	public enum GameScreenMode
	{
		RecentGames,
		SaveStatePicker
	}

	public class RecentGameInfo
	{
		public string FileName { get; set; } = "";
		public string Name { get; set; } = "";
		public string BadgeText { get; set; } = "";
		public string BadgeColor { get; set; } = "#1E6FD9";

		//Set for timestamped saves (picker mode)
		public DateTime? Timestamp { get; set; }
		public Action? OnDeleted { get; set; }

		//Set for recent games: when true, the saved state is ignored and the game starts from the beginning
		public Func<bool>? ForceRestart { get; set; }

		public bool IsEnabled()
		{
			return File.Exists(FileName);
		}

		public void Delete()
		{
			if(Timestamp.HasValue && EmuApi.DeleteSaveState(FileName)) {
				OnDeleted?.Invoke();
			}
		}

		public void Load()
		{
			if(Timestamp.HasValue) {
				Task.Run(() => {
					//Run in another thread to prevent deadlocks etc. when emulator notifications are processed UI-side
					EmuApi.LoadStateFile(FileName);
					EmuApi.Resume();
				});
			} else {
				LoadRomHelper.LoadRecentGame(FileName, false, ForceRestart?.Invoke() == true);
				EmuApi.Resume();
			}
		}
	}
}
