#pragma once
#include "pch.h"

class IGenesisM68kBus {
public:
	virtual ~IGenesisM68kBus() = default;
	virtual uint8_t ReadByte(uint32_t address) = 0;
	virtual void WriteByte(uint32_t address, uint8_t value) = 0;
};

enum class GenesisBusOwner : uint8_t {
	Rom = 0,
	Z80 = 1,
	Io = 2,
	Vdp = 3,
	WorkRam = 4,
	OpenBus = 5
};

enum class GenesisVdpDmaMode : uint8_t {
	None = 0,
	Copy = 1,
	Fill = 2
};

struct GenesisPlatformBusSaveState {
	vector<uint8_t> Rom;
	vector<uint8_t> WorkRam;
	vector<uint8_t> Io;
	vector<uint8_t> ExpansionIo;
	vector<uint8_t> VdpIo;
	std::array<uint8_t, 0x20> VdpRegisters = {};
	uint16_t VdpStatus = 0;
	uint16_t VdpDataPortLatch = 0;
	uint16_t VdpControlWordLatch = 0;
	uint8_t PlaneASample = 0;
	uint8_t PlaneBSample = 0;
	uint8_t WindowSample = 0;
	uint8_t SpriteSample = 0;
	bool PlaneAPriority = false;
	bool PlaneBPriority = false;
	bool WindowEnabled = false;
	bool WindowPriority = false;
	bool SpritePriority = false;
	uint16_t ScrollX = 0;
	uint16_t ScrollY = 0;
	vector<uint8_t> RenderLine;
	string RenderLineDigest;
	GenesisVdpDmaMode DmaMode = GenesisVdpDmaMode::None;
	uint32_t DmaTransferWords = 0;
	uint32_t DmaActiveCyclesRemaining = 0;
	uint32_t DmaContentionCycles = 0;
	uint32_t DmaContentionEvents = 0;
	bool Z80Bootstrapped = false;
	bool Z80Running = false;
	bool Z80BusRequested = false;
	uint32_t Z80BootstrapCount = 0;
	uint32_t Z80HandoffCount = 0;
	uint64_t Z80ExecutedCycles = 0;
	std::array<uint8_t, 0x200> Ym2612Registers = {};
	uint8_t Ym2612AddressPort0 = 0;
	uint8_t Ym2612AddressPort1 = 0;
	uint64_t Ym2612ClockAccumulator = 0;
	uint32_t Ym2612SampleCount = 0;
	int16_t Ym2612LastSample = 0;
	uint32_t Ym2612WriteCount = 0;
	string Ym2612Digest;
	std::array<uint8_t, 16> Sn76489Registers = {};
	uint8_t Sn76489LatchedRegister = 0;
	uint64_t Sn76489ClockAccumulator = 0;
	uint32_t Sn76489SampleCount = 0;
	int16_t Sn76489LastSample = 0;
	uint32_t Sn76489WriteCount = 0;
	string Sn76489Digest;
	int16_t MixedLastSample = 0;
	uint32_t MixedSampleCount = 0;
	string MixedDigest;
	bool Z80WindowAccessed = false;
	bool IoWindowAccessed = false;
	bool VdpWindowAccessed = false;
	bool DmaRequested = false;
	uint32_t RomReadCount = 0;
	uint32_t Z80ReadCount = 0;
	uint32_t Z80WriteCount = 0;
	uint32_t IoReadCount = 0;
	uint32_t IoWriteCount = 0;
	uint32_t VdpReadCount = 0;
	uint32_t VdpWriteCount = 0;
	uint32_t WorkRamReadCount = 0;
	uint32_t WorkRamWriteCount = 0;
	uint32_t OpenBusReadCount = 0;
	uint32_t OpenBusWriteCount = 0;
	uint32_t LastVdpAddress = 0;
	uint8_t LastVdpValue = 0;
	uint32_t OwnershipTraceCount = 0;
	string OwnershipTraceDigest;
	uint32_t CommandResponseSequence = 0;
	uint32_t CommandResponseLaneCount = 0;
	string CommandResponseLaneDigest;
	vector<string> CommandResponseLane;

	bool operator==(const GenesisPlatformBusSaveState&) const = default;
};

