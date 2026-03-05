#include "pch.h"
#include "Lynx/LynxCart.h"
#include "Lynx/LynxConsole.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Utilities/Serializer.h"

void LynxCart::Init(Emulator* emu, LynxConsole* console, const LynxCartInfo& info) {
	_emu = emu;
	_console = console;
	_state.Info = info;

	_romData = console->GetPrgRom();
	_romSize = console->GetPrgRomSize();

	// Calculate bank sizes from page counts
	// LNX header stores page sizes in 256-byte pages
	_bank0Size = static_cast<uint32_t>(info.PageSizeBank0) * 256;
	_bank1Size = static_cast<uint32_t>(info.PageSizeBank1) * 256;

	// Bank 0 is at the start of ROM, bank 1 follows
	_bank0Offset = 0;
	_bank1Offset = _bank0Size;

	// Validate sizes against ROM
	if (_bank0Size + _bank1Size > _romSize) {
		MessageManager::Log(std::format("Warning: Bank sizes ({} + {} = {}) exceed ROM size ({})",
			_bank0Size, _bank1Size, _bank0Size + _bank1Size, _romSize));
		// Adjust to fit
		if (_bank0Size > _romSize) {
			_bank0Size = _romSize;
		}
		_bank1Offset = _bank0Size;
		if (_bank1Offset + _bank1Size > _romSize) {
			_bank1Size = _romSize - _bank1Offset;
		}
	}

	// Initialize state
	_state.CurrentBank = 0;
	_state.ShiftRegister = 0;
	_state.AddressCounter = 0;
	_state.AddressShift = 0;
	_state.ShiftCount = 0;

	// Compute address bit count for the shift register protocol.
	// The boot ROM shifts in this many bits via SYSCTL1 to set the cart page.
	// Each bit is clocked on a falling edge of the strobe line.
	// addrBitCount = ceil(log2(bankSize)) — enough bits to address every byte.
	uint32_t maxSize = std::max(_bank0Size, _bank1Size);
	_addrBitCount = 0;
	if (maxSize > 0) {
		uint32_t tmp = maxSize - 1;
		while (tmp > 0) {
			_addrBitCount++;
			tmp >>= 1;
		}
	}
	if (_addrBitCount < 8) {
		_addrBitCount = 8; // Minimum 8 bits (256-byte page)
	}

	MessageManager::Log(std::format("Cart: Bank 0 = {} KB, Bank 1 = {} KB, AddrBits = {}",
		_bank0Size / 1024, _bank1Size / 1024, _addrBitCount));
}

uint8_t LynxCart::ReadData() {
	uint8_t value = PeekData();
	_state.AddressCounter++;
	return value;
}

void LynxCart::WriteData(uint8_t value) {
	// Write to cart bus — used by games with writable cart SRAM (homebrew/multicarts).
	// The EEPROM is accessed via Mikey I/O pins, NOT through this path.
	// On hardware, this drives the data bus toward the cart and strobes WE.
	// We write into SaveRAM if available, otherwise the write is ignored.
	uint8_t* saveRam = _console->GetSaveRam();
	uint32_t saveRamSize = _console->GetSaveRamSize();
	if (saveRam && saveRamSize > 0) {
		uint32_t addr = _state.AddressCounter;
		if (addr < saveRamSize) {
			saveRam[addr] = value;
		}
	}
	_state.AddressCounter++;
}

uint8_t LynxCart::PeekData() const {
	uint32_t addr = GetCurrentRomAddress();
	if (addr < _romSize) {
		return _romData[addr];
	}
	return 0xff; // Open bus
}

void LynxCart::SetAddressLow(uint8_t value) {
	_state.AddressCounter = (_state.AddressCounter & 0xff00) | value;
}

void LynxCart::SetAddressHigh(uint8_t value) {
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(value) << 8);
}

void LynxCart::WriteShiftRegister(uint8_t value) {
	// Legacy method — kept for test compatibility.
	_state.ShiftRegister = value;
}

void LynxCart::CartAddressWrite(bool data) {
	// Shift in one address bit (MSB first) from the SYSCTL1 bit-bang protocol.
	// Called by Mikey on each falling edge of SYSCTL1 strobe (bit 1).
	// After _addrBitCount bits, the accumulated value replaces AddressCounter.
	_state.ShiftCount++;
	_state.AddressShift <<= 1;
	if (data) {
		_state.AddressShift |= 1;
	}

	if (_state.ShiftCount >= _addrBitCount) {
		_state.AddressCounter = _state.AddressShift;
		_state.ShiftCount = 0;
		_state.AddressShift = 0;
	}
}

void LynxCart::SetBank0Page(uint8_t page) {
	_state.CurrentBank = 0;
	// Page counter selects which 256-byte page within bank 0.
	// This intentionally overwrites the high byte of AddressCounter —
	// on real hardware, Suzy writes the page register and then sets
	// CART0/CART1 to start sequential reads, so any previous address
	// state is irrelevant once a new page is selected.
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(page) << 8);
}

void LynxCart::SetBank1Page(uint8_t page) {
	_state.CurrentBank = 1;
	// Same page-select logic as SetBank0Page — see comment above.
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(page) << 8);
}

void LynxCart::SelectBank(uint8_t bank) {
	_state.CurrentBank = bank;
}

uint32_t LynxCart::GetCurrentRomAddress() const {
	uint32_t bankOffset = (_state.CurrentBank == 0) ? _bank0Offset : _bank1Offset;
	uint32_t bankSize = (_state.CurrentBank == 0) ? _bank0Size : _bank1Size;

	// Address within bank wraps around bank size
	uint32_t addr = _state.AddressCounter;
	if (bankSize > 0) {
		addr %= bankSize;
	}

	return bankOffset + addr;
}

void LynxCart::Serialize(Serializer& s) {
	// CartInfo (name, sizes, EEPROM type) is derived from the LNX header
	// and set once in Init() — no need to serialize it since it's
	// reconstructed from the ROM on load.
	SV(_state.CurrentBank);
	SV(_state.ShiftRegister);
	SV(_state.AddressCounter);
	SV(_state.AddressShift);
	SV(_state.ShiftCount);
}
