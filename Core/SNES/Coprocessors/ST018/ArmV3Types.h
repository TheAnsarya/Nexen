#pragma once
#include "pch.h"
#include "Shared/MemoryType.h"
#include "Shared/BaseState.h"
#include "Utilities/Serializer.h"

/// <summary>
/// ARM v3 CPU operating modes.
/// The ARM processor has multiple privilege levels with separate register banks.
/// </summary>
enum class ArmV3CpuMode : uint8_t {
	/// <summary>Normal execution mode with lowest privilege.</summary>
	User = 0b10000,

	/// <summary>Fast interrupt mode with dedicated registers R8-R14.</summary>
	Fiq = 0b10001,

	/// <summary>Standard interrupt mode with dedicated R13-R14.</summary>
	Irq = 0b10010,

	/// <summary>Supervisor mode for OS/privileged code.</summary>
	Supervisor = 0b10011,

	/// <summary>Abort mode for memory access violations.</summary>
	Abort = 0b10111,

	/// <summary>Undefined instruction exception mode.</summary>
	Undefined = 0b11011,

	/// <summary>System mode (ARM v4+, same privileges as Supervisor).</summary>
	System = 0b11111,
};

/// <summary>
/// ARM v3 exception vector addresses.
/// When an exception occurs, the CPU branches to these fixed addresses.
/// </summary>
enum class ArmV3CpuVector : uint32_t {
	/// <summary>Undefined instruction exception vector at 0x04.</summary>
	Undefined = 0x04,

	/// <summary>Software interrupt (SWI) vector at 0x08.</summary>
	SoftwareIrq = 0x08,

	/// <summary>Prefetch abort (instruction fetch failed) at 0x0C.</summary>
	AbortPrefetch = 0x0C,

	/// <summary>Data abort (data access failed) at 0x10.</summary>
	AbortData = 0x10,

	/// <summary>Standard interrupt request vector at 0x18.</summary>
	Irq = 0x18,

	/// <summary>Fast interrupt request vector at 0x1C.</summary>
	Fiq = 0x1C,
};

/// <summary>Type alias for ARM memory access mode flags.</summary>
typedef uint8_t ArmV3AccessModeVal;

/// <summary>
/// ARM v3 memory access mode flags.
/// Controls how memory accesses are performed (sequential, word/byte, etc.).
/// </summary>
namespace ArmV3AccessMode {
enum Mode {
	/// <summary>Sequential access (faster timing for consecutive addresses).</summary>
	Sequential = (1 << 0),

	/// <summary>Word (32-bit) access.</summary>
	Word = (1 << 1),

	/// <summary>Byte (8-bit) access.</summary>
	Byte = (1 << 2),

	/// <summary>Do not rotate unaligned word reads.</summary>
	NoRotate = (1 << 3),

	/// <summary>This is an instruction prefetch.</summary>
	Prefetch = (1 << 4)
};
} // namespace ArmV3AccessMode

/// <summary>
/// ARM v3 CPU status flags (CPSR - Current Program Status Register).
/// Contains condition flags, interrupt masks, and current execution mode.
/// </summary>
struct ArmV3CpuFlags {
	/// <summary>Current CPU operating mode (User, FIQ, IRQ, Supervisor, etc.).</summary>
	ArmV3CpuMode Mode;

	/// <summary>FIQ disable bit - when set, fast interrupts are masked.</summary>
	bool FiqDisable;

	/// <summary>IRQ disable bit - when set, standard interrupts are masked.</summary>
	bool IrqDisable;

	/// <summary>Overflow flag (V) - set on signed arithmetic overflow.</summary>
	bool Overflow;

	/// <summary>Carry flag (C) - set on unsigned overflow or shift out.</summary>
	bool Carry;

	/// <summary>Zero flag (Z) - set when result is zero.</summary>
	bool Zero;

	/// <summary>Negative flag (N) - set when result bit 31 is set.</summary>
	bool Negative;

	/// <summary>
	/// Converts flags to 32-bit CPSR register format.
	/// </summary>
	/// <returns>CPSR value with flags in ARM-standard positions.</returns>
	uint32_t ToInt32() {
		return (
		    (Negative << 31) |
		    (Zero << 30) |
		    (Carry << 29) |
		    (Overflow << 28) |

		    (IrqDisable << 7) |
		    (FiqDisable << 6) |
		    (uint8_t)Mode);
	}
};

/// <summary>
/// ARM instruction data for pipeline stages.
/// </summary>
struct ArmV3InstructionData {
	/// <summary>Address where instruction was fetched from.</summary>
	uint32_t Address;

	/// <summary>32-bit instruction opcode.</summary>
	uint32_t OpCode;
};

/// <summary>
/// ARM 3-stage instruction pipeline state.
/// The ARM uses a fetch-decode-execute pipeline.
/// </summary>
struct ArmV3CpuPipeline {
	/// <summary>Instruction being fetched from memory.</summary>
	ArmV3InstructionData Fetch;

	/// <summary>Instruction being decoded.</summary>
	ArmV3InstructionData Decode;

	/// <summary>Instruction being executed.</summary>
	ArmV3InstructionData Execute;

	/// <summary>Pipeline flush requested (branch taken, etc.).</summary>
	bool ReloadRequested;

	/// <summary>Current memory access mode for pipeline.</summary>
	ArmV3AccessModeVal Mode;
};

/// <summary>
/// Complete ARM v3 CPU state including all registers and banked registers.
/// </summary>
struct ArmV3CpuState : BaseState {
	/// <summary>Instruction pipeline state.</summary>
	ArmV3CpuPipeline Pipeline;

	/// <summary>Current register set (R0-R15, R15 is PC).</summary>
	uint32_t R[16];

	/// <summary>Current Program Status Register (flags and mode).</summary>
	ArmV3CpuFlags CPSR;

	/// <summary>User mode banked registers (R8-R14).</summary>
	uint32_t UserRegs[7];

	/// <summary>FIQ mode banked registers (R8-R14).</summary>
	uint32_t FiqRegs[7];

	/// <summary>IRQ mode banked registers (R13-R14).</summary>
	uint32_t IrqRegs[2];

	/// <summary>Supervisor mode banked registers (R13-R14).</summary>
	uint32_t SupervisorRegs[2];

	/// <summary>Abort mode banked registers (R13-R14).</summary>
	uint32_t AbortRegs[2];

	/// <summary>Undefined mode banked registers (R13-R14).</summary>
	uint32_t UndefinedRegs[2];

	/// <summary>FIQ Saved Program Status Register.</summary>
	ArmV3CpuFlags FiqSpsr;

	/// <summary>IRQ Saved Program Status Register.</summary>
	ArmV3CpuFlags IrqSpsr;

	/// <summary>Supervisor Saved Program Status Register.</summary>
	ArmV3CpuFlags SupervisorSpsr;

	/// <summary>Abort Saved Program Status Register.</summary>
	ArmV3CpuFlags AbortSpsr;

	/// <summary>Undefined Saved Program Status Register.</summary>
	ArmV3CpuFlags UndefinedSpsr;

	/// <summary>Total cycles executed by the CPU.</summary>
	uint64_t CycleCount;
};
