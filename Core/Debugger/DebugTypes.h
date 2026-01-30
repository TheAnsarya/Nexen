#pragma once
#include "pch.h"
#include "Shared/MemoryOperationType.h"
#include "Shared/CpuType.h"
#include "Debugger/DisassemblyInfo.h"
#include "Debugger/AddressInfo.h"

enum class MemoryType;
enum class CpuType : uint8_t;

/// <summary>
/// Memory operation information (read/write).
/// </summary>
/// <remarks>
/// Used by breakpoints, event viewer, trace logger.
/// Records address, value, operation type, memory type.
/// </remarks>
struct MemoryOperationInfo {
	uint32_t Address;         ///< Memory address
	int32_t Value;            ///< Value read/written
	MemoryOperationType Type; ///< Operation type (read/write/exec)
	MemoryType MemType;       ///< Memory type (PRG ROM/RAM/etc)

	/// <summary>
	/// Default constructor.
	/// </summary>
	MemoryOperationInfo() {
		Address = 0;
		Value = 0;
		Type = (MemoryOperationType)0;
		MemType = (MemoryType)0;
	}

	/// <summary>
	/// Constructor with initial values.
	/// </summary>
	MemoryOperationInfo(uint32_t addr, int32_t val, MemoryOperationType opType, MemoryType memType) {
		Address = addr;
		Value = val;
		Type = opType;
		MemType = memType;
	}
};

/// <summary>
/// Breakpoint type flags (bitfield).
/// </summary>
enum class BreakpointTypeFlags {
	None = 0,    ///< No breakpoint
	Read = 1,    ///< Break on memory read
	Write = 2,   ///< Break on memory write
	Execute = 4, ///< Break on code execution
	Forbid = 8,  ///< Forbid breakpoint (prevents execution)
};

/// <summary>
/// Breakpoint type (exclusive).
/// </summary>
enum class BreakpointType {
	Execute = 0, ///< Execute breakpoint
	Read = 1,    ///< Read breakpoint
	Write = 2,   ///< Write breakpoint
	Forbid = 3,  ///< Forbid breakpoint (prevents execution)
};

/// <summary>
/// Code/Data Logger (CDL) flags namespace.
/// </summary>
namespace CdlFlags {
/// <summary>
/// CDL flags for marking ROM bytes.
/// </summary>
enum CdlFlags : uint8_t {
	None = 0x00,          ///< Unmarked
	Code = 0x01,          ///< Executed as code
	Data = 0x02,          ///< Accessed as data
	JumpTarget = 0x04,    ///< Branch/jump target
	SubEntryPoint = 0x08, ///< Subroutine entry point
};
} // namespace CdlFlags

/// <summary>
/// CDL strip options for ROM stripping.
/// </summary>
enum class CdlStripOption {
	StripNone = 0, ///< No stripping
	StripUnused,   ///< Strip unused bytes
	StripUsed,     ///< Strip used bytes (keep only unused)
};

/// <summary>
/// CDL statistics for ROM analysis.
/// </summary>
struct CdlStatistics {
	uint32_t CodeBytes;  ///< Bytes executed as code
	uint32_t DataBytes;  ///< Bytes accessed as data
	uint32_t TotalBytes; ///< Total ROM bytes

	uint32_t JumpTargetCount; ///< Number of jump targets
	uint32_t FunctionCount;   ///< Number of subroutines

	// CHR ROM (NES-specific)
	uint32_t DrawnChrBytes; ///< CHR bytes drawn to screen
	uint32_t TotalChrBytes; ///< Total CHR ROM bytes
};

/// <summary>
/// Disassembly result for one instruction.
/// </summary>
/// <remarks>
/// Used by Disassembler to return disassembled instruction info.
/// Includes address info, flags, CPU address, comment line.
/// </remarks>
struct DisassemblyResult {
	AddressInfo Address; ///< Absolute address (PRG ROM/RAM)
	int32_t CpuAddress;  ///< CPU address ($0000-$FFFF)
	uint16_t Flags;      ///< LineFlags bitfield
	int16_t CommentLine; ///< Comment line number (or byte count)

