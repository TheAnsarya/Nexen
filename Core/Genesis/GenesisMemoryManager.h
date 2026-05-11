#pragma once
#include "pch.h"
#include "Genesis/GenesisTypes.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisControlManager.h"
#include "Shared/Emulator.h"
#include "Shared/MemoryOperationType.h"
#include "Debugger/AddressInfo.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

class Emulator;
class GenesisConsole;
class GenesisM68k;
class GenesisVdp;
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
	bool _z80BusAck = false;
	uint16_t _z80BusReqDelayMclk = 0;
	uint16_t _z80ResumeDelayMclk = 0;
	static constexpr uint16_t DefaultZ80BusReqAckDelayMclk = 7;
	static constexpr uint16_t DefaultZ80BusResumeDelayMclk = 7;
	uint16_t _z80BusReqAckDelayMclkSetting = DefaultZ80BusReqAckDelayMclk;
	uint16_t _z80BusResumeDelayMclkSetting = DefaultZ80BusResumeDelayMclk;
	bool _z80LatchOnlyHighByteWrites = true;
	bool _z80RuntimeRunning = false;
	uint64_t _z80RuntimeRunnableCycles = 0;
	uint64_t _z80RuntimeStalledCycles = 0;
	uint64_t _z80RuntimeTransitionCount = 0;
	uint64_t _z80RuntimeStateEpoch = 0;
	uint64_t _z80RuntimeLastTransitionClock = 0;
	bool _romBankMapperEnabled = false;
	uint8_t _romBankRegisters[MapperBankWindowCount] = {};
	bool _ramEnable = false;
	bool _ramWritable = true;

	// TMSS (Trademark Security System)
	bool _tmssEnabled = false;
	bool _tmssStrictMode = false;
	bool _tmssUnlocked = false;
	bool _tmssVdpBlockLogged = false;
	bool _tmssStartupBypassLogged = false;
	bool _tmssUnlockPending = false;
	uint16_t _tmssUnlockDelayMclk = 0;
	uint16_t _tmssUnlockDelayMclkSetting = 0;
	uint8_t _tmssCartRegister = 0;
	uint8_t _startupProfileKindValue = 0;
	uint32_t _startupWindowFrames = 8;
	uint32_t _startupBootRelaxFrames = 0;
	uint32_t _startupLogoPhaseEndFrame = 0;
	uint32_t _startupStrictPhaseStartFrame = 0;
	uint32_t _startupBusTimingRetuneCount = 0;
	uint32_t _startupLastBusTimingFrame = 0;
	uint16_t _startupEarlyBusReqAckDelayMclk = DefaultZ80BusReqAckDelayMclk;
	uint16_t _startupEarlyBusResumeDelayMclk = DefaultZ80BusResumeDelayMclk;
	uint16_t _startupLateBusReqAckDelayMclk = DefaultZ80BusReqAckDelayMclk;
	uint16_t _startupLateBusResumeDelayMclk = DefaultZ80BusResumeDelayMclk;
	bool _startupUseDynamicBusTiming = false;
	bool _startupMesenCompatMode = false;
	bool _startupHybridBusHandoff = false;
	bool _startupStrictTmssDuringLogo = false;
	bool _startupForceTmssUntilUnlock = false;
	bool _startupHadTmssSignature = false;
	bool _startupTmssUnlockLogged = false;
	uint32_t _startupTraceSequence = 0;
	uint64_t _startupTraceDigest = 0;
	bool _startupHasNexenClockAnchor = false;
	uint64_t _startupNexenClockAnchor = 0;
	bool _startupHasNexenPcAnchor = false;
	uint32_t _startupNexenPcAnchor = 0;
	uint32_t _startupCheckpointIntervalFrames = 1;
	uint32_t _startupCheckpointEndFrame = 600;
	uint32_t _startupNextCheckpointFrame = 0;
	uint32_t _startupDisplayTransitionCount = 0;
	bool _startupLastDisplayEnabled = false;
	bool _startupHasLastDisplayState = false;
	bool _startupHasLastZ80RunState = false;
	bool _startupLastZ80Running = false;
	bool _startupHasLastZ80BusReqState = false;
	bool _startupLastZ80BusReq = false;
	bool _startupHasLastZ80ResetState = false;
	bool _startupLastZ80Reset = false;
	bool _startupHasLastVdpRegs = false;
	uint8_t _startupLastVdpRegs[24] = {};
	uint16_t _startupLastVdpStatus = 0;
	bool _startupProfilePreferNexenBusHandoff = true;
	bool _startupProfilePreferMesenBusHandoff = false;
	uint8_t _startupArbitrationDigest = 0;
	uint8_t _startupArbitrationEpoch = 0;
	uint16_t _startupLastArbitrationMclk = 0;
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
	uint8_t _m32xCoprocMasterSignal = 0;
	uint8_t _m32xCoprocSlaveSignal = 0;
	uint8_t _m32xCoprocPhaseSignal = 0;
	uint8_t _m32xCoprocFenceSignal = 0;
	uint32_t _m32xCoprocEventCount = 0;
	uint8_t _m32xCoprocDigest = 0;
	uint32_t _m32xCoprocEdgeCount = 0;
	uint16_t _m32xCoprocPhaseEpoch = 0;
	uint8_t _m32xCoprocFenceEpoch = 0;
	uint8_t _m32xCoprocArbiterLatch = 0;
	uint8_t _m32xHostDebuggerSignal = 0;
	uint8_t _m32xHostTasSignal = 0;
	uint8_t _m32xHostSaveStateSignal = 0;
	uint8_t _m32xHostCheatSignal = 0;
	uint32_t _m32xHostEventCount = 0;
	uint8_t _m32xHostDigest = 0;
	uint32_t _m32xHostEdgeCount = 0;
	uint8_t _m32xHostCommandNonce = 0;
	uint8_t _m32xHostAckToken = 0;
	uint8_t _m32xHostDeterminismLatch = 0;
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
	bool Is32xCoprocControlAddress(uint32_t addr) const;
	bool Is32xCoprocStatusAddress(uint32_t addr) const;
	bool Is32xHostToolingControlAddress(uint32_t addr) const;
	bool Is32xHostToolingStatusAddress(uint32_t addr) const;
	void UpdateSegaCdSubCpuControl(uint8_t value);
	void UpdateSegaCdAudioPath(uint32_t addr, uint8_t value);
	void UpdateSegaCdToolingContract(uint32_t addr, uint8_t value);
	void Update32xSh2Staging(uint32_t addr, uint8_t value);
	void Update32xCompositionStaging(uint32_t addr, uint8_t value);
	void Update32xToolingContract(uint32_t addr, uint8_t value);
	void Update32xCoprocContract(uint32_t addr, uint8_t value);
	void Update32xHostToolingContract(uint32_t addr, uint8_t value);
	uint8_t Normalize32xCoprocControlValue(uint32_t addr, uint8_t value) const;
	uint8_t Normalize32xHostControlValue(uint32_t addr, uint8_t value) const;
	void Recompute32xCoprocDigest();
	void Recompute32xHostDigest();
	uint8_t GetSegaCdSubCpuStatusByte() const;
	uint8_t GetSegaCdAudioStatusByte(uint32_t addr) const;
	uint8_t GetSegaCdToolingStatusByte(uint32_t addr) const;
	uint8_t Get32xSh2StatusByte(uint32_t addr) const;
	uint8_t Get32xCompositionStatusByte(uint32_t addr) const;
	uint8_t Get32xToolingStatusByte(uint32_t addr) const;
	uint8_t Get32xCoprocStatusByte(uint32_t addr) const;
	uint8_t Get32xHostToolingStatusByte(uint32_t addr) const;
	void TrackTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags);
	void TrackDebugTranscriptEntry(uint32_t addr, bool isWrite, uint8_t value, uint8_t roleFlags);
	void TrackSegaCdTranscript(uint32_t addr, bool isWrite, uint8_t value);
	void TrackSegaCdHandshakeTranscript(uint32_t addr, bool isWrite, uint8_t value);
	void ResetRomBankMapper();
	bool TryGetRomBankRegisterSlot(uint32_t addr, uint8_t& slot) const;
	bool TryWriteRomBankRegister(uint32_t addr, uint8_t value);
	bool IsRamControlRegister(uint32_t addr) const;
	uint8_t GetRamControlRegisterValue() const;
	void WriteRamControlRegister(uint8_t value);
	uint32_t TranslateRomAddress(uint32_t addr) const;
	bool IsZ80BusGranted() const;
	void AdvanceZ80BusArbitration(uint32_t masterClocks);
	void SetZ80BusRequest(bool request, bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag);
	void SetZ80Reset(bool resetAsserted, bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag);
	bool ComputeZ80RuntimeRunning() const;
	void UpdateZ80RuntimeState(bool allowTransitionLog, uint32_t addr, uint32_t pc, const char* sourceTag);
	uint32_t GetStartupFrame() const;
	bool IsStartupLogoPhase(uint32_t frame) const;
	bool IsStartupStrictPhase(uint32_t frame) const;
	void RefreshStartupBusTiming(uint32_t frame, bool allowTrace, uint32_t addr, uint32_t pc, const char* sourceTag);
	uint16_t GetEffectiveZ80BusReqAckDelayMclk(uint32_t frame) const;
	uint16_t GetEffectiveZ80BusResumeDelayMclk(uint32_t frame) const;
	bool ShouldAllowTmssStartupBypassPort(uint32_t addr, bool isWrite) const;
	void EvaluateTmssUnlockState(bool allowLog, uint32_t addr, uint32_t value, bool isWrite);
	void UpdateTmssUnlockWindow(uint32_t masterClocks);
	void ApplyStartupEnvironmentProfile();
	void EmitStartupTransitionMarkers();
	void EmitStartupCheckpointIfNeeded(const char* sourceTag);
	bool IsTmssVdpLockEnforced() const;
	bool IsStartupWindowActive() const;
	bool IsTmssLockedVdpReadAllowed(uint32_t addr) const;
	bool IsTmssLockedVdpWriteAllowed(uint32_t addr) const;
	void TraceStartupEvent(const char* tag, uint32_t addr, uint16_t value, uint16_t auxValue = 0);
	uint8_t ReadVersionRegister() const;
	void SyncIoPadRuntimeState(uint8_t port);
	uint8_t ReadIoDataPort(uint8_t port);
	uint8_t ReadIoControlPort(uint8_t port);
	void WriteIoDataPort(uint8_t port, uint8_t value);
	void WriteIoControlPort(uint8_t port, uint8_t value);

