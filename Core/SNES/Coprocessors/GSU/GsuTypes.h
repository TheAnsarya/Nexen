#pragma once
#include "pch.h"
#include "Shared/BaseState.h"

/// <summary>
/// GSU (Super FX) status and flag register state.
/// Contains all CPU flags and control bits for the GSU RISC processor.
/// The flags are split into low byte (SFR low) and high byte (SFR high) for register access.
/// </summary>
struct GsuFlags {
	/// <summary>Zero flag - Set when result is zero.</summary>
	bool Zero;

	/// <summary>Carry flag - Set on unsigned overflow/underflow.</summary>
	bool Carry;

	/// <summary>Sign flag - Set when result is negative (bit 15 set).</summary>
	bool Sign;

	/// <summary>Overflow flag - Set on signed overflow.</summary>
	bool Overflow;

	/// <summary>Running flag (G) - GSU is executing code when set.</summary>
	bool Running;

	/// <summary>ROM read pending flag (R) - ROM buffer read is in progress.</summary>
	bool RomReadPending;

	/// <summary>ALT1 prefix flag - Modifies instruction behavior.</summary>
	bool Alt1;

	/// <summary>ALT2 prefix flag - Modifies instruction behavior.</summary>
	bool Alt2;

	/// <summary>Immediate low flag (IL) - Low byte of immediate value pending.</summary>
	bool ImmLow;

	/// <summary>Immediate high flag (IH) - High byte of immediate value pending.</summary>
	bool ImmHigh;

	/// <summary>Prefix flag (B) - Instruction prefix is active.</summary>
	bool Prefix;

	/// <summary>IRQ flag - Interrupt is pending/requested.</summary>
	bool Irq;

	/// <summary>
	/// Gets the low byte of the status flag register (SFR).
	/// Contains Zero, Carry, Sign, Overflow, Running, and RomReadPending flags.
	/// </summary>
	/// <returns>SFR low byte with flags in bits 1-6.</returns>
	uint8_t GetFlagsLow() {
		return (
		    (Zero << 1) |
		    (Carry << 2) |
		    (Sign << 3) |
		    (Overflow << 4) |
		    (Running << 5) |
		    (RomReadPending << 6));
	}

	/// <summary>
	/// Gets the high byte of the status flag register (SFR).
	/// Contains Alt1, Alt2, ImmLow, ImmHigh, Prefix, and Irq flags.
	/// </summary>
	/// <returns>SFR high byte with flags in bits 0-4 and 7.</returns>
	uint8_t GetFlagsHigh() {
		return (
		    (Alt1 << 0) |
		    (Alt2 << 1) |
		    (ImmLow << 2) |
		    (ImmHigh << 3) |
		    (Prefix << 4) |
		    (Irq << 7));
	}
};

/// <summary>
/// GSU pixel cache for plot operations.
/// The Super FX uses pixel caching to optimize tile-based rendering.
/// Pixels are accumulated in the cache and flushed when a new tile is accessed.
/// </summary>
struct GsuPixelCache {
	/// <summary>X coordinate (0-7 within tile) of cached pixels.</summary>
	uint8_t X;

	/// <summary>Y coordinate (0-7 within tile) of cached pixels.</summary>
	uint8_t Y;

	/// <summary>Cached pixel data for up to 8 pixels (one tile row).</summary>
	uint8_t Pixels[8];

	/// <summary>Bitmask indicating which pixels in the cache are valid.</summary>
	uint8_t ValidBits;
};

/// <summary>
/// Complete state of the GSU (Super FX) coprocessor.
/// The GSU is a custom RISC CPU designed by Argonaut Software for 3D/scaling effects.
/// Used in games like Star Fox, Yoshi's Island, Doom, and Stunt Race FX.
/// Runs at 10.74 MHz (21.47 MHz clock with /2 divider) with 16 general-purpose registers.
/// </summary>
struct GsuState : BaseState {
	/// <summary>Total cycles executed by the GSU.</summary>
	uint64_t CycleCount;

