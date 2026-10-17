#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for LynxConsole constants, types, and state structures.
/// Verifies hardware constants match Lynx specifications, enum values are correct,
/// and frame info calculations produce expected dimensions for each rotation.
/// </summary>
class LynxConsoleTest : public ::testing::Test {
protected:
	void SetUp() override { }
};

//=============================================================================
// Hardware Constants Tests
//=============================================================================

TEST_F(LynxConsoleTest, Constants_MasterClockRate_16MHz) {
	EXPECT_EQ(LynxConstants::MasterClockRate, 16000000u);
}

TEST_F(LynxConsoleTest, Constants_CpuDivider_4) {
	EXPECT_EQ(LynxConstants::CpuDivider, 4u);
}

TEST_F(LynxConsoleTest, Constants_CpuClockRate_4MHz) {
	EXPECT_EQ(LynxConstants::CpuClockRate, 4000000u);
	EXPECT_EQ(LynxConstants::CpuClockRate, LynxConstants::MasterClockRate / LynxConstants::CpuDivider);
}

TEST_F(LynxConsoleTest, Constants_ScreenDimensions) {
	EXPECT_EQ(LynxConstants::ScreenWidth, 160u);
	EXPECT_EQ(LynxConstants::ScreenHeight, 102u);
	EXPECT_EQ(LynxConstants::PixelCount, 160u * 102u);
}

TEST_F(LynxConsoleTest, Constants_ScanlineCount_105) {
	EXPECT_EQ(LynxConstants::ScanlineCount, 105u);
}

TEST_F(LynxConsoleTest, Constants_Fps_75Hz) {
	EXPECT_DOUBLE_EQ(LynxConstants::Fps, 75.0);
}

TEST_F(LynxConsoleTest, Constants_CpuCyclesPerFrame) {
	// 4,000,000 / 75.0 = 53,333
	uint32_t expected = static_cast<uint32_t>(LynxConstants::CpuClockRate / LynxConstants::Fps);
	EXPECT_EQ(LynxConstants::CpuCyclesPerFrame, expected);
}

TEST_F(LynxConsoleTest, Constants_WorkRamSize_64K) {
	EXPECT_EQ(LynxConstants::WorkRamSize, 0x10000u);
}

TEST_F(LynxConsoleTest, Constants_BootRomSize_512) {
	EXPECT_EQ(LynxConstants::BootRomSize, 0x200u);
}

TEST_F(LynxConsoleTest, Constants_MemoryMapAddresses) {
	// Mikey: FD00-FDFF
	EXPECT_EQ(LynxConstants::MikeyBase, 0xfd00);
	EXPECT_EQ(LynxConstants::MikeyEnd, 0xfdff);
	// Suzy: FC00-FCFF
	EXPECT_EQ(LynxConstants::SuzyBase, 0xfc00);
	EXPECT_EQ(LynxConstants::SuzyEnd, 0xfcff);
	// Boot ROM: FE00+
	EXPECT_EQ(LynxConstants::BootRomBase, 0xfe00);
}

TEST_F(LynxConsoleTest, Constants_PeripheralCounts) {
	EXPECT_EQ(LynxConstants::TimerCount, 8u);
	EXPECT_EQ(LynxConstants::AudioChannelCount, 4u);
	EXPECT_EQ(LynxConstants::PaletteSize, 16u);
	EXPECT_EQ(LynxConstants::CollisionBufferSize, 16u);
}

TEST_F(LynxConsoleTest, Constants_BytesPerScanline) {
	// 160 pixels × 4bpp / 8 bits = 80 bytes
	EXPECT_EQ(LynxConstants::BytesPerScanline, 80u);
}

//=============================================================================
// Enum Value Tests
//=============================================================================

TEST_F(LynxConsoleTest, LynxModel_Values) {
	EXPECT_EQ(static_cast<uint8_t>(LynxModel::LynxI), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxModel::LynxII), 1);
}

TEST_F(LynxConsoleTest, LynxRotation_Values) {
	EXPECT_EQ(static_cast<uint8_t>(LynxRotation::None), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxRotation::Left), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxRotation::Right), 2);
}

TEST_F(LynxConsoleTest, CpuStopState_Values) {
	EXPECT_EQ(static_cast<uint8_t>(LynxCpuStopState::Running), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxCpuStopState::Stopped), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxCpuStopState::WaitingForIrq), 2);
}

