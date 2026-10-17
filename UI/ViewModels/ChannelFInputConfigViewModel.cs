using System;
using Nexen.Config;
using ReactiveUI.SourceGenerators;

namespace Nexen.ViewModels;

public sealed partial class ChannelFInputConfigViewModel : DisposableViewModel {
	[Reactive] public partial ChannelFConfig Config { get; set; }

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
