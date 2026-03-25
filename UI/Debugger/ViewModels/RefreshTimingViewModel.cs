using System;
using System.Reactive;
using Nexen.Config;
using Nexen.Interop;
using Nexen.ViewModels;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.ViewModels;
/// <summary>
/// ViewModel for configuring refresh timing settings in debugger windows.
/// </summary>
/// <remarks>
/// <para>
/// Controls when debugger views (memory, registers, etc.) refresh relative to
/// the emulated scanline and cycle. This allows examining the exact state at
/// specific points during frame rendering.
/// </para>
/// <para>
/// Each console type has different default refresh points:
/// <list type="bullet">
///   <item><description>SNES: scanline 240</description></item>
///   <item><description>NES: scanline 241 (start of vblank)</description></item>
///   <item><description>Game Boy: scanline 144</description></item>
///   <item><description>GBA: scanline 160</description></item>
/// </list>
/// </para>
/// </remarks>
public sealed class RefreshTimingViewModel : ViewModelBase {
	/// <summary>Gets the refresh timing configuration being edited.</summary>
	public RefreshTimingConfig Config { get; }

	/// <summary>Gets the minimum valid scanline number for this console.</summary>
	[Reactive] public int MinScanline { get; private set; }

	/// <summary>Gets the maximum valid scanline number for this console.</summary>
	[Reactive] public int MaxScanline { get; private set; }

	/// <summary>Gets the maximum valid cycle number for this console.</summary>
	[Reactive] public int MaxCycle { get; private set; }

	/// <summary>Gets the command to reset timing to default values.</summary>
	public ReactiveCommand<Unit, Unit> ResetCommand { get; }

	private CpuType _cpuType;

	/// <summary>
	/// Designer-only constructor. Do not use in code.
	/// </summary>
	[Obsolete("For designer only")]
	public RefreshTimingViewModel() : this(new RefreshTimingConfig(), CpuType.Snes) { }

	/// <summary>
	/// Initializes a new instance of the <see cref="RefreshTimingViewModel"/> class.
	/// </summary>
	/// <param name="config">The timing configuration to edit.</param>
	/// <param name="cpuType">The CPU type to get timing limits from.</param>
	public RefreshTimingViewModel(RefreshTimingConfig config, CpuType cpuType) {
		Config = config;
		_cpuType = cpuType;

		UpdateMinMaxValues(_cpuType);
		ResetCommand = ReactiveCommand.Create(Reset);
	}

	/// <summary>
	/// Resets the refresh timing to default values for the current console type.
	/// </summary>
	public void Reset() {
		Config.RefreshScanline = _cpuType.GetConsoleType() switch {
			ConsoleType.Snes => 240,
			ConsoleType.Nes => 241,
			ConsoleType.Gameboy => 144,
			ConsoleType.PcEngine => 240, //TODOv2
			ConsoleType.Sms => 192,
			ConsoleType.Gba => 160,
			ConsoleType.Ws => 144,
			ConsoleType.Lynx => 102,
			ConsoleType.Atari2600 => 192,
			_ => throw new Exception("Invalid console type")
		};

		Config.RefreshCycle = 0;
	}

	/// <summary>
	/// Updates the min/max scanline and cycle values for the specified CPU type.
	/// </summary>
	/// <param name="cpuType">The CPU type to get timing information from.</param>
	/// <remarks>
	/// Resets timing to defaults if current values are out of range for the new CPU type.
	/// </remarks>
	public void UpdateMinMaxValues(CpuType cpuType) {
		_cpuType = cpuType;
		TimingInfo timing = EmuApi.GetTimingInfo(_cpuType);
		MinScanline = timing.FirstScanline;
		MaxScanline = (int)timing.ScanlineCount + timing.FirstScanline - 1;
		MaxCycle = (int)timing.CycleCount - 1;

		// Reset if current values are out of range
		if (Config.RefreshScanline < MinScanline || Config.RefreshScanline > MaxScanline || Config.RefreshCycle > MaxCycle) {
			Reset();
		}
	}
}
