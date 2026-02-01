using System;
using System.IO;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels {
	public class MovieRecordConfigViewModel : ViewModelBase {
		[Reactive] public string SavePath { get; set; }
		[Reactive] public MovieRecordConfig Config { get; set; }

		public MovieRecordConfigViewModel() {
			Config = ConfigManager.Config.MovieRecord.Clone();

			SavePath = Path.Join(ConfigManager.MovieFolder, EmuApi.GetRomInfo().GetRomName() + "." + FileDialogHelper.NexenMovieExt);
		}

		public void SaveConfig() {
			ConfigManager.Config.MovieRecord = Config.Clone();
		}
	}
}
