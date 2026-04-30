#include "pch.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Genesis/GenesisPsg.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"
#include "Shared/BatteryManager.h"
#include "Shared/MessageManager.h"
#include "Utilities/Serializer.h"

namespace {
	__forceinline uint32_t ReadBe32(const vector<uint8_t>& data, size_t offset) {
		size_t effectiveOffset = offset;
		uint32_t byte0 = (uint32_t)data[effectiveOffset];
		uint32_t byte1 = (uint32_t)data[effectiveOffset + 1];
		uint32_t byte2 = (uint32_t)data[effectiveOffset + 2];
		uint32_t byte3 = (uint32_t)data[effectiveOffset + 3];
		uint32_t value = (byte0 << 24)
			| (byte1 << 16)
			| (byte2 << 8)
			| byte3;
		return value;
	}

	__forceinline bool IsZ80BusReqAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isBusReqAddress = (effectiveAddr & 0xFFFF00) == 0xA11100;
		return isBusReqAddress;
	}

	__forceinline bool IsZ80ResetAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isResetAddress = (effectiveAddr & 0xFFFF00) == 0xA11200;
		return isResetAddress;
	}

	__forceinline bool IsYm2612Address(uint32_t addr) {
		uint32_t effectiveAddr = addr & 0xFFFFFF;
		bool isYm2612Address = effectiveAddr >= 0xA04000 && effectiveAddr <= 0xA04003;
		return isYm2612Address;
	}

	__forceinline uint8_t GetZ80BusAckStatusBit(bool busRequested, bool resetAsserted) {
		// BUSACK bit is low only while the 68k has requested the Z80 bus and Z80 is not in reset.
		uint8_t ackStatus = (busRequested && !resetAsserted) ? 0x00 : 0x01;
		return ackStatus;
	}

	__forceinline bool IsTmssAddress(uint32_t addr) {
		uint32_t effectiveAddr = addr;
		bool isTmssAddress = effectiveAddr >= 0xA14000 && effectiveAddr <= 0xA14003;
		return isTmssAddress;
	}

	__forceinline uint8_t BuildVersionRegister(ConsoleRegion region) {
		uint8_t versionByte = 0xA0; // Overseas + base hardware profile
		if (region == ConsoleRegion::Pal) {
			versionByte |= 0x40;
		} else {
			versionByte &= (uint8_t)~0x40;
		}
		return versionByte;
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
	_tmssEnabled = _emu->GetSettings()->GetGenesisConfig().EnableTmss;
	_tmssUnlocked = false;
	_tmssVdpBlockLogged = false;
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
	_ioState.RomReadHeartbeat = 0;
	_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
	_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
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

	ResetRomBankMapper();
}

void GenesisMemoryManager::ResetRomBankMapper() {
	uint32_t bankCount = (_prgRomSize + MapperWindowSize - 1) / MapperWindowSize;
	if (bankCount == 0) {
		bankCount = 1;
	}

	_romBankMapperEnabled = _prgRomSize > MapperWindowSize;
	for (uint32_t i = 0; i < MapperBankWindowCount; i++) {
		_romBankRegisters[i] = (uint8_t)((i + 1) % bankCount);
	}
}

void GenesisMemoryManager::UpdateExecutionHeartbeat(uint32_t programCounter, uint64_t cycleCount) {
	_ioState.CpuProgramCounterHeartbeat = programCounter & 0x00ffffff;
	_ioState.CpuCycleHeartbeat = cycleCount;
	_ioState.CpuInstructionHeartbeat++;
}

bool GenesisMemoryManager::TryGetRomBankRegisterSlot(uint32_t addr, uint8_t& slot) const {
	if ((addr & 0x01) == 0) {
		return false;
	}

	if (addr < 0xA130F3 || addr > 0xA130FF) {
		return false;
	}

	slot = (uint8_t)((addr - 0xA130F3) >> 1);
	return slot < MapperBankWindowCount;
}

bool GenesisMemoryManager::TryWriteRomBankRegister(uint32_t addr, uint8_t value) {
	uint8_t slot = 0;
	if (!TryGetRomBankRegisterSlot(addr, slot)) {
		return false;
	}

	uint8_t effectiveValue = value;
	_romBankRegisters[slot] = effectiveValue;
	return true;
}

uint32_t GenesisMemoryManager::TranslateRomAddress(uint32_t addr) const {
	if (_prgRomSize == 0) {
		return 0;
	}

	uint32_t effectiveAddr = addr & 0x3FFFFF;
	uint32_t bankCount = (_prgRomSize + MapperWindowSize - 1) / MapperWindowSize;
	if (bankCount == 0) {
		bankCount = 1;
	}

	if (_romBankMapperEnabled && effectiveAddr >= 0x080000 && effectiveAddr < 0x400000) {
		uint32_t windowOffset = effectiveAddr - 0x080000;
		uint32_t slot = windowOffset / MapperWindowSize;
		if (slot < MapperBankWindowCount) {
			uint32_t bank = _romBankRegisters[slot] % bankCount;
			uint32_t mappedAddress = bank * MapperWindowSize + (windowOffset % MapperWindowSize);
			return mappedAddress % _prgRomSize;
		}
	}

	return effectiveAddr % _prgRomSize;
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
	return addr >= 0xA12016 && addr <= 0xA1201F;
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
	uint8_t effectiveValue = value;
	bool nextRunning = (effectiveValue & 0x01) != 0;
	bool nextBusRequest = (effectiveValue & 0x02) != 0;
	if (nextRunning != _segaCdSubCpuRunning || nextBusRequest != _segaCdSubCpuBusRequest) {
		_segaCdSubCpuTransitionCount++;
	}
	_segaCdSubCpuRunning = nextRunning;
	_segaCdSubCpuBusRequest = nextBusRequest;
}

uint8_t GenesisMemoryManager::GetSegaCdSubCpuStatusByte() const {
	uint8_t statusByte = 0;
	if (_segaCdSubCpuRunning) {
		statusByte |= 0x01;
	}
	if (_segaCdSubCpuBusRequest) {
		statusByte |= 0x02;
	}
	statusByte |= (uint8_t)((_segaCdSubCpuTransitionCount & 0x0F) << 4);
	return statusByte;
}

