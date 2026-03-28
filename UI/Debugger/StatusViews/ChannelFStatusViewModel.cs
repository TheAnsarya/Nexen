using System;
using System.Text;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;

namespace Nexen.Debugger.StatusViews;

/// <summary>
/// ViewModel for the Channel F CPU status panel in the debugger.
/// </summary>
/// <remarks>
/// Displays and allows editing of Fairchild F8 registers:
/// A (accumulator), W (status), ISAR, PC0/PC1, DC0/DC1,
/// scratchpad preview, and video state.
/// W flags: Sign(bit 3), Carry(bit 2), Zero(bit 1), Overflow(bit 0).
/// </remarks>
public sealed class ChannelFStatusViewModel : BaseConsoleStatusViewModel {
	/// <summary>Accumulator register.</summary>
	[Reactive] public byte RegA { get; set; }
	/// <summary>Status register (W) — flags.</summary>
	[Reactive] public byte RegW { get; set; }
	/// <summary>Indirect Scratchpad Address Register (6-bit).</summary>
	[Reactive] public byte RegISAR { get; set; }
	/// <summary>Program Counter 0 (active).</summary>
	[Reactive] public UInt16 RegPC0 { get; set; }
	/// <summary>Program Counter 1 (backup/stack).</summary>
	[Reactive] public UInt16 RegPC1 { get; set; }
	/// <summary>Data Counter 0 (active).</summary>
	[Reactive] public UInt16 RegDC0 { get; set; }
	/// <summary>Data Counter 1 (backup).</summary>
	[Reactive] public UInt16 RegDC1 { get; set; }

	/// <summary>Sign flag (bit 3 of W).</summary>
	[Reactive] public bool FlagS { get; set; }
	/// <summary>Carry/Link flag (bit 2 of W).</summary>
	[Reactive] public bool FlagC { get; set; }
	/// <summary>Zero flag (bit 1 of W).</summary>
	[Reactive] public bool FlagZ { get; set; }
	/// <summary>Overflow flag (bit 0 of W).</summary>
	[Reactive] public bool FlagO { get; set; }

	/// <summary>Interrupts enabled flag.</summary>
	[Reactive] public bool InterruptsEnabled { get; set; }

	/// <summary>Video: current drawing color.</summary>
	[Reactive] public byte VideoColor { get; private set; }
	/// <summary>Video: current X position.</summary>
	[Reactive] public byte VideoX { get; private set; }
	/// <summary>Video: current Y position.</summary>
	[Reactive] public byte VideoY { get; private set; }

	/// <summary>Hex dump of scratchpad registers pointed to by ISAR.</summary>
	[Reactive] public string ScratchpadPreview { get; private set; } = "";

	public ChannelFStatusViewModel() {
		// Sync individual flags → W register
		this.WhenAnyValue(x => x.FlagS, x => x.FlagC, x => x.FlagZ, x => x.FlagO)
			.Subscribe(x => RegW = (byte)(
				(FlagS ? 0x08 : 0) |
				(FlagC ? 0x04 : 0) |
				(FlagZ ? 0x02 : 0) |
				(FlagO ? 0x01 : 0)
			));

		// Sync W register → individual flags
		this.WhenAnyValue(x => x.RegW).Subscribe(x => {
			using var delayNotifs = DelayChangeNotifications();
			FlagS = (x & 0x08) != 0;
			FlagC = (x & 0x04) != 0;
			FlagZ = (x & 0x02) != 0;
			FlagO = (x & 0x01) != 0;
		});
	}

	/// <summary>Update UI state from emulator core.</summary>
	protected override void InternalUpdateUiState() {
		ChannelFCpuState cpu = DebugApi.GetCpuState<ChannelFCpuState>(CpuType.ChannelF);
		ChannelFVideoState video = DebugApi.GetPpuState<ChannelFVideoState>(CpuType.ChannelF);

		UpdateCycleCount(cpu.CycleCount);

		RegA = cpu.A;
		RegW = cpu.W;
		RegISAR = cpu.ISAR;
		RegPC0 = cpu.PC0;
		RegPC1 = cpu.PC1;
		RegDC0 = cpu.DC0;
		RegDC1 = cpu.DC1;
		InterruptsEnabled = cpu.InterruptsEnabled;

		VideoColor = video.Color;
		VideoX = video.X;
		VideoY = video.Y;

		// Show scratchpad around ISAR pointer (8 bytes centered on ISAR)
		if (cpu.Scratchpad is { Length: > 0 }) {
			StringBuilder sb = new();
			int isar = Math.Clamp(cpu.ISAR, 0, cpu.Scratchpad.Length - 1);
			int start = Math.Max(0, isar - 4);
			int end = Math.Min(cpu.Scratchpad.Length, start + 8);
			for (int i = start; i < end; i++) {
				if (i == isar) {
					sb.Append($"[{cpu.Scratchpad[i]:x2}]");
				} else {
					sb.Append($" {cpu.Scratchpad[i]:x2} ");
				}
			}
			ScratchpadPreview = sb.ToString();
		}
	}

	/// <summary>Apply edited register values back to emulator core.</summary>
	protected override void InternalUpdateConsoleState() {
		ChannelFCpuState cpu = DebugApi.GetCpuState<ChannelFCpuState>(CpuType.ChannelF);

		cpu.A = RegA;
		cpu.W = RegW;
		cpu.ISAR = RegISAR;
		cpu.PC0 = RegPC0;
		cpu.PC1 = RegPC1;
		cpu.DC0 = RegDC0;
		cpu.DC1 = RegDC1;
		cpu.InterruptsEnabled = InterruptsEnabled;

		DebugApi.SetCpuState(cpu, CpuType.ChannelF);
	}
}
