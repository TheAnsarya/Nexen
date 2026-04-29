#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Genesis/GenesisVdp.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"
#include "Debugger/AddressInfo.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

class Emulator;
class GenesisConsole;
class GenesisM68k;
class GenesisVdp;
class GenesisControlManager;
class GenesisPsg;

class GenesisMemoryManager final : public ISerializable {
private:
	static constexpr uint32_t WorkRamSize = 0x10000;   // 64KB work RAM
	static constexpr uint32_t Z80RamSize = 0x2000;     // 8KB Z80 RAM
	static constexpr uint32_t MapperWindowSize = 0x80000; // 512KB
	static constexpr uint32_t MapperBankWindowCount = 7;  // $080000-$3FFFFF

	Emulator* _emu = nullptr;
	GenesisConsole* _console = nullptr;
	GenesisM68k* _cpu = nullptr;
	GenesisVdp* _vdp = nullptr;
	GenesisControlManager* _controlManager = nullptr;
	GenesisPsg* _psg = nullptr;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	uint8_t* _workRam = nullptr;
	uint8_t* _z80Ram = nullptr;
	uint8_t* _saveRam = nullptr;
	uint32_t _saveRamSize = 0;
	uint32_t _sramStart = 0;
	uint32_t _sramEnd = 0;
	bool _hasSram = false;
	bool _sramEvenBytes = true;
	bool _sramOddBytes = true;

	GenesisIoState _ioState = {};

	uint64_t _masterClock = 0;
	uint8_t _openBus = 0;

	// Z80 bus
	bool _z80BusRequest = false;
	bool _z80Reset = true;
	bool _romBankMapperEnabled = false;
	uint8_t _romBankRegisters[MapperBankWindowCount] = {};

	// TMSS (Trademark Security System)
	bool _tmssEnabled = false;
	bool _tmssUnlocked = false;
	bool _tmssVdpBlockLogged = false;
	bool _segaCdSubCpuRunning = false;
	bool _segaCdSubCpuBusRequest = false;
	uint32_t _segaCdSubCpuTransitionCount = 0;
	uint8_t _segaCdPcmLeft = 0;
	uint8_t _segaCdPcmRight = 0;
	uint8_t _segaCdCddaLeft = 0;
	uint8_t _segaCdCddaRight = 0;
	uint8_t _segaCdMixedLeft = 0;
	uint8_t _segaCdMixedRight = 0;
	uint32_t _segaCdAudioCheckpointCount = 0;
	uint8_t _segaCdToolingDebuggerSignal = 0;
	uint8_t _segaCdToolingTasSignal = 0;
	uint8_t _segaCdToolingSaveStateSignal = 0;
	uint8_t _segaCdToolingCheatSignal = 0;
	uint32_t _segaCdToolingEventCount = 0;
	uint8_t _segaCdToolingDigest = 0;
	bool _m32xMasterSh2Running = false;
	bool _m32xSlaveSh2Running = false;
	uint8_t _m32xSh2SyncPhase = 0;
	uint8_t _m32xSh2Milestone = 0;
	uint32_t _m32xSh2SyncEpoch = 0;
	uint8_t _m32xSh2Digest = 0;
	uint8_t _m32xCompositionBlend = 0;
	uint8_t _m32xFrameSyncMarker = 0;
	uint32_t _m32xFrameSyncEpoch = 0;
	uint8_t _m32xCompositionDigest = 0;
	uint8_t _m32xToolingDebuggerSignal = 0;
	uint8_t _m32xToolingTasSignal = 0;
	uint8_t _m32xToolingSaveStateSignal = 0;
	uint8_t _m32xToolingCheatSignal = 0;
	uint32_t _m32xToolingEventCount = 0;
	uint8_t _m32xToolingDigest = 0;
	uint8_t _segaCdBridgeA120[0x20] = {};
	uint8_t _segaCdBridgeA130[0x20] = {};
	uint8_t _segaCdBridgeA140[0x20] = {};
	uint8_t _segaCdBridgeA150[0x20] = {};
	uint8_t _segaCdBridgeA160[0x20] = {};
	uint8_t _segaCdBridgeA180[0x20] = {};

