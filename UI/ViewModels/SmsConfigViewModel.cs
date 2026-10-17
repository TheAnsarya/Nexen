using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Controls;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class SmsConfigViewModel : DisposableViewModel {
	[Reactive] public partial SmsConfig Config { get; set; }
	[Reactive] public partial SmsConfig OriginalConfig { get; set; }
	[Reactive] public partial SmsConfigTab SelectedTab { get; set; } = 0;

	public SmsInputConfigViewModel Input { get; private set; }

	public Enum[] AvailableRegionsSms => new Enum[] {
		ConsoleRegion.Auto,
		ConsoleRegion.Ntsc,
		ConsoleRegion.Pal
	};

	public Enum[] AvailableRegionsGg => new Enum[] {
		ConsoleRegion.Auto,
		ConsoleRegion.Ntsc,
		ConsoleRegion.NtscJapan,
		ConsoleRegion.Pal
	};

	public SmsConfigViewModel() {
		Config = ConfigManager.Config.Sms;
		OriginalConfig = Config.Clone();
		Input = new SmsInputConfigViewModel(Config);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(Input);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}

public enum SmsConfigTab {
	General,
	Audio,
	Emulation,
	Input,
	Video
}
