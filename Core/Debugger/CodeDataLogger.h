#pragma once
#include "pch.h"
#include <memory>
#include "Debugger/DebugTypes.h"

class Disassembler;
class Debugger;

/// <summary>
/// Tracks which bytes in ROM/RAM are code vs data (CDL - Code/Data Log).
/// </summary>
/// <remarks>
/// Purpose:
/// - Track code coverage (which instructions have executed)
/// - Distinguish code from data (improve disassembly accuracy)
/// - Identify dead code (never executed)
/// - Generate CDL files for ROM hacking tools
/// 
/// CDL flags (per byte):
/// - Code: Byte executed as instruction
/// - Data: Byte read as data
/// - JumpTarget: Destination of jump/branch
/// - SubEntryPoint: Subroutine entry point (JSR/CALL target)
/// - IndirectCode: Code accessed via indirect jump
/// 
/// File format:
/// - Header: "CDLv2" + 4-byte ROM CRC32
/// - Data: One flag byte per ROM byte
/// - Portable across emulators (Mesen, FCEUX, etc.)
/// 
/// Performance optimizations:
/// - Template SetCode<flags, accessWidth>() for compile-time optimization
/// - Direct byte array access (no overhead)
/// - Inline flag checks
/// 
/// Use cases:
/// - ROM hacking: Identify unused code space
/// - Disassembly: Improve code/data detection
/// - Testing: Verify code coverage
/// - Strip unused data for size reduction
/// </remarks>
class CodeDataLogger {
protected:
	constexpr static int HeaderSize = 9; ///< CDL file header size ("CDLv2" + 4-byte CRC32)

	std::unique_ptr<uint8_t[]> _cdlData;  ///< CDL flags (one byte per ROM byte)
	CpuType _cpuType = CpuType::Snes;  ///< CPU type
	MemoryType _memType = {};     ///< Memory type being tracked
	uint32_t _memSize = 0;        ///< Memory size
	uint32_t _romCrc32 = 0;       ///< ROM CRC32 for file validation

	/// <summary>
	/// Load platform-specific CDL data.
	/// </summary>
	/// <param name="cdlData">CDL data buffer</param>
	/// <param name="cdlSize">CDL data size</param>
	virtual void InternalLoadCdlFile(uint8_t* cdlData, uint32_t cdlSize) {}
	
	/// <summary>
	/// Save platform-specific CDL data.
	/// </summary>
	/// <param name="cdlFile">Output file stream</param>
	virtual void InternalSaveCdlFile(ofstream& cdlFile) {}

public:
	/// <summary>
	/// Constructor for code/data logger.
	/// </summary>
	/// <param name="debugger">Debugger instance</param>
	/// <param name="memType">Memory type to track</param>
	/// <param name="memSize">Memory size</param>
	/// <param name="cpuType">CPU type</param>
	/// <param name="romCrc32">ROM CRC32 for validation</param>
	CodeDataLogger(Debugger* debugger, MemoryType memType, uint32_t memSize, CpuType cpuType, uint32_t romCrc32);
	
	/// <summary>
	/// Destructor - free CDL data.
	/// </summary>
	virtual ~CodeDataLogger();

	/// <summary>
	/// Reset all CDL flags to zero.
	/// </summary>
	virtual void Reset();
	
	/// <summary>
	/// Get raw CDL data buffer.
	/// </summary>
	/// <returns>CDL data pointer</returns>
	uint8_t* GetRawData();
	
	/// <summary>
	/// Get CDL data size.
	/// </summary>
	/// <returns>Size in bytes</returns>
	uint32_t GetSize();
	
	/// <summary>
	/// Get memory type being tracked.
	/// </summary>
	/// <returns>Memory type</returns>
	MemoryType GetMemoryType();

	/// <summary>
	/// Load CDL file from disk.
	/// </summary>
	/// <param name="cdlFilepath">CDL file path</param>
	/// <param name="autoResetCdl">True to reset CDL before loading</param>
	/// <returns>True if loaded successfully</returns>
	bool LoadCdlFile(string cdlFilepath, bool autoResetCdl);
	
	/// <summary>
	/// Save CDL file to disk.
	/// </summary>
	/// <param name="cdlFilepath">CDL file path</param>
	/// <returns>True if saved successfully</returns>
	bool SaveCdlFile(string cdlFilepath);
	
	/// <summary>
	/// Get default CDL file path for ROM.
	/// </summary>
	/// <param name="romName">ROM name</param>
	/// <returns>CDL file path</returns>
	string GetCdlFilePath(string romName);

