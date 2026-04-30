#include "pch.h"
#include "Genesis/Debugger/GenesisVdpTools.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisTypes.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/MemoryDumper.h"

GenesisVdpTools::GenesisVdpTools(Debugger* debugger, Emulator* emu, GenesisConsole* console)
	: PpuTools(debugger, emu), _console(console) {
}

uint32_t GenesisVdpTools::CramToArgb(uint16_t cramColor) {
	uint8_t r3 = (uint8_t)((cramColor >> 1) & 0x07);
	uint8_t g3 = (uint8_t)((cramColor >> 5) & 0x07);
	uint8_t b3 = (uint8_t)((cramColor >> 9) & 0x07);

	uint8_t r = (uint8_t)((r3 << 5) | (r3 << 2) | (r3 >> 1));
	uint8_t g = (uint8_t)((g3 << 5) | (g3 << 2) | (g3 >> 1));
	uint8_t b = (uint8_t)((b3 << 5) | (b3 << 2) | (b3 >> 1));

	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

uint16_t GenesisVdpTools::ArgbToCram(uint32_t color) {
	uint8_t r = (uint8_t)((color >> 16) & 0xFF);
	uint8_t g = (uint8_t)((color >> 8) & 0xFF);
	uint8_t b = (uint8_t)(color & 0xFF);

	uint8_t r3 = (uint8_t)(r >> 5);
	uint8_t g3 = (uint8_t)(g >> 5);
	uint8_t b3 = (uint8_t)(b >> 5);

	return (uint16_t)((r3 << 1) | (g3 << 5) | (b3 << 9));
}

uint32_t GenesisVdpTools::GetPlaneWidthTiles(const GenesisVdpState& state) {
	switch (state.Registers[16] & 0x03) {
		case 0: return 32;
		case 1: return 64;
		default: return 128;
	}
}

uint32_t GenesisVdpTools::GetPlaneHeightTiles(const GenesisVdpState& state) {
	switch ((state.Registers[16] >> 4) & 0x03) {
		case 0: return 32;
		case 1: return 64;
		default: return 128;
	}
}

FrameInfo GenesisVdpTools::GetTilemapSize([[maybe_unused]] GetTilemapOptions options, BaseState& baseState) {
	GenesisVdpState& state = (GenesisVdpState&)baseState;
	uint32_t width = GetPlaneWidthTiles(state) * 8;
	uint32_t height = GetPlaneHeightTiles(state) * 8;
	return { width, height };
}

DebugTilemapInfo GenesisVdpTools::GetTilemap(GetTilemapOptions options, BaseState& baseState, [[maybe_unused]] BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) {
	DebugTilemapInfo result = {};
	GenesisVdpState& state = (GenesisVdpState&)baseState;

	result.Bpp = 4;
	result.Format = TileFormat::Bpp4;
	result.TileWidth = 8;
	result.TileHeight = 8;
	result.ColumnCount = GetPlaneWidthTiles(state);
	result.RowCount = GetPlaneHeightTiles(state);
	result.ScrollWidth = result.ColumnCount * 8;
	result.ScrollHeight = result.RowCount * 8;
	result.ScrollX = 0;
	result.ScrollY = 0;

	uint32_t layerBase = options.Layer == 1 ? (uint32_t)((state.Registers[4] & 0x07) << 13) : (uint32_t)((state.Registers[2] & 0x38) << 10);
	result.TilemapAddress = layerBase;
	result.TilesetAddress = 0;

	if (!vram || !outBuffer || !palette) {
		return result;
	}

	uint32_t planeWidth = result.ColumnCount;
	uint32_t planeHeight = result.RowCount;

	for (uint32_t row = 0; row < planeHeight; row++) {
		for (uint32_t col = 0; col < planeWidth; col++) {
			uint32_t entryAddr = (layerBase + ((row * planeWidth + col) * 2)) & 0xFFFF;
			uint16_t entry = (uint16_t)(vram[entryAddr] | (vram[(entryAddr + 1) & 0xFFFF] << 8));

			uint16_t tileIndex = entry & 0x07FF;
			uint8_t paletteIndex = (uint8_t)((entry >> 13) & 0x03);
			bool vFlip = (entry & 0x1000) != 0;
			bool hFlip = (entry & 0x0800) != 0;

			for (uint32_t y = 0; y < 8; y++) {
				uint32_t tileY = vFlip ? (7 - y) : y;
				uint32_t rowStart = tileIndex * 32 + tileY * 4;
				for (uint32_t x = 0; x < 8; x++) {
					uint32_t tileX = hFlip ? (7 - x) : x;
					uint8_t colorIndex = GetTilePixelColor<TileFormat::Bpp4>(vram, 0xFFFF, rowStart, (uint8_t)tileX);
					uint32_t paletteEntry = (paletteIndex * 16 + colorIndex) & 0x3F;
					uint32_t outX = col * 8 + x;
					uint32_t outY = row * 8 + y;
					outBuffer[outY * (planeWidth * 8) + outX] = palette[paletteEntry];
				}
			}
		}
	}

	return result;
}

DebugTilemapTileInfo GenesisVdpTools::GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& baseState, [[maybe_unused]] BaseState& ppuToolsState) {
	DebugTilemapTileInfo info = {};
	GenesisVdpState& state = (GenesisVdpState&)baseState;

	uint32_t planeWidthTiles = GetPlaneWidthTiles(state);
	uint32_t planeHeightTiles = GetPlaneHeightTiles(state);
	if (planeWidthTiles == 0 || planeHeightTiles == 0) {
		return info;
	}

	uint32_t col = x / 8;
	uint32_t row = y / 8;
	if (col >= planeWidthTiles || row >= planeHeightTiles || !vram) {
		return info;
	}

	uint32_t layerBase = options.Layer == 1 ? (uint32_t)((state.Registers[4] & 0x07) << 13) : (uint32_t)((state.Registers[2] & 0x38) << 10);
	uint32_t entryAddr = (layerBase + ((row * planeWidthTiles + col) * 2)) & 0xFFFF;
	uint16_t entry = (uint16_t)(vram[entryAddr] | (vram[(entryAddr + 1) & 0xFFFF] << 8));

	uint16_t tileIndex = entry & 0x07FF;
	uint8_t paletteIndex = (uint8_t)((entry >> 13) & 0x03);
	bool vFlip = (entry & 0x1000) != 0;
	bool hFlip = (entry & 0x0800) != 0;

	uint32_t px = x & 0x07;
	uint32_t py = y & 0x07;
	uint32_t tileX = hFlip ? (7 - px) : px;
	uint32_t tileY = vFlip ? (7 - py) : py;
	uint32_t rowStart = tileIndex * 32 + tileY * 4;
	uint8_t pixelData = GetTilePixelColor<TileFormat::Bpp4>(vram, 0xFFFF, rowStart, (uint8_t)tileX);

	info.Row = (int32_t)row;
	info.Column = (int32_t)col;
	info.Width = 8;
	info.Height = 8;
	info.TileMapAddress = (int32_t)entryAddr;
	info.AttributeData = (int16_t)entry;
	info.TileIndex = (int32_t)tileIndex;
	info.TileAddress = (int32_t)(tileIndex * 32);
	info.PaletteIndex = (int32_t)paletteIndex;
	info.BasePaletteIndex = (int32_t)(paletteIndex * 16);
	info.PaletteAddress = (int32_t)(paletteIndex * 16 * 2);
	info.PixelData = pixelData;
	info.HorizontalMirroring = hFlip ? NullableBoolean::True : NullableBoolean::False;
	info.VerticalMirroring = vFlip ? NullableBoolean::True : NullableBoolean::False;
	info.HighPriority = (entry & 0x8000) ? NullableBoolean::True : NullableBoolean::False;

	return info;
}