void GenesisMemoryManager::UpdateSegaCdAudioPath(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	if (addr == 0xA12002) {
		_segaCdPcmLeft = effectiveValue;
	} else if (addr == 0xA12003) {
		_segaCdPcmRight = effectiveValue;
	} else if (addr == 0xA12004) {
		_segaCdCddaLeft = effectiveValue;
	} else if (addr == 0xA12005) {
		_segaCdCddaRight = effectiveValue;
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
		uint8_t statusByte = _segaCdMixedLeft;
		return statusByte;
	}
	if (addr == 0xA12011) {
		uint8_t statusByte = _segaCdMixedRight;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
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

	uint8_t effectiveValue = value;
	if (*target != effectiveValue) {
		*target = effectiveValue;
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
	if (addr == 0xA12016) {
		return (uint8_t)(_segaCdToolingEventCount & 0xFF);
	}
	if (addr == 0xA12017) {
		return (uint8_t)((_segaCdToolingEventCount >> 8) & 0xFF);
	}
	if (addr == 0xA12018) {
		return (uint8_t)(_ioState.DebugTranscriptLaneCount & 0xFF);
	}
	if (addr == 0xA12019) {
		return (uint8_t)(_ioState.DebugTranscriptLaneDigest & 0xFF);
	}
	if (addr == 0xA1201A) {
		uint8_t statusByte = 0x0F;
		return statusByte;
	}
	if (addr == 0xA1201B) {
		uint8_t statusByte = _segaCdToolingDigest;
		return statusByte;
	}
	if (addr == 0xA1201C) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortCapabilities(0) : 0;
		return statusByte;
	}
	if (addr == 0xA1201D) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortDigest(0) : 0;
		return statusByte;
	}
	if (addr == 0xA1201E) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortCapabilities(1) : 0;
		return statusByte;
	}
	if (addr == 0xA1201F) {
		uint8_t statusByte = _controlManager ? _controlManager->GetDeterministicPortDigest(1) : 0;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xSh2Staging(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	bool changed = false;
	if (addr == 0xA15012) {
		bool nextMaster = (effectiveValue & 0x01) != 0;
		bool nextSlave = (effectiveValue & 0x02) != 0;
		changed = nextMaster != _m32xMasterSh2Running || nextSlave != _m32xSlaveSh2Running;
		_m32xMasterSh2Running = nextMaster;
		_m32xSlaveSh2Running = nextSlave;
	} else if (addr == 0xA15013) {
		uint8_t phase = (uint8_t)(effectiveValue & 0x0F);
		changed = phase != _m32xSh2SyncPhase;
		_m32xSh2SyncPhase = phase;
	} else if (addr == 0xA15014) {
		changed = effectiveValue != _m32xSh2Milestone;
		_m32xSh2Milestone = effectiveValue;
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
		uint8_t statusByte = _m32xSh2Digest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::Update32xCompositionStaging(uint32_t addr, uint8_t value) {
	uint8_t effectiveValue = value;
	bool changed = false;
	if (addr == 0xA15016) {
		uint8_t blend = (uint8_t)(effectiveValue & 0x0F);
		changed = blend != _m32xCompositionBlend;
		_m32xCompositionBlend = blend;
	} else if (addr == 0xA15017) {
		uint8_t nextMarker = effectiveValue;
		changed = nextMarker != _m32xFrameSyncMarker;
		_m32xFrameSyncMarker = nextMarker;
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
		uint8_t statusByte = _m32xCompositionDigest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
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

	uint8_t effectiveValue = value;
	if (*target != effectiveValue) {
		*target = effectiveValue;
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
		uint8_t statusByte = (uint8_t)(_m32xToolingEventCount & 0xFF);
		return statusByte;
	}
	if (addr == 0xA15019) {
		uint8_t statusByte = (uint8_t)((_m32xToolingEventCount >> 8) & 0xFF);
		return statusByte;
	}
	if (addr == 0xA1501E) {
		uint8_t statusByte = 0x0F;
		return statusByte;
	}
	if (addr == 0xA1501F) {
		uint8_t statusByte = _m32xToolingDigest;
		return statusByte;
	}
	uint8_t statusByte = 0;
	return statusByte;
}

void GenesisMemoryManager::TrackSegaCdTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = 0;
	if ((addr & 0x10) != 0) {
		effectiveRoleFlags |= 0x02;
	}
	if (addr >= 0xA13000 && addr <= 0xA1301F) {
		effectiveRoleFlags |= 0x04;
	} else if (addr >= 0xA14000 && addr <= 0xA1401F) {
		effectiveRoleFlags |= 0x08;
	} else if (addr >= 0xA15000 && addr <= 0xA1501F) {
		effectiveRoleFlags |= 0x10;
	} else if (addr >= 0xA16000 && addr <= 0xA1601F) {
		effectiveRoleFlags |= 0x20;
	} else if (addr >= 0xA18000 && addr <= 0xA1801F) {
		effectiveRoleFlags |= 0x40;
	}

	TrackTranscriptEntry(addr, isWrite, effectiveValue, effectiveRoleFlags);
}

void GenesisMemoryManager::TrackSegaCdHandshakeTranscript(uint32_t addr, bool isWrite, uint8_t value) {
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = 0x80;
	if (IsZ80ResetAddress(addr)) {
		effectiveRoleFlags |= 0x04;
	}
	if (!isWrite) {
		effectiveRoleFlags |= 0x02;
	}

	TrackTranscriptEntry(addr, isWrite, effectiveValue, effectiveRoleFlags);
}

void GenesisMemoryManager::TrackTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = roleFlags;

	if (isWrite) {
		effectiveRoleFlags |= 0x01;
	}

	uint64_t hash = _ioState.TranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.TranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= effectiveValue;
	hash *= FnvPrime;
	hash ^= effectiveRoleFlags;
	hash *= FnvPrime;
	_ioState.TranscriptLaneDigest = hash;

	uint32_t index = _ioState.TranscriptLaneCount % 4;
	_ioState.TranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.TranscriptEntryValue[index] = effectiveValue;
	_ioState.TranscriptEntryFlags[index] = effectiveRoleFlags;
	_ioState.TranscriptLaneCount++;
}

void GenesisMemoryManager::TrackDebugTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags) {
	static constexpr uint64_t FnvOffsetBasis = 1469598103934665603ull;
	static constexpr uint64_t FnvPrime = 1099511628211ull;
	uint8_t effectiveValue = value;
	uint8_t effectiveRoleFlags = roleFlags;

	if (isWrite) {
		effectiveRoleFlags |= 0x01;
	}
	effectiveRoleFlags |= 0x40;

	uint64_t hash = _ioState.DebugTranscriptLaneDigest == 0 ? FnvOffsetBasis : _ioState.DebugTranscriptLaneDigest;
	hash ^= (addr & 0xFFFFFF);
	hash *= FnvPrime;
	hash ^= effectiveValue;
	hash *= FnvPrime;
	hash ^= effectiveRoleFlags;
	hash *= FnvPrime;
	_ioState.DebugTranscriptLaneDigest = hash;

	uint32_t index = _ioState.DebugTranscriptLaneCount % 4;
	_ioState.DebugTranscriptEntryAddress[index] = addr & 0xFFFFFF;
	_ioState.DebugTranscriptEntryValue[index] = effectiveValue;
	_ioState.DebugTranscriptEntryFlags[index] = effectiveRoleFlags;
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
	uint32_t effectiveAddr = addr;
	if (!IsSramAddress(effectiveAddr)) {
		return false;
	}

	if ((effectiveAddr & 0x01) == 0) {
		if (!_sramEvenBytes) {
			return false;
		}
	} else if (!_sramOddBytes) {
		return false;
	}

	if (_sramEvenBytes && _sramOddBytes) {
		offset = effectiveAddr - _sramStart;
	} else {
		offset = (effectiveAddr - _sramStart) >> 1;
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
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = _segaCdBridgeA140[addr & 0x03];
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		uint8_t effectiveValue = _saveRam[sramOffset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Read);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr < 0x400000) [[likely]] {
		// Cartridge ROM
		uint32_t mappedAddr = TranslateRomAddress(addr);
		uint8_t effectiveValue = _prgRom[mappedAddr];
		_ioState.RomReadHeartbeat++;
		_emu->ProcessMemoryRead<CpuType::Genesis>(mappedAddr, effectiveValue, MemoryOperationType::Read);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xFF0000) [[likely]] {
		// Work RAM
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveValue = _workRam[offset];
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Read);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		if (_tmssEnabled && !_tmssUnlocked) {
			if (!_tmssVdpBlockLogged) {
				_tmssVdpBlockLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP read8 access at ${:06x}", addr));
			}
			uint8_t effectiveValue = _openBus;
			_openBus = effectiveValue;
			return effectiveValue;
		}
		// VDP ports (byte access from word port)
		uint16_t effectiveWord = ReadVdpPort(addr);
		uint8_t effectiveValue = (addr & 1) ? (uint8_t)(effectiveWord & 0xFF) : (uint8_t)(effectiveWord >> 8);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (IsYm2612Address(addr)) {
			// YM2612 status/data ports. Report not-busy so startup polls can progress.
			uint8_t effectiveValue = 0x00;
			_openBus = effectiveValue;
			return effectiveValue;
		}

		// Z80 address space
		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			uint8_t effectiveValue = _z80Ram[z80Addr];
			_openBus = effectiveValue;
			return effectiveValue;
		}
		uint8_t effectiveValue = _openBus;
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		return ReadIo(addr);
	}

	uint8_t bankSlot = 0;
	if (TryGetRomBankRegisterSlot(addr, bankSlot)) [[unlikely]] {
		uint8_t effectiveValue = _romBankRegisters[bankSlot];
		_openBus = effectiveValue;
		return effectiveValue;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t effectiveValue = bridgeSlot[bridgeIndex];
		if (IsSegaCdSubCpuControlAddress(addr)) {
			effectiveValue = GetSegaCdSubCpuStatusByte();
		} else if (IsSegaCdAudioStatusAddress(addr)) {
			effectiveValue = GetSegaCdAudioStatusByte(addr);
		} else if (IsSegaCdToolingStatusAddress(addr)) {
			effectiveValue = GetSegaCdToolingStatusByte(addr);
		} else if (Is32xSh2StatusAddress(addr)) {
			effectiveValue = Get32xSh2StatusByte(addr);
		} else if (Is32xCompositionStatusAddress(addr)) {
			effectiveValue = Get32xCompositionStatusByte(addr);
		} else if (Is32xToolingStatusAddress(addr)) {
			effectiveValue = Get32xToolingStatusByte(addr);
		}
		TrackSegaCdTranscript(addr, false, effectiveValue);
		_openBus = effectiveValue;
		return effectiveValue;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		if (addr & 0x01) {
			uint8_t effectiveValue = _openBus;
			_openBus = effectiveValue;
			TrackSegaCdHandshakeTranscript(addr, false, effectiveValue);
			return effectiveValue;
		}

		// Z80 bus request: bit 0 indicates bus grant, upper bits preserve open bus.
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusRequest, _z80Reset);
		uint8_t effectiveValue = (uint8_t)((_openBus & 0xFE) | ackStatus);
		_openBus = effectiveValue;
		TrackSegaCdHandshakeTranscript(addr, false, effectiveValue);
		return effectiveValue;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint8_t resetStatus = _z80Reset ? 0x00 : 0x01;
		uint8_t effectiveValue = (uint8_t)((_openBus & 0xFE) | resetStatus);
		_openBus = effectiveValue;
		TrackSegaCdHandshakeTranscript(addr, false, effectiveValue);
		return effectiveValue;
	}

	uint8_t effectiveValue = _openBus;
	_openBus = effectiveValue;
	return effectiveValue;
}

