#include "pch.h"
#include "SMS/SmsTypes.h"

// =============================================================================
// SMS PSG (Programmable Sound Generator) Tests
// =============================================================================
// Tests for SN76489-compatible PSG: 3 tone channels + 1 noise channel,
// volume attenuation, Game Gear stereo panning, LFSR noise generation.

namespace SmsPsgTestHelpers {
	/// <summary>
	/// Volume lookup: 0=loudest, 15=silent.
	/// Each step is ~2dB attenuation. Volume 15 = mute.
	/// </summary>
	static bool IsVolumeMuted(uint8_t volume) {
		return volume == 15;
	}

	/// <summary>
	/// Computes tone period from 10-bit reload value.
	/// Frequency = MasterClock / (32 * ReloadValue)
	/// If ReloadValue is 0 or 1, channel outputs DC level.
	/// </summary>
	static bool IsToneDC(uint16_t reloadValue) {
		return reloadValue <= 1;
	}

	/// <summary>
	/// Decodes noise control register.
	/// Bits 0-1: rate (00=high, 01=med, 10=low, 11=use tone3)
	/// Bit 2: 0=periodic noise, 1=white noise
	/// </summary>
	static uint8_t GetNoiseRate(uint8_t control) {
		return control & 0x03;
	}

	static bool IsWhiteNoise(uint8_t control) {
		return (control & 0x04) != 0;
	}

	static bool UsesTone3Period(uint8_t control) {
		return (control & 0x03) == 0x03;
	}

	/// <summary>
	/// Game Gear panning register: each bit enables a channel on left/right.
	/// Bit 0: Tone1 Right, Bit 1: Tone2 Right, Bit 2: Tone3 Right, Bit 3: Noise Right
	/// Bit 4: Tone1 Left,  Bit 5: Tone2 Left,  Bit 6: Tone3 Left,  Bit 7: Noise Left
	/// Default: 0xFF (all channels on both sides)
	/// </summary>
	static bool IsChannelOnRight(uint8_t panning, int channel) {
		return (panning & (1 << channel)) != 0;
	}

	static bool IsChannelOnLeft(uint8_t panning, int channel) {
		return (panning & (1 << (channel + 4))) != 0;
	}

	/// <summary>
	/// LFSR step for noise generation.
	/// Periodic noise: feedback from bit 0 only (simple shift)
	/// White noise: feedback from XOR of bits 0 and 3 (tapped)
	/// </summary>
	static uint16_t LfsrStep(uint16_t lfsr, bool whiteNoise) {
		uint8_t feedback;
		if (whiteNoise) {
			feedback = ((lfsr ^ (lfsr >> 3)) & 1);
		} else {
			feedback = lfsr & 1;
		}
		return (uint16_t)((lfsr >> 1) | (feedback << 14));
	}
}

// =============================================================================
// PSG State Initialization
// =============================================================================

TEST(SmsPsgStateTest, DefaultState_ZeroInitialized) {
	SmsPsgState state = {};
	EXPECT_EQ(state.SelectedReg, 0);
	EXPECT_EQ(state.GameGearPanningReg, 0xff);
}

TEST(SmsPsgStateTest, ToneChannels_DefaultZero) {
	SmsPsgState state = {};
	for (int i = 0; i < 3; i++) {
		EXPECT_EQ(state.Tone[i].ReloadValue, 0);
		EXPECT_EQ(state.Tone[i].Timer, 0);
		EXPECT_EQ(state.Tone[i].Output, 0);
		EXPECT_EQ(state.Tone[i].Volume, 0);
	}
}

TEST(SmsPsgStateTest, NoiseChannel_DefaultZero) {
	SmsPsgState state = {};
	EXPECT_EQ(state.Noise.Timer, 0);
	EXPECT_EQ(state.Noise.Lfsr, 0);
	EXPECT_EQ(state.Noise.LfsrInputBit, 0);
	EXPECT_EQ(state.Noise.Control, 0);
	EXPECT_EQ(state.Noise.Output, 0);
	EXPECT_EQ(state.Noise.Volume, 0);
}

// =============================================================================
// Volume Attenuation
// =============================================================================

TEST(SmsPsgVolumeTest, Volume0_IsLoudest) {
	EXPECT_FALSE(SmsPsgTestHelpers::IsVolumeMuted(0));
}

TEST(SmsPsgVolumeTest, Volume14_NotMuted) {
	EXPECT_FALSE(SmsPsgTestHelpers::IsVolumeMuted(14));
}

TEST(SmsPsgVolumeTest, Volume15_IsMuted) {
	EXPECT_TRUE(SmsPsgTestHelpers::IsVolumeMuted(15));
}

// =============================================================================
// Tone DC Level Detection
// =============================================================================

TEST(SmsPsgToneTest, ReloadZero_IsDC) {
	EXPECT_TRUE(SmsPsgTestHelpers::IsToneDC(0));
}

