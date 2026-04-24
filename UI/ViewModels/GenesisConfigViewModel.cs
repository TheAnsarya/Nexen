using System;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;
public sealed partial class GenesisConfigViewModel : DisposableViewModel {
	[Reactive] public partial GenesisConfig Config { get; set; }
	[Reactive] public partial GenesisConfig OriginalConfig { get; set; }
	[Reactive] public partial GenesisConfigTab SelectedTab { get; set; } = 0;

	public GenesisInputConfigViewModel Input { get; private set; }

	public Enum[] AvailableRegions => new Enum[] {
		ConsoleRegion.Auto,
		ConsoleRegion.Ntsc,
		ConsoleRegion.Pal,
	};

	public GenesisConfigViewModel() {
		Config = ConfigManager.Config.Genesis;
		OriginalConfig = Config.Clone();
		Input = new GenesisInputConfigViewModel(Config);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(Input);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}

public enum GenesisConfigTab {
	General,
	Emulation,
	Input,
	Video
}