	/// <summary>
	/// 16 general-purpose 16-bit registers (R0-R15).
	/// R0: Default source/destination for many operations.
	/// R13: Used as loop counter by LOOP instruction.
	/// R14: ROM address pointer for GETB/GETBH/GETBL.
	/// R15: Program counter (PC).
	/// </summary>
	uint16_t R[16];

	/// <summary>Status Flag Register containing all processor flags.</summary>
	GsuFlags SFR;

	/// <summary>Register latch for multi-byte register writes.</summary>
	uint8_t RegisterLatch;

	/// <summary>Program bank register (PBR) - Bank for instruction fetches.</summary>
	uint8_t ProgramBank;

	/// <summary>ROM bank register (ROMBR) - Bank for ROM data access.</summary>
	uint8_t RomBank;

	/// <summary>RAM bank register (RAMBR) - Bank for RAM access (0 or 1).</summary>
	uint8_t RamBank;

	/// <summary>IRQ disabled flag (I) - When set, GSU cannot trigger IRQs.</summary>
	bool IrqDisabled;

	/// <summary>High speed mode flag - Enables faster GSU clock.</summary>
	bool HighSpeedMode;

	/// <summary>Clock select - Determines GSU clock divider.</summary>
	bool ClockSelect;

	/// <summary>Backup RAM enabled - Allows GSU to access battery-backed SRAM.</summary>
	bool BackupRamEnabled;

	/// <summary>Screen base address (SCBR) - Base address for plot operations.</summary>
	uint8_t ScreenBase;

	/// <summary>Color gradient mode setting.</summary>
	uint8_t ColorGradient;

	/// <summary>Plot bits per pixel (2, 4, or 8 BPP).</summary>
	uint8_t PlotBpp;

	/// <summary>Screen height mode (128 or 160 pixels).</summary>
	uint8_t ScreenHeight;

	/// <summary>GSU RAM access flag - GSU has bus ownership of RAM.</summary>
	bool GsuRamAccess;

	/// <summary>GSU ROM access flag - GSU has bus ownership of ROM.</summary>
	bool GsuRomAccess;

	/// <summary>Cache base address (CBR) - Base address for instruction cache.</summary>
	uint16_t CacheBase;

	/// <summary>Plot transparent mode - Allows transparent pixels in plot.</summary>
	bool PlotTransparent;

	/// <summary>Plot dither mode - Enables dithering for plots.</summary>
	bool PlotDither;

	/// <summary>Color high nibble mode - Use high nibble of color register.</summary>
	bool ColorHighNibble;

	/// <summary>Color freeze high - Freezes high nibble of color.</summary>
	bool ColorFreezeHigh;

	/// <summary>Object mode - Special mode for sprite rendering.</summary>
	bool ObjMode;

	/// <summary>Color register (COLR) - Current color for plot operations.</summary>
	uint8_t ColorReg;

	/// <summary>Source register index (SREG) - Set by FROM instruction.</summary>
	uint8_t SrcReg;

	/// <summary>Destination register index (DREG) - Set by TO instruction.</summary>
	uint8_t DestReg;

	/// <summary>ROM read buffer - Holds data from ROM read operations.</summary>
	uint8_t RomReadBuffer;

	/// <summary>ROM delay counter - Cycles remaining for ROM access.</summary>
	uint8_t RomDelay;

	/// <summary>Program read buffer - Prefetched instruction byte.</summary>
	uint8_t ProgramReadBuffer;

	/// <summary>RAM write address - Pending RAM write destination.</summary>
	uint16_t RamWriteAddress;

	/// <summary>RAM write value - Pending RAM write data.</summary>
	uint8_t RamWriteValue;

	/// <summary>RAM delay counter - Cycles remaining for RAM access.</summary>
	uint8_t RamDelay;

	/// <summary>RAM address register - Current RAM access address.</summary>
	uint16_t RamAddress;

	/// <summary>Primary pixel cache for active plot operations.</summary>
	GsuPixelCache PrimaryCache;

	/// <summary>Secondary pixel cache for double-buffered plotting.</summary>
	GsuPixelCache SecondaryCache;
};