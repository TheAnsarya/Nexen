#pragma once
#include "pch.h"
#include <memory>
#include <array>
#include "GBA/GbaTypes.h"
#include "Shared/Emulator.h"
#include "Utilities/Timer.h"
#include "Utilities/ISerializable.h"

class Emulator;
class GbaConsole;
class GbaMemoryManager;

/// <summary>
/// Per-layer rendering state for background tilemap fetching.
/// </summary>
struct GbaLayerRendererData {
	uint16_t TilemapData;    ///< Current tilemap entry (tile index + attributes)
	uint16_t TileData;       ///< Current tile pattern data
	uint16_t FetchAddr;      ///< VRAM address for next fetch

	uint16_t TileIndex;      ///< Tile number being rendered
	int16_t RenderX;         ///< Current X position in output
	int16_t NextLoad;        ///< X position where next tile load needed

	int32_t TransformX;      ///< Affine X parameter (modes 1/2)
	int32_t TransformY;      ///< Affine Y parameter (modes 1/2)
	uint32_t XPos;           ///< Current X in transform space
	uint32_t YPos;           ///< Current Y in transform space

	uint8_t PaletteIndex;    ///< Palette bank for 4bpp tiles

	bool HoriMirror;         ///< Horizontal flip flag
	bool VertMirror;         ///< Vertical flip flag

	uint8_t TileRow;         ///< Row within current tile (0-7)
	uint8_t TileColumn;      ///< Column within current tile (0-7)
	uint8_t MosaicColor;     ///< Held color for mosaic effect

	uint8_t TileFetchCounter; ///< Cycles until next fetch complete
	bool RenderingDone;       ///< Layer rendering finished for scanline
	bool LastTile;            ///< Processing final tile of scanline
};

/// <summary>
/// Per-sprite rendering state for OAM sprite processing.
/// </summary>
struct GbaSpriteRendererData {
	int16_t MatrixData[4];   ///< Affine transform matrix (PA, PB, PC, PD)
	int32_t XValue;          ///< Accumulated X in transform space
	int32_t YValue;          ///< Accumulated Y in transform space
	uint32_t XPos;           ///< Source X position
	uint32_t YPos;           ///< Source Y position
	int16_t CenterX;         ///< Rotation center X
	int16_t CenterY;         ///< Rotation center Y

	uint16_t SpriteX;        ///< Screen X position
	uint16_t TileIndex;      ///< Base tile number
	uint16_t Addr;           ///< VRAM address
	uint8_t RenderX;         ///< Current render position
	uint8_t SpriteY;         ///< Screen Y position
	uint8_t YOffset;         ///< Y offset within sprite
	bool TransformEnabled;   ///< Affine transform active
	bool DoubleSize;         ///< Double rendering area for affine
	bool HideSprite;         ///< Sprite disabled

	GbaPpuObjMode Mode;      ///< Normal, semi-transparent, or window
	bool Mosaic;             ///< Mosaic effect enabled
	bool Bpp8Mode;           ///< 8bpp (256 colors) vs 4bpp (16 colors)
	uint8_t Shape;           ///< Square, horizontal, or vertical
	bool HoriMirror;         ///< Horizontal flip (non-affine only)
	bool VertMirror;         ///< Vertical flip (non-affine only)
	uint8_t TransformParamSelect; ///< Which OAM matrix to use
	uint8_t Size;            ///< Size index (0-3)

	uint8_t Priority;        ///< Display priority (0=highest)
	uint8_t PaletteIndex;    ///< Palette bank for 4bpp sprites

	uint8_t Width;           ///< Sprite width in pixels
	uint8_t Height;          ///< Sprite height in pixels
};

/// <summary>
/// Pixel data for layer composition with priority tracking.
/// </summary>
struct GbaPixelData {
	uint16_t Color = 0;      ///< RGB555 color value
	uint8_t Priority = 0xFF; ///< Layer priority (0xFF = transparent)
	uint8_t Layer = 5;       ///< Source layer index
};

