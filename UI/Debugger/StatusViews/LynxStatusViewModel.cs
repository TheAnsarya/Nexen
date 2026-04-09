using System;
using System.Text;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.StatusViews;

/// <summary>
/// ViewModel for the Lynx CPU status panel in the debugger.
/// </summary>
/// <remarks>
/// Displays and allows editing of:
/// - 65C02 registers (A, X, Y, SP, PC, PS)
/// - Processor status flags (N, V, D, I, Z, C)
/// - PPU timing (cycle, scanline, frame count)
/// - Stack preview (current stack contents)
/// </remarks>
public sealed partial class LynxStatusViewModel : BaseConsoleStatusViewModel {
	/// <summary>Accumulator register.</summary>
	[Reactive] public partial byte RegA { get; set; }
	/// <summary>X index register.</summary>
	[Reactive] public partial byte RegX { get; set; }
	/// <summary>Y index register.</summary>
	[Reactive] public partial byte RegY { get; set; }
	/// <summary>Stack pointer (page $01).</summary>
	[Reactive] public partial byte RegSP { get; set; }
	/// <summary>Program counter.</summary>
	[Reactive] public partial UInt16 RegPC { get; set; }
	/// <summary>Processor status register.</summary>
	[Reactive] public partial byte RegPS { get; set; }

	/// <summary>Negative flag (bit 7 of PS).</summary>
	[Reactive] public partial bool FlagN { get; set; }
	/// <summary>Overflow flag (bit 6 of PS).</summary>
	[Reactive] public partial bool FlagV { get; set; }
	/// <summary>Decimal mode flag (bit 3 of PS).</summary>
	[Reactive] public partial bool FlagD { get; set; }
	/// <summary>Interrupt disable flag (bit 2 of PS).</summary>
	[Reactive] public partial bool FlagI { get; set; }
	/// <summary>Zero flag (bit 1 of PS).</summary>
	[Reactive] public partial bool FlagZ { get; set; }
	/// <summary>Carry flag (bit 0 of PS).</summary>
	[Reactive] public partial bool FlagC { get; set; }

	/// <summary>Current horizontal position within scanline.</summary>
	[Reactive] public partial UInt16 Cycle { get; private set; }
	/// <summary>Current scanline (0-104).</summary>
	[Reactive] public partial UInt16 Scanline { get; private set; }
	/// <summary>Total frames rendered.</summary>
	[Reactive] public partial UInt32 FrameCount { get; private set; }

	/// <summary>Hex dump of current stack contents.</summary>
	[Reactive] public partial string StackPreview { get; private set; } = "";

	/// <summary>Create a new Lynx status view model.</summary>
	public LynxStatusViewModel() {
		this.WhenAnyValue(x => x.FlagC, x => x.FlagD, x => x.FlagI, x => x.FlagN, x => x.FlagV, x => x.FlagZ).Subscribe(x => RegPS = (byte)(
			(FlagN ? 0x80 : 0) |
			(FlagV ? 0x40 : 0) |
			0x20 | // unused bit always set
			(FlagD ? 0x08 : 0) |
			(FlagI ? 0x04 : 0) |
			(FlagZ ? 0x02 : 0) |
			(FlagC ? 0x01 : 0)
		));

		this.WhenAnyValue(x => x.RegPS).Subscribe(x => {
			using var delayNotifs = DelayChangeNotifications();
			FlagN = (x & 0x80) != 0;
			FlagV = (x & 0x40) != 0;
			FlagD = (x & 0x08) != 0;
			FlagI = (x & 0x04) != 0;
			FlagZ = (x & 0x02) != 0;
			FlagC = (x & 0x01) != 0;
		});
	}

	/// <summary>Update UI state from emulator core.</summary>
	protected override void InternalUpdateUiState() {
		LynxCpuState cpu = DebugApi.GetCpuState<LynxCpuState>(CpuType.Lynx);
		LynxPpuState ppu = DebugApi.GetPpuState<LynxPpuState>(CpuType.Lynx);

		UpdateCycleCount(cpu.CycleCount);

		RegA = cpu.A;
		RegX = cpu.X;
		RegY = cpu.Y;
		RegSP = cpu.SP;
		RegPC = cpu.PC;
		RegPS = cpu.PS;

		StringBuilder sb = new StringBuilder();
		for (UInt32 i = (UInt32)0x0100 + cpu.SP + 1; i < 0x0200; i++) {
			sb.Append($"${DebugApi.GetMemoryValue(MemoryType.LynxMemory, i):X2} ");
		}

		StackPreview = sb.ToString();

		Cycle = ppu.Cycle;
		Scanline = ppu.Scanline;
		FrameCount = ppu.FrameCount;
	}

	/// <summary>Apply edited register values back to emulator core.</summary>
	protected override void InternalUpdateConsoleState() {
		LynxCpuState cpu = DebugApi.GetCpuState<LynxCpuState>(CpuType.Lynx);

		cpu.A = RegA;
		cpu.X = RegX;
		cpu.Y = RegY;
		cpu.SP = RegSP;
		cpu.PC = RegPC;
		cpu.PS = RegPS;

		DebugApi.SetCpuState(cpu, CpuType.Lynx);
	}
}
