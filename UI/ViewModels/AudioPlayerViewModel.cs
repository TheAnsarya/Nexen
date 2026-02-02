using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Nexen.Config;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the audio player control displayed when playing audio files (NSF, SPC, GBS, HES).
/// </summary>
public class AudioPlayerViewModel : ViewModelBase {
	/// <summary>Gets or sets the audio player configuration.</summary>
	[Reactive] public AudioPlayerConfig Config { get; set; }

	/// <summary>Gets or sets whether playback is currently paused.</summary>
	[Reactive] public bool IsPaused { get; set; }

	/// <summary>
	/// Initializes a new instance of the <see cref="AudioPlayerViewModel"/> class.
	/// </summary>
	public AudioPlayerViewModel() {
		Config = ConfigManager.Config.AudioPlayer;

		this.WhenAnyValue(x => x.Config.Volume).Subscribe((vol) => Config.ApplyConfig());
	}

	/// <summary>
	/// Updates the paused state from the emulator.
	/// </summary>
	public void UpdatePauseFlag() {
		IsPaused = EmuApi.IsPaused();
	}
}
