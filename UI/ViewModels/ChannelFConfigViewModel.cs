using System;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;

public sealed partial class ChannelFConfigViewModel : DisposableViewModel {
	[Reactive] public partial ChannelFConfig Config { get; set; }
	[Reactive] public partial ChannelFConfig OriginalConfig { get; set; }
	[Reactive] public partial ChannelFConfigTab SelectedTab { get; set; } = 0;

	public ChannelFInputConfigViewModel Input { get; private set; }

	public ChannelFConfigViewModel() {
		Config = ConfigManager.Config.ChannelF;
		OriginalConfig = Config.Clone();
		Input = new ChannelFInputConfigViewModel(Config);

		if (Design.IsDesignMode) {
			return;
		}

		AddDisposable(Input);
		AddDisposable(ReactiveHelper.RegisterRecursiveObserver(Config, (s, e) => Config.ApplyConfig()));
	}
}

public enum ChannelFConfigTab {
	Audio,
	Emulation,
	Input,
	Video
}
