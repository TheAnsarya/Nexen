#include "pch.h"
#include "Genesis/Debugger/GenesisVdpTools.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisTypes.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/MemoryDumper.h"

namespace {
	uint16_t GetSpriteBaseAddress(uint8_t reg5, bool h40) {
		return h40 ? (uint16_t)(reg5 & 0x7E) << 9 : (uint16_t)(reg5 & 0x7F) << 9;
	}

	uint16_t GetHScrollBase(uint8_t reg13) {
		return (uint16_t)(reg13 & 0x3F) << 10;
	}

	uint16_t GetHScroll(const uint8_t* vram, uint8_t reg11, uint16_t hScrollBase, uint16_t line, bool planeA) {
		uint32_t entryOffset = 0;
		switch (reg11 & 0x03) {
			case 0x01:
				entryOffset = 0;
				break;
			case 0x02:
				entryOffset = (line & ~7u) * 4u;
				break;
			case 0x03:
				entryOffset = (uint32_t)line * 4u;
				break;
			default:
				entryOffset = 0;
				break;
		}

		uint16_t byteAddr = (uint16_t)((hScrollBase + entryOffset) & 0xFFFFu);
		uint16_t scrollA = ((uint16_t)vram[byteAddr] << 8) | vram[(byteAddr + 1u) & 0xFFFFu];
		uint16_t scrollB = ((uint16_t)vram[(byteAddr + 2u) & 0xFFFFu] << 8) | vram[(byteAddr + 3u) & 0xFFFFu];
		return planeA ? (scrollA & 0x03FFu) : (scrollB & 0x03FFu);
	}

	uint16_t GetVScroll(const uint16_t* vsram, uint8_t reg11, uint16_t tileCol2, bool planeA) {
		uint8_t vsramIdx;
		if ((reg11 & 0x04u) != 0) {
			uint8_t base = (uint8_t)((tileCol2 & 0x1Fu) * 2u);
			vsramIdx = planeA ? base : (base + 1u);
		} else {
			vsramIdx = planeA ? 0u : 1u;
		}

		return (uint16_t)(vsram[vsramIdx & 0x27u] & 0x03FFu);
	}

	struct DebugGenesisLineSprite {
		uint16_t Tile = 0;
		uint16_t RawX = 0;
		int16_t X = 0;
		uint8_t Palette = 0;
		uint8_t VertCells = 1;
		uint8_t HorizCells = 1;
		uint8_t CellRow = 0;
		uint8_t PixRow = 0;
		bool HFlip = false;
		bool VFlip = false;
	};

	struct DebugGenesisLineSpriteCell {
		uint16_t Tile = 0;
		int16_t X = 0;
		uint8_t Palette = 0;
		uint8_t VertCells = 1;
		uint8_t ScreenCellCol = 0;
		uint8_t PatternCellOffsetX = 0;
		uint8_t PatternCellOffsetY = 0;
		uint8_t PixRow = 0;
		bool HFlip = false;
		bool VFlip = false;
	};

	uint8_t GetSpriteTilePixel(const uint8_t* vram, uint32_t tileBase, uint8_t row, uint8_t col) {
		uint32_t byteAddr = (tileBase + (uint32_t)row * 4 + (col >> 1)) & 0xFFFF;
		uint8_t value = vram[byteAddr];
		return (col & 1) != 0 ? (value & 0x0F) : (value >> 4);
	}
}

GenesisVdpTools::GenesisVdpTools(Debugger* debugger, Emulator* emu, GenesisConsole* console)
	: PpuTools(debugger, emu), _console(console) {
}

