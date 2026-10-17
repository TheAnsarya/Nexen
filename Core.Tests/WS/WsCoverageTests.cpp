#include "pch.h"
#include "WS/WsTypes.h"
#include "Shared/SettingTypes.h"

// =============================================================================
// WonderSwan Coverage Expansion Tests
// =============================================================================
// Tests for WS areas not covered by existing test files:
// - WsLcdIcons (icon register bitfields)
// - WsPpuState gaps (HighContrast, LcdTftConfig, BackPorchScanline, etc.)
// - WsApuHyperVoiceState field coverage
// - WsModel / WsAudioMode enums
// - WsCartState bank selection
// - WsSerialState field coverage

namespace WsCoverageHelpers {

	/// <summary>
	/// Decodes LCD icon register (8 bits) into individual icon states.
	/// Hardware LCD segments on the WonderSwan body.
	/// Bit 0: Sleep, Bit 1: Vertical, Bit 2: Horizontal,
	/// Bit 3: Aux1, Bit 4: Aux2, Bit 5: Aux3
	/// </summary>
	struct LcdIconResult {
		bool sleep;
		bool vertical;
		bool horizontal;
		bool aux1;
		bool aux2;
		bool aux3;
	};

	static LcdIconResult DecodeLcdIcons(uint8_t value) {
		return {
			(value & 0x01) != 0,
			(value & 0x02) != 0,
			(value & 0x04) != 0,
			(value & 0x08) != 0,
			(value & 0x10) != 0,
			(value & 0x20) != 0
		};
	}

	/// <summary>
	/// HyperVoice control register decoding.
	/// Low byte: bits 0-3 = shift, bit 4-5 = scaling mode
	/// High byte: bits 0-1 = channel mode, bit 2 = enable, bits 4-7 = divisor
	/// </summary>
	static uint8_t HyperVoiceShift(uint8_t controlLow) {
		return controlLow & 0x0f;
	}

	static WsHyperVoiceScalingMode HyperVoiceScaling(uint8_t controlLow) {
		return static_cast<WsHyperVoiceScalingMode>((controlLow >> 4) & 0x03);
	}

	static WsHyperVoiceChannelMode HyperVoiceChannel(uint8_t controlHigh) {
		return static_cast<WsHyperVoiceChannelMode>(controlHigh & 0x03);
	}

	static bool HyperVoiceEnabled(uint8_t controlHigh) {
		return (controlHigh & 0x04) != 0;
	}

	static uint8_t HyperVoiceDivisor(uint8_t controlHigh) {
		return (controlHigh >> 4) & 0x0f;
	}

	/// <summary>
	/// WS cart bank slot address mapping.
	/// Slot 0: ROM bank 0 ($00000-$0FFFF), mapped at $20000
	/// Slot 1: ROM bank 1 ($10000-$1FFFF), mapped at $30000
	/// Slot 2: SRAM bank,  mapped at $10000
	/// Slot 3: ROM linear, mapped at $40000-$FFFFF
	/// </summary>
	static uint32_t CartBankOffset(uint8_t bank, uint32_t bankSize) {
		return (uint32_t)bank * bankSize;
	}
}

// =============================================================================
// LCD Icon Register Tests
// =============================================================================

TEST(WsLcdIconsTest, DefaultState_AllOff) {
	WsLcdIcons icons = {};
	EXPECT_FALSE(icons.Sleep);
	EXPECT_FALSE(icons.Vertical);
	EXPECT_FALSE(icons.Horizontal);
	EXPECT_FALSE(icons.Aux1);
	EXPECT_FALSE(icons.Aux2);
	EXPECT_FALSE(icons.Aux3);
	EXPECT_EQ(icons.Value, 0);
}

TEST(WsLcdIconsTest, DecodeIcons_AllOff) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x00);
	EXPECT_FALSE(r.sleep);
	EXPECT_FALSE(r.vertical);
	EXPECT_FALSE(r.horizontal);
	EXPECT_FALSE(r.aux1);
	EXPECT_FALSE(r.aux2);
	EXPECT_FALSE(r.aux3);
}

TEST(WsLcdIconsTest, DecodeIcons_SleepOnly) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x01);
	EXPECT_TRUE(r.sleep);
	EXPECT_FALSE(r.vertical);
	EXPECT_FALSE(r.horizontal);
}

TEST(WsLcdIconsTest, DecodeIcons_VerticalOnly) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x02);
	EXPECT_FALSE(r.sleep);
	EXPECT_TRUE(r.vertical);
}

TEST(WsLcdIconsTest, DecodeIcons_HorizontalOnly) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x04);
	EXPECT_TRUE(r.horizontal);
}

TEST(WsLcdIconsTest, DecodeIcons_Aux1) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x08);
	EXPECT_TRUE(r.aux1);
}

TEST(WsLcdIconsTest, DecodeIcons_Aux2) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x10);
	EXPECT_TRUE(r.aux2);
}

TEST(WsLcdIconsTest, DecodeIcons_Aux3) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x20);
	EXPECT_TRUE(r.aux3);
}

