#pragma once
#include "pch.h"
#include "Lynx/LynxTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class LynxConsole;

/// <summary>
/// Lynx cartridge emulation — handles ROM data, bank switching,
/// and LNX format metadata.
///
/// The Lynx cart has two banks with independent page counters.
/// CART0/CART1 accent lines from Suzy select which bank is active.
/// Data is read sequentially via CARTDATA register; the address counter
/// auto-increments each read.
///
/// Bank switching is driven by Suzy registers:
///   $FCB2 — CART0 page counter (bank 0)
///   $FCB3 — CART1 page counter (bank 1)
///   $FCA0-$FCA1 — Cart address (low/high)
///
/// ROM is organized in pages. Page size varies per bank (from LNX header).
/// </summary>
class LynxCart final : public ISerializable {
private:
	Emulator* _emu = nullptr;
	LynxConsole* _console = nullptr;

	uint8_t* _romData = nullptr;
	uint32_t _romSize = 0;

	LynxCartState _state = {};

	// Bank page sizes in bytes (converted from 256-byte page count)
	uint32_t _bank0Size = 0;
	uint32_t _bank1Size = 0;
	uint32_t _bank0Offset = 0; // Offset into ROM where bank 0 data starts
	uint32_t _bank1Offset = 0; // Offset into ROM where bank 1 data starts

	/// <summary>Number of address bits for the shift register protocol.
	/// Computed from ROM size: ceil(log2(romSize)). After this many bits
	/// have been shifted in via SYSCTL1, the accumulated value becomes
	/// the new AddressCounter.</summary>
	uint8_t _addrBitCount = 0;

public:
	LynxCart() = default;

	void Init(Emulator* emu, LynxConsole* console, const LynxCartInfo& info);

	/// <summary>Get cart state for debugger/serializer</summary>
	[[nodiscard]] LynxCartState& GetState() { return _state; }

	/// <summary>Get cart info from LNX header</summary>
	[[nodiscard]] const LynxCartInfo& GetInfo() const { return _state.Info; }

	/// <summary>Read the next byte from cart (CARTDATA access)</summary>
	[[nodiscard]] uint8_t ReadData();

	/// <summary>Write a byte to cart bus (SRAM-equipped carts only, auto-increments)</summary>
	void WriteData(uint8_t value);

	/// <summary>Peek at current cart data byte without advancing counter</summary>
	[[nodiscard]] uint8_t PeekData() const;

	/// <summary>Shift one address bit into the cart via the SYSCTL1 bit-bang protocol.
	/// Called by Mikey on falling edge of SYSCTL1 strobe (bit 1).
	/// After _addrBitCount bits, the accumulated value becomes the new AddressCounter.</summary>
	void CartAddressWrite(bool data);

	/// <summary>Set cart address counter low byte</summary>
	void SetAddressLow(uint8_t value);

	/// <summary>Set cart address counter high byte</summary>
	void SetAddressHigh(uint8_t value);

	/// <summary>Write to cart shift register (for bank selection)</summary>
	void WriteShiftRegister(uint8_t value);

	/// <summary>Set page counter for bank 0</summary>
	void SetBank0Page(uint8_t page);

	/// <summary>Set page counter for bank 1</summary>
	void SetBank1Page(uint8_t page);

	/// <summary>Select active bank (0 or 1, driven by CART0/CART1 lines)</summary>
	void SelectBank(uint8_t bank);

	void Serialize(Serializer& s) override;

private:
	/// <summary>Get the absolute ROM address for the current state</summary>
	[[nodiscard]] uint32_t GetCurrentRomAddress() const;
};