	/// <summary>
	/// Constructor with CPU address only.
	/// </summary>
	DisassemblyResult(int32_t cpuAddress, uint16_t flags, int16_t commentLine = -1) {
		Flags = flags;
		CpuAddress = cpuAddress;
		Address.Address = -1;
		Address.Type = {};
		CommentLine = commentLine;
	}

	/// <summary>
	/// Constructor with absolute address.
	/// </summary>
	DisassemblyResult(AddressInfo address, int32_t cpuAddress, uint16_t flags = 0, int16_t commentLine = -1) {
		Address = address;
		CpuAddress = cpuAddress;
		Flags = flags;
		CommentLine = commentLine;
	}

	/// <summary>
	/// Set byte count (uses CommentLine field).
	/// </summary>
	void SetByteCount(uint8_t byteCount) {
		CommentLine = byteCount;
	}

	/// <summary>
	/// Get byte count (from CommentLine field).
	/// </summary>
	uint8_t GetByteCount() {
		return (uint8_t)CommentLine;
	}
};

/// <summary>
/// Disassembly line flags namespace.
/// </summary>
namespace LineFlags {
/// <summary>
/// Line flags for disassembly display.
/// </summary>
enum LineFlags : uint16_t {
	None = 0,                ///< No flags
	PrgRom = 0x01,           ///< PRG ROM
	WorkRam = 0x02,          ///< Work RAM
	SaveRam = 0x04,          ///< Save RAM (battery-backed)
	VerifiedData = 0x08,     ///< Verified as data (CDL)
	VerifiedCode = 0x10,     ///< Verified as code (CDL)
	BlockStart = 0x20,       ///< Block start marker
	BlockEnd = 0x40,         ///< Block end marker
	SubStart = 0x80,         ///< Subroutine start
	Label = 0x100,           ///< Has label
	Comment = 0x200,         ///< Has comment
	ShowAsData = 0x400,      ///< Force display as data
	UnexecutedCode = 0x800,  ///< Code never executed (CDL)
	UnmappedMemory = 0x1000, ///< Unmapped address
	Empty = 0x2000           ///< Empty line
};
} // namespace LineFlags

/// <summary>
/// Code line data for disassembly viewer.
/// </summary>
/// <remarks>
/// Full disassembled line info: address, opcode bytes, text, comment.
/// Used for debugger disassembly window display.
/// </remarks>
struct CodeLineData {
	int32_t Address;             ///< CPU address
	AddressInfo AbsoluteAddress; ///< Absolute address (PRG ROM/RAM)
	uint8_t OpSize;              ///< Opcode size (bytes)
	uint16_t Flags;              ///< LineFlags bitfield

	EffectiveAddressInfo EffectiveAddress; ///< Effective address (operand)
	uint32_t Value;                        ///< Operand value
	CpuType LineCpuType;                   ///< CPU type for this line

	uint8_t ByteCode[8]; ///< Opcode bytes
	char Text[1000];     ///< Disassembled text
	char Comment[1000];  ///< Comment text
};

/// <summary>
/// Tilemap display mode for tilemap viewer.
/// </summary>
enum class TilemapDisplayMode {
	Default,      ///< Normal rendering
	Grayscale,    ///< Grayscale display
	AttributeView ///< Show attribute data (palettes/flip)
};

struct AddressCounters;

/// <summary>
/// Tilemap highlight mode for access visualization.
/// </summary>
enum class TilemapHighlightMode {
	None,    ///< No highlighting
	Changes, ///< Highlight changed tiles
	Writes   ///< Highlight written tiles
};

/// <summary>
/// Tilemap viewer options.
/// </summary>
struct GetTilemapOptions {
	uint8_t Layer;                   ///< Layer number (0-3)
	uint8_t* CompareVram;            ///< VRAM snapshot for change detection
	AddressCounters* AccessCounters; ///< Access counters for highlighting

	uint64_t MasterClock;                        ///< Master clock for timing
	TilemapHighlightMode TileHighlightMode;      ///< Tile highlight mode
	TilemapHighlightMode AttributeHighlightMode; ///< Attribute highlight mode

	TilemapDisplayMode DisplayMode; ///< Display mode
};

