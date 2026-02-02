using System;
using System.Collections.Generic;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Interop;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;
/// <summary>
/// ViewModel for the audio configuration tab.
/// </summary>
public class AudioConfigViewModel : DisposableViewModel {
	/// <summary>Gets or sets the current audio configuration.</summary>
	[Reactive] public AudioConfig Config { get; set; }

	/// <summary>Gets or sets the original audio configuration for revert.</summary>
	[Reactive] public AudioConfig OriginalConfig { get; set; }

	/// <summary>Gets or sets the list of available audio devices.</summary>
	[Reactive] public List<string> AudioDevices { get; set; } = new();

	/// <summary>Gets or sets whether to show the latency warning.</summary>
	[Reactive] public bool ShowLatencyWarning { get; set; } = false;

	/// <summary>
	/// Initializes a new instance of the <see cref="AudioConfigViewModel"/> class.
	/// </summary>
	public AudioConfigViewModel() {
		Config = ConfigManager.Config.Audio;
		OriginalConfig = Config.Clone();

		if (Design.IsDesignMode) {
			return;
		}

		AudioDevices = ConfigApi.GetAudioDevices();
		if (AudioDevices.Count > 0 && !AudioDevices.Contains(Config.AudioDevice)) {
			Config.AudioDevice = AudioDevices[0];
		}

		AddDisposable(this.WhenAnyValue(x => x.Config.AudioLatency).Subscribe(x => ShowLatencyWarning = Config.AudioLatency <= 55));

		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}
