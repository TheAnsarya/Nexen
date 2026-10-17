#include "pch.h"
#include "ChannelF/ChannelFTypes.h"
#include "ChannelF/ChannelFDefaultVideoFilter.h"

// ============================================================================
// ChannelFTypes state struct tests
// ============================================================================

// --- ChannelFVideoState ---

TEST(ChannelFVideoStateTest, DefaultState_ZeroInitialized) {
	ChannelFVideoState video = {};
	EXPECT_EQ(video.Color, 0);
	EXPECT_EQ(video.X, 0);
	EXPECT_EQ(video.Y, 0);
	EXPECT_EQ(video.BackgroundColor, 0);
	EXPECT_EQ(video.PendingWrites, 0);
}

TEST(ChannelFVideoStateTest, ColorField_TwoBitRange) {
	ChannelFVideoState video = {};
	for (uint8_t c = 0; c < 4; c++) {
		video.Color = c;
		EXPECT_EQ(video.Color, c);
	}
}

TEST(ChannelFVideoStateTest, Position_FullRange) {
	ChannelFVideoState video = {};
	video.X = 127;
	video.Y = 63;
	EXPECT_EQ(video.X, 127);
	EXPECT_EQ(video.Y, 63);
}

// --- ChannelFAudioState ---

TEST(ChannelFAudioStateTest, DefaultState_ZeroInitialized) {
	ChannelFAudioState audio = {};
	EXPECT_EQ(audio.ToneSelect, 0);
	EXPECT_EQ(audio.Volume, 0x0f);
	EXPECT_FALSE(audio.SoundEnabled);
	EXPECT_EQ(audio.HalfPeriodCycles, 0u);
	EXPECT_EQ(audio.CycleCounter, 0u);
	EXPECT_FALSE(audio.OutputHigh);
}

TEST(ChannelFAudioStateTest, SoundEnabled_Toggle) {
	ChannelFAudioState audio = {};
	audio.SoundEnabled = true;
	EXPECT_TRUE(audio.SoundEnabled);
	audio.SoundEnabled = false;
	EXPECT_FALSE(audio.SoundEnabled);
}

TEST(ChannelFAudioStateTest, ToneSelect_ValidRange) {
	ChannelFAudioState audio = {};
	// ToneSelect is 2-bit: 0=silence, 1=~1kHz, 2=~500Hz, 3=~120Hz
	for (uint8_t i = 0; i < 4; i++) {
		audio.ToneSelect = i;
		EXPECT_EQ(audio.ToneSelect, i);
	}
}

// --- ChannelFPortState ---

TEST(ChannelFPortStateTest, DefaultState_ControllerPortsHighNoInput) {
	ChannelFPortState ports = {};
	EXPECT_EQ(ports.Port0, 0);
	EXPECT_EQ(ports.Port1, 0);
	EXPECT_EQ(ports.Port4, 0xff);
	EXPECT_EQ(ports.Port5, 0xff);
}

TEST(ChannelFPortStateTest, ControllerPorts_ActiveLowButtonPress) {
	ChannelFPortState ports = {};
	// Simulate pressing right on controller 1 (bit 0 cleared)
	ports.Port4 = 0xff & ~0x01;
	EXPECT_EQ(ports.Port4, 0xfe);
	EXPECT_EQ(ports.Port4 & 0x01, 0); // right pressed
	EXPECT_NE(ports.Port4 & 0x02, 0); // left not pressed
}

TEST(ChannelFPortStateTest, AllButtonsPressed_AllBitsClear) {
	ChannelFPortState ports = {};
	ports.Port4 = 0x00; // All 8 buttons pressed
	EXPECT_EQ(ports.Port4, 0x00);
}

// --- ChannelFCpuState ---

TEST(ChannelFCpuStateTest, DefaultState_ZeroInitialized) {
	ChannelFCpuState cpu = {};
	EXPECT_EQ(cpu.CycleCount, 0u);
	EXPECT_EQ(cpu.PC0, 0);
	EXPECT_EQ(cpu.PC1, 0);
	EXPECT_EQ(cpu.DC0, 0);
	EXPECT_EQ(cpu.DC1, 0);
	EXPECT_EQ(cpu.A, 0);
	EXPECT_EQ(cpu.W, 0);
	EXPECT_EQ(cpu.ISAR, 0);
	EXPECT_FALSE(cpu.InterruptsEnabled);
}

