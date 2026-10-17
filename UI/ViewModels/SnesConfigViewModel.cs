using System;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Localization;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels; 
public sealed partial class SnesConfigViewModel : DisposableViewModel {
	[Reactive] public partial SnesConfig Config { get; set; }
	[Reactive] public partial SnesConfig OriginalConfig { get; set; }
	[Reactive] public partial SnesConfigTab SelectedTab { get; set; } = 0;

	public SnesInputConfigViewModel Input { get; private set; }

	[Reactive] public partial bool IsDefaultSpcClockSpeed { get; set; } = true;
	[Reactive] public partial string SpcEffectiveClockSpeed { get; set; } = "";

	public Enum[] AvailableRegions => new Enum[] {
		ConsoleRegion.Auto,
		ConsoleRegion.Ntsc,
		ConsoleRegion.Pal
	};

	public SnesConfigViewModel() {
		Config = ConfigManager.Config.Snes;
		OriginalConfig = Config.Clone();
		Input = new SnesInputConfigViewModel(Config);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(Input);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));

		AddDisposable(this.WhenAnyValue(x => x.Config.SpcClockSpeedAdjustment).Subscribe(x => {
			SpcEffectiveClockSpeed = ResourceHelper.GetMessage("SpcClockSpeedMsg", ((32000 + x) * 32).ToString());
			IsDefaultSpcClockSpeed = x == 40;
		}));
	}
}

public enum SnesConfigTab {
	General,
	Audio,
	Emulation,
	Input,
	Overclocking,
	Video,
	Bsx
}