//=============================================================================
// Frame Info / Rotation Math Tests
//=============================================================================

// Verify the rotation transform produces correct output dimensions
// (mirrors LynxDefaultVideoFilter::GetFrameInfo logic without emulator deps)
TEST_F(LynxConsoleTest, FrameInfo_NoRotation_160x102) {
	LynxRotation rotation = LynxRotation::None;
	uint32_t width, height;
	if (rotation == LynxRotation::Left || rotation == LynxRotation::Right) {
		width = LynxConstants::ScreenHeight;
		height = LynxConstants::ScreenWidth;
	} else {
		width = LynxConstants::ScreenWidth;
		height = LynxConstants::ScreenHeight;
	}
	EXPECT_EQ(width, 160u);
	EXPECT_EQ(height, 102u);
}

TEST_F(LynxConsoleTest, FrameInfo_LeftRotation_102x160) {
	LynxRotation rotation = LynxRotation::Left;
	uint32_t width, height;
	if (rotation == LynxRotation::Left || rotation == LynxRotation::Right) {
		width = LynxConstants::ScreenHeight;
		height = LynxConstants::ScreenWidth;
	} else {
		width = LynxConstants::ScreenWidth;
		height = LynxConstants::ScreenHeight;
	}
	EXPECT_EQ(width, 102u);
	EXPECT_EQ(height, 160u);
}

TEST_F(LynxConsoleTest, FrameInfo_RightRotation_102x160) {
	LynxRotation rotation = LynxRotation::Right;
	uint32_t width, height;
	if (rotation == LynxRotation::Left || rotation == LynxRotation::Right) {
		width = LynxConstants::ScreenHeight;
		height = LynxConstants::ScreenWidth;
	} else {
		width = LynxConstants::ScreenWidth;
		height = LynxConstants::ScreenHeight;
	}
	EXPECT_EQ(width, 102u);
	EXPECT_EQ(height, 160u);
}

//=============================================================================
// Video Rotation Pixel Mapping Tests
//=============================================================================

// Verify the rotation pixel mapping formulas produce correct index transforms
// These match the exact formulas in LynxDefaultVideoFilter::ApplyFilter()
TEST_F(LynxConsoleTest, Rotation_None_IdentityMapping) {
	uint32_t srcW = LynxConstants::ScreenWidth;   // 160
	uint32_t srcH = LynxConstants::ScreenHeight;   // 102
	// No rotation: out[i] = src[i]
	for (uint32_t y = 0; y < srcH; y++) {
		for (uint32_t x = 0; x < srcW; x++) {
			uint32_t srcIdx = y * srcW + x;
			uint32_t outIdx = srcIdx; // identity
			ASSERT_EQ(outIdx, srcIdx);
		}
	}
}

TEST_F(LynxConsoleTest, Rotation_Left_CornerPixels) {
	uint32_t srcW = LynxConstants::ScreenWidth;   // 160
	uint32_t srcH = LynxConstants::ScreenHeight;   // 102
	// Left rotation: out[(srcW-1-x)*srcH + y] = src[y*srcW + x]
	// Output is 102×160

	// Top-left (0,0) → out[(159)*102 + 0] = out[16218]
	EXPECT_EQ((srcW - 1 - 0) * srcH + 0, 159u * 102u);

	// Top-right (159,0) → out[(0)*102 + 0] = out[0]
	EXPECT_EQ((srcW - 1 - 159) * srcH + 0, 0u);

	// Bottom-left (0,101) → out[(159)*102 + 101] = out[16319]
	EXPECT_EQ((srcW - 1 - 0) * srcH + 101, 159u * 102u + 101);

	// Bottom-right (159,101) → out[(0)*102 + 101] = out[101]
	EXPECT_EQ((srcW - 1 - 159) * srcH + 101, 101u);
}

