#include "pch.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisPsg.h"
#include "Shared/Emulator.h"
#include "Shared/BatteryManager.h"
#include "Utilities/Serializer.h"

namespace {
	__forceinline uint32_t ReadBe32(const vector<uint8_t>& data, size_t offset) {
		return ((uint32_t)data[offset] << 24)
			| ((uint32_t)data[offset + 1] << 16)
			| ((uint32_t)data[offset + 2] << 8)
			| (uint32_t)data[offset + 3];
	}
}

GenesisMemoryManager::GenesisMemoryManager() {
}

GenesisMemoryManager::~GenesisMemoryManager() {
}

void GenesisMemoryManager::Init(Emulator* emu, GenesisConsole* console, vector<uint8_t>& romData, GenesisVdp* vdp, GenesisControlManager* controlManager, GenesisPsg* psg) {
	_emu = emu;
	_console = console;
	_vdp = vdp;
	_controlManager = controlManager;
	_psg = psg;

	// Register and allocate ROM
	_prgRomSize = (uint32_t)romData.size();
	_prgRom = new uint8_t[_prgRomSize];
	memcpy(_prgRom, romData.data(), _prgRomSize);
	_emu->RegisterMemory(MemoryType::GenesisPrgRom, _prgRom, _prgRomSize);

	// Register and allocate work RAM
	_workRam = new uint8_t[WorkRamSize];
	memset(_workRam, 0, WorkRamSize);
	_emu->RegisterMemory(MemoryType::GenesisWorkRam, _workRam, WorkRamSize);

	// Register and allocate Z80 RAM
	_z80Ram = new uint8_t[Z80RamSize];
	memset(_z80Ram, 0, Z80RamSize);

	// I/O defaults
	memset(&_ioState, 0, sizeof(_ioState));
	_ioState.CtrlPort[0] = 0;
	_ioState.CtrlPort[1] = 0;
	_ioState.CtrlPort[2] = 0;

	_z80BusRequest = false;
	_z80Reset = true;

	_hasSram = false;
	_sramStart = 0;
	_sramEnd = 0;
	_sramEvenBytes = true;
	_sramOddBytes = true;
	_saveRam = nullptr;
	_saveRamSize = 0;

	if (romData.size() >= 0x1BC && romData[0x1B0] == 'R' && romData[0x1B1] == 'A') {
		uint32_t start = ReadBe32(romData, 0x1B4) & 0xFFFFFF;
		uint32_t end = ReadBe32(romData, 0x1B8) & 0xFFFFFF;
		uint8_t type = romData[0x1B2];

		if (end >= start) {
			_sramStart = start;
			_sramEnd = end;
			_sramEvenBytes = true;
			_sramOddBytes = true;

			if (type == 0xB0) {
				_sramOddBytes = false;
			} else if (type == 0xB8) {
				_sramEvenBytes = false;
			}

			if (!_sramEvenBytes && !_sramOddBytes) {
				_sramEvenBytes = true;
				_sramOddBytes = true;
			}

			if (_sramEvenBytes && _sramOddBytes) {
				_saveRamSize = (_sramEnd - _sramStart) + 1;
			} else {
				_saveRamSize = ((_sramEnd - _sramStart) >> 1) + 1;
			}

			if (_saveRamSize > 0) {
				_saveRam = new uint8_t[_saveRamSize];
				memset(_saveRam, 0xFF, _saveRamSize);
				_hasSram = true;
			}
		}
	}
}

bool GenesisMemoryManager::IsSramAddress(uint32_t addr) const {
	return HasSaveRam() && addr >= _sramStart && addr <= _sramEnd;
}

bool GenesisMemoryManager::TryGetSramOffset(uint32_t addr, uint32_t& offset) const {
	if (!IsSramAddress(addr)) {
		return false;
	}

	if ((addr & 0x01) == 0) {
		if (!_sramEvenBytes) {
			return false;
		}
	} else if (!_sramOddBytes) {
		return false;
	}

	if (_sramEvenBytes && _sramOddBytes) {
		offset = addr - _sramStart;
	} else {
		offset = (addr - _sramStart) >> 1;
	}

	return offset < _saveRamSize;
}

// =============================================
// Genesis 68000 memory map (24-bit, big-endian)
// =============================================
// $000000-$3FFFFF : Cartridge ROM (up to 4MB)
// $400000-$7FFFFF : Reserved / expansion
// $A00000-$A0FFFF : Z80 address space (8KB RAM + mirrored)
// $A10000-$A1001F : I/O registers
// $A11100         : Z80 bus request
// $A11200         : Z80 reset
// $C00000-$C0001F : VDP ports
// $FF0000-$FFFFFF : 68000 work RAM (64KB, mirrored)

