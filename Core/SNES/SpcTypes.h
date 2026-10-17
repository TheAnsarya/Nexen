#pragma once
#include "pch.h"
#include "SNES/SpcTimer.h"
#include "SNES/SnesCpuTypes.h"
#include "Shared/BaseState.h"

/// <summary>
/// Complete SPC700 APU state for the SNES audio processor.
/// Contains all registers, timers, and internal state for the Sony SPC700 CPU.
/// </summary>
/// <remarks>
/// The SPC700 is an 8-bit CPU dedicated to audio processing in the SNES.
/// It runs independently at ~1.024MHz with its own 64KB of RAM.
/// Key features:
/// - 8-bit accumulator (A), X, Y registers
/// - 8-bit stack pointer (stack in page 1)
/// - 256-byte direct page (selectable between page 0 or 1)
/// - 3 timers with different dividers
/// - 4 I/O ports for communication with main CPU
/// - IPL ROM for bootloader (can be disabled)
/// </remarks>
struct SpcState : BaseState {
	/// <summary>Total cycle count since power-on.</summary>
	uint64_t Cycle = 0;

	/// <summary>16-bit Program Counter.</summary>
	uint16_t PC = 0;

	/// <summary>8-bit Accumulator.</summary>
	uint8_t A = 0;

	/// <summary>8-bit Index Register X.</summary>
	uint8_t X = 0;

	/// <summary>8-bit Index Register Y.</summary>
	uint8_t Y = 0;

	/// <summary>8-bit Stack Pointer (stack is in page 1: $0100-$01FF).</summary>
	uint8_t SP = 0;

	/// <summary>8-bit Processor Status Word (PSW).</summary>
	uint8_t PS = 0;

	/// <summary>True when writes to RAM are enabled (via CONTROL register).</summary>
	bool WriteEnabled = false;

	/// <summary>True when IPL ROM is mapped at $FFC0-$FFFF.</summary>
	bool RomEnabled = false;

	/// <summary>Internal speed divider setting.</summary>
	uint8_t InternalSpeed = 0;

	/// <summary>External speed divider setting.</summary>
	uint8_t ExternalSpeed = 0;

	/// <summary>True when timers are globally enabled.</summary>
	bool TimersEnabled = false;

	/// <summary>True when timers are globally disabled.</summary>
	bool TimersDisabled = false;

	/// <summary>Current stop state (running, stopped, or waiting).</summary>
	SnesCpuStopState StopState = {};

	/// <summary>DSP register address for indirect access.</summary>
	uint8_t DspReg = 0;

	/// <summary>Output ports 0-3 ($F4-$F7 from SPC side, $2140-$2143 from CPU side).</summary>
	uint8_t OutputReg[4] = {};

	/// <summary>RAM registers for address calculation.</summary>
	uint8_t RamReg[2] = {};

	/// <summary>CPU-to-SPC input ports (latched values).</summary>
	uint8_t CpuRegs[4] = {};

	/// <summary>New CPU input values (not yet latched).</summary>
	uint8_t NewCpuRegs[4] = {};

	/// <summary>Timer 0 state (8KHz base, 128 divider).</summary>
	SpcTimer<128> Timer0;

	/// <summary>Timer 1 state (8KHz base, 128 divider).</summary>
	SpcTimer<128> Timer1;

	/// <summary>Timer 2 state (64KHz base, 16 divider).</summary>
	SpcTimer<16> Timer2;
};

/// <summary>
/// Processor Status Word flag bits for the SPC700.
/// </summary>
namespace SpcFlags {
enum SpcFlags : uint8_t {
	/// <summary>Carry flag - set by arithmetic and shift operations.</summary>
	Carry = 0x01,

	/// <summary>Zero flag - set when result is zero.</summary>
	Zero = 0x02,

	/// <summary>Interrupt Enable flag - unused on SPC700 (no IRQ line).</summary>
	IrqEnable = 0x04,

	/// <summary>Half-Carry flag - set on BCD half-carry (bit 3 to 4).</summary>
	HalfCarry = 0x08,

	/// <summary>Break flag - set by BRK instruction.</summary>
	Break = 0x10,

	/// <summary>
	/// Direct Page flag - selects direct page location.
	/// When set: direct page is $0100-$01FF. When clear: $0000-$00FF.
	/// </summary>
	DirectPage = 0x20,

	/// <summary>Overflow flag - set on signed arithmetic overflow.</summary>
	Overflow = 0x40,

	/// <summary>Negative flag - set when result bit 7 is set.</summary>
	Negative = 0x80
};
} // namespace SpcFlags

/// <summary>
/// CONTROL register ($F1) flag bits for SPC700 configuration.
/// </summary>
namespace SpcControlFlags {
enum SpcControlFlags : uint8_t {
	/// <summary>Enable Timer 0.</summary>
	Timer0 = 0x01,

	/// <summary>Enable Timer 1.</summary>
	Timer1 = 0x02,

	/// <summary>Enable Timer 2.</summary>
	Timer2 = 0x04,

	/// <summary>Clear input ports 0-1 when written.</summary>
	ClearPortsA = 0x10,

	/// <summary>Clear input ports 2-3 when written.</summary>
	ClearPortsB = 0x20,

	/// <summary>Enable IPL ROM mapping at $FFC0-$FFFF.</summary>
	EnableRom = 0x80
};
} // namespace SpcControlFlags

/// <summary>
/// TEST register ($F0) flag bits for undocumented SPC700 features.
/// </summary>
namespace SpcTestFlags {
enum SpcTestFlags : uint8_t {
	/// <summary>Disable all timers when set.</summary>
	TimersDisabled = 0x01,

	/// <summary>Enable RAM writes when set.</summary>
	WriteEnabled = 0x02,

	/// <summary>Unknown/unused flag.</summary>
	Unknown = 0x04,

	/// <summary>Enable timers when set.</summary>
	TimersEnabled = 0x08,

	/// <summary>External speed divider (bits 4-5).</summary>
	ExternalSpeed = 0x30,

	/// <summary>Internal speed divider (bits 6-7).</summary>
	InternalSpeed = 0xC0,
};
} // namespace SpcTestFlags

/// <summary>
/// SPC700 instruction execution step for cycle-accurate emulation.
/// </summary>
enum class SpcOpStep : uint8_t {
	/// <summary>Fetching opcode byte.</summary>
	ReadOpCode = 0,

	/// <summary>Processing addressing mode.</summary>
	Addressing = 1,

	/// <summary>Completing addressing mode calculation.</summary>
	AfterAddressing = 2,

	/// <summary>Executing the operation.</summary>
	Operation = 3
};