/// <summary>
/// Tile format for tile viewer.
/// </summary>
enum class TileFormat {
	Bpp2,                 ///< 2bpp linear (NES, GB)
	Bpp4,                 ///< 4bpp linear
	Bpp8,                 ///< 8bpp linear
	DirectColor,          ///< Direct color (SNES mode 3/4)
	Mode7,                ///< SNES Mode 7
	Mode7DirectColor,     ///< SNES Mode 7 direct color
	Mode7ExtBg,           ///< SNES Mode 7 extended BG
	NesBpp2,              ///< NES 2bpp (planar)
	PceSpriteBpp4,        ///< PCE sprite 4bpp
	PceSpriteBpp2Sp01,    ///< PCE sprite 2bpp (SP0/SP1)
	PceSpriteBpp2Sp23,    ///< PCE sprite 2bpp (SP2/SP3)
	PceBackgroundBpp2Cg0, ///< PCE background 2bpp (CG0)
	PceBackgroundBpp2Cg1, ///< PCE background 2bpp (CG1)
	SmsBpp4,              ///< SMS 4bpp
	SmsSgBpp1,            ///< SMS SG-1000 1bpp
	GbaBpp4,              ///< GBA 4bpp
	GbaBpp8,              ///< GBA 8bpp
	WsBpp4Packed          ///< WonderSwan 4bpp packed
};

/// <summary>
/// Tile layout for tile viewer.
/// </summary>
enum class TileLayout {
	Normal,         ///< 8x8 tiles
	SingleLine8x16, ///< 8x16 tiles (single line)
	SingleLine16x16 ///< 16x16 tiles (single line)
};

/// <summary>
/// Tile background color for tile viewer.
/// </summary>
enum class TileBackground {
	Default,      ///< Default background
	Transparent,  ///< Transparent (color 0)
	PaletteColor, ///< Specific palette color
	Black,        ///< Black
	White,        ///< White
	Magenta,      ///< Magenta (common transparency color)
};

/// <summary>
/// Tile filter for tile viewer.
/// </summary>
enum class TileFilter {
	None,       ///< Show all tiles
	HideUnused, ///< Hide unused tiles (CDL)
	HideUsed    ///< Hide used tiles (CDL)
};

/// <summary>
/// Tile viewer options.
/// </summary>
struct GetTileViewOptions {
	MemoryType MemType;        ///< Memory type (CHR ROM/VRAM)
	TileFormat Format;         ///< Tile format
	TileLayout Layout;         ///< Tile layout
	TileFilter Filter;         ///< Tile filter
	TileBackground Background; ///< Background color
	int32_t Width;             ///< Viewer width (pixels)
	int32_t Height;            ///< Viewer height (pixels)
	int32_t StartAddress;      ///< Start address in memory
	int32_t Palette;           ///< Palette number
	bool UseGrayscalePalette;  ///< True for grayscale
};

/// <summary>
/// Sprite background color for sprite viewer.
/// </summary>
enum class SpriteBackground {
	Gray,        ///< Gray background
	Background,  ///< Use background layer
	Transparent, ///< Transparent
	Black,       ///< Black
	White,       ///< White
	Magenta,     ///< Magenta
};

/// <summary>
/// Sprite preview options.
/// </summary>
struct GetSpritePreviewOptions {
	SpriteBackground Background; ///< Background color/mode
};

/// <summary>
/// Palette viewer options.
/// </summary>
struct GetPaletteInfoOptions {
	TileFormat Format; ///< Tile format (affects palette size)
};

/// <summary>
/// Stack frame flags for interrupt context.
/// </summary>
enum class StackFrameFlags {
	None = 0, ///< Normal call
	Nmi = 1,  ///< NMI handler
	Irq = 2   ///< IRQ handler
};

/// <summary>
/// Callstack stack frame information.
/// </summary>
struct StackFrameInfo {
	uint32_t Source;             ///< Source address (caller)
	AddressInfo AbsSource;       ///< Absolute source address
	uint32_t Target;             ///< Target address (callee)
	AddressInfo AbsTarget;       ///< Absolute target address
	uint32_t Return;             ///< Return address
	uint32_t ReturnStackPointer; ///< Stack pointer at return
	AddressInfo AbsReturn;       ///< Absolute return address
	StackFrameFlags Flags;       ///< Interrupt flags
};

