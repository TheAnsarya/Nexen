#pragma once

#include "pch.h"
#include "Utilities/ISerializable.h"
#include "NES/NesTypes.h"
#include "NES/BaseNesPpu.h"
#include "NES/BaseMapper.h"
#include "NES/NesTypes.h"
#include "NES/INesMemoryHandler.h"
#include "Shared/MemoryOperationType.h"

enum class ConsoleRegion;

class Emulator;
class SnesControlManager;
class NesConsole;
class EmuSettings;

/// <summary>PPU register addresses (memory-mapped I/O)</summary>
enum PpuRegisters {
	Control = 0x00,       ///< $2000 - PPUCTRL: NMI enable, sprite size, BG/sprite pattern tables
	Mask = 0x01,          ///< $2001 - PPUMASK: Color emphasis, sprite/BG enable, clipping
	Status = 0x02,        ///< $2002 - PPUSTATUS: V-blank, sprite 0 hit, overflow (read clears V-blank)
	SpriteAddr = 0x03,    ///< $2003 - OAMADDR: OAM address for $2004 access
	SpriteData = 0x04,    ///< $2004 - OAMDATA: OAM read/write data
	ScrollOffsets = 0x05, ///< $2005 - PPUSCROLL: Fine X/Y scroll (write twice)
	VideoMemoryAddr = 0x06, ///< $2006 - PPUADDR: VRAM address (write twice, high then low)
	VideoMemoryData = 0x07, ///< $2007 - PPUDATA: VRAM read/write data
	SpriteDMA = 0x4014,   ///< $4014 - OAMDMA: Sprite DMA source page (triggers 256-byte copy)
};

/// <summary>
/// NES PPU (Picture Processing Unit) emulator - Ricoh 2C02/2C07 implementation.
/// Cycle-accurate rendering with proper timing for raster effects.
/// </summary>
/// <remarks>
/// The PPU renders 256x240 pixels at 60Hz (NTSC) or 50Hz (PAL):
/// - 262 scanlines (NTSC) / 312 scanlines (PAL)
/// - 341 PPU cycles per scanline
/// - 1 CPU cycle = 3 PPU cycles (NTSC) / ~3.2 PPU cycles (PAL)
///
/// **Display Composition:**
/// - Background: 32x30 tiles (8x8 pixels each) with scrolling
/// - Sprites: 64 sprites in OAM, 8 per scanline limit
/// - 2 pattern tables (256 tiles each) for BG and sprites
/// - 4 nametables (only 2KB VRAM, mirroring configured by mapper)
///
/// **Rendering Pipeline:**
/// - Scanlines 0-239: Visible (fetch tiles, evaluate sprites, render)
/// - Scanline 240: Post-render (idle)
/// - Scanlines 241-260: V-blank (NMI triggered at start of 241)
/// - Scanline 261: Pre-render (setup for next frame)
///
/// **Key Timing Events:**
/// - Sprite 0 hit: When BG and sprite 0 opaque pixels overlap
/// - NMI: Start of V-blank if PPUCTRL.7 enabled
/// - Sprite overflow: More than 8 sprites on a scanline (buggy)
///
/// **OAM (Object Attribute Memory):**
/// - 256 bytes: 64 sprites × 4 bytes (Y, tile, attributes, X)
/// - Secondary OAM: 32 bytes for 8 sprites on current scanline
/// - OAM decay: Values decay without refresh (not always emulated)
///
/// **Template Parameter:**
/// - T: Derived class for CRTP (Curiously Recurring Template Pattern)
///   - Enables static polymorphism for console-specific variants
///   - Used for VS System dual-PPU configurations
/// </remarks>
template <class T>
class NesPpu : public BaseNesPpu {
private:
	static constexpr int32_t OamDecayCycleCount = 3000;  ///< Cycles before OAM values decay

protected:
	void UpdateStatusFlag();

	void SetControlRegister(uint8_t value);
	void SetMaskRegister(uint8_t value);

	/// Handle scroll register glitch when writing during rendering
	void ProcessTmpAddrScrollGlitch(uint16_t normalAddr, uint16_t value, uint16_t mask);

	// Open bus behavior (PPU data bus retains values briefly)
	void SetOpenBus(uint8_t mask, uint8_t value);
	uint8_t ApplyOpenBus(uint8_t mask, uint8_t value);