TEST(ChannelFCpuStateTest, Scratchpad_64Bytes_ZeroInit) {
	ChannelFCpuState cpu = {};
	for (int i = 0; i < 64; i++) {
		EXPECT_EQ(cpu.Scratchpad[i], 0);
	}
}

TEST(ChannelFCpuStateTest, Scratchpad_ReadWriteAllSlots) {
	ChannelFCpuState cpu = {};
	for (int i = 0; i < 64; i++) {
		cpu.Scratchpad[i] = static_cast<uint8_t>(i * 4 + 1);
	}
	for (int i = 0; i < 64; i++) {
		EXPECT_EQ(cpu.Scratchpad[i], static_cast<uint8_t>(i * 4 + 1));
	}
}

TEST(ChannelFCpuStateTest, ProgramCounters_16BitRange) {
	ChannelFCpuState cpu = {};
	cpu.PC0 = 0xffff;
	cpu.PC1 = 0x0800;
	EXPECT_EQ(cpu.PC0, 0xffff);
	EXPECT_EQ(cpu.PC1, 0x0800);
}

TEST(ChannelFCpuStateTest, DataCounters_16BitRange) {
	ChannelFCpuState cpu = {};
	cpu.DC0 = 0x1234;
	cpu.DC1 = 0xabcd;
	EXPECT_EQ(cpu.DC0, 0x1234);
	EXPECT_EQ(cpu.DC1, 0xabcd);
}

TEST(ChannelFCpuStateTest, ISAR_SixBitRange) {
	ChannelFCpuState cpu = {};
	// ISAR is 6-bit (0-63) addressing scratchpad
	cpu.ISAR = 63;
	EXPECT_EQ(cpu.ISAR, 63);
	cpu.ISAR = 0;
	EXPECT_EQ(cpu.ISAR, 0);
}

TEST(ChannelFCpuStateTest, StatusRegisterW_FlagBits) {
	ChannelFCpuState cpu = {};
	// W register: bit 0 = sign (complementary), bit 1 = carry, bit 2 = zero, bit 3 = overflow
	cpu.W = 0x0f;
	EXPECT_EQ(cpu.W & 0x01, 1); // sign
	EXPECT_EQ(cpu.W & 0x02, 2); // carry
	EXPECT_EQ(cpu.W & 0x04, 4); // zero
	EXPECT_EQ(cpu.W & 0x08, 8); // overflow
}

// --- ChannelFState (aggregate) ---

TEST(ChannelFStateTest, DefaultState_AllSubstatesZero) {
	ChannelFState state = {};
	EXPECT_EQ(state.Cpu.PC0, 0);
	EXPECT_EQ(state.Video.Color, 0);
	EXPECT_EQ(state.Audio.ToneSelect, 0);
	EXPECT_EQ(state.Ports.Port4, 0xff);
	EXPECT_EQ(state.Ports.Port5, 0xff);
}

TEST(ChannelFStateTest, MutateSubstates_Independent) {
	ChannelFState state = {};
	state.Cpu.A = 0x42;
	state.Video.Color = 3;
	state.Audio.SoundEnabled = true;
	state.Ports.Port4 = 0x55;

	EXPECT_EQ(state.Cpu.A, 0x42);
	EXPECT_EQ(state.Video.Color, 3);
	EXPECT_TRUE(state.Audio.SoundEnabled);
	EXPECT_EQ(state.Ports.Port4, 0x55);
}

// ============================================================================
// ChannelF Constants tests
// ============================================================================

TEST(ChannelFConstantsTest, ScreenDimensions) {
	EXPECT_EQ(ChannelFConstants::ScreenWidth, 128u);
	EXPECT_EQ(ChannelFConstants::ScreenHeight, 64u);
	EXPECT_EQ(ChannelFConstants::PixelCount, 128u * 64u);
}