/// <summary>
/// Debug event types for event viewer.
/// </summary>
enum class DebugEventType {
	Register,      ///< Register write
	Nmi,           ///< NMI interrupt
	Irq,           ///< IRQ interrupt
	Breakpoint,    ///< Breakpoint hit
	BgColorChange, ///< Background color change
	SpriteZeroHit, ///< Sprite 0 hit (NES)
	DmcDmaRead,    ///< DMC DMA read (NES)
	DmaRead        ///< DMA read
};

/// <summary>
/// Break source for debugger break reasons.
/// </summary>
/// <remarks>
/// Values after InternalOperation are treated as "exceptions".
/// Forbid breakpoints can block exceptions, but not user breaks.
/// </remarks>
enum class BreakSource {
	Unspecified = -1, ///< No break source
	Breakpoint = 0,   ///< Breakpoint hit
	Pause,            ///< User pause
	CpuStep,          ///< CPU step
	PpuStep,          ///< PPU step

	Irq, ///< IRQ interrupt
	Nmi, ///< NMI interrupt

	// Used by DebugBreakHelper, prevents debugger getting focus
	InternalOperation,

	// Everything after InternalOperation is treated as an "Exception"
	// Forbid breakpoints can block these, but not the other types above
	BreakOnBrk,              ///< BRK instruction (6502)
	BreakOnCop,              ///< COP instruction (65816)
	BreakOnWdm,              ///< WDM instruction (65816)
	BreakOnStp,              ///< STP instruction (65816)
	BreakOnUninitMemoryRead, ///< Uninitialized memory read

	GbInvalidOamAccess,        ///< Game Boy: Invalid OAM access
	GbInvalidVramAccess,       ///< Game Boy: Invalid VRAM access
	GbDisableLcdOutsideVblank, ///< Game Boy: LCD disabled outside VBlank
	GbInvalidOpCode,           ///< Game Boy: Invalid opcode
	GbNopLoad,                 ///< Game Boy: NOP load (ld b,b)
	GbOamCorruption,           ///< Game Boy: OAM corruption

	NesBreakOnDecayedOamRead,  ///< NES: Decayed OAM read
	NesBreakOnPpuScrollGlitch, ///< NES: PPU scroll glitch
	BreakOnUnofficialOpCode,   ///< NES: Unofficial opcode
	BreakOnUnstableOpCode,     ///< NES: Unstable opcode
	NesBusConflict,            ///< NES: Bus conflict
	NesBreakOnCpuCrash,        ///< NES: CPU crash
	NesBreakOnExtOutputMode,   ///< NES: Extended output mode
	NesInvalidVramAccess,      ///< NES: Invalid VRAM access
	NesInvalidOamWrite,        ///< NES: Invalid OAM write
	NesDmaInputRead,           ///< NES: DMA input read

	PceBreakOnInvalidVramAddress, ///< PCE: Invalid VRAM address

	SmsNopLoad, ///< SMS: NOP load

	GbaInvalidOpCode,         ///< GBA: Invalid opcode
	GbaNopLoad,               ///< GBA: NOP load
	GbaUnalignedMemoryAccess, ///< GBA: Unaligned memory access

	SnesInvalidPpuAccess,  ///< SNES: Invalid PPU access
	SnesReadDuringAutoJoy, ///< SNES: Read during auto-joypad

	BreakOnUndefinedOpCode ///< Undefined opcode
};

/// <summary>
/// Break event information.
/// </summary>
struct BreakEvent {
	BreakSource Source;            ///< Break source
	CpuType SourceCpu;             ///< CPU type
	MemoryOperationInfo Operation; ///< Memory operation (if applicable)
	int32_t BreakpointId;          ///< Breakpoint ID (-1 if N/A)
};

