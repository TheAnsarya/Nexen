#pragma once
#include "pch.h"
#include <array>
#include <memory>
#include "SNES/SnesPpuTypes.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Timer.h"

class Emulator;
class SnesConsole;
class InternalRegisters;
class SnesMemoryManager;
class Spc;
class EmuSettings;

/// <summary>
/// SNES PPU (Picture Processing Unit) emulator - S-PPU1/S-PPU2 implementation.
/// Renders 256x224/239 at up to 512x478 in hi-res/interlaced modes.
/// </summary>
/// <remarks>
/// The SNES PPU consists of two chips (PPU1/PPU2) providing:
/// - 4 background layers with various modes (Mode 0-7)
/// - 128 sprites (32x32 max, 32 per scanline)
/// - 256 colors from 32768 color palette
/// - Hardware effects: mosaic, color math, windowing
///
/// **Background Modes:**
/// - Mode 0: 4 layers, 4 colors each (16 total palettes)
/// - Mode 1: 2 layers 16-color + 1 layer 4-color
/// - Mode 2: 2 layers 16-color with offset-per-tile
/// - Mode 3: 1 layer 256-color + 1 layer 16-color
/// - Mode 4: 1 layer 256-color + 1 layer 4-color + OPT
/// - Mode 5: 1 layer 16-color hi-res (512 width)
/// - Mode 6: 1 layer 16-color hi-res + OPT
/// - Mode 7: 1 layer affine transform (rotation/scaling)
///
/// **Memory:**
/// - 64KB VRAM: Tilemaps, tile graphics
/// - 544 bytes OAM: Sprite attributes
/// - 512 bytes CGRAM: Color palette (256 colors × 2 bytes)
///
/// **Rendering Pipeline (per scanline):**
/// 1. Sprite evaluation: Find sprites on this line
/// 2. BG tile fetch: Load tilemap and pattern data
/// 3. Sprite tile fetch: Load sprite pattern data
/// 4. Pixel composition: Layer priority, windowing, color math
///
/// **Special Features:**
/// - Offset-per-tile: Individual tile scrolling (Modes 2, 4, 6)
/// - Mode 7: Matrix transformation for pseudo-3D (F-Zero, Mario Kart)
/// - Color math: Add/subtract colors between layers
/// - Windows: Rectangular clipping regions
/// </remarks>
class SnesPpu : public ISerializable {
public:
	constexpr static uint32_t SpriteRamSize = 544;   ///< OAM size in bytes
	constexpr static uint32_t CgRamSize = 512;       ///< Palette RAM size
	constexpr static uint32_t VideoRamSize = 0x10000; ///< 64KB VRAM

private:
	constexpr static int SpriteLayerIndex = 4;  ///< Layer index for sprites
	constexpr static int ColorWindowIndex = 5;  ///< Layer index for color window

	Emulator* _emu;
	SnesConsole* _console;
	InternalRegisters* _regs;
	SnesMemoryManager* _memoryManager;
	Spc* _spc;
	EmuSettings* _settings;

	// Background layer fetching state
	LayerData _layerData[4] = {};  ///< Per-layer rendering data
	uint16_t _hOffset = 0;          ///< Current horizontal offset
	uint16_t _vOffset = 0;          ///< Current vertical offset
	uint16_t _fetchBgStart = 0;     ///< BG fetch start cycle
	uint16_t _fetchBgEnd = 0;       ///< BG fetch end cycle

	// Sprite evaluation and fetching state
	SpriteInfo _currentSprite = {};   ///< Sprite being processed
	uint8_t _oamEvaluationIndex = 0;  ///< Current OAM index in evaluation
	uint8_t _oamTimeIndex = 0;        ///< Time-based OAM index (priority rotation)
	uint16_t _fetchSpriteStart = 0;
	uint16_t _fetchSpriteEnd = 0;
	uint16_t _spriteEvalStart = 0;
	uint16_t _spriteEvalEnd = 0;
	bool _spriteFetchingDone = false;
	uint8_t _spriteIndexes[32] = {};  ///< Sprites selected for this scanline
	uint8_t _spriteCount = 0;         ///< Number of sprites on scanline
	uint8_t _spriteTileCount = 0;     ///< Total sprite tiles on scanline
	bool _hasSpritePriority[4] = {};  ///< Sprites exist at each priority level

	// Timing state
	uint16_t _scanline = 0;
	uint32_t _frameCount = 0;

	uint16_t _vblankStartScanline;
	uint16_t _vblankEndScanline;
	uint16_t _baseVblankEndScanline;
	uint16_t _adjustedVblankEndScanline;
	uint16_t _nmiScanline;
	bool _overclockEnabled;
	bool _inOverclockedScanline = false;

