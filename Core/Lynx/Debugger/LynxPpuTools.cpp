#include "pch.h"
#include "Lynx/Debugger/LynxPpuTools.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxSuzy.h"
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

	// Walk SCB chain to count sprites
	uint8_t* ram = _console->GetWorkRam();
	uint16_t scbAddr = _console->GetSuzy()->GetState().SCBAddress;
	uint32_t count = 0;

	while ((scbAddr >> 8) != 0 && count < 256) {
		count++;
		// SCBNEXT is at SCB offset 3-4 (after CTL0, CTL1, COLL)
		scbAddr = ram[(scbAddr + 3) & 0xFFFF] | (ram[(scbAddr + 4) & 0xFFFF] << 8);
	}

	info.SpriteCount = count;
	return info;
}

void LynxPpuTools::GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) {
	LynxSuzyState& suzyState = _console->GetSuzy()->GetState();
	uint16_t scbAddr = suzyState.SCBAddress;

	// Persistent values across sprites (reused when reload flags skip loading)
	int16_t persistHpos = 0;
	int16_t persistVpos = 0;
	uint16_t persistHsize = 0x0100; // 1.0 in 8.8 fixed-point
	uint16_t persistVsize = 0x0100;
	int16_t persistStretch = 0;
	int16_t persistTilt = 0;
	uint8_t penIndex[16];
	for (int i = 0; i < 16; i++) {
		penIndex[i] = (uint8_t)i; // Identity mapping
	}

	int spriteIndex = 0;

	while ((scbAddr >> 8) != 0 && spriteIndex < 256) {
		DebugSpriteInfo& sprite = outBuffer[spriteIndex];
		sprite.Init();
		uint32_t* spritePreview = spritePreviews + (spriteIndex * _spritePreviewSize);

		// SCB Layout (matching Handy/hardware):
		// Offset 0: SPRCTL0, Offset 1: SPRCTL1, Offset 2: SPRCOLL
		// Offset 3-4: SCBNEXT, Offset 5-6: SPRDLINE
		// Offset 7-8: HPOS, Offset 9-10: VPOS
		// Offset 11+: Variable (SIZE, STRETCH, TILT, PALETTE based on reload flags)
		uint8_t sprCtl0 = vram[scbAddr & 0xFFFF];             // Offset 0
		uint8_t sprCtl1 = vram[(scbAddr + 1) & 0xFFFF];       // Offset 1

		// BPP from SPRCTL0 bits 7:6 (0=1bpp, 1=2bpp, 2=3bpp, 3=4bpp)
		int bpp = ((sprCtl0 >> 6) & 0x03) + 1;

		// Sprite type from SPRCTL0 bits 2:0
		LynxSpriteType spriteType = static_cast<LynxSpriteType>(sprCtl0 & 0x07);

		// Flip flags from SPRCTL0 bits 5:4
		bool hFlip = (sprCtl0 & 0x20) != 0;
		bool vFlip = (sprCtl0 & 0x10) != 0;

		// SPRCTL1 decoding (Handy/hardware bit layout):
		// Bit 2: skip this sprite
		// Bit 3: ReloadPalette (0 = reload from SCB)
		// Bits 5:4: ReloadDepth (0-3)
		// Bit 7: Literal mode
		bool skipSprite = (sprCtl1 & 0x04) != 0;
		bool reloadPalette = (sprCtl1 & 0x08) == 0;   // Bit 3: 0 means reload
		int reloadDepth    = (sprCtl1 >> 4) & 0x03;    // Bits 5:4
		bool literalMode   = (sprCtl1 & 0x80) != 0;    // Bit 7

		// Data pointer at SCB offset 5-6 (always loaded)
		uint16_t dataAddr = vram[(scbAddr + 5) & 0xFFFF] | (vram[(scbAddr + 6) & 0xFFFF] << 8);

		// Position at offset 7-8, 9-10 (always loaded)
		persistHpos = (int16_t)(vram[(scbAddr + 7) & 0xFFFF] | (vram[(scbAddr + 8) & 0xFFFF] << 8));
		persistVpos = (int16_t)(vram[(scbAddr + 9) & 0xFFFF] | (vram[(scbAddr + 10) & 0xFFFF] << 8));

		// Variable-length fields start at offset 11
		int scbOffset = 11;

		// Load size if ReloadDepth >= 1
		if (reloadDepth >= 1) {
			persistHsize = vram[(scbAddr + scbOffset) & 0xFFFF] | (vram[(scbAddr + scbOffset + 1) & 0xFFFF] << 8);
			persistVsize = vram[(scbAddr + scbOffset + 2) & 0xFFFF] | (vram[(scbAddr + scbOffset + 3) & 0xFFFF] << 8);
			scbOffset += 4;
		}
		// Load stretch if ReloadDepth >= 2
		if (reloadDepth >= 2) {
			persistStretch = (int16_t)(vram[(scbAddr + scbOffset) & 0xFFFF] | (vram[(scbAddr + scbOffset + 1) & 0xFFFF] << 8));
			scbOffset += 2;
		}
		// Load tilt if ReloadDepth >= 3
		if (reloadDepth >= 3) {
			persistTilt = (int16_t)(vram[(scbAddr + scbOffset) & 0xFFFF] | (vram[(scbAddr + scbOffset + 1) & 0xFFFF] << 8));
			scbOffset += 2;
		}
		// Load palette remap if ReloadPalette (bit 3 = 0)
		if (reloadPalette) {
			for (int i = 0; i < 8; i++) {
				uint8_t byte = vram[(scbAddr + scbOffset + i) & 0xFFFF];
				penIndex[i * 2] = byte >> 4;
				penIndex[i * 2 + 1] = byte & 0x0f;
			}
		}

		// Populate sprite metadata
		sprite.SpriteIndex = (int16_t)spriteIndex;
		sprite.X = persistHpos;
		sprite.Y = persistVpos;
		sprite.RawX = persistHpos;
		sprite.RawY = persistVpos;
		sprite.Bpp = (int16_t)bpp;
		sprite.TileAddress = dataAddr;
		sprite.TileIndex = dataAddr;
		sprite.PaletteAddress = -1;
		sprite.Palette = 0;
		sprite.HorizontalMirror = hFlip ? NullableBoolean::True : NullableBoolean::False;
		sprite.VerticalMirror = vFlip ? NullableBoolean::True : NullableBoolean::False;
		sprite.Format = TileFormat::Bpp4;
		sprite.TileCount = 1;
		sprite.TileAddresses[0] = dataAddr;

		// Map sprite type to priority/mode
		switch (spriteType) {
			case LynxSpriteType::Background:
				sprite.Priority = DebugSpritePriority::Background;
				sprite.Mode = DebugSpriteMode::Normal;
				break;
			case LynxSpriteType::Shadow:
			case LynxSpriteType::NormalShadow:
			case LynxSpriteType::BoundaryShadow:
			case LynxSpriteType::XorShadow:
				sprite.Priority = DebugSpritePriority::Foreground;
				sprite.Mode = DebugSpriteMode::Blending;
				break;
			default:
				sprite.Priority = DebugSpritePriority::Foreground;
				sprite.Mode = DebugSpriteMode::Normal;
				break;
		}

		// Clear sprite preview buffer
		std::fill(spritePreview, spritePreview + _spritePreviewSize, (uint32_t)0);

		if (skipSprite) {
			sprite.Visibility = SpriteVisibility::Disabled;
			sprite.Width = 0;
			sprite.Height = 0;
		} else {
			// Pass 1: Walk sprite data lines to determine dimensions
			// Each line starts with a length byte (0 = end of sprite)
			uint16_t scanAddr = dataAddr;
			int maxWidth = 0;
			int lineCount = 0;

			for (int line = 0; line < 256; line++) {
				uint8_t lineHeader = vram[scanAddr & 0xFFFF];
				scanAddr++;
				if (lineHeader == 0) {
					break;
				}
				int dataBytes = lineHeader - 1;
				if (dataBytes > 0) {
					int pixelCount = (dataBytes * 8) / bpp;
					if (pixelCount > maxWidth) {
						maxWidth = pixelCount;
					}
				}
				scanAddr += dataBytes;
				lineCount++;
			}

			// Clamp to 128×128 preview area
			int previewWidth = std::min(maxWidth, 128);
			int previewHeight = std::min(lineCount, 128);
			sprite.Width = (uint16_t)previewWidth;
			sprite.Height = (uint16_t)previewHeight;

			// Pass 2: Decode sprite pixel data and render into preview buffer
			if (previewWidth > 0 && previewHeight > 0) {
				uint16_t currentDataAddr = dataAddr;
				uint8_t bppMask = (1 << bpp) - 1;

				for (int line = 0; line < lineCount; line++) {
					uint8_t lineHeader = vram[currentDataAddr & 0xFFFF];
					currentDataAddr++;
					int dataBytes = lineHeader - 1;

					if (line < 128 && dataBytes > 0) {
						// Decode packed pixel data for this line
						uint8_t pixelBuf[512];
						int pixelCount = 0;
						int bitBuffer = 0;
						int bitsRemaining = 0;
						uint16_t readAddr = currentDataAddr;
						uint16_t lineEnd = currentDataAddr + dataBytes;

						while (readAddr < lineEnd && pixelCount < 512) {
							bitBuffer = (bitBuffer << 8) | vram[readAddr & 0xFFFF];
							readAddr++;
							bitsRemaining += 8;
							while (bitsRemaining >= bpp && pixelCount < 512) {
								bitsRemaining -= bpp;
								pixelBuf[pixelCount++] = (bitBuffer >> bitsRemaining) & bppMask;
							}
						}

						// Render decoded pixels into preview buffer (stride = previewWidth)
						int renderWidth = std::min(pixelCount, 128);
						for (int px = 0; px < renderWidth; px++) {
							uint8_t penRaw = pixelBuf[px];
							if (penRaw != 0) {
								uint8_t penMapped = penIndex[penRaw & 0x0f];
								spritePreview[line * previewWidth + px] = palette[penMapped];
							}
						}
					}

					currentDataAddr += dataBytes;
				}
			}

			// Determine visibility based on screen bounds
			bool onScreen = (persistHpos + previewWidth > 0) &&
				(persistHpos < (int16_t)LynxConstants::ScreenWidth) &&
				(persistVpos + previewHeight > 0) &&
				(persistVpos < (int16_t)LynxConstants::ScreenHeight);
			sprite.Visibility = onScreen ? SpriteVisibility::Visible : SpriteVisibility::Offscreen;
		}

		spriteIndex++;
		// SCBNEXT at SCB offset 3-4
		scbAddr = vram[(scbAddr + 3) & 0xFFFF] | (vram[(scbAddr + 4) & 0xFFFF] << 8);
	}

	// Composite all sprites onto the screen preview
	GetSpritePreview(options, baseState, outBuffer, spriteIndex, spritePreviews, palette, screenPreview);
}

