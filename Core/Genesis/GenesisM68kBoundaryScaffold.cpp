#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

GenesisPlatformBusStub::GenesisPlatformBusStub()
	: _workRam(64 * 1024, 0),
	  _vdpIo(0x20, 0) {
}

void GenesisPlatformBusStub::LoadRom(const vector<uint8_t>& romData) {
	_rom = romData;
	if (_rom.empty()) {
		_rom.resize(0x10000, 0);
	}
}

void GenesisPlatformBusStub::Reset() {
	std::fill(_workRam.begin(), _workRam.end(), 0);
	std::fill(_vdpIo.begin(), _vdpIo.end(), 0);
	_z80WindowAccessed = false;
	_vdpWindowAccessed = false;
	_dmaRequested = false;
	_vdpReadCount = 0;
	_vdpWriteCount = 0;
	_lastVdpAddress = 0;
	_lastVdpValue = 0;
}

uint8_t GenesisPlatformBusStub::ReadByte(uint32_t address) {
	address &= 0xFFFFFF;

	if (address < 0x400000) {
		if (_rom.empty()) {
			return 0xFF;
		}
		return _rom[address % _rom.size()];
	}

	if (address >= 0xA00000 && address <= 0xA0FFFF) {
		_z80WindowAccessed = true;
		return 0;
	}

	if (address >= 0xC00000 && address <= 0xC0001F) {
		_vdpWindowAccessed = true;
		_vdpReadCount++;
		_lastVdpAddress = address;
		_lastVdpValue = _vdpIo[address & 0x1F];
		return _lastVdpValue;
	}

	if (address >= 0xFF0000) {
		return _workRam[address & 0xFFFF];
	}

	return 0;
}

void GenesisPlatformBusStub::WriteByte(uint32_t address, uint8_t value) {
	address &= 0xFFFFFF;

	if (address >= 0xA00000 && address <= 0xA0FFFF) {
		_z80WindowAccessed = true;
		return;
	}

	if (address >= 0xC00000 && address <= 0xC0001F) {
		_vdpWindowAccessed = true;
		_vdpWriteCount++;
		_lastVdpAddress = address;
		_lastVdpValue = value;
		_vdpIo[address & 0x1F] = value;
		if (address >= 0xC00004 && address <= 0xC00007 && (value & 0x80) != 0) {
			_dmaRequested = true;
		}
		return;
	}

	if (address >= 0xFF0000) {
		_workRam[address & 0xFFFF] = value;
	}
}

void GenesisM68kCpuStub::AttachBus(IGenesisM68kBus* bus) {
	_bus = bus;
}

void GenesisM68kCpuStub::Reset() {
	_programCounter = 0;
	_cycleCount = 0;
	_interruptLevel = 0;
}

void GenesisM68kCpuStub::StepCycles(uint32_t cycles) {
	if (!_bus) {
		return;
	}

	for (uint32_t i = 0; i < cycles; i++) {
		volatile uint8_t opcodeByte = _bus->ReadByte(_programCounter);
		(void)opcodeByte;
		_programCounter = (_programCounter + 2) & 0xFFFFFF;
		_cycleCount++;
	}
}

void GenesisM68kCpuStub::SetInterrupt(uint8_t level) {
	_interruptLevel = std::min<uint8_t>(level, 7);
}

GenesisM68kBoundaryScaffold::GenesisM68kBoundaryScaffold() {
	_cpu.AttachBus(&_bus);
}

void GenesisM68kBoundaryScaffold::LoadRom(const vector<uint8_t>& romData) {
	_bus.LoadRom(romData);
}

void GenesisM68kBoundaryScaffold::Startup() {
	_bus.Reset();
	_cpu.Reset();
	_started = true;
}

void GenesisM68kBoundaryScaffold::StepFrameScaffold(uint32_t cpuCycles) {
	if (!_started) {
		Startup();
	}
	_cpu.StepCycles(cpuCycles);
}
