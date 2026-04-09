using System;
using System.IO;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
public sealed partial class MovieRecordConfigViewModel : ViewModelBase {
	[Reactive] public partial string SavePath { get; set; }
	[Reactive] public partial MovieRecordConfig Config { get; set; }
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