uint16_t GenesisMemoryManager::Read16(uint32_t addr) {
	addr &= 0xFFFFFE;
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = ((uint16_t)_segaCdBridgeA140[addr & 0x03] << 8)
			| (uint16_t)_segaCdBridgeA140[(addr + 1) & 0x03];
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		uint8_t hi = Read8(addr);
		uint8_t lo = Read8(addr + 1);
		return ((uint16_t)hi << 8) | lo;
	}

	if (addr < 0x400000) [[likely]] {
		uint32_t mappedAddrHi = TranslateRomAddress(addr);
		uint32_t mappedAddrLo = TranslateRomAddress(addr + 1);
		uint8_t effectiveHighByte = _prgRom[mappedAddrHi];
		uint8_t effectiveLowByte = _prgRom[mappedAddrLo];
		uint16_t effectiveValue = ((uint16_t)effectiveHighByte << 8) | effectiveLowByte;
		_ioState.RomReadHeartbeat += 2;
		_emu->ProcessMemoryRead<CpuType::Genesis>(mappedAddrHi, effectiveHighByte, MemoryOperationType::Read);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveHighByte = _workRam[offset];
		uint8_t effectiveLowByte = _workRam[(offset + 1) & 0xFFFF];
		uint16_t effectiveValue = ((uint16_t)effectiveHighByte << 8) | effectiveLowByte;
		_emu->ProcessMemoryRead<CpuType::Genesis>(addr, effectiveHighByte, MemoryOperationType::Read);
		_openBus = effectiveLowByte;
		return effectiveValue;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		if (_tmssEnabled && !_tmssUnlocked) {
			if (!_tmssVdpBlockLogged) {
				_tmssVdpBlockLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP read16 access at ${:06x}", addr));
			}
			uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
			_openBus = (uint8_t)(effectiveValue & 0xFF);
			return effectiveValue;
		}
		uint16_t effectiveValue = ReadVdpPort(addr);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		if (IsYm2612Address(addr)) {
			uint8_t effectiveHighByte = 0x00;
			uint8_t effectiveLowByte = 0x00;
			uint16_t effectiveValue = (uint16_t)(((uint16_t)effectiveHighByte << 8) | effectiveLowByte);
			_openBus = effectiveLowByte;
			return effectiveValue;
		}

		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			uint8_t effectiveHighByte = _z80Ram[z80Addr];
			uint8_t effectiveLowByte = _z80Ram[(z80Addr + 1) & 0x1FFF];
			uint16_t effectiveValue = (uint16_t)(((uint16_t)effectiveHighByte << 8) | effectiveLowByte);
			_openBus = (uint8_t)(effectiveValue & 0xFF);
			return effectiveValue;
		}
		uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		// 68k word access targets even addresses; Genesis I/O registers are byte-mapped on odd addresses.
		// Keep the high byte neutral and read the register from addr+1.
		uint8_t effectiveHi = 0x00;
		uint8_t effectiveLo = ReadIo(addr + 1);
		uint16_t effectiveValue = ((uint16_t)effectiveHi << 8) | effectiveLo;
		_openBus = effectiveLo;
		return effectiveValue;
	}

	if (addr >= 0xA130F0 && addr <= 0xA130FE) [[unlikely]] {
		uint8_t effectiveHi = Read8(addr);
		uint8_t effectiveLo = Read8(addr + 1);
		return ((uint16_t)effectiveHi << 8) | effectiveLo;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t effectiveHi = Read8(addr);
		uint8_t effectiveLo = Read8(addr + 1);
		return ((uint16_t)effectiveHi << 8) | effectiveLo;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusRequest, _z80Reset);
		uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
		if (ackStatus) {
			effectiveValue |= 0x0100;
		} else {
			effectiveValue &= (uint16_t)~0x0100;
		}
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		TrackSegaCdHandshakeTranscript(addr, false, effectiveHighByte);
		return effectiveValue;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
		if (_z80Reset) {
			effectiveValue &= (uint16_t)~0x0100;
		} else {
			effectiveValue |= 0x0100;
		}
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		TrackSegaCdHandshakeTranscript(addr, false, effectiveHighByte);
		return effectiveValue;
	}

	uint16_t effectiveValue = (uint16_t)((_openBus << 8) | _openBus);
	_openBus = (uint8_t)(effectiveValue & 0xFF);
	return effectiveValue;
}

