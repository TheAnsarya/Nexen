#pragma once
#include "pch.h"
#include "Debugger/DebugTypes.h"
#include "Shared/NotificationManager.h"
#include "Shared/Emulator.h"
#include "Shared/ColorUtilities.h"

class Debugger;

/// <summary>
/// Configuration for when to refresh PPU viewer.
/// </summary>
struct ViewerRefreshConfig {
	uint16_t Scanline; ///< Scanline to refresh on
	uint16_t Cycle;    ///< Cycle to refresh on
};

/// <summary>
/// Sprite visibility status.
/// </summary>
enum class SpriteVisibility : uint8_t {
	Visible = 0,   ///< Sprite visible on screen
	Offscreen = 1, ///< Sprite outside screen bounds
	Disabled = 2   ///< Sprite disabled in OAM
};

/// <summary>
/// Nullable boolean for platform-specific sprite properties.
/// </summary>
enum class NullableBoolean : int8_t {
	Undefined = -1, ///< Property not applicable
	False = 0,      ///< Property false
	True = 1        ///< Property true
};

/// <summary>
/// Sprite priority for layering.
/// </summary>
enum class DebugSpritePriority : int8_t {
	Undefined = -1, ///< Priority not applicable
	Number0 = 0,    ///< Priority 0 (highest)
	Number1 = 1,    ///< Priority 1
	Number2 = 2,    ///< Priority 2
	Number3 = 3,    ///< Priority 3 (lowest)
	Foreground = 4, ///< Foreground layer
	Background = 5  ///< Background layer
};

/// <summary>
/// Sprite rendering mode.
/// </summary>
enum class DebugSpriteMode : int8_t {
	Undefined = -1, ///< Mode not applicable
	Normal = 0,     ///< Normal sprite
	Blending,       ///< Alpha blending
	Window,         ///< Window mask sprite
	Stereoscopic    ///< 3D stereoscopic sprite
};

/// <summary>
/// Sprite information for debugger sprite viewer.
/// </summary>
struct DebugSpriteInfo {
	int32_t TileIndex;      ///< Tile number in tileset
	int32_t TileAddress;    ///< VRAM address of tile
	int32_t PaletteAddress; ///< Palette RAM address
	TileFormat Format;      ///< Tile format (bpp, encoding)

	int16_t SpriteIndex; ///< Sprite number in OAM

	int16_t X;    ///< Sprite X position (screen)
	int16_t Y;    ///< Sprite Y position (screen)
	int16_t RawX; ///< Raw X from OAM
	int16_t RawY; ///< Raw Y from OAM

	int16_t Bpp;                      ///< Bits per pixel (2, 4, 8)
	int16_t Palette;                  ///< Palette index
	DebugSpritePriority Priority;     ///< Sprite priority
	DebugSpriteMode Mode;             ///< Sprite mode
	uint16_t Width;                   ///< Sprite width (pixels)
	uint16_t Height;                  ///< Sprite height (pixels)
	NullableBoolean HorizontalMirror; ///< Horizontal flip
	NullableBoolean VerticalMirror;   ///< Vertical flip
	NullableBoolean MosaicEnabled;    ///< Mosaic effect enabled
	NullableBoolean TransformEnabled; ///< Affine transform enabled
	NullableBoolean DoubleSize;       ///< Double size (GBA)
	int8_t TransformParamIndex;       ///< Affine parameter index
	SpriteVisibility Visibility;      ///< Visibility status
	bool UseExtendedVram;             ///< Use extended VRAM (SNES)
	NullableBoolean UseSecondTable;   ///< Use second tile table

