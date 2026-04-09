using System;
using System.Runtime.InteropServices;
using Nexen.Interop;
using ReactiveUI.SourceGenerators;

namespace Nexen.Config;

/// <summary>
/// Atari Lynx emulation configuration.
/// </summary>
/// <remarks>
/// Settings include:
/// - Boot ROM usage (required for some games, optional for most)
/// - Screen rotation (auto-detect from LNX header or manual override)
/// - Frame blending (reduces flicker on LCD games)
/// - Sprite/background layer toggles (debugging)
/// - Per-channel audio volume (0-100%)
/// </remarks>
public sealed partial class LynxConfig : BaseConfig<LynxConfig> {
	/// <summary>Console-specific overrides (video, speed, etc.).</summary>
	[Reactive] public partial ConsoleOverrideConfig ConfigOverrides { get; set; } = new();

	/// <summary>Controller configuration (key mappings, turbo).</summary>
	[Reactive] public partial ControllerConfig Controller { get; set; } = new();

	/// <summary>Use boot ROM for startup (512-byte Atari boot sequence).</summary>
	/// <remarks>
	/// When true, the emulator executes the Lynx boot ROM which displays
	/// the Atari logo and performs hardware initialization. Most games
	/// work without it via HLE boot, but some require the authentic boot.
	/// </remarks>
	[Reactive] public partial bool UseBootRom { get; set; } = false;

	/// <summary>Auto-detect screen rotation from LNX header.</summary>
	/// <remarks>
	/// When true, the emulator reads the rotation field from the LNX header
	/// and applies 90° left/right rotation automatically. Some games like
	/// Gauntlet: The Third Encounter are designed for portrait mode.
	/// </remarks>
	[Reactive] public partial bool AutoRotate { get; set; } = true;

	/// <summary>Blend consecutive frames to reduce LCD ghosting.</summary>
	/// <remarks>
	/// The Lynx LCD had significant persistence, causing visible trails
	/// on moving objects. Some games designed around this effect may
	/// look better with blending enabled.
	/// </remarks>
	[Reactive] public partial bool BlendFrames { get; set; } = false;

	/// <summary>Disable sprite rendering (debugging feature).</summary>
	[Reactive] public partial bool DisableSprites { get; set; } = false;

	/// <summary>Disable background rendering (debugging feature).</summary>
	[Reactive] public partial bool DisableBackground { get; set; } = false;

	/// <summary>Audio channel 1 volume (0-100%).</summary>
	[Reactive][MinMax(0, 100)] public partial UInt32 Channel1Vol { get; set; } = 100;

	/// <summary>Audio channel 2 volume (0-100%).</summary>
	[Reactive][MinMax(0, 100)] public partial UInt32 Channel2Vol { get; set; } = 100;

	/// <summary>Audio channel 3 volume (0-100%).</summary>
	[Reactive][MinMax(0, 100)] public partial UInt32 Channel3Vol { get; set; } = 100;

	/// <summary>Audio channel 4 volume (0-100%).</summary>
	[Reactive][MinMax(0, 100)] public partial UInt32 Channel4Vol { get; set; } = 100;

	/// <summary>Apply current configuration to the emulator core.</summary>
	public void ApplyConfig() {
		Controller.Type = ControllerType.LynxController;

		ConfigManager.Config.Video.ApplyConfig();

		ConfigApi.SetLynxConfig(new InteropLynxConfig() {
			Controller = Controller.ToInterop(),

			UseBootRom = UseBootRom,
			AutoRotate = AutoRotate,
			BlendFrames = BlendFrames,

			DisableSprites = DisableSprites,
			DisableBackground = DisableBackground,

			Channel1Vol = Channel1Vol,
			Channel2Vol = Channel2Vol,
			Channel3Vol = Channel3Vol,
			Channel4Vol = Channel4Vol,
		});
	}

	/// <summary>Initialize default key mappings for the Lynx controller.</summary>
	internal void InitializeDefaults(DefaultKeyMappingType defaultMappings) {
		Controller.InitDefaults(defaultMappings, ControllerType.LynxController);
	}
}

/// <summary>
/// Lynx configuration structure for interop with C++ core.
/// </summary>
/// <remarks>
/// This struct must match the memory layout of InteropLynxConfig in
/// InteropDLL/ConfigApiTypes.h. Fields are ordered to minimize padding.
/// </remarks>
[StructLayout(LayoutKind.Sequential)]
public struct InteropLynxConfig {
	/// <summary>Controller configuration.</summary>
	public InteropControllerConfig Controller;

	/// <summary>Use boot ROM.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool UseBootRom;
	/// <summary>Auto-detect rotation from LNX header.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool AutoRotate;
	/// <summary>Blend consecutive frames.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool BlendFrames;

	/// <summary>Disable sprite layer.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool DisableSprites;
	/// <summary>Disable background layer.</summary>
	[MarshalAs(UnmanagedType.I1)] public bool DisableBackground;

	/// <summary>Channel 1 volume (0-100).</summary>
	public UInt32 Channel1Vol;
	/// <summary>Channel 2 volume (0-100).</summary>
	public UInt32 Channel2Vol;
	/// <summary>Channel 3 volume (0-100).</summary>
	public UInt32 Channel3Vol;
	/// <summary>Channel 4 volume (0-100).</summary>
	public UInt32 Channel4Vol;
}
