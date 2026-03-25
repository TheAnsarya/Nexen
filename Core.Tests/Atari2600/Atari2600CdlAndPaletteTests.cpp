#include "pch.h"
#include "Atari2600/Atari2600DefaultVideoFilter.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Shared/MemoryType.h"
#include "Debugger/DebugTypes.h"

namespace {

//=============================================================================
// Palette Consistency Tests
//=============================================================================

TEST(Atari2600CdlAndPaletteTests, PaletteLutHas128Entries) {
	// Video filter palette must cover all 128 TIA color indices
	EXPECT_EQ(sizeof(Atari2600DefaultVideoFilter::NtscPalette) / sizeof(uint32_t), 128u);
}

TEST(Atari2600CdlAndPaletteTests, PaletteFirstEntryIsBlack) {
	// TIA color index 0 (gray hue, minimum luminance) must be black
	EXPECT_EQ(Atari2600DefaultVideoFilter::NtscPalette[0], 0x000000u);
}

TEST(Atari2600CdlAndPaletteTests, PaletteLastEntryIsNonZero) {
	// Last palette entry should be a valid color
	EXPECT_NE(Atari2600DefaultVideoFilter::NtscPalette[127], 0x000000u);
}

TEST(Atari2600CdlAndPaletteTests, PaletteHasNoAlphaChannel) {
	// NtscPalette stores raw RGB (0x00RRGGBB), no alpha premultiplied
	for (int i = 0; i < 128; i++) {
		uint32_t color = Atari2600DefaultVideoFilter::NtscPalette[i];
		EXPECT_EQ(color & 0xff000000, 0x00000000u)
			<< "Palette entry " << i << " has alpha bits set: 0x" << std::hex << color;
	}
}

TEST(Atari2600CdlAndPaletteTests, PaletteHueRowsHaveIncreasingLuminance) {
	// Each hue row of 8 entries should have generally increasing luminance
	for (int hue = 0; hue < 16; hue++) {
		uint32_t firstColor = Atari2600DefaultVideoFilter::NtscPalette[hue * 8];
		uint32_t lastColor = Atari2600DefaultVideoFilter::NtscPalette[hue * 8 + 7];

		uint32_t firstLum = ((firstColor >> 16) & 0xff) + ((firstColor >> 8) & 0xff) + (firstColor & 0xff);
		uint32_t lastLum = ((lastColor >> 16) & 0xff) + ((lastColor >> 8) & 0xff) + (lastColor & 0xff);

		EXPECT_GT(lastLum, firstLum)
			<< "Hue row " << hue << " should have increasing luminance";
	}
}

TEST(Atari2600CdlAndPaletteTests, GrayHueRowHasEqualRGB) {
	// Hue 0 (gray) should have R == G == B for all luminance levels
	for (int lum = 0; lum < 8; lum++) {
		uint32_t color = Atari2600DefaultVideoFilter::NtscPalette[lum];
		uint8_t r = (color >> 16) & 0xff;
		uint8_t g = (color >> 8) & 0xff;
		uint8_t b = color & 0xff;
		EXPECT_EQ(r, g) << "Gray entry " << lum << " R != G";
		EXPECT_EQ(g, b) << "Gray entry " << lum << " G != B";
	}
}

//=============================================================================
// Frame Buffer Pipeline Tests
//=============================================================================

TEST(Atari2600CdlAndPaletteTests, FrameBufferDimensionsAre160x192) {
	EXPECT_EQ(Atari2600Console::ScreenWidth, 160u);
	EXPECT_EQ(Atari2600Console::ScreenHeight, 192u);
}

//=============================================================================
// Memory Type Verification Tests
//=============================================================================

TEST(Atari2600CdlAndPaletteTests, Atari2600PrgRomMemoryTypeExists) {
	MemoryType prgRom = MemoryType::Atari2600PrgRom;
	EXPECT_NE(static_cast<int>(prgRom), 0);
}

TEST(Atari2600CdlAndPaletteTests, Atari2600RamMemoryTypeExists) {
	MemoryType ram = MemoryType::Atari2600Ram;
	EXPECT_NE(static_cast<int>(ram), static_cast<int>(MemoryType::Atari2600PrgRom));
}

//=============================================================================
// CdlStripOption Enum Tests
//=============================================================================

TEST(Atari2600CdlAndPaletteTests, CdlStripOptionsAreDistinct) {
	EXPECT_NE(static_cast<int>(CdlStripOption::StripNone), static_cast<int>(CdlStripOption::StripUnused));
	EXPECT_NE(static_cast<int>(CdlStripOption::StripNone), static_cast<int>(CdlStripOption::StripUsed));
	EXPECT_NE(static_cast<int>(CdlStripOption::StripUnused), static_cast<int>(CdlStripOption::StripUsed));
}

} // namespace