	uint32_t TileCount;            ///< Number of tiles in sprite
	uint32_t TileAddresses[8 * 8]; ///< VRAM addresses of all tiles (max 8x8 tiles)

public:
	/// <summary>
	/// Initialize sprite info to default values.
	/// </summary>
	void Init() {
		TileIndex = -1;
		TileAddress = -1;
		PaletteAddress = -1;
		Format = {};
		SpriteIndex = -1;
		X = -1;
		Y = -1;
		RawX = -1;
		RawY = -1;
		Bpp = 2;
		Palette = -1;
		Priority = DebugSpritePriority::Undefined;
		Width = 0;
		Height = 0;
		HorizontalMirror = NullableBoolean::Undefined;
		VerticalMirror = NullableBoolean::Undefined;
		MosaicEnabled = NullableBoolean::Undefined;
		TransformEnabled = NullableBoolean::Undefined;
		Mode = DebugSpriteMode::Undefined;
		DoubleSize = NullableBoolean::Undefined;
		TransformParamIndex = -1;
		Visibility = SpriteVisibility::Offscreen;
		UseExtendedVram = false;
		UseSecondTable = NullableBoolean::Undefined;
		TileCount = 0;
	}
};

/// <summary>
/// Nametable mirroring modes.
/// </summary>
enum class TilemapMirroring {
	None,          ///< No mirroring
	Horizontal,    ///< Horizontal mirroring (NES)
	Vertical,      ///< Vertical mirroring (NES)
	SingleScreenA, ///< Single screen A (NES)
	SingleScreenB, ///< Single screen B (NES)
	FourScreens,   ///< Four screens (NES)
};

/// <summary>
/// Tilemap layer information for debugger tilemap viewer.
/// </summary>
struct DebugTilemapInfo {
	uint32_t Bpp;               ///< Bits per pixel (2, 4, 8)
	TileFormat Format;          ///< Tile format
	TilemapMirroring Mirroring; ///< Nametable mirroring

	uint32_t TileWidth;  ///< Tile width (pixels, usually 8)
	uint32_t TileHeight; ///< Tile height (pixels, usually 8)

	uint32_t ScrollX;      ///< Scroll X position
	uint32_t ScrollWidth;  ///< Scroll width (for wraparound)
	uint32_t ScrollY;      ///< Scroll Y position
	uint32_t ScrollHeight; ///< Scroll height (for wraparound)

	uint32_t RowCount;       ///< Number of tile rows
	uint32_t ColumnCount;    ///< Number of tile columns
	uint32_t TilemapAddress; ///< Nametable/tilemap address
	uint32_t TilesetAddress; ///< CHR/pattern table address
	int8_t Priority = -1;    ///< Background layer priority
};

/// <summary>
/// Individual tile information for tilemap viewer.
/// </summary>
struct DebugTilemapTileInfo {
	int32_t Row = -1;    ///< Tile row in tilemap
	int32_t Column = -1; ///< Tile column in tilemap
	int32_t Width = -1;  ///< Tile width (pixels)
	int32_t Height = -1; ///< Tile height (pixels)

	int32_t TileMapAddress = -1; ///< Nametable entry address

	int32_t TileIndex = -1;   ///< Tile number
	int32_t TileAddress = -1; ///< CHR/pattern address

	int32_t PixelData = -1; ///< Pixel data value

	int32_t PaletteIndex = -1;     ///< Palette number
	int32_t PaletteAddress = -1;   ///< Palette RAM address
	int32_t BasePaletteIndex = -1; ///< Base palette index

	int32_t AttributeAddress = -1; ///< Attribute table address
	int16_t AttributeData = -1;    ///< Attribute byte value

	NullableBoolean HorizontalMirroring = NullableBoolean::Undefined; ///< H-flip
	NullableBoolean VerticalMirroring = NullableBoolean::Undefined;   ///< V-flip
	NullableBoolean HighPriority = NullableBoolean::Undefined;        ///< High priority
};

/// <summary>
/// Sprite preview viewport information.
/// </summary>
struct DebugSpritePreviewInfo {
	uint32_t Width;       ///< Sprite canvas width
	uint32_t Height;      ///< Sprite canvas height
	uint32_t SpriteCount; ///< Number of sprites
	int32_t CoordOffsetX; ///< X coordinate offset
	int32_t CoordOffsetY; ///< Y coordinate offset

	uint32_t VisibleX;      ///< Visible region X
	uint32_t VisibleY;      ///< Visible region Y
	uint32_t VisibleWidth;  ///< Visible region width
	uint32_t VisibleHeight; ///< Visible region height

