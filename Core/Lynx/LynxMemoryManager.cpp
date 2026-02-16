#include "pch.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxConsole.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Utilities/Serializer.h"

// Component includes
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxSuzy.h"

void LynxMemoryManager::Init(Emulator* emu, LynxConsole* console, uint8_t* workRam, uint8_t* bootRom, uint32_t bootRomSize) {
	_emu = emu;
	_console = console;

	_workRam = workRam;
	_workRamSize = LynxConstants::WorkRamSize;
	_bootRom = bootRom;
	_bootRomSize = bootRomSize;

	// Initial MAPCTL state: all overlays visible (bits 0-3 = 0)
	UpdateMapctl(0x00);
}

void LynxMemoryManager::UpdateMapctl(uint8_t value) {
	_state.Mapctl = value;
	// Active-low: bit = 0 means overlay IS visible
	_state.SuzySpaceVisible = !(value & 0x01);
	_state.MikeySpaceVisible = !(value & 0x02);
	_state.RomSpaceVisible = !(value & 0x04);
	_state.VectorSpaceVisible = !(value & 0x08);
}

uint8_t LynxMemoryManager::Read(uint16_t addr, MemoryOperationType opType) {
	uint8_t value = 0;

	if (addr >= 0xfc00) {
		// High memory — check overlays in priority order
		if (addr == 0xfff9) {
			// MAPCTL register — always readable
			value = _state.Mapctl;
		} else if (_state.SuzySpaceVisible && addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd) {
			// Suzy register read
			value = _suzy ? _suzy->ReadRegister(addr & 0xff) : 0;
		} else if (_state.MikeySpaceVisible && addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd) {
			// Mikey register read
			value = _mikey ? _mikey->ReadRegister(addr & 0xff) : 0;
		} else if (_state.VectorSpaceVisible && addr >= 0xfffa) {
			// Vector space — read from boot ROM
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = addr - LynxConstants::BootRomBase;
				if (romOffset < _bootRomSize) {
					value = _bootRom[romOffset];
				}
			} else {
				// No boot ROM — fall through to RAM
				value = _workRam[addr];
			}
		} else if (_state.RomSpaceVisible && addr >= LynxConstants::BootRomBase && addr <= 0xfff7) {
			// ROM space — read from boot ROM
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = addr - LynxConstants::BootRomBase;
				if (romOffset < _bootRomSize) {
					value = _bootRom[romOffset];
				}
			} else {
				value = _workRam[addr];
			}
		} else {
			// Overlay not active — read from RAM
			value = _workRam[addr];
		}
	} else {
		// $0000-$FBFF — always RAM
		value = _workRam[addr];
	}

	_emu->ProcessMemoryRead<CpuType::Lynx>(addr, value, opType);
	return value;
}

void LynxMemoryManager::Write(uint16_t addr, uint8_t value, MemoryOperationType opType) {
	if (!_emu->ProcessMemoryWrite<CpuType::Lynx>(addr, value, opType)) {
		return;
	}

	if (addr >= 0xfc00) {
		if (addr == 0xfff9) {
			// MAPCTL register write
			UpdateMapctl(value);
			// Also write to RAM (MAPCTL is in RAM too)
			_workRam[addr] = value;
			return;
		}

		if (_state.SuzySpaceVisible && addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd) {
			// Suzy register write
			if (_suzy) _suzy->WriteRegister(addr & 0xff, value);
			return;
		}

		if (_state.MikeySpaceVisible && addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd) {
			// Mikey register write
			if (_mikey) _mikey->WriteRegister(addr & 0xff, value);
			return;
		}

		if (_state.RomSpaceVisible && addr >= LynxConstants::BootRomBase) {
			// ROM space — writes are ignored (read-only)
			return;
		}
	}

	// Write to RAM (covers $0000-$FBFF and any overlay-disabled high areas)
	_workRam[addr] = value;
}