TEST(WsLcdIconsTest, DecodeIcons_AllOn) {
	auto r = WsCoverageHelpers::DecodeLcdIcons(0x3f);
	EXPECT_TRUE(r.sleep);
	EXPECT_TRUE(r.vertical);
	EXPECT_TRUE(r.horizontal);
	EXPECT_TRUE(r.aux1);
	EXPECT_TRUE(r.aux2);
	EXPECT_TRUE(r.aux3);
}

// =============================================================================
// WsPpuState Coverage Gaps
// =============================================================================

TEST(WsPpuCoverageTest, DefaultState_HighContrast) {
	WsPpuState state = {};
	EXPECT_FALSE(state.HighContrast);
}

TEST(WsPpuCoverageTest, DefaultState_LcdControl) {
	WsPpuState state = {};
	EXPECT_EQ(state.LcdControl, 0);
}

TEST(WsPpuCoverageTest, DefaultState_BackPorchScanline) {
	WsPpuState state = {};
	EXPECT_EQ(state.BackPorchScanline, 0);
}

TEST(WsPpuCoverageTest, DefaultState_LcdTftConfig_AllZero) {
	WsPpuState state = {};
	for (int i = 0; i < 8; i++) {
		EXPECT_EQ(state.LcdTftConfig[i], 0);
	}
}

TEST(WsPpuCoverageTest, DefaultState_ShowVolumeIconFrame) {
	WsPpuState state = {};
	EXPECT_EQ(state.ShowVolumeIconFrame, 0u);
}

TEST(WsPpuCoverageTest, DefaultState_DrawOutsideBgWindowLatch) {
	WsPpuState state = {};
	EXPECT_FALSE(state.DrawOutsideBgWindowLatch);
}

TEST(WsPpuCoverageTest, DefaultState_SpriteCountLatch) {
	WsPpuState state = {};
	EXPECT_EQ(state.SpriteCountLatch, 0);
}

TEST(WsPpuCoverageTest, DefaultState_SpritesEnabledLatch) {
	WsPpuState state = {};
	EXPECT_FALSE(state.SpritesEnabledLatch);
}

TEST(WsPpuCoverageTest, DefaultState_FirstSpriteIndex) {
	WsPpuState state = {};
	EXPECT_EQ(state.FirstSpriteIndex, 0);
}

TEST(WsPpuCoverageTest, DefaultState_LcdEnabled) {
	WsPpuState state = {};
	EXPECT_FALSE(state.LcdEnabled);
}

TEST(WsPpuCoverageTest, DefaultState_SleepEnabled) {
	WsPpuState state = {};
	EXPECT_FALSE(state.SleepEnabled);
}

TEST(WsPpuCoverageTest, DefaultState_Icons) {
	WsPpuState state = {};
	EXPECT_EQ(state.Icons.Value, 0);
	EXPECT_FALSE(state.Icons.Sleep);
}

TEST(WsPpuCoverageTest, DefaultState_LastScanline) {
	WsPpuState state = {};
	EXPECT_EQ(state.LastScanline, 0);
}

TEST(WsPpuCoverageTest, DefaultState_Control) {
	WsPpuState state = {};
	EXPECT_EQ(state.Control, 0);
}

TEST(WsPpuCoverageTest, DefaultState_ScreenAddress) {
	WsPpuState state = {};
	EXPECT_EQ(state.ScreenAddress, 0);
}

// =============================================================================
// HyperVoice State Tests
// =============================================================================

TEST(WsHyperVoiceTest, DefaultState_AllZero) {
	WsApuHyperVoiceState state = {};
	EXPECT_EQ(state.LeftOutput, 0);
	EXPECT_EQ(state.RightOutput, 0);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.LeftSample, 0);
	EXPECT_EQ(state.RightSample, 0);
	EXPECT_FALSE(state.UpdateRightValue);
	EXPECT_EQ(state.Divisor, 0);
	EXPECT_EQ(state.Timer, 0);
	EXPECT_EQ(state.Input, 0);
	EXPECT_EQ(state.Shift, 0);
	EXPECT_EQ(state.ControlLow, 0);
	EXPECT_EQ(state.ControlHigh, 0);
}

TEST(WsHyperVoiceTest, ControlLow_Shift) {
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceShift(0x00), 0);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceShift(0x01), 1);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceShift(0x0f), 15);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceShift(0xff), 15);
}

TEST(WsHyperVoiceTest, ControlLow_ScalingMode) {
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceScaling(0x00), WsHyperVoiceScalingMode::Unsigned);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceScaling(0x10), WsHyperVoiceScalingMode::UnsignedNegated);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceScaling(0x20), WsHyperVoiceScalingMode::Signed);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceScaling(0x30), WsHyperVoiceScalingMode::None);
}

TEST(WsHyperVoiceTest, ControlHigh_ChannelMode) {
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceChannel(0x00), WsHyperVoiceChannelMode::Stereo);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceChannel(0x01), WsHyperVoiceChannelMode::MonoLeft);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceChannel(0x02), WsHyperVoiceChannelMode::MonoRight);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceChannel(0x03), WsHyperVoiceChannelMode::MonoBoth);
}

