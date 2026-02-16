#include "pch.h"
#include "Lynx/Debugger/LynxPpuTools.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxTypes.h"
#include "Debugger/Debugger.h"
#include "Debugger/MemoryDumper.h"
#include "Shared/ColorUtilities.h"

LynxPpuTools::LynxPpuTools(Debugger* debugger, Emulator* emu, LynxConsole* console)
	: PpuTools(debugger, emu) {
	_console = console;
}

DebugPaletteInfo LynxPpuTools::GetPaletteInfo(GetPaletteInfoOptions options) {
	DebugPaletteInfo info = {};

	// Lynx has a single 16-color palette (no separate BG/sprite palettes)
	info.RawFormat = RawPaletteFormat::Rgb444;
	info.ColorsPerPalette = 16;
	info.ColorCount = 16;
	info.BgColorCount = 16;
	info.SpriteColorCount = 0;
	info.SpritePaletteOffset = 0;
	info.HasMemType = false;

	LynxMikeyState state = _console->GetMikey()->GetState();

	for (int i = 0; i < LynxConstants::PaletteSize; i++) {
		// Raw register values: PaletteBR = [7:4]=blue [3:0]=red, PaletteGreen = [3:0]=green
		uint8_t r = state.PaletteBR[i] & 0x0f;
		uint8_t g = state.PaletteGreen[i] & 0x0f;
		uint8_t b = (state.PaletteBR[i] >> 4) & 0x0f;

		// Pack as RGB444 for raw display: RRRRGGGGBBBB
		uint16_t rgb444 = (r << 8) | (g << 4) | b;
		info.RawPalette[i] = rgb444;

		// ARGB32 from the already-converted palette
		info.RgbPalette[i] = state.Palette[i] | 0xff000000;
	}

	return info;
}

void LynxPpuTools::SetPaletteColor(int32_t colorIndex, uint32_t color) {
	if (colorIndex < 0 || colorIndex >= LynxConstants::PaletteSize) {
		return;
	}

	// Access Mikey state directly — Lynx SetPpuState is a no-op
	LynxMikeyState& state = _console->GetMikey()->GetState();

	// Extract 4-bit components from ARGB8888
	uint8_t r = (color >> 20) & 0x0f;
	uint8_t g = (color >> 12) & 0x0f;
	uint8_t b = (color >> 4) & 0x0f;

	state.PaletteGreen[colorIndex] = g;
	state.PaletteBR[colorIndex] = (b << 4) | r;

	// Update the expanded ARGB32 palette too
	state.Palette[colorIndex] = ColorUtilities::Rgb444ToArgb((r << 8) | (g << 4) | b);
}

DebugTilemapInfo LynxPpuTools::GetTilemap(GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) {
	DebugTilemapInfo info = {};

	// Lynx has no tilemap hardware — show the 160x102 linear framebuffer
	LynxMikeyState& mikeyState = (LynxMikeyState&)state;
	uint16_t displayAddr = mikeyState.DisplayAddress;

	info.Bpp = 4;
	info.Format = TileFormat::Bpp4;
	info.Mirroring = TilemapMirroring::None;
	info.TileWidth = 1;
	info.TileHeight = 1;
	info.ColumnCount = LynxConstants::ScreenWidth;
	info.RowCount = LynxConstants::ScreenHeight;
	info.ScrollX = 0;
	info.ScrollY = 0;
	info.ScrollWidth = LynxConstants::ScreenWidth;
	info.ScrollHeight = LynxConstants::ScreenHeight;
	info.TilemapAddress = displayAddr;
	info.TilesetAddress = -1;

	// Render the framebuffer: 4bpp packed pixels, 2 pixels per byte
	// Each scanline is 80 bytes (160 pixels / 2), with padding bytes
	uint32_t width = LynxConstants::ScreenWidth;
	uint32_t height = LynxConstants::ScreenHeight;

	// The display buffer address points to the start of the first scanline control block.
	// Each scanline has: [video DMA start addr (2 bytes)] [80 bytes pixel data] [1 byte end]
	// For simplicity in the stub, we just show what's in the framebuffer region.

	// TODO: Parse actual scanline DMA control blocks for accurate rendering
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			// Each byte holds 2 pixels: high nibble = left pixel, low nibble = right pixel
			uint32_t byteOffset = displayAddr + (y * (width / 2)) + (x / 2);
			uint8_t pixelByte = vram[byteOffset & 0xffff];
			uint8_t colorIndex = (x & 1) ? (pixelByte & 0x0f) : ((pixelByte >> 4) & 0x0f);

			outBuffer[y * width + x] = palette[colorIndex];
		}
	}

	return info;
}

FrameInfo LynxPpuTools::GetTilemapSize(GetTilemapOptions options, BaseState& state) {
	return { LynxConstants::ScreenWidth, LynxConstants::ScreenHeight };
}

DebugTilemapTileInfo LynxPpuTools::GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& baseState, BaseState& ppuToolsState) {
	DebugTilemapTileInfo info = {};

	// Show pixel-level info from the framebuffer
	LynxMikeyState& mikeyState = (LynxMikeyState&)baseState;

	if (x < LynxConstants::ScreenWidth && y < LynxConstants::ScreenHeight) {
		uint16_t displayAddr = mikeyState.DisplayAddress;
		uint32_t byteOffset = displayAddr + (y * (LynxConstants::ScreenWidth / 2)) + (x / 2);

		info.Row = y;
		info.Column = x;
		info.Width = 1;
		info.Height = 1;
		info.TileMapAddress = byteOffset & 0xffff;
		info.TileAddress = byteOffset & 0xffff;

		uint8_t pixelByte = vram[byteOffset & 0xffff];
		uint8_t colorIndex = (x & 1) ? (pixelByte & 0x0f) : ((pixelByte >> 4) & 0x0f);
		info.PaletteIndex = colorIndex;
		info.PixelData = colorIndex;
	}

	return info;
}

DebugSpritePreviewInfo LynxPpuTools::GetSpritePreviewInfo(GetSpritePreviewOptions options, BaseState& state, BaseState& ppuToolsState) {
	DebugSpritePreviewInfo info = {};

	// Canvas matches screen size
	info.Width = LynxConstants::ScreenWidth;
	info.Height = LynxConstants::ScreenHeight;
	info.VisibleX = 0;
	info.VisibleY = 0;
	info.VisibleWidth = LynxConstants::ScreenWidth;
	info.VisibleHeight = LynxConstants::ScreenHeight;
	info.CoordOffsetX = 0;
	info.CoordOffsetY = 0;
	info.WrapBottomToTop = false;
	info.WrapRightToLeft = false;

	// TODO: Walk SCB linked list to count sprites
	info.SpriteCount = 0;

	return info;
}

void LynxPpuTools::GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) {
	// TODO: Walk the Suzy SCB (Sprite Control Block) linked list and populate sprite info.
	// Each SCB contains: control bytes, collision number, pointer to next SCB,
	// tile data pointer, X/Y position, stretch/tilt parameters.
	// For now, this is a no-op stub (SpriteCount = 0 in GetSpritePreviewInfo).
}