struct GenesisM68kCpuSaveState {
	uint32_t ProgramCounter = 0;
	uint64_t CycleCount = 0;
	uint8_t InterruptLevel = 0;
	uint16_t StatusRegister = 0;
	uint32_t SupervisorStackPointer = 0;
	uint32_t LastExceptionVectorAddress = 0;
	uint32_t InterruptSequenceCount = 0;
	uint8_t InstructionCyclesRemaining = 0;

	bool operator==(const GenesisM68kCpuSaveState&) const = default;
};

struct GenesisBoundaryScaffoldSaveState {
	GenesisPlatformBusSaveState Bus;
	GenesisM68kCpuSaveState Cpu;
	bool Started = false;
	uint32_t TimingScanline = 0;
	uint32_t TimingFrame = 0;
	uint32_t TimingCycleRemainder = 0;
	bool HInterruptEnabled = true;
	bool VInterruptEnabled = true;
	uint32_t HInterruptIntervalScanlines = 16;
	uint32_t HInterruptCount = 0;
	uint32_t VInterruptCount = 0;
	vector<string> TimingEvents;

	bool operator==(const GenesisBoundaryScaffoldSaveState&) const = default;
};

class GenesisPlatformBusStub final : public IGenesisM68kBus {
private:
	vector<uint8_t> _rom;
	vector<uint8_t> _workRam;
	vector<uint8_t> _io;
	vector<uint8_t> _expansionIo;
	vector<uint8_t> _vdpIo;
	std::array<uint8_t, 0x20> _vdpRegisters = {};
	uint16_t _vdpStatus = 0x0001;
	uint16_t _vdpDataPortLatch = 0;
	uint16_t _vdpControlWordLatch = 0;
	uint8_t _planeASample = 0;
	uint8_t _planeBSample = 0;
	uint8_t _windowSample = 0;
	uint8_t _spriteSample = 0;
	bool _planeAPriority = false;
	bool _planeBPriority = false;
	bool _windowEnabled = false;
	bool _windowPriority = false;
	bool _spritePriority = false;
	uint16_t _scrollX = 0;
	uint16_t _scrollY = 0;
	vector<uint8_t> _renderLine;
	string _renderLineDigest;
	GenesisVdpDmaMode _dmaMode = GenesisVdpDmaMode::None;
	uint32_t _dmaTransferWords = 0;
	uint32_t _dmaActiveCyclesRemaining = 0;
	uint32_t _dmaContentionCycles = 0;
	uint32_t _dmaContentionEvents = 0;
	bool _z80Bootstrapped = false;
	bool _z80Running = false;
	bool _z80BusRequested = false;
	uint32_t _z80BootstrapCount = 0;
	uint32_t _z80HandoffCount = 0;
	uint64_t _z80ExecutedCycles = 0;
	std::array<uint8_t, 0x200> _ym2612Registers = {};
	uint8_t _ym2612AddressPort0 = 0;
	uint8_t _ym2612AddressPort1 = 0;
	uint64_t _ym2612ClockAccumulator = 0;
	uint32_t _ym2612SampleCount = 0;
	int16_t _ym2612LastSample = 0;
	uint32_t _ym2612WriteCount = 0;
	string _ym2612Digest;
	std::array<uint8_t, 16> _sn76489Registers = {};
	uint8_t _sn76489LatchedRegister = 0;
	uint64_t _sn76489ClockAccumulator = 0;
	uint32_t _sn76489SampleCount = 0;
	int16_t _sn76489LastSample = 0;
	uint32_t _sn76489WriteCount = 0;
	string _sn76489Digest;
	int16_t _mixedLastSample = 0;
	uint32_t _mixedSampleCount = 0;
	string _mixedDigest;
	bool _z80WindowAccessed = false;
	bool _ioWindowAccessed = false;
	bool _vdpWindowAccessed = false;
	bool _dmaRequested = false;
	uint32_t _romReadCount = 0;
	uint32_t _z80ReadCount = 0;
	uint32_t _z80WriteCount = 0;
	uint32_t _ioReadCount = 0;
	uint32_t _ioWriteCount = 0;
	uint32_t _vdpReadCount = 0;
	uint32_t _vdpWriteCount = 0;
	uint32_t _workRamReadCount = 0;
	uint32_t _workRamWriteCount = 0;
	uint32_t _openBusReadCount = 0;
	uint32_t _openBusWriteCount = 0;
	uint32_t _lastVdpAddress = 0;
	uint8_t _lastVdpValue = 0;
	uint32_t _ownershipTraceCount = 0;
	string _ownershipTraceDigest;
	uint32_t _commandResponseSequence = 0;
	uint32_t _commandResponseLaneCount = 0;
	string _commandResponseLaneDigest;
	vector<string> _commandResponseLane;

