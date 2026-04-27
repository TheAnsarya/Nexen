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
	memset(_segaCdBridgeA120, 0, sizeof(_segaCdBridgeA120));
	memset(_segaCdBridgeA130, 0, sizeof(_segaCdBridgeA130));
	memset(_segaCdBridgeA140, 0, sizeof(_segaCdBridgeA140));
	memset(_segaCdBridgeA150, 0, sizeof(_segaCdBridgeA150));
	memset(_segaCdBridgeA160, 0, sizeof(_segaCdBridgeA160));
	memset(_segaCdBridgeA180, 0, sizeof(_segaCdBridgeA180));

	_z80BusRequest = false;
	_z80Reset = true;
	_segaCdSubCpuRunning = false;
	_segaCdSubCpuBusRequest = false;
	_segaCdSubCpuTransitionCount = 0;
	_segaCdPcmLeft = 0;
	_segaCdPcmRight = 0;
	_segaCdCddaLeft = 0;
	_segaCdCddaRight = 0;
	_segaCdMixedLeft = 0;
	_segaCdMixedRight = 0;
	_segaCdAudioCheckpointCount = 0;
	_segaCdToolingDebuggerSignal = 0;
	_segaCdToolingTasSignal = 0;
	_segaCdToolingSaveStateSignal = 0;
	_segaCdToolingCheatSignal = 0;
	_segaCdToolingEventCount = 0;
	_segaCdToolingDigest = 0;
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

	_hasSram = false;
	_sramStart = 0;
	_sramEnd = 0;
	_ioState.DebugTranscriptLaneCount = 0;
	_ioState.DebugTranscriptLaneDigest = 0;
	for (int i = 0; i < 4; i++) {
		_ioState.DebugTranscriptEntryAddress[i] = 0;
		_ioState.DebugTranscriptEntryValue[i] = 0;
		_ioState.DebugTranscriptEntryFlags[i] = 0;
	}
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

