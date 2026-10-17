#pragma once
#include "pch.h"
#include "Shared/BaseState.h"

/// <summary>
/// CX4 DMA transfer state.
/// The CX4 has built-in DMA capability for fast memory-to-memory transfers.
/// </summary>
struct Cx4Dma {
	/// <summary>Source address for DMA transfer.</summary>
	uint32_t Source;

	/// <summary>Destination address for DMA transfer.</summary>
	uint32_t Dest;

	/// <summary>Length of DMA transfer in bytes.</summary>
	uint16_t Length;

	/// <summary>Current position within the transfer.</summary>
	uint32_t Pos;

	/// <summary>DMA transfer is active when true.</summary>
	bool Enabled;
};

/// <summary>
/// CX4 suspend/wait state for timed delays.
/// </summary>
struct Cx4Suspend {
	/// <summary>Number of cycles remaining in suspend.</summary>
	uint32_t Duration;

	/// <summary>Suspend mode is active when true.</summary>
	bool Enabled;
};

/// <summary>
/// CX4 program cache state.
/// The CX4 uses a 2-page cache for program code to improve execution speed.
/// </summary>
struct Cx4Cache {
	/// <summary>Cache is enabled when true.</summary>
	bool Enabled;

	/// <summary>Cache preload mode — skip address match check and force load.</summary>
	bool Preload;

	/// <summary>Currently active cache page (0 or 1).</summary>
	uint8_t Page;

	/// <summary>Lock flags for each cache page (prevents eviction).</summary>
	bool Lock[2];

	/// <summary>Base addresses loaded in each cache page.</summary>
	uint32_t Address[2];

	/// <summary>Cache base address register.</summary>
	uint32_t Base;

	/// <summary>Current program bank being cached.</summary>
	uint16_t ProgramBank;

	/// <summary>Current program counter within cache.</summary>
	uint8_t ProgramCounter;

	/// <summary>Current position within cache page.</summary>
	uint16_t Pos;
};

/// <summary>
/// CX4 bus access state for memory operations.
/// </summary>
struct Cx4Bus {
	/// <summary>Bus operation is active when true.</summary>
	bool Enabled;

	/// <summary>A read operation is in progress.</summary>
	bool Reading;

	/// <summary>A write operation is in progress.</summary>
	bool Writing;

	/// <summary>Cycles remaining until bus operation completes.</summary>
	uint8_t DelayCycles;

	/// <summary>Address for current bus operation.</summary>
	uint32_t Address;
};

/// <summary>
/// Complete state of the CX4 (Hitachi HG51B169) coprocessor.
/// The CX4 is a Hitachi microcontroller used for wireframe 3D graphics in Mega Man X2/X3.
/// Features a 24-bit accumulator, 16 general registers, hardware multiplication,
/// DMA engine, and programmable micro-code execution.
/// </summary>
struct Cx4State : BaseState {
	/// <summary>Total cycles executed by the CX4.</summary>
	uint64_t CycleCount;

	/// <summary>Program bank register (PB) - selects code page.</summary>
	uint16_t PB;

	/// <summary>Program counter (PC) - instruction pointer within bank.</summary>
	uint8_t PC;

	/// <summary>Accumulator (A) - 24-bit main working register.</summary>
	uint32_t A;

	/// <summary>Page register (P) - memory page selection.</summary>
	uint16_t P;

	/// <summary>Stack pointer (SP) - points to top of 8-level call stack.</summary>
	uint8_t SP;

	/// <summary>Call stack - 8 levels of return addresses for subroutines.</summary>
	uint32_t Stack[8];

	/// <summary>Multiplier result register - holds 48-bit multiplication result.</summary>
	uint64_t Mult;

	/// <summary>ROM read buffer - holds data from ROM read operations.</summary>
	uint32_t RomBuffer;

	/// <summary>RAM read buffer - holds bytes from RAM read operations.</summary>
	uint8_t RamBuffer[3];

	/// <summary>Memory data register (MDR) - data for memory operations.</summary>
	uint32_t MemoryDataReg;

	/// <summary>Memory address register (MAR) - address for memory operations.</summary>
	uint32_t MemoryAddressReg;

	/// <summary>Data pointer register - additional address register.</summary>
	uint32_t DataPointerReg;

	/// <summary>General purpose registers (R0-R15).</summary>
	uint32_t Regs[16];

	/// <summary>Negative flag (N) - set when result is negative.</summary>
	bool Negative;

	/// <summary>Zero flag (Z) - set when result is zero.</summary>
	bool Zero;

	/// <summary>Carry flag (C) - set on unsigned overflow.</summary>
	bool Carry;

	/// <summary>Overflow flag (V) - set on signed overflow.</summary>
	bool Overflow;

	/// <summary>IRQ flag - interrupt is pending.</summary>
	bool IrqFlag;

	/// <summary>Stopped flag - CX4 execution is halted.</summary>
	bool Stopped;

	/// <summary>Locked flag - CX4 registers are locked from CPU access.</summary>
	bool Locked;

	/// <summary>IRQ disabled flag - interrupts are masked.</summary>
	bool IrqDisabled;

	/// <summary>Single ROM mode - reduced ROM access timing.</summary>
	bool SingleRom;

	/// <summary>ROM access delay in cycles.</summary>
	uint8_t RomAccessDelay;

	/// <summary>RAM access delay in cycles.</summary>
	uint8_t RamAccessDelay;

	/// <summary>Current bus operation state.</summary>
	Cx4Bus Bus;

	/// <summary>DMA transfer state.</summary>
	Cx4Dma Dma;

	/// <summary>Program cache state.</summary>
	Cx4Cache Cache;

	/// <summary>Suspend/wait state.</summary>
	Cx4Suspend Suspend;

	/// <summary>Interrupt vectors (32 bytes).</summary>
	uint8_t Vectors[0x20];
};