	[[nodiscard]] GenesisBusOwner DecodeOwner(uint32_t address) const;
	[[nodiscard]] bool TryGetExpansionIoOffset(uint32_t address, uint32_t& offset) const;
	[[nodiscard]] bool IsCommandResponseLaneAddress(uint32_t address) const;
	[[nodiscard]] const char* GetCommandResponseLaneRole(uint32_t address) const;
	void AppendOwnershipTrace(uint32_t address, GenesisBusOwner owner, bool isWrite, uint8_t value);
	void AppendCommandResponseLane(uint32_t address, bool isWrite, uint8_t value);
	void ApplyVdpControlWord(uint16_t controlWord);
	[[nodiscard]] uint8_t ComposeRenderPixel() const;
	[[nodiscard]] GenesisVdpDmaMode DecodeDmaModeFromControl(uint8_t registerValue) const;

public:
	GenesisPlatformBusStub();
	void LoadRom(const vector<uint8_t>& romData);
	void Reset();

	uint8_t ReadByte(uint32_t address) override;
	void WriteByte(uint32_t address, uint8_t value) override;

	[[nodiscard]] bool WasZ80WindowAccessed() const { return _z80WindowAccessed; }
	[[nodiscard]] bool WasIoWindowAccessed() const { return _ioWindowAccessed; }
	[[nodiscard]] bool WasVdpWindowAccessed() const { return _vdpWindowAccessed; }
	[[nodiscard]] bool WasDmaRequested() const { return _dmaRequested; }
	[[nodiscard]] uint32_t GetRomReadCount() const { return _romReadCount; }
	[[nodiscard]] uint32_t GetZ80ReadCount() const { return _z80ReadCount; }
	[[nodiscard]] uint32_t GetZ80WriteCount() const { return _z80WriteCount; }
	[[nodiscard]] uint32_t GetIoReadCount() const { return _ioReadCount; }
	[[nodiscard]] uint32_t GetIoWriteCount() const { return _ioWriteCount; }
	[[nodiscard]] uint32_t GetVdpReadCount() const { return _vdpReadCount; }
	[[nodiscard]] uint32_t GetVdpWriteCount() const { return _vdpWriteCount; }
	[[nodiscard]] uint32_t GetWorkRamReadCount() const { return _workRamReadCount; }
	[[nodiscard]] uint32_t GetWorkRamWriteCount() const { return _workRamWriteCount; }
	[[nodiscard]] uint32_t GetOpenBusReadCount() const { return _openBusReadCount; }
	[[nodiscard]] uint32_t GetOpenBusWriteCount() const { return _openBusWriteCount; }
	[[nodiscard]] uint32_t GetLastVdpAddress() const { return _lastVdpAddress; }
	[[nodiscard]] uint8_t GetLastVdpValue() const { return _lastVdpValue; }
	[[nodiscard]] uint32_t GetOwnershipTraceCount() const { return _ownershipTraceCount; }
	[[nodiscard]] const string& GetOwnershipTraceDigest() const { return _ownershipTraceDigest; }
	[[nodiscard]] uint32_t GetCommandResponseLaneCount() const { return _commandResponseLaneCount; }
	[[nodiscard]] const string& GetCommandResponseLaneDigest() const { return _commandResponseLaneDigest; }
	[[nodiscard]] const vector<string>& GetCommandResponseLane() const { return _commandResponseLane; }
	[[nodiscard]] uint32_t GetWorkRamSize() const { return (uint32_t)_workRam.size(); }
	[[nodiscard]] GenesisBusOwner GetOwnerForAddress(uint32_t address) const { return DecodeOwner(address); }
	[[nodiscard]] uint8_t GetVdpRegister(uint8_t index) const { return _vdpRegisters[index & 0x1F]; }
	[[nodiscard]] uint16_t GetVdpStatus() const { return _vdpStatus; }
	[[nodiscard]] uint16_t GetVdpDataPortLatch() const { return _vdpDataPortLatch; }
	[[nodiscard]] uint16_t GetVdpControlWordLatch() const { return _vdpControlWordLatch; }
	void SetRenderCompositionInputs(uint8_t planeA, bool planeAPriority, uint8_t planeB, bool planeBPriority, uint8_t window, bool windowEnabled, bool windowPriority, uint8_t sprite, bool spritePriority);
	void SetScroll(uint16_t scrollX, uint16_t scrollY);
	void RenderScaffoldLine(uint32_t pixelCount = 64);
	[[nodiscard]] uint8_t GetRenderLinePixel(uint32_t index) const;
	[[nodiscard]] const string& GetRenderLineDigest() const { return _renderLineDigest; }
	void BeginDmaTransfer(GenesisVdpDmaMode mode, uint32_t transferWords);
	uint32_t ConsumeDmaContention(uint32_t requestedCycles);
	void BootstrapZ80();
	void RequestZ80Bus(bool requestBusForM68k);
	void StepZ80Cycles(uint32_t cycles);
	void YmWriteAddress(uint8_t port, uint8_t value);
	void YmWriteData(uint8_t port, uint8_t value);
	void StepYm2612(uint32_t masterCycles);
	void PsgWrite(uint8_t value);
	void StepSn76489(uint32_t masterCycles);
	void UpdateMixedSample();
	[[nodiscard]] GenesisVdpDmaMode GetDmaMode() const { return _dmaMode; }
	[[nodiscard]] uint32_t GetDmaTransferWords() const { return _dmaTransferWords; }
	[[nodiscard]] uint32_t GetDmaActiveCyclesRemaining() const { return _dmaActiveCyclesRemaining; }
	[[nodiscard]] uint32_t GetDmaContentionCycles() const { return _dmaContentionCycles; }
	[[nodiscard]] uint32_t GetDmaContentionEvents() const { return _dmaContentionEvents; }
	[[nodiscard]] bool IsZ80Bootstrapped() const { return _z80Bootstrapped; }
	[[nodiscard]] bool IsZ80Running() const { return _z80Running; }
	[[nodiscard]] bool IsZ80BusRequested() const { return _z80BusRequested; }
	[[nodiscard]] uint32_t GetZ80BootstrapCount() const { return _z80BootstrapCount; }
	[[nodiscard]] uint32_t GetZ80HandoffCount() const { return _z80HandoffCount; }
	[[nodiscard]] uint64_t GetZ80ExecutedCycles() const { return _z80ExecutedCycles; }
	[[nodiscard]] uint8_t GetYmRegister(uint16_t index) const { return _ym2612Registers[index & 0x1FF]; }
	[[nodiscard]] uint32_t GetYmSampleCount() const { return _ym2612SampleCount; }
	[[nodiscard]] int16_t GetYmLastSample() const { return _ym2612LastSample; }
	[[nodiscard]] uint32_t GetYmWriteCount() const { return _ym2612WriteCount; }
	[[nodiscard]] const string& GetYmDigest() const { return _ym2612Digest; }
	[[nodiscard]] uint8_t GetPsgRegister(uint8_t index) const { return _sn76489Registers[index & 0x0F]; }
	[[nodiscard]] uint32_t GetPsgSampleCount() const { return _sn76489SampleCount; }
	[[nodiscard]] int16_t GetPsgLastSample() const { return _sn76489LastSample; }
	[[nodiscard]] uint32_t GetPsgWriteCount() const { return _sn76489WriteCount; }
	[[nodiscard]] const string& GetPsgDigest() const { return _sn76489Digest; }
	[[nodiscard]] int16_t GetMixedLastSample() const { return _mixedLastSample; }
	[[nodiscard]] uint32_t GetMixedSampleCount() const { return _mixedSampleCount; }
	[[nodiscard]] const string& GetMixedDigest() const { return _mixedDigest; }
	void ClearOwnershipTrace();
	void ClearCommandResponseLane();
	[[nodiscard]] GenesisPlatformBusSaveState SaveState() const;
	void LoadState(const GenesisPlatformBusSaveState& state);
};