	uint8_t _oddFrame = 0;  ///< Odd/even frame for interlacing

	SnesPpuState _state;  ///< PPU register state

	uint16_t _drawStartX = 0;
	uint16_t _drawEndX = 0;

	// Memory
	std::unique_ptr<uint16_t[]> _vram;               ///< 64KB Video RAM
	uint16_t _cgram[SnesPpu::CgRamSize >> 1] = {};   ///< Color palette (256 colors)
	uint8_t _oamRam[SnesPpu::SpriteRamSize] = {};    ///< Object Attribute Memory

	// Output buffers (double-buffered)
	std::array<std::unique_ptr<uint16_t[]>, 2> _outputBuffers;
	uint16_t* _currentBuffer = nullptr;
	bool _useHighResOutput = false;    ///< Hi-res mode active (512 width)
	bool _interlacedFrame = false;     ///< Interlaced mode active
	bool _overscanFrame = false;       ///< Overscan enabled (239 lines)

	// Scanline rendering buffers
	uint8_t _mainScreenFlags[256] = {};    ///< Main screen layer flags
	uint16_t _mainScreenBuffer[256] = {};  ///< Main screen pixel colors

	uint8_t _subScreenPriority[256] = {};  ///< Sub screen priorities
	uint16_t _subScreenBuffer[256] = {};   ///< Sub screen pixel colors

	// Mosaic effect state
	uint32_t _mosaicColor[4] = {};         ///< Mosaic color per layer
	uint32_t _mosaicPriority[4] = {};      ///< Mosaic priority per layer
	uint16_t _mosaicScanlineCounter = 0;

	/// <summary>
	/// Precomputed window mask arrays per layer (6 layers x 256 pixels).
	/// Computed once per RenderScanline() call for the current _drawStartX.._drawEndX range.
	/// Replaces per-pixel ProcessMaskWindow() switch evaluation with simple array lookups.
	/// </summary>
	bool _windowMask[6][256] = {};
	/// <summary>Number of active windows per layer (0, 1, or 2). Used to skip mask lookup entirely.</summary>
	uint8_t _mainWindowCount[6] = {};
	uint8_t _subWindowCount[6] = {};

	uint8_t _oamWriteBuffer = 0;  ///< OAM write latch

	bool _timeOver = false;   ///< Too many sprite tiles (>34 per scanline)
	bool _rangeOver = false;  ///< Too many sprites (>32 per scanline)

	// H/V scroll latching (for $2137/$213C/$213D reads)
	uint8_t _hvScrollLatchValue = 0;
	uint8_t _hScrollLatchValue = 0;

	uint16_t _horizontalLocation = 0;
	bool _horizontalLocToggle = false;
	uint16_t _verticalLocation = 0;
	bool _verticalLocationToggle = false;
	bool _locationLatched = false;
	bool _latchRequest = false;
	uint16_t _latchRequestX = 0;
	uint16_t _latchRequestY = 0;

	// Frame skip for performance
	Timer _frameSkipTimer;
	bool _skipRender = false;
	uint8_t _configVisibleLayers = 0xFF;  ///< Debug: visible layers mask

	// Sprite scanline buffers
	uint8_t _spritePriority[256] = {};
	uint8_t _spritePalette[256] = {};
	uint8_t _spriteColors[256] = {};
	uint8_t _spritePriorityCopy[256] = {};
	uint8_t _spritePaletteCopy[256] = {};
	uint8_t _spriteColorsCopy[256] = {};

	int32_t _debugMode7StartX = 0;
	int32_t _debugMode7StartY = 0;
	int32_t _debugMode7EndX = 0;
	int32_t _debugMode7EndY = 0;

	bool _needFullFrame = false;

	/// <summary>Renders all sprites for the current scanline.</summary>
	/// <param name="priorities">4-element array of priority levels to render.</param>
	void RenderSprites(const uint8_t priorities[4]);

	/// <summary>Fetches tilemap data for a background layer column.</summary>
	/// <typeparam name="hiResMode">True for 512-pixel wide modes (5/6).</typeparam>
	template <bool hiResMode>
	void GetTilemapData(uint8_t layerIndex, uint8_t columnIndex);

	/// <summary>Fetches character (pattern) data for a tile.</summary>
	/// <typeparam name="bpp">Bits per pixel (2, 4, or 8).</typeparam>
	template <bool hiResMode, uint8_t bpp, bool secondTile = false>
	void GetChrData(uint8_t layerIndex, uint8_t column, uint8_t plane);

