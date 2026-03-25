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

	if (address >= 0xA04000 && address <= 0xA04003) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA00000 && address <= 0xA0FFFF) {
		return GenesisBusOwner::Z80;
	}

	if ((address >= 0xA10000 && address <= 0xA1001F) || (address >= 0xA11100 && address <= 0xA11201)) {
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

		if (regIndex == 23) {
			_dmaMode = DecodeDmaModeFromControl(regValue);
		}

		if (regIndex == 1) {
			if ((regValue & 0x40) != 0) {
				_vdpStatus |= 0x0008;
			} else {
				_vdpStatus = (uint16_t)(_vdpStatus & ~0x0008);
			}

			if ((regValue & 0x20) != 0) {
				_dmaRequested = true;
				if (_dmaMode == GenesisVdpDmaMode::None) {
					_dmaMode = GenesisVdpDmaMode::Copy;
				}
				BeginDmaTransfer(_dmaMode, 16);
			}
		}
	} else if ((controlWord & 0xC000) == 0x4000) {
		// Model a deterministic status side effect for control-word command routing.
		_vdpStatus |= 0x8000;
		_vdpStatus = (uint16_t)(_vdpStatus & ~0x0001);
		_vdpStatus |= 0x0002;
	}
}

GenesisVdpDmaMode GenesisPlatformBusStub::DecodeDmaModeFromControl(uint8_t registerValue) const {
	uint8_t modeBits = (uint8_t)(registerValue & 0xC0);
	if (modeBits == 0x80) {
		return GenesisVdpDmaMode::Fill;
	}
	if (modeBits == 0xC0) {
		return GenesisVdpDmaMode::Copy;
	}
	return GenesisVdpDmaMode::None;
}

void GenesisPlatformBusStub::BeginDmaTransfer(GenesisVdpDmaMode mode, uint32_t transferWords) {
	if (transferWords == 0) {
		_dmaMode = GenesisVdpDmaMode::None;
		_dmaTransferWords = 0;
		_dmaActiveCyclesRemaining = 0;
		_dmaRequested = false;
		return;
	}

	_dmaMode = mode;
	_dmaTransferWords = transferWords;
	uint64_t totalCycles = (uint64_t)transferWords * 4ull;
	if (totalCycles > std::numeric_limits<uint32_t>::max()) {
		_dmaActiveCyclesRemaining = std::numeric_limits<uint32_t>::max();
	} else {
		_dmaActiveCyclesRemaining = (uint32_t)totalCycles;
	}
	_dmaRequested = true;
}

uint32_t GenesisPlatformBusStub::ConsumeDmaContention(uint32_t requestedCycles) {
	if (_dmaMode == GenesisVdpDmaMode::None || _dmaActiveCyclesRemaining == 0 || requestedCycles == 0) {
		return 0;
	}

	uint32_t penalty = (requestedCycles + 3) / 4;
	if (penalty > _dmaActiveCyclesRemaining) {
		penalty = _dmaActiveCyclesRemaining;
	}

	if (penalty > 0) {
		_dmaContentionEvents++;
		_dmaContentionCycles += penalty;
		_dmaActiveCyclesRemaining -= penalty;
	}

	if (_dmaActiveCyclesRemaining == 0) {
		_dmaMode = GenesisVdpDmaMode::None;
	}

	return penalty;
}

void GenesisPlatformBusStub::BootstrapZ80() {
	if (!_z80Bootstrapped) {
		_z80Bootstrapped = true;
		_z80BootstrapCount++;
	}

	if (!_z80BusRequested) {
		_z80Running = true;
	}
}

void GenesisPlatformBusStub::RequestZ80Bus(bool requestBusForM68k) {
	if (_z80BusRequested != requestBusForM68k) {
		_z80HandoffCount++;
	}

	_z80BusRequested = requestBusForM68k;
	if (_z80BusRequested) {
		_z80Running = false;
	} else if (_z80Bootstrapped) {
		_z80Running = true;
	}
}

void GenesisPlatformBusStub::StepZ80Cycles(uint32_t cycles) {
	if (_z80Running && !_z80BusRequested) {
		_z80ExecutedCycles += cycles;
	}
}