/// <summary>
/// Step type for debugger stepping.
/// </summary>
enum class StepType {
	Step,             ///< Step one instruction
	StepOut,          ///< Step out of function
	StepOver,         ///< Step over function call
	CpuCycleStep,     ///< Step one CPU cycle
	PpuStep,          ///< Step one PPU cycle
	PpuScanline,      ///< Step one scanline
	PpuFrame,         ///< Step one frame
	SpecificScanline, ///< Run to specific scanline
	RunToNmi,         ///< Run until NMI
	RunToIrq,         ///< Run until IRQ
	StepBack          ///< Step backwards (rewind)
};

/// <summary>
/// Break type for debugger break classification.
/// </summary>
enum class BreakType {
	None = 0,      ///< No break
	User = 1,      ///< User break (breakpoint/pause)
	Exception = 2, ///< Exception break (BRK/invalid opcode/etc)
	Both = 3       ///< Both user and exception
};

/// <summary>
/// Step request for debugger stepping (hot path structure).
/// </summary>
/// <remarks>
/// This structure is checked EVERY instruction execution (hot path).
/// Performance-critical: uses __forceinline methods, bitflags.
///
/// Step tracking:
/// - StepCount: Instruction step counter
/// - PpuStepCount: PPU cycle step counter
/// - CpuCycleStepCount: CPU cycle step counter
/// - BreakAddress: Target address for step over/out
/// - BreakStackPointer: Stack pointer for step out
/// - BreakScanline: Target scanline for specific scanline step
///
/// Break classification:
/// - BreakType::User: User-initiated (breakpoint, pause, step)
/// - BreakType::Exception: Exception (BRK, invalid opcode, etc)
/// - Source: User break source
/// - ExSource: Exception break source
///
/// Forbid breakpoints:
/// - Can block exceptions but not user breaks
/// - InternalOperation threshold: values > InternalOperation are exceptions
/// </remarks>
struct StepRequest {
	int64_t BreakAddress = -1;         ///< Target address for break
	int64_t BreakStackPointer = -1;    ///< Stack pointer for step out
	int32_t StepCount = -1;            ///< Instruction step counter
	int32_t PpuStepCount = -1;         ///< PPU step counter
	int32_t CpuCycleStepCount = -1;    ///< CPU cycle step counter
	int32_t BreakScanline = INT32_MIN; ///< Target scanline
	StepType Type = StepType::Step;    ///< Step type

	bool HasRequest = false; ///< True if step request active

	BreakType BreakNeeded = BreakType::None;         ///< Break classification
	BreakSource Source = BreakSource::Unspecified;   ///< User break source
	BreakSource ExSource = BreakSource::Unspecified; ///< Exception break source

	/// <summary>
	/// Default constructor.
	/// </summary>
	StepRequest() {
	}

	/// <summary>
	/// Constructor with step type.
	/// </summary>
	StepRequest(StepType type) {
		Type = type;
	}

	/// <summary>
	/// Copy constructor (sets HasRequest based on counters).
	/// </summary>
	StepRequest(const StepRequest& obj) {
		Type = obj.Type;
		StepCount = obj.StepCount;
		PpuStepCount = obj.PpuStepCount;
		CpuCycleStepCount = obj.CpuCycleStepCount;
		BreakAddress = obj.BreakAddress;
		BreakStackPointer = obj.BreakStackPointer;
		BreakScanline = obj.BreakScanline;
		HasRequest = (StepCount != -1 || PpuStepCount != -1 || BreakAddress != -1 || BreakScanline != INT32_MIN || CpuCycleStepCount != -1);
	}

	/// <summary>
	/// Clear exception break (allows continued execution).
	/// </summary>
	void ClearException() {
		ExSource = BreakSource::Unspecified;
		ClearBreakType(BreakType::Exception);
	}

	/// <summary>
	/// Set break source and break flag (hot path).
	/// </summary>
	/// <remarks>
	/// Inline for performance - called every instruction.
	/// Values > InternalOperation are exceptions, others are user breaks.
	/// </remarks>
	__forceinline void SetBreakSource(BreakSource source, bool breakNeeded) {
		if (source > BreakSource::InternalOperation) {
			if (ExSource == BreakSource::Unspecified) {
				ExSource = source;
			}

			if (breakNeeded) {
				SetBreakType(BreakType::Exception);
			}
		} else {
			if (Source == BreakSource::Unspecified) {
				Source = source;
			}

			if (breakNeeded) {
				SetBreakType(BreakType::User);
			}
		}
	}

