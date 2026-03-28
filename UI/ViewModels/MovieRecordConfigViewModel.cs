using System;
using System.IO;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
public sealed class MovieRecordConfigViewModel : ViewModelBase {
	[Reactive] public string SavePath { get; set; }
	[Reactive] public MovieRecordConfig Config { get; set; }
	public bool CanRecord => !string.IsNullOrWhiteSpace(SavePath);

	public MovieRecordConfigViewModel() {
		Config = ConfigManager.Config.MovieRecord.Clone();

		SavePath = Path.Join(ConfigManager.MovieFolder, EmuApi.GetRomInfo().GetRomName() + "." + FileDialogHelper.NexenMovieExt);

		this.WhenAnyValue(x => x.SavePath).Subscribe(_ => {
			this.RaisePropertyChanged(nameof(CanRecord));
		});
	}

	public void SaveConfig() {
		ConfigManager.Config.MovieRecord = Config.Clone();
	}
}
