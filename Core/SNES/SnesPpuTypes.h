#pragma once
#include "pch.h"
#include "Shared/BaseState.h"

/// <summary>
/// Logic operation modes for combining Window 1 and Window 2 results.
/// </summary>
enum class WindowMaskLogic {
	/// <summary>OR the two window results together.</summary>
	Or = 0,

	/// <summary>AND the two window results together.</summary>
	And = 1,

	/// <summary>XOR the two window results together.</summary>
	Xor = 2,

	/// <summary>XNOR (exclusive-nor) the two window results together.</summary>
	Xnor = 3
};

/// <summary>
/// Color math window clipping modes for controlling where color math is applied.
/// </summary>
enum class ColorWindowMode {
	/// <summary>Color math is never applied (regardless of window).</summary>
	Never = 0,

	/// <summary>Color math applies only outside the window region.</summary>
	OutsideWindow = 1,

	/// <summary>Color math applies only inside the window region.</summary>
	InsideWindow = 2,

	/// <summary>Color math is always applied (regardless of window).</summary>
	Always = 3
};

/// <summary>
/// Sprite/OBJ information extracted from OAM for rendering.
/// </summary>
/// <remarks>
/// Contains position, size, and rendering attributes for a single sprite.
/// The SNES supports 128 sprites with up to 32 visible per scanline.
/// </remarks>
struct SpriteInfo {
	/// <summary>X position (-256 to +255).</summary>
	int16_t X;

	/// <summary>Y position (0-255, wraps at 256).</summary>
	uint8_t Y;

	/// <summary>OAM index (0-127).</summary>
	uint8_t Index;

	/// <summary>Sprite width in pixels (8, 16, 32, or 64).</summary>
	uint8_t Width;

	/// <summary>Sprite height in pixels (8, 16, 32, or 64).</summary>
	uint8_t Height;

	/// <summary>True when sprite is horizontally mirrored.</summary>
	bool HorizontalMirror;

	/// <summary>Sprite priority (0-3, higher = in front).</summary>
	uint8_t Priority;

	/// <summary>Palette index (0-7, uses palettes 128-255).</summary>
	uint8_t Palette;

	/// <summary>Column offset for tile fetching.</summary>
	int8_t ColumnOffset;

	/// <summary>Final X coordinate for drawing.</summary>
	int16_t DrawX;

	/// <summary>VRAM address for character data fetch.</summary>
	uint16_t FetchAddress;

	/// <summary>Fetched character data (low and high planes).</summary>
	uint16_t ChrData[2];

	/// <summary>
	/// Checks if this sprite is visible on the specified scanline.
	/// </summary>
	/// <param name="scanline">Current scanline number.</param>
	/// <param name="interlace">True if object interlace is enabled.</param>
	/// <returns>True if the sprite is visible on this scanline.</returns>
	bool IsVisible(uint16_t scanline, bool interlace) {
		if (X != -256 && (X + Width <= 0 || X > 255)) {
			// Sprite is not visible (and must be ignored for time/range flag calculations)
			// Sprites at X=-256 are always used when considering Time/Range flag calculations, but not actually drawn.
			return false;
		}

		uint16_t endY = Y + (interlace ? (Height >> 1) : Height);
		return (
		    (scanline >= Y && scanline < endY) ||
		    ((uint8_t)endY < Y && scanline < (uint8_t)endY) // wrap-around occurs after 256 scanlines
		);
	}
};

/// <summary>
/// Cached tile data for a single background tile.
/// </summary>
struct TileData {
	/// <summary>Tilemap entry (tile number, palette, flip flags, priority).</summary>
	uint16_t TilemapData;

	/// <summary>Vertical scroll position at time of fetch.</summary>
	uint16_t VScroll;

	/// <summary>Fetched character data bitplanes (up to 8bpp = 4 words).</summary>
	uint16_t ChrData[4];
};

/// <summary>
/// Cached tile data for an entire background layer scanline.
/// </summary>
struct LayerData {
	/// <summary>Cached tiles for this scanline (33 tiles covers 256px + 8px scroll).</summary>
	TileData Tiles[33];
};

/// <summary>
/// Background layer configuration settings.
/// </summary>
struct LayerConfig {
	/// <summary>VRAM address for tilemap data (word address).</summary>
	uint16_t TilemapAddress = 0;

	/// <summary>VRAM address for character/tile graphics (word address).</summary>
	uint16_t ChrAddress = 0;

	/// <summary>Horizontal scroll position (0-1023).</summary>
	uint16_t HScroll = 0;

	/// <summary>Vertical scroll position (0-1023).</summary>
	uint16_t VScroll = 0;

	/// <summary>True for 64-tile wide tilemap (32 otherwise).</summary>
	bool DoubleWidth = false;

	/// <summary>True for 64-tile tall tilemap (32 otherwise).</summary>
	bool DoubleHeight = false;

	/// <summary>True for 16x16 tiles (8x8 otherwise).</summary>
	bool LargeTiles = false;
};