/// <summary>
/// GBA PPU (Picture Processing Unit) emulator.
/// Renders 240x160 display at 59.727 Hz with hardware effects.
/// </summary>
/// <remarks>
/// The GBA PPU provides:
/// - 4 background layers with various modes (Mode 0-5)
/// - 128 sprites (64x64 max, 128 per scanline)
/// - 15-bit color (32768 colors)
/// - Hardware effects: blending, mosaic, windowing, affine transforms
///
/// **Background Modes:**
/// - Mode 0: 4 regular BG layers (text mode)
/// - Mode 1: 2 regular + 1 affine BG layer
/// - Mode 2: 2 affine BG layers (rotation/scaling)
/// - Mode 3: 240x160 16bpp bitmap (single buffer)
/// - Mode 4: 240x160 8bpp bitmap (double buffer)
/// - Mode 5: 160x128 16bpp bitmap (double buffer)
///
/// **Memory:**
/// - 96KB VRAM: Tilemaps, tiles, bitmaps
/// - 1KB OAM: 128 sprite entries + affine parameters
/// - 1KB Palette: 256 BG + 256 sprite colors
///
/// **Timing:**
/// - 4 cycles per pixel (960 cycles visible + 272 cycles hblank)
/// - 228 scanlines (160 visible + 68 vblank)
/// - ~280,896 cycles per frame
///
/// **Special Features:**
/// - Affine transforms: Rotation, scaling, shearing (Modes 1/2)
/// - Color blending: Alpha, brighten, darken effects
/// - Windows: 2 rectangular + sprite window + outside
/// - Mosaic: Pixelation effect for BG and sprites
/// </remarks>
class GbaPpu final : public ISerializable {
private:
	static constexpr int SpriteLayerIndex = 4;    ///< Layer index for sprites
	static constexpr int BackdropLayerIndex = 5;  ///< Layer index for backdrop color
	static constexpr int EffectLayerIndex = 5;    ///< Layer index for effects
	static constexpr uint16_t DirectColorFlag = 0x8000;  ///< Flag for direct color mode
	static constexpr uint16_t SpriteBlendFlag = 0x4000;  ///< Flag for semi-transparent sprite
	static constexpr uint16_t SpriteMosaicFlag = 0x2000; ///< Flag for mosaic sprite

	// Window indices
	static constexpr uint8_t Window0 = 0;
	static constexpr uint8_t Window1 = 1;
	static constexpr uint8_t ObjWindow = 2;
	static constexpr uint8_t OutsideWindow = 3;
	static constexpr uint8_t NoWindow = 4;

	static constexpr uint16_t BlackColor = 0;
	static constexpr uint16_t WhiteColor = 0x7FFF;

	GbaPpuState _state = {};

	uint16_t _vblankStartScanline = 160;  ///< First vblank scanline
	uint16_t _lastScanline = 227;         ///< Last scanline before wrap

	bool _inOverclock = false;  ///< Running in overclocked scanlines

	Emulator* _emu = nullptr;
	GbaConsole* _console = nullptr;
	GbaMemoryManager* _memoryManager = nullptr;

	// Video memory pointers
	uint8_t* _vram = nullptr;       ///< 96KB Video RAM (byte access)
	uint16_t* _vram16 = nullptr;    ///< VRAM as 16-bit words
	uint32_t* _oam = nullptr;       ///< 1KB Object Attribute Memory
	uint16_t* _paletteRam = nullptr; ///< 1KB Palette RAM (512 colors)

	// Output buffers (double-buffered for tear-free display)
	std::array<std::unique_ptr<uint16_t[]>, 2> _outputBuffers;
	uint16_t* _currentBuffer = nullptr;

	// Sprite rendering state (double-buffered for evaluation)
	GbaPixelData* _oamWriteOutput = nullptr;  ///< OAM output being written
	GbaPixelData* _oamReadOutput = nullptr;   ///< OAM output being displayed
	GbaPixelData _renderSprPixel = {};

	GbaPixelData _oamOutputBuffers[2][GbaConstants::ScreenWidth] = {};
	GbaPixelData _layerOutput[4][GbaConstants::ScreenWidth] = {};

	// Window state
	uint8_t _oamWindow[GbaConstants::ScreenWidth] = {};   ///< Sprite window mask
	uint8_t _activeWindow[256] = {};  ///< Active window per pixel

	Timer _frameSkipTimer;
	bool _skipRender = false;  ///< Skip rendering for performance

	bool _triggerSpecialDma = false;  ///< HBlank/VBlank DMA trigger