	bool IsSramAddress(uint32_t addr) const;
	bool TryGetSramOffset(uint32_t addr, uint32_t& offset) const;
	bool TryGetSegaCdBridgeSlot(uint32_t addr, uint8_t*& slot, uint32_t& slotIndex);
	bool IsSegaCdSubCpuControlAddress(uint32_t addr) const;
	bool IsSegaCdAudioDataAddress(uint32_t addr) const;
	bool IsSegaCdAudioStatusAddress(uint32_t addr) const;
	bool IsSegaCdToolingControlAddress(uint32_t addr) const;
	bool IsSegaCdToolingStatusAddress(uint32_t addr) const;
	bool Is32xSh2ControlAddress(uint32_t addr) const;
	bool Is32xSh2StatusAddress(uint32_t addr) const;
	bool Is32xCompositionControlAddress(uint32_t addr) const;
	bool Is32xCompositionStatusAddress(uint32_t addr) const;
	bool Is32xToolingControlAddress(uint32_t addr) const;
	bool Is32xToolingStatusAddress(uint32_t addr) const;
	void UpdateSegaCdSubCpuControl(uint8_t value);
	void UpdateSegaCdAudioPath(uint32_t addr, uint8_t value);
	void UpdateSegaCdToolingContract(uint32_t addr, uint8_t value);
	void Update32xSh2Staging(uint32_t addr, uint8_t value);
	void Update32xCompositionStaging(uint32_t addr, uint8_t value);
	void Update32xToolingContract(uint32_t addr, uint8_t value);
	uint8_t GetSegaCdSubCpuStatusByte() const;
	uint8_t GetSegaCdAudioStatusByte(uint32_t addr) const;
	uint8_t GetSegaCdToolingStatusByte(uint32_t addr) const;
	uint8_t Get32xSh2StatusByte(uint32_t addr) const;
	uint8_t Get32xCompositionStatusByte(uint32_t addr) const;
	uint8_t Get32xToolingStatusByte(uint32_t addr) const;
	void TrackTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags);
	void TrackDebugTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags);
	void TrackSegaCdTranscript(uint32_t addr, bool isWrite, uint8_t value);
	void TrackSegaCdHandshakeTranscript(uint32_t addr, bool isWrite, uint8_t value);
	void ResetRomBankMapper();
	bool TryGetRomBankRegisterSlot(uint32_t addr, uint8_t& slot) const;
	bool TryWriteRomBankRegister(uint32_t addr, uint8_t value);
	uint32_t TranslateRomAddress(uint32_t addr) const;

public:
	GenesisMemoryManager();
	~GenesisMemoryManager();

	void Init(Emulator* emu, GenesisConsole* console, vector<uint8_t>& romData, GenesisVdp* vdp, GenesisControlManager* controlManager, GenesisPsg* psg);
	void LoadBattery();
	void SaveBattery();
	void ResetRuntimeState(bool hardReset);

	void SetCpu(GenesisM68k* cpu) { _cpu = cpu; }

	// Memory access (24-bit address space)
	uint8_t Read8(uint32_t addr);
	uint16_t Read16(uint32_t addr);
	void Write8(uint32_t addr, uint8_t value);
	void Write16(uint32_t addr, uint16_t value);

	// Debug access
	uint8_t DebugRead8(uint32_t addr);
	void DebugWrite8(uint32_t addr, uint8_t value);

	// VDP access
	uint16_t ReadVdpPort(uint32_t addr);
	void WriteVdpPort(uint32_t addr, uint16_t value);

	// I/O access
	uint8_t ReadIo(uint32_t addr);
	void WriteIo(uint32_t addr, uint8_t value);

	// Clock
	__forceinline void Exec(uint32_t cycles) {
		_masterClock += cycles;
		_vdp->Run(_masterClock);
	}

	uint64_t GetMasterClock() const { return _masterClock; }
	GenesisIoState GetIoState() const { return _ioState; }
	uint32_t GetDebugTranscriptLaneCount() const { return _ioState.DebugTranscriptLaneCount; }
	uint64_t GetDebugTranscriptLaneDigest() const { return _ioState.DebugTranscriptLaneDigest; }
	void ClearDebugTranscriptLane();
	bool HasSaveRam() const { return _hasSram && _saveRam && _saveRamSize > 0; }

	// Address info
	AddressInfo GetAbsoluteAddress(uint32_t addr);
	int32_t GetRelativeAddress(AddressInfo& absAddress);

	uint8_t GetOpenBus() const { return _openBus; }

	void Serialize(Serializer& s) override;
};
