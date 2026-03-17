#pragma once
#include "pch.h"

class IGenesisM68kBus {
public:
	virtual ~IGenesisM68kBus() = default;
	virtual uint8_t ReadByte(uint32_t address) = 0;
	virtual void WriteByte(uint32_t address, uint8_t value) = 0;
};

class GenesisPlatformBusStub final : public IGenesisM68kBus {
private:
	vector<uint8_t> _rom;
	vector<uint8_t> _workRam;
	vector<uint8_t> _vdpIo;
	bool _z80WindowAccessed = false;
	bool _vdpWindowAccessed = false;
	bool _dmaRequested = false;
	uint32_t _vdpReadCount = 0;
	uint32_t _vdpWriteCount = 0;
	uint32_t _lastVdpAddress = 0;
	uint8_t _lastVdpValue = 0;

public:
	GenesisPlatformBusStub();
	void LoadRom(const vector<uint8_t>& romData);
	void Reset();

	uint8_t ReadByte(uint32_t address) override;
	void WriteByte(uint32_t address, uint8_t value) override;

	[[nodiscard]] bool WasZ80WindowAccessed() const { return _z80WindowAccessed; }
	[[nodiscard]] bool WasVdpWindowAccessed() const { return _vdpWindowAccessed; }
	[[nodiscard]] bool WasDmaRequested() const { return _dmaRequested; }
	[[nodiscard]] uint32_t GetVdpReadCount() const { return _vdpReadCount; }
	[[nodiscard]] uint32_t GetVdpWriteCount() const { return _vdpWriteCount; }
	[[nodiscard]] uint32_t GetLastVdpAddress() const { return _lastVdpAddress; }
	[[nodiscard]] uint8_t GetLastVdpValue() const { return _lastVdpValue; }
	[[nodiscard]] uint32_t GetWorkRamSize() const { return (uint32_t)_workRam.size(); }
};

class GenesisM68kCpuStub {
private:
	IGenesisM68kBus* _bus = nullptr;
	uint32_t _programCounter = 0;
	uint64_t _cycleCount = 0;
	uint8_t _interruptLevel = 0;

public:
	void AttachBus(IGenesisM68kBus* bus);
	void Reset();
	void StepCycles(uint32_t cycles);
	void SetInterrupt(uint8_t level);

	[[nodiscard]] uint32_t GetProgramCounter() const { return _programCounter; }
	[[nodiscard]] uint64_t GetCycleCount() const { return _cycleCount; }
	[[nodiscard]] uint8_t GetInterruptLevel() const { return _interruptLevel; }
};

class GenesisM68kBoundaryScaffold {
private:
	GenesisPlatformBusStub _bus;
	GenesisM68kCpuStub _cpu;
	bool _started = false;

public:
	GenesisM68kBoundaryScaffold();
	void LoadRom(const vector<uint8_t>& romData);
	void Startup();
	void StepFrameScaffold(uint32_t cpuCycles = 12000);

	[[nodiscard]] bool IsStarted() const { return _started; }
	GenesisM68kCpuStub& GetCpu() { return _cpu; }
	GenesisPlatformBusStub& GetBus() { return _bus; }
};
