#pragma once
#include "pch.h"
#include "Debugger/PpuTools.h"
#include "Genesis/GenesisTypes.h"

class GenesisConsole;

class GenesisVdpTools final : public PpuTools {
private:
	GenesisConsole* _console = nullptr;

	static uint32_t CramToArgb(uint16_t cramColor);
	static uint16_t ArgbToCram(uint32_t color);
	static uint32_t GetPlaneWidthTiles(const GenesisVdpState& state);
	static uint32_t GetPlaneHeightTiles(const GenesisVdpState& state);

public:
	GenesisVdpTools(Debugger* debugger, Emulator* emu, GenesisConsole* console);

	DebugTilemapInfo GetTilemap(GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) override;
	FrameInfo GetTilemapSize(GetTilemapOptions options, BaseState& state) override;
	DebugTilemapTileInfo GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState) override;

	DebugSpritePreviewInfo GetSpritePreviewInfo(GetSpritePreviewOptions options, BaseState& state, BaseState& ppuToolsState) override;
	void GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) override;

	DebugPaletteInfo GetPaletteInfo(GetPaletteInfoOptions options) override;
	void SetPaletteColor(int32_t colorIndex, uint32_t color) override;
};