void GenesisPlatformBusStub::YmWriteAddress(uint8_t port, uint8_t value) {
	if (port == 0) {
		_ym2612AddressPort0 = value;
	} else {
		_ym2612AddressPort1 = value;
	}
}

void GenesisPlatformBusStub::YmWriteData(uint8_t port, uint8_t value) {
	uint16_t regIndex = (uint16_t)(port == 0 ? _ym2612AddressPort0 : (0x100 | _ym2612AddressPort1));
	_ym2612Registers[regIndex & 0x1FF] = value;
	_ym2612WriteCount++;
}

void GenesisPlatformBusStub::StepYm2612(uint32_t masterCycles) {
	_ym2612ClockAccumulator += masterCycles;
	while (_ym2612ClockAccumulator >= 144) {
		_ym2612ClockAccumulator -= 144;
		int32_t mix =
			(int32_t)_ym2612Registers[0x22] +
			(int32_t)_ym2612Registers[0x27] * 2 +
			(int32_t)_ym2612Registers[0x28] * 3 +
			(int32_t)_ym2612Registers[0x130] * 2 +
			(int32_t)_ym2612SampleCount;
		_ym2612LastSample = (int16_t)(((mix * 97) & 0x7FFF) - 0x4000);
		_ym2612SampleCount++;

		uint64_t hash = _ym2612Digest.empty() ? 1469598103934665603ull : std::stoull(_ym2612Digest, nullptr, 16);
		hash ^= (uint16_t)_ym2612LastSample;
		hash *= 1099511628211ull;
		_ym2612Digest = ToHexDigest(hash);
	}
}

void GenesisPlatformBusStub::PsgWrite(uint8_t value) {
	if ((value & 0x80) != 0) {
		_sn76489LatchedRegister = (uint8_t)((value >> 4) & 0x07);
		_sn76489Registers[_sn76489LatchedRegister] = (uint8_t)((_sn76489Registers[_sn76489LatchedRegister] & 0xF0) | (value & 0x0F));
	} else {
		_sn76489Registers[_sn76489LatchedRegister] = (uint8_t)((_sn76489Registers[_sn76489LatchedRegister] & 0x0F) | ((value & 0x3F) << 4));
	}
	_sn76489WriteCount++;
}

void GenesisPlatformBusStub::StepSn76489(uint32_t masterCycles) {
	_sn76489ClockAccumulator += masterCycles;
	while (_sn76489ClockAccumulator >= 128) {
		_sn76489ClockAccumulator -= 128;
		int32_t mix =
			(int32_t)_sn76489Registers[0] +
			(int32_t)_sn76489Registers[1] * 2 +
			(int32_t)_sn76489Registers[2] * 3 +
			(int32_t)_sn76489Registers[7] +
			(int32_t)_sn76489SampleCount;
		_sn76489LastSample = (int16_t)(((mix * 53) & 0x7FFF) - 0x4000);
		_sn76489SampleCount++;

		uint64_t hash = _sn76489Digest.empty() ? 1469598103934665603ull : std::stoull(_sn76489Digest, nullptr, 16);
		hash ^= (uint16_t)_sn76489LastSample;
		hash *= 1099511628211ull;
		_sn76489Digest = ToHexDigest(hash);
	}
}

