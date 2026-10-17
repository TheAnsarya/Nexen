#pragma once
#include "pch.h"
#include "NES/INesMemoryHandler.h"

/// <summary>
/// Handler for NES internal (work) RAM.
/// </summary>
/// <typeparam name="Mask">Address mask for RAM size (0x7FF for 2KB).</typeparam>
/// <remarks>
/// **NES Internal RAM:**
/// The NES has 2KB of internal work RAM at $0000-$07FF.
/// This RAM is mirrored 4 times across $0000-$1FFF.
///
/// **Memory Layout:**
/// - $0000-$00FF: Zero page (fast access via zp addressing)
/// - $0100-$01FF: Stack (6502 stack grows downward)
/// - $0200-$07FF: General purpose RAM
///
/// **Mirroring:**
/// - $0000-$07FF: Actual RAM
/// - $0800-$0FFF: Mirror of $0000-$07FF
/// - $1000-$17FF: Mirror of $0000-$07FF
/// - $1800-$1FFF: Mirror of $0000-$07FF
///
/// **Usage Patterns:**
/// - Zero page: Variables, pointers, temporary storage
/// - Stack page: Return addresses, saved registers
/// - Upper RAM: Sprite data (often $0200), buffers, game state
/// </remarks>
template <size_t Mask>
class InternalRamHandler : public INesMemoryHandler {
private:
	/// <summary>Pointer to RAM array (allocated externally).</summary>
	uint8_t* _internalRam = nullptr;

public:
	/// <summary>
	/// Sets the internal RAM pointer.
	/// </summary>
	/// <param name="internalRam">Pointer to 2KB RAM array.</param>
	void SetInternalRam(uint8_t* internalRam) {
		_internalRam = internalRam;
	}

	/// <summary>
	/// Registers handler for $0000-$1FFF (all RAM mirrors).
	/// </summary>
	/// <param name="ranges">Range collection to add to.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.SetAllowOverride();
		ranges.AddHandler(MemoryOperation::Any, 0, 0x1FFF);
	}

	/// <summary>
	/// Reads from internal RAM (applies mirror mask).
	/// </summary>
	/// <param name="addr">Address ($0000-$1FFF).</param>
	/// <returns>RAM value.</returns>
	uint8_t ReadRam(uint16_t addr) override {
		return _internalRam[addr & Mask];
	}

	/// <summary>
	/// Peeks internal RAM (same as read, no side effects).
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>RAM value.</returns>
	uint8_t PeekRam(uint16_t addr) override {
		return ReadRam(addr);
	}

	/// <summary>
	/// Writes to internal RAM (applies mirror mask).
	/// </summary>
	/// <param name="addr">Address ($0000-$1FFF).</param>
	/// <param name="value">Value to write.</param>
	void WriteRam(uint16_t addr, uint8_t value) override {
		_internalRam[addr & Mask] = value;
	}
};
