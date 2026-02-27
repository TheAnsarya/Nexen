#pragma once
#include "pch.h"
#include "GBA/GbaTypes.h"
#include "Debugger/DebugTypes.h"
#include "Shared/SettingTypes.h"
#include "Shared/Interfaces/IConsole.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GbaCpu;
class GbaPpu;
class GbaApu;
class GbaCart;
class GbaMemoryManager;
class GbaControlManager;
class GbaDmaController;
class GbaTimer;
class GbaRomPrefetch;
class GbaSerial;
class VirtualFile;
class BaseControlManager;

/// <summary>
/// Game Boy Advance console emulator.
/// Implements the complete GBA hardware: ARM7TDMI CPU, PPU, APU, and subsystems.
/// </summary>
/// <remarks>
/// **Hardware Specifications:**
/// - **CPU**: ARM7TDMI @ 16.78 MHz (ARM/Thumb instruction sets)
/// - **Display**: 240×160 pixels, 15-bit color (32768 colors)
/// - **Memory**: 32KB internal + 256KB external WRAM, 96KB VRAM
/// - **Audio**: 4 Game Boy channels + 2 Direct Sound (PCM) channels
///
/// **Display Features:**
/// - 4 background layers (modes 0-5)
/// - 128 sprites (64×64 max, 128 per scanline)
/// - Affine transformation (rotation/scaling) for BG2/BG3 and sprites
/// - Alpha blending and brightness control
///
/// **Memory Map:**
/// - $00000000-$00003FFF: BIOS (16KB)
/// - $02000000-$0203FFFF: External WRAM (256KB)
/// - $03000000-$03007FFF: Internal WRAM (32KB)
/// - $04000000-$040003FF: I/O Registers
/// - $05000000-$050003FF: Palette RAM (1KB)
/// - $06000000-$06017FFF: VRAM (96KB)
/// - $07000000-$070003FF: OAM (1KB)
/// - $08000000-$09FFFFFF: ROM (32MB max)
///
/// **DMA Features:**
/// - 4 DMA channels with different priorities
/// - Sound DMA (channels 1-2)
/// - Video capture DMA (channel 3)
/// </remarks>
class GbaConsole final : public IConsole {
public:
	/// <summary>BIOS ROM size (16KB).</summary>
	static constexpr int BootRomSize = 0x4000;

	/// <summary>Video RAM size (96KB).</summary>
	static constexpr int VideoRamSize = 0x18000;

	/// <summary>Sprite/OAM RAM size (1KB).</summary>
	static constexpr int SpriteRamSize = 0x400;

	/// <summary>Palette RAM size (1KB).</summary>
	static constexpr int PaletteRamSize = 0x400;

	/// <summary>Internal work RAM size (32KB).</summary>
	static constexpr int IntWorkRamSize = 0x8000;

	/// <summary>External work RAM size (256KB).</summary>
	static constexpr int ExtWorkRamSize = 0x40000;

private:
	Emulator* _emu = nullptr;

	unique_ptr<GbaCpu> _cpu;
	unique_ptr<GbaPpu> _ppu;
	unique_ptr<GbaApu> _apu;
	unique_ptr<GbaDmaController> _dmaController;
	unique_ptr<GbaTimer> _timer;
	unique_ptr<GbaMemoryManager> _memoryManager;
	unique_ptr<GbaControlManager> _controlManager;
	unique_ptr<GbaCart> _cart;
	unique_ptr<GbaSerial> _serial;
	unique_ptr<GbaRomPrefetch> _prefetch;

	GbaSaveType _saveType = GbaSaveType::AutoDetect;
	GbaRtcType _rtcType = GbaRtcType::AutoDetect;
	GbaCartridgeType _cartType = GbaCartridgeType::Default;

	uint8_t* _prgRom = nullptr;
	uint32_t _prgRomSize = 0;

	uint8_t* _saveRam = nullptr;
	uint32_t _saveRamSize = 0;

	uint8_t* _intWorkRam = nullptr;
	uint8_t* _extWorkRam = nullptr;

	uint16_t* _videoRam = nullptr;
	uint32_t* _spriteRam = nullptr;
	uint16_t* _paletteRam = nullptr;

	uint8_t* _bootRom = nullptr;

	void InitSaveRam(string& gameCode, vector<uint8_t>& romData);
	void InitCart(VirtualFile& romFile, vector<uint8_t>& romData);

public:
	GbaConsole(Emulator* emu);
	~GbaConsole();

	static vector<string> GetSupportedExtensions() { return {".gba"}; }
	static vector<string> GetSupportedSignatures() { return {}; }

	void LoadBattery();
	void SaveBattery() override;

	Emulator* GetEmulator();

	GbaCpu* GetCpu();
	GbaPpu* GetPpu();
	GbaApu* GetApu();
	GbaDmaController* GetDmaController();
	GbaState GetState();
	void GetConsoleState(BaseState& state, ConsoleType consoleType) override;

	GbaMemoryManager* GetMemoryManager();

	void ProcessEndOfFrame();

	void Serialize(Serializer& s) override;

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

	void ClearCpuSequentialFlag();
	void SetCpuSequentialFlag();
	void SetCpuStopFlag();

	void RefreshRamCheats();

	void InitializeRam(void* data, uint32_t length);
};