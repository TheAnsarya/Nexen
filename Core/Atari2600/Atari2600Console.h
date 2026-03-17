#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"

class Emulator;
class BaseControlManager;
class Atari2600CpuAdapter;
class Atari2600Riot;
class Atari2600Tia;
class Atari2600Mapper;
class Atari2600Bus;

struct Atari2600RiotState {
	uint8_t PortA = 0;
	uint8_t PortB = 0;
	uint16_t Timer = 0;
	bool TimerUnderflow = false;
	uint64_t CpuCycles = 0;
};

struct Atari2600TiaState {
	uint32_t FrameCount = 0;
	uint32_t Scanline = 0;
	uint32_t ColorClock = 0;
	bool WsyncHold = false;
	uint64_t TotalColorClocks = 0;
};

struct Atari2600FrameStepSummary {
	uint32_t FrameCount = 0;
	uint32_t CpuCyclesThisFrame = 0;
	uint32_t ScanlineAtFrameEnd = 0;
	uint32_t ColorClockAtFrameEnd = 0;
};

class Atari2600Console final : public IConsole {
private:
	Emulator* _emu = nullptr;
	unique_ptr<Atari2600CpuAdapter> _cpu;
	unique_ptr<Atari2600Riot> _riot;
	unique_ptr<Atari2600Tia> _tia;
	unique_ptr<Atari2600Mapper> _mapper;
	unique_ptr<Atari2600Bus> _bus;
	unique_ptr<BaseControlManager> _controlManager;
	vector<uint16_t> _frameBuffer;
	Atari2600FrameStepSummary _lastFrameSummary = {};
	bool _romLoaded = false;

	void RenderDebugFrame();

public:
	static constexpr uint32_t ScreenWidth = 160;
	static constexpr uint32_t ScreenHeight = 192;
	static constexpr uint32_t CpuCyclesPerFrame = 19912;

	[[nodiscard]] static vector<string> GetSupportedExtensions() { return {".a26"}; }
	[[nodiscard]] static vector<string> GetSupportedSignatures() { return {}; }

	explicit Atari2600Console(Emulator* emu);
	~Atari2600Console() override;

	LoadRomResult LoadRom(VirtualFile& romFile) override;
	void Reset() override;
	void RunFrame() override;
	void SaveBattery() override;

	BaseControlManager* GetControlManager() override;
	ConsoleRegion GetRegion() override;
	ConsoleType GetConsoleType() override;
	vector<CpuType> GetCpuTypes() override;
	uint64_t GetMasterClock() override;
	uint32_t GetMasterClockRate() override;
	double GetFps() override;
	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;
	PpuFrameInfo GetPpuFrame() override;
	RomFormat GetRomFormat() override;
	AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;
	AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	AddressInfo GetPcAbsoluteAddress() override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;
	void Serialize(Serializer& s) override;

	void StepCpuCycles(uint32_t cpuCycles);
	void RequestWsync();
	Atari2600RiotState GetRiotState() const;
	Atari2600TiaState GetTiaState() const;
	Atari2600FrameStepSummary GetLastFrameSummary() const { return _lastFrameSummary; }
	uint8_t DebugReadCartridge(uint16_t addr) const;
	void DebugWriteCartridge(uint16_t addr, uint8_t value);
	uint8_t DebugGetMapperBankIndex() const;
	string DebugGetMapperMode() const;
};
