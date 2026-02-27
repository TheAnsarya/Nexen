#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"
#include "Lynx/LynxTypes.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxCart.h"
#include "Lynx/LynxControlManager.h"
#include "Lynx/LynxApu.h"
#include "Lynx/LynxEeprom.h"
#include "Lynx/LynxDefaultVideoFilter.h"

class Emulator;

/// <summary>
/// Atari Lynx portable console emulator.
///
/// Hardware overview:
/// - CPU: WDC 65C02 @ 4 MHz (16 MHz master clock / 4)
/// - Video: Mikey custom chip — 160×102 @ 4bpp, 4096-color palette
/// - Audio: Mikey — 4 channels, 8-bit DAC, LFSR-based
/// - Custom: Suzy — sprite engine, math coprocessor, collision detection
/// - RAM: 64 KB, Boot ROM: 512 bytes
/// - Cart: LNX format with bank switching
/// </summary>
class LynxConsole final : public IConsole {
private:
	Emulator* _emu = nullptr;
	unique_ptr<LynxCpu> _cpu;
	unique_ptr<LynxMikey> _mikey;
	unique_ptr<LynxSuzy> _suzy;
	unique_ptr<LynxMemoryManager> _memoryManager;
	unique_ptr<LynxCart> _cart;
	unique_ptr<LynxControlManager> _controlManager;
	unique_ptr<LynxApu> _apu;
	unique_ptr<LynxEeprom> _eeprom;

	std::unique_ptr<uint8_t[]> _workRam;
	uint32_t _workRamSize = 0;
	std::unique_ptr<uint8_t[]> _prgRom;
	uint32_t _prgRomSize = 0;
	std::unique_ptr<uint8_t[]> _bootRom;
	uint32_t _bootRomSize = 0;
	std::unique_ptr<uint8_t[]> _saveRam;
	uint32_t _saveRamSize = 0;

	LynxModel _model = LynxModel::LynxII;
	LynxRotation _rotation = LynxRotation::None;
	uint32_t _frameCount = 0;

	uint32_t _frameBuffer[LynxConstants::PixelCount] = {};

public:
	LynxConsole(Emulator* emu);
	~LynxConsole();

	[[nodiscard]] static vector<string> GetSupportedExtensions() { return { ".lnx", ".o" }; }
	[[nodiscard]] static vector<string> GetSupportedSignatures() { return { "LYNX" }; }

	// IConsole overrides
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
	void Serialize(Serializer& s) override;

	// Lynx-specific
	[[nodiscard]] LynxState GetState();
	[[nodiscard]] LynxModel GetModel() { return _model; }
	[[nodiscard]] LynxRotation GetRotation() { return _rotation; }
	[[nodiscard]] uint32_t GetFrameCount() const { return _frameCount; }
	void LoadBattery();
	void ApplyHleBootState();

	// Component accessors
	[[nodiscard]] LynxCpu* GetCpu() { return _cpu.get(); }
	[[nodiscard]] LynxMikey* GetMikey() { return _mikey.get(); }
	[[nodiscard]] LynxSuzy* GetSuzy() { return _suzy.get(); }
	[[nodiscard]] LynxMemoryManager* GetMemoryManager() { return _memoryManager.get(); }
	[[nodiscard]] LynxCart* GetCart() { return _cart.get(); }
	[[nodiscard]] LynxApu* GetApu() { return _apu.get(); }
	[[nodiscard]] LynxEeprom* GetEeprom() { return _eeprom.get(); }

	// Memory accessors for components
	[[nodiscard]] uint8_t* GetWorkRam() { return _workRam.get(); }
	[[nodiscard]] uint32_t GetWorkRamSize() { return _workRamSize; }
	[[nodiscard]] uint8_t* GetPrgRom() { return _prgRom.get(); }
	[[nodiscard]] uint32_t GetPrgRomSize() { return _prgRomSize; }
	[[nodiscard]] uint32_t* GetFrameBuffer() { return _frameBuffer; }
};
