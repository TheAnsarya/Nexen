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

uint16_t GenesisM68kCpuStub::ReadWord(uint32_t address) const {
	if (!_bus) {
		return 0;
	}

	uint8_t hi = _bus->ReadByte(address & 0xFFFFFF);
	uint8_t lo = _bus->ReadByte((address + 1) & 0xFFFFFF);
	return (uint16_t)(((uint16_t)hi << 8) | lo);
}

uint32_t GenesisM68kCpuStub::ReadLong(uint32_t address) const {
	uint16_t hi = ReadWord(address);
	uint16_t lo = ReadWord(address + 2);
	return ((uint32_t)hi << 16) | lo;
}

void GenesisM68kCpuStub::WriteWord(uint32_t address, uint16_t value) {
	if (!_bus) {
		return;
	}

	address &= 0xFFFFFF;
	_bus->WriteByte(address, (uint8_t)((value >> 8) & 0xFF));
	_bus->WriteByte((address + 1) & 0xFFFFFF, (uint8_t)(value & 0xFF));
}

void GenesisM68kCpuStub::WriteLong(uint32_t address, uint32_t value) {
	WriteWord(address, (uint16_t)((value >> 16) & 0xFFFF));
	WriteWord(address + 2, (uint16_t)(value & 0xFFFF));
}

void GenesisM68kCpuStub::BeginInterruptSequence(uint8_t level) {
	uint32_t vectorAddress = (uint32_t)(24 + level) * 4u;
	uint16_t previousStatus = _statusRegister;
	uint32_t previousPc = _programCounter;

	_supervisorStackPointer = (_supervisorStackPointer - 4) & 0xFFFFFF;
	WriteLong(_supervisorStackPointer, previousPc);
	_supervisorStackPointer = (_supervisorStackPointer - 2) & 0xFFFFFF;
	WriteWord(_supervisorStackPointer, previousStatus);

	_statusRegister = (uint16_t)((previousStatus | 0x2000) & 0xF8FF);
	_statusRegister = (uint16_t)(_statusRegister | ((uint16_t)level << 8));

	_programCounter = ReadLong(vectorAddress) & 0xFFFFFF;
	_lastExceptionVectorAddress = vectorAddress;
	_interruptSequenceCount++;
	_interruptLevel = 0;
	_instructionCyclesRemaining = 44;
}

void GenesisM68kCpuStub::BeginNextInstruction() {
	uint8_t activeMask = (uint8_t)((_statusRegister >> 8) & 0x7);
	if (_interruptLevel > activeMask) {
		BeginInterruptSequence(_interruptLevel);
		return;
	}

	uint16_t opcode = ReadWord(_programCounter);
	uint8_t instructionSize = 2;
	uint8_t instructionCycles = 4;

	if (opcode == 0x4E71) {
		instructionSize = 2;
		instructionCycles = 4;
	} else if ((opcode & 0xF100) == 0x7000) {
		instructionSize = 2;
		instructionCycles = 4;
	} else {
		instructionSize = 2;
		instructionCycles = 4;
	}

	_programCounter = (_programCounter + instructionSize) & 0xFFFFFF;
	_instructionCyclesRemaining = instructionCycles;
}

void GenesisM68kCpuStub::Reset() {
	_programCounter = 0;
	_cycleCount = 0;
	_interruptLevel = 0;
	_statusRegister = 0x2000;
	_supervisorStackPointer = 0xFFFFFE;
	_lastExceptionVectorAddress = 0;
	_interruptSequenceCount = 0;
	_instructionCyclesRemaining = 0;
}

void GenesisM68kCpuStub::StepCycles(uint32_t cycles) {
	if (!_bus) {
		return;
	}

	for (uint32_t i = 0; i < cycles; i++) {
		if (_instructionCyclesRemaining == 0) {
			BeginNextInstruction();
		}

		if (_instructionCyclesRemaining > 0) {
			_instructionCyclesRemaining--;
		}
		_cycleCount++;
	}
}

void GenesisM68kCpuStub::SetInterrupt(uint8_t level) {
	_interruptLevel = std::min<uint8_t>(level, 7);
}

GenesisM68kBoundaryScaffold::GenesisM68kBoundaryScaffold() {
	_cpu.AttachBus(&_bus);
}

void GenesisM68kBoundaryScaffold::AdvanceTiming(uint32_t cpuCycles) {
	_timingCycleRemainder += cpuCycles;
	while (_timingCycleRemainder >= TimingCyclesPerScanline) {
		_timingCycleRemainder -= TimingCyclesPerScanline;
		_timingScanline++;
		if (_timingScanline >= TimingScanlinesPerFrame) {
			_timingScanline = 0;
			_timingFrame++;
		}
	}
}

void GenesisM68kBoundaryScaffold::LoadRom(const vector<uint8_t>& romData) {
	_bus.LoadRom(romData);
}

void GenesisM68kBoundaryScaffold::Startup() {
	_bus.Reset();
	_cpu.Reset();
	_started = true;
	_timingScanline = 0;
	_timingFrame = 0;
	_timingCycleRemainder = 0;
}

void GenesisM68kBoundaryScaffold::StepFrameScaffold(uint32_t cpuCycles) {
	if (!_started) {
		Startup();
	}
	_cpu.StepCycles(cpuCycles);
	AdvanceTiming(cpuCycles);
}
