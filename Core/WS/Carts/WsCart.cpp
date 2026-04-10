#include "pch.h"
#include "WS/Carts/WsCart.h"
#include "WS/WsMemoryManager.h"
#include "WS/WsEeprom.h"
#include "WS/WsRtc.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

void WsCart::Map(uint32_t start, uint32_t end, MemoryType type, uint32_t offset, bool readonly) {
	_memoryManager->Map(start, end, type, offset, readonly);
}

void WsCart::Unmap(uint32_t start, uint32_t end) {
	_memoryManager->Unmap(start, end);
}

WsCart::WsCart() {
	_state.SelectedBanks[0] = 0xFF;
	_state.SelectedBanks[1] = 0xFF;
	_state.SelectedBanks[2] = 0xFF;
	_state.SelectedBanks[3] = 0xFF;
}

WsCart::~WsCart() {
}

void WsCart::Init(Emulator* emu, WsMemoryManager* memoryManager, WsEeprom* cartEeprom, uint8_t* prgRom, uint32_t prgRomSize, bool hasFlash, bool hasRtc) {
	_emu = emu;
	_memoryManager = memoryManager;
	_cartEeprom = cartEeprom;
	_prgRom = prgRom;
	_prgRomSize = prgRomSize;
	_state.HasFlash = hasFlash;

	if (hasRtc) {
		_rtc = std::make_unique<WsRtc>(emu);
	}
}

void WsCart::RefreshMappings() {
	Map(0x10000, 0x1FFFF, MemoryType::WsCartRam, _state.SelectedBanks[1] * 0x10000, false);
	Map(0x20000, 0x2FFFF, MemoryType::WsPrgRom, _state.SelectedBanks[2] * 0x10000, true);
	Map(0x30000, 0x3FFFF, MemoryType::WsPrgRom, _state.SelectedBanks[3] * 0x10000, true);
	Map(0x40000, 0xFFFFF, MemoryType::WsPrgRom, _state.SelectedBanks[0] * 0x100000 + 0x40000, true);
}

uint8_t WsCart::ReadPort(uint16_t port) {
	if (port < 0xC4) {
		return _state.SelectedBanks[port - 0xC0];
	} else if (port < 0xC9 && _cartEeprom) {
		return _cartEeprom->ReadPort(port - 0xC4);
	} else if (port == 0xCA && _rtc) {
		return _rtc->Read();
	}

	return _memoryManager->GetUnmappedPort();
}

void WsCart::WritePort(uint16_t port, uint8_t value) {
	if (port < 0xC4) {
		_state.SelectedBanks[port - 0xC0] = value;
		_memoryManager->RefreshMappings();
	} else if (port < 0xC9 && _cartEeprom) {
		_cartEeprom->WritePort(port - 0xC4, value);
	} else if (port == 0xCA && _rtc) {
		_rtc->Write(value);
	}
}

uint8_t WsCart::FlashRead(uint32_t addr) {
	if (_state.FlashSoftwareId) {
		// Software ID mode: return manufacturer/device ID
		uint32_t offset = addr & 0x01;
		if (offset == 0) {
			return 0xbf; // SST manufacturer ID
		} else {
			return 0xb5; // SST39VF040 device ID (512KB)
		}
	}

	// Normal read: return ROM data
	if (addr < _prgRomSize) {
		return _prgRom[addr];
	}
	return 0xff;
}

void WsCart::FlashWrite(uint32_t addr, uint8_t value) {
	if (_state.FlashMode == WsFlashMode::WriteByte) {
		// Program a single byte (can only clear bits, not set)
		if (addr < _prgRomSize) {
			uint8_t newValue = _prgRom[addr] & value;
			if (newValue != _prgRom[addr]) {
				_prgRom[addr] = newValue;
				_state.FlashDirty = true;
			}
		}
		_state.FlashMode = WsFlashMode::Idle;
		_state.FlashCycle = 0;
		return;
	}

	if (_state.FlashMode == WsFlashMode::Erase) {
		if (_state.FlashCycle == 3) {
			if ((addr & 0xffff) == 0x5555 && value == 0xaa) {
				_state.FlashCycle++;
			} else {
				_state.FlashMode = WsFlashMode::Idle;
				_state.FlashCycle = 0;
			}
		} else if (_state.FlashCycle == 4) {
			if ((addr & 0xffff) == 0x2aaa && value == 0x55) {
				_state.FlashCycle++;
			} else {
				_state.FlashMode = WsFlashMode::Idle;
				_state.FlashCycle = 0;
			}
		} else if (_state.FlashCycle == 5) {
			if ((addr & 0xffff) == 0x5555 && value == 0x10) {
				// Chip erase
				memset(_prgRom, 0xff, _prgRomSize);
				_state.FlashDirty = true;
			} else if (value == 0x30) {
				// Sector erase (4KB)
				uint32_t sectorBase = addr & ~0xfff;
				if (sectorBase + 0x1000 <= _prgRomSize) {
					memset(_prgRom + sectorBase, 0xff, 0x1000);
					_state.FlashDirty = true;
				}
			}
			_state.FlashMode = WsFlashMode::Idle;
			_state.FlashCycle = 0;
		}
		return;
	}

	// Idle — process command sequence
	if (_state.FlashCycle == 0) {
		if ((addr & 0xffff) == 0x5555 && value == 0xaa) {
			_state.FlashCycle++;
		} else if (value == 0xf0) {
			// Exit Software ID
			_state.FlashSoftwareId = false;
			_state.FlashCycle = 0;
		}
	} else if (_state.FlashCycle == 1) {
		if ((addr & 0xffff) == 0x2aaa && value == 0x55) {
			_state.FlashCycle++;
		} else {
			_state.FlashCycle = 0;
		}
	} else if (_state.FlashCycle == 2) {
		if ((addr & 0xffff) == 0x5555) {
			switch (value) {
				case 0x80:
					_state.FlashMode = WsFlashMode::Erase;
					_state.FlashCycle = 3;
					break;
				case 0x90:
					_state.FlashSoftwareId = true;
					_state.FlashCycle = 0;
					break;
				case 0xa0:
					_state.FlashMode = WsFlashMode::WriteByte;
					_state.FlashCycle = 0;
					break;
				case 0xf0:
					_state.FlashSoftwareId = false;
					_state.FlashCycle = 0;
					break;
				default:
					_state.FlashCycle = 0;
					break;
			}
		} else {
			_state.FlashCycle = 0;
		}
	}
}

void WsCart::Serialize(Serializer& s) {
	SVArray(_state.SelectedBanks, 4);
	SV(_state.FlashMode);
	SV(_state.FlashCycle);
	SV(_state.FlashSoftwareId);
	SV(_state.HasFlash);
	SV(_state.FlashDirty);

	if (_rtc) {
		SV(_rtc);
	}
}
