#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/AddressInfo.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/DebugUtilities.h"
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
/// - Page cache lookup for address translation (~2ns, no virtual calls)
/// - CDL byte OR operation (~1ns)
///
/// Total overhead: ~3-5ns per memory read vs ~15-20ns with virtual dispatch.
///
/// Performance optimization: A page cache is built at initialization that maps CPU-space
/// addresses to absolute ROM offsets. This eliminates all virtual calls (IConsole + IMemoryHandler)
/// from the per-read hot path. RecordInstruction still uses one virtual call for PC retrieval.
///
/// Usage:
/// 1. Emulator::StartLightweightCdl() creates this recorder
/// 2. ProcessInstruction/ProcessMemoryRead check _cdlRecorder before _debugger
/// 3. Page cache provides near-zero-cost address translation for memory reads
/// 4. CDL data can be saved/loaded in standard CDL file format
///
/// Thread safety: Only accessed from emulation thread (same as standard CDL).
/// Page cache must be rebuilt if memory mappings change (e.g., NES bank switching).
/// </remarks>
class LightweightCdlRecorder {
private:
	/// <summary>Cached address mapping for a single 4KB page.</summary>
	struct CdlPageEntry {
		int32_t baseOffset;  ///< Absolute offset of page start in ROM, or -1 if not ROM
	};

	static constexpr uint32_t PageShift = 12;                ///< 4KB pages (1 << 12)
	static constexpr uint32_t PageMask = (1 << PageShift) - 1;
	static constexpr uint32_t MaxCacheablePages = 0x10000;   ///< Max 64K pages (256MB address space)

	std::unique_ptr<uint8_t[]> _cdlData;   ///< CDL flags (one byte per ROM byte)
	uint32_t _cdlSize = 0;                 ///< Size of CDL data array (= PRG ROM size)
	MemoryType _prgRomType = {};            ///< ROM memory type for this console (e.g., NesPrgRom)
	CpuType _cpuType = {};                  ///< Main CPU type for this console
	MemoryType _cpuMemType = {};            ///< CPU memory type (e.g., SnesMemory)
	uint32_t _romCrc32 = 0;                 ///< ROM CRC32 for CDL file validation

	IConsole* _console = nullptr;           ///< Console for address translation (not owned)

	std::unique_ptr<CdlPageEntry[]> _pageCache;  ///< Page cache for fast address translation
	uint32_t _pageCacheSize = 0;                  ///< Number of entries in page cache

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
	/// Cost: ~10-15ns (one virtual call for PC + page cache lookup + one byte OR).
	/// </summary>
	__forceinline void RecordInstruction() {
		AddressInfo absAddr = GetPcAbsoluteAddress();
		if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
			uint8_t& entry = _cdlData[absAddr.Address];
			if ((entry & CdlFlags::Code) != CdlFlags::Code) {
				entry |= CdlFlags::Code;
			}
		}
	}

	/// <summary>
	/// Record a memory read as data in CDL.
	/// Called from Emulator::ProcessMemoryRead() for non-exec reads.
	/// Only records if the read address maps to PRG ROM.
	/// Uses page cache for near-zero-cost address translation (no virtual calls).
	/// </summary>
	/// <param name="relAddr">Relative address in CPU address space</param>
	/// <param name="memType">CPU memory type</param>
	/// <param name="opType">Memory operation type (to filter exec vs data reads)</param>
	__forceinline void RecordRead(uint32_t relAddr, MemoryType memType, MemoryOperationType opType) {
		if (opType != MemoryOperationType::ExecOperand && opType != MemoryOperationType::Read) [[likely]] {
			return; // DummyRead, DmaRead, InternalOperation, etc. — skip
		}

		uint8_t flag = (opType == MemoryOperationType::ExecOperand) ? CdlFlags::Code : CdlFlags::Data;

		// Fast path: use page cache (no virtual calls)
		uint32_t pageIndex = relAddr >> PageShift;
		if (memType == _cpuMemType && pageIndex < _pageCacheSize) [[likely]] {
			int32_t baseOffset = _pageCache[pageIndex].baseOffset;
			if (baseOffset >= 0) {
				uint32_t offset = (uint32_t)baseOffset + (relAddr & PageMask);
				if (offset < _cdlSize) {
					uint8_t& entry = _cdlData[offset];
					if ((entry & flag) != flag) {
						entry |= flag;
					}
				}
			}
			return;
		}

		// Slow path: virtual call fallback (coprocessor memory types or uncached addresses)
		AddressInfo addrInfo = { (int32_t)relAddr, memType };
		AddressInfo absAddr = _console->GetAbsoluteAddress(addrInfo);
		if (absAddr.Address >= 0 && absAddr.Type == _prgRomType) {
			uint8_t& entry = _cdlData[absAddr.Address];
			if ((entry & flag) != flag) {
				entry |= flag;
			}
		}
	}

	/// <summary>Reset all CDL flags to zero.</summary>
	void Reset();

	/// <summary>Get raw CDL data buffer.</summary>
	[[nodiscard]] uint8_t* GetRawData() { return _cdlData.get(); }

	/// <summary>Get CDL data size.</summary>
	[[nodiscard]] uint32_t GetSize() { return _cdlSize; }

	/// <summary>Get the ROM memory type being tracked.</summary>
	[[nodiscard]] MemoryType GetMemoryType() { return _prgRomType; }

	/// <summary>Get CDL statistics.</summary>
	[[nodiscard]] CdlStatistics GetStatistics();

	/// <summary>Load CDL data from file.</summary>
	[[nodiscard]] bool LoadCdlFile(const string& cdlFilepath);

	/// <summary>Save CDL data to file.</summary>
	[[nodiscard]] bool SaveCdlFile(const string& cdlFilepath);

	/// <summary>Get CDL data for a range.</summary>
	void GetCdlData(uint32_t offset, uint32_t length, uint8_t* cdlData);

	/// <summary>Set CDL data from buffer.</summary>
	void SetCdlData(uint8_t* cdlData, uint32_t length);

	/// <summary>Get CDL flags for a single address.</summary>
	[[nodiscard]] uint8_t GetFlags(uint32_t addr);

	/// <summary>
	/// Rebuild the page cache for address translation.
	/// Call this when memory mappings change (e.g., NES bank switching).
	/// Safe to call multiple times — rebuilds from scratch each time.
	/// </summary>
	void RebuildPageCache();

private:
	/// <summary>
	/// Get the absolute address of the current program counter.
	/// Delegates to IConsole::GetPcAbsoluteAddress().
	/// </summary>
	AddressInfo GetPcAbsoluteAddress();

	/// <summary>
	/// Build the page cache by resolving every 4KB page in the CPU address space.
	/// Each page maps to an absolute ROM offset (or -1 if not ROM).
	/// Verifies linear mapping within each page to ensure correctness.
	/// </summary>
	void BuildPageCache();

	/// <summary>
	/// Get the CPU address space size for a given CPU type.
	/// </summary>
	static uint32_t GetAddressSpaceSize(CpuType type);
};