uint32_t GenesisVdpTools::CramToArgb(uint16_t cramColor) {
	static constexpr uint8_t kGenesisDacLevels[15] = {
		0, 27, 49, 71, 87, 103, 119, 130, 146, 157, 174, 190, 206, 228, 255
	};

	uint8_t r3 = (uint8_t)((cramColor >> 1) & 0x07);
	uint8_t g3 = (uint8_t)((cramColor >> 5) & 0x07);
	uint8_t b3 = (uint8_t)((cramColor >> 9) & 0x07);

	uint8_t r = kGenesisDacLevels[r3 << 1];
	uint8_t g = kGenesisDacLevels[g3 << 1];
	uint8_t b = kGenesisDacLevels[b3 << 1];

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
	bool planeA = options.Layer == 0;

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

	uint8_t reg11 = state.Registers[11];
	uint8_t reg13 = state.Registers[13];
	uint16_t hScrollBase = GetHScrollBase(reg13);
	uint16_t visibleHeight = (state.Registers[1] & 0x08) ? 240u : 224u;
	uint16_t activeLine = (uint16_t)std::min<uint16_t>(state.VCounter, (uint16_t)(visibleHeight - 1u));

	result.ScrollX = GetHScroll(vram, reg11, hScrollBase, activeLine, planeA);
	uint16_t vScrollColumn2 = (uint16_t)((result.ScrollX >> 4) & 0x1Fu);
	result.ScrollY = GetVScroll(state.Vsram, reg11, vScrollColumn2, planeA);

	uint32_t planeWidth = result.ColumnCount;
	uint32_t planeHeight = result.RowCount;

	for (uint32_t row = 0; row < planeHeight; row++) {
		for (uint32_t col = 0; col < planeWidth; col++) {
			uint32_t entryAddr = (layerBase + ((row * planeWidth + col) * 2)) & 0xFFFF;
			uint16_t entry = (uint16_t)((vram[entryAddr] << 8) | vram[(entryAddr + 1) & 0xFFFF]);

			uint16_t tileIndex = entry & 0x07FF;
			uint8_t paletteIndex = (uint8_t)((entry >> 13) & 0x03);
			bool vFlip = (entry & 0x1000) != 0;
			bool hFlip = (entry & 0x0800) != 0;

			for (uint32_t y = 0; y < 8; y++) {
				uint32_t tileY = vFlip ? (7 - y) : y;
				for (uint32_t x = 0; x < 8; x++) {
					uint32_t tileX = hFlip ? (7 - x) : x;
					uint8_t colorIndex = GetSpriteTilePixel(vram, (uint32_t)tileIndex * 32u, (uint8_t)tileY, (uint8_t)tileX);
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

	if (!vram) {
		return info;
	}

	bool planeA = options.Layer == 0;
	uint8_t reg11 = state.Registers[11];
	uint8_t reg13 = state.Registers[13];
	uint16_t hScrollBase = GetHScrollBase(reg13);
	uint16_t visibleHeight = (state.Registers[1] & 0x08) ? 240u : 224u;
	uint16_t activeLine = (uint16_t)std::min<uint16_t>(state.VCounter, (uint16_t)(visibleHeight - 1u));
	uint16_t sampledLine = (uint16_t)std::min<uint32_t>(y, (uint32_t)(visibleHeight - 1u));
	uint16_t hScrollLine = activeLine;
	uint8_t hScrollMode = reg11 & 0x03u;
	if (hScrollMode == 0x02u || hScrollMode == 0x03u) {
		hScrollLine = sampledLine;
	}
	uint16_t scrollX = GetHScroll(vram, reg11, hScrollBase, hScrollLine, planeA);

	uint32_t planeWidthPx = planeWidthTiles * 8u;
	uint32_t planeHeightPx = planeHeightTiles * 8u;
	uint32_t adjustedX = (x + scrollX) % planeWidthPx;
	uint16_t vScrollColumn2 = (uint16_t)((adjustedX >> 4) & 0x1Fu);
	uint16_t scrollY = GetVScroll(state.Vsram, reg11, vScrollColumn2, planeA);
	uint32_t adjustedY = (y + scrollY) % planeHeightPx;

	uint32_t col = adjustedX / 8u;
	uint32_t row = adjustedY / 8u;

	uint32_t layerBase = options.Layer == 1 ? (uint32_t)((state.Registers[4] & 0x07) << 13) : (uint32_t)((state.Registers[2] & 0x38) << 10);
	uint32_t entryAddr = (layerBase + ((row * planeWidthTiles + col) * 2)) & 0xFFFF;
	uint16_t entry = (uint16_t)((vram[entryAddr] << 8) | vram[(entryAddr + 1) & 0xFFFF]);

	uint16_t tileIndex = entry & 0x07FF;
	uint8_t paletteIndex = (uint8_t)((entry >> 13) & 0x03);
	bool vFlip = (entry & 0x1000) != 0;
	bool hFlip = (entry & 0x0800) != 0;

	uint32_t px = adjustedX & 0x07;
	uint32_t py = adjustedY & 0x07;
	uint32_t tileX = hFlip ? (7 - px) : px;
	uint32_t tileY = vFlip ? (7 - py) : py;
	uint8_t pixelData = GetSpriteTilePixel(vram, (uint32_t)tileIndex * 32u, (uint8_t)tileY, (uint8_t)tileX);

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

	info.Width = 512;
	info.Height = 512;
	info.SpriteCount = h40 ? 80 : 64;
	info.CoordOffsetX = 128;
	info.CoordOffsetY = 128;
	info.VisibleX = 128;
	info.VisibleY = 128;
	info.VisibleWidth = info.Width;
	info.VisibleHeight = v30 ? 240 : 224;
	info.WrapBottomToTop = false;
	info.WrapRightToLeft = false;
	if (h40) {
		info.VisibleWidth = 320;
	} else {
		info.VisibleWidth = 256;
	}

	return info;
}

void GenesisVdpTools::GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, [[maybe_unused]] uint8_t* vram, [[maybe_unused]] uint8_t* oamRam, [[maybe_unused]] uint32_t* palette, DebugSpriteInfo outBuffer[], [[maybe_unused]] uint32_t* spritePreviews, [[maybe_unused]] uint32_t* screenPreview) {
	GenesisVdpState& vdpState = (GenesisVdpState&)baseState;
	DebugSpritePreviewInfo preview = GetSpritePreviewInfo(options, baseState, ppuToolsState);

	if (!vram || !palette || !outBuffer || !spritePreviews || !screenPreview) {
		for (uint32_t i = 0; i < preview.SpriteCount; i++) {
			outBuffer[i].Init();
			outBuffer[i].SpriteIndex = (int16_t)i;
			outBuffer[i].Visibility = SpriteVisibility::Disabled;
		}
		return;
	}

	bool h40 = (vdpState.Registers[12] & 0x01) != 0;
	uint16_t spriteCount = h40 ? 80u : 64u;
	uint16_t spriteBase = GetSpriteBaseAddress(vdpState.Registers[5], h40);
	uint16_t maxPerLine = h40 ? 20u : 16u;
	uint16_t maxCells = h40 ? 40u : 32u;
	uint16_t cellPixH = 8u;

	uint32_t bgColor = GetSpriteBackgroundColor(options.Background, palette, false);
	uint32_t offscreenBgColor = GetSpriteBackgroundColor(options.Background, palette, true);

	std::fill(screenPreview, screenPreview + preview.Width * preview.Height, offscreenBgColor);
	for (uint32_t y = 0; y < preview.VisibleHeight; y++) {
		uint32_t* row = screenPreview + (preview.VisibleY + y) * preview.Width + preview.VisibleX;
		std::fill(row, row + preview.VisibleWidth, bgColor);
	}

	for (uint16_t i = 0; i < spriteCount; i++) {
		DebugSpriteInfo& sprite = outBuffer[i];
		sprite.Init();
		sprite.SpriteIndex = (int16_t)i;

		uint32_t* spritePreview = spritePreviews + (i * _spritePreviewSize);
		std::fill(spritePreview, spritePreview + _spritePreviewSize, 0u);

		uint16_t entryBase = (uint16_t)(spriteBase + i * 8u);
		uint16_t w0 = ((uint16_t)vram[(entryBase + 0u) & 0xFFFFu] << 8) | vram[(entryBase + 1u) & 0xFFFFu];
		uint16_t w1 = ((uint16_t)vram[(entryBase + 2u) & 0xFFFFu] << 8) | vram[(entryBase + 3u) & 0xFFFFu];
		uint16_t w2 = ((uint16_t)vram[(entryBase + 4u) & 0xFFFFu] << 8) | vram[(entryBase + 5u) & 0xFFFFu];
		uint16_t w3 = ((uint16_t)vram[(entryBase + 6u) & 0xFFFFu] << 8) | vram[(entryBase + 7u) & 0xFFFFu];

		uint8_t vertCells = (uint8_t)(((w1 >> 8) & 0x03u) + 1u);
		uint8_t horizCells = (uint8_t)(((w1 >> 10) & 0x03u) + 1u);
		bool pri = (w2 & 0x8000u) != 0;
		uint8_t pal = (uint8_t)((w2 >> 13) & 0x03u);
		bool vflip = (w2 & 0x1000u) != 0;
		bool hflip = (w2 & 0x0800u) != 0;
		uint16_t tile = w2 & 0x07FFu;
		uint16_t rawY = w0 & 0x01FFu;
		uint16_t rawX = w3 & 0x01FFu;
		int16_t x = (int16_t)rawX - 128;
		int16_t y = (int16_t)rawY - 128;
		uint16_t width = (uint16_t)horizCells * 8u;
		uint16_t height = (uint16_t)vertCells * cellPixH;

		sprite.Format = TileFormat::Bpp4;
		sprite.Bpp = 4;
		sprite.X = x;
		sprite.Y = y;
		sprite.RawX = (int16_t)rawX;
		sprite.RawY = (int16_t)rawY;
		sprite.Width = width;
		sprite.Height = height;
		sprite.TileIndex = tile;
		sprite.TileAddress = tile * 32;
		sprite.Palette = pal;
		sprite.PaletteAddress = pal * 32;
		sprite.Priority = pri ? DebugSpritePriority::Foreground : DebugSpritePriority::Background;
		sprite.Mode = DebugSpriteMode::Normal;
		sprite.HorizontalMirror = hflip ? NullableBoolean::True : NullableBoolean::False;
		sprite.VerticalMirror = vflip ? NullableBoolean::True : NullableBoolean::False;
		sprite.UseExtendedVram = false;
		sprite.UseSecondTable = NullableBoolean::Undefined;

		bool visible = x < (int16_t)preview.VisibleWidth && y < (int16_t)preview.VisibleHeight
			&& (x + (int16_t)width) > 0 && (y + (int16_t)height) > 0;
		sprite.Visibility = visible ? SpriteVisibility::Visible : SpriteVisibility::Offscreen;

		uint32_t tileCount = 0;
		for (uint8_t cx = 0; cx < horizCells && tileCount < 64u; cx++) {
			for (uint8_t cy = 0; cy < vertCells && tileCount < 64u; cy++) {
				uint8_t patternCellX = hflip ? (uint8_t)(horizCells - 1u - cx) : cx;
				uint8_t patternCellY = vflip ? (uint8_t)(vertCells - 1u - cy) : cy;
				uint16_t tileIdx = tile + (uint16_t)(patternCellX * vertCells) + patternCellY;
				sprite.TileAddresses[tileCount++] = tileIdx * 32u;
			}
		}
		sprite.TileCount = tileCount;

		for (uint16_t py = 0; py < height; py++) {
			uint8_t cellRow = (uint8_t)(py / cellPixH);
			uint8_t pixRow = (uint8_t)(py % cellPixH);
			uint8_t patternCellY = vflip ? (uint8_t)(vertCells - 1u - cellRow) : cellRow;
			uint8_t row = vflip ? (uint8_t)(cellPixH - 1u - pixRow) : pixRow;

			for (uint16_t px = 0; px < width; px++) {
				uint8_t screenCellCol = (uint8_t)(px >> 3);
				uint8_t patternCellX = hflip ? (uint8_t)(horizCells - 1u - screenCellCol) : screenCellCol;
				uint16_t tileIdx = tile + (uint16_t)(patternCellX * vertCells) + patternCellY;
				uint32_t tileBase = (uint32_t)tileIdx * 32u;
				uint8_t col = hflip ? (uint8_t)(7u - (px & 7u)) : (uint8_t)(px & 7u);
				uint8_t color = GetSpriteTilePixel(vram, tileBase, row, col);
				if (color != 0) {
					uint32_t previewIndex = py * width + px;
					if (previewIndex < _spritePreviewSize) {
						spritePreview[previewIndex] = palette[pal * 16u + color];
					}
				}
			}
		}
	}

	bool prevLineDotOverflow = false;
	for (uint16_t line = 0; line < preview.VisibleHeight; line++) {
		DebugGenesisLineSprite lineSprites[80] = {};
		DebugGenesisLineSpriteCell lineCells[40] = {};
		uint16_t lineSpriteCount = 0;
		uint16_t lineCellCount = 0;
		bool lineDotOverflow = false;
		uint8_t idx = 0;

		for (uint16_t s = 0; s < spriteCount; s++) {
			uint16_t entryBase = (uint16_t)(spriteBase + (uint16_t)idx * 8u);
			uint16_t w0 = ((uint16_t)vram[(entryBase + 0u) & 0xFFFFu] << 8) | vram[(entryBase + 1u) & 0xFFFFu];
			uint16_t w1 = ((uint16_t)vram[(entryBase + 2u) & 0xFFFFu] << 8) | vram[(entryBase + 3u) & 0xFFFFu];
			uint16_t w2 = ((uint16_t)vram[(entryBase + 4u) & 0xFFFFu] << 8) | vram[(entryBase + 5u) & 0xFFFFu];
			uint8_t link = (uint8_t)(w1 & 0x7Fu);
			int16_t sprY = (int16_t)(w0 & 0x01FFu) - 128;
			uint8_t vertCells = (uint8_t)(((w1 >> 8) & 0x03u) + 1u);
			uint8_t horizCells = (uint8_t)(((w1 >> 10) & 0x03u) + 1u);
			uint16_t sprH = (uint16_t)vertCells * cellPixH;

			if ((int16_t)line >= sprY && (int16_t)line < (int16_t)(sprY + sprH)) {
				if (lineSpriteCount >= maxPerLine) {
					break;
				}

				bool spriteVflip = (w2 & 0x1000u) != 0;
				uint16_t sprRow = (uint16_t)((int16_t)line - sprY);
				uint8_t cellRow = (uint8_t)(sprRow / cellPixH);
				uint8_t pixRow = (uint8_t)(sprRow % cellPixH);

				DebugGenesisLineSprite& sprite = lineSprites[lineSpriteCount++];
				sprite.Tile = w2 & 0x07FFu;
				sprite.RawX = ((uint16_t)vram[(entryBase + 6u) & 0xFFFFu] << 8) | vram[(entryBase + 7u) & 0xFFFFu];
				sprite.RawX &= 0x01FFu;
				sprite.X = (int16_t)sprite.RawX - 128;
				sprite.Palette = (uint8_t)((w2 >> 13) & 0x03u);
				sprite.VertCells = vertCells;
				sprite.HorizCells = horizCells;
				sprite.CellRow = spriteVflip ? (uint8_t)(vertCells - 1u - cellRow) : cellRow;
				sprite.PixRow = pixRow;
				sprite.HFlip = (w2 & 0x0800u) != 0;
				sprite.VFlip = spriteVflip;
			}

			if (link == 0 || link >= spriteCount) {
				break;
			}
			idx = link;
		}

		bool maskActive = false;
		bool nonMaskCellEncountered = false;
		for (uint16_t i = 0; i < lineSpriteCount; i++) {
			const DebugGenesisLineSprite& sprite = lineSprites[i];
			if (sprite.RawX == 0) {
				if (nonMaskCellEncountered || prevLineDotOverflow) {
					maskActive = true;
				}
				continue;
			}

			nonMaskCellEncountered = true;
			if (maskActive) {
				continue;
			}

			for (uint8_t screenCellCol = 0; screenCellCol < sprite.HorizCells; screenCellCol++) {
				if (lineCellCount >= maxCells) {
					lineDotOverflow = true;
					break;
				}

				DebugGenesisLineSpriteCell& cell = lineCells[lineCellCount++];
				cell.Tile = sprite.Tile;
				cell.X = sprite.X;
				cell.Palette = sprite.Palette;
				cell.VertCells = sprite.VertCells;
				cell.ScreenCellCol = screenCellCol;
				cell.PatternCellOffsetX = sprite.HFlip ? (uint8_t)(sprite.HorizCells - 1u - screenCellCol) : screenCellCol;
				cell.PatternCellOffsetY = sprite.CellRow;
				cell.PixRow = sprite.PixRow;
				cell.HFlip = sprite.HFlip;
				cell.VFlip = sprite.VFlip;
			}

			if (lineDotOverflow) {
				break;
			}
		}

		prevLineDotOverflow = lineDotOverflow;
		uint32_t* outLine = screenPreview + (preview.VisibleY + line) * preview.Width + preview.VisibleX;
		for (uint16_t i = 0; i < lineCellCount; i++) {
			const DebugGenesisLineSpriteCell& cell = lineCells[i];
			uint16_t tileIdx = cell.Tile + (uint16_t)(cell.PatternCellOffsetX * cell.VertCells) + cell.PatternCellOffsetY;
			uint32_t tileBase = (uint32_t)tileIdx * 32u;
			for (uint8_t px = 0; px < 8u; px++) {
				int16_t screenX = cell.X + (int16_t)(cell.ScreenCellCol * 8u + px);
				if (screenX < 0 || screenX >= (int16_t)preview.VisibleWidth) {
					continue;
				}

				uint8_t col = cell.HFlip ? (uint8_t)(7u - px) : px;
				uint8_t row = cell.VFlip ? (uint8_t)(cellPixH - 1u - cell.PixRow) : cell.PixRow;
				uint8_t color = GetSpriteTilePixel(vram, tileBase, row, col);
				if (color != 0 && outLine[screenX] == bgColor) {
					outLine[screenX] = palette[cell.Palette * 16u + color];
				}
			}
		}
	}

	for (uint16_t i = 0; i < spriteCount; i++) {
		DebugSpriteInfo& sprite = outBuffer[i];
		uint32_t* spritePreview = spritePreviews + (i * _spritePreviewSize);
		uint32_t used = (uint32_t)sprite.Width * sprite.Height;
		used = std::min<uint32_t>(used, _spritePreviewSize);
		for (uint32_t p = 0; p < used; p++) {
			if (spritePreview[p] == 0) {
				spritePreview[p] = bgColor;
			}
		}
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
