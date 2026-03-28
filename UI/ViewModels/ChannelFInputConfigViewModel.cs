using System;
using Nexen.Config;

namespace Nexen.ViewModels;

public sealed class ChannelFInputConfigViewModel : DisposableViewModel {
	[ReactiveUI.Fody.Helpers.Reactive] public ChannelFConfig Config { get; set; }

	public Enum[] AvailableControllerTypes => new Enum[] {
		ControllerType.None,
		ControllerType.ChannelFController,
	};

	public Enum[] AvailableConsolePanelTypes => new Enum[] {
		ControllerType.None,
		ControllerType.ChannelFConsolePanel,
	};

	public ChannelFInputConfigViewModel() {
		Config = ConfigManager.Config.ChannelF;
	}

	public ChannelFInputConfigViewModel(ChannelFConfig config) {
		Config = config;
	}
}
