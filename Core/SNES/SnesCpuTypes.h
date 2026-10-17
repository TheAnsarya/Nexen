#pragma once
#include "pch.h"
#include "Shared/BaseState.h"

/// <summary>
/// CPU stop state for power management and interrupt waiting.
/// </summary>
enum class SnesCpuStopState : uint8_t {
	/// <summary>CPU is running normally.</summary>
	Running = 0,

	/// <summary>CPU is halted by STP instruction (until reset).</summary>
	Stopped = 1,

	/// <summary>CPU is halted by WAI instruction (until interrupt).</summary>
	WaitingForIrq = 2
};

/// <summary>
/// Complete 65C816 CPU state for the SNES main CPU.
/// Contains all registers, flags, and internal state needed for emulation and debugging.
/// </summary>
/// <remarks>
/// The 65C816 is a 16-bit extension of the 6502 processor family.
/// It can operate in:
/// - Native mode: Full 16-bit capability with 24-bit addressing
/// - Emulation mode: 6502-compatible with 8-bit registers and 16-bit addressing
/// </remarks>
struct SnesCpuState : BaseState {
	/// <summary>Total CPU cycle count since power-on.</summary>
	uint64_t CycleCount;

	/// <summary>16-bit Accumulator register (or 8-bit A with hidden B byte).</summary>
	uint16_t A;

	/// <summary>16-bit Index Register X (or 8-bit in emulation/index mode).</summary>
	uint16_t X;

	/// <summary>16-bit Index Register Y (or 8-bit in emulation/index mode).</summary>
	uint16_t Y;

	/// <summary>
	/// 16-bit Stack Pointer.
	/// Points to next free location on stack (grows downward).
	/// In emulation mode, high byte is forced to $01.
	/// </summary>
	uint16_t SP;

	/// <summary>
	/// 16-bit Direct Page Register.
	/// Provides a 16-bit offset for direct addressing modes,
	/// allowing direct page to be relocated anywhere in bank 0.
	/// </summary>
	uint16_t D;

	/// <summary>16-bit Program Counter within the current program bank.</summary>
	uint16_t PC;

	/// <summary>
	/// 8-bit Program Bank Register (K).
	/// Holds the bank address ($00-$FF) for instruction fetches.
	/// Combined with PC to form 24-bit program address.
	/// </summary>
	uint8_t K;

	/// <summary>
	/// 8-bit Data Bank Register (DBR or B).
	/// Holds the bank address for memory transfers.
	/// Combined with 16-bit effective address for 24-bit data address.
	/// </summary>
	uint8_t DBR;

	/// <summary>8-bit Processor Status register containing flag bits.</summary>
	uint8_t PS;

	/// <summary>
	/// True when CPU is in 6502 emulation mode.
	/// In emulation mode: 8-bit registers, stack page $01, no bank registers.
	/// </summary>
	bool EmulationMode;

	// ==================== Internal State ====================

	/// <summary>NMI flag delay counter for edge detection.</summary>
	uint8_t NmiFlagCounter;

	/// <summary>True when IRQ polling is locked (during interrupt processing).</summary>
	bool IrqLock;

	/// <summary>True when NMI has been detected and needs processing.</summary>
	bool NeedNmi;

	/// <summary>Bitmask of active IRQ sources.</summary>
	uint8_t IrqSource;

	/// <summary>Previous IRQ source state for edge detection.</summary>
	uint8_t PrevIrqSource;

	/// <summary>Current CPU stop state (running, stopped, or waiting).</summary>
	SnesCpuStopState StopState;
};

/// <summary>
/// Processor status flag bits for the 65C816.
/// </summary>
namespace ProcFlags {
enum ProcFlags : uint8_t {
	/// <summary>Carry flag - set by arithmetic and shift operations.</summary>
	Carry = 0x01,

	/// <summary>Zero flag - set when result is zero.</summary>
	Zero = 0x02,

	/// <summary>Interrupt Disable flag - blocks IRQ when set (NMI still works).</summary>
	IrqDisable = 0x04,

	/// <summary>Decimal mode flag - enables BCD arithmetic for ADC/SBC.</summary>
	Decimal = 0x08,

	/// <summary>
	/// Index Register Size flag (X flag).
	/// When set: X and Y are 8-bit. When clear: X and Y are 16-bit.
	/// Always 1 in emulation mode.
	/// </summary>
	IndexMode8 = 0x10,

	/// <summary>
	/// Memory/Accumulator Size flag (M flag).
	/// When set: A is 8-bit. When clear: A is 16-bit.
	/// Always 1 in emulation mode.
	/// </summary>
	MemoryMode8 = 0x20,

	/// <summary>Overflow flag - set on signed arithmetic overflow.</summary>
	Overflow = 0x40,

	/// <summary>Negative flag - set when result bit 7 (or 15) is set.</summary>
	Negative = 0x80
};
} // namespace ProcFlags

/// <summary>
/// IRQ source identifiers for tracking multiple interrupt sources.
/// </summary>
enum class SnesIrqSource {
	/// <summary>No active IRQ source.</summary>
	None = 0,

	/// <summary>IRQ from PPU (H/V counter interrupt).</summary>
	Ppu = 1,

	/// <summary>IRQ from coprocessor (SA-1, SPC7110, etc.).</summary>
	Coprocessor = 2
};