	/// <summary>
	/// Get effective break source (exception > user).
	/// </summary>
	BreakSource GetBreakSource() {
		if (ExSource != BreakSource::Unspecified) {
			return ExSource;
		}

		if (Source == BreakSource::Unspecified) {
			if (BreakScanline != INT32_MIN || PpuStepCount >= 0) {
				return BreakSource::PpuStep;
			}
		}

		return Source;
	}

	/// <summary>
	/// Set break type flag (hot path).
	/// </summary>
	__forceinline void SetBreakType(BreakType type) {
		BreakNeeded = (BreakType)((int)BreakNeeded | (int)type);
	}

	/// <summary>
	/// Clear break type flag (hot path).
	/// </summary>
	__forceinline void ClearBreakType(BreakType type) {
		BreakNeeded = (BreakType)((int)BreakNeeded & ~(int)type);
	}

	/// <summary>
	/// Request break (hot path).
	/// </summary>
	__forceinline void Break(BreakSource src) {
		SetBreakSource(src, true);
	}

	/// <summary>
	/// Process CPU instruction execution (hot path).
	/// </summary>
	/// <remarks>
	/// Inline for performance - called every instruction.
	/// Decrements step counter and sets break when 0.
	/// </remarks>
	__forceinline void ProcessCpuExec() {
		if (StepCount > 0) {
			StepCount--;
			if (StepCount == 0) {
				SetBreakSource(BreakSource::CpuStep, true);
			}
		}
	}

	/// <summary>
	/// Process CPU cycle (hot path).
	/// </summary>
	/// <returns>True if break reached</returns>
	__forceinline bool ProcessCpuCycle() {
		if (CpuCycleStepCount > 0) {
			CpuCycleStepCount--;
			if (CpuCycleStepCount == 0) {
				SetBreakSource(BreakSource::CpuStep, true);
				return true;
			}
		}
		return false;
	}

	/// <summary>
	/// Process NMI/IRQ interrupt (hot path).
	/// </summary>
	__forceinline void ProcessNmiIrq(bool forNmi) {
		if (forNmi) {
			if (Type == StepType::RunToNmi) {
				SetBreakSource(BreakSource::Nmi, true);
			}
		} else {
			if (Type == StepType::RunToIrq) {
				SetBreakSource(BreakSource::Irq, true);
			}
		}
	}

	/// <summary>
	/// Check if scanline break request active.
	/// </summary>
	bool HasScanlineBreakRequest() {
		return BreakScanline != INT32_MIN;
	}
};

/// <summary>
/// CPU instruction progress tracking (for instruction-level debugging).
/// </summary>
struct CpuInstructionProgress {
	uint64_t StartCycle = 0;                   ///< Cycle when instruction started
	uint64_t CurrentCycle = 0;                 ///< Current cycle within instruction
	uint32_t LastOpCode = 0;                   ///< Last opcode executed
	MemoryOperationInfo LastMemOperation = {}; ///< Last memory operation
};

/// <summary>
/// Debug controller state (for input recording/playback).
/// </summary>
struct DebugControllerState {
	bool A;      ///< A button
	bool B;      ///< B button
	bool X;      ///< X button (SNES/etc)
	bool Y;      ///< Y button (SNES/etc)
	bool L;      ///< L shoulder (SNES/etc)
	bool R;      ///< R shoulder (SNES/etc)
	bool U;      ///< U direction
	bool D;      ///< D direction
	bool Up;     ///< Up direction (D-pad)
	bool Down;   ///< Down direction (D-pad)
	bool Left;   ///< Left direction (D-pad)
	bool Right;  ///< Right direction (D-pad)
	bool Select; ///< Select button
	bool Start;  ///< Start button

	/// <summary>
	/// Check if any button pressed.
	/// </summary>
	bool HasPressedButton() {
		return A || B || X || Y || L || R || U || D || Up || Down || Left || Right || Select || Start;
	}
};