	bool WrapBottomToTop; ///< Wrap Y coordinates (bottom wraps to top)
	bool WrapRightToLeft; ///< Wrap X coordinates (right wraps to left)
};

/// <summary>
/// Raw palette data format.
/// </summary>
enum class RawPaletteFormat {
	Indexed, ///< Indexed palette (color number references master palette)
	Rgb555,  ///< 15-bit RGB (5 bits per channel)
	Rgb333,  ///< 9-bit RGB (3 bits per channel)
	Rgb222,  ///< 6-bit RGB (2 bits per channel)
	Rgb444,  ///< 12-bit RGB (4 bits per channel)
	Bgr444   ///< 12-bit BGR (4 bits per channel)
};

/// <summary>
/// Palette information for debugger palette viewer.
/// </summary>
struct DebugPaletteInfo {
	MemoryType PaletteMemType; ///< Memory type of palette
	uint32_t PaletteMemOffset; ///< Offset in palette memory
	bool HasMemType;           ///< True if memory type available

	uint32_t ColorCount;          ///< Total color count
	uint32_t BgColorCount;        ///< Background color count
	uint32_t SpriteColorCount;    ///< Sprite color count
	uint32_t SpritePaletteOffset; ///< Offset to sprite palette

	uint32_t ColorsPerPalette; ///< Colors per sub-palette

	RawPaletteFormat RawFormat; ///< Raw palette format
	uint32_t RawPalette[512];   ///< Raw palette data
	uint32_t RgbPalette[512];   ///< RGB converted palette
};

/// <summary>
/// Platform-agnostic PPU debugging tools (tile viewer, sprite viewer, tilemap viewer, palette viewer).
/// </summary>
/// <remarks>
/// Architecture:
/// - Base class for platform-specific implementations (NesPpuTools, SnesPpuTools, etc.)
/// - Provides shared tile/sprite/palette rendering logic
/// - Platform-specific classes override GetTilemap, GetSpriteList, etc.
///
/// Debugging tools:
/// - Tile viewer: Display all tiles in CHR ROM/VRAM
/// - Sprite viewer: Display all sprites with OAM data
/// - Tilemap viewer: Display background layers (nametables)
/// - Palette viewer: Display and edit palette colors
///
/// Viewer refresh:
/// - SetViewerUpdateTiming(): Register viewer to refresh at scanline/cycle
/// - UpdateViewers(): Called at each scanline/cycle to refresh registered viewers
/// - Viewers refresh at specific timing for accuracy (mid-scanline effects)
///
/// Tile rendering:
/// - Template GetRgbPixelColor<format>(): Convert palette index to RGB
/// - Template GetTilePixelColor<format>(): Extract pixel from tile data
/// - Template InternalGetTileView<format>(): Render tile view with templates
///
/// Performance optimizations:
/// - __forceinline template functions for tile rendering
/// - Constexpr grayscale palettes for 1/2/4bpp
/// - Platform-specific fast paths
///
/// Use cases:
/// - View CHR/VRAM contents (tile viewer)
/// - Debug sprite positions/palettes (sprite viewer)
/// - View background layers (tilemap viewer)
/// - Edit palette colors (palette editor)
/// </remarks>
class PpuTools {
protected:
	static constexpr uint32_t _spritePreviewSize = 128 * 128;                                             ///< Sprite preview canvas size
	static constexpr uint32_t _grayscaleColorsBpp1[2] = {0xFF000000, 0xFFFFFFFF};                         ///< 1bpp grayscale palette
	static constexpr uint32_t _grayscaleColorsBpp2[4] = {0xFF000000, 0xFF666666, 0xFFBBBBBB, 0xFFFFFFFF}; ///< 2bpp grayscale palette
	static constexpr uint32_t _grayscaleColorsBpp4[16] = {                                                ///< 4bpp grayscale palette
	    0xFF000000, 0xFF303030, 0xFF404040, 0xFF505050, 0xFF606060, 0xFF707070, 0xFF808080, 0xFF909090,
	    0xFF989898, 0xFFA0A0A0, 0xFFAAAAAA, 0xFFBBBBBB, 0xFFCCCCCC, 0xFFDDDDDD, 0xFFEEEEEE, 0xFFFFFFFF};