	void ProcessStatusRegOpenBus(uint8_t& openBusMask, uint8_t& returnValue);

	// VRAM address manipulation (scrolling implementation)
	__forceinline void UpdateVideoRamAddr();
	__forceinline void IncVerticalScrolling();   ///< Increment fine/coarse Y
	__forceinline void IncHorizontalScrolling(); ///< Increment coarse X
	__forceinline uint16_t GetNameTableAddr();   ///< Current nametable address
	__forceinline uint16_t GetAttributeAddr();   ///< Current attribute table address

	// Scanline processing
	void ProcessScanlineFirstCycle();
	__forceinline void ProcessScanlineImpl();
	__forceinline void ProcessSpriteEvaluation();      ///< Find sprites for next scanline
	__noinline void ProcessSpriteEvaluationStart();
	__noinline void ProcessSpriteEvaluationEnd();

	// V-blank handling
	void BeginVBlank();  ///< Start V-blank period
	void TriggerNmi();   ///< Generate NMI to CPU

	// Tile and sprite loading
	__forceinline void LoadTileInfo();
	void LoadSprite(uint8_t spriteY, uint8_t tileIndex, uint8_t attributes, uint8_t spriteX, bool extraSprite);
	void LoadSpriteTileInfo();
	void LoadExtraSprites();  ///< Load sprites beyond 8-per-line limit (optional accuracy)
	__forceinline void ShiftTileRegisters();

	// OAM (sprite memory) access
	__forceinline uint8_t ReadSpriteRam(uint8_t addr);
	__forceinline void WriteSpriteRam(uint8_t addr, uint8_t value);

	// OAM corruption emulation (hardware bug)
	void SetOamCorruptionFlags();
	void ProcessOamCorruption();

	__forceinline uint8_t GetPixelColor();  ///< Compute final pixel from BG and sprites

	void SendFrame();                    ///< Complete frame to video output
	void SendFrameVsDualSystem();        ///< VS System dual-screen frame

	void UpdateState();
	void UpdateApuStatus();

	/// Convert address to register enum
	PpuRegisters GetRegisterID(uint16_t addr) {
		if (addr == 0x4014) {
			return PpuRegisters::SpriteDMA;
		} else {
			return (PpuRegisters)(addr & 0x07);
		}
	}

	// VRAM bus operations
	__forceinline void SetBusAddress(uint16_t addr);
	__forceinline uint8_t ReadVram(uint16_t addr, MemoryOperationType type = MemoryOperationType::PpuRenderingRead);
	__forceinline void WriteVram(uint16_t addr, uint8_t value);

	void Serialize(Serializer& s) override;

public:
	NesPpu(NesConsole* console);
	virtual ~NesPpu();

	void Reset(bool softReset) override;

	uint16_t* GetScreenBuffer(bool previousBuffer, bool processGrayscaleEmphasisBits = false) override;
	void DebugCopyOutputBuffer(uint16_t* target);
	void DebugUpdateFrameBuffer(bool toGrayscale);

	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.AddHandler(MemoryOperation::Read, 0x2000, 0x3FFF);
		ranges.AddHandler(MemoryOperation::Write, 0x2000, 0x3FFF);
		ranges.AddHandler(MemoryOperation::Write, 0x4014);
	}

	PpuModel GetPpuModel() override;

	uint8_t ReadRam(uint16_t addr) override;
	uint8_t PeekRam(uint16_t addr) override;
	void WriteRam(uint16_t addr, uint8_t value) override;

	void UpdateTimings(ConsoleRegion region, bool overclockAllowed = true) override;

	__forceinline void Exec();
	void Run(uint64_t runTo) override;

	uint32_t GetPixelBrightness(uint8_t x, uint8_t y) override;

	uint16_t GetPixel(uint8_t x, uint8_t y) {
		return _currentOutputBuffer[y << 8 | x];
	}
};

template <class T>
void NesPpu<T>::Run(uint64_t runTo) {
	do {
		// Always need to run at least once, check condition at the end of the loop (slightly faster)
		Exec();
		_masterClock += _masterClockDivider;
	} while (_masterClock + _masterClockDivider <= runTo);
}

template <class T>
NesPpu<T>::~NesPpu() = default;