DebugSpritePreviewInfo GenesisVdpTools::GetSpritePreviewInfo([[maybe_unused]] GetSpritePreviewOptions options, BaseState& state, [[maybe_unused]] BaseState& ppuToolsState) {
	DebugSpritePreviewInfo info = {};
	GenesisVdpState& vdpState = (GenesisVdpState&)state;

	bool h40 = (vdpState.Registers[12] & 0x01) != 0;
	bool v30 = (vdpState.Registers[1] & 0x08) != 0;

	info.Width = h40 ? 320 : 256;
	info.Height = v30 ? 240 : 224;
	info.SpriteCount = h40 ? 80 : 64;
	info.CoordOffsetX = 0;
	info.CoordOffsetY = 0;
	info.VisibleX = 0;
	info.VisibleY = 0;
	info.VisibleWidth = info.Width;
	info.VisibleHeight = info.Height;
	info.WrapBottomToTop = false;
	info.WrapRightToLeft = false;

	return info;
}

void GenesisVdpTools::GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, [[maybe_unused]] uint8_t* vram, [[maybe_unused]] uint8_t* oamRam, [[maybe_unused]] uint32_t* palette, DebugSpriteInfo outBuffer[], [[maybe_unused]] uint32_t* spritePreviews, [[maybe_unused]] uint32_t* screenPreview) {
	DebugSpritePreviewInfo preview = GetSpritePreviewInfo(options, baseState, ppuToolsState);
	for (uint32_t i = 0; i < preview.SpriteCount; i++) {
		outBuffer[i].Init();
		outBuffer[i].SpriteIndex = (int16_t)i;
		outBuffer[i].Visibility = SpriteVisibility::Disabled;
	}
}

DebugPaletteInfo GenesisVdpTools::GetPaletteInfo([[maybe_unused]] GetPaletteInfoOptions options) {
	DebugPaletteInfo info = {};
	info.ColorCount = 64;
	info.BgColorCount = 64;
	info.SpriteColorCount = 0;
	info.SpritePaletteOffset = 0;
	info.ColorsPerPalette = 16;
	info.HasMemType = true;
	info.PaletteMemType = MemoryType::GenesisPaletteRam;
	info.PaletteMemOffset = 0;
	info.RawFormat = RawPaletteFormat::Rgb333;

	uint8_t* paletteRam = _debugger->GetMemoryDumper()->GetMemoryBuffer(MemoryType::GenesisPaletteRam);
	if (!paletteRam) {
		for (uint32_t i = 0; i < info.ColorCount; i++) {
			info.RawPalette[i] = 0;
			info.RgbPalette[i] = 0xFF000000;
		}
		return info;
	}

	for (uint32_t i = 0; i < info.ColorCount; i++) {
		uint16_t raw = (uint16_t)(paletteRam[i * 2] | (paletteRam[i * 2 + 1] << 8));
		info.RawPalette[i] = raw;
		info.RgbPalette[i] = CramToArgb(raw);
	}

	return info;
}

void GenesisVdpTools::SetPaletteColor(int32_t colorIndex, uint32_t color) {
	if (colorIndex < 0 || colorIndex >= 64) {
		return;
	}

	uint16_t cram = ArgbToCram(color);
	uint32_t offset = (uint32_t)colorIndex * 2;
	_debugger->GetMemoryDumper()->SetMemoryValue(MemoryType::GenesisPaletteRam, offset, (uint8_t)(cram & 0xFF));
	_debugger->GetMemoryDumper()->SetMemoryValue(MemoryType::GenesisPaletteRam, offset + 1, (uint8_t)(cram >> 8));
}