void GenesisMemoryManager::Write8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	uint32_t sramOffset = 0;
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		uint32_t slot = addr & 0x03;
		bool wasUnlocked = _tmssUnlocked;
		_segaCdBridgeA140[slot] = effectiveValue;
		_openBus = effectiveValue;
		_tmssUnlocked = _segaCdBridgeA140[0] == 'S'
			&& _segaCdBridgeA140[1] == 'E'
			&& _segaCdBridgeA140[2] == 'G'
			&& _segaCdBridgeA140[3] == 'A';
		if (_tmssEnabled) {
			MessageManager::Log(std::format("[Genesis][MMU] TMSS write ${:06x}=${:02x} state='{}{}{}{}' unlocked={}",
				addr,
				effectiveValue,
				(char)_segaCdBridgeA140[0],
				(char)_segaCdBridgeA140[1],
				(char)_segaCdBridgeA140[2],
				(char)_segaCdBridgeA140[3],
				_tmssUnlocked ? "true" : "false"));
		}
		if (!wasUnlocked && _tmssUnlocked) {
			_tmssVdpBlockLogged = false;
			MessageManager::Log("[Genesis][MMU] TMSS unlocked - VDP access enabled");
		}
		_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
		_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
		return;
	}

	if (TryGetSramOffset(addr, sramOffset)) [[unlikely]] {
		uint8_t effectiveValue = value;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Write);
		_saveRam[sramOffset] = effectiveValue;
		_openBus = effectiveValue;
		return;
	}

	if (addr < _prgRomSize) [[likely]] {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		TrackSegaCdTranscript(addr, true, effectiveValue);
		return;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveValue = value;
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveValue, MemoryOperationType::Write);
		_workRam[offset] = effectiveValue;
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		uint8_t effectiveValue = value;
		static uint64_t vdpWrite8GateCount = 0;
		if (vdpWrite8GateCount < 64) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] VDP write8 gate #{} addr=${:06x} val=${:02x} pc=${:06x}",
				vdpWrite8GateCount + 1,
				addr,
				effectiveValue,
				pc));
		}
		vdpWrite8GateCount++;
		if (_tmssEnabled && !_tmssUnlocked) {
			if (!_tmssVdpBlockLogged) {
				_tmssVdpBlockLogged = true;
				MessageManager::Log(std::format("[Genesis][MMU] TMSS is locking VDP write8 access at ${:06x}", addr));
			}
			_openBus = effectiveValue;
			return;
		}
		// VDP byte write - promote to word write
		WriteVdpPort(addr, (uint16_t)effectiveValue | ((uint16_t)effectiveValue << 8));
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (IsYm2612Address(addr)) {
			_openBus = effectiveValue;
			return;
		}

		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = effectiveValue;
		}
		_openBus = effectiveValue;
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		uint8_t effectiveValue = value;
		WriteIo(addr, effectiveValue);
		return;
	}

	if (TryWriteRomBankRegister(addr, value)) [[unlikely]] {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint8_t effectiveValue = value;
		bridgeSlot[bridgeIndex] = effectiveValue;
		_openBus = effectiveValue;
		if (IsSegaCdSubCpuControlAddress(addr)) {
			UpdateSegaCdSubCpuControl(effectiveValue);
		} else if (IsSegaCdAudioDataAddress(addr)) {
			UpdateSegaCdAudioPath(addr, effectiveValue);
		} else if (IsSegaCdToolingControlAddress(addr)) {
			UpdateSegaCdToolingContract(addr, effectiveValue);
		} else if (Is32xSh2ControlAddress(addr)) {
			Update32xSh2Staging(addr, effectiveValue);
		} else if (Is32xCompositionControlAddress(addr)) {
			Update32xCompositionStaging(addr, effectiveValue);
		} else if (Is32xToolingControlAddress(addr)) {
			Update32xToolingContract(addr, effectiveValue);
		}
		TrackSegaCdTranscript(addr, true, effectiveValue);
		return;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (!(addr & 0x01)) {
			_z80BusRequest = (effectiveValue & 0x01) != 0;
		}
		_openBus = effectiveValue;
		TrackSegaCdHandshakeTranscript(addr, true, effectiveValue);
		return;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint8_t effectiveValue = value;
		if (!(addr & 0x01)) {
			_z80Reset = !(effectiveValue & 0x01);
		}
		_openBus = effectiveValue;
		TrackSegaCdHandshakeTranscript(addr, true, effectiveValue);
		return;
	}

	// Unmapped/ROM area — ignore writes (no mapper for now)
	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
}