void GenesisPlatformBusStub::UpdateMixedSample() {
	_mixedLastSample = (int16_t)(((int32_t)_ym2612LastSample + (int32_t)_sn76489LastSample) / 2);
	_mixedSampleCount++;
	uint64_t hash = _mixedDigest.empty() ? 1469598103934665603ull : std::stoull(_mixedDigest, nullptr, 16);
	hash ^= (uint16_t)_mixedLastSample;
	hash *= 1099511628211ull;
	_mixedDigest = ToHexDigest(hash);
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
	_dmaMode = GenesisVdpDmaMode::None;
	_dmaTransferWords = 0;
	_dmaActiveCyclesRemaining = 0;
	_dmaContentionCycles = 0;
	_dmaContentionEvents = 0;
	_z80Bootstrapped = false;
	_z80Running = false;
	_z80BusRequested = false;
	_z80BootstrapCount = 0;
	_z80HandoffCount = 0;
	_z80ExecutedCycles = 0;
	std::fill(_ym2612Registers.begin(), _ym2612Registers.end(), 0);
	_ym2612AddressPort0 = 0;
	_ym2612AddressPort1 = 0;
	_ym2612ClockAccumulator = 0;
	_ym2612SampleCount = 0;
	_ym2612LastSample = 0;
	_ym2612WriteCount = 0;
	_ym2612Digest.clear();
	std::fill(_sn76489Registers.begin(), _sn76489Registers.end(), 0);
	_sn76489LatchedRegister = 0;
	_sn76489ClockAccumulator = 0;
	_sn76489SampleCount = 0;
	_sn76489LastSample = 0;
	_sn76489WriteCount = 0;
	_sn76489Digest.clear();
	_mixedLastSample = 0;
	_mixedSampleCount = 0;
	_mixedDigest.clear();
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

GenesisPlatformBusSaveState GenesisPlatformBusStub::SaveState() const {
	GenesisPlatformBusSaveState state = {};
	state.Rom = _rom;
	state.WorkRam = _workRam;
	state.Io = _io;
	state.VdpIo = _vdpIo;
	state.VdpRegisters = _vdpRegisters;
	state.VdpStatus = _vdpStatus;
	state.VdpDataPortLatch = _vdpDataPortLatch;
	state.VdpControlWordLatch = _vdpControlWordLatch;
	state.PlaneASample = _planeASample;
	state.PlaneBSample = _planeBSample;
	state.WindowSample = _windowSample;
	state.SpriteSample = _spriteSample;
	state.PlaneAPriority = _planeAPriority;
	state.PlaneBPriority = _planeBPriority;
	state.WindowEnabled = _windowEnabled;
	state.WindowPriority = _windowPriority;
	state.SpritePriority = _spritePriority;
	state.ScrollX = _scrollX;
	state.ScrollY = _scrollY;
	state.RenderLine = _renderLine;
	state.RenderLineDigest = _renderLineDigest;
	state.DmaMode = _dmaMode;
	state.DmaTransferWords = _dmaTransferWords;
	state.DmaActiveCyclesRemaining = _dmaActiveCyclesRemaining;
	state.DmaContentionCycles = _dmaContentionCycles;
	state.DmaContentionEvents = _dmaContentionEvents;
	state.Z80Bootstrapped = _z80Bootstrapped;
	state.Z80Running = _z80Running;
	state.Z80BusRequested = _z80BusRequested;
	state.Z80BootstrapCount = _z80BootstrapCount;
	state.Z80HandoffCount = _z80HandoffCount;
	state.Z80ExecutedCycles = _z80ExecutedCycles;
	state.Ym2612Registers = _ym2612Registers;
	state.Ym2612AddressPort0 = _ym2612AddressPort0;
	state.Ym2612AddressPort1 = _ym2612AddressPort1;
	state.Ym2612ClockAccumulator = _ym2612ClockAccumulator;
	state.Ym2612SampleCount = _ym2612SampleCount;
	state.Ym2612LastSample = _ym2612LastSample;
	state.Ym2612WriteCount = _ym2612WriteCount;
	state.Ym2612Digest = _ym2612Digest;
	state.Sn76489Registers = _sn76489Registers;
	state.Sn76489LatchedRegister = _sn76489LatchedRegister;
	state.Sn76489ClockAccumulator = _sn76489ClockAccumulator;
	state.Sn76489SampleCount = _sn76489SampleCount;
	state.Sn76489LastSample = _sn76489LastSample;
	state.Sn76489WriteCount = _sn76489WriteCount;
	state.Sn76489Digest = _sn76489Digest;
	state.MixedLastSample = _mixedLastSample;
	state.MixedSampleCount = _mixedSampleCount;
	state.MixedDigest = _mixedDigest;
	state.Z80WindowAccessed = _z80WindowAccessed;
	state.IoWindowAccessed = _ioWindowAccessed;
	state.VdpWindowAccessed = _vdpWindowAccessed;
	state.DmaRequested = _dmaRequested;
	state.RomReadCount = _romReadCount;
	state.Z80ReadCount = _z80ReadCount;
	state.Z80WriteCount = _z80WriteCount;
	state.IoReadCount = _ioReadCount;
	state.IoWriteCount = _ioWriteCount;
	state.VdpReadCount = _vdpReadCount;
	state.VdpWriteCount = _vdpWriteCount;
	state.WorkRamReadCount = _workRamReadCount;
	state.WorkRamWriteCount = _workRamWriteCount;
	state.OpenBusReadCount = _openBusReadCount;
	state.OpenBusWriteCount = _openBusWriteCount;
	state.LastVdpAddress = _lastVdpAddress;
	state.LastVdpValue = _lastVdpValue;
	return state;
}

void GenesisPlatformBusStub::LoadState(const GenesisPlatformBusSaveState& state) {
	_rom = state.Rom;
	_workRam = state.WorkRam;
	_io = state.Io;
	_vdpIo = state.VdpIo;
	_vdpRegisters = state.VdpRegisters;
	_vdpStatus = state.VdpStatus;
	_vdpDataPortLatch = state.VdpDataPortLatch;
	_vdpControlWordLatch = state.VdpControlWordLatch;
	_planeASample = state.PlaneASample;
	_planeBSample = state.PlaneBSample;
	_windowSample = state.WindowSample;
	_spriteSample = state.SpriteSample;
	_planeAPriority = state.PlaneAPriority;
	_planeBPriority = state.PlaneBPriority;
	_windowEnabled = state.WindowEnabled;
	_windowPriority = state.WindowPriority;
	_spritePriority = state.SpritePriority;
	_scrollX = state.ScrollX;
	_scrollY = state.ScrollY;
	_renderLine = state.RenderLine;
	_renderLineDigest = state.RenderLineDigest;
	_dmaMode = state.DmaMode;
	_dmaTransferWords = state.DmaTransferWords;
	_dmaActiveCyclesRemaining = state.DmaActiveCyclesRemaining;
	_dmaContentionCycles = state.DmaContentionCycles;
	_dmaContentionEvents = state.DmaContentionEvents;
	_z80Bootstrapped = state.Z80Bootstrapped;
	_z80Running = state.Z80Running;
	_z80BusRequested = state.Z80BusRequested;
	_z80BootstrapCount = state.Z80BootstrapCount;
	_z80HandoffCount = state.Z80HandoffCount;
	_z80ExecutedCycles = state.Z80ExecutedCycles;
	_ym2612Registers = state.Ym2612Registers;
	_ym2612AddressPort0 = state.Ym2612AddressPort0;
	_ym2612AddressPort1 = state.Ym2612AddressPort1;
	_ym2612ClockAccumulator = state.Ym2612ClockAccumulator;
	_ym2612SampleCount = state.Ym2612SampleCount;
	_ym2612LastSample = state.Ym2612LastSample;
	_ym2612WriteCount = state.Ym2612WriteCount;
	_ym2612Digest = state.Ym2612Digest;
	_sn76489Registers = state.Sn76489Registers;
	_sn76489LatchedRegister = state.Sn76489LatchedRegister;
	_sn76489ClockAccumulator = state.Sn76489ClockAccumulator;
	_sn76489SampleCount = state.Sn76489SampleCount;
	_sn76489LastSample = state.Sn76489LastSample;
	_sn76489WriteCount = state.Sn76489WriteCount;
	_sn76489Digest = state.Sn76489Digest;
	_mixedLastSample = state.MixedLastSample;
	_mixedSampleCount = state.MixedSampleCount;
	_mixedDigest = state.MixedDigest;
	_z80WindowAccessed = state.Z80WindowAccessed;
	_ioWindowAccessed = state.IoWindowAccessed;
	_vdpWindowAccessed = state.VdpWindowAccessed;
	_dmaRequested = state.DmaRequested;
	_romReadCount = state.RomReadCount;
	_z80ReadCount = state.Z80ReadCount;
	_z80WriteCount = state.Z80WriteCount;
	_ioReadCount = state.IoReadCount;
	_ioWriteCount = state.IoWriteCount;
	_vdpReadCount = state.VdpReadCount;
	_vdpWriteCount = state.VdpWriteCount;
	_workRamReadCount = state.WorkRamReadCount;
	_workRamWriteCount = state.WorkRamWriteCount;
	_openBusReadCount = state.OpenBusReadCount;
	_openBusWriteCount = state.OpenBusWriteCount;
	_lastVdpAddress = state.LastVdpAddress;
	_lastVdpValue = state.LastVdpValue;
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
			if (_z80Running && !_z80BusRequested) {
				return 0xFF;
			}
			return 0;

		case GenesisBusOwner::Io:
			_ioWindowAccessed = true;
			_ioReadCount++;
			if (address == 0xA04000) {
				return _ym2612AddressPort0;
			}
			if (address == 0xA04001) {
				return _ym2612Registers[_ym2612AddressPort0 & 0x1FF];
			}
			if (address == 0xA04002) {
				return _ym2612AddressPort1;
			}
			if (address == 0xA04003) {
				return _ym2612Registers[(0x100 | _ym2612AddressPort1) & 0x1FF];
			}
			if (address == 0xA11100) {
				return _z80BusRequested ? 0x01 : 0x00;
			}
			if (address == 0xA11200) {
				return _z80Running ? 0x01 : 0x00;
			}
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
			if (_z80Running && !_z80BusRequested) {
				return;
			}
			return;

		case GenesisBusOwner::Io:
			_ioWindowAccessed = true;
			_ioWriteCount++;
			_io[address & 0x1F] = value;
			if (address == 0xA04000) {
				YmWriteAddress(0, value);
			}
			if (address == 0xA04001) {
				YmWriteData(0, value);
			}
			if (address == 0xA04002) {
				YmWriteAddress(1, value);
			}
			if (address == 0xA04003) {
				YmWriteData(1, value);
			}
			if (address == 0xA11100) {
				RequestZ80Bus((value & 0x01) != 0);
			}
			if (address == 0xA11200) {
				if ((value & 0x01) == 0) {
					_z80Running = false;
				} else {
					BootstrapZ80();
				}
			}
			return;

		case GenesisBusOwner::Vdp:
			_vdpWindowAccessed = true;
			_vdpWriteCount++;
			_lastVdpAddress = address;
			_lastVdpValue = value;
			_vdpIo[address & 0x1F] = value;
			if (address == 0xC00011) {
				PsgWrite(value);
			}
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
				if (_dmaMode == GenesisVdpDmaMode::None) {
					_dmaMode = GenesisVdpDmaMode::Copy;
				}
				if (_dmaActiveCyclesRemaining == 0) {
					BeginDmaTransfer(_dmaMode, 8);
				}
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

GenesisM68kCpuSaveState GenesisM68kCpuStub::SaveState() const {
	GenesisM68kCpuSaveState state = {};
	state.ProgramCounter = _programCounter;
	state.CycleCount = _cycleCount;
	state.InterruptLevel = _interruptLevel;
	state.StatusRegister = _statusRegister;
	state.SupervisorStackPointer = _supervisorStackPointer;
	state.LastExceptionVectorAddress = _lastExceptionVectorAddress;
	state.InterruptSequenceCount = _interruptSequenceCount;
	state.InstructionCyclesRemaining = _instructionCyclesRemaining;
	return state;
}

void GenesisM68kCpuStub::LoadState(const GenesisM68kCpuSaveState& state) {
	_programCounter = state.ProgramCounter;
	_cycleCount = state.CycleCount;
	_interruptLevel = state.InterruptLevel;
	_statusRegister = state.StatusRegister;
	_supervisorStackPointer = state.SupervisorStackPointer;
	_lastExceptionVectorAddress = state.LastExceptionVectorAddress;
	_interruptSequenceCount = state.InterruptSequenceCount;
	_instructionCyclesRemaining = state.InstructionCyclesRemaining;
}

GenesisM68kBoundaryScaffold::GenesisM68kBoundaryScaffold() {
	_cpu.AttachBus(&_bus);
}

void GenesisM68kBoundaryScaffold::AdvanceTiming(uint32_t cpuCycles) {
	_timingCycleRemainder += cpuCycles;
	while (_timingCycleRemainder >= TimingCyclesPerScanline) {
		_timingCycleRemainder -= TimingCyclesPerScanline;
		_timingScanline++;

		if (_hInterruptEnabled && _hInterruptIntervalScanlines > 0 && (_timingScanline % _hInterruptIntervalScanlines) == 0) {
			_cpu.SetInterrupt(4);
			_hInterruptCount++;
			_timingEvents.push_back(std::format("HINT frame={} scanline={} count={}", _timingFrame, _timingScanline, _hInterruptCount));
		}

		if (_timingScanline >= TimingScanlinesPerFrame) {
			if (_vInterruptEnabled) {
				_cpu.SetInterrupt(6);
				_vInterruptCount++;
				_timingEvents.push_back(std::format("VINT frame={} count={}", _timingFrame + 1, _vInterruptCount));
			}

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
	_hInterruptCount = 0;
	_vInterruptCount = 0;
	_timingEvents.clear();
}

void GenesisM68kBoundaryScaffold::ConfigureInterruptSchedule(bool hInterruptEnabled, uint32_t hInterruptIntervalScanlines, bool vInterruptEnabled) {
	_hInterruptEnabled = hInterruptEnabled;
	_hInterruptIntervalScanlines = std::max<uint32_t>(1, hInterruptIntervalScanlines);
	_vInterruptEnabled = vInterruptEnabled;
}

void GenesisM68kBoundaryScaffold::ClearTimingEvents() {
	_timingEvents.clear();
}

void GenesisM68kBoundaryScaffold::StepFrameScaffold(uint32_t cpuCycles) {
	if (!_started) {
		Startup();
	}
	uint32_t contentionPenalty = _bus.ConsumeDmaContention(cpuCycles);
	uint32_t executableCycles = cpuCycles > contentionPenalty ? cpuCycles - contentionPenalty : 0;
	_cpu.StepCycles(executableCycles);
	_bus.StepZ80Cycles(cpuCycles / 2);
	_bus.StepYm2612(cpuCycles);
	_bus.StepSn76489(cpuCycles);
	_bus.UpdateMixedSample();
	AdvanceTiming(cpuCycles);
}

GenesisBoundaryScaffoldSaveState GenesisM68kBoundaryScaffold::SaveState() const {
	GenesisBoundaryScaffoldSaveState state = {};
	state.Bus = _bus.SaveState();
	state.Cpu = _cpu.SaveState();
	state.Started = _started;
	state.TimingScanline = _timingScanline;
	state.TimingFrame = _timingFrame;
	state.TimingCycleRemainder = _timingCycleRemainder;
	state.HInterruptEnabled = _hInterruptEnabled;
	state.VInterruptEnabled = _vInterruptEnabled;
	state.HInterruptIntervalScanlines = _hInterruptIntervalScanlines;
	state.HInterruptCount = _hInterruptCount;
	state.VInterruptCount = _vInterruptCount;
	state.TimingEvents = _timingEvents;
	return state;
}

void GenesisM68kBoundaryScaffold::LoadState(const GenesisBoundaryScaffoldSaveState& state) {
	_bus.LoadState(state.Bus);
	_cpu.LoadState(state.Cpu);
	_cpu.AttachBus(&_bus);
	_started = state.Started;
	_timingScanline = state.TimingScanline;
	_timingFrame = state.TimingFrame;
	_timingCycleRemainder = state.TimingCycleRemainder;
	_hInterruptEnabled = state.HInterruptEnabled;
	_vInterruptEnabled = state.VInterruptEnabled;
	_hInterruptIntervalScanlines = std::max<uint32_t>(1, state.HInterruptIntervalScanlines);
	_hInterruptCount = state.HInterruptCount;
	_vInterruptCount = state.VInterruptCount;
	_timingEvents = state.TimingEvents;
}
