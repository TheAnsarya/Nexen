#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"
#include "Shared/SettingTypes.h"
#include "Genesis/GenesisTypes.h"

class Emulator;
class VirtualFile;
class BaseControlManager;

class GenesisM68k;
class GenesisVdp;
class GenesisControlManager;
class GenesisMemoryManager;
class GenesisPsg;

class GenesisConsole final : public IConsole {
private:
	Emulator* _emu = nullptr;
	unique_ptr<GenesisM68k> _cpu;
	unique_ptr<GenesisVdp> _vdp;
	unique_ptr<GenesisControlManager> _controlManager;
	unique_ptr<GenesisMemoryManager> _memoryManager;
	unique_ptr<GenesisPsg> _psg;
	ConsoleRegion _region = ConsoleRegion::Ntsc;

public:
	static vector<string> GetSupportedExtensions() { return {".md", ".gen", ".bin", ".smd"}; }
	static vector<string> GetSupportedSignatures() { return {"SEGA"}; }

	GenesisConsole(Emulator* emu);
	virtual ~GenesisConsole();

	Emulator* GetEmulator() { return _emu; }
	GenesisM68k* GetCpu() { return _cpu.get(); }
	GenesisVdp* GetVdp() { return _vdp.get(); }
	GenesisMemoryManager* GetMemoryManager() { return _memoryManager.get(); }

	LoadRomResult LoadRom(VirtualFile& romFile) override;

	void Reset() override;
	void RunFrame() override;
	void ProcessEndOfFrame();

	void SaveBattery() override;

	BaseControlManager* GetControlManager() override;
	ConsoleRegion GetRegion() override;
	ConsoleType GetConsoleType() override;
	vector<CpuType> GetCpuTypes() override;
	RomFormat GetRomFormat() override;
	double GetFps() override;
	PpuFrameInfo GetPpuFrame() override;
	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;

	uint64_t GetMasterClock() override;
	uint32_t GetMasterClockRate() override;

	AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;

	AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	AddressInfo GetPcAbsoluteAddress() override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;

	GenesisState GetState();
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;

	void InitializeRam(void* data, uint32_t length);

	void Serialize(Serializer& s) override;
};