void GenesisMemoryManager::Write16(uint32_t addr, uint16_t value) {
	addr &= 0xFFFFFE;
	if (IsTmssAddress(addr)) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}
	if (HasSaveRam() && addr >= _sramStart && addr <= _sramEnd) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (addr >= 0xA130F0 && addr <= 0xA130FE) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (addr < _prgRomSize) [[likely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xFF0000) [[likely]] {
		uint32_t offset = addr & 0xFFFF;
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		_emu->ProcessMemoryWrite<CpuType::Genesis>(addr, effectiveHighByte, MemoryOperationType::Write);
		_workRam[offset] = effectiveHighByte;
		_workRam[(offset + 1) & 0xFFFF] = effectiveLowByte;
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xC00000 && addr <= 0xC0001F) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		static uint64_t vdpWrite16GateCount = 0;
		if (vdpWrite16GateCount < 128) {
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] VDP write16 gate #{} addr=${:06x} val=${:04x} pc=${:06x}",
				vdpWrite16GateCount + 1,
				addr,
				effectiveValue,
				pc));
		}
		vdpWrite16GateCount++;
		if (_tmssEnabled && !_tmssUnlocked) {
			_openBus = effectiveLowByte;
			return;
		}
		WriteVdpPort(addr, effectiveValue);
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xA00000 && addr <= 0xA0FFFF) [[unlikely]] {
		uint8_t effectiveHighByte = (uint8_t)(value >> 8);
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		if (IsYm2612Address(addr)) {
			_openBus = effectiveLowByte;
			return;
		}

		if (_z80BusRequest || _z80Reset) {
			uint32_t z80Addr = addr & 0x1FFF;
			_z80Ram[z80Addr] = effectiveHighByte;
			_z80Ram[(z80Addr + 1) & 0x1FFF] = effectiveLowByte;
		}
		_openBus = effectiveLowByte;
		return;
	}

	if (addr >= 0xA10000 && addr <= 0xA1001F) [[unlikely]] {
		uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
		// 68k word writes hit even addresses; only the low byte maps to the odd-byte I/O register.
		WriteIo(addr + 1, effectiveLowByte);
		_openBus = effectiveLowByte;
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(addr, bridgeSlot, bridgeIndex)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		uint8_t effectiveLowByte = (uint8_t)(effectiveValue & 0xFF);
		Write8(addr, effectiveHighByte);
		Write8(addr + 1, effectiveLowByte);
		return;
	}

	if (IsZ80BusReqAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		_z80BusRequest = (effectiveHighByte & 0x01) != 0;
		_openBus = effectiveHighByte;
		TrackSegaCdHandshakeTranscript(addr, true, effectiveHighByte);
		return;
	}

	if (IsZ80ResetAddress(addr)) [[unlikely]] {
		uint16_t effectiveValue = value;
		uint8_t effectiveHighByte = (uint8_t)(effectiveValue >> 8);
		_z80Reset = !(effectiveHighByte & 0x01);
		_openBus = effectiveHighByte;
		TrackSegaCdHandshakeTranscript(addr, true, effectiveHighByte);
		return;
	}

	uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
	_openBus = effectiveLowByte;
}

// VDP port access
uint16_t GenesisMemoryManager::ReadVdpPort(uint32_t addr) {
	uint32_t port = addr & 0x1F;
	if (port < 0x04) {
		uint16_t effectiveValue = _vdp->ReadDataPort();
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	} else if (port < 0x08) {
		uint16_t effectiveValue = _vdp->ReadControlPort();
		static uint64_t controlPortReadCount = 0;
		controlPortReadCount++;
		if (controlPortReadCount <= 128 || (controlPortReadCount % 4096) == 0) {
			GenesisVdpState vdpState = _vdp->GetState();
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			uint8_t statusLo = (uint8_t)(effectiveValue & 0xff);
			bool vblankBit = (statusLo & (uint8_t)VdpStatus::VBlankFlag) != 0;
			bool displayEnabled = (vdpState.Registers[1] & 0x40) != 0;
			MessageManager::Log(std::format("[Genesis][MMU] CtrlPortRead #{} addr=${:06x} pc=${:06x} status=${:04x} lo=${:02x} vb={} display={} vc={} hc={} r1=${:02x}",
				controlPortReadCount,
				addr & 0x00ffffff,
				pc & 0x00ffffff,
				effectiveValue,
				statusLo,
				vblankBit ? 1 : 0,
				displayEnabled ? 1 : 0,
				vdpState.VCounter,
				vdpState.HCounter,
				vdpState.Registers[1]));
		}
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	} else if (port < 0x10) {
		uint16_t effectiveValue = _vdp->ReadHVCounter();
		_openBus = (uint8_t)(effectiveValue & 0xFF);
		return effectiveValue;
	}
	uint16_t effectiveValue = _openBus;
	_openBus = (uint8_t)(effectiveValue & 0xFF);
	return effectiveValue;
}