	// Window tracking
	int16_t _lastWindowCycle = -1;
	bool _window0ActiveY = false;  ///< Window 0 active on this scanline
	bool _window1ActiveY = false;  ///< Window 1 active on this scanline
	bool _window0ActiveX = false;  ///< Window 0 active at current X
	bool _window1ActiveX = false;  ///< Window 1 active at current X
	uint8_t _windowChangePos[4] = {};  ///< X positions where windows change

	// Layer rendering state
	int16_t _lastRenderCycle = -1;
	GbaLayerRendererData _layerData[4] = {};  ///< Per-layer render state
	GbaSpriteRendererData _objData[2] = {};   ///< Current sprite being rendered

	// OAM evaluation state
	int16_t _evalOamIndex = -1;      ///< Current OAM entry being evaluated
	int16_t _loadOamTileCounter = 0; ///< Tiles remaining to load
	int16_t _oamLastCycle = -1;
	bool _oamHasWindowModeSprite = false;  ///< Scanline has window sprites
	bool _loadOamAttr01 = false;     ///< Loading OAM attributes 0/1
	bool _loadOamAttr2 = false;      ///< Loading OAM attribute 2
	bool _isFirstOamTileLoad = false;
	uint8_t _loadObjMatrix = 0;      ///< Loading affine matrix parameters
	uint8_t _oamScanline = 0;        ///< Scanline for OAM evaluation
	uint8_t _oamMosaicScanline = 0;  ///< Scanline for mosaic calculation
	uint8_t _oamMosaicY = 0;

	bool _newObjLayerEnabled = false;  ///< OBJ layer enable pending

	uint8_t _memoryAccess[308 * 4] = {};  ///< Memory access tracking per cycle

	// Color math function pointer table (templated specializations)
	typedef void (GbaPpu::*Func)();
	Func _colorMathFunc[128] = {};

	uint16_t _skippedOutput[240];  ///< Buffer for skipped frames

	/// <summary>
	/// Processes a single layer's pixel for composition.
	/// Templated for compile-time window optimization.
	/// </summary>
	template <int i, bool windowEnabled>
	__forceinline void ProcessLayerPixel(int x, uint8_t wnd, GbaPixelData& main, GbaPixelData& sub) {
		if constexpr (windowEnabled) {
			// Check if layer is visible in current window
			if (!_state.WindowActiveLayers[wnd][i]) {
				return;
			}
		}

		// Priority-based pixel selection (main gets higher priority, sub gets displaced main)
		if (_layerOutput[i][x].Priority < main.Priority) {
			sub = main;
			main = _layerOutput[i][x];
		} else if (_layerOutput[i][x].Priority < sub.Priority) {
			sub = _layerOutput[i][x];
		}
	}

	/// <summary>Applies color blending between layers based on current effect settings.</summary>
	template <GbaPpuBlendEffect effect, bool bg0Enabled, bool bg1Enabled, bool bg2Enabled, bool bg3Enabled, bool windowEnabled>
	void ProcessColorMath();

	/// <summary>Blends two colors with specified coefficients (alpha blending).</summary>
	void BlendColors(uint16_t* dst, int x, uint16_t a, uint8_t aCoeff, uint16_t b, uint8_t bCoeff);

	/// <summary>Reads a color from palette or direct color.</summary>
	template <bool isSubColor>
	uint16_t ReadColor(int x, uint16_t addr);

	// Window processing
	void InitializeWindows();       ///< Initializes window state for frame
	void ProcessWindow();           ///< Updates window state per scanline
	void SetWindowX(uint8_t& regValue, uint8_t newValue);
	void UpdateWindowChangePoints();
	void SetWindowActiveLayers(int window, uint8_t cfg);

	void SendFrame();  ///< Sends completed frame to display

	/// <summary>Outputs background pixels to layer buffer.</summary>
	template <int i, bool mosaic, bool bpp8>
	__forceinline void PushBgPixels(int renderX);
	template <int i, bool mosaic, bool bpp8, bool mirror>
	__forceinline void PushBgPixels(int renderX);

	/// <summary>Renders a regular (non-affine) tilemap background layer.</summary>
	template <int layer>
	void RenderTilemap();
	template <int layer, bool mosaic, bool bpp8>
	void RenderTilemap();

	/// <summary>Renders an affine (rotation/scaling) tilemap background layer.</summary>
	template <int layer>
	void RenderTransformTilemap();

