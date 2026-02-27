#pragma once
#include "pch.h"
#include "Gameboy/GameboyHeader.h"
#include "Gameboy/GbTypes.h"
#include "Debugger/DebugTypes.h"
#include "Shared/SettingTypes.h"
#include "Shared/Interfaces/IConsole.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GbPpu;
class GbApu;
class GbCpu;
class GbCart;
class GbTimer;
class GbMemoryManager;
class GbDmaController;
class GbControlManager;
class SuperGameboy;
class VirtualFile;
class BaseControlManager;

/// <summary>
/// Game Boy / Game Boy Color console emulator.
/// Implements the complete GB/GBC hardware including Super Game Boy support.
/// </summary>
/// <remarks>
/// **System Variants:**
/// - **Game Boy (DMG)**: Original monochrome handheld
/// - **Game Boy Pocket (MGB)**: Lighter, improved screen
/// - **Super Game Boy (SGB/SGB2)**: SNES adapter with borders and palettes
/// - **Game Boy Color (CGB)**: Color display, faster CPU mode
///
/// **Hardware Specifications:**
/// - **CPU**: Sharp LR35902 (Z80/8080 hybrid)
///   - Normal: 4.194304 MHz
///   - CGB Double Speed: 8.388608 MHz
/// - **Display**: 160×144 pixels
///   - DMG: 4 shades of green
///   - CGB: 32768 colors (56 on screen)
/// - **Memory**: 8KB WRAM (DMG) / 32KB WRAM (CGB)
/// - **VRAM**: 8KB (DMG) / 16KB banked (CGB)
///
/// **Video Features:**
/// - 1 background layer (256×256 tilemap)
/// - 1 window layer (overlay)
/// - 40 sprites (10 per scanline limit)
/// - CGB: Tile attributes, VRAM banking
///
/// **Audio:**
/// - 4 sound channels
///   - 2 square wave (channel 1 has sweep)
///   - 1 programmable wave
///   - 1 noise (LFSR)
/// - Stereo output with panning
///
/// **Mappers (MBC):**
/// - MBC1, MBC2, MBC3 (RTC), MBC5, MBC6, MBC7 (accelerometer)
/// - HuC1, HuC3, TAMA5, Pocket Camera, and more
///
/// **Super Game Boy Features:**
/// - Custom color palettes
/// - Border graphics
/// - SNES controller multiplayer
/// </remarks>
class Gameboy final : public IConsole {
private:
	/// <summary>OAM/Sprite RAM size (160 bytes = 40 sprites × 4 bytes).</summary>
	static constexpr int SpriteRamSize = 0xA0;

	/// <summary>High RAM (HRAM) size at $FF80-$FFFE (127 bytes).</summary>
	static constexpr int HighRamSize = 0x7F;

	Emulator* _emu = nullptr;
	SuperGameboy* _superGameboy = nullptr;
	bool _allowSgb = false;

	unique_ptr<GbMemoryManager> _memoryManager;
	unique_ptr<GbCpu> _cpu;
	unique_ptr<GbPpu> _ppu;
	unique_ptr<GbApu> _apu;
	unique_ptr<GbCart> _cart;
	unique_ptr<GbTimer> _timer;
	unique_ptr<GbDmaController> _dmaController;
	unique_ptr<GbControlManager> _controlManager;

	GameboyModel _model = GameboyModel::AutoFavorGbc;

	bool _hasBattery = false;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	uint8_t* _cartRam = nullptr;
	uint32_t _cartRamSize = 0;

	uint8_t* _workRam = nullptr;
	uint32_t _workRamSize = 0;

	uint8_t* _videoRam = nullptr;
	uint32_t _videoRamSize = 0;

	uint8_t* _spriteRam = nullptr;
	uint8_t* _highRam = nullptr;

	uint8_t* _bootRom = nullptr;
	uint32_t _bootRomSize = 0;

	void Init(GbCart* cart, std::vector<uint8_t>& romData, uint32_t cartRamSize, bool hasBattery);
	GameboyModel GetEffectiveModel(GameboyHeader& header);
	static GameboyHeader GetHeader(uint8_t* romData, uint32_t romSize);

public:
	static constexpr int HeaderOffset = 0x134;

	Gameboy(Emulator* emu, bool allowSgb = false);
	virtual ~Gameboy();

	static vector<string> GetSupportedExtensions() { return {".gb", ".gbc", ".gbx", ".gbs"}; }
	static vector<string> GetSupportedSignatures() { return {"GBS"}; }

	void PowerOn(SuperGameboy* sgb);

	void Run(uint64_t runUntilClock);

	void LoadBattery();
	void SaveBattery() override;

	Emulator* GetEmulator();

	GbPpu* GetPpu();
	GbCpu* GetCpu();
	GbTimer* GetTimer();
	void GetSoundSamples(int16_t*& samples, uint32_t& sampleCount);
	GbState GetState();
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;
	GameboyHeader GetHeader();

	uint32_t DebugGetMemorySize(MemoryType type);
	uint8_t* DebugGetMemory(MemoryType type);
	GbMemoryManager* GetMemoryManager();
	AddressInfo GetAbsoluteAddress(uint16_t addr);
	int32_t GetRelativeAddress(AddressInfo& absAddress);

	bool IsCpuStopped();
	bool IsCgb();
	bool IsSgb();
	SuperGameboy* GetSgb();

	uint64_t GetCycleCount();
	uint64_t GetApuCycleCount();

	void ProcessEndOfFrame();

	void RunApu();

	void Serialize(Serializer& s) override;
	SaveStateCompatInfo ValidateSaveStateCompatibility(ConsoleType stateConsoleType) override;

	// Inherited via IConsole
	void Reset() override;
	LoadRomResult LoadRom(VirtualFile& romFile) override;
	void RunFrame() override;
	BaseControlManager* GetControlManager() override;
	ConsoleRegion GetRegion() override;
	ConsoleType GetConsoleType() override;
	double GetFps() override;
	PpuFrameInfo GetPpuFrame() override;
	vector<CpuType> GetCpuTypes() override;

	AddressInfo GetAbsoluteAddress(AddressInfo& relAddress) override;
	AddressInfo GetPcAbsoluteAddress() override;
	AddressInfo GetRelativeAddress(AddressInfo& absAddress, CpuType cpuType) override;

	uint64_t GetMasterClock() override;
	uint32_t GetMasterClockRate() override;

	BaseVideoFilter* GetVideoFilter(bool getDefaultFilter) override;

	RomFormat GetRomFormat() override;
	AudioTrackInfo GetAudioTrackInfo() override;
	void ProcessAudioPlayerAction(AudioPlayerActionParams p) override;
	void InitGbsPlayback(uint8_t selectedTrack);

	void RefreshRamCheats();
	void InitializeRam(void* data, uint32_t length);
};
