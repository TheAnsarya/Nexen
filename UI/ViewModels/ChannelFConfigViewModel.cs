using System;
using Avalonia.Controls;
using Nexen.Config;
using Nexen.Utilities;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.ViewModels;

public sealed class ChannelFConfigViewModel : DisposableViewModel {
	[Reactive] public ChannelFConfig Config { get; set; }
	[Reactive] public ChannelFConfig OriginalConfig { get; set; }
	[Reactive] public ChannelFConfigTab SelectedTab { get; set; } = 0;

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