	Emulator* _emu;                                              ///< Emulator instance
	Debugger* _debugger;                                         ///< Debugger instance
	unordered_map<uint32_t, ViewerRefreshConfig> _updateTimings; ///< Viewer ID → refresh timing

	/// <summary>
	/// Blend two colors (alpha compositing).
	/// </summary>
	/// <param name="output">Output RGBA color</param>
	/// <param name="input">Input RGBA color</param>
	void BlendColors(uint8_t output[4], uint8_t input[4]);

	/// <summary>
	/// Get RGB color for palette index (template for compile-time optimization).
	/// </summary>
	/// <typeparam name="format">Tile format</typeparam>
	/// <param name="colors">Palette array</param>
	/// <param name="colorIndex">Color index</param>
	/// <param name="palette">Palette number</param>
	/// <returns>ARGB color</returns>
	template <TileFormat format>
	__forceinline uint32_t GetRgbPixelColor(const uint32_t* colors, uint8_t colorIndex, uint8_t palette);

	/// <summary>
	/// Get tile pixel color index (template for compile-time optimization).
	/// </summary>
	/// <typeparam name="format">Tile format</typeparam>
	/// <param name="ram">Tile data RAM</param>
	/// <param name="ramMask">RAM size mask</param>
	/// <param name="rowStart">Row start address</param>
	/// <param name="pixelIndex">Pixel index (0-7)</param>
	/// <returns>Color index</returns>
	template <TileFormat format>
	__forceinline uint8_t GetTilePixelColor(const uint8_t* ram, const uint32_t ramMask, uint32_t rowStart, uint8_t pixelIndex);

	/// <summary>
	/// Check if tile should be hidden (based on tile viewer options).
	/// </summary>
	/// <param name="memType">Memory type</param>
	/// <param name="addr">Tile address</param>
	/// <param name="options">Tile view options</param>
	/// <returns>True if tile should be hidden</returns>
	bool IsTileHidden(MemoryType memType, uint32_t addr, GetTileViewOptions& options);

	/// <summary>
	/// Get background color for tile viewer.
	/// </summary>
	/// <param name="bgColor">Background color option</param>
	/// <param name="colors">Palette colors</param>
	/// <param name="paletteIndex">Palette index</param>
	/// <param name="bpp">Bits per pixel</param>
	/// <returns>ARGB background color</returns>
	uint32_t GetBackgroundColor(TileBackground bgColor, const uint32_t* colors, uint8_t paletteIndex = 0, uint8_t bpp = 0);

	/// <summary>
	/// Get background color for sprite viewer.
	/// </summary>
	/// <param name="bgColor">Background color option</param>
	/// <param name="colors">Palette colors</param>
	/// <param name="useDarkerColor">True to use darker background</param>
	/// <returns>ARGB background color</returns>
	uint32_t GetSpriteBackgroundColor(SpriteBackground bgColor, const uint32_t* colors, bool useDarkerColor);

	/// <summary>
	/// Get or set tile pixel (internal helper).
	/// </summary>
	/// <param name="tileAddress">Tile address</param>
	/// <param name="format">Tile format</param>
	/// <param name="x">Pixel X (0-7)</param>
	/// <param name="y">Pixel Y (0-7)</param>
	/// <param name="color">Color index (input for set, output for get)</param>
	/// <param name="forGet">True for get, false for set</param>
	void GetSetTilePixel(AddressInfo tileAddress, TileFormat format, int32_t x, int32_t y, int32_t& color, bool forGet);

public:
	/// <summary>
	/// Constructor for PPU tools.
	/// </summary>
	/// <param name="debugger">Debugger instance</param>
	/// <param name="emu">Emulator instance</param>
	PpuTools(Debugger* debugger, Emulator* emu);

	/// <summary>
	/// Get platform-specific PPU tools state (override in platform classes).
	/// </summary>
	/// <param name="state">Output state</param>
	virtual void GetPpuToolsState(BaseState& state) {};

	/// <summary>
	/// Get palette information.
	/// </summary>
	/// <param name="options">Palette options</param>
	/// <returns>Palette info</returns>
	virtual DebugPaletteInfo GetPaletteInfo(GetPaletteInfoOptions options) = 0;

