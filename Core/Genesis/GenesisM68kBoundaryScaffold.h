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

class GenesisPlatformBusStub final : public IGenesisM68kBus {
private:
	vector<uint8_t> _rom;
	vector<uint8_t> _workRam;
	vector<uint8_t> _io;
	vector<uint8_t> _vdpIo;
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

	[[nodiscard]] GenesisBusOwner DecodeOwner(uint32_t address) const;

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
	[[nodiscard]] uint32_t GetWorkRamSize() const { return (uint32_t)_workRam.size(); }
	[[nodiscard]] GenesisBusOwner GetOwnerForAddress(uint32_t address) const { return DecodeOwner(address); }
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

	void AdvanceTiming(uint32_t cpuCycles);

public:
	GenesisM68kBoundaryScaffold();
	void LoadRom(const vector<uint8_t>& romData);
	void Startup();
	void StepFrameScaffold(uint32_t cpuCycles = 12000);

	[[nodiscard]] bool IsStarted() const { return _started; }
	[[nodiscard]] uint32_t GetTimingScanline() const { return _timingScanline; }
	[[nodiscard]] uint32_t GetTimingFrame() const { return _timingFrame; }
	GenesisM68kCpuStub& GetCpu() { return _cpu; }
	GenesisPlatformBusStub& GetBus() { return _bus; }
};