TEST(SmsPsgToneTest, ReloadOne_IsDC) {
	EXPECT_TRUE(SmsPsgTestHelpers::IsToneDC(1));
}

TEST(SmsPsgToneTest, ReloadTwo_NotDC) {
	EXPECT_FALSE(SmsPsgTestHelpers::IsToneDC(2));
}

TEST(SmsPsgToneTest, MaxReload_NotDC) {
	EXPECT_FALSE(SmsPsgTestHelpers::IsToneDC(0x3ff));
}

// =============================================================================
// Noise Control Register Decoding
// =============================================================================

TEST(SmsPsgNoiseTest, NoiseRate_Bits01) {
	EXPECT_EQ(SmsPsgTestHelpers::GetNoiseRate(0x00), 0);	// High frequency
	EXPECT_EQ(SmsPsgTestHelpers::GetNoiseRate(0x01), 1);	// Medium frequency
	EXPECT_EQ(SmsPsgTestHelpers::GetNoiseRate(0x02), 2);	// Low frequency
	EXPECT_EQ(SmsPsgTestHelpers::GetNoiseRate(0x03), 3);	// Tone3 period
}

TEST(SmsPsgNoiseTest, WhiteNoise_Bit2) {
	EXPECT_FALSE(SmsPsgTestHelpers::IsWhiteNoise(0x00));	// Periodic
	EXPECT_FALSE(SmsPsgTestHelpers::IsWhiteNoise(0x01));	// Periodic
	EXPECT_TRUE(SmsPsgTestHelpers::IsWhiteNoise(0x04));		// White
	EXPECT_TRUE(SmsPsgTestHelpers::IsWhiteNoise(0x07));		// White + tone3
}

TEST(SmsPsgNoiseTest, UsesTone3_Rate3) {
	EXPECT_FALSE(SmsPsgTestHelpers::UsesTone3Period(0x00));
	EXPECT_FALSE(SmsPsgTestHelpers::UsesTone3Period(0x01));
	EXPECT_FALSE(SmsPsgTestHelpers::UsesTone3Period(0x02));
	EXPECT_TRUE(SmsPsgTestHelpers::UsesTone3Period(0x03));
	EXPECT_TRUE(SmsPsgTestHelpers::UsesTone3Period(0x07));
}

// =============================================================================
// Game Gear Stereo Panning
// =============================================================================

TEST(SmsPsgPanningTest, DefaultPanning_AllEnabled) {
	uint8_t panning = 0xff;
	for (int ch = 0; ch < 4; ch++) {
		EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnRight(panning, ch));
		EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnLeft(panning, ch));
	}
}

TEST(SmsPsgPanningTest, Tone1_RightOnly) {
	// Tone1 right = bit 0, Tone1 left = bit 4
	uint8_t panning = 0x01;	// Only right tone1
	EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnRight(panning, 0));
	EXPECT_FALSE(SmsPsgTestHelpers::IsChannelOnLeft(panning, 0));
}

TEST(SmsPsgPanningTest, Tone1_LeftOnly) {
	uint8_t panning = 0x10;	// Only left tone1
	EXPECT_FALSE(SmsPsgTestHelpers::IsChannelOnRight(panning, 0));
	EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnLeft(panning, 0));
}

TEST(SmsPsgPanningTest, AllMuted_ZeroPanning) {
	uint8_t panning = 0x00;
	for (int ch = 0; ch < 4; ch++) {
		EXPECT_FALSE(SmsPsgTestHelpers::IsChannelOnRight(panning, ch));
		EXPECT_FALSE(SmsPsgTestHelpers::IsChannelOnLeft(panning, ch));
	}
}

TEST(SmsPsgPanningTest, NoiseChannel_Bit3Right_Bit7Left) {
	// Noise right = bit 3, noise left = bit 7
	uint8_t panning = 0x88;	// Noise both sides
	EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnRight(panning, 3));
	EXPECT_TRUE(SmsPsgTestHelpers::IsChannelOnLeft(panning, 3));
}

// =============================================================================
// LFSR Noise Generation
// =============================================================================

TEST(SmsPsgLfsrTest, PeriodicNoise_ShiftsRight) {
	// If LSB is 0, periodic noise shifts and puts 0 at top
	uint16_t lfsr = 0x4000;	// Bit 14 set, bit 0 clear
	uint16_t next = SmsPsgTestHelpers::LfsrStep(lfsr, false);
	EXPECT_EQ(next, 0x2000);	// Shifted right by 1
}

TEST(SmsPsgLfsrTest, PeriodicNoise_FeedbackFromBit0) {
	// If LSB is 1, periodic noise feeds back to bit 14
	uint16_t lfsr = 0x0001;	// Only bit 0 set
	uint16_t next = SmsPsgTestHelpers::LfsrStep(lfsr, false);
	// Shift right: 0x0001 >> 1 = 0x0000, feedback = 1 << 14 = 0x4000
	EXPECT_EQ(next, 0x4000);
}