TEST(ChannelFConstantsTest, ClockAndTiming) {
	EXPECT_EQ(ChannelFConstants::CpuClockHz, 1789773u);
	EXPECT_EQ(ChannelFConstants::CyclesPerFrame, 29829u);
	EXPECT_DOUBLE_EQ(ChannelFConstants::Fps, 60.0);
}

TEST(ChannelFConstantsTest, MemorySizes) {
	EXPECT_EQ(ChannelFConstants::BiosSize, 0x0800u);
	EXPECT_EQ(ChannelFConstants::MaxCartSize, 0x10000u);
	EXPECT_EQ(ChannelFConstants::VramSize, 128u * 64u);
	EXPECT_EQ(ChannelFConstants::ScratchpadSize, 64u);
	EXPECT_EQ(ChannelFConstants::NumColors, 8);
}

// ============================================================================
// VideoFilter palette tests
// ============================================================================

TEST(ChannelFVideoFilterTest, Colors_EightHardwareColors) {
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[0], 0xff101010u); // BLACK
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[1], 0xfffdfdfdu); // WHITE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[2], 0xffff3153u); // RED
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[3], 0xff02cc5du); // GREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[4], 0xff4b3ff3u); // BLUE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[5], 0xffe0e0e0u); // LTGRAY
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[6], 0xff91ffa6u); // LTGREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColors[7], 0xffced0ffu); // LTBLUE
}

TEST(ChannelFVideoFilterTest, Colors_AllOpaque) {
	for (int i = 0; i < 8; i++) {
		uint32_t alpha = ChannelFDefaultVideoFilter::ChannelFColors[i] >> 24;
		EXPECT_EQ(alpha, 0xffu);
	}
}

TEST(ChannelFVideoFilterTest, Colormap_Palette0_MonochromeWhite) {
	// Palette 0: BLACK, WHITE, WHITE, WHITE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[0], 0xff101010u); // BLACK
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[1], 0xfffdfdfdu); // WHITE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[2], 0xfffdfdfdu); // WHITE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[3], 0xfffdfdfdu); // WHITE
}

TEST(ChannelFVideoFilterTest, Colormap_Palette1_LtblueBG) {
	// Palette 1: LTBLUE, BLUE, RED, GREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[4], 0xffced0ffu); // LTBLUE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[5], 0xff4b3ff3u); // BLUE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[6], 0xffff3153u); // RED
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[7], 0xff02cc5du); // GREEN
}

TEST(ChannelFVideoFilterTest, Colormap_Palette2_LtgrayBG) {
	// Palette 2: LTGRAY, BLUE, RED, GREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[8], 0xffe0e0e0u);  // LTGRAY
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[9], 0xff4b3ff3u);  // BLUE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[10], 0xffff3153u); // RED
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[11], 0xff02cc5du); // GREEN
}

TEST(ChannelFVideoFilterTest, Colormap_Palette3_LtgreenBG) {
	// Palette 3: LTGREEN, BLUE, RED, GREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[12], 0xff91ffa6u); // LTGREEN
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[13], 0xff4b3ff3u); // BLUE
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[14], 0xffff3153u); // RED
	EXPECT_EQ(ChannelFDefaultVideoFilter::ChannelFColormap[15], 0xff02cc5du); // GREEN
}

TEST(ChannelFVideoFilterTest, Colormap_IndexMasking) {
	// Verify 4-bit index works with mask (0x0f)
	for (uint16_t raw = 0; raw < 256; raw++) {
		uint8_t idx = raw & 0x0f;
		EXPECT_LT(idx, 16);
		uint32_t color = ChannelFDefaultVideoFilter::ChannelFColormap[idx];
		EXPECT_NE(color, 0u); // All colormap entries have alpha
	}
}

// ============================================================================
// Controller input encoding tests (pure bitwise, no hardware needed)
// ============================================================================

