#include "pch.h"
#include "Lynx/LynxEeprom.h"
#include "Lynx/LynxConsole.h"
#include "Shared/Emulator.h"
#include "Shared/BatteryManager.h"
#include "Utilities/Serializer.h"

LynxEeprom::LynxEeprom(Emulator* emu, LynxConsole* console)
	: _emu(emu), _console(console) {
}

void LynxEeprom::Init(LynxEepromType type) {
	_state = {};
	_state.Type = type;
	_state.State = LynxEepromState::Idle;
	_state.WriteEnabled = false;
	_state.DataOut = true; // DO idles high

	// Allocate storage based on chip type
	switch (type) {
		case LynxEepromType::Eeprom93c46:
			_dataSize = 128;   // 64 × 16-bit words
			break;
		case LynxEepromType::Eeprom93c56:
			_dataSize = 256;   // 128 × 16-bit words
			break;
		case LynxEepromType::Eeprom93c66:
			_dataSize = 512;   // 256 × 16-bit words
			break;
		case LynxEepromType::Eeprom93c76:
			_dataSize = 1024;  // 512 × 16-bit words
			break;
		case LynxEepromType::Eeprom93c86:
			_dataSize = 2048;  // 1024 × 16-bit words
			break;
		default:
			_dataSize = 0;
			return;
	}

	_data = std::make_unique<uint8_t[]>(_dataSize);
	std::fill_n(_data.get(), _dataSize, uint8_t{0xff}); // EEPROM erased state = all 1s
}

uint8_t LynxEeprom::GetAddressBits() const {
	switch (_state.Type) {
		case LynxEepromType::Eeprom93c46: return 6;
		case LynxEepromType::Eeprom93c56: return 7;
		case LynxEepromType::Eeprom93c66: return 8;
		case LynxEepromType::Eeprom93c76: return 9;
		case LynxEepromType::Eeprom93c86: return 10;
		default: return 0;
	}
}

uint16_t LynxEeprom::GetWordCount() const {
	return static_cast<uint16_t>(_dataSize / 2);
}

uint16_t LynxEeprom::ReadWord(uint16_t wordAddr) const {
	if (!_data || wordAddr >= GetWordCount()) {
		return 0xffff;
	}
	uint32_t byteAddr = wordAddr * 2;
	return _data[byteAddr] | (_data[byteAddr + 1] << 8);
}

void LynxEeprom::WriteWord(uint16_t wordAddr, uint16_t value) {
	if (!_data || wordAddr >= GetWordCount() || !_state.WriteEnabled) {
		return;
	}
	uint32_t byteAddr = wordAddr * 2;
	_data[byteAddr] = static_cast<uint8_t>(value);
	_data[byteAddr + 1] = static_cast<uint8_t>(value >> 8);
}

void LynxEeprom::SetChipSelect(bool active) {
	if (active && !_state.CsActive) {
		// Rising edge: reset state machine
		_state.State = LynxEepromState::ReceivingOpcode;
		_state.Opcode = 0;
		_state.Address = 0;
		_state.DataBuffer = 0;
		_state.BitCount = 0;
		_state.DataOut = true; // DO high (idle/ready)
	} else if (!active && _state.CsActive) {
		// Falling edge: end command, return to idle
		_state.State = LynxEepromState::Idle;
	}
	_state.CsActive = active;
}

