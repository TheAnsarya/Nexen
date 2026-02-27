#pragma once
#include "pch.h"
#include "Shared/Interfaces/IConsole.h"
#include "WS/WsTypes.h"

class Emulator;
class WsCpu;
class WsPpu;
class WsApu;
class WsCart;
class WsTimer;
class WsSerial;
class WsMemoryManager;
class WsControlManager;
class WsDmaController;
class WsEeprom;
enum class WsModel : uint8_t;

/// <summary>
/// WonderSwan / WonderSwan Color console emulator.
/// Implements Bandai's handheld console with portrait/landscape display rotation.
/// </summary>
/// <remarks>
/// **System Variants:**
/// - **WonderSwan**: Original monochrome (8 shades)
/// - **WonderSwan Color**: 241 colors on screen from 4096 palette
/// - **SwanCrystal**: Improved WonderSwan Color with TFT display
///
/// **Hardware Specifications:**
/// - **CPU**: NEC V30MZ (80186 compatible) @ 3.072 MHz
/// - **Display**: 224×144 pixels (rotatable)
/// - **Memory**: 16KB internal RAM (64KB on Color)
/// - **Storage**: EEPROM for saves (internal + cartridge)
///
/// **Video Features:**
/// - 4 background layers with scrolling
/// - 128 sprites
/// - Wavetable-based sound with 4 channels
/// - Portrait/landscape screen rotation
///
/// **Audio:**
/// - 4 wavetable channels (32 samples each)
/// - Channel 2: Optional PCM voice mode
/// - Channel 3: Optional frequency sweep
/// - Channel 4: Optional noise mode
/// - HyperVoice: DMA-driven 8-bit PCM (Color only)
///
/// **Unique Features:**
/// - Orientation sensor for automatic screen rotation
/// - Low power consumption design
/// - Cartridge EEPROM for saves
/// </remarks>
class WsConsole final : public IConsole {
private:
	Emulator* _emu = nullptr;
	unique_ptr<WsCpu> _cpu;
	unique_ptr<WsPpu> _ppu;
	unique_ptr<WsApu> _apu;
	unique_ptr<WsCart> _cart;
	unique_ptr<WsTimer> _timer;
	unique_ptr<WsSerial> _serial;
	unique_ptr<WsMemoryManager> _memoryManager;
	unique_ptr<WsControlManager> _controlManager;
	unique_ptr<WsDmaController> _dmaController;
	unique_ptr<WsEeprom> _internalEeprom;
	unique_ptr<WsEeprom> _cartEeprom;

	uint8_t* _workRam = nullptr;
	uint32_t _workRamSize = 0;

	uint8_t* _saveRam = nullptr;
	uint32_t _saveRamSize = 0;

	uint8_t* _bootRom = nullptr;
	uint32_t _bootRomSize = 0;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	uint8_t* _internalEepromData = nullptr;
	uint32_t _internalEepromSize = 0;

	uint8_t* _cartEepromData = nullptr;
	uint32_t _cartEepromSize = 0;

	WsModel _model = {};
	bool _verticalMode = false;

	void InitPostBootRomState();

public:
	WsConsole(Emulator* emu);
	~WsConsole();

	[[nodiscard]] static vector<string> GetSupportedExtensions() { return {".ws", ".wsc"}; }
	[[nodiscard]] static vector<string> GetSupportedSignatures() { return {}; }

	LoadRomResult LoadRom(VirtualFile& romFile) override;
	void RunFrame() override;

	void GetScreenRotationOverride(uint32_t& rotation) override;
	[[nodiscard]] bool IsColorMode();
	[[nodiscard]] bool IsPowerOff();
	[[nodiscard]] bool IsVerticalMode();
	[[nodiscard]] WsModel GetModel();

	void ProcessEndOfFrame();

	void Reset() override;
	void LoadBattery();
	void SaveBattery() override;

	[[nodiscard]] BaseControlManager* GetControlManager() override;
	[[nodiscard]] ConsoleRegion GetRegion() override;
	[[nodiscard]] ConsoleType GetConsoleType() override;
	[[nodiscard]] vector<CpuType> GetCpuTypes() override;
	[[nodiscard]] uint64_t GetMasterClock() override;
	[[nodiscard]] uint32_t GetMasterClockRate() override;
	[[nodiscard]] double GetFps() override;
	[[nodiscard]] BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;
	[[nodiscard]] PpuFrameInfo GetPpuFrame() override;
	[[nodiscard]] RomFormat GetRomFormat() override;
	[[nodiscard]] AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;
	[[nodiscard]] AddressInfo GetAbsoluteAddress(uint32_t relAddr);
	[[nodiscard]] AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	[[nodiscard]] AddressInfo GetPcAbsoluteAddress() override;
	[[nodiscard]] AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;

	[[nodiscard]] WsState GetState();
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;

	[[nodiscard]] WsCpu* GetCpu() { return _cpu.get(); }
	[[nodiscard]] WsPpu* GetPpu() { return _ppu.get(); }
	[[nodiscard]] WsApu* GetApu() { return _apu.get(); }
	[[nodiscard]] WsMemoryManager* GetMemoryManager() { return _memoryManager.get(); }

	void Serialize(Serializer& s) override;
};