/// <summary>
/// Mode 7 transformation configuration.
/// </summary>
/// <remarks>
/// Mode 7 provides affine transformation (rotation, scaling, shearing)
/// for a single background layer using a 2x2 transformation matrix.
/// </remarks>
struct Mode7Config {
	/// <summary>
	/// 2x2 transformation matrix coefficients [A, B, C, D].
	/// Transformation: X' = A*(X-CX) + B*(Y-CY) + CX + HScroll
	///                 Y' = C*(X-CX) + D*(Y-CY) + CY + VScroll
	/// </summary>
	int16_t Matrix[4] = {};

	/// <summary>Horizontal scroll offset (13-bit signed).</summary>
	int16_t HScroll = 0;

	/// <summary>Vertical scroll offset (13-bit signed).</summary>
	int16_t VScroll = 0;

	/// <summary>Rotation/scaling center X coordinate.</summary>
	int16_t CenterX = 0;

	/// <summary>Rotation/scaling center Y coordinate.</summary>
	int16_t CenterY = 0;

	/// <summary>Latch for writing 16-bit values via 8-bit registers.</summary>
	uint8_t ValueLatch = 0;

	/// <summary>True for 1024x1024 map, false for 128x128.</summary>
	bool LargeMap = false;

	/// <summary>When outside map: true=use tile 0, false=transparent.</summary>
	bool FillWithTile0 = false;

	/// <summary>Horizontal mirroring of the transformed layer.</summary>
	bool HorizontalMirroring = false;

	/// <summary>Vertical mirroring of the transformed layer.</summary>
	bool VerticalMirroring = false;

	/// <summary>Latched horizontal scroll at scanline start.</summary>
	int16_t HScrollLatch = 0;

	/// <summary>Latched vertical scroll at scanline start.</summary>
	int16_t VScrollLatch = 0;
};

/// <summary>
/// Window configuration for masking regions of the screen.
/// </summary>
struct WindowConfig {
	/// <summary>Per-layer enable flags [BG1, BG2, BG3, BG4, OBJ, Math].</summary>
	bool ActiveLayers[6];

	/// <summary>Per-layer inversion flags (inverts inside/outside logic).</summary>
	bool InvertedLayers[6];

	/// <summary>Left edge of window (0-255).</summary>
	uint8_t Left;

	/// <summary>Right edge of window (0-255).</summary>
	uint8_t Right;

	/// <summary>
	/// Tests if a pixel needs masking for the specified layer.
	/// </summary>
	/// <typeparam name="layerIndex">Layer index (0-5).</typeparam>
	/// <param name="x">X coordinate to test.</param>
	/// <returns>True if the pixel should be masked.</returns>
	template <uint8_t layerIndex>
	bool PixelNeedsMasking(int x) {
		if (InvertedLayers[layerIndex]) {
			if (Left > Right) {
				return true;
			} else {
				return x < Left || x > Right;
			}
		} else {
			if (Left > Right) {
				return false;
			} else {
				return x >= Left && x <= Right;
			}
		}
	}

	/// <summary>
	/// Non-template version for runtime layer index.
	/// </summary>
	bool PixelNeedsMasking(uint8_t layerIndex, int x) {
		if (InvertedLayers[layerIndex]) {
			return (Left > Right) ? true : (x < Left || x > Right);
		} else {
			return (Left > Right) ? false : (x >= Left && x <= Right);
		}
	}
};

/// <summary>
/// Complete SNES PPU (Picture Processing Unit) state.
/// </summary>
/// <remarks>
/// The SNES PPU handles all graphics rendering:
/// - Up to 4 background layers (mode-dependent)
/// - 128 sprites (OBJ layer)
/// - Multiple BG modes (0-7) with different bit depths
/// - Two hardware windows for masking
/// - Color math for transparency effects
/// - Mode 7 affine transformation
/// </remarks>
struct SnesPpuState : public BaseState {
	// =========================================================================
	// Cache Line 1: Hottest rendering fields (~64 bytes)
	// These fields are accessed in 3+ rendering functions per scanline.
	// =========================================================================

	/// <summary>Current background mode (0-7).</summary>
	uint8_t BgMode = 0;

	/// <summary>True when display is disabled (screen is black).</summary>
	bool ForcedBlank = false;

	/// <summary>Layers enabled on main screen (bit flags).</summary>
	uint8_t MainScreenLayers = 0;

	/// <summary>Layers enabled on sub screen (bit flags).</summary>
	uint8_t SubScreenLayers = 0;

	/// <summary>Master brightness level (0-15, 0=black, 15=full).</summary>
	uint8_t ScreenBrightness = 0;

	/// <summary>Color math enable flags per layer (bit flags).</summary>
	uint8_t ColorMathEnabled = 0;

	/// <summary>Mosaic block size (1-16 pixels).</summary>
	uint8_t MosaicSize = 0;

	/// <summary>Mosaic enable flags per BG layer (bit flags).</summary>
	uint8_t MosaicEnabled = 0;

	/// <summary>True for pseudo-hi-res mode (512px wide).</summary>
	bool HiResMode = false;

	/// <summary>True for interlaced display (480i).</summary>
	bool ScreenInterlace = false;

	/// <summary>True for interlaced OBJ rendering.</summary>
	bool ObjInterlace = false;