public:
	GenesisMemoryManager();
	~GenesisMemoryManager();

	void Init(Emulator* emu, GenesisConsole* console, vector<uint8_t>& romData, GenesisVdp* vdp, GenesisControlManager* controlManager, GenesisPsg* psg);
	void LoadBattery();
	void SaveBattery();
	void ResetRuntimeState(bool hardReset);

	void SetCpu(GenesisM68k* cpu) { _cpu = cpu; }
	void UpdateExecutionHeartbeat(uint32_t instructionProgramCounter, uint64_t cycleCount);

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
	void TraceVdpStartupEvent(const char* tag, uint16_t value, uint16_t auxValue = 0) {
		TraceStartupEvent(tag, 0xC00000, value, auxValue);
	}

	// I/O access
	uint8_t ReadIo(uint32_t addr);
	void WriteIo(uint32_t addr, uint8_t value);

	// Clock
	__forceinline void Exec(uint32_t cycles) {
		_masterClock += cycles;
		AdvanceZ80BusArbitration(cycles);
		if (_controlManager) {
			_controlManager->AdvanceMasterClock(cycles);
		}
		UpdateTmssUnlockWindow(cycles);
		UpdateZ80RuntimeState(false, 0, 0, "exec");
		EmitStartupCheckpointIfNeeded("exec");
		if (_z80RuntimeRunning) {
			_z80RuntimeRunnableCycles += cycles;
		} else {
			_z80RuntimeStalledCycles += cycles;
		}
		_vdp->Run(_masterClock);
		EmitStartupTransitionMarkers();
	}

	uint64_t GetMasterClock() const { return _masterClock; }
	GenesisIoState GetIoState() const { return _ioState; }
	bool GetZ80RuntimeRunning() const { return _z80RuntimeRunning; }
	uint64_t GetZ80RuntimeRunnableCycles() const { return _z80RuntimeRunnableCycles; }
	uint64_t GetZ80RuntimeStalledCycles() const { return _z80RuntimeStalledCycles; }
	uint64_t GetZ80RuntimeTransitionCount() const { return _z80RuntimeTransitionCount; }
	uint64_t GetZ80RuntimeStateEpoch() const { return _z80RuntimeStateEpoch; }
	uint64_t GetZ80RuntimeLastTransitionClock() const { return _z80RuntimeLastTransitionClock; }
	bool GetZ80BusRequestLatched() const { return _z80BusRequest; }
	bool GetZ80ResetAsserted() const { return _z80Reset; }
	bool GetZ80BusAckLatched() const { return _z80BusAck; }
	uint16_t GetZ80BusReqDelayMclk() const { return _z80BusReqDelayMclk; }
	uint16_t GetZ80ResumeDelayMclk() const { return _z80ResumeDelayMclk; }
	uint16_t GetZ80BusReqAckDelaySettingMclk() const { return _z80BusReqAckDelayMclkSetting; }
	uint16_t GetZ80BusResumeDelaySettingMclk() const { return _z80BusResumeDelayMclkSetting; }
	uint32_t GetStartupWindowFrames() const { return _startupWindowFrames; }
	uint32_t GetStartupBootRelaxFrames() const { return _startupBootRelaxFrames; }
	uint32_t GetStartupLogoPhaseEndFrame() const { return _startupLogoPhaseEndFrame; }
	uint32_t GetStartupStrictPhaseStartFrame() const { return _startupStrictPhaseStartFrame; }
	uint32_t GetStartupCheckpointEndFrame() const { return _startupCheckpointEndFrame; }
	uint32_t GetStartupCheckpointIntervalFrames() const { return _startupCheckpointIntervalFrames; }
	uint32_t GetStartupDisplayTransitionCount() const { return _startupDisplayTransitionCount; }
	bool GetStartupProfilePreferNexenBusHandoff() const { return _startupProfilePreferNexenBusHandoff; }
	bool GetStartupProfilePreferMesenBusHandoff() const { return _startupProfilePreferMesenBusHandoff; }
	bool GetStartupUseDynamicBusTiming() const { return _startupUseDynamicBusTiming; }
	bool GetTmssStrictMode() const { return _tmssStrictMode; }
	bool GetTmssEnabled() const { return _tmssEnabled; }
	bool GetTmssUnlocked() const { return _tmssUnlocked; }
	bool GetTmssUnlockPending() const { return _tmssUnlockPending; }
	uint16_t GetTmssUnlockDelayMclk() const { return _tmssUnlockDelayMclk; }
	bool GetStartupForceTmssUntilUnlock() const { return _startupForceTmssUntilUnlock; }
	bool GetStartupStrictTmssDuringLogo() const { return _startupStrictTmssDuringLogo; }
	bool GetStartupHadTmssSignature() const { return _startupHadTmssSignature; }
	bool IsStartupLogoPhaseForFrame(uint32_t frame) const { return IsStartupLogoPhase(frame); }
	bool IsStartupStrictPhaseForFrame(uint32_t frame) const { return IsStartupStrictPhase(frame); }
	uint16_t GetStartupEarlyBusReqAckDelayMclk() const { return _startupEarlyBusReqAckDelayMclk; }
	uint16_t GetStartupEarlyBusResumeDelayMclk() const { return _startupEarlyBusResumeDelayMclk; }
	uint16_t GetStartupLateBusReqAckDelayMclk() const { return _startupLateBusReqAckDelayMclk; }
	uint16_t GetStartupLateBusResumeDelayMclk() const { return _startupLateBusResumeDelayMclk; }
	uint16_t GetEffectiveZ80BusReqAckDelayForFrame(uint32_t frame) const { return GetEffectiveZ80BusReqAckDelayMclk(frame); }
	uint16_t GetEffectiveZ80BusResumeDelayForFrame(uint32_t frame) const { return GetEffectiveZ80BusResumeDelayMclk(frame); }
	void RefreshStartupBusTimingForFrame(uint32_t frame) { RefreshStartupBusTiming(frame, false, 0, 0, "test"); }
	uint32_t GetStartupBusTimingRetuneCount() const { return _startupBusTimingRetuneCount; }
	uint32_t GetStartupLastBusTimingFrame() const { return _startupLastBusTimingFrame; }
	uint32_t GetStartupTraceSequence() const { return _startupTraceSequence; }
	uint64_t GetStartupTraceDigest() const { return _startupTraceDigest; }
	uint8_t GetStartupArbitrationDigest() const { return _startupArbitrationDigest; }
	uint8_t GetStartupArbitrationEpoch() const { return _startupArbitrationEpoch; }
	uint16_t GetStartupLastArbitrationMclk() const { return _startupLastArbitrationMclk; }
	bool IsTmssLockedReadAllowedForAddr(uint32_t addr) const { return IsTmssLockedVdpReadAllowed(addr); }
	bool IsTmssLockedWriteAllowedForAddr(uint32_t addr) const { return IsTmssLockedVdpWriteAllowed(addr); }
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

