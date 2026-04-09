using System;
using System.Text;
using Nexen.Interop;
using ReactiveUI;
using ReactiveUI.SourceGenerators;

namespace Nexen.Debugger.StatusViews; 
public sealed partial class NesStatusViewModel : BaseConsoleStatusViewModel {
	[Reactive] public partial byte RegA { get; set; }
	[Reactive] public partial byte RegX { get; set; }
	[Reactive] public partial byte RegY { get; set; }
	[Reactive] public partial byte RegSP { get; set; }
	[Reactive] public partial UInt16 RegPC { get; set; }
	[Reactive] public partial byte RegPS { get; set; }

	[Reactive] public partial bool FlagN { get; set; }
	[Reactive] public partial bool FlagV { get; set; }
	[Reactive] public partial bool FlagD { get; set; }
	[Reactive] public partial bool FlagI { get; set; }
	[Reactive] public partial bool FlagZ { get; set; }
	[Reactive] public partial bool FlagC { get; set; }

	[Reactive] public partial bool FlagNmi { get; set; }

	[Reactive] public partial bool FlagIrqExternal { get; set; }
	[Reactive] public partial bool FlagIrqFrameCount { get; set; }
	[Reactive] public partial bool FlagIrqDmc { get; set; }
	[Reactive] public partial bool FlagIrqFdsDisk { get; set; }

	[Reactive] public partial uint Cycle { get; private set; }
	[Reactive] public partial int Scanline { get; private set; }
	[Reactive] public partial UInt32 FrameCount { get; private set; }
	[Reactive] public partial UInt16 VramAddr { get; set; }
	[Reactive] public partial UInt16 TmpVramAddr { get; set; }
	[Reactive] public partial UInt16 BusAddr { get; set; }
	[Reactive] public partial byte ScrollX { get; set; }
	[Reactive] public partial bool Sprite0Hit { get; set; }
	[Reactive] public partial bool SpriteOverflow { get; set; }
	[Reactive] public partial bool VerticalBlank { get; set; }
	[Reactive] public partial bool WriteToggle { get; set; }

	//Mask
	[Reactive] public partial bool BgEnabled { get; set; }
	[Reactive] public partial bool SpritesEnabled { get; set; }
	[Reactive] public partial bool BgMaskLeft { get; set; }
	[Reactive] public partial bool SpriteMaskLeft { get; set; }
	[Reactive] public partial bool Grayscale { get; set; }
	[Reactive] public partial bool IntensifyRed { get; set; }
	[Reactive] public partial bool IntensifyGreen { get; set; }
	[Reactive] public partial bool IntensifyBlue { get; set; }

	//Control
	[Reactive] public partial bool LargeSprites { get; set; }
	[Reactive] public partial bool NmiOnVBlank { get; set; }
	[Reactive] public partial bool VerticalWrite { get; set; }
	[Reactive] public partial bool BgAt1000 { get; set; }
	[Reactive] public partial bool SpritesAt1000 { get; set; }

	[Reactive] public partial string StackPreview { get; private set; } = "";

	public NesStatusViewModel() {
		this.WhenAnyValue(x => x.FlagC, x => x.FlagD, x => x.FlagI, x => x.FlagN, x => x.FlagV, x => x.FlagZ).Subscribe(x => RegPS = (byte)(
				(FlagN ? (byte)NesCpuFlags.Negative : 0) |
				(FlagV ? (byte)NesCpuFlags.Overflow : 0) |
				(FlagD ? (byte)NesCpuFlags.Decimal : 0) |
				(FlagI ? (byte)NesCpuFlags.IrqDisable : 0) |
				(FlagZ ? (byte)NesCpuFlags.Zero : 0) |
				(FlagC ? (byte)NesCpuFlags.Carry : 0)
			));

		this.WhenAnyValue(x => x.RegPS).Subscribe(x => {
			using var delayNotifs = DelayChangeNotifications(); //don't reupdate PS while updating the flags
			FlagN = (x & (byte)NesCpuFlags.Negative) != 0;
			FlagV = (x & (byte)NesCpuFlags.Overflow) != 0;
			FlagD = (x & (byte)NesCpuFlags.Decimal) != 0;
			FlagI = (x & (byte)NesCpuFlags.IrqDisable) != 0;
			FlagZ = (x & (byte)NesCpuFlags.Zero) != 0;
			FlagC = (x & (byte)NesCpuFlags.Carry) != 0;
		});
	}

