using System;
using System.Runtime.InteropServices;
using Nexen.Interop;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Config;

public sealed class Atari2600Config : BaseConfig<Atari2600Config> {
	[Reactive] public ConsoleOverrideConfig ConfigOverrides { get; set; } = new();

	[Reactive] public ControllerConfig Port1 { get; set; } = new();
	[Reactive] public ControllerConfig Port2 { get; set; } = new();

	[Reactive] public bool P0DifficultyB { get; set; } = true;
	[Reactive] public bool P1DifficultyB { get; set; } = true;
	[Reactive] public bool ColorMode { get; set; } = true;

	[Reactive] public RamState RamPowerOnState { get; set; } = RamState.AllZeros;

	[Reactive][MinMax(0, 100)] public UInt32 Channel0Vol { get; set; } = 100;
	[Reactive][MinMax(0, 100)] public UInt32 Channel1Vol { get; set; } = 100;

	[Reactive] public bool HidePlayfield { get; set; } = false;
	[Reactive] public bool HidePlayer0 { get; set; } = false;
	[Reactive] public bool HidePlayer1 { get; set; } = false;
	[Reactive] public bool HideMissile0 { get; set; } = false;
	[Reactive] public bool HideMissile1 { get; set; } = false;
	[Reactive] public bool HideBall { get; set; } = false;

	public void ApplyConfig() {
		ConfigManager.Config.Video.ApplyConfig();

		ConfigApi.SetAtari2600Config(new InteropAtari2600Config() {
			Port1 = Port1.ToInterop(),
			Port2 = Port2.ToInterop(),

			P0DifficultyB = P0DifficultyB,
			P1DifficultyB = P1DifficultyB,
			ColorMode = ColorMode,

			RamPowerOnState = RamPowerOnState,

			Channel0Vol = Channel0Vol,
			Channel1Vol = Channel1Vol,

			HidePlayfield = HidePlayfield,
			HidePlayer0 = HidePlayer0,
			HidePlayer1 = HidePlayer1,
			HideMissile0 = HideMissile0,
			HideMissile1 = HideMissile1,
			HideBall = HideBall,
		});
	}

	internal void InitializeDefaults(DefaultKeyMappingType defaultMappings) {
		Port1.InitDefaults(defaultMappings, ControllerType.Atari2600Joystick);
	}
}

[StructLayout(LayoutKind.Sequential)]
public struct InteropAtari2600Config {
	public InteropControllerConfig Port1;
	public InteropControllerConfig Port2;

	[MarshalAs(UnmanagedType.I1)] public bool P0DifficultyB;
	[MarshalAs(UnmanagedType.I1)] public bool P1DifficultyB;
	[MarshalAs(UnmanagedType.I1)] public bool ColorMode;

	public RamState RamPowerOnState;

	public UInt32 Channel0Vol;
	public UInt32 Channel1Vol;

	[MarshalAs(UnmanagedType.I1)] public bool HidePlayfield;
	[MarshalAs(UnmanagedType.I1)] public bool HidePlayer0;
	[MarshalAs(UnmanagedType.I1)] public bool HidePlayer1;
	[MarshalAs(UnmanagedType.I1)] public bool HideMissile0;
	[MarshalAs(UnmanagedType.I1)] public bool HideMissile1;
	[MarshalAs(UnmanagedType.I1)] public bool HideBall;
}
