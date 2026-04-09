using System;
using System.IO;
using System.Reactive.Linq;
using Nexen.Config;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class VideoRecordConfigViewModel : DisposableViewModel {
	[Reactive] public partial string SavePath { get; set; }
	[Reactive] public partial VideoRecordConfig Config { get; set; }

	[ObservableAsProperty] private bool _compressionAvailable;

	public VideoRecordConfigViewModel() {
		Config = ConfigManager.Config.VideoRecord.Clone();

		SavePath = Path.Join(ConfigManager.AviFolder, EmuApi.GetRomInfo().GetRomName() + (Config.Codec == VideoCodec.GIF ? ".gif" : ".avi"));

		AddDisposable(_compressionAvailableHelper = this.WhenAnyValue(x => x.Config.Codec).Select(x => x is VideoCodec.ZMBV or VideoCodec.CSCD).ToProperty(this, _ => _.CompressionAvailable));
		AddDisposable(this.WhenAnyValue(x => x.Config.Codec).Subscribe((codec) => {
			if (codec == VideoCodec.GIF && Path.GetExtension(SavePath).ToLowerInvariant() != ".gif") {
				SavePath = Path.ChangeExtension(SavePath, ".gif");
			} else if (codec != VideoCodec.GIF && Path.GetExtension(SavePath).ToLowerInvariant() == ".gif") {
				SavePath = Path.ChangeExtension(SavePath, ".avi");
			}
		}));
	}

	public void SaveConfig() {
		ConfigManager.Config.VideoRecord = Config.Clone();
	}
}