TEST(SmsPsgLfsrTest, WhiteNoise_XorBits0And3) {
	// Bit 0 = 1, bit 3 = 0: XOR = 1, feedback to bit 14
	uint16_t lfsr = 0x0001;
	uint16_t next = SmsPsgTestHelpers::LfsrStep(lfsr, true);
	EXPECT_EQ(next, 0x4000);
}

TEST(SmsPsgLfsrTest, WhiteNoise_BothBitsSet_XorZero) {
	// Bit 0 = 1, bit 3 = 1: XOR = 0, no feedback
	uint16_t lfsr = 0x0009;	// bits 0 and 3 set
	uint16_t next = SmsPsgTestHelpers::LfsrStep(lfsr, true);
	// Shift right: 0x0009 >> 1 = 0x0004, feedback = 0
	EXPECT_EQ(next, 0x0004);
}

TEST(SmsPsgLfsrTest, WhiteNoise_NeitherBitSet_XorZero) {
	// Bit 0 = 0, bit 3 = 0: XOR = 0, no feedback
	uint16_t lfsr = 0x0002;
	uint16_t next = SmsPsgTestHelpers::LfsrStep(lfsr, true);
	// Shift right: 0x0002 >> 1 = 0x0001, feedback = 0
	EXPECT_EQ(next, 0x0001);
}

TEST(SmsPsgLfsrTest, LfsrSequence_DoesNotStall) {
	// Run LFSR for 100 steps and verify it doesn't get stuck at 0
	uint16_t lfsr = 0x0001;
	bool sawNonZero = false;
	for (int i = 0; i < 100; i++) {
		lfsr = SmsPsgTestHelpers::LfsrStep(lfsr, true);
		if (lfsr != 0) sawNonZero = true;
	}
	EXPECT_TRUE(sawNonZero);
}

// =============================================================================
// Memory Manager State
// =============================================================================

TEST(SmsMemoryManagerTest, DefaultState_IoConfig) {
	SmsMemoryManagerState state = {};
	EXPECT_EQ(state.OpenBus, 0);
	EXPECT_FALSE(state.ExpEnabled);
	EXPECT_FALSE(state.CartridgeEnabled);
	EXPECT_FALSE(state.CardEnabled);
	EXPECT_FALSE(state.WorkRamEnabled);
	EXPECT_FALSE(state.BiosEnabled);
	EXPECT_FALSE(state.IoEnabled);
}

TEST(SmsMemoryManagerTest, DefaultState_GgRegisters) {
	SmsMemoryManagerState state = {};
	EXPECT_EQ(state.GgExtData, 0);
	EXPECT_EQ(state.GgExtConfig, 0);
	EXPECT_EQ(state.GgSendData, 0);
	EXPECT_EQ(state.GgSerialConfig, 0);
}

// =============================================================================
// SMS Controller Manager State
// =============================================================================

TEST(SmsControlManagerTest, DefaultState_ZeroControl) {
	SmsControlManagerState state = {};
	EXPECT_EQ(state.ControlPort, 0);
}

// =============================================================================
// SMS Composite State
// =============================================================================

TEST(SmsStateTest, CompositeState_AllSubsystemsZero) {
	SmsState state = {};
	EXPECT_EQ(state.Cpu.PC, 0);
	EXPECT_EQ(state.Cpu.SP, 0);
	EXPECT_EQ(state.Vdp.FrameCount, 0u);
	EXPECT_EQ(state.Psg.SelectedReg, 0);
	EXPECT_EQ(state.MemoryManager.OpenBus, 0);
	EXPECT_EQ(state.ControlManager.ControlPort, 0);
}

// =============================================================================
// SMS Model Enumeration
// =============================================================================

TEST(SmsModelTest, EnumValues) {
	EXPECT_NE(static_cast<int>(SmsModel::Sms), static_cast<int>(SmsModel::GameGear));
	EXPECT_NE(static_cast<int>(SmsModel::Sms), static_cast<int>(SmsModel::Sg));
	EXPECT_NE(static_cast<int>(SmsModel::Sms), static_cast<int>(SmsModel::ColecoVision));
	EXPECT_NE(static_cast<int>(SmsModel::GameGear), static_cast<int>(SmsModel::Sg));
}

// =============================================================================
// SMS IRQ Source
// =============================================================================

TEST(SmsIrqTest, IrqSource_None_IsZero) {
	EXPECT_EQ(static_cast<int>(SmsIrqSource::None), 0);
}

TEST(SmsIrqTest, IrqSource_Vdp_IsOne) {
	EXPECT_EQ(static_cast<int>(SmsIrqSource::Vdp), 1);
}
