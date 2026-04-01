using System;
using System.Runtime.InteropServices;
using Nexen.Interop;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class ChannelFConfig : BaseConfig<ChannelFConfig> {
	[Reactive] public ConsoleOverrideConfig ConfigOverrides { get; set; } = new();
	[Reactive] public ControllerConfig Controller { get; set; } = new();
	[Reactive] public ControllerConfig ConsolePanel { get; set; } = new();
	[Reactive] public ChannelFConsoleVariant ConsoleVariant { get; set; } = ChannelFConsoleVariant.Auto;
	[Reactive] public ConsoleRegion Region { get; set; } = ConsoleRegion.Auto;

	[Reactive] public RamState RamPowerOnState { get; set; } = RamState.AllZeros;

	[Reactive][MinMax(0, 100)] public UInt32 AudioVol { get; set; } = 100;

	public void ApplyConfig() {
		ConfigManager.Config.Video.ApplyConfig();

		ConfigApi.SetChannelFConfig(new InteropChannelFConfig() {
			Controller = Controller.ToInterop(),
			ConsolePanel = ConsolePanel.ToInterop(),
			ConsoleVariant = ConsoleVariant,
			Region = Region,
			RamPowerOnState = RamPowerOnState,
			AudioVol = AudioVol,
		});
	}

	internal void InitializeDefaults(DefaultKeyMappingType defaultMappings) {
		Controller.InitDefaults(defaultMappings, ControllerType.ChannelFController);
	}
}

[StructLayout(LayoutKind.Sequential)]
public struct InteropChannelFConfig {
	public InteropControllerConfig Controller;
	public InteropControllerConfig ConsolePanel;
	public ChannelFConsoleVariant ConsoleVariant;
	public ConsoleRegion Region;
	public RamState RamPowerOnState;
	public UInt32 AudioVol;
}

public enum ChannelFConsoleVariant {
	Auto,
	SystemI,
	SystemII,
	SystemII_Luxor,
	Clone
}