uint8_t LynxMemoryManager::DebugRead(uint16_t addr) {
	if (addr >= 0xfc00) {
		if (addr == 0xfff9) {
			return _state.Mapctl;
		}

		if (_state.SuzySpaceVisible && addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd) {
			return _suzy ? _suzy->ReadRegister(addr & 0xff) : 0;
		}

		if (_state.MikeySpaceVisible && addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd) {
			return _mikey ? _mikey->ReadRegister(addr & 0xff) : 0;
		}

		if (_state.VectorSpaceVisible && addr >= 0xfffa) {
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = addr - LynxConstants::BootRomBase;
				if (romOffset < _bootRomSize) {
					return _bootRom[romOffset];
				}
			}
			return _workRam[addr];
		}

		if (_state.RomSpaceVisible && addr >= LynxConstants::BootRomBase && addr <= 0xfff7) {
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = addr - LynxConstants::BootRomBase;
				if (romOffset < _bootRomSize) {
					return _bootRom[romOffset];
				}
			}
			return _workRam[addr];
		}
	}

	return _workRam[addr];
}

void LynxMemoryManager::DebugWrite(uint16_t addr, uint8_t value) {
	if (addr >= 0xfc00) {
		if (addr == 0xfff9) {
			UpdateMapctl(value);
		}
		// For debug, we allow writing to RAM even in overlay regions
	}
	_workRam[addr] = value;
}

AddressInfo LynxMemoryManager::GetAbsoluteAddress(uint16_t relAddr) {
	if (relAddr >= 0xfc00) {
		if (relAddr == 0xfff9) {
			// MAPCTL — part of RAM
			return { relAddr, MemoryType::LynxWorkRam };
		}

		if (_state.SuzySpaceVisible && relAddr >= LynxConstants::SuzyBase && relAddr <= LynxConstants::SuzyEnd) {
			// Suzy registers — no absolute address
			return { -1, MemoryType::None };
		}

		if (_state.MikeySpaceVisible && relAddr >= LynxConstants::MikeyBase && relAddr <= LynxConstants::MikeyEnd) {
			// Mikey registers — no absolute address
			return { -1, MemoryType::None };
		}

		if (_state.VectorSpaceVisible && relAddr >= 0xfffa) {
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = relAddr - LynxConstants::BootRomBase;
				return { (int32_t)romOffset, MemoryType::LynxBootRom };
			}
		}

		if (_state.RomSpaceVisible && relAddr >= LynxConstants::BootRomBase && relAddr <= 0xfff7) {
			if (_bootRom && _bootRomSize > 0) {
				uint16_t romOffset = relAddr - LynxConstants::BootRomBase;
				return { (int32_t)romOffset, MemoryType::LynxBootRom };
			}
		}
	}

	// Default: RAM
	return { relAddr, MemoryType::LynxWorkRam };
}

int32_t LynxMemoryManager::GetRelativeAddress(AddressInfo& absAddress) {
	switch (absAddress.Type) {
		case MemoryType::LynxWorkRam:
			return absAddress.Address & 0xffff;

		case MemoryType::LynxBootRom: {
			// Boot ROM is mapped at $FE00-$FFFF when ROM overlay is active
			if (_state.RomSpaceVisible || _state.VectorSpaceVisible) {
				return LynxConstants::BootRomBase + absAddress.Address;
			}
			return -1;
		}

		case MemoryType::LynxPrgRom:
			// Cart ROM is accessed through Suzy, not directly memory-mapped
			return -1;

		case MemoryType::LynxSaveRam:
			// EEPROM is accessed through Suzy registers
			return -1;

		default:
			return -1;
	}
}

void LynxMemoryManager::Serialize(Serializer& s) {
	SV(_state.Mapctl);
	SV(_state.SuzySpaceVisible);
	SV(_state.MikeySpaceVisible);
	SV(_state.RomSpaceVisible);
	SV(_state.VectorSpaceVisible);
	SV(_state.BootRomActive);

	if (!s.IsSaving()) {
		// Rebuild pointers from console on deserialization
		_workRam = _console->GetWorkRam();
		_workRamSize = _console->GetWorkRamSize();
	}
}