namespace ChannelFInputHelpers {
	// Reproduce active-low encoding from ChannelFController.h
	static constexpr uint8_t EncodeController(bool right, bool left, bool back, bool forward,
		bool twistCCW, bool twistCW, bool pull, bool push) {
		uint8_t value = 0xff;
		if (right)    value &= ~0x01;
		if (left)     value &= ~0x02;
		if (back)     value &= ~0x04;
		if (forward)  value &= ~0x08;
		if (twistCCW) value &= ~0x10;
		if (twistCW)  value &= ~0x20;
		if (pull)     value &= ~0x40;
		if (push)     value &= ~0x80;
		return value;
	}

	static constexpr uint8_t EncodeConsolePanel(bool time, bool mode, bool hold, bool start) {
		uint8_t value = 0xff;
		if (time)  value &= ~0x01;
		if (mode)  value &= ~0x02;
		if (hold)  value &= ~0x04;
		if (start) value &= ~0x08;
		return value;
	}
}

TEST(ChannelFInputEncodingTest, NoInput_AllBitsHigh) {
	uint8_t val = ChannelFInputHelpers::EncodeController(false, false, false, false, false, false, false, false);
	EXPECT_EQ(val, 0xff);
}

TEST(ChannelFInputEncodingTest, SingleButton_ClearsSingleBit) {
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(true, false, false, false, false, false, false, false), 0xfe);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, true, false, false, false, false, false, false), 0xfd);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, true, false, false, false, false, false), 0xfb);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, false, true, false, false, false, false), 0xf7);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, false, false, true, false, false, false), 0xef);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, false, false, false, true, false, false), 0xdf);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, false, false, false, false, true, false), 0xbf);
	EXPECT_EQ(ChannelFInputHelpers::EncodeController(false, false, false, false, false, false, false, true), 0x7f);
}

TEST(ChannelFInputEncodingTest, AllButtons_AllBitsClear) {
	uint8_t val = ChannelFInputHelpers::EncodeController(true, true, true, true, true, true, true, true);
	EXPECT_EQ(val, 0x00);
}

TEST(ChannelFInputEncodingTest, DiagonalInput_TwoBitsClear) {
	// Right + Forward (bits 0 and 3)
	uint8_t val = ChannelFInputHelpers::EncodeController(true, false, false, true, false, false, false, false);
	EXPECT_EQ(val, 0xff & ~0x01 & ~0x08);
	EXPECT_EQ(val, 0xf6);
}

TEST(ChannelFInputEncodingTest, ConsolePanelNoInput_AllHigh) {
	uint8_t val = ChannelFInputHelpers::EncodeConsolePanel(false, false, false, false);
	EXPECT_EQ(val, 0xff);
}

TEST(ChannelFInputEncodingTest, ConsolePanelSingleButton) {
	EXPECT_EQ(ChannelFInputHelpers::EncodeConsolePanel(true, false, false, false), 0xfe);
	EXPECT_EQ(ChannelFInputHelpers::EncodeConsolePanel(false, true, false, false), 0xfd);
	EXPECT_EQ(ChannelFInputHelpers::EncodeConsolePanel(false, false, true, false), 0xfb);
	EXPECT_EQ(ChannelFInputHelpers::EncodeConsolePanel(false, false, false, true), 0xf7);
}

TEST(ChannelFInputEncodingTest, ConsolePanelAllPressed) {
	uint8_t val = ChannelFInputHelpers::EncodeConsolePanel(true, true, true, true);
	EXPECT_EQ(val, 0xf0); // Only low nibble affected
}

// ============================================================================
// VRAM addressing helpers
// ============================================================================

namespace ChannelFVramHelpers {
	// Compute flat VRAM index from X,Y coordinates
	static constexpr uint32_t VramIndex(uint8_t x, uint8_t y) {
		return static_cast<uint32_t>(y) * ChannelFConstants::ScreenWidth + x;
	}

	// Verify coordinates are in bounds
	static constexpr bool InBounds(uint8_t x, uint8_t y) {
		return x < ChannelFConstants::ScreenWidth && y < ChannelFConstants::ScreenHeight;
	}
}

TEST(ChannelFVramTest, Index_TopLeft) {
	EXPECT_EQ(ChannelFVramHelpers::VramIndex(0, 0), 0u);
}