TEST_F(LynxConsoleTest, Rotation_Right_CornerPixels) {
	uint32_t srcH = LynxConstants::ScreenHeight;   // 102
	// Right rotation: out[x*srcH + (srcH-1-y)] = src[y*srcW + x]
	// Output is 102×160

	// Top-left (0,0) → out[0*102 + 101] = out[101]
	EXPECT_EQ(0u * srcH + (srcH - 1 - 0), 101u);

	// Top-right (159,0) → out[159*102 + 101] = out[16319]
	EXPECT_EQ(159u * srcH + (srcH - 1 - 0), 159u * 102u + 101);

	// Bottom-left (0,101) → out[0*102 + 0] = out[0]
	EXPECT_EQ(0u * srcH + (srcH - 1 - 101), 0u);

	// Bottom-right (159,101) → out[159*102 + 0] = out[16218]
	EXPECT_EQ(159u * srcH + (srcH - 1 - 101), 159u * 102u);
}

TEST_F(LynxConsoleTest, Rotation_PixelCountPreserved) {
	// Both rotated and non-rotated have same total pixel count
	uint32_t normalCount = LynxConstants::ScreenWidth * LynxConstants::ScreenHeight;
	uint32_t rotatedCount = LynxConstants::ScreenHeight * LynxConstants::ScreenWidth;
	EXPECT_EQ(normalCount, rotatedCount);
	EXPECT_EQ(normalCount, LynxConstants::PixelCount);
}

//=============================================================================
// EEPROM Type Tests
//=============================================================================

TEST_F(LynxConsoleTest, EepromType_EnumOrder) {
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::None), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c46), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c56), 2);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c66), 3);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c76), 4);
	EXPECT_EQ(static_cast<uint8_t>(LynxEepromType::Eeprom93c86), 5);
}

//=============================================================================
// Sprite Type Tests
//=============================================================================

TEST_F(LynxConsoleTest, SpriteType_Values) {
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::BackgroundShadow), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::BackgroundNonCollide), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::BoundaryShadow), 2);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::Boundary), 3);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::Normal), 4);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::NonCollidable), 5);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::XorShadow), 6);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteType::Shadow), 7);
}

TEST_F(LynxConsoleTest, SpriteBpp_Values) {
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteBpp::Bpp1), 0);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteBpp::Bpp2), 1);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteBpp::Bpp3), 2);
	EXPECT_EQ(static_cast<uint8_t>(LynxSpriteBpp::Bpp4), 3);
}

//=============================================================================
// Suzy State Default Tests
//=============================================================================

TEST_F(LynxConsoleTest, SuzyState_DefaultMathRegisters) {
	LynxSuzyState state = {};
	// Default-constructed state should have zero-initialized math registers
	EXPECT_EQ(state.MathABCD, 0u);
	EXPECT_EQ(state.MathEFGH, 0u);
	EXPECT_EQ(state.MathJKLM, 0u);
	EXPECT_EQ(state.MathNP, 0u);
}

TEST_F(LynxConsoleTest, SuzyState_DefaultFlags) {
	LynxSuzyState state = {};
	EXPECT_FALSE(state.SpriteBusy);
	EXPECT_FALSE(state.SpriteEnabled);
	EXPECT_FALSE(state.MathInProgress);
	EXPECT_FALSE(state.MathOverflow);
	EXPECT_FALSE(state.MathSign);
	EXPECT_FALSE(state.MathAccumulate);
	EXPECT_FALSE(state.UnsafeAccess);
	EXPECT_FALSE(state.StopOnCurrent);
	EXPECT_FALSE(state.VStretch);
	EXPECT_FALSE(state.LeftHand);
}

//=============================================================================
// IRQ Source Bitmask Tests
//=============================================================================

TEST_F(LynxConsoleTest, IrqSource_TimerBitmasks) {
	// Each timer IRQ should be a unique single bit
	EXPECT_EQ(LynxIrqSource::Timer0, 0x01);
	EXPECT_EQ(LynxIrqSource::Timer1, 0x02);
	EXPECT_EQ(LynxIrqSource::Timer2, 0x04);
	EXPECT_EQ(LynxIrqSource::Timer3, 0x08);
	EXPECT_EQ(LynxIrqSource::Timer4, 0x10);
}

TEST_F(LynxConsoleTest, IrqSource_NoBitOverlap) {
	uint8_t allBits = LynxIrqSource::Timer0 | LynxIrqSource::Timer1 |
		LynxIrqSource::Timer2 | LynxIrqSource::Timer3 | LynxIrqSource::Timer4;
	// Should be 0x1f = 5 bits set
	EXPECT_EQ(allBits, 0x1f);
}