	/// <summary>
	/// Render tile view.
	/// </summary>
	/// <param name="options">Tile view options</param>
	/// <param name="source">Source data (CHR ROM/VRAM)</param>
	/// <param name="srcSize">Source size</param>
	/// <param name="palette">Palette colors</param>
	/// <param name="outBuffer">Output ARGB buffer</param>
	void GetTileView(GetTileViewOptions options, uint8_t* source, uint32_t srcSize, const uint32_t* palette, uint32_t* outBuffer);

	/// <summary>
	/// Get information for tilemap tile at position.
	/// </summary>
	/// <param name="x">Tilemap X (pixel)</param>
	/// <param name="y">Tilemap Y (pixel)</param>
	/// <param name="vram">VRAM data</param>
	/// <param name="options">Tilemap options</param>
	/// <param name="baseState">Base emulator state</param>
	/// <param name="ppuToolsState">PPU tools state</param>
	/// <returns>Tile info</returns>
	virtual DebugTilemapTileInfo GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& baseState, BaseState& ppuToolsState) = 0;

	/// <summary>
	/// Get tilemap size.
	/// </summary>
	/// <param name="options">Tilemap options</param>
	/// <param name="state">Emulator state</param>
	/// <returns>Frame info with tilemap dimensions</returns>
	virtual FrameInfo GetTilemapSize(GetTilemapOptions options, BaseState& state) = 0;

	/// <summary>
	/// Render tilemap.
	/// </summary>
	/// <param name="options">Tilemap options</param>
	/// <param name="state">Emulator state</param>
	/// <param name="ppuToolsState">PPU tools state</param>
	/// <param name="vram">VRAM data</param>
	/// <param name="palette">Palette colors</param>
	/// <param name="outBuffer">Output ARGB buffer</param>
	/// <returns>Tilemap info</returns>
	virtual DebugTilemapInfo GetTilemap(GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) = 0;

	/// <summary>
	/// Get sprite preview viewport info.
	/// </summary>
	/// <param name="options">Sprite preview options</param>
	/// <param name="state">Emulator state</param>
	/// <param name="ppuToolsState">PPU tools state</param>
	/// <returns>Sprite preview info</returns>
	virtual DebugSpritePreviewInfo GetSpritePreviewInfo(GetSpritePreviewOptions options, BaseState& state, BaseState& ppuToolsState) = 0;

	/// <summary>
	/// Get sprite list and render preview.
	/// </summary>
	/// <param name="options">Sprite preview options</param>
	/// <param name="baseState">Base emulator state</param>
	/// <param name="ppuToolsState">PPU tools state</param>
	/// <param name="vram">VRAM data</param>
	/// <param name="oamRam">OAM data</param>
	/// <param name="palette">Palette colors</param>
	/// <param name="outBuffer">Output sprite info array</param>
	/// <param name="spritePreviews">Output sprite preview bitmaps</param>
	/// <param name="screenPreview">Output screen preview bitmap</param>
	virtual void GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) = 0;

	/// <summary>
	/// Get tile pixel color.
	/// </summary>
	/// <param name="tileAddress">Tile address</param>
	/// <param name="format">Tile format</param>
	/// <param name="x">Pixel X (0-7)</param>
	/// <param name="y">Pixel Y (0-7)</param>
	/// <returns>Color index</returns>
	int32_t GetTilePixel(AddressInfo tileAddress, TileFormat format, int32_t x, int32_t y);

	/// <summary>
	/// Set tile pixel color.
	/// </summary>
	/// <param name="tileAddress">Tile address</param>
	/// <param name="format">Tile format</param>
	/// <param name="x">Pixel X (0-7)</param>
	/// <param name="y">Pixel Y (0-7)</param>
	/// <param name="color">Color index</param>
	void SetTilePixel(AddressInfo tileAddress, TileFormat format, int32_t x, int32_t y, int32_t color);

	/// <summary>
	/// Set palette color.
	/// </summary>
	/// <param name="colorIndex">Color index</param>
	/// <param name="color">ARGB color</param>
	virtual void SetPaletteColor(int32_t colorIndex, uint32_t color) = 0;

	/// <summary>
	/// Set viewer update timing.
	/// </summary>
	/// <param name="viewerId">Viewer ID</param>
	/// <param name="scanline">Scanline to refresh on</param>
	/// <param name="cycle">Cycle to refresh on</param>
	virtual void SetViewerUpdateTiming(uint32_t viewerId, uint16_t scanline, uint16_t cycle);

	/// <summary>
	/// Remove viewer.
	/// </summary>
	/// <param name="viewerId">Viewer ID to remove</param>
	void RemoveViewer(uint32_t viewerId);

	/// <summary>
	/// Update viewers at current scanline/cycle.
	/// </summary>
	/// <param name="scanline">Current scanline</param>
	/// <param name="cycle">Current cycle</param>
	void UpdateViewers(uint16_t scanline, uint16_t cycle);

	/// <summary>
	/// Check if any viewers are open (hot path).
	/// </summary>
	/// <returns>True if viewers registered</returns>
	/// <remarks>
	/// Inline for performance - called frequently to skip viewer updates.
	/// </remarks>
	__forceinline bool HasOpenedViewer() {
		return _updateTimings.size() > 0;
	}

	/// <summary>
	/// Internal tile view renderer (template for compile-time optimization).
	/// </summary>
	/// <typeparam name="format">Tile format</typeparam>
	/// <param name="options">Tile view options</param>
	/// <param name="source">Source data</param>
	/// <param name="srcSize">Source size</param>
	/// <param name="colors">Palette colors</param>
	/// <param name="outBuffer">Output ARGB buffer</param>
	template <TileFormat format>
	void InternalGetTileView(GetTileViewOptions options, uint8_t* source, uint32_t srcSize, const uint32_t* colors, uint32_t* outBuffer);
};

