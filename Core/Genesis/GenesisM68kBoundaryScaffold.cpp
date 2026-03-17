#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	string ToHexDigest(uint64_t value) {
		return std::format("{:016x}", value);
	}
}

GenesisPlatformBusStub::GenesisPlatformBusStub()
	: _workRam(64 * 1024, 0),
	  _io(0x20, 0),
	  _vdpIo(0x20, 0) {
}

GenesisBusOwner GenesisPlatformBusStub::DecodeOwner(uint32_t address) const {
	address &= 0xFFFFFF;

	if (address < 0x400000) {
		return GenesisBusOwner::Rom;
	}

	if (address >= 0xA00000 && address <= 0xA0FFFF) {
		return GenesisBusOwner::Z80;
	}

	if (address >= 0xA10000 && address <= 0xA1001F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xC00000 && address <= 0xC0001F) {
		return GenesisBusOwner::Vdp;
	}

	if (address >= 0xFF0000) {
		return GenesisBusOwner::WorkRam;
	}

	return GenesisBusOwner::OpenBus;
}

void GenesisPlatformBusStub::ApplyVdpControlWord(uint16_t controlWord) {
	if ((controlWord & 0xC000) == 0x8000) {
		uint8_t regIndex = (uint8_t)((controlWord >> 8) & 0x1F);
		uint8_t regValue = (uint8_t)(controlWord & 0xFF);
		_vdpRegisters[regIndex] = regValue;

		if (regIndex == 1) {
			if ((regValue & 0x40) != 0) {
				_vdpStatus |= 0x0008;
			} else {
				_vdpStatus = (uint16_t)(_vdpStatus & ~0x0008);
			}

			if ((regValue & 0x20) != 0) {
				_dmaRequested = true;
			}
		}
	} else if ((controlWord & 0xC000) == 0x4000) {
		// Model a deterministic status side effect for control-word command routing.
		_vdpStatus |= 0x8000;
		_vdpStatus = (uint16_t)(_vdpStatus & ~0x0001);
		_vdpStatus |= 0x0002;
	}
}

void GenesisPlatformBusStub::LoadRom(const vector<uint8_t>& romData) {
	_rom = romData;
	if (_rom.empty()) {
		_rom.resize(0x10000, 0);
	}
}

void GenesisPlatformBusStub::Reset() {
	std::fill(_workRam.begin(), _workRam.end(), 0);
	std::fill(_io.begin(), _io.end(), 0);
	std::fill(_vdpIo.begin(), _vdpIo.end(), 0);
	std::fill(_vdpRegisters.begin(), _vdpRegisters.end(), 0);
	_vdpStatus = 0x0001;
	_vdpDataPortLatch = 0;
	_vdpControlWordLatch = 0;
	_planeASample = 0;
	_planeBSample = 0;
	_windowSample = 0;
	_spriteSample = 0;
	_planeAPriority = false;
	_planeBPriority = false;
	_windowEnabled = false;
	_windowPriority = false;
	_spritePriority = false;
	_scrollX = 0;
	_scrollY = 0;
	_renderLine.clear();
	_renderLineDigest.clear();
	_z80WindowAccessed = false;
	_ioWindowAccessed = false;
	_vdpWindowAccessed = false;
	_dmaRequested = false;
	_romReadCount = 0;
	_z80ReadCount = 0;
	_z80WriteCount = 0;
	_ioReadCount = 0;
	_ioWriteCount = 0;
	_vdpReadCount = 0;
	_vdpWriteCount = 0;
	_workRamReadCount = 0;
	_workRamWriteCount = 0;
	_openBusReadCount = 0;
	_openBusWriteCount = 0;
	_lastVdpAddress = 0;
	_lastVdpValue = 0;
}

uint8_t GenesisPlatformBusStub::ComposeRenderPixel() const {
	uint8_t output = 0;

	if (_planeBSample != 0) {
		output = _planeBSample;
	}

	if (_planeASample != 0 && (_planeAPriority || output == 0)) {
		output = _planeASample;
	}

	if (_windowEnabled && _windowSample != 0 && (_windowPriority || output == 0)) {
		output = _windowSample;
	}

	if (_spriteSample != 0 && (_spritePriority || output == 0)) {
		output = _spriteSample;
	}

	return output;
}

void GenesisPlatformBusStub::SetRenderCompositionInputs(uint8_t planeA, bool planeAPriority, uint8_t planeB, bool planeBPriority, uint8_t window, bool windowEnabled, bool windowPriority, uint8_t sprite, bool spritePriority) {
	_planeASample = planeA;
	_planeAPriority = planeAPriority;
	_planeBSample = planeB;
	_planeBPriority = planeBPriority;
	_windowSample = window;
	_windowEnabled = windowEnabled;
	_windowPriority = windowPriority;
	_spriteSample = sprite;
	_spritePriority = spritePriority;
}

