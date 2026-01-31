#pragma once
#include "pch.h"
#include <memory>
#include <array>
#include "Gameboy/GbTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class Gameboy;
class GbMemoryManager;
class GbDmaController;

/// <summary>
/// Game Boy PPU (Picture Processing Unit) emulator.
/// Renders 160x144 display at ~59.7 Hz with tile-based graphics.
/// </summary>
/// <remarks>
/// The Game Boy PPU provides:
/// - 1 background layer (256x256 tilemap, scrollable)
/// - 1 window layer (fixed position overlay)
/// - 40 sprites (8x8 or 8x16 pixels)
/// - 4 shades of gray (DMG) or 32768 colors (CGB)
///
/// **Memory:**
/// - 8KB VRAM (DMG) / 16KB VRAM in 2 banks (CGB)
/// - 160 bytes OAM (40 sprites × 4 bytes)
///
/// **Timing (per scanline = 456 dots):**
/// - Mode 2 (OAM scan): ~80 dots
/// - Mode 3 (Drawing): ~172-289 dots (variable)
/// - Mode 0 (HBlank): Remainder of 456 dots
/// - Mode 1 (VBlank): Scanlines 144-153
///
/// **CGB Enhancements:**
/// - 8 background palettes × 4 colors
/// - 8 sprite palettes × 4 colors
/// - VRAM banking for tiles/tilemaps
/// - Per-tile attributes (palette, flip, priority, bank)
/// - HDMA for efficient VRAM updates
/// </remarks>
class GbPpu : public ISerializable {
private:
	/// <summary>Emulator instance for frame output and debugging.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Gameboy instance for system info (CGB mode, etc.).</summary>
	Gameboy* _gameboy = nullptr;

	/// <summary>PPU state (registers, scanline, mode, etc.).</summary>
	GbPpuState _state = {};

	/// <summary>Memory manager for bus access.</summary>
	GbMemoryManager* _memoryManager = nullptr;

	/// <summary>DMA controller for OAM DMA timing.</summary>
	GbDmaController* _dmaController = nullptr;

	/// <summary>Double-buffered output (160×144 × 16bpp).</summary>
	std::array<std::unique_ptr<uint16_t[]>, 2> _outputBuffers;

	/// <summary>Current frame buffer being rendered to.</summary>
	uint16_t* _currentBuffer = nullptr;

	/// <summary>Double-buffered event viewer output (456×154).</summary>
	std::array<std::unique_ptr<uint16_t[]>, 2> _eventViewerBuffers;

	/// <summary>Current event viewer buffer.</summary>
	uint16_t* _currentEventViewerBuffer = nullptr;

	/// <summary>Current event viewer color type.</summary>
	EvtColor _evtColor = EvtColor::HBlank;

	/// <summary>Previous cycle's drawn pixel count.</summary>
	int16_t _prevDrawnPixels = 0;

	/// <summary>Video RAM pointer (8KB DMG, 16KB CGB).</summary>
	uint8_t* _vram = nullptr;

	/// <summary>Object Attribute Memory pointer (160 bytes).</summary>
	uint8_t* _oam = nullptr;

	/// <summary>Timestamp of last completed frame.</summary>
	uint64_t _lastFrameTime = 0;

	/// <summary>Background pixel FIFO for mixing.</summary>
	GbPpuFifo _bgFifo;

	/// <summary>Background tile fetcher state machine.</summary>
	GbPpuFetcher _bgFetcher;

	/// <summary>Sprite pixel FIFO for mixing.</summary>
	GbPpuFifo _oamFifo;

	/// <summary>Sprite tile fetcher state machine.</summary>
	GbPpuFetcher _oamFetcher;

	/// <summary>Number of pixels drawn this scanline.</summary>
	int16_t _drawnPixels = 0;

	/// <summary>Current tile column being fetched.</summary>
	uint8_t _fetchColumn = 0;

	/// <summary>True if currently fetching window layer.</summary>
	bool _fetchWindow = false;

	/// <summary>Window internal line counter.</summary>
	int16_t _windowCounter = -1;

	/// <summary>Window Y trigger flag (WY matched LY).</summary>
	bool _wyEnableFlag = false;

	/// <summary>Window X trigger flag (WX reached).</summary>
	bool _wxEnableFlag = false;

	/// <summary>Insert glitch pixel for SCX fine scroll.</summary>
	bool _insertGlitchBgPixel = false;

	/// <summary>Index of sprite being fetched (-1 if none).</summary>
	int16_t _fetchSprite = -1;

	/// <summary>Number of sprites on current scanline.</summary>
	uint8_t _spriteCount = 0;

