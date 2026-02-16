#pragma once
#include "pch.h"
#include "Debugger/PpuTools.h"

class Debugger;
class Emulator;
class LynxConsole;

/// <summary>
/// Atari Lynx PPU debugging tools — palette viewer, sprite viewer, tilemap viewer.
///
/// The Lynx has no dedicated PPU. Mikey handles display DMA and palette,
/// while Suzy handles the sprite engine. The display is a 160x102 linear
/// framebuffer in work RAM with 4bpp packed pixels.
///
/// TODO items for full implementation:
/// - Sprite list: Walk the SCB linked list and display each sprite's properties
/// - Tile viewer: Decode sprite pixel data from cart ROM as tiles
/// - Tilemap: The Lynx has no tilemap hardware; could show the framebuffer contents
/// </summary>
class LynxPpuTools final : public PpuTools {
private:
	LynxConsole* _console = nullptr;

public:
	LynxPpuTools(Debugger* debugger, Emulator* emu, LynxConsole* console);

	DebugTilemapInfo GetTilemap(GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) override;
	FrameInfo GetTilemapSize(GetTilemapOptions options, BaseState& state) override;
	DebugTilemapTileInfo GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& baseState, BaseState& ppuToolsState) override;

	DebugSpritePreviewInfo GetSpritePreviewInfo(GetSpritePreviewOptions options, BaseState& state, BaseState& ppuToolsState) override;
	void GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) override;

	DebugPaletteInfo GetPaletteInfo(GetPaletteInfoOptions options) override;
	void SetPaletteColor(int32_t colorIndex, uint32_t color) override;
};