template <TileFormat format>
uint32_t PpuTools::GetRgbPixelColor(const uint32_t* colors, uint8_t colorIndex, uint8_t palette) {
	switch (format) {
		case TileFormat::DirectColor:
			return ColorUtilities::Rgb555ToArgb(
			    ((((colorIndex & 0x07) << 1) | (palette & 0x01)) << 1) |
			    (((colorIndex & 0x38) | ((palette & 0x02) << 1)) << 4) |
			    (((colorIndex & 0xC0) | ((palette & 0x04) << 3)) << 7));

		case TileFormat::NesBpp2:
		case TileFormat::Bpp2:
			return colors[palette * 4 + colorIndex];

		case TileFormat::Bpp4:
		case TileFormat::SmsBpp4:
		case TileFormat::GbaBpp4:
		case TileFormat::WsBpp4Packed:
		case TileFormat::PceSpriteBpp4:
		case TileFormat::PceSpriteBpp2Sp01:
		case TileFormat::PceSpriteBpp2Sp23:
		case TileFormat::PceBackgroundBpp2Cg0:
		case TileFormat::PceBackgroundBpp2Cg1:
			return colors[palette * 16 + colorIndex];

		case TileFormat::Bpp8:
		case TileFormat::GbaBpp8:
		case TileFormat::Mode7:
		case TileFormat::Mode7ExtBg:
			return colors[palette * 256 + colorIndex];

		case TileFormat::Mode7DirectColor:
			return ColorUtilities::Rgb555ToArgb(((colorIndex & 0x07) << 2) | ((colorIndex & 0x38) << 4) | ((colorIndex & 0xC0) << 7));

		case TileFormat::SmsSgBpp1:
			return colors[palette * 2 + colorIndex];

		[[unlikely]] default:
			throw std::runtime_error("unsupported format");
	}
}

