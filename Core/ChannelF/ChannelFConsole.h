#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"
#include "ChannelF/ChannelFCoreScaffold.h"
#include "ChannelF/ChannelFCpu.h"
#include "ChannelF/ChannelFMemoryManager.h"
#include "ChannelF/ChannelFControlManager.h"
#include "ChannelF/ChannelFDefaultVideoFilter.h"

class Emulator;

class ChannelFConsole final : public IConsole {
public:
	static constexpr uint32_t ScreenWidth = 128;
	static constexpr uint32_t ScreenHeight = 64;
	static constexpr uint32_t NtscCpuClockHz = 1789773;
	static constexpr uint32_t PalCpuClockHz = 1773447;
	static constexpr double NtscFps = 60.0;
	static constexpr double PalFps = 50.0;
	static constexpr uint32_t NtscScanlineCount = 262;
	static constexpr uint32_t PalScanlineCount = 312;

private:
	Emulator* _emu = nullptr;
	ChannelFCoreScaffold _core;
	unique_ptr<ChannelFCpu> _cpu;
	unique_ptr<ChannelFMemoryManager> _memoryManager;
	unique_ptr<ChannelFControlManager> _controlManager;
	vector<uint16_t> _frameBuffer;
	uint32_t _frameCount = 0;
	RomFormat _romFormat = RomFormat::Unknown;
	string _romSha1;
	bool _romLoaded = false;
	ChannelFBiosVariant _activeVariant = ChannelFBiosVariant::Unknown;
	ConsoleRegion _activeRegion = ConsoleRegion::Ntsc;

	// Scanline timing state
	uint16_t _scanline = 0;
	uint16_t _scanlineCycle = 0;

	[[nodiscard]] uint32_t GetCyclesPerFrame() const;
	[[nodiscard]] uint32_t GetScanlineCount() const;
	void RenderScanline(uint16_t line);

public:
	[[nodiscard]] static vector<string> GetSupportedExtensions() { return {".chf", ".bin"}; }
	[[nodiscard]] static vector<string> GetSupportedSignatures() { return {}; }

	explicit ChannelFConsole(Emulator* emu);
	~ChannelFConsole() override;

	LoadRomResult LoadRom(VirtualFile& romFile) override;
	void RunFrame() override;
	void Reset() override;
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
	string GetHash(HashType hashType) override;
	void Serialize(Serializer& s) override;

	// Debugger helpers
	ChannelFCpuState GetCpuState();
	void SetCpuState(ChannelFCpuState& state);
	uint8_t DebugRead(uint16_t addr);
	void DebugRenderFrame();
	uint32_t GetFrameCount();
};