void GenesisPlatformBusStub::SetScroll(uint16_t scrollX, uint16_t scrollY) {
	_scrollX = scrollX;
	_scrollY = scrollY;
}

void GenesisPlatformBusStub::RenderScaffoldLine(uint32_t pixelCount) {
	_renderLine.clear();
	_renderLine.reserve(pixelCount);

	uint8_t base = ComposeRenderPixel();
	uint64_t hash = 1469598103934665603ull;
	for (uint32_t i = 0; i < pixelCount; i++) {
		uint8_t offset = (uint8_t)(((uint32_t)_scrollX + (uint32_t)_scrollY + i) & 0x03);
		uint8_t pixel = (uint8_t)((base + offset) & 0x3F);
		_renderLine.push_back(pixel);
		hash ^= pixel;
		hash *= 1099511628211ull;
	}

	_renderLineDigest = ToHexDigest(hash);
}

uint8_t GenesisPlatformBusStub::GetRenderLinePixel(uint32_t index) const {
	if (index >= _renderLine.size()) {
		return 0;
	}
	return _renderLine[index];
}

uint8_t GenesisPlatformBusStub::ReadByte(uint32_t address) {
	address &= 0xFFFFFF;
	GenesisBusOwner owner = DecodeOwner(address);

	switch (owner) {
		case GenesisBusOwner::Rom:
			_romReadCount++;
			if (_rom.empty()) {
				return 0xFF;
			}
			return _rom[address % _rom.size()];

		case GenesisBusOwner::Z80:
			_z80WindowAccessed = true;
			_z80ReadCount++;
			return 0;

		case GenesisBusOwner::Io:
			_ioWindowAccessed = true;
			_ioReadCount++;
			return _io[address & 0x1F];

		case GenesisBusOwner::Vdp:
			_vdpWindowAccessed = true;
			_vdpReadCount++;
			_lastVdpAddress = address;
			if (address <= 0xC00003) {
				_lastVdpValue = (uint8_t)((_vdpDataPortLatch >> ((address & 1) == 0 ? 8 : 0)) & 0xFF);
			} else {
				_lastVdpValue = (uint8_t)((_vdpStatus >> ((address & 1) == 0 ? 8 : 0)) & 0xFF);
				if ((address & 1) == 0) {
					_vdpStatus = (uint16_t)(_vdpStatus & ~0x8000);
				}
			}
			return _lastVdpValue;

		case GenesisBusOwner::WorkRam:
			_workRamReadCount++;
			return _workRam[address & 0xFFFF];

		case GenesisBusOwner::OpenBus:
		default:
			_openBusReadCount++;
			return 0xFF;
	}
}

void GenesisPlatformBusStub::WriteByte(uint32_t address, uint8_t value) {
	address &= 0xFFFFFF;
	GenesisBusOwner owner = DecodeOwner(address);

	switch (owner) {
		case GenesisBusOwner::Rom:
			return;

		case GenesisBusOwner::Z80:
			_z80WindowAccessed = true;
			_z80WriteCount++;
			return;

		case GenesisBusOwner::Io:
			_ioWindowAccessed = true;
			_ioWriteCount++;
			_io[address & 0x1F] = value;
			return;

		case GenesisBusOwner::Vdp:
			_vdpWindowAccessed = true;
			_vdpWriteCount++;
			_lastVdpAddress = address;
			_lastVdpValue = value;
			_vdpIo[address & 0x1F] = value;
			if (address <= 0xC00003) {
				if ((address & 1) == 0) {
					_vdpDataPortLatch = (uint16_t)((_vdpDataPortLatch & 0x00FF) | ((uint16_t)value << 8));
				} else {
					_vdpDataPortLatch = (uint16_t)((_vdpDataPortLatch & 0xFF00) | value);
				}
				_vdpStatus = (uint16_t)(_vdpStatus & ~0x0002);
				_vdpStatus |= 0x0001;
			} else {
				if ((address & 1) == 0) {
					_vdpControlWordLatch = (uint16_t)((_vdpControlWordLatch & 0x00FF) | ((uint16_t)value << 8));
				} else {
					_vdpControlWordLatch = (uint16_t)((_vdpControlWordLatch & 0xFF00) | value);
					ApplyVdpControlWord(_vdpControlWordLatch);
				}
			}
			if (address >= 0xC00004 && address <= 0xC00007 && (value & 0x80) != 0) {
				_dmaRequested = true;
			}
			return;

		case GenesisBusOwner::WorkRam:
			_workRamWriteCount++;
			_workRam[address & 0xFFFF] = value;
			return;

		case GenesisBusOwner::OpenBus:
		default:
			_openBusWriteCount++;
			return;
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
