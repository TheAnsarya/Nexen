#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"
#include "Atari2600/Atari2600Types.h"

class Emulator;
class BaseControlManager;
class Atari2600CpuAdapter;
class Atari2600Riot;
class Atari2600Tia;
class Atari2600Mapper;
class Atari2600Bus;

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
	static constexpr uint32_t ScreenWidth = Atari2600Constants::ScreenWidth;
	static constexpr uint32_t ScreenHeight = Atari2600Constants::ScreenHeight;
	static constexpr uint32_t CpuCyclesPerFrame = Atari2600Constants::CpuCyclesPerFrame;

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
	void RequestHmove();
	Atari2600RiotState GetRiotState() const;
	Atari2600TiaState GetTiaState() const;
	void SetTiaState(const Atari2600TiaState& state);
	Atari2600FrameStepSummary GetLastFrameSummary() const { return _lastFrameSummary; }
	uint8_t DebugReadCartridge(uint16_t addr);
	void DebugWriteCartridge(uint16_t addr, uint8_t value);
	Atari2600ScanlineRenderState DebugGetScanlineRenderState(uint32_t scanline) const;
	uint8_t DebugGetMapperBankIndex() const;
	string DebugGetMapperMode() const;

	// Debugger accessors
	Atari2600CpuState GetCpuState() const;
	void SetCpuState(const Atari2600CpuState& state);
	uint8_t DebugRead(uint16_t addr);
	void DebugWrite(uint16_t addr, uint8_t value);
	uint32_t GetFrameCount() const;
	uint32_t GetCurrentScanline() const;
	uint32_t GetCurrentColorClock() const;
	uint32_t* GetFrameBuffer();
	void DebugRenderFrame();
};