void LynxPpuTools::GetSpritePreview(GetSpritePreviewOptions options, BaseState& state,
	DebugSpriteInfo* sprites, uint32_t spriteCount,
	uint32_t* spritePreviews, uint32_t* palette, uint32_t* outBuffer) {

	uint32_t width = LynxConstants::ScreenWidth;
	uint32_t height = LynxConstants::ScreenHeight;
	uint32_t bgColor = GetSpriteBackgroundColor(options.Background, palette, false);

	std::fill(outBuffer, outBuffer + width * height, bgColor);

	// Draw sprites in reverse order (back-to-front).
	// Lynx renders front-to-back (first sprite = highest priority), so to
	// reproduce the visual with standard "paint on top" blitting, draw the
	// last sprite first and the first sprite last (on top).
	for (int i = (int)spriteCount - 1; i >= 0; i--) {
		DebugSpriteInfo& sprite = sprites[i];
		if (sprite.Visibility == SpriteVisibility::Disabled || sprite.Width == 0 || sprite.Height == 0) {
			continue;
		}

		uint32_t* spritePreview = spritePreviews + i * _spritePreviewSize;
		bool hFlip = sprite.HorizontalMirror == NullableBoolean::True;
		bool vFlip = sprite.VerticalMirror == NullableBoolean::True;

		for (int y = 0; y < sprite.Height; y++) {
			for (int x = 0; x < sprite.Width; x++) {
				uint32_t color = spritePreview[y * sprite.Width + x];
				if (color != 0) {
					int screenX = sprite.X + (hFlip ? -x : x);
					int screenY = sprite.Y + (vFlip ? -y : y);
					if (screenX >= 0 && screenX < (int)width &&
						screenY >= 0 && screenY < (int)height) {
						outBuffer[screenY * width + screenX] = color;
					}
				}
			}
		}
	}
}