	/// <summary>Renders bitmap mode backgrounds (modes 3/4/5).</summary>
	template <int mode>
	void RenderBitmapMode();

	/// <summary>Sets pixel data in layer output buffer with all attributes.</summary>
	__forceinline void SetPixelData(GbaPixelData& pixel, uint16_t color, uint8_t priority, uint8_t layer);

	/// <summary>Processes all sprites for the current scanline.</summary>
	void ProcessSprites();

	__noinline void InitSpriteEvaluation();  ///< Initializes sprite evaluation state
	void AddVisibleSprite(uint32_t sprData); ///< Adds sprite to visible list
	void LoadSpriteAttr2();                  ///< Loads sprite attribute 2 (tile/palette)
	void LoadSpriteTransformMatrix();        ///< Loads affine matrix for sprite

	/// <summary>Renders a single sprite with optional affine transform.</summary>
	template <bool transformEnabled, bool blockFirst16k>
	__forceinline void RenderSprite(GbaSpriteRendererData& spr);
	template <bool blockFirst16k>
	void RenderSprites();
	/// <summary>Reads sprite VRAM with bitmap mode blocking.</summary>
	template <bool blockFirst16k>
	__forceinline uint8_t ReadSpriteVram(uint32_t addr);

	/// <summary>Sets affine transform origin for a layer.</summary>
	template <int bit>
	void SetTransformOrigin(uint8_t i, uint8_t value, bool setY);
	void SetLayerEnabled(int layer, bool enabled);

	void ProcessEndOfScanline();     ///< End-of-scanline processing
	void ProcessHBlank();            ///< HBlank period processing
	void ProcessLayerToggleDelay();  ///< Delayed layer enable/disable

	void DebugProcessMemoryAccessView();
	uint16_t GetCurrentScanline();
	bool IsScanlineMatch();  ///< Checks if current scanline matches DISPSTAT

	/// <summary>Updates affine transform parameters for a layer each scanline.</summary>
	template <int i>
	void UpdateLayerTransform();

public:
	void Init(Emulator* emu, GbaConsole* console, GbaMemoryManager* memoryManager);
	~GbaPpu();

	/// <summary>
	/// Main PPU execution - runs one cycle.
	/// Called at 16.78 MHz (4x CPU clock).
	/// </summary>
	__forceinline void Exec() {
		_state.Cycle++;

		if (_state.Cycle == 308 * 4) {
			// End of scanline (1232 cycles)
			ProcessEndOfScanline();
		} else if (_state.Cycle >= 1006) {
			if (_state.Cycle == 1006) {
				// HBlank start (cycle 1006)
				ProcessHBlank();
			} else if (_state.Cycle == 1056) {
				// Render scanline (during hblank for next-line sprites)
				RenderScanline();
			}
		}

		_emu->ProcessPpuCycle<CpuType::Gba>();
	}

	/// <summary>Checks if PPU is accessing the specified memory type this cycle.</summary>
	bool IsAccessingMemory(uint8_t memType) {
		return _memoryAccess[_state.Cycle] & memType;
	}

	void ProcessObjEnableChange();  ///< Handles OBJ enable register changes

	void RenderScanline(bool forceRender = false);

	void DebugSendFrame();

	void WriteRegister(uint32_t addr, uint8_t value);  ///< PPU register write handler
	uint8_t ReadRegister(uint32_t addr);               ///< PPU register read handler

	uint16_t* GetScreenBuffer() { return _currentBuffer; }
	uint16_t* GetPreviousScreenBuffer() { return _currentBuffer == _outputBuffers[0].get() ? _outputBuffers[1].get() : _outputBuffers[0].get(); }

	GbaPpuState& GetState() { return _state; }
	[[nodiscard]] uint32_t GetFrameCount() { return _state.FrameCount; }
	[[nodiscard]] int32_t GetScanline() { return _state.Scanline; }
	[[nodiscard]] uint32_t GetScanlineCount() { return _lastScanline + 1; }
	[[nodiscard]] uint32_t GetCycle() { return _state.Cycle; }
	[[nodiscard]] bool IsBitmapMode() { return _state.BgMode >= 3; }  ///< Modes 3/4/5 are bitmap
	[[nodiscard]] bool IsOverclockScanline() { return _inOverclock; }

	void Serialize(Serializer& s) override;
};
