#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/AddressInfo.h"
#include "Debugger/DebugTypes.h"
#include "Shared/MemoryOperationType.h"
#include "Shared/Interfaces/IConsole.h"

/// <summary>
/// Lightweight Code/Data Logger that records CDL data without the full debugger.
/// </summary>
/// <remarks>
/// The standard CDL recording path requires InitializeDebugger() which creates ~20 subsystems
/// (Disassembler, MemoryAccessCounter, BreakpointManager, DummyCPU, etc.) adding 30-50% overhead.
///
/// This lightweight recorder provides the same CDL byte-array output (Code vs Data marking)
/// with only the minimal operations needed:
/// - Address translation via IConsole::GetAbsoluteAddress() (~5ns)
/// - CDL byte OR operation (~1ns)
///
/// Total overhead: ~10-15ns per instruction vs ~200-700ns with full debugger.
///
/// Usage:
/// 1. Emulator::StartLightweightCdl() creates this recorder
/// 2. ProcessInstruction/ProcessMemoryRead check _cdlRecorder before _debugger
/// 3. IConsole::GetPcAbsoluteAddress() provides the current PC's absolute address
/// 4. CDL data can be saved/loaded in standard CDL file format
///
/// Thread safety: Only accessed from emulation thread (same as standard CDL).
/// </remarks>
class LightweightCdlRecorder {
private:
	std::unique_ptr<uint8_t[]> _cdlData;   ///< CDL flags (one byte per ROM byte)
	uint32_t _cdlSize = 0;                 ///< Size of CDL data array (= PRG ROM size)
	MemoryType _prgRomType = {};            ///< ROM memory type for this console (e.g., NesPrgRom)
	CpuType _cpuType = {};                  ///< Main CPU type for this console
	uint32_t _romCrc32 = 0;                 ///< ROM CRC32 for CDL file validation

	IConsole* _console = nullptr;           ///< Console for address translation (not owned)

	static constexpr int HeaderSize = 9;    ///< CDL file header: "CDLv2" (5) + CRC32 (4)

public:
	/// <summary>
	/// Create a lightweight CDL recorder.
	/// </summary>
	/// <param name="console">Console instance for address translation</param>
	/// <param name="prgRomType">ROM memory type (e.g., MemoryType::NesPrgRom)</param>
	/// <param name="prgRomSize">Size of PRG ROM in bytes</param>
	/// <param name="cpuType">Main CPU type</param>
	/// <param name="romCrc32">ROM CRC32 for file validation</param>
	LightweightCdlRecorder(IConsole* console, MemoryType prgRomType, uint32_t prgRomSize, CpuType cpuType, uint32_t romCrc32);

	~LightweightCdlRecorder();

	/// <summary>
	/// Record current instruction as code in CDL.
	/// Called from Emulator::ProcessInstruction() on every CPU instruction.
	/// Cost: ~10-15ns (one virtual call for PC + one byte OR).
	/// </summary>
	__forceinline void RecordInstruction() {
		AddressInfo absAddr = GetPcAbsoluteAddress();
		if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
			_cdlData[absAddr.Address] |= CdlFlags::Code;
		}
	}

	/// <summary>
	/// Record a memory read as data in CDL.
	/// Called from Emulator::ProcessMemoryRead() for non-exec reads.
	/// Only records if the read address maps to PRG ROM.
	/// </summary>
	/// <param name="relAddr">Relative address in CPU address space</param>
	/// <param name="memType">CPU memory type</param>
	/// <param name="opType">Memory operation type (to filter exec vs data reads)</param>
	__forceinline void RecordRead(uint32_t relAddr, MemoryType memType, MemoryOperationType opType) {
		if (opType == MemoryOperationType::ExecOperand) {
			// Operand bytes are part of the instruction — mark as code
			AddressInfo addrInfo = { (int32_t)relAddr, memType };
			AddressInfo absAddr = _console->GetAbsoluteAddress(addrInfo);
			if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
				_cdlData[absAddr.Address] |= CdlFlags::Code;
			}
		} else if (opType == MemoryOperationType::Read) {
			// Data read from ROM
			AddressInfo addrInfo = { (int32_t)relAddr, memType };
			AddressInfo absAddr = _console->GetAbsoluteAddress(addrInfo);
			if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
				_cdlData[absAddr.Address] |= CdlFlags::Data;
			}
		}
		// DummyRead, DmaRead, InternalOperation, etc. — skip for lightweight CDL
	}

	/// <summary>Reset all CDL flags to zero.</summary>
	void Reset();

	/// <summary>Get raw CDL data buffer.</summary>
	uint8_t* GetRawData() { return _cdlData.get(); }

	/// <summary>Get CDL data size.</summary>
	uint32_t GetSize() { return _cdlSize; }

	/// <summary>Get the ROM memory type being tracked.</summary>
	MemoryType GetMemoryType() { return _prgRomType; }

	/// <summary>Get CDL statistics.</summary>
	CdlStatistics GetStatistics();

	/// <summary>Load CDL data from file.</summary>
	bool LoadCdlFile(string cdlFilepath);

	/// <summary>Save CDL data to file.</summary>
	bool SaveCdlFile(string cdlFilepath);

	/// <summary>Get CDL data for a range.</summary>
	void GetCdlData(uint32_t offset, uint32_t length, uint8_t* cdlData);

	/// <summary>Set CDL data from buffer.</summary>
	void SetCdlData(uint8_t* cdlData, uint32_t length);

	/// <summary>Get CDL flags for a single address.</summary>
	uint8_t GetFlags(uint32_t addr);

private:
	/// <summary>
	/// Get the absolute address of the current program counter.
	/// Delegates to IConsole::GetPcAbsoluteAddress().
	/// </summary>
	AddressInfo GetPcAbsoluteAddress();
};
