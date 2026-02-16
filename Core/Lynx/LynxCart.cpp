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
	_bank0Size = (uint32_t)info.PageSizeBank0 * 256;
	_bank1Size = (uint32_t)info.PageSizeBank1 * 256;

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

	MessageManager::Log(std::format("Cart: Bank 0 = {} KB, Bank 1 = {} KB",
		_bank0Size / 1024, _bank1Size / 1024));
}

uint8_t LynxCart::ReadData() {
	uint8_t value = PeekData();
	_state.AddressCounter++;
	return value;
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
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | ((uint32_t)value << 8);
}

void LynxCart::WriteShiftRegister(uint8_t value) {
	_state.ShiftRegister = value;
}

void LynxCart::SetBank0Page(uint8_t page) {
	_state.CurrentBank = 0;
	// Page counter selects which page within bank 0
	// Address = bank0Offset + (page * pageSize) + addressCounter
	// The page multiplied by some granularity determines offset within the bank
}

void LynxCart::SetBank1Page(uint8_t page) {
	_state.CurrentBank = 1;
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
	SV(_state.CurrentBank);
	SV(_state.ShiftRegister);
	SV(_state.AddressCounter);
}