class GenesisM68kCpuStub {
private:
	IGenesisM68kBus* _bus = nullptr;
	uint32_t _programCounter = 0;
	uint64_t _cycleCount = 0;
	uint8_t _interruptLevel = 0;
	uint16_t _statusRegister = 0x2000;
	uint32_t _supervisorStackPointer = 0xFFFFFE;
	uint32_t _lastExceptionVectorAddress = 0;
	uint32_t _interruptSequenceCount = 0;
	uint8_t _instructionCyclesRemaining = 0;

	[[nodiscard]] uint16_t ReadWord(uint32_t address) const;
	[[nodiscard]] uint32_t ReadLong(uint32_t address) const;
	void WriteWord(uint32_t address, uint16_t value);
	void WriteLong(uint32_t address, uint32_t value);
	void BeginInterruptSequence(uint8_t level);
	void BeginNextInstruction();

public:
	void AttachBus(IGenesisM68kBus* bus);
	void Reset();
	void StepCycles(uint32_t cycles);
	void SetInterrupt(uint8_t level);

	[[nodiscard]] uint32_t GetProgramCounter() const { return _programCounter; }
	[[nodiscard]] uint64_t GetCycleCount() const { return _cycleCount; }
	[[nodiscard]] uint8_t GetInterruptLevel() const { return _interruptLevel; }
	[[nodiscard]] uint16_t GetStatusRegister() const { return _statusRegister; }
	[[nodiscard]] uint32_t GetSupervisorStackPointer() const { return _supervisorStackPointer; }
	[[nodiscard]] uint32_t GetLastExceptionVectorAddress() const { return _lastExceptionVectorAddress; }
	[[nodiscard]] uint32_t GetInterruptSequenceCount() const { return _interruptSequenceCount; }
	[[nodiscard]] GenesisM68kCpuSaveState SaveState() const;
	void LoadState(const GenesisM68kCpuSaveState& state);
};

