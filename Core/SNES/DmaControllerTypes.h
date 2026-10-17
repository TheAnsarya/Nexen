#pragma once
#include "pch.h"

/// <summary>
/// Configuration state for a single DMA/HDMA channel.
/// </summary>
/// <remarks>
/// The SNES has 8 DMA channels that can be used for:
/// - General DMA: Block transfers triggered by software
/// - HDMA: Automatic transfers during HBlank every scanline
/// Each channel has independent configuration for source, destination,
/// transfer mode, and direction.
/// </remarks>
struct DmaChannelConfig {
	/// <summary>Source address within the source bank (A-Bus address low 16 bits).</summary>
	uint16_t SrcAddress;

	/// <summary>
	/// Transfer size/count for general DMA.
	/// For HDMA indirect mode: current indirect address.
	/// </summary>
	uint16_t TransferSize;

	/// <summary>HDMA table address in CPU address space.</summary>
	uint16_t HdmaTableAddress;

	/// <summary>Source bank number (A-Bus address bits 16-23).</summary>
	uint8_t SrcBank;

	/// <summary>B-Bus destination register address ($21xx, only low byte stored).</summary>
	uint8_t DestAddress;

	/// <summary>True when this channel is actively performing DMA.</summary>
	bool DmaActive;

	/// <summary>Transfer direction: false=A-Bus→B-Bus (to PPU), true=B-Bus→A-Bus (from PPU).</summary>
	bool InvertDirection;

	/// <summary>A-Bus address adjustment: false=increment, true=decrement.</summary>
	bool Decrement;

	/// <summary>True to keep A-Bus address fixed (no increment/decrement).</summary>
	bool FixedTransfer;

	/// <summary>HDMA addressing mode: false=direct (table has data), true=indirect (table has pointers).</summary>
	bool HdmaIndirectAddressing;

	/// <summary>
	/// Transfer mode (0-7) controlling B-Bus register pattern:
	/// 0: 1 register  (write once)
	/// 1: 2 registers (write to p, p+1)
	/// 2: 2 registers (write to p twice)
	/// 3: 4 registers (write to p, p, p+1, p+1)
	/// 4: 4 registers (write to p, p+1, p+2, p+3)
	/// 5: 4 registers (write to p, p+1, p, p+1)
	/// 6: Same as mode 2
	/// 7: Same as mode 3
	/// </summary>
	uint8_t TransferMode;

	/// <summary>HDMA indirect mode: bank for indirect data.</summary>
	uint8_t HdmaBank;

	/// <summary>
	/// HDMA line counter and repeat flag.
	/// Bits 0-6: Lines remaining in current entry.
	/// Bit 7: Repeat flag (true=transfer every line, false=transfer once then decrement).
	/// </summary>
	uint8_t HdmaLineCounterAndRepeat;

	/// <summary>True when HDMA should perform a transfer this scanline.</summary>
	bool DoTransfer;

	/// <summary>True when HDMA has reached end-of-table (line count = 0).</summary>
	bool HdmaFinished;

	/// <summary>Unused bit in control register (preserved for accuracy).</summary>
	bool UnusedControlFlag;

	/// <summary>Unused DMA register value (open bus behavior).</summary>
	uint8_t UnusedRegister;
};

/// <summary>
/// Complete DMA controller state for all 8 channels.
/// </summary>
struct SnesDmaControllerState {
	/// <summary>Configuration for DMA channels 0-7.</summary>
	DmaChannelConfig Channel[8];

	/// <summary>Bit flags indicating which channels have HDMA enabled.</summary>
	uint8_t HdmaChannels;
};