bool GenesisMemoryManager::TryGetSegaCdBridgeSlot(uint32_t addr, uint8_t*& slot, uint32_t& slotIndex) {
	if (addr >= 0xA12000 && addr <= 0xA1201F) {
		slot = &_segaCdBridgeA120[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA13000 && addr <= 0xA1301F) {
		slot = &_segaCdBridgeA130[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA14000 && addr <= 0xA1401F) {
		slot = &_segaCdBridgeA140[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA15000 && addr <= 0xA1501F) {
		slot = &_segaCdBridgeA150[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA16000 && addr <= 0xA1601F) {
		slot = &_segaCdBridgeA160[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	if (addr >= 0xA18000 && addr <= 0xA1801F) {
		slot = &_segaCdBridgeA180[0];
		slotIndex = addr & 0x1F;
		return true;
	}

	return false;
}

bool GenesisMemoryManager::IsSegaCdSubCpuControlAddress(uint32_t addr) const {
	return addr == 0xA12000 || addr == 0xA12001;
}

bool GenesisMemoryManager::IsSegaCdAudioDataAddress(uint32_t addr) const {
	return addr >= 0xA12002 && addr <= 0xA12005;
}

bool GenesisMemoryManager::IsSegaCdAudioStatusAddress(uint32_t addr) const {
	return addr == 0xA12010 || addr == 0xA12011;
}

bool GenesisMemoryManager::IsSegaCdToolingControlAddress(uint32_t addr) const {
	return addr >= 0xA12012 && addr <= 0xA12015;
}

bool GenesisMemoryManager::IsSegaCdToolingStatusAddress(uint32_t addr) const {
	return addr >= 0xA12018 && addr <= 0xA1201F;
}

bool GenesisMemoryManager::Is32xSh2ControlAddress(uint32_t addr) const {
	return addr == 0xA15012 || addr == 0xA15013 || addr == 0xA15014;
}

bool GenesisMemoryManager::Is32xSh2StatusAddress(uint32_t addr) const {
	return addr == 0xA1501A || addr == 0xA1501B;
}

bool GenesisMemoryManager::Is32xCompositionControlAddress(uint32_t addr) const {
	return addr == 0xA15016 || addr == 0xA15017;
}

bool GenesisMemoryManager::Is32xCompositionStatusAddress(uint32_t addr) const {
	return addr == 0xA1501C || addr == 0xA1501D;
}

bool GenesisMemoryManager::Is32xToolingControlAddress(uint32_t addr) const {
	return addr >= 0xA15008 && addr <= 0xA1500B;
}

bool GenesisMemoryManager::Is32xToolingStatusAddress(uint32_t addr) const {
	return addr >= 0xA15018 && addr <= 0xA1501F;
}

void GenesisMemoryManager::UpdateSegaCdSubCpuControl(uint8_t value) {
	bool running = (value & 0x01) != 0;
	bool busRequest = (value & 0x02) != 0;
	if (running != _segaCdSubCpuRunning || busRequest != _segaCdSubCpuBusRequest) {
		_segaCdSubCpuTransitionCount++;
	}
	_segaCdSubCpuRunning = running;
	_segaCdSubCpuBusRequest = busRequest;
}

uint8_t GenesisMemoryManager::GetSegaCdSubCpuStatusByte() const {
	uint8_t status = 0;
	if (_segaCdSubCpuRunning) {
		status |= 0x01;
	}
	if (_segaCdSubCpuBusRequest) {
		status |= 0x02;
	}
	status |= (uint8_t)((_segaCdSubCpuTransitionCount & 0x0F) << 4);
	return status;
}

void GenesisMemoryManager::UpdateSegaCdAudioPath(uint32_t addr, uint8_t value) {
	if (addr == 0xA12002) {
		_segaCdPcmLeft = value;
	} else if (addr == 0xA12003) {
		_segaCdPcmRight = value;
	} else if (addr == 0xA12004) {
		_segaCdCddaLeft = value;
	} else if (addr == 0xA12005) {
		_segaCdCddaRight = value;
	} else {
		return;
	}

	int16_t mixedLeft = (int16_t)(int8_t)_segaCdPcmLeft + (int16_t)(int8_t)_segaCdCddaLeft;
	int16_t mixedRight = (int16_t)(int8_t)_segaCdPcmRight + (int16_t)(int8_t)_segaCdCddaRight;
	mixedLeft = std::clamp<int16_t>(mixedLeft, -128, 127);
	mixedRight = std::clamp<int16_t>(mixedRight, -128, 127);
	_segaCdMixedLeft = (uint8_t)(int8_t)mixedLeft;
	_segaCdMixedRight = (uint8_t)(int8_t)mixedRight;
	_segaCdAudioCheckpointCount++;
}

uint8_t GenesisMemoryManager::GetSegaCdAudioStatusByte(uint32_t addr) const {
	if (addr == 0xA12010) {
		return _segaCdMixedLeft;
	}
	if (addr == 0xA12011) {
		return _segaCdMixedRight;
	}
	return 0;
}

void GenesisMemoryManager::UpdateSegaCdToolingContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA12012) {
		target = &_segaCdToolingDebuggerSignal;
	} else if (addr == 0xA12013) {
		target = &_segaCdToolingTasSignal;
	} else if (addr == 0xA12014) {
		target = &_segaCdToolingSaveStateSignal;
	} else if (addr == 0xA12015) {
		target = &_segaCdToolingCheatSignal;
	}

	if (!target) {
		return;
	}

	if (*target != value) {
		*target = value;
		_segaCdToolingEventCount++;
	}

	uint8_t digest = 0x0F;
	digest ^= _segaCdToolingDebuggerSignal;
	digest ^= (uint8_t)(_segaCdToolingTasSignal << 1);
	digest ^= (uint8_t)(_segaCdToolingSaveStateSignal << 2);
	digest ^= (uint8_t)(_segaCdToolingCheatSignal << 3);
	digest ^= (uint8_t)(_segaCdToolingEventCount & 0xFF);
	_segaCdToolingDigest = digest;
}

uint8_t GenesisMemoryManager::GetSegaCdToolingStatusByte(uint32_t addr) const {
	if (addr == 0xA12018) {
		return (uint8_t)(_ioState.DebugTranscriptLaneCount & 0xFF);
	}
	if (addr == 0xA12019) {
		return (uint8_t)(_ioState.DebugTranscriptLaneDigest & 0xFF);
	}
	if (addr == 0xA1201A) {
		return 0x0F;
	}
	if (addr == 0xA1201B) {
		return _segaCdToolingDigest;
	}
	if (addr == 0xA1201C) {
		return _controlManager ? _controlManager->GetDeterministicPortCapabilities(0) : 0;
	}
	if (addr == 0xA1201D) {
		return _controlManager ? _controlManager->GetDeterministicPortDigest(0) : 0;
	}
	if (addr == 0xA1201E) {
		return _controlManager ? _controlManager->GetDeterministicPortCapabilities(1) : 0;
	}
	if (addr == 0xA1201F) {
		return _controlManager ? _controlManager->GetDeterministicPortDigest(1) : 0;
	}
	return 0;
}

void GenesisMemoryManager::Update32xSh2Staging(uint32_t addr, uint8_t value) {
	bool changed = false;
	if (addr == 0xA15012) {
		bool nextMaster = (value & 0x01) != 0;
		bool nextSlave = (value & 0x02) != 0;
		changed = nextMaster != _m32xMasterSh2Running || nextSlave != _m32xSlaveSh2Running;
		_m32xMasterSh2Running = nextMaster;
		_m32xSlaveSh2Running = nextSlave;
	} else if (addr == 0xA15013) {
		uint8_t phase = (uint8_t)(value & 0x0F);
		changed = phase != _m32xSh2SyncPhase;
		_m32xSh2SyncPhase = phase;
	} else if (addr == 0xA15014) {
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

uint8_t GenesisMemoryManager::Get32xSh2StatusByte(uint32_t addr) const {
	if (addr == 0xA1501A) {
		uint8_t status = 0;
		if (_m32xMasterSh2Running) {
			status |= 0x01;
		}
		if (_m32xSlaveSh2Running) {
			status |= 0x02;
		}
		status |= (uint8_t)((_m32xSh2SyncPhase & 0x0F) << 2);
		return status;
	}
	if (addr == 0xA1501B) {
		return _m32xSh2Digest;
	}
	return 0;
}

void GenesisMemoryManager::Update32xCompositionStaging(uint32_t addr, uint8_t value) {
	bool changed = false;
	if (addr == 0xA15016) {
		uint8_t blend = (uint8_t)(value & 0x0F);
		changed = blend != _m32xCompositionBlend;
		_m32xCompositionBlend = blend;
	} else if (addr == 0xA15017) {
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

uint8_t GenesisMemoryManager::Get32xCompositionStatusByte(uint32_t addr) const {
	if (addr == 0xA1501C) {
		uint8_t status = (uint8_t)(_m32xCompositionBlend & 0x0F);
		status |= (uint8_t)((_m32xFrameSyncEpoch & 0x03) << 6);
		return status;
	}
	if (addr == 0xA1501D) {
		return _m32xCompositionDigest;
	}
	return 0;
}

void GenesisMemoryManager::Update32xToolingContract(uint32_t addr, uint8_t value) {
	uint8_t* target = nullptr;
	if (addr == 0xA15008) {
		target = &_m32xToolingDebuggerSignal;
	} else if (addr == 0xA15009) {
		target = &_m32xToolingTasSignal;
	} else if (addr == 0xA1500A) {
		target = &_m32xToolingSaveStateSignal;
	} else if (addr == 0xA1500B) {
		target = &_m32xToolingCheatSignal;
	}

	if (!target) {
		return;
	}

	if (*target != value) {
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

uint8_t GenesisMemoryManager::Get32xToolingStatusByte(uint32_t addr) const {
	if (addr == 0xA15018) {
		return (uint8_t)(_m32xToolingEventCount & 0xFF);
	}
	if (addr == 0xA15019) {
		return (uint8_t)((_m32xToolingEventCount >> 8) & 0xFF);
	}
	if (addr == 0xA1501E) {
		return 0x0F;
	}
	if (addr == 0xA1501F) {
		return _m32xToolingDigest;
	}
	return 0;
}

void GenesisMemoryManager::TrackSegaCdTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t roleFlags = 0;
	if ((addr & 0x10) != 0) {
		roleFlags |= 0x02;
	}
	if (addr >= 0xA13000 && addr <= 0xA1301F) {
		roleFlags |= 0x04;
	} else if (addr >= 0xA14000 && addr <= 0xA1401F) {
		roleFlags |= 0x08;
	} else if (addr >= 0xA15000 && addr <= 0xA1501F) {
		roleFlags |= 0x10;
	} else if (addr >= 0xA16000 && addr <= 0xA1601F) {
		roleFlags |= 0x20;
	} else if (addr >= 0xA18000 && addr <= 0xA1801F) {
		roleFlags |= 0x40;
	}

	TrackTranscriptEntry(addr, isWrite, value, roleFlags);
}

void GenesisMemoryManager::TrackSegaCdHandshakeTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t roleFlags = 0x80;
	if (addr >= 0xA11200 && addr <= 0xA11201) {
		roleFlags |= 0x04;
	}
	if (!isWrite) {
		roleFlags |= 0x02;
	}

	TrackTranscriptEntry(addr, isWrite, value, roleFlags);
}

void GenesisMemoryManager::TrackTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;

	if (isWrite) {
		roleFlags |= 0x01;
	}

	uint64_t hash = _ioState.TranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.TranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= value;
	hash *= FnvPrime;
	hash ^= roleFlags;
	hash *= FnvPrime;
	_ioState.TranscriptLaneDigest = hash;

	uint32_t index = _ioState.TranscriptLaneCount % 4;
	_ioState.TranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.TranscriptEntryValue[index] = value;
	_ioState.TranscriptEntryFlags[index] = roleFlags;
	_ioState.TranscriptLaneCount++;
}

void GenesisMemoryManager::TrackDebugTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;

	if (isWrite) {
		roleFlags |= 0x01;
	}
	roleFlags |= 0x40;

	uint64_t hash = _ioState.DebugTranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.DebugTranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= value;
	hash *= FnvPrime;
	hash ^= roleFlags;
	hash *= FnvPrime;
	_ioState.DebugTranscriptLaneDigest = hash;

	uint32_t index = _ioState.DebugTranscriptLaneCount % 4;
	_ioState.DebugTranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.DebugTranscriptEntryValue[index] = value;
	_ioState.DebugTranscriptEntryFlags[index] = roleFlags;
	_ioState.DebugTranscriptLaneCount++;
}

void GenesisMemoryManager::ClearDebugTranscriptLane() {
	_ioState.DebugTranscriptLaneCount = 0;
	_ioState.DebugTranscriptLaneDigest = 0;
	for (uint32_t i = 0; i < 4; i++) {
		_ioState.DebugTranscriptEntryAddress[i] = 0;
		_ioState.DebugTranscriptEntryValue[i] = 0;
		_ioState.DebugTranscriptEntryFlags[i] = 0;
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

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		uint8_t value = _saveRam[sramOffset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
		_openBus = value;
		return value;
	}

	if (addr < 0x400000) [[likely]] {
		// Cartridge ROM
		if (addr < _prgRomSize) {
			uint8_t value = _prgRom[addr];
			_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
			_openBus = value;
			return value;
		}
		return _openBus;
	}

	if (addr >= 0xFF0000) [[likely]] {
		// Work RAM
		uint32_t offset = addr & 0xFFFF;
		uint8_t value = _workRam[offset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, value, MemoryOperationType::Read);
		_openBus = value;
		return value;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		// VDP ports (byte access from word port)
		uint16_t word = ReadVdpPort(addr);
		if (addr & 1) return (uint8_t)(word & 0xFF);
		return (uint8_t)(word >> 8);
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		// Z80 address space
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			return _z80Ram[z80Addr];
		}
		return _openBus;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		return ReadIo(addr);
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t value = bridgeSlot[bridgeIndex];
		if (IsSegaCdSubCpuControlAddress(addr)) {
			value = GetSegaCdSubCpuStatusByte();
		} else if (IsSegaCdAudioStatusAddress(addr)) {
			value = GetSegaCdAudioStatusByte(addr);
		} else if (IsSegaCdToolingStatusAddress(addr)) {
			value = GetSegaCdToolingStatusByte(addr);
		} else if (Is32xSh2StatusAddress(addr)) {
			value = Get32xSh2StatusByte(addr);
		} else if (Is32xCompositionStatusAddress(addr)) {
			value = Get32xCompositionStatusByte(addr);
		} else if (Is32xToolingStatusAddress(addr)) {
			value = Get32xToolingStatusByte(addr);
		}
		TrackSegaCdTranscript(addr, false, value);
		_openBus = value;
		return value;
	}

	if (addr == 0xA11100) [[unlikely]] {
		// Z80 bus request — bit 0 indicates bus granted
		uint8_t value = _z80BusRequest ? 0x00 : 0x01;
		TrackSegaCdHandshakeTranscript(addr, false, value);
		return value;
	}

	if (addr == 0xA11101) [[unlikely]] {
		uint8_t value = _z80BusRequest ? 0x00 : 0x01;
		TrackSegaCdHandshakeTranscript(addr, false, value);
		return value;
	}

	if (addr == 0xA11200 || addr == 0xA11201) [[unlikely]] {
		TrackSegaCdHandshakeTranscript(addr, false, _openBus);
		return _openBus;
	}

	return _openBus;
}

uint16_t GenesisMemoryManager::Read16(uint32_t addr) {
	addr &= 0xFFFFFE;
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		uint8_t hi = Read8(addr);
		uint8_t lo = Read8(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr < 0x400000) [[likely]] {
		if (addr + 1 < _prgRomSize) {
			uint16_t value = ((uint16_t)_prgRom[addr] << 8) | _prgRom[addr + 1];
			_emu->ProcessMemoryRead<CpuType::Genesis>(addr, _prgRom[addr], MemoryOperationType::Read);
			_openBus = _prgRom[addr + 1];
			return value;
		}
		return (_openBus << 8) | _openBus;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint16_t value = ((uint16_t)_workRam[offset] << 8) | _workRam[(offset + 1) & 0xFFFF];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, _workRam[offset], MemoryOperationType::Read);
		_openBus = _workRam[(offset + 1) & 0xFFFF];
		return value;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		return ReadVdpPort(addr);
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			return ((uint16_t)_z80Ram[z80Addr] << 8) | _z80Ram[z80Addr];
		}
		return (_openBus << 8) | _openBus;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		uint8_t hi = ReadIo(addr);
		uint8_t lo = ReadIo(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t hi = Read8(addr);
		uint8_t lo = Read8(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr == 0xA11100) [[unlikely]] {
		uint16_t value = _z80BusRequest ? 0x0000 : 0x0100;
		TrackSegaCdHandshakeTranscript(addr, false, (uint8_t)(value >> 8));
		return value;
	}

	if (addr == 0xA11200) [[unlikely]] {
		TrackSegaCdHandshakeTranscript(addr, false, _openBus);
		return (_openBus << 8) | _openBus;
	}

	return (_openBus << 8) | _openBus;
}

void GenesisMemoryManager::Write8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	uint32_t sramOffset = 0;

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, value, MemoryOperationType::Write);
		_saveRam[sramOffset] = value;
		return;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, value, MemoryOperationType::Write);
		_workRam[offset] = value;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		// VDP byte write - promote to word write
		WriteVdpPort(addr, (uint16_t)value | ((uint16_t)value << 8));
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = value;
		}
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		WriteIo(addr, value);
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		bridgeSlot[bridgeIndex] = value;
		if (IsSegaCdSubCpuControlAddress(addr)) {
			UpdateSegaCdSubCpuControl(value);
		} else if (IsSegaCdAudioDataAddress(addr)) {
			UpdateSegaCdAudioPath(addr, value);
		} else if (IsSegaCdToolingControlAddress(addr)) {
			UpdateSegaCdToolingContract(addr, value);
		} else if (Is32xSh2ControlAddress(addr)) {
			Update32xSh2Staging(addr, value);
		} else if (Is32xCompositionControlAddress(addr)) {
			Update32xCompositionStaging(addr, value);
		} else if (Is32xToolingControlAddress(addr)) {
			Update32xToolingContract(addr, value);
		}
		TrackSegaCdTranscript(addr, true, value);
		return;
	}

	if (addr == 0xA11100 || addr == 0xA11101) [[unlikely]] {
		_z80BusRequest = (value & 0x01) != 0;
		TrackSegaCdHandshakeTranscript(addr, true, value);
		return;
	}

	if (addr == 0xA11200 || addr == 0xA11201) [[unlikely]] {
		_z80Reset = !(value & 0x01);
		TrackSegaCdHandshakeTranscript(addr, true, value);
		return;
	}

	// ROM area — ignore writes (no mapper for now)
}

void GenesisMemoryManager::Write16(uint32_t addr, uint16_t value) {
	addr &= 0xFFFFFE;
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		Write8(addr, (uint8_t)(value >> 8));
		Write8(addr + 1, (uint8_t)(value & 0xFF));
		return;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t highByte = (uint8_t)(value >> 8);
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, highByte, MemoryOperationType::Write);
		_workRam[offset] = (uint8_t)(value >> 8);
		_workRam[(offset + 1) & 0xFFFF] = (uint8_t)(value & 0xFF);
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		WriteVdpPort(addr, value);
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = (uint8_t)(value >> 8);
		}
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		WriteIo(addr, (uint8_t)(value >> 8));
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		Write8(addr, (uint8_t)(value >> 8));
		Write8(addr + 1, (uint8_t)(value & 0xFF));
		return;
	}

	if (addr == 0xA11100) [[unlikely]] {
		_z80BusRequest = (value & 0x0100) != 0;
		TrackSegaCdHandshakeTranscript(addr, true, (uint8_t)(value >> 8));
		return;
	}

	if (addr == 0xA11200) [[unlikely]] {
		_z80Reset = !(value & 0x0100);
		TrackSegaCdHandshakeTranscript(addr, true, (uint8_t)(value >> 8));
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
	if (addr < _prgRomSize) {
		return _prgRom[addr];
	}
	if (addr >= 0xFF0000) {
		return _workRam[addr & 0xFFFF];
	}
	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		uint32_t reg = addr & 0x1F;
		uint8_t value = _openBus;
		switch (reg) {
			case 0x01:
				value = 0xA0;
				break;
			case 0x03:
				value = _controlManager ? _controlManager->ReadDataPort(0) : 0x7F;
				break;
			case 0x05:
				value = _controlManager ? _controlManager->ReadDataPort(1) : 0x7F;
				break;
			case 0x07:
				value = 0xFF;
				break;
			case 0x09:
				value = _ioState.CtrlPort[0];
				break;
			case 0x0B:
				value = _ioState.CtrlPort[1];
				break;
			case 0x0D:
				value = _ioState.CtrlPort[2];
				break;
			default:
				value = _openBus;
				break;
		}
		TrackDebugTranscriptEntry(addr, false, value, 0x10);
		return value;
	}
	if (addr == 0xA11100 || addr == 0xA11101) {
		uint8_t value = _z80BusRequest ? 0x00 : 0x01;
		TrackSegaCdHandshakeTranscript(addr, false, value);
		TrackDebugTranscriptEntry(addr, false, value, 0x80);
		return value;
	}
	if (addr == 0xA11200 || addr == 0xA11201) {
		TrackSegaCdHandshakeTranscript(addr, false, _openBus);
		TrackDebugTranscriptEntry(addr, false, _openBus, 0x84);
		return _openBus;
	}
	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		uint8_t value = _openBus;
		if (_z80BusRequest || _z80Reset) {
			value = _z80Ram[addr & 0x1FFF];
		}
		TrackDebugTranscriptEntry(addr, false, value, 0x20);
		return value;
	}
	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		uint8_t value = _openBus;
		if (_vdp) {
			uint16_t word = ReadVdpPort(addr);
			if (addr & 1) {
				value = (uint8_t)(word & 0xFF);
			} else {
				value = (uint8_t)(word >> 8);
			}
		}
		TrackDebugTranscriptEntry(addr, false, value, 0x30);
		return value;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) {
		if (IsSegaCdSubCpuControlAddress(addr)) {
			uint8_t value = GetSegaCdSubCpuStatusByte();
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		if (IsSegaCdAudioStatusAddress(addr)) {
			uint8_t value = GetSegaCdAudioStatusByte(addr);
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		if (IsSegaCdToolingStatusAddress(addr)) {
			uint8_t value = GetSegaCdToolingStatusByte(addr);
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		if (Is32xSh2StatusAddress(addr)) {
			uint8_t value = Get32xSh2StatusByte(addr);
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		if (Is32xCompositionStatusAddress(addr)) {
			uint8_t value = Get32xCompositionStatusByte(addr);
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		if (Is32xToolingStatusAddress(addr)) {
			uint8_t value = Get32xToolingStatusByte(addr);
			TrackSegaCdTranscript(addr, false, value);
			TrackDebugTranscriptEntry(addr, false, value, 0x02);
			return value;
		}
		uint8_t value = bridgeSlot[bridgeIndex];
		TrackSegaCdTranscript(addr, false, value);
		TrackDebugTranscriptEntry(addr, false, value, 0x02);
		return value;
	}
	return 0;
}

void GenesisMemoryManager::DebugWrite8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	if (addr >= 0xFF0000) {
		_workRam[addr & 0xFFFF] = value;
		return;
	}
	if (addr >= 0xA10000 && addr <= 0xA1001F) {
		uint32_t reg = addr & 0x1F;
		switch (reg) {
			case 0x03:
				if (_controlManager) {
					_controlManager->WriteDataPort(0, value);
				}
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
			case 0x05:
				if (_controlManager) {
					_controlManager->WriteDataPort(1, value);
				}
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
			case 0x09:
				_ioState.CtrlPort[0] = value;
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
			case 0x0B:
				_ioState.CtrlPort[1] = value;
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
			case 0x0D:
				_ioState.CtrlPort[2] = value;
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
			default:
				TrackDebugTranscriptEntry(addr, true, value, 0x10);
				return;
		}
	}
	if (addr == 0xA11100 || addr == 0xA11101) {
		_z80BusRequest = (value & 0x01) != 0;
		TrackSegaCdHandshakeTranscript(addr, true, value);
		TrackDebugTranscriptEntry(addr, true, value, 0x80);
		return;
	}
	if (addr == 0xA11200 || addr == 0xA11201) {
		_z80Reset = !(value & 0x01);
		TrackSegaCdHandshakeTranscript(addr, true, value);
		TrackDebugTranscriptEntry(addr, true, value, 0x84);
		return;
	}
	if (addr >= 0xA00000 && addr <= 0xA0FFFF) {
		if (_z80BusRequest || _z80Reset) {
			_z80Ram[addr & 0x1FFF] = value;
		}
		TrackDebugTranscriptEntry(addr, true, value, 0x20);
		return;
	}
	if (addr >= 0xC00000 && addr <= 0xC0001F) {
		if (_vdp) {
			WriteVdpPort(addr, (uint16_t)value | ((uint16_t)value << 8));
		}
		TrackDebugTranscriptEntry(addr, true, value, 0x30);
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) {
		bridgeSlot[bridgeIndex] = value;
		if (IsSegaCdSubCpuControlAddress(addr)) {
			UpdateSegaCdSubCpuControl(value);
		} else if (IsSegaCdAudioDataAddress(addr)) {
			UpdateSegaCdAudioPath(addr, value);
		} else if (IsSegaCdToolingControlAddress(addr)) {
			UpdateSegaCdToolingContract(addr, value);
		} else if (Is32xSh2ControlAddress(addr)) {
			Update32xSh2Staging(addr, value);
		} else if (Is32xCompositionControlAddress(addr)) {
			Update32xCompositionStaging(addr, value);
		} else if (Is32xToolingControlAddress(addr)) {
			Update32xToolingContract(addr, value);
		}
		TrackSegaCdTranscript(addr, true, value);
		TrackDebugTranscriptEntry(addr, true, value, 0x00);
	}
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
	SV(_segaCdSubCpuRunning);
	SV(_segaCdSubCpuBusRequest);
	SV(_segaCdSubCpuTransitionCount);
	SV(_segaCdPcmLeft);
	SV(_segaCdPcmRight);
	SV(_segaCdCddaLeft);
	SV(_segaCdCddaRight);
	SV(_segaCdMixedLeft);
	SV(_segaCdMixedRight);
	SV(_segaCdAudioCheckpointCount);
	SV(_segaCdToolingDebuggerSignal);
	SV(_segaCdToolingTasSignal);
	SV(_segaCdToolingSaveStateSignal);
	SV(_segaCdToolingCheatSignal);
	SV(_segaCdToolingEventCount);
	SV(_segaCdToolingDigest);
	SV(_m32xMasterSh2Running);
	SV(_m32xSlaveSh2Running);
	SV(_m32xSh2SyncPhase);
	SV(_m32xSh2Milestone);
	SV(_m32xSh2SyncEpoch);
	SV(_m32xSh2Digest);
	SV(_m32xCompositionBlend);
	SV(_m32xFrameSyncMarker);
	SV(_m32xFrameSyncEpoch);
	SV(_m32xCompositionDigest);
	SV(_m32xToolingDebuggerSignal);
	SV(_m32xToolingTasSignal);
	SV(_m32xToolingSaveStateSignal);
	SV(_m32xToolingCheatSignal);
	SV(_m32xToolingEventCount);
	SV(_m32xToolingDigest);
	SVArray(_segaCdBridgeA120, (uint32_t)sizeof(_segaCdBridgeA120));
	SVArray(_segaCdBridgeA130, (uint32_t)sizeof(_segaCdBridgeA130));
	SVArray(_segaCdBridgeA140, (uint32_t)sizeof(_segaCdBridgeA140));
	SVArray(_segaCdBridgeA150, (uint32_t)sizeof(_segaCdBridgeA150));
	SVArray(_segaCdBridgeA160, (uint32_t)sizeof(_segaCdBridgeA160));
	SVArray(_segaCdBridgeA180, (uint32_t)sizeof(_segaCdBridgeA180));
	SV(_ioState.DataPort[0]); SV(_ioState.DataPort[1]); SV(_ioState.DataPort[2]);
	SV(_ioState.CtrlPort[0]); SV(_ioState.CtrlPort[1]); SV(_ioState.CtrlPort[2]);
	SV(_ioState.TranscriptLaneCount);
	SV(_ioState.TranscriptLaneDigest);
	for (uint32_t i = 0; i < 4; i++) {
		SV(_ioState.TranscriptEntryAddress[i]);
		SV(_ioState.TranscriptEntryValue[i]);
		SV(_ioState.TranscriptEntryFlags[i]);
	}
	SV(_ioState.DebugTranscriptLaneCount);
	SV(_ioState.DebugTranscriptLaneDigest);
	for (uint32_t i = 0; i < 4; i++) {
		SV(_ioState.DebugTranscriptEntryAddress[i]);
		SV(_ioState.DebugTranscriptEntryValue[i]);
		SV(_ioState.DebugTranscriptEntryFlags[i]);
	}
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
