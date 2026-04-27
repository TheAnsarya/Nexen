#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include <charconv>

namespace {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;

	string ToHexDigest(uint64_t value) {
		return std::format("{:016x}", value);
	}

	bool IsSegaCdToolingControlAddress(uint32_t address) {
		return address >= 0xA12012 && address <= 0xA12015;
	}

	bool IsSegaCdToolingStatusAddress(uint32_t address) {
		return address >= 0xA12018 && address <= 0xA1201F;
	}

	uint8_t GetDigestLowByte(const string& digestHex) {
		if (digestHex.size() < 2) {
			return 0;
		}

		uint32_t value = 0;
		auto [ptr, ec] = std::from_chars(digestHex.data() + (digestHex.size() - 2), digestHex.data() + digestHex.size(), value, 16);
		if (ec != std::errc() || ptr != digestHex.data() + digestHex.size()) {
			return 0;
		}

		return (uint8_t)(value & 0xFF);
	}

	bool Is32xSh2ControlAddress(uint32_t address) {
		return address == 0xA15012 || address == 0xA15013 || address == 0xA15014;
	}

	bool Is32xSh2StatusAddress(uint32_t address) {
		return address == 0xA1501A || address == 0xA1501B;
	}

	bool Is32xCompositionControlAddress(uint32_t address) {
		return address == 0xA15016 || address == 0xA15017;
	}

	bool Is32xCompositionStatusAddress(uint32_t address) {
		return address == 0xA1501C || address == 0xA1501D;
	}

	bool Is32xToolingControlAddress(uint32_t address) {
		return address >= 0xA15008 && address <= 0xA1500B;
	}

	bool Is32xToolingStatusAddress(uint32_t address) {
		return address == 0xA1501E || address == 0xA1501F;
	}
}