	/// <summary>Fetches horizontal offset-per-tile data (Modes 2/4/6).</summary>
	void GetHorizontalOffsetByte(uint8_t columnIndex);
	/// <summary>Fetches vertical offset-per-tile data (Modes 2/4/6).</summary>
	void GetVerticalOffsetByte(uint8_t columnIndex);
	/// <summary>Main tile data fetch orchestrator for current scanline.</summary>
	void FetchTileData();

	// Background mode renderers - each implements specific layer configurations
	void RenderMode0();  ///< 4 layers × 4 colors (2bpp each)
	void RenderMode1();  ///< 2 layers × 16 colors + 1 layer × 4 colors
	void RenderMode2();  ///< 2 layers × 16 colors with offset-per-tile
	void RenderMode3();  ///< 1 layer × 256 colors + 1 layer × 16 colors
	void RenderMode4();  ///< 256-color + 4-color with offset-per-tile
	void RenderMode5();  ///< 16-color hi-res (512 width)
	void RenderMode6();  ///< 16-color hi-res with offset-per-tile
	void RenderMode7();  ///< Affine transformation (rotation/scaling)

	/// <summary>Renders the backdrop color for transparent pixels.</summary>
	void RenderBgColor();

	/// <summary>
	/// Core tilemap renderer - heavily templated for zero-overhead specialization.
	/// </summary>
	/// <typeparam name="layerIndex">Background layer (0-3).</typeparam>
	/// <typeparam name="bpp">Color depth: 2, 4, or 8 bits per pixel.</typeparam>
	/// <typeparam name="normalPriority">Priority value for low-priority tiles.</typeparam>
	/// <typeparam name="highPriority">Priority value for high-priority tiles.</typeparam>
	template <uint8_t layerIndex, uint8_t bpp, uint8_t normalPriority, uint8_t highPriority, uint16_t basePaletteOffset = 0>
	__forceinline void RenderTilemap();

	template <uint8_t layerIndex, uint8_t bpp, uint8_t normalPriority, uint8_t highPriority, uint16_t basePaletteOffset, bool hiResMode>
	__forceinline void RenderTilemap();

	template <uint8_t layerIndex, uint8_t bpp, uint8_t normalPriority, uint8_t highPriority, uint16_t basePaletteOffset, bool hiResMode, bool applyMosaic>
	__forceinline void RenderTilemap();

	template <uint8_t layerIndex, uint8_t bpp, uint8_t normalPriority, uint8_t highPriority, uint16_t basePaletteOffset, bool hiResMode, bool applyMosaic, bool directColorMode>
	void RenderTilemap();

	/// <summary>Converts palette + color index to RGB555 color value.</summary>
	template <uint8_t bpp, bool directColorMode, uint8_t basePaletteOffset>
	__forceinline uint16_t GetRgbColor(uint8_t paletteIndex, uint8_t colorIndex);

	/// <summary>Checks if layer rendering is needed (enabled and visible).</summary>
	[[nodiscard]] __forceinline bool IsRenderRequired(uint8_t layerIndex);

	/// <summary>Extracts pixel color from character data bitplanes.</summary>
	template <uint8_t bpp>
	__forceinline uint8_t GetTilePixelColor(const uint16_t chrData[4], const uint8_t shift);

	/// <summary>Mode 7 affine transform renderer with matrix multiplication.</summary>
	template <uint8_t layerIndex, uint8_t normalPriority, uint8_t highPriority>
	__forceinline void RenderTilemapMode7();

	template <uint8_t layerIndex, uint8_t normalPriority, uint8_t highPriority, bool applyMosaic>
	__forceinline void RenderTilemapMode7();

	template <uint8_t layerIndex, uint8_t normalPriority, uint8_t highPriority, bool applyMosaic, bool directColorMode>
	void RenderTilemapMode7();

	/// <summary>Draws a pixel to the main screen buffer with priority check.</summary>
	__forceinline void DrawMainPixel(uint8_t x, uint16_t color, uint8_t flags);
	/// <summary>Draws a pixel to the sub screen buffer (for color math).</summary>
	__forceinline void DrawSubPixel(uint8_t x, uint16_t color, uint8_t priority);

	/// <summary>
	/// Applies color math effects (add/subtract) between main and sub screens.
	/// Used for transparency, shadows, and special effects.
	/// </summary>
	void ApplyColorMath();

	template <bool subtractMode>
	__forceinline void ApplyColorMathToPixel(
		uint16_t& pixelA, uint16_t pixelB, int x, bool isInsideWindow,
		ColorWindowMode clipMode, ColorWindowMode preventMode,
		bool addSubscreen, uint16_t fixedColor, uint8_t baseHalfShift);