void GenesisMemoryManager::WriteVdpPort(uint32_t addr, uint16_t value) {
	uint32_t port = addr & 0x1F;
	uint16_t effectiveValue = value;
	uint8_t effectiveHighByte = (uint8_t)(value >> 8);
	uint8_t effectiveLowByte = (uint8_t)(value & 0xFF);
	static uint64_t vdpPortWriteCount = 0;
	static bool loggedFirstNonZeroDataWrite = false;
	static bool loggedFirstNonZeroControlWrite = false;
	vdpPortWriteCount++;

	if (port < 0x04) {
		if (!loggedFirstNonZeroDataWrite && effectiveValue != 0) {
			loggedFirstNonZeroDataWrite = true;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] First non-zero VDP data write #{} addr=${:06x} port=${:02x} word=${:04x} hi=${:02x} lo=${:02x} pc=${:06x}",
				vdpPortWriteCount,
				addr & 0x00ffffff,
				port,
				effectiveValue,
				effectiveHighByte,
				effectiveLowByte,
				pc));
		}
		_vdp->WriteDataPort(effectiveValue);
	} else if (port < 0x08) {
		if (!loggedFirstNonZeroControlWrite && effectiveValue != 0) {
			loggedFirstNonZeroControlWrite = true;
			uint32_t pc = _cpu ? (_cpu->GetState().PC & 0x00ffffff) : 0xffffffff;
			MessageManager::Log(std::format("[Genesis][MMU] First non-zero VDP control write #{} addr=${:06x} port=${:02x} word=${:04x} hi=${:02x} lo=${:02x} pc=${:06x}",
				vdpPortWriteCount,
				addr & 0x00ffffff,
				port,
				effectiveValue,
				effectiveHighByte,
				effectiveLowByte,
				pc));
		}
		_vdp->WriteControlPort(effectiveValue);
	} else if (port >= 0x11 && port < 0x14) {
		// PSG write — SN76489 accepts byte writes via top byte of word
		if (_psg) {
			_psg->Write(effectiveHighByte);
		}
	}
	_openBus = effectiveLowByte;
}

// I/O registers ($A10001-$A1001F)
uint8_t GenesisMemoryManager::ReadIo(uint32_t addr) {
	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x01:
			{
				uint8_t effectiveValue = BuildVersionRegister(_console ? _console->GetRegion() : ConsoleRegion::Ntsc);
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x03:
			_ioState.DataPort[0] = _controlManager ? _controlManager->ReadDataPort(0) : 0x7F; // Controller 1 data
			_ioState.ThState[0] = _controlManager ? _controlManager->GetThState(0) : 0;
			_ioState.ThCount[0] = _controlManager ? _controlManager->GetThCount(0) : 0;
			{
				uint8_t effectiveValue = _ioState.DataPort[0];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x05:
			_ioState.DataPort[1] = _controlManager ? _controlManager->ReadDataPort(1) : 0x7F; // Controller 2 data
			_ioState.ThState[1] = _controlManager ? _controlManager->GetThState(1) : 0;
			_ioState.ThCount[1] = _controlManager ? _controlManager->GetThCount(1) : 0;
			{
				uint8_t effectiveValue = _ioState.DataPort[1];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x07:
			_ioState.DataPort[2] = 0xFF; // EXT port
			{
				uint8_t effectiveValue = _ioState.DataPort[2];
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x09:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[0]; // Controller 1 ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x0B:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[1]; // Controller 2 ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		case 0x0D:
			{
				uint8_t effectiveValue = _ioState.CtrlPort[2]; // EXT ctrl
				_openBus = effectiveValue;
				return effectiveValue;
			}
		default:
			{
				uint8_t effectiveValue = _openBus;
				_openBus = effectiveValue;
				return effectiveValue;
			}
	}
}

void GenesisMemoryManager::WriteIo(uint32_t addr, uint8_t value) {
	uint32_t reg = addr & 0x1F;
	switch (reg) {
		case 0x03:
			{
				uint8_t effectiveValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(0, effectiveValue);
					_ioState.DataPort[0] = _controlManager->GetDataPortWriteLatch(0);
					_ioState.ThState[0] = _controlManager->GetThState(0);
					_ioState.ThCount[0] = _controlManager->GetThCount(0);
				} else {
					_ioState.DataPort[0] = effectiveValue;
					_ioState.ThState[0] = 0;
					_ioState.ThCount[0] = 0;
				}
				break;
			}
		case 0x05:
			{
				uint8_t effectiveValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(1, effectiveValue);
					_ioState.DataPort[1] = _controlManager->GetDataPortWriteLatch(1);
					_ioState.ThState[1] = _controlManager->GetThState(1);
					_ioState.ThCount[1] = _controlManager->GetThCount(1);
				} else {
					_ioState.DataPort[1] = effectiveValue;
					_ioState.ThState[1] = 0;
					_ioState.ThCount[1] = 0;
				}
				break;
			}
		case 0x09:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[0] = effectiveValue;
				break;
			}
		case 0x0B:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[1] = effectiveValue;
				break;
			}
		case 0x0D:
			{
				uint8_t effectiveValue = value;
				_ioState.CtrlPort[2] = effectiveValue;
				break;
			}
	}
	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
}