	/// <summary>Mode 1 BG3 priority flag (gives BG3 highest priority).</summary>
	bool Mode1Bg3Priority = false;

	/// <summary>True when EXTBG mode is enabled (Mode 7 external BG).</summary>
	bool ExtBgEnabled = false;

	/// <summary>True for 239-line mode (PAL-like), false for 224-line.</summary>
	bool OverscanMode = false;

	/// <summary>True to enable direct color mode (256-color palettes).</summary>
	bool DirectColorMode = false;

	/// <summary>OBJ size mode (0-7, determines small/large sprite sizes).</summary>
	uint8_t OamMode = 0;

	/// <summary>Color math window clipping mode.</summary>
	ColorWindowMode ColorMathClipMode = ColorWindowMode::Never;

	/// <summary>Color math window prevention mode.</summary>
	ColorWindowMode ColorMathPreventMode = ColorWindowMode::Never;

	/// <summary>True to use sub screen for color math (false = fixed color).</summary>
	bool ColorMathAddSubscreen = false;

	/// <summary>True for subtraction, false for addition.</summary>
	bool ColorMathSubtractMode = false;

	/// <summary>True to halve the color math result.</summary>
	bool ColorMathHalveResult = false;

	// 1 byte padding here (alignment for uint16_t)

	/// <summary>Fixed color for color math (15-bit BGR).</summary>
	uint16_t FixedColor = 0;

	// =========================================================================
	// Cache Lines 2-3: Layer config + Mode 7 (~72 bytes)
	// Accessed during tilemap rendering and Mode 7.
	// =========================================================================

	/// <summary>Configuration for BG layers 1-4.</summary>
	LayerConfig Layers[4] = {};

	/// <summary>Mode 7 transformation settings.</summary>
	Mode7Config Mode7 = {};

	// =========================================================================
	// Cache Lines 3-4: Window config (~76 bytes)
	// Accessed during PrecomputeWindowMasks (per scanline).
	// =========================================================================

	/// <summary>Window 1 and Window 2 configuration.</summary>
	WindowConfig Window[2] = {};

	/// <summary>Window mask logic for each layer [BG1-4, OBJ, Math].</summary>
	WindowMaskLogic MaskLogic[6] = {};

	/// <summary>Main screen window masking enable per layer.</summary>
	bool WindowMaskMain[5] = {};

	/// <summary>Sub screen window masking enable per layer.</summary>
	bool WindowMaskSub[5] = {};

	// =========================================================================
	// Cold fields: OAM, VRAM, CGRAM registers, timing
	// Only accessed during register I/O, not in rendering loops.
	// =========================================================================

	/// <summary>Base VRAM address for OBJ character data.</summary>
	uint16_t OamBaseAddress = 0;

	/// <summary>VRAM address offset for second OBJ name table.</summary>
	uint16_t OamAddressOffset = 0;

	/// <summary>OAM word address for CPU access.</summary>
	uint16_t OamRamAddress = 0;

	/// <summary>Internal OAM address (may differ during rendering).</summary>
	uint16_t InternalOamAddress = 0;

	/// <summary>True to enable OBJ priority rotation.</summary>
	bool EnableOamPriority = false;

	/// <summary>Current VRAM word address for reads/writes.</summary>
	uint16_t VramAddress = 0;

	/// <summary>VRAM address increment amount (1, 32, or 128).</summary>
	uint8_t VramIncrementValue = 0;

	/// <summary>VRAM address remapping mode (0-3).</summary>
	uint8_t VramAddressRemapping = 0;

	/// <summary>True to increment VRAM address on high byte access.</summary>
	bool VramAddrIncrementOnSecondReg = false;

	/// <summary>Prefetch buffer for VRAM reads.</summary>
	uint16_t VramReadBuffer = 0;

	/// <summary>Open bus value for PPU1 ($2134-$2136).</summary>
	uint8_t Ppu1OpenBus = 0;

	/// <summary>Open bus value for PPU2 ($213x registers).</summary>
	uint8_t Ppu2OpenBus = 0;

	/// <summary>CGRAM (palette) word address for writes.</summary>
	uint8_t CgramAddress = 0;

	/// <summary>Internal CGRAM address (may differ during rendering).</summary>
	uint8_t InternalCgramAddress = 0;

	/// <summary>Latch for writing 16-bit palette entries via 8-bit register.</summary>
	uint8_t CgramWriteBuffer = 0;

	/// <summary>Low/high byte latch for CGRAM access.</summary>
	bool CgramAddressLatch = false;

	/// <summary>Current cycle within the scanline (0-340).</summary>
	uint16_t Cycle = 0;

	/// <summary>Current scanline number (0-261/311).</summary>
	uint16_t Scanline = 0;

	/// <summary>Horizontal dot clock position.</summary>
	uint16_t HClock = 0;

	/// <summary>Total frames rendered since power-on.</summary>
	uint32_t FrameCount = 0;
};

/// <summary>
/// Per-pixel flags used during rendering.
/// </summary>
enum PixelFlags {
	/// <summary>Flag indicating this pixel can participate in color math.</summary>
	AllowColorMath = 0x80,
};