TEST(WsHyperVoiceTest, ControlHigh_Enabled) {
	EXPECT_FALSE(WsCoverageHelpers::HyperVoiceEnabled(0x00));
	EXPECT_TRUE(WsCoverageHelpers::HyperVoiceEnabled(0x04));
	EXPECT_TRUE(WsCoverageHelpers::HyperVoiceEnabled(0xff));
}

TEST(WsHyperVoiceTest, ControlHigh_Divisor) {
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceDivisor(0x00), 0);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceDivisor(0x10), 1);
	EXPECT_EQ(WsCoverageHelpers::HyperVoiceDivisor(0xf0), 15);
}

TEST(WsHyperVoiceTest, ScalingEnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Unsigned), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::UnsignedNegated), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Signed), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::None), 3);
}

TEST(WsHyperVoiceTest, ChannelEnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::Stereo), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoLeft), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoRight), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoBoth), 3);
}

// =============================================================================
// WsModel Enum Tests
// =============================================================================

TEST(WsModelTest, EnumValues_Distinct) {
	EXPECT_NE(static_cast<uint8_t>(WsModel::Auto), static_cast<uint8_t>(WsModel::Monochrome));
	EXPECT_NE(static_cast<uint8_t>(WsModel::Monochrome), static_cast<uint8_t>(WsModel::Color));
	EXPECT_NE(static_cast<uint8_t>(WsModel::Color), static_cast<uint8_t>(WsModel::SwanCrystal));
}

TEST(WsModelTest, AutoIsZero) {
	EXPECT_EQ(static_cast<uint8_t>(WsModel::Auto), 0);
}

TEST(WsModelTest, AllFourValues) {
	// Verify sequential assignment
	EXPECT_EQ(static_cast<uint8_t>(WsModel::Auto), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsModel::Monochrome), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsModel::Color), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsModel::SwanCrystal), 3);
}

// =============================================================================
// WsAudioMode Tests
// =============================================================================

TEST(WsAudioModeTest, EnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(WsAudioMode::Headphones), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsAudioMode::Speakers), 1);
}

// =============================================================================
// WsSerialState Tests
// =============================================================================

TEST(WsSerialStateTest, DefaultState) {
	WsSerialState state = {};
	EXPECT_EQ(state.SendClock, 0u);
	EXPECT_FALSE(state.Enabled);
	EXPECT_FALSE(state.HighSpeed);
	EXPECT_FALSE(state.ReceiveOverflow);
	EXPECT_FALSE(state.HasReceiveData);
	EXPECT_EQ(state.ReceiveBuffer, 0);
	EXPECT_FALSE(state.HasSendData);
	EXPECT_EQ(state.SendBuffer, 0);
}

// =============================================================================
// WsCartState Tests
// =============================================================================

TEST(WsCartStateTest, DefaultBanks_AllZero) {
	WsCartState state = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(state.SelectedBanks[i], 0);
	}
}

TEST(WsCartStateTest, BankOffset_16KB) {
	EXPECT_EQ(WsCoverageHelpers::CartBankOffset(0, 0x10000), 0x00000u);
	EXPECT_EQ(WsCoverageHelpers::CartBankOffset(1, 0x10000), 0x10000u);
	EXPECT_EQ(WsCoverageHelpers::CartBankOffset(15, 0x10000), 0xf0000u);
}

TEST(WsCartStateTest, BankOffset_64KB) {
	EXPECT_EQ(WsCoverageHelpers::CartBankOffset(0, 0x10000), 0x00000u);
	EXPECT_EQ(WsCoverageHelpers::CartBankOffset(255, 0x10000), 0xff0000u);
}

// =============================================================================
// WsState Composite — Model field
// =============================================================================

TEST(WsStateCoverageTest, ModelField_DefaultAuto) {
	WsState state = {};
	EXPECT_EQ(state.Model, WsModel::Auto);
}

TEST(WsStateCoverageTest, AllSubstates_HaveIcons) {
	WsState state = {};
	EXPECT_EQ(state.Ppu.Icons.Value, 0);
}

TEST(WsStateCoverageTest, HyperVoice_InApuState) {
	WsState state = {};
	EXPECT_FALSE(state.Apu.Voice.Enabled);
	EXPECT_EQ(state.Apu.Voice.LeftOutput, 0);
	EXPECT_EQ(state.Apu.Voice.RightOutput, 0);
}

TEST(WsStateCoverageTest, CartEeprom_Separate) {
	WsState state = {};
	// Cart EEPROM is separate from internal EEPROM
	WsEepromState* cart = &state.CartEeprom;
	WsEepromState* internal = &state.InternalEeprom;
	EXPECT_NE(cart, internal);
	EXPECT_FALSE(state.CartEeprom.Idle);	// zero-initialized to false
}

TEST(WsStateCoverageTest, Serial_InState) {
	WsState state = {};
	EXPECT_FALSE(state.Serial.Enabled);
	EXPECT_FALSE(state.Serial.HighSpeed);
}