	protected override void InternalUpdateUiState() {
		NesCpuState cpu = DebugApi.GetCpuState<NesCpuState>(CpuType.Nes);
		NesPpuState ppu = DebugApi.GetPpuState<NesPpuState>(CpuType.Nes);

		UpdateCycleCount(cpu.CycleCount);

		RegA = cpu.A;
		RegX = cpu.X;
		RegY = cpu.Y;
		RegSP = cpu.SP;
		RegPC = cpu.PC;
		RegPS = cpu.PS;

		FlagNmi = cpu.NMIFlag;
		FlagIrqExternal = (cpu.IRQFlag & (byte)NesIrqSources.External) != 0;
		FlagIrqFrameCount = (cpu.IRQFlag & (byte)NesIrqSources.FrameCounter) != 0;
		FlagIrqDmc = (cpu.IRQFlag & (byte)NesIrqSources.DMC) != 0;
		FlagIrqFdsDisk = (cpu.IRQFlag & (byte)NesIrqSources.FdsDisk) != 0;

		StringBuilder sb = new StringBuilder();
		for (UInt32 i = (UInt32)0x100 + cpu.SP + 1; i < 0x200; i++) {
			sb.Append($"${DebugApi.GetMemoryValue(MemoryType.NesMemory, i):X2} ");
		}

		StackPreview = sb.ToString();

		Cycle = ppu.Cycle;
		Scanline = ppu.Scanline;
		FrameCount = ppu.FrameCount;
		VramAddr = ppu.VideoRamAddr;
		TmpVramAddr = ppu.TmpVideoRamAddr;
		ScrollX = ppu.ScrollX;
		BusAddr = ppu.BusAddress;

		Sprite0Hit = ppu.StatusFlags.Sprite0Hit;
		SpriteOverflow = ppu.StatusFlags.SpriteOverflow;
		VerticalBlank = ppu.StatusFlags.VerticalBlank;
		WriteToggle = ppu.WriteToggle;

		LargeSprites = ppu.Control.LargeSprites;
		NmiOnVBlank = ppu.Control.NmiOnVerticalBlank;
		VerticalWrite = ppu.Control.VerticalWrite;
		BgAt1000 = ppu.Control.BackgroundPatternAddr == 0x1000;
		SpritesAt1000 = ppu.Control.SpritePatternAddr == 0x1000;

		BgEnabled = ppu.Mask.BackgroundEnabled;
		SpritesEnabled = ppu.Mask.SpritesEnabled;
		BgMaskLeft = ppu.Mask.BackgroundMask;
		SpriteMaskLeft = ppu.Mask.SpriteMask;
		Grayscale = ppu.Mask.Grayscale;
		IntensifyRed = ppu.Mask.IntensifyRed;
		IntensifyGreen = ppu.Mask.IntensifyGreen;
		IntensifyBlue = ppu.Mask.IntensifyBlue;
	}

	protected override void InternalUpdateConsoleState() {
		NesCpuState cpu = DebugApi.GetCpuState<NesCpuState>(CpuType.Nes);
		NesPpuState ppu = DebugApi.GetPpuState<NesPpuState>(CpuType.Nes);

		cpu.A = RegA;
		cpu.X = RegX;
		cpu.Y = RegY;
		cpu.SP = RegSP;
		cpu.PC = RegPC;
		cpu.PS = RegPS;

		cpu.NMIFlag = FlagNmi;

		cpu.IRQFlag = (byte)(
			(FlagIrqExternal ? NesIrqSources.External : 0) |
			(FlagIrqFrameCount ? NesIrqSources.FrameCounter : 0) |
			(FlagIrqDmc ? NesIrqSources.DMC : 0) |
			(FlagIrqFdsDisk ? NesIrqSources.FdsDisk : 0)
		);

		ppu.Cycle = Cycle;
		ppu.Scanline = Scanline;
		ppu.VideoRamAddr = VramAddr;
		ppu.TmpVideoRamAddr = TmpVramAddr;
		ppu.ScrollX = ScrollX;
		ppu.BusAddress = BusAddr;

		ppu.StatusFlags.Sprite0Hit = Sprite0Hit;
		ppu.StatusFlags.SpriteOverflow = SpriteOverflow;
		ppu.StatusFlags.VerticalBlank = VerticalBlank;
		ppu.WriteToggle = WriteToggle;

		ppu.Control.LargeSprites = LargeSprites;
		ppu.Control.NmiOnVerticalBlank = NmiOnVBlank;
		ppu.Control.VerticalWrite = VerticalWrite;
		ppu.Control.BackgroundPatternAddr = (UInt16)(BgAt1000 ? 0x1000 : 0);
		ppu.Control.SpritePatternAddr = (UInt16)(SpritesAt1000 ? 0x1000 : 0);

		ppu.Mask.BackgroundEnabled = BgEnabled;
		ppu.Mask.SpritesEnabled = SpritesEnabled;
		ppu.Mask.BackgroundMask = BgMaskLeft;
		ppu.Mask.SpriteMask = SpriteMaskLeft;
		ppu.Mask.Grayscale = Grayscale;
		ppu.Mask.IntensifyRed = IntensifyRed;
		ppu.Mask.IntensifyGreen = IntensifyGreen;
		ppu.Mask.IntensifyBlue = IntensifyBlue;

		DebugApi.SetCpuState(cpu, CpuType.Nes);
		DebugApi.SetPpuState(ppu, CpuType.Nes);
	}
}