	/// <summary>X positions of sprites on current scanline.</summary>
	uint8_t _spriteX[10] = {};

	/// <summary>Y positions of sprites on current scanline.</summary>
	uint8_t _spriteY[10] = {};

	/// <summary>OAM indices of sprites on current scanline.</summary>
	uint8_t _spriteIndexes[10] = {};

	/// <summary>OAM read buffer for sprite evaluation.</summary>
	uint8_t _oamReadBuffer[2] = {};

	/// <summary>LCD disabled state flag.</summary>
	bool _lcdDisabled = true;

	/// <summary>OAM blocked during STOP mode.</summary>
	bool _stopOamBlocked = false;

	/// <summary>VRAM blocked during STOP mode.</summary>
	bool _stopVramBlocked = false;

	/// <summary>Palette blocked during STOP mode.</summary>
	bool _stopPaletteBlocked = false;

	/// <summary>OAM reads blocked (Mode 2/3).</summary>
	bool _oamReadBlocked = false;

	/// <summary>OAM writes blocked (Mode 2/3).</summary>
	bool _oamWriteBlocked = false;

	/// <summary>VRAM reads blocked (Mode 3).</summary>
	bool _vramReadBlocked = false;
	bool _vramWriteBlocked = false;

	bool _isFirstFrame = true;
	bool _forceBlankFrame = true;
	bool _rendererIdle = false;

	uint8_t _tileIndex = 0;
	uint8_t _gbcTileGlitch = 0;

	GbPixelType _lastPixelType = {};
	uint8_t _lastBgColor = 0;

	__forceinline void WriteBgPixel(uint8_t colorIndex);
	__forceinline void WriteObjPixel(uint8_t colorIndex);

	__forceinline void ProcessPpuCycle();

	__forceinline void ExecCycle();
	__forceinline void ProcessVblankScanline();
	void ProcessFirstScanlineAfterPowerOn();
	__forceinline void ProcessVisibleScanline();
	__forceinline void RunDrawCycle();
	__forceinline void RunSpriteEvaluation();
	void ResetRenderer();
	void ClockSpriteFetcher();
	void FindNextSprite();
	__forceinline void ClockTileFetcher();
	__forceinline void PushSpriteToPixelFifo();
	__forceinline void PushTileToPixelFifo();

	void UpdateStatIrq();

	__forceinline uint8_t LcdReadOam(uint8_t addr);
	__forceinline uint8_t LcdReadVram(uint16_t addr);
	__forceinline uint16_t LcdReadBgPalette(uint8_t addr);
	__forceinline uint16_t LcdReadObjPalette(uint8_t addr);

	void SendFrame();
	void UpdatePalette();

	void SetMode(PpuMode mode);

	uint8_t ReadCgbPalette(uint8_t& pos, uint16_t* pal);
	void WriteCgbPalette(uint8_t& pos, uint16_t* pal, bool autoInc, uint8_t value);

public:
	virtual ~GbPpu();

	void Init(Emulator* emu, Gameboy* gameboy, GbMemoryManager* memoryManager, GbDmaController* dmaController, uint8_t* vram, uint8_t* oam);

	GbPpuState GetState();
	GbPpuState& GetStateRef();
	uint16_t* GetOutputBuffer();
	uint16_t* GetEventViewerBuffer();
	uint16_t* GetPreviousEventViewerBuffer();

	void SetCpuStopState(bool stopped);

	uint32_t GetFrameCount();
	uint8_t GetScanline();
	uint16_t GetCycle();
	bool IsLcdEnabled();
	bool IsCgbEnabled();
	PpuMode GetMode();

	template <bool singleStep>
	void Exec();

	uint8_t Read(uint16_t addr);
	void Write(uint16_t addr, uint8_t value);

	void SetTileFetchGlitchState();

	bool IsVramReadAllowed();
	bool IsVramWriteAllowed();
	uint8_t ReadVram(uint16_t addr);
	uint8_t PeekVram(uint16_t addr);
	void WriteVram(uint16_t addr, uint8_t value);

	bool IsOamReadAllowed();
	bool IsOamWriteAllowed();
	uint8_t ReadOam(uint8_t addr);
	uint8_t PeekOam(uint8_t addr);
	void WriteOam(uint8_t addr, uint8_t value, bool forDma);

	template <GbOamCorruptionType oamCorruptionType>
	void ProcessOamCorruption(uint16_t addr);

	void ProcessOamIncDecCorruption(int row);

	uint8_t ReadCgbRegister(uint16_t addr);
	void WriteCgbRegister(uint16_t addr, uint8_t value);

	void DebugSendFrame();

	void Serialize(Serializer& s) override;
};
