using System;
using System.Runtime.InteropServices;
using Nexen.Interop;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class ChannelFConfig : BaseConfig<ChannelFConfig> {
	[Reactive] public ConsoleOverrideConfig ConfigOverrides { get; set; } = new();
	[Reactive] public ControllerConfig Controller { get; set; } = new();
	[Reactive] public ControllerConfig ConsolePanel { get; set; } = new();

	[Reactive] public RamState RamPowerOnState { get; set; } = RamState.AllZeros;

	[Reactive][MinMax(0, 100)] public UInt32 AudioVol { get; set; } = 100;

	public void ApplyConfig() {
		ConfigManager.Config.Video.ApplyConfig();

		ConfigApi.SetChannelFConfig(new InteropChannelFConfig() {
			Controller = Controller.ToInterop(),
			ConsolePanel = ConsolePanel.ToInterop(),
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
	public RamState RamPowerOnState;
	public UInt32 AudioVol;
}