bool LynxEeprom::ClockData(bool dataIn) {
	if (!_state.CsActive || _state.Type == LynxEepromType::None) {
		return _state.DataOut;
	}

	switch (_state.State) {
		case LynxEepromState::ReceivingOpcode: {
			// Shift in the start bit + 2-bit opcode (3 bits total)
			_state.Opcode = (_state.Opcode << 1) | (dataIn ? 1 : 0);
			_state.BitCount++;

			if (_state.BitCount == 3) {
				// Start bit should be 1, opcode is bits 1-0
				// If start bit was 0, this is invalid — ignore
				if ((_state.Opcode & 0x04) == 0) {
					_state.State = LynxEepromState::Idle;
					break;
				}
				_state.Opcode &= 0x03; // Keep only 2-bit opcode
				_state.BitCount = 0;
				_state.State = LynxEepromState::ReceivingAddress;
			}
			break;
		}

		case LynxEepromState::ReceivingAddress: {
			uint8_t addrBits = GetAddressBits();
			_state.Address = (_state.Address << 1) | (dataIn ? 1 : 0);
			_state.BitCount++;

			if (_state.BitCount >= addrBits) {
				_state.BitCount = 0;
				ExecuteCommand();
			}
			break;
		}

		case LynxEepromState::ReceivingData: {
			// Receiving 16 data bits for WRITE or WRAL
			_state.DataBuffer = (_state.DataBuffer << 1) | (dataIn ? 1 : 0);
			_state.BitCount++;

			if (_state.BitCount >= 16) {
				uint8_t opcode = static_cast<uint8_t>(_state.Opcode);
				if (opcode == 0x01) {
					// WRITE: write word at address
					WriteWord(_state.Address, _state.DataBuffer);
				} else if (opcode == 0x00) {
					// WRAL: write all words
					uint16_t wordCount = GetWordCount();
					for (uint16_t i = 0; i < wordCount; i++) {
						WriteWord(i, _state.DataBuffer);
					}
				}
				_state.DataOut = true; // Ready
				_state.State = LynxEepromState::Idle;
			}
			break;
		}

		case LynxEepromState::SendingData: {
			// Output 16 data bits MSB first on DO
			_state.DataOut = (_state.DataBuffer & 0x8000) != 0;
			_state.DataBuffer <<= 1;
			_state.BitCount++;

			if (_state.BitCount >= 16) {
				_state.State = LynxEepromState::Idle;
			}
			break;
		}

		default:
			break;
	}

	return _state.DataOut;
}

void LynxEeprom::ExecuteCommand() {
	uint8_t opcode = static_cast<uint8_t>(_state.Opcode);
	uint8_t addrBits = GetAddressBits();
	uint16_t addrMask = (1 << addrBits) - 1;
	uint16_t addr = _state.Address & addrMask;

	switch (opcode) {
		case 0x02: {
			// READ: load word, enter sending state
			_state.DataBuffer = ReadWord(addr);
			_state.BitCount = 0;
			_state.DataOut = false; // Initial 0 bit before data (dummy bit)
			_state.State = LynxEepromState::SendingData;
			break;
		}

		case 0x01: {
			// WRITE: need 16 more data bits
			_state.DataBuffer = 0;
			_state.BitCount = 0;
			_state.State = LynxEepromState::ReceivingData;
			break;
		}

		case 0x03: {
			// ERASE: set word to 0xFFFF
			if (_state.WriteEnabled) {
				WriteWord(addr, 0xffff);
			}
			_state.DataOut = true; // Ready
			_state.State = LynxEepromState::Idle;
			break;
		}

		case 0x00: {
			// Extended commands: decode from top 2 bits of address
			uint8_t extCmd = (addr >> (addrBits - 2)) & 0x03;
			switch (extCmd) {
				case 0x00:
					// EWDS — write disable
					_state.WriteEnabled = false;
					_state.State = LynxEepromState::Idle;
					break;

				case 0x01:
					// WRAL — write all: need 16 data bits
					_state.DataBuffer = 0;
					_state.BitCount = 0;
					_state.State = LynxEepromState::ReceivingData;
					break;

				case 0x02: {
					// ERAL — erase all
					if (_state.WriteEnabled) {
						uint16_t wordCount = GetWordCount();
						for (uint16_t i = 0; i < wordCount; i++) {
							WriteWord(i, 0xffff);
						}
					}
					_state.State = LynxEepromState::Idle;
					break;
				}

				case 0x03:
					// EWEN — write enable
					_state.WriteEnabled = true;
					_state.State = LynxEepromState::Idle;
					break;
			}
			break;
		}
	}
}

void LynxEeprom::LoadBattery() {
	if (_data && _dataSize > 0) {
		_emu->GetBatteryManager()->LoadBattery(".eeprom", std::span<uint8_t>(_data.get(), _dataSize));
	}
}

void LynxEeprom::SaveBattery() {
	if (_data && _dataSize > 0) {
		_emu->GetBatteryManager()->SaveBattery(".eeprom", std::span<const uint8_t>(_data.get(), _dataSize));
	}
}

void LynxEeprom::Serialize(Serializer& s) {
	SV(_state.Type);
	SV(_state.State);
	SV(_state.Opcode);
	SV(_state.Address);
	SV(_state.DataBuffer);
	SV(_state.BitCount);
	SV(_state.WriteEnabled);
	SV(_state.CsActive);
	SV(_state.ClockState);
	SV(_state.DataOut);

	if (_data && _dataSize > 0) {
		SVArray(_data.get(), _dataSize);
	}
}
