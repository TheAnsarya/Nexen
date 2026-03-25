using System;
using System.Runtime.InteropServices;
using Nexen.Interop;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class GenesisConfig : BaseConfig<GenesisConfig> {
	[Reactive] public ConsoleOverrideConfig ConfigOverrides { get; set; } = new();

	[Reactive] public ControllerConfig Port1 { get; set; } = new();
	[Reactive] public ControllerConfig Port2 { get; set; } = new();

	[ValidValues(ConsoleRegion.Auto, ConsoleRegion.Ntsc, ConsoleRegion.Pal)]
	[Reactive] public ConsoleRegion Region { get; set; } = ConsoleRegion.Auto;

	[Reactive] public RamState RamPowerOnState { get; set; } = RamState.Random;

	[Reactive] public bool RemoveSpriteLimit { get; set; } = false;
	[Reactive] public bool DisableSprites { get; set; } = false;
	[Reactive] public bool DisableBackground { get; set; } = false;

	[Reactive] public OverscanConfig Overscan { get; set; } = new();

	public void ApplyConfig() {
		ConfigManager.Config.Video.ApplyConfig();

		ConfigApi.SetGenesisConfig(new InteropGenesisConfig() {
			Port1 = Port1.ToInterop(),
			Port2 = Port2.ToInterop(),

			Region = Region,
			RamPowerOnState = RamPowerOnState,

			RemoveSpriteLimit = RemoveSpriteLimit,
			DisableSprites = DisableSprites,
			DisableBackground = DisableBackground,

			Overscan = Overscan.ToInterop(),
		});
	}

	internal void InitializeDefaults(DefaultKeyMappingType defaultMappings) {
		Port1.InitDefaults(defaultMappings, ControllerType.GenesisController);
	}
}

[StructLayout(LayoutKind.Sequential)]
public struct InteropGenesisConfig {
	public InteropControllerConfig Port1;
	public InteropControllerConfig Port2;

	public ConsoleRegion Region;
	public RamState RamPowerOnState;

	[MarshalAs(UnmanagedType.I1)] public bool RemoveSpriteLimit;
	[MarshalAs(UnmanagedType.I1)] public bool DisableSprites;
	[MarshalAs(UnmanagedType.I1)] public bool DisableBackground;

	public InteropOverscanDimensions Overscan;
}
