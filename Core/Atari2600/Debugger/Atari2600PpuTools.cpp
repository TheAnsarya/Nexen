#include "pch.h"
#include "Atari2600/Debugger/Atari2600PpuTools.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600Types.h"
#include "Debugger/Debugger.h"
#include "Debugger/MemoryDumper.h"
#include "Shared/Emulator.h"

// Standard Stella NTSC TIA palette — 128 colors as 0xAARRGGBB
// 16 hues x 8 luminance levels, indexed by (tiaColor >> 1)
static constexpr uint32_t Atari2600NtscPalette[128] = {
	// Hue 0  — Gray
	0xff000000, 0xff404040, 0xff6c6c6c, 0xff909090, 0xffb0b0b0, 0xffc8c8c8, 0xffdcdcdc, 0xfff4f4f4,
	// Hue 1  — Gold
	0xff444400, 0xff646410, 0xff848424, 0xffa0a034, 0xffb8b840, 0xffd0d050, 0xffe8e85c, 0xfffcfc68,
	// Hue 2  — Orange
	0xff702800, 0xff844414, 0xff985c28, 0xffac783c, 0xffbc8c4c, 0xffcca05c, 0xffdcb468, 0xffecc878,
	// Hue 3  — Red-Orange
	0xff841800, 0xff983418, 0xffac5030, 0xffc06848, 0xffd0805c, 0xffe09470, 0xffeca880, 0xfffcbc94,
	// Hue 4  — Red
	0xff880000, 0xff9c2020, 0xffb03c3c, 0xffc05858, 0xffd07070, 0xffe08888, 0xffeca0a0, 0xfffcb4b4,
	// Hue 5  — Purple-Red
	0xff78005c, 0xff8c2074, 0xffa03c88, 0xffb0589c, 0xffc070b0, 0xffd084c0, 0xffdc9cd0, 0xffecb0e0,
	// Hue 6  — Purple
	0xff480078, 0xff602090, 0xff783ca4, 0xff8c58b8, 0xffa070cc, 0xffb484dc, 0xffc49cec, 0xffd4b0fc,
	// Hue 7  — Blue-Purple
	0xff140084, 0xff302098, 0xff4c3cac, 0xff6858c0, 0xff7c70d0, 0xff9488e0, 0xffa8a0ec, 0xffbcb4fc,
	// Hue 8  — Blue
	0xff000088, 0xff1c209c, 0xff3840b0, 0xff505cc0, 0xff6874d0, 0xff7c8ce0, 0xff90a4ec, 0xffa4b8fc,
	// Hue 9  — Light Blue
	0xff00187c, 0xff1c3890, 0xff3854a8, 0xff5070bc, 0xff689cc0, 0xff7c9cdc, 0xff90b4ec, 0xffa4c8fc,
	// Hue 10 — Cyan
	0xff002c5c, 0xff1c4c78, 0xff386890, 0xff5084ac, 0xff689cc0, 0xff7cb4d4, 0xff90cce8, 0xffa4e0fc,
	// Hue 11 — Teal
	0xff003c2c, 0xff1c5c48, 0xff387c64, 0xff509c80, 0xff68b494, 0xff7cd0ac, 0xff90e4c0, 0xffa4fcd4,
	// Hue 12 — Green
	0xff003c00, 0xff205c20, 0xff407c40, 0xff5c9c5c, 0xff74b474, 0xff8cd08c, 0xffa4e4a4, 0xffb8fcb8,
	// Hue 13 — Yellow-Green
	0xff143800, 0xff345c1c, 0xff507c38, 0xff6c9850, 0xff84b468, 0xff9ccc7c, 0xffb4e490, 0xffc8fca4,
	// Hue 14 — Yellow-Green (warm)
	0xff2c3000, 0xff4c501c, 0xff687034, 0xff848c4c, 0xff9ca864, 0xffb4c078, 0xffccd488, 0xffe0ec9c,
	// Hue 15 — Dark Yellow
	0xff442800, 0xff644818, 0xff846830, 0xffa08444, 0xffb89c58, 0xffd0b46c, 0xffe8cc7c, 0xfffce08c,
};

Atari2600PpuTools::Atari2600PpuTools(Debugger* debugger, Emulator* emu, Atari2600Console* console)
	: PpuTools(debugger, emu), _console(console) {
}

DebugPaletteInfo Atari2600PpuTools::GetPaletteInfo(GetPaletteInfoOptions options) {
	DebugPaletteInfo info = {};

	// Atari 2600 TIA has 128 fixed NTSC colors (16 hues x 8 luminances)
	// No separate BG/sprite palettes — all objects use the same master palette
	info.RawFormat = RawPaletteFormat::Indexed;
	info.ColorCount = 128;
	info.BgColorCount = 128;
	info.SpriteColorCount = 0;
	info.SpritePaletteOffset = 0;
	info.ColorsPerPalette = 8; // 8 luminance levels per hue
	info.HasMemType = false;

	for (uint32_t i = 0; i < 128; i++) {
		info.RawPalette[i] = i;
		info.RgbPalette[i] = Atari2600NtscPalette[i];
	}

	return info;
}

void Atari2600PpuTools::SetPaletteColor(int32_t colorIndex, uint32_t color) {
	// Atari 2600 palette is fixed in hardware (NTSC color generation) — not editable
	// No-op: the TIA generates colors from the NTSC signal, there's no palette RAM
}

DebugTilemapInfo Atari2600PpuTools::GetTilemap(GetTilemapOptions options, BaseState& state, BaseState& ppuToolsState, uint8_t* vram, uint32_t* palette, uint32_t* outBuffer) {
	// Atari 2600 has no tilemap hardware — all rendering is scanline-based
	DebugTilemapInfo info = {};
	return info;
}

FrameInfo Atari2600PpuTools::GetTilemapSize(GetTilemapOptions options, BaseState& state) {
	return { 0, 0 };
}

DebugTilemapTileInfo Atari2600PpuTools::GetTilemapTileInfo(uint32_t x, uint32_t y, uint8_t* vram, GetTilemapOptions options, BaseState& baseState, BaseState& ppuToolsState) {
	return {};
}

DebugSpritePreviewInfo Atari2600PpuTools::GetSpritePreviewInfo(GetSpritePreviewOptions options, BaseState& state, BaseState& ppuToolsState) {
	// Atari 2600 has player/missile/ball objects but they are not traditional sprites
	DebugSpritePreviewInfo info = {};
	info.Width = 0;
	info.Height = 0;
	info.SpriteCount = 0;
	return info;
}

void Atari2600PpuTools::GetSpriteList(GetSpritePreviewOptions options, BaseState& baseState, BaseState& ppuToolsState, uint8_t* vram, uint8_t* oamRam, uint32_t* palette, DebugSpriteInfo outBuffer[], uint32_t* spritePreviews, uint32_t* screenPreview) {
	// No traditional sprite hardware on Atari 2600
}