TEST(ChannelFVramTest, Index_TopRight) {
	EXPECT_EQ(ChannelFVramHelpers::VramIndex(127, 0), 127u);
}

TEST(ChannelFVramTest, Index_BottomLeft) {
	EXPECT_EQ(ChannelFVramHelpers::VramIndex(0, 63), 63u * 128u);
}

TEST(ChannelFVramTest, Index_BottomRight) {
	EXPECT_EQ(ChannelFVramHelpers::VramIndex(127, 63), 128u * 64u - 1u);
}

TEST(ChannelFVramTest, Index_AllPixelsUnique) {
	std::set<uint32_t> indices;
	for (uint8_t y = 0; y < 64; y++) {
		for (uint8_t x = 0; x < 128; x++) {
			uint32_t idx = ChannelFVramHelpers::VramIndex(x, y);
			ASSERT_TRUE(indices.insert(idx).second) << "Duplicate index at (" << (int)x << "," << (int)y << ")";
		}
	}
	EXPECT_EQ(indices.size(), ChannelFConstants::PixelCount);
}

TEST(ChannelFVramTest, Bounds_ValidCoordinates) {
	EXPECT_TRUE(ChannelFVramHelpers::InBounds(0, 0));
	EXPECT_TRUE(ChannelFVramHelpers::InBounds(127, 63));
	EXPECT_TRUE(ChannelFVramHelpers::InBounds(64, 32));
}

TEST(ChannelFVramTest, Bounds_InvalidCoordinates) {
	EXPECT_FALSE(ChannelFVramHelpers::InBounds(128, 0));
	EXPECT_FALSE(ChannelFVramHelpers::InBounds(0, 64));
	EXPECT_FALSE(ChannelFVramHelpers::InBounds(128, 64));
	EXPECT_FALSE(ChannelFVramHelpers::InBounds(255, 255));
}

// ============================================================================
// Audio frequency/tone encoding
// ============================================================================

namespace ChannelFAudioHelpers {
	// Square wave period from frequency register
	// Channel F PSG: frequency divider = tone value, higher = lower pitch
	static constexpr uint32_t ComputeSquareWavePeriod(uint8_t freq) {
		if (freq == 0) return 0;
		return ChannelFConstants::CpuClockHz / (2u * freq);
	}
}

TEST(ChannelFAudioEncodingTest, ZeroFreq_ZeroPeriod) {
	EXPECT_EQ(ChannelFAudioHelpers::ComputeSquareWavePeriod(0), 0u);
}

TEST(ChannelFAudioEncodingTest, MaxFreq_MinPeriod) {
	uint32_t period = ChannelFAudioHelpers::ComputeSquareWavePeriod(0xff);
	EXPECT_GT(period, 0u);
	EXPECT_LT(period, ChannelFConstants::CpuClockHz);
}

TEST(ChannelFAudioEncodingTest, FreqMonotonic_HigherFreqShorterPeriod) {
	uint32_t periodLow = ChannelFAudioHelpers::ComputeSquareWavePeriod(10);
	uint32_t periodHigh = ChannelFAudioHelpers::ComputeSquareWavePeriod(100);
	EXPECT_GT(periodLow, periodHigh);
}

// ============================================================================
// Memory map boundary tests (address constants only)
// ============================================================================

TEST(ChannelFMemoryMapTest, BiosRegions) {
	// BIOS ROM 1: $0000-$03FF (1KB)
	// BIOS ROM 2: $0400-$07FF (1KB)
	EXPECT_EQ(ChannelFConstants::BiosSize, 0x0800u); // Total 2KB BIOS
}

TEST(ChannelFMemoryMapTest, CartridgeStartAddress) {
	// Cartridge ROM starts at $0800
	uint16_t cartStart = 0x0800;
	EXPECT_EQ(cartStart, ChannelFConstants::BiosSize);
}

TEST(ChannelFMemoryMapTest, MaxAddressSpace) {
	// F8 CPU has 16-bit address bus = 64KB
	EXPECT_EQ(ChannelFConstants::MaxCartSize, 0x10000u);
}