	/// <summary>Applies master brightness to all pixels ($2100 setting).</summary>
	template <bool forMainScreen>
	void ApplyBrightness();

	/// <summary>Converts standard 256-width output to 512-width hi-res.</summary>
	void ConvertToHiRes();
	/// <summary>Applies hi-res mode rendering with pseudo-transparency.</summary>
	void ApplyHiResMode();

	/// <summary>Processes window mask logic for a layer at position x.</summary>
	template <uint8_t layerIndex>
	bool ProcessMaskWindow(uint8_t activeWindowCount, int x);

	/// <summary>
	/// Precomputes window mask arrays for all 6 layers across the current draw range.
	/// Called once per RenderScanline() invocation, replacing per-pixel ProcessMaskWindow() calls.
	/// </summary>
	void PrecomputeWindowMasks();

	void ProcessWindowMaskSettings(uint8_t value, uint8_t offset);

	/// <summary>Updates VRAM read buffer after VRAM address changes.</summary>
	void UpdateVramReadBuffer();
	/// <summary>Gets current VRAM address with translation applied.</summary>
	uint16_t GetVramAddress();

	/// <summary>Sends completed frame to video output.</summary>
	void SendFrame();

	[[nodiscard]] bool IsDoubleHeight();  ///< True when interlaced (478 lines)
	[[nodiscard]] bool IsDoubleWidth();   ///< True when hi-res (512 pixels)

	[[nodiscard]] bool CanAccessCgram();  ///< True when CGRAM is accessible (during vblank/fblank)
	[[nodiscard]] bool CanAccessVram();   ///< True when VRAM is accessible

	/// <summary>Evaluates which sprites appear on the next scanline.</summary>
	void EvaluateNextLineSprites();
	/// <summary>Fetches sprite tile data for current scanline.</summary>
	void FetchSpriteData();
	__forceinline void FetchSpritePosition(uint8_t oamAddress);
	void FetchSpriteAttributes(uint16_t oamAddress);
	void FetchSpriteTile(bool secondCycle);

	void UpdateOamAddress();
	uint16_t GetOamAddress();

	void RandomizeState();  ///< Randomizes PPU state for testing

	__noinline void DebugProcessMode7Overlay();
	__noinline void DebugProcessMainSubScreenViews();

	__noinline void FillInterlacedFrame();

public:
	SnesPpu(Emulator* emu, SnesConsole* console);
	virtual ~SnesPpu();

	void PowerOn();
	void Reset();

	/// <summary>Renders one scanline of video output.</summary>
	void RenderScanline();

	[[nodiscard]] uint32_t GetFrameCount();
	[[nodiscard]] uint16_t GetRealScanline();      ///< Actual scanline including vblank
	[[nodiscard]] uint16_t GetVblankEndScanline(); ///< Last scanline of vblank period
	[[nodiscard]] uint16_t GetScanline();          ///< Current visible scanline
	[[nodiscard]] uint16_t GetCycle();             ///< Current H-counter position
	[[nodiscard]] uint16_t GetNmiScanline();       ///< Scanline that triggers NMI
	[[nodiscard]] uint16_t GetVblankStart();       ///< First scanline of vblank

	SnesPpuState GetState();
	SnesPpuState& GetStateRef();
	void GetState(SnesPpuState& state, bool returnPartialState);

	bool ProcessEndOfScanline(uint16_t& hClock);
	[[nodiscard]] bool IsInOverclockedScanline();
	void UpdateSpcState();
	void UpdateNmiScanline();
	[[nodiscard]] uint16_t GetLastScanline();

	[[nodiscard]] bool IsHighResOutput();
	[[nodiscard]] uint16_t* GetScreenBuffer();
	[[nodiscard]] uint16_t* GetPreviousScreenBuffer();
	[[nodiscard]] uint8_t* GetVideoRam();
	[[nodiscard]] uint8_t* GetCgRam();
	[[nodiscard]] uint8_t* GetSpriteRam();

	void DebugSendFrame();

	/// <summary>Requests H/V counter latch at specific coordinates.</summary>
	void SetLocationLatchRequest(uint16_t x, uint16_t y);
	void ProcessLocationLatchRequest();
	void LatchLocationValues();

	/// <summary>Handles PPU register reads ($2134-$213F).</summary>
	uint8_t Read(uint16_t addr);
	/// <summary>Handles PPU register writes ($2100-$2133).</summary>
	void Write(uint32_t addr, uint8_t value);

	void Serialize(Serializer& s) override;
};
