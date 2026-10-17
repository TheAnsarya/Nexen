using System;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;

public sealed partial class Atari2600ConfigViewModel : DisposableViewModel {
	[Reactive] public partial Atari2600Config Config { get; set; }
	[Reactive] public partial Atari2600Config OriginalConfig { get; set; }
	[Reactive] public partial Atari2600ConfigTab SelectedTab { get; set; } = 0;

	public Atari2600InputConfigViewModel Input { get; private set; }

	public Atari2600ConfigViewModel() {
		Config = ConfigManager.Config.Atari2600;
		OriginalConfig = Config.Clone();
		Input = new Atari2600InputConfigViewModel(Config);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(Input);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}

public enum Atari2600ConfigTab {
	General,
	Audio,
	Emulation,
	Input,
	Video
}