	/// <summary>
	/// Mark bytes as code (compile-time flags).
	/// </summary>
	/// <typeparam name="flags">Extra CDL flags (JumpTarget, SubEntryPoint, etc.)</typeparam>
	/// <typeparam name="accessWidth">Number of bytes (1-4)</typeparam>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <remarks>
	/// Template for zero-overhead flag setting:
	/// - Compiler inlines and unrolls loop
	/// - No runtime branching
	/// </remarks>
	template <uint8_t flags = 0, uint8_t accessWidth = 1>
	void SetCode(int32_t absoluteAddr) {
		for (int i = 0; i < accessWidth; i++) {
			_cdlData[absoluteAddr + i] |= CdlFlags::Code | flags;
		}
	}

	/// <summary>
	/// Mark bytes as code (runtime flags).
	/// </summary>
	/// <typeparam name="accessWidth">Number of bytes (1-4)</typeparam>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <param name="flags">Extra CDL flags</param>
	/// <remarks>
	/// Extra flags only set on first byte.
	/// Remaining bytes marked as Code only.
	/// </remarks>
	template <uint8_t accessWidth = 1>
	void SetCode(int32_t absoluteAddr, uint8_t flags) {
		_cdlData[absoluteAddr] |= CdlFlags::Code | flags; // only sets extra flags on first byte
		if constexpr (accessWidth > 1) {
			for (int i = 1; i < accessWidth; i++) {
				_cdlData[absoluteAddr + i] |= CdlFlags::Code;
			}
		}
	}

	/// <summary>
	/// Mark bytes as data.
	/// </summary>
	/// <typeparam name="flags">Extra CDL flags</typeparam>
	/// <typeparam name="accessWidth">Number of bytes (1-4)</typeparam>
	/// <param name="absoluteAddr">Absolute address</param>
	template <uint8_t flags = 0, uint8_t accessWidth = 1>
	void SetData(int32_t absoluteAddr) {
		for (int i = 0; i < accessWidth; i++) {
			_cdlData[absoluteAddr + i] |= CdlFlags::Data | flags;
		}
	}

	/// <summary>
	/// Get CDL statistics (code/data byte counts).
	/// </summary>
	/// <returns>CDL statistics</returns>
	virtual CdlStatistics GetStatistics();

	/// <summary>
	/// Check if byte is marked as code.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <returns>True if code</returns>
	bool IsCode(uint32_t absoluteAddr);
	
	/// <summary>
	/// Check if byte is jump target.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <returns>True if jump target</returns>
	bool IsJumpTarget(uint32_t absoluteAddr);
	
	/// <summary>
	/// Check if byte is subroutine entry point.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <returns>True if sub entry</returns>
	bool IsSubEntryPoint(uint32_t absoluteAddr);
	
	/// <summary>
	/// Check if byte is marked as data.
	/// </summary>
	/// <param name="absoluteAddr">Absolute address</param>
	/// <returns>True if data</returns>
	bool IsData(uint32_t absoluteAddr);

	/// <summary>
	/// Set CDL data from buffer.
	/// </summary>
	/// <param name="cdlData">CDL data buffer</param>
	/// <param name="length">Data length</param>
	void SetCdlData(uint8_t* cdlData, uint32_t length);
	
	/// <summary>
	/// Get CDL data range.
	/// </summary>
	/// <param name="offset">Start offset</param>
	/// <param name="length">Data length</param>
	/// <param name="cdlData">Output buffer</param>
	void GetCdlData(uint32_t offset, uint32_t length, uint8_t* cdlData);
	
	/// <summary>
	/// Get CDL flags for address.
	/// </summary>
	/// <param name="addr">Address</param>
	/// <returns>CDL flags byte</returns>
	uint8_t GetFlags(uint32_t addr);

	/// <summary>
	/// Get all subroutine entry points.
	/// </summary>
	/// <param name="functions">Output function addresses</param>
	/// <param name="maxSize">Maximum array size</param>
	/// <returns>Number of functions found</returns>
	uint32_t GetFunctions(uint32_t functions[], uint32_t maxSize);

	/// <summary>
	/// Mark address range with flags.
	/// </summary>
	/// <param name="start">Start address</param>
	/// <param name="end">End address (inclusive)</param>
	/// <param name="flags">CDL flags to set</param>
	void MarkBytesAs(uint32_t start, uint32_t end, uint8_t flags);
	
	/// <summary>
	/// Strip data from ROM based on CDL flags.
	/// </summary>
	/// <param name="romBuffer">ROM buffer to modify</param>
	/// <param name="flag">Strip option (unused code, data, etc.)</param>
	/// <remarks>
	/// Use case: Remove unused data to reduce ROM size.
	/// </remarks>
	virtual void StripData(uint8_t* romBuffer, CdlStripOption flag);

	/// <summary>
	/// Rebuild disassembly cache based on CDL data.
	/// </summary>
	/// <param name="dis">Disassembler instance</param>
	virtual void RebuildPrgCache(Disassembler* dis);
};