uint8_t GenesisMemoryManager::DebugRead8(uint32_t addr) {
	addr &= 0xFFFFFF;
	uint32_t effectiveAddr = addr;
	if (IsTmssAddress(effectiveAddr)) {
		uint8_t effectiveValue = _segaCdBridgeA140[effectiveAddr & 0x03];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	if (effectiveAddr < 0x400000) {
		uint32_t mappedAddr = TranslateRomAddress(effectiveAddr);
		uint8_t effectiveValue = _prgRom[mappedAddr];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x01);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xFF0000) {
		uint8_t effectiveValue = _workRam[effectiveAddr & 0xFFFF];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x04);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xA10000 && effectiveAddr <= 0xA1001F) {
		uint32_t reg = effectiveAddr & 0x1F;
		uint8_t effectiveValue = _openBus;
		switch (reg) {
			case 0x01:
				{
					uint8_t statusByte = BuildVersionRegister(_console ? _console->GetRegion() : ConsoleRegion::Ntsc);
					effectiveValue = statusByte;
				}
				break;
			case 0x03:
				_ioState.DataPort[0] = _controlManager ? _controlManager->ReadDataPort(0) : 0x7F;
				_ioState.ThState[0] = _controlManager ? _controlManager->GetThState(0) : 0;
				_ioState.ThCount[0] = _controlManager ? _controlManager->GetThCount(0) : 0;
				{
					uint8_t statusByte = _ioState.DataPort[0];
					effectiveValue = statusByte;
				}
				break;
			case 0x05:
				_ioState.DataPort[1] = _controlManager ? _controlManager->ReadDataPort(1) : 0x7F;
				_ioState.ThState[1] = _controlManager ? _controlManager->GetThState(1) : 0;
				_ioState.ThCount[1] = _controlManager ? _controlManager->GetThCount(1) : 0;
				{
					uint8_t statusByte = _ioState.DataPort[1];
					effectiveValue = statusByte;
				}
				break;
			case 0x07:
				{
					uint8_t statusByte = 0xFF;
					effectiveValue = statusByte;
				}
				break;
			case 0x09:
				{
					uint8_t statusByte = _ioState.CtrlPort[0];
					effectiveValue = statusByte;
				}
				break;
			case 0x0B:
				{
					uint8_t statusByte = _ioState.CtrlPort[1];
					effectiveValue = statusByte;
				}
				break;
			case 0x0D:
				{
					uint8_t statusByte = _ioState.CtrlPort[2];
					effectiveValue = statusByte;
				}
				break;
			default:
				{
					uint8_t statusByte = _openBus;
					effectiveValue = statusByte;
				}
				break;
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x10);
		return effectiveValue;
	}
	uint8_t bankSlot = 0;
	if (TryGetRomBankRegisterSlot(effectiveAddr, bankSlot)) {
		uint8_t effectiveValue = _romBankRegisters[bankSlot];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x11);
		return effectiveValue;
	}
	if (IsZ80BusReqAddress(effectiveAddr)) {
		if (effectiveAddr & 0x01) {
			uint8_t effectiveValue = _openBus;
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x82);
			return effectiveValue;
		}
		uint8_t ackStatus = GetZ80BusAckStatusBit(_z80BusRequest, _z80Reset);
		uint8_t effectiveValue = (uint8_t)((_openBus & 0xFE) | ackStatus);
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x82);
		return effectiveValue;
	}
	if (IsZ80ResetAddress(effectiveAddr)) {
		uint8_t effectiveValue = _openBus;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x86);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xA00000 && effectiveAddr <= 0xA0FFFF) {
		uint8_t effectiveValue = _openBus;
		if (_z80BusRequest || _z80Reset) {
			effectiveValue = _z80Ram[effectiveAddr & 0x1FFF];
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x20);
		return effectiveValue;
	}
	if (effectiveAddr >= 0xC00000 && effectiveAddr <= 0xC0001F) {
		if (_tmssEnabled && !_tmssUnlocked) {
			uint8_t effectiveValue = _openBus;
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x30);
			return effectiveValue;
		}
		uint8_t effectiveValue = _openBus;
		if (_vdp) {
			uint16_t effectiveWord = ReadVdpPort(effectiveAddr);
			if (effectiveAddr & 1) {
				effectiveValue = (uint8_t)(effectiveWord & 0xFF);
			} else {
				effectiveValue = (uint8_t)(effectiveWord >> 8);
			}
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x30);
		return effectiveValue;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(effectiveAddr, bridgeSlot, bridgeIndex)) {
		if (IsSegaCdSubCpuControlAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdSubCpuStatusByte();
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (IsSegaCdAudioStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdAudioStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (IsSegaCdToolingStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = GetSegaCdToolingStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xSh2StatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xSh2StatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xCompositionStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xCompositionStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		if (Is32xToolingStatusAddress(effectiveAddr)) {
			uint8_t effectiveValue = Get32xToolingStatusByte(effectiveAddr);
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
			return effectiveValue;
		}
		uint8_t effectiveValue = bridgeSlot[bridgeIndex];
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x02);
		return effectiveValue;
	}
	uint8_t effectiveValue = _openBus;
	_openBus = effectiveValue;
	TrackDebugTranscriptEntry(effectiveAddr, false, effectiveValue, 0x00);
	return effectiveValue;
}

void GenesisMemoryManager::DebugWrite8(uint32_t addr, uint8_t value) {
	addr &= 0xFFFFFF;
	uint32_t effectiveAddr = addr;
	if (IsTmssAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		uint32_t slot = effectiveAddr & 0x03;
		_segaCdBridgeA140[slot] = effectiveValue;
		_openBus = effectiveValue;
		_tmssUnlocked = _segaCdBridgeA140[0] == 'S'
			&& _segaCdBridgeA140[1] == 'E'
			&& _segaCdBridgeA140[2] == 'G'
			&& _segaCdBridgeA140[3] == 'A';
		_ioState.TmssEnabled = _tmssEnabled ? 1 : 0;
		_ioState.TmssUnlocked = _tmssUnlocked ? 1 : 0;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x02);
		return;
	}
	if (effectiveAddr < _prgRomSize) {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x01);
		return;
	}
	if (effectiveAddr >= 0xFF0000) {
		uint8_t effectiveValue = value;
		_workRam[effectiveAddr & 0xFFFF] = effectiveValue;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x04);
		return;
	}
	if (effectiveAddr >= 0xA10000 && effectiveAddr <= 0xA1001F) {
		uint32_t reg = effectiveAddr & 0x1F;
		switch (reg) {
			case 0x03:
				{
					uint8_t ioWriteValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(0, ioWriteValue);
					_ioState.DataPort[0] = _controlManager->GetDataPortWriteLatch(0);
					_ioState.ThState[0] = _controlManager->GetThState(0);
					_ioState.ThCount[0] = _controlManager->GetThCount(0);
				} else {
					_ioState.DataPort[0] = ioWriteValue;
					_ioState.ThState[0] = 0;
					_ioState.ThCount[0] = 0;
				}
				{
					uint8_t effectiveValue = ioWriteValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
				}
			case 0x05:
				{
					uint8_t ioWriteValue = value;
				if (_controlManager) {
					_controlManager->WriteDataPort(1, ioWriteValue);
					_ioState.DataPort[1] = _controlManager->GetDataPortWriteLatch(1);
					_ioState.ThState[1] = _controlManager->GetThState(1);
					_ioState.ThCount[1] = _controlManager->GetThCount(1);
				} else {
					_ioState.DataPort[1] = ioWriteValue;
					_ioState.ThState[1] = 0;
					_ioState.ThCount[1] = 0;
				}
				{
					uint8_t effectiveValue = ioWriteValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
				}
			case 0x09:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[0] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			case 0x0B:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[1] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			case 0x0D:
				{
					uint8_t effectiveValue = value;
					uint8_t controlValue = effectiveValue;
					_ioState.CtrlPort[2] = controlValue;
					_openBus = effectiveValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x10);
				}
				return;
			default:
				{
					uint8_t ioWriteValue = value;
					_openBus = ioWriteValue;
					TrackDebugTranscriptEntry(effectiveAddr, true, ioWriteValue, 0x10);
				}
				return;
		}
	}
	if (TryWriteRomBankRegister(effectiveAddr, value)) {
		uint8_t effectiveValue = value;
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x11);
		return;
	}
	if (IsZ80BusReqAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		if (!(effectiveAddr & 0x01)) {
			_z80BusRequest = (effectiveValue & 0x01) != 0;
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x80);
		return;
	}
	if (IsZ80ResetAddress(effectiveAddr)) {
		uint8_t effectiveValue = value;
		if (!(effectiveAddr & 0x01)) {
			_z80Reset = !(effectiveValue & 0x01);
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x84);
		return;
	}
	if (effectiveAddr >= 0xA00000 && effectiveAddr <= 0xA0FFFF) {
		uint8_t effectiveValue = value;
		if (_z80BusRequest || _z80Reset) {
			_z80Ram[effectiveAddr & 0x1FFF] = effectiveValue;
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x20);
		return;
	}
	if (effectiveAddr >= 0xC00000 && effectiveAddr <= 0xC0001F) {
		uint8_t effectiveValue = value;
		if (_tmssEnabled && !_tmssUnlocked) {
			_openBus = effectiveValue;
			TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x30);
			return;
		}
		if (_vdp) {
			WriteVdpPort(effectiveAddr, (uint16_t)effectiveValue | ((uint16_t)effectiveValue << 8));
		}
		_openBus = effectiveValue;
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x30);
		return;
	}

	uint8_t* bridgeSlot = nullptr;
	uint32_t bridgeIndex = 0;
	if (TryGetSegaCdBridgeSlot(effectiveAddr, bridgeSlot, bridgeIndex)) {
		uint8_t effectiveValue = value;
		bridgeSlot[bridgeIndex] = effectiveValue;
		_openBus = effectiveValue;
		if (IsSegaCdSubCpuControlAddress(effectiveAddr)) {
			UpdateSegaCdSubCpuControl(effectiveValue);
		} else if (IsSegaCdAudioDataAddress(effectiveAddr)) {
			UpdateSegaCdAudioPath(effectiveAddr, effectiveValue);
		} else if (IsSegaCdToolingControlAddress(effectiveAddr)) {
			UpdateSegaCdToolingContract(effectiveAddr, effectiveValue);
		} else if (Is32xSh2ControlAddress(effectiveAddr)) {
			Update32xSh2Staging(effectiveAddr, effectiveValue);
		} else if (Is32xCompositionControlAddress(effectiveAddr)) {
			Update32xCompositionStaging(effectiveAddr, effectiveValue);
		} else if (Is32xToolingControlAddress(effectiveAddr)) {
			Update32xToolingContract(effectiveAddr, effectiveValue);
		}
		TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x02);
		return;
	}

	uint8_t effectiveValue = value;
	_openBus = effectiveValue;
	TrackDebugTranscriptEntry(effectiveAddr, true, effectiveValue, 0x00);
}