class GenesisM68kBoundaryScaffold {
private:
	static constexpr uint32_t TimingCyclesPerScanline = 488;
	static constexpr uint32_t TimingScanlinesPerFrame = 262;

	GenesisPlatformBusStub _bus;
	GenesisM68kCpuStub _cpu;
	bool _started = false;
	uint32_t _timingScanline = 0;
	uint32_t _timingFrame = 0;
	uint32_t _timingCycleRemainder = 0;
	bool _hInterruptEnabled = true;
	bool _vInterruptEnabled = true;
	uint32_t _hInterruptIntervalScanlines = 16;
	uint32_t _hInterruptCount = 0;
	uint32_t _vInterruptCount = 0;
	vector<string> _timingEvents;

	void AdvanceTiming(uint32_t cpuCycles);

public:
	GenesisM68kBoundaryScaffold();
	void LoadRom(const vector<uint8_t>& romData);
	void Startup();
	void StepFrameScaffold(uint32_t cpuCycles = 12000);
	void ConfigureInterruptSchedule(bool hInterruptEnabled, uint32_t hInterruptIntervalScanlines, bool vInterruptEnabled);
	void ClearTimingEvents();

	[[nodiscard]] bool IsStarted() const { return _started; }
	[[nodiscard]] uint32_t GetTimingScanline() const { return _timingScanline; }
	[[nodiscard]] uint32_t GetTimingFrame() const { return _timingFrame; }
	[[nodiscard]] uint32_t GetHorizontalInterruptCount() const { return _hInterruptCount; }
	[[nodiscard]] uint32_t GetVerticalInterruptCount() const { return _vInterruptCount; }
	[[nodiscard]] const vector<string>& GetTimingEvents() const { return _timingEvents; }
	[[nodiscard]] GenesisBoundaryScaffoldSaveState SaveState() const;
	void LoadState(const GenesisBoundaryScaffoldSaveState& state);
	GenesisM68kCpuStub& GetCpu() { return _cpu; }
	GenesisPlatformBusStub& GetBus() { return _bus; }
};
