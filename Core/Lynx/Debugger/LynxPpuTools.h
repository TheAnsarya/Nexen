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
/// Sprite viewer walks the SCB (Sprite Control Block) linked list from
/// Suzy's SCBAddress register. Each SCB entry contains control bytes,
/// position, size, stretch/tilt, palette remap, and collision data.
/// Persistent fields carry forward across sprites via reload-skip flags.
///
/// Tile viewer uses WsBpp4Packed format (packed nibbles, high-nibble-first)
/// which matches the Lynx 4bpp framebuffer layout exactly.
/// </summary>
class LynxPpuTools final : public PpuTools {
private:
	LynxConsole* _console = nullptr;

	void GetSpritePreview(GetSpritePreviewOptions options, BaseState& state,
		DebugSpriteInfo* sprites, uint32_t spriteCount,
		uint32_t* spritePreviews, uint32_t* palette, uint32_t* outBuffer);

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