template <TileFormat format>
__forceinline uint8_t PpuTools::GetTilePixelColor(const uint8_t* ram, const uint32_t ramMask, uint32_t rowStart, uint8_t pixelIndex) {
	uint8_t shift = (7 - pixelIndex);
	uint8_t color;
	switch (format) {
		case TileFormat::PceSpriteBpp4:
		case TileFormat::PceSpriteBpp2Sp01:
		case TileFormat::PceSpriteBpp2Sp23:
			shift = 15 - pixelIndex;
			if (shift >= 8) {
				shift -= 8;
				rowStart++;
			}
			break;

		default:
			break;
	}

	switch (format) {
		case TileFormat::PceSpriteBpp4:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 32) & ramMask] >> shift) & 0x01) << 1);
			color |= (((ram[(rowStart + 64) & ramMask] >> shift) & 0x01) << 2);
			color |= (((ram[(rowStart + 96) & ramMask] >> shift) & 0x01) << 3);
			return color;

		case TileFormat::PceSpriteBpp2Sp01:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 32) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::PceSpriteBpp2Sp23:
			color = (((ram[(rowStart + 64) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 96) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::PceBackgroundBpp2Cg0:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 1) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::PceBackgroundBpp2Cg1:
			color = (((ram[(rowStart + 16) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 17) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::Bpp2:
			color = (((ram[rowStart & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 1) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::NesBpp2:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 8) & ramMask] >> shift) & 0x01) << 1);
			return color;

		case TileFormat::Bpp4:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 1) & ramMask] >> shift) & 0x01) << 1);
			color |= (((ram[(rowStart + 16) & ramMask] >> shift) & 0x01) << 2);
			color |= (((ram[(rowStart + 17) & ramMask] >> shift) & 0x01) << 3);
			return color;

		case TileFormat::Bpp8:
		case TileFormat::DirectColor:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 1) & ramMask] >> shift) & 0x01) << 1);
			color |= (((ram[(rowStart + 16) & ramMask] >> shift) & 0x01) << 2);
			color |= (((ram[(rowStart + 17) & ramMask] >> shift) & 0x01) << 3);
			color |= (((ram[(rowStart + 32) & ramMask] >> shift) & 0x01) << 4);
			color |= (((ram[(rowStart + 33) & ramMask] >> shift) & 0x01) << 5);
			color |= (((ram[(rowStart + 48) & ramMask] >> shift) & 0x01) << 6);
			color |= (((ram[(rowStart + 49) & ramMask] >> shift) & 0x01) << 7);
			return color;

		case TileFormat::Mode7:
		case TileFormat::Mode7DirectColor:
			return ram[(rowStart + pixelIndex * 2 + 1) & ramMask];

		case TileFormat::Mode7ExtBg:
			return ram[(rowStart + pixelIndex * 2 + 1) & ramMask] & 0x7F;

		case TileFormat::SmsBpp4:
			color = (((ram[(rowStart + 0) & ramMask] >> shift) & 0x01) << 0);
			color |= (((ram[(rowStart + 1) & ramMask] >> shift) & 0x01) << 1);
			color |= (((ram[(rowStart + 2) & ramMask] >> shift) & 0x01) << 2);
			color |= (((ram[(rowStart + 3) & ramMask] >> shift) & 0x01) << 3);
			return color;

		case TileFormat::SmsSgBpp1:
			color = ((ram[rowStart & ramMask] >> shift) & 0x01);
			return color;

		case TileFormat::GbaBpp4: {
			uint8_t pixelOffset = (7 - shift);
			uint32_t addr = (rowStart + (pixelOffset >> 1));
			if (addr <= ramMask) {
				if (pixelOffset & 0x01) {
					return ram[addr] >> 4;
				} else {
					return ram[addr] & 0x0F;
				}
			} else {
				return 0;
			}
		}

		case TileFormat::GbaBpp8: {
			uint8_t pixelOffset = (7 - shift);
			uint32_t addr = rowStart + pixelOffset;
			if (addr <= ramMask) {
				return ram[addr];
			} else {
				return 0;
			}
		}

		case TileFormat::WsBpp4Packed: {
			uint8_t pixelOffset = (7 - shift);
			uint32_t addr = (rowStart + (pixelOffset >> 1));
			if (addr <= ramMask) {
				if (pixelOffset & 0x01) {
					return ram[addr] & 0x0F;
				} else {
					return ram[addr] >> 4;
				}
			} else {
				return 0;
			}
		}

		[[unlikely]] default:
			throw std::runtime_error("unsupported format");
	}
}