uint8_t GenesisMemoryManager::Read8(uint32_t addr) {
	addr &= 0xFFFFFF;
	uint32_t sramOffset = 0;

	if (TryGetSramOffset(addr, sramOffset)) {
		uint8_t value = _saveRam[sramOffset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
		_openBus = value;
		return value;
	}

	if (addr < 0x400000) {
		// Cartridge ROM
		if (addr < _prgRomSize) {
			uint8_t value = _prgRom[addr];
			_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
			_openBus = value;
			return value;
		}
		return _openBus;
	}

	if (addr >= 0xFF0000) {
		// Work RAM
		uint32_t offset = addr & 0xFFFF;
		uint8_t value = _workRam[offset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
		_openBus = value;
		return value;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		// VDP ports (byte access from word port)
		uint16_t word = ReadVdpPort(addr);
		if (addr & 1) return (uint8_t)(word & 0xFF);
		return (uint8_t)(word >> 8);
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		// Z80 address space
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			return _z80Ram[z80Addr];
		}
		return _openBus;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		return ReadIo(addr);
	}

	if (addr == 0xA11100) {
		// Z80 bus request — bit 0 indicates bus granted
		return _z80BusRequest ? 0x00 : 0x01;
	}

	return _openBus;
}

uint16_t GenesisMemoryManager::Read16(uint32_t addr) {
	addr &= 0xFFFFFE;
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) {
		uint8_t hi = Read8(addr);
		uint8_t lo = Read8(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr < 0x400000) {
		if (addr + 1 < _prgRomSize) {
			uint16_t value = ((uint16_t)_prgRom[addr] << 8) | _prgRom[addr + 1];
			_emu->ProcessMemoryRead<CpuType::Genesis>(addr, _prgRom[addr], MemoryOperationType::Read);
			_openBus = _prgRom[addr + 1];
			return value;
		}
		return (_openBus << 8) | _openBus;
	}

	if (addr >= 0xFF0000) {
		uint32_t offset = addr & 0xFFFF;
		uint16_t value = ((uint16_t)_workRam[offset] << 8) | _workRam[offset + 1];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, _workRam[offset], MemoryOperationType::Read);
		_openBus = _workRam[offset + 1];
		return value;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		return ReadVdpPort(addr);
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			return ((uint16_t)_z80Ram[z80Addr] << 8) | _z80Ram[z80Addr];
		}
		return (_openBus << 8) | _openBus;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		uint8_t hi = ReadIo(addr);
		uint8_t lo = ReadIo(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr == 0xA11100) {
		return _z80BusRequest ? 0x0000 : 0x0100;
	}

	return (_openBus << 8) | _openBus;
}

void GenesisMemoryManager::Write8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	uint32_t sramOffset = 0;

	if (TryGetSramOffset(addr, sramOffset)) {
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, value, MemoryOperationType::Write);
		_saveRam[sramOffset] = value;
		return;
	}

	if (addr >= 0xFF0000) {
		uint32_t offset = addr & 0xFFFF;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, value, MemoryOperationType::Write);
		_workRam[offset] = value;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		// VDP byte write - promote to word write
		WriteVdpPort(addr, (uint16_t)value | ((uint16_t)value << 8));
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = value;
		}
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		WriteIo(addr, value);
		return;
	}

	if (addr == 0xA11100 || addr == 0xA11101) {
		_z80BusRequest = (value & 0x01) != 0;
		return;
	}

	if (addr == 0xA11200 || addr == 0xA11201) {
		_z80Reset = !(value & 0x01);
		return;
	}

	// ROM area — ignore writes (no mapper for now)
}

void GenesisMemoryManager::Write16(uint32_t addr, uint16_t value) {
	addr &= 0xFFFFFE;
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) {
		Write8(addr, (uint8_t)(value >> 8));
		Write8(addr + 1, (uint8_t)(value & 0xFF));
		return;
	}

	if (addr >= 0xFF0000) {
		uint32_t offset = addr & 0xFFFF;
		uint8_t highByte = (uint8_t)(value >> 8);
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, highByte, MemoryOperationType::Write);
		_workRam[offset] = (uint8_t)(value >> 8);
		_workRam[offset + 1] = (uint8_t)(value & 0xFF);
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		WriteVdpPort(addr, value);
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = (uint8_t)(value >> 8);
		}
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		WriteIo(addr, (uint8_t)(value >> 8));
		return;
	}

	if (addr == 0xA11100) {
		_z80BusRequest = (value & 0x0100) != 0;
		return;
	}

	if (addr == 0xA11200) {
		_z80Reset = !(value & 0x0100);
		return;
	}
}