AddressInfo GenesisMemoryManager::GetAbsoluteAddress(uint32_t addr) {
	addr &= 0xFFFFFF;
	uint32_t effectiveAddress = addr;
	AddressInfo info = {};
	if (effectiveAddress < 0x400000) {
		info.Address = TranslateRomAddress(effectiveAddress);
		info.Type = MemoryType::GenesisPrgRom;
	} else if (effectiveAddress >= 0xFF0000) {
		info.Address = effectiveAddress & 0xFFFF;
		info.Type = MemoryType::GenesisWorkRam;
	} else {
		info.Address = -1;
		info.Type = MemoryType::None;
	}
	return info;
}

int32_t GenesisMemoryManager::GetRelativeAddress(AddressInfo& absAddress) {
	int32_t relativeAddress = -1;
	if (absAddress.Type == MemoryType::GenesisPrgRom) {
		relativeAddress = absAddress.Address;
	} else if (absAddress.Type == MemoryType::GenesisWorkRam) {
		relativeAddress = 0xFF0000 + absAddress.Address;
	}
	return relativeAddress;
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
	SV(_romBankMapperEnabled);
	SVArray(_romBankRegisters, (uint32_t)sizeof(_romBankRegisters));
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
	SV(_ioState.TmssEnabled);
	SV(_ioState.TmssUnlocked);
	SV(_ioState.CpuProgramCounterHeartbeat);
	SV(_ioState.CpuCycleHeartbeat);
	SV(_ioState.CpuInstructionHeartbeat);
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
	SV(_ioState.RomReadHeartbeat);
}

void GenesisMemoryManager::LoadBattery() {
	if (HasSaveRam()) {
		BatteryManager* batteryManager = _emu->GetBatteryManager();
		batteryManager->LoadBattery(".sav", std::span<uint8_t>(_saveRam, _saveRamSize));
	}
}

void GenesisMemoryManager::SaveBattery() {
	if (HasSaveRam()) {
		BatteryManager* batteryManager = _emu->GetBatteryManager();
		batteryManager->SaveBattery(".sav", std::span<const uint8_t>(_saveRam, _saveRamSize));
	}
}

void GenesisMemoryManager::ResetRuntimeState(bool hardReset) {
	bool nextZ80BusRequest = false;
	bool nextZ80Reset = true;
	uint8_t nextOpenBus = 0;
	bool nextTmssUnlocked = false;
	bool tmssEnabled = _tmssEnabled;
	_z80BusRequest = nextZ80BusRequest;
	_z80Reset = nextZ80Reset;
	_openBus = nextOpenBus;
	_tmssUnlocked = nextTmssUnlocked;
	_tmssVdpBlockLogged = false;

	if (tmssEnabled) {
		memset(_segaCdBridgeA140, 0, sizeof(_segaCdBridgeA140));
	}

	ResetRomBankMapper();

	memset(_ioState.DataPort, 0, sizeof(_ioState.DataPort));
	memset(_ioState.TxData, 0, sizeof(_ioState.TxData));
	memset(_ioState.RxData, 0, sizeof(_ioState.RxData));
	memset(_ioState.SCtrl, 0, sizeof(_ioState.SCtrl));
	memset(_ioState.ThCount, 0, sizeof(_ioState.ThCount));
	memset(_ioState.ThState, 0, sizeof(_ioState.ThState));
	uint32_t resetTranscriptLaneCount = 0;
	uint64_t resetTranscriptLaneDigest = 0;
	_ioState.TranscriptLaneCount = resetTranscriptLaneCount;
	_ioState.TranscriptLaneDigest = resetTranscriptLaneDigest;
	_ioState.RomReadHeartbeat = 0;
	for (uint32_t i = 0; i < 4; i++) {
		_ioState.TranscriptEntryAddress[i] = 0;
		_ioState.TranscriptEntryValue[i] = 0;
		_ioState.TranscriptEntryFlags[i] = 0;
	}

	uint8_t tmssEnabledValue = tmssEnabled ? 1 : 0;
	uint8_t tmssUnlockedValue = nextTmssUnlocked ? 1 : 0;
	_ioState.TmssEnabled = tmssEnabledValue;
	_ioState.TmssUnlocked = tmssUnlockedValue;

	bool doHardReset = hardReset;
	if (doHardReset) {
		ClearDebugTranscriptLane();
	}
}