GenesisPlatformBusStub::GenesisPlatformBusStub()
	: _workRam(64 * 1024, 0),
	  _io(0x20, 0),
	  _expansionIo(0xC0, 0),
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

	if (address >= 0xA12000 && address <= 0xA1201F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA13000 && address <= 0xA1301F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA14000 && address <= 0xA1401F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA15000 && address <= 0xA1501F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA16000 && address <= 0xA1601F) {
		return GenesisBusOwner::Io;
	}

	if (address >= 0xA18000 && address <= 0xA1801F) {
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

bool GenesisPlatformBusStub::TryGetExpansionIoOffset(uint32_t address, uint32_t& offset) const {
	if (address >= 0xA12000 && address <= 0xA1201F) {
		offset = address & 0x1F;
		return true;
	}

	if (address >= 0xA13000 && address <= 0xA1301F) {
		offset = 0x20 + (address & 0x1F);
		return true;
	}

	if (address >= 0xA14000 && address <= 0xA1401F) {
		offset = 0x40 + (address & 0x1F);
		return true;
	}

	if (address >= 0xA15000 && address <= 0xA1501F) {
		offset = 0x60 + (address & 0x1F);
		return true;
	}

	if (address >= 0xA16000 && address <= 0xA1601F) {
		offset = 0x80 + (address & 0x1F);
		return true;
	}

	if (address >= 0xA18000 && address <= 0xA1801F) {
		offset = 0xA0 + (address & 0x1F);
		return true;
	}

	return false;
}

bool GenesisPlatformBusStub::IsCommandResponseLaneAddress(uint32_t address) const {
	return (address >= 0xA10000 && address <= 0xA1001F)
		|| (address >= 0xA12000 && address <= 0xA1201F)
		|| (address >= 0xA13000 && address <= 0xA1301F)
		|| (address >= 0xA14000 && address <= 0xA1401F)
		|| (address >= 0xA15000 && address <= 0xA1501F)
		|| (address >= 0xA16000 && address <= 0xA1601F)
		|| (address >= 0xA18000 && address <= 0xA1801F);
}

const char* GenesisPlatformBusStub::GetCommandResponseLaneRole(uint32_t address) const {
	if (address >= 0xA12000 && address <= 0xA1200F) {
		return "SCD-G1-CMD";
	}

	if (address >= 0xA12010 && address <= 0xA1201F) {
		return "SCD-G1-RSP";
	}

	if (address >= 0xA13000 && address <= 0xA1300F) {
		return "SCD-G2-CMD";
	}

	if (address >= 0xA13010 && address <= 0xA1301F) {
		return "SCD-G2-RSP";
	}

	if (address >= 0xA14000 && address <= 0xA1400F) {
		return "SCD-G3-CMD";
	}

	if (address >= 0xA14010 && address <= 0xA1401F) {
		return "SCD-G3-RSP";
	}

	if (address >= 0xA15000 && address <= 0xA1500F) {
		return "SCD-G4-CMD";
	}

	if (address >= 0xA15010 && address <= 0xA1501F) {
		return "SCD-G4-RSP";
	}

	if (address >= 0xA16000 && address <= 0xA1600F) {
		return "SCD-G5-CMD";
	}

	if (address >= 0xA16010 && address <= 0xA1601F) {
		return "SCD-G5-RSP";
	}

	if (address >= 0xA18000 && address <= 0xA1800F) {
		return "SCD-G6-CMD";
	}

	if (address >= 0xA18010 && address <= 0xA1801F) {
		return "SCD-G6-RSP";
	}

	return "GEN-IO";
}

void GenesisPlatformBusStub::AppendOwnershipTrace(uint32_t address, GenesisBusOwner owner, bool isWrite, uint8_t value) {
	uint64_t hash = _ownershipTraceDigest.empty() ? FnvOffsetBasis : std::stoull(_ownershipTraceDigest, nullptr, 16);
	uint64_t token = ((uint64_t)(address & 0xFFFFFF) << 16) |
		((uint64_t)(uint8_t)owner << 8) |
		((uint64_t)(isWrite ? 1u : 0u) << 7) |
		(uint64_t)value;
	hash ^= token;
	hash *= FnvPrime;
	_ownershipTraceDigest = ToHexDigest(hash);
	_ownershipTraceCount++;
}

void GenesisPlatformBusStub::AppendCommandResponseLane(uint32_t address, bool isWrite, uint8_t value) {
	_commandResponseSequence++;
	const char* role = GetCommandResponseLaneRole(address);
	string laneEntry = std::format(
		"SCD-LANE seq={} role={} op={} addr={:06x} value={:02x}",
		_commandResponseSequence,
		role,
		isWrite ? "W" : "R",
		address & 0xFFFFFF,
		value);
	_commandResponseLane.push_back(laneEntry);

	uint64_t hash = _commandResponseLaneDigest.empty() ? FnvOffsetBasis : std::stoull(_commandResponseLaneDigest, nullptr, 16);
	for (char ch : laneEntry) {
		hash ^= (uint8_t)ch;
		hash *= FnvPrime;
	}
	_commandResponseLaneDigest = ToHexDigest(hash);
	_commandResponseLaneCount++;
}

uint8_t GenesisPlatformBusStub::BuildControlPortCapabilities(uint8_t port) const {
	if (port > 1) {
		return 0;
	}

	uint8_t capabilities = (uint8_t)(port == 0 ? 0x10 : 0x20);
	capabilities |= 0x01;
	if (_controlThCount[port] >= 4) {
		capabilities |= 0x02;
	}
	if ((_controlThState[port] & 0x40) != 0) {
		capabilities |= 0x04;
	}
	if ((_controlDataPortWrite[port] & 0x40) != 0) {
		capabilities |= 0x08;
	}
	return capabilities;
}

uint8_t GenesisPlatformBusStub::BuildControlPortDigest(uint8_t port) const {
	if (port > 1) {
		return 0;
	}

	uint8_t digest = 0x5a;
	digest ^= BuildControlPortCapabilities(port);
	digest ^= _controlDataPortWrite[port];
	digest ^= (uint8_t)(_controlThState[port] >> 1);
	digest ^= (uint8_t)(_controlThCount[port] * 13u);
	return digest;
}

void GenesisPlatformBusStub::TrackControlDataPortWrite(uint32_t address, uint8_t value) {
	uint8_t port = 0xFF;
	if (address == 0xA10003) {
		port = 0;
	} else if (address == 0xA10005) {
		port = 1;
	}

	if (port > 1) {
		return;
	}

	uint8_t nextThState = (uint8_t)(value & 0x40);
	if (nextThState != _controlThState[port]) {
		_controlThState[port] = nextThState;
		if (_controlThCount[port] < 0xFF) {
			_controlThCount[port]++;
		}
	}

	_controlDataPortWrite[port] = value;
}

void GenesisPlatformBusStub::ClearOwnershipTrace() {
	_ownershipTraceCount = 0;
	_ownershipTraceDigest.clear();
}

void GenesisPlatformBusStub::ClearCommandResponseLane() {
	_commandResponseSequence = 0;
	_commandResponseLaneCount = 0;
	_commandResponseLaneDigest.clear();
	_commandResponseLane.clear();
	_segaCdToolingCapabilities = 0x0F;
	_segaCdToolingDebuggerSignal = 0;
	_segaCdToolingTasSignal = 0;
	_segaCdToolingSaveStateSignal = 0;
	_segaCdToolingCheatSignal = 0;
	_segaCdToolingDigest = 0;
	_segaCdToolingEventCount = 0;
	_controlDataPortWrite = {};
	_controlThState = {};
	_controlThCount = {};
	_m32xMasterSh2Running = false;
	_m32xSlaveSh2Running = false;
	_m32xSh2SyncPhase = 0;
	_m32xSh2Milestone = 0;
	_m32xSh2SyncEpoch = 0;
	_m32xSh2Digest = 0;
	_m32xCompositionBlend = 0;
	_m32xFrameSyncMarker = 0;
	_m32xFrameSyncEpoch = 0;
	_m32xCompositionDigest = 0;
	_m32xToolingDebuggerSignal = 0;
	_m32xToolingTasSignal = 0;
	_m32xToolingSaveStateSignal = 0;
	_m32xToolingCheatSignal = 0;
	_m32xToolingEventCount = 0;
	_m32xToolingDigest = 0;
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
	std::fill(_expansionIo.begin(), _expansionIo.end(), 0);
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
	_ownershipTraceCount = 0;
	_ownershipTraceDigest.clear();
	_commandResponseSequence = 0;
	_commandResponseLaneCount = 0;
	_commandResponseLaneDigest.clear();
	_commandResponseLane.clear();
}

GenesisPlatformBusSaveState GenesisPlatformBusStub::SaveState() const {
	GenesisPlatformBusSaveState state = {};
	state.Rom = _rom;
	state.WorkRam = _workRam;
	state.Io = _io;
	state.ExpansionIo = _expansionIo;
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
	state.OwnershipTraceCount = _ownershipTraceCount;
	state.OwnershipTraceDigest = _ownershipTraceDigest;
	state.CommandResponseSequence = _commandResponseSequence;
	state.CommandResponseLaneCount = _commandResponseLaneCount;
	state.CommandResponseLaneDigest = _commandResponseLaneDigest;
	state.CommandResponseLane = _commandResponseLane;
	state.SegaCdToolingCapabilities = _segaCdToolingCapabilities;
	state.SegaCdToolingDebuggerSignal = _segaCdToolingDebuggerSignal;
	state.SegaCdToolingTasSignal = _segaCdToolingTasSignal;
	state.SegaCdToolingSaveStateSignal = _segaCdToolingSaveStateSignal;
	state.SegaCdToolingCheatSignal = _segaCdToolingCheatSignal;
	state.SegaCdToolingDigest = _segaCdToolingDigest;
	state.SegaCdToolingEventCount = _segaCdToolingEventCount;
	state.ControlDataPortWrite = _controlDataPortWrite;
	state.ControlThState = _controlThState;
	state.ControlThCount = _controlThCount;
	state.M32xMasterSh2Running = _m32xMasterSh2Running;
	state.M32xSlaveSh2Running = _m32xSlaveSh2Running;
	state.M32xSh2SyncPhase = _m32xSh2SyncPhase;
	state.M32xSh2Milestone = _m32xSh2Milestone;
	state.M32xSh2SyncEpoch = _m32xSh2SyncEpoch;
	state.M32xSh2Digest = _m32xSh2Digest;
	state.M32xCompositionBlend = _m32xCompositionBlend;
	state.M32xFrameSyncMarker = _m32xFrameSyncMarker;
	state.M32xFrameSyncEpoch = _m32xFrameSyncEpoch;
	state.M32xCompositionDigest = _m32xCompositionDigest;
	state.M32xToolingDebuggerSignal = _m32xToolingDebuggerSignal;
	state.M32xToolingTasSignal = _m32xToolingTasSignal;
	state.M32xToolingSaveStateSignal = _m32xToolingSaveStateSignal;
	state.M32xToolingCheatSignal = _m32xToolingCheatSignal;
	state.M32xToolingEventCount = _m32xToolingEventCount;
	state.M32xToolingDigest = _m32xToolingDigest;
	return state;
}

void GenesisPlatformBusStub::LoadState(const GenesisPlatformBusSaveState& state) {
	_rom = state.Rom;
	_workRam = state.WorkRam;
	_io = state.Io;
	_expansionIo = state.ExpansionIo;
	if (_expansionIo.size() != 0xC0) {
		_expansionIo.resize(0xC0, 0);
	}
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
	_ownershipTraceCount = state.OwnershipTraceCount;
	_ownershipTraceDigest = state.OwnershipTraceDigest;
	_commandResponseSequence = state.CommandResponseSequence;
	_commandResponseLaneCount = state.CommandResponseLaneCount;
	_commandResponseLaneDigest = state.CommandResponseLaneDigest;
	_commandResponseLane = state.CommandResponseLane;
	_segaCdToolingCapabilities = state.SegaCdToolingCapabilities;
	_segaCdToolingDebuggerSignal = state.SegaCdToolingDebuggerSignal;
	_segaCdToolingTasSignal = state.SegaCdToolingTasSignal;
	_segaCdToolingSaveStateSignal = state.SegaCdToolingSaveStateSignal;
	_segaCdToolingCheatSignal = state.SegaCdToolingCheatSignal;
	_segaCdToolingDigest = state.SegaCdToolingDigest;
	_segaCdToolingEventCount = state.SegaCdToolingEventCount;
	_controlDataPortWrite = state.ControlDataPortWrite;
	_controlThState = state.ControlThState;
	_controlThCount = state.ControlThCount;
	_m32xMasterSh2Running = state.M32xMasterSh2Running;
	_m32xSlaveSh2Running = state.M32xSlaveSh2Running;
	_m32xSh2SyncPhase = state.M32xSh2SyncPhase;
	_m32xSh2Milestone = state.M32xSh2Milestone;
	_m32xSh2SyncEpoch = state.M32xSh2SyncEpoch;
	_m32xSh2Digest = state.M32xSh2Digest;
	_m32xCompositionBlend = state.M32xCompositionBlend;
	_m32xFrameSyncMarker = state.M32xFrameSyncMarker;
	_m32xFrameSyncEpoch = state.M32xFrameSyncEpoch;
	_m32xCompositionDigest = state.M32xCompositionDigest;
	_m32xToolingDebuggerSignal = state.M32xToolingDebuggerSignal;
	_m32xToolingTasSignal = state.M32xToolingTasSignal;
	_m32xToolingSaveStateSignal = state.M32xToolingSaveStateSignal;
	_m32xToolingCheatSignal = state.M32xToolingCheatSignal;
	_m32xToolingEventCount = state.M32xToolingEventCount;
	_m32xToolingDigest = state.M32xToolingDigest;
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
	uint8_t result = 0xFF;

	switch (owner) {
		case GenesisBusOwner::Rom:
			_romReadCount++;
			if (_rom.empty()) {
				result = 0xFF;
				break;
			}
			result = _rom[address % _rom.size()];
			break;

		case GenesisBusOwner::Z80:
			_z80WindowAccessed = true;
			_z80ReadCount++;
			if (_z80Running && !_z80BusRequested) {
				result = 0xFF;
				break;
			}
			result = 0;
			break;

		case GenesisBusOwner::Io: {
			_ioWindowAccessed = true;
			_ioReadCount++;
			if (IsSegaCdToolingStatusAddress(address)) {
				if (address == 0xA12018) {
					result = (uint8_t)(_commandResponseLaneCount & 0xFF);
				} else if (address == 0xA12019) {
					result = GetDigestLowByte(_commandResponseLaneDigest);
				} else if (address == 0xA1201A) {
					result = _segaCdToolingCapabilities;
				} else if (address == 0xA1201B) {
					result = _segaCdToolingDigest;
				} else if (address == 0xA1201C) {
					result = BuildControlPortCapabilities(0);
				} else if (address == 0xA1201D) {
					result = BuildControlPortDigest(0);
				} else if (address == 0xA1201E) {
					result = BuildControlPortCapabilities(1);
				} else {
					result = BuildControlPortDigest(1);
				}
				break;
			}
			if (Is32xSh2StatusAddress(address)) {
				if (address == 0xA1501A) {
					result = 0;
					result |= _m32xMasterSh2Running ? 0x01 : 0x00;
					result |= _m32xSlaveSh2Running ? 0x02 : 0x00;
					result |= (uint8_t)((_m32xSh2SyncPhase & 0x0F) << 2);
				} else {
					result = _m32xSh2Digest;
				}
				break;
			}
			if (Is32xCompositionStatusAddress(address)) {
				if (address == 0xA1501C) {
					result = (uint8_t)(_m32xCompositionBlend & 0x0F);
					result |= (uint8_t)((_m32xFrameSyncEpoch & 0x03) << 6);
				} else {
					result = _m32xCompositionDigest;
				}
				break;
			}
			if (Is32xToolingStatusAddress(address)) {
				if (address == 0xA1501E) {
					result = 0x0F;
				} else {
					result = _m32xToolingDigest;
				}
				break;
			}
			uint32_t expansionOffset = 0;
			if (TryGetExpansionIoOffset(address, expansionOffset)) {
				result = _expansionIo[expansionOffset];
				break;
			}
			if (address == 0xA04000) {
				result = _ym2612AddressPort0;
				break;
			}
			if (address == 0xA04001) {
				result = _ym2612Registers[_ym2612AddressPort0 & 0x1FF];
				break;
			}
			if (address == 0xA04002) {
				result = _ym2612AddressPort1;
				break;
			}
			if (address == 0xA04003) {
				result = _ym2612Registers[(0x100 | _ym2612AddressPort1) & 0x1FF];
				break;
			}
			if (address == 0xA11100) {
				result = _z80BusRequested ? 0x01 : 0x00;
				break;
			}
			if (address == 0xA11200) {
				result = _z80Running ? 0x01 : 0x00;
				break;
			}
			result = _io[address & 0x1F];
			break;
		}

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
			result = _lastVdpValue;
			break;

		case GenesisBusOwner::WorkRam:
			_workRamReadCount++;
			result = _workRam[address & 0xFFFF];
			break;

		case GenesisBusOwner::OpenBus:
		default:
			_openBusReadCount++;
			result = 0xFF;
			break;
	}

	if (owner == GenesisBusOwner::Io && IsCommandResponseLaneAddress(address)) {
		AppendCommandResponseLane(address, false, result);
	}

	AppendOwnershipTrace(address, owner, false, result);
	return result;
}

void GenesisPlatformBusStub::WriteByte(uint32_t address, uint8_t value) {
	address &= 0xFFFFFF;
	GenesisBusOwner owner = DecodeOwner(address);
	AppendOwnershipTrace(address, owner, true, value);

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

		case GenesisBusOwner::Io: {
			_ioWindowAccessed = true;
			_ioWriteCount++;
			if (IsSegaCdToolingControlAddress(address)) {
				uint8_t* target = nullptr;
				if (address == 0xA12012) {
					target = &_segaCdToolingDebuggerSignal;
				} else if (address == 0xA12013) {
					target = &_segaCdToolingTasSignal;
				} else if (address == 0xA12014) {
					target = &_segaCdToolingSaveStateSignal;
				} else if (address == 0xA12015) {
					target = &_segaCdToolingCheatSignal;
				}

				if (target && *target != value) {
					*target = value;
					_segaCdToolingEventCount++;
				}

				uint8_t digest = _segaCdToolingCapabilities;
				digest ^= _segaCdToolingDebuggerSignal;
				digest ^= (uint8_t)(_segaCdToolingTasSignal << 1);
				digest ^= (uint8_t)(_segaCdToolingSaveStateSignal << 2);
				digest ^= (uint8_t)(_segaCdToolingCheatSignal << 3);
				digest ^= (uint8_t)(_segaCdToolingEventCount & 0xFF);
				_segaCdToolingDigest = digest;
			}
			if (Is32xSh2ControlAddress(address)) {
				bool changed = false;
				if (address == 0xA15012) {
					bool nextMaster = (value & 0x01) != 0;
					bool nextSlave = (value & 0x02) != 0;
					changed = nextMaster != _m32xMasterSh2Running || nextSlave != _m32xSlaveSh2Running;
					_m32xMasterSh2Running = nextMaster;
					_m32xSlaveSh2Running = nextSlave;
				} else if (address == 0xA15013) {
					uint8_t phase = (uint8_t)(value & 0x0F);
					changed = phase != _m32xSh2SyncPhase;
					_m32xSh2SyncPhase = phase;
				} else if (address == 0xA15014) {
					changed = value != _m32xSh2Milestone;
					_m32xSh2Milestone = value;
				}

				if (changed) {
					_m32xSh2SyncEpoch++;
				}

				uint8_t digest = 0;
				digest |= _m32xMasterSh2Running ? 0x01 : 0x00;
				digest |= _m32xSlaveSh2Running ? 0x02 : 0x00;
				digest ^= (uint8_t)(_m32xSh2SyncPhase << 2);
				digest ^= _m32xSh2Milestone;
				digest ^= (uint8_t)(_m32xSh2SyncEpoch & 0xFF);
				_m32xSh2Digest = digest;
			}
			if (Is32xCompositionControlAddress(address)) {
				bool changed = false;
				if (address == 0xA15016) {
					uint8_t blend = (uint8_t)(value & 0x0F);
					changed = blend != _m32xCompositionBlend;
					_m32xCompositionBlend = blend;
				} else if (address == 0xA15017) {
					changed = value != _m32xFrameSyncMarker;
					_m32xFrameSyncMarker = value;
				}

				if (changed) {
					_m32xFrameSyncEpoch++;
				}

				uint8_t digest = _m32xSh2Digest;
				digest ^= (uint8_t)(_m32xCompositionBlend << 1);
				digest ^= _m32xFrameSyncMarker;
				digest ^= (uint8_t)(_m32xFrameSyncEpoch & 0xFF);
				_m32xCompositionDigest = digest;
			}
			if (Is32xToolingControlAddress(address)) {
				uint8_t* target = nullptr;
				if (address == 0xA15008) {
					target = &_m32xToolingDebuggerSignal;
				} else if (address == 0xA15009) {
					target = &_m32xToolingTasSignal;
				} else if (address == 0xA1500A) {
					target = &_m32xToolingSaveStateSignal;
				} else if (address == 0xA1500B) {
					target = &_m32xToolingCheatSignal;
				}

				if (target && *target != value) {
					*target = value;
					_m32xToolingEventCount++;
				}

				uint8_t digest = _m32xCompositionDigest;
				digest ^= _m32xToolingDebuggerSignal;
				digest ^= (uint8_t)(_m32xToolingTasSignal << 1);
				digest ^= (uint8_t)(_m32xToolingSaveStateSignal << 2);
				digest ^= (uint8_t)(_m32xToolingCheatSignal << 3);
				digest ^= (uint8_t)(_m32xToolingEventCount & 0xFF);
				_m32xToolingDigest = digest;
			}
			uint32_t expansionOffset = 0;
			if (TryGetExpansionIoOffset(address, expansionOffset)) {
				_expansionIo[expansionOffset] = value;
			} else {
				_io[address & 0x1F] = value;
				TrackControlDataPortWrite(address, value);
			}
			if (IsCommandResponseLaneAddress(address)) {
				AppendCommandResponseLane(address, true, value);
			}
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
		}

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
