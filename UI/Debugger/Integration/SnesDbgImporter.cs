using Nexen.Interop;

namespace Nexen.Debugger.Integration; 
public sealed class SnesDbgImporter : DbgImporter {
	public SnesDbgImporter(RomFormat romFormat) : base(CpuType.Snes, romFormat, new() { MemoryType.SnesPrgRom, MemoryType.SnesWorkRam, MemoryType.SnesSaveRam, MemoryType.SpcRam }) {
	}

	protected override CdlFlags GetOpFlags(byte opCode, int opSize) {
		if (opSize == 2) {
			if (IsVarWidthMemoryInstruction(opCode)) {
				//8-bit immediate memory operation, set M flag
				return CdlFlags.MemoryMode8;
			} else if (IsVarWidthIndexInstruction(opCode)) {
				//8-bit immediate index operation, set X flag
				return CdlFlags.IndexMode8;
			}
		}

		return CdlFlags.None;
	}

	protected override bool IsBranchInstruction(byte opCode) {
		return opCode is 0x20 or 0x10 or 0x30 or 0x50 or 0x70 or 0x80 or 0x90 or 0xB0 or 0xD0 or 0xF0 or 0x4C or 0x20 or 0x4C or 0x5C or 0x6C;
	}

	protected override bool IsJumpToSubroutine(byte opCode) {
		return opCode is 0x20 or 0x22; //JSR/JSL
	}

	private bool IsVarWidthIndexInstruction(byte opCode) {
		return opCode is 0xA0 or 0xA2 or 0xC0 or 0xE0;
	}

	private bool IsVarWidthMemoryInstruction(byte opCode) {
		return opCode is 0x09 or 0x29 or 0x49 or 0x69 or 0x89 or 0xA9 or 0xC9 or 0xE9;
	}
}