// VDP port access
uint16_t GenesisMemoryManager::ReadVdpPort(uint32_t addr) {
	uint32_t port = addr & 0x1F;
	if (port < 0x04) {
		return _vdp->ReadDataPort();
	} else if (port < 0x08) {
		return _vdp->ReadControlPort();
	} else if (port < 0x10) {
		return _vdp->ReadHVCounter();
	}
	return _openBus;
}

void GenesisMemoryManager::WriteVdpPort(uint32_t addr, uint16_t value) {
	uint32_t port = addr & 0x1F;
	if (port < 0x04) {
		_vdp->WriteDataPort(value);
	} else if (port < 0x08) {
		_vdp->WriteControlPort(value);
	} else if (port >= 0x11 && port < 0x14) {
		// PSG write — SN76489 accepts byte writes via top byte of word
		if (_psg) {
			_psg->Write((uint8_t)(value >> 8));
		}
	}
}

// I/O registers ($A10001-$A1001F)
uint8_t GenesisMemoryManager::ReadIo(uint32_t addr) {
	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x01: return 0xA0; // Version register (overseas, NTSC, no expansion)
		case 0x03: return _controlManager->ReadDataPort(0); // Controller 1 data
		case 0x05: return _controlManager->ReadDataPort(1); // Controller 2 data
		case 0x07: return 0xFF; // EXT port
		case 0x09: return _ioState.CtrlPort[0]; // Controller 1 ctrl
		case 0x0B: return _ioState.CtrlPort[1]; // Controller 2 ctrl
		case 0x0D: return _ioState.CtrlPort[2]; // EXT ctrl
		default: return _openBus;
	}
}

void GenesisMemoryManager::WriteIo(uint32_t addr, uint8_t value) {
	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x03: _controlManager->WriteDataPort(0, value); break;
		case 0x05: _controlManager->WriteDataPort(1, value); break;
		case 0x09: _ioState.CtrlPort[0] = value; break;
		case 0x0B: _ioState.CtrlPort[1] = value; break;
		case 0x0D: _ioState.CtrlPort[2] = value; break;
	}
}

uint8_t GenesisMemoryManager::DebugRead8(uint32_t addr) {
	addr &= 0xFFFFFF;
	if (addr < _prgRomSize) return _prgRom[addr];
	if (addr >= 0xFF0000) return _workRam[addr & 0xFFFF];
	return 0;
}

void GenesisMemoryManager::DebugWrite8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	if (addr >= 0xFF0000) _workRam[addr & 0xFFFF] = value;
}

AddressInfo GenesisMemoryManager::GetAbsoluteAddress(uint32_t addr) {
	addr &= 0xFFFFFF;
	AddressInfo info = {};
	if (addr < _prgRomSize) {
		info.Address = addr;
		info.Type = MemoryType::GenesisPrgRom;
	} else if (addr >= 0xFF0000) {
		info.Address = addr & 0xFFFF;
		info.Type = MemoryType::GenesisWorkRam;
	} else {
		info.Address = -1;
		info.Type = MemoryType::None;
	}
	return info;
}

int32_t GenesisMemoryManager::GetRelativeAddress(AddressInfo& absAddress) {
	if (absAddress.Type == MemoryType::GenesisPrgRom) {
		return absAddress.Address;
	} else if (absAddress.Type == MemoryType::GenesisWorkRam) {
		return 0xFF0000 + absAddress.Address;
	}
	return -1;
}

void GenesisMemoryManager::Serialize(Serializer& s) {
	SVArray(_workRam, WorkRamSize);
	SVArray(_z80Ram, Z80RamSize);
	SV(_hasSram);
	SV(_sramStart);
	SV(_sramEnd);
	SV(_sramEvenBytes);
	SV(_sramOddBytes);
	if (_saveRam && _saveRamSize > 0) {
		SVArray(_saveRam, _saveRamSize);
	}
	SV(_masterClock);
	SV(_openBus);
	SV(_z80BusRequest);
	SV(_z80Reset);
	SV(_tmssEnabled);
	SV(_tmssUnlocked);
	SV(_ioState.DataPort[0]); SV(_ioState.DataPort[1]); SV(_ioState.DataPort[2]);
	SV(_ioState.CtrlPort[0]); SV(_ioState.CtrlPort[1]); SV(_ioState.CtrlPort[2]);
}

void GenesisMemoryManager::LoadBattery() {
	if (HasSaveRam()) {
		_emu->GetBatteryManager()->LoadBattery(".sav", std::span<uint8_t>(_saveRam, _saveRamSize));
	}
}

void GenesisMemoryManager::SaveBattery() {
	if (HasSaveRam()) {
		_emu->GetBatteryManager()->SaveBattery(".sav", std::span<const uint8_t>(_saveRam, _saveRamSize));
	}
}
