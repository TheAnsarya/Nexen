using Nexen.Interop;

namespace Nexen.Debugger.Integration; 
public sealed class NesDbgImporter : DbgImporter {
	public NesDbgImporter(RomFormat romFormat) : base(CpuType.Nes, romFormat, new() { MemoryType.NesPrgRom, MemoryType.NesWorkRam, MemoryType.NesSaveRam, MemoryType.NesInternalRam }) {
	}

	protected override CdlFlags GetOpFlags(byte opCode, int opSize) {
		return CdlFlags.None;
	}

	protected override bool IsBranchInstruction(byte opCode) {
		return opCode is 0x20 or 0x10 or 0x30 or 0x50 or 0x70 or 0x90 or 0xB0 or 0xD0 or 0xF0 or 0x4C or 0x20 or 0x4C or 0x6C;
	}

	protected override bool IsJumpToSubroutine(byte opCode) {
		return opCode == 0x20; //JSR/JSL
	}
}
