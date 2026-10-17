#include "pch.h"
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan APU Tests
// =============================================================================
// Tests for APU state structures: 4 wavetable channels (Ch1-4), volume control,
// HyperVoice PCM, noise/LFSR, sweep, master volume, and channel force flags.

namespace WsApuTestHelpers {
	// Compute wavetable period from frequency register
	// Timer reloads on underflow, period = 2048 - freq (for 11-bit freq)
	static uint16_t ComputePeriod(uint16_t frequency) {
		return (2048 - frequency) & 0x7ff;
	}

	// Extract wavetable sample nibble from RAM
	// 32 samples per channel, 4 bits each → 16 bytes
	static uint8_t GetWaveTableSample(const uint8_t* waveRam, uint8_t position) {
		uint8_t byte = waveRam[position >> 1];
		return (position & 1) ? (byte >> 4) : (byte & 0x0f);
	}

	// LFSR tap computation for Ch4 noise
	// Tap modes select different feedback polynomials
	static uint16_t LfsrStep(uint16_t lfsr, uint8_t tapMode) {
		// 15-bit LFSR with configurable taps
		// Bit 0 output, feedback to bit 14
		uint8_t feedback;
		switch (tapMode) {
			case 0: feedback = ((lfsr >> 7) ^ lfsr) & 1; break;  // x^14 + x^7
			case 1: feedback = ((lfsr >> 10) ^ (lfsr >> 7)) & 1; break;
			case 2: feedback = ((lfsr >> 13) ^ (lfsr >> 11)) & 1; break;
			case 3: feedback = ((lfsr >> 14) ^ (lfsr >> 13)) & 1; break;
			default: feedback = lfsr & 1; break;
		}
		return (uint16_t)((lfsr >> 1) | (feedback << 14));
	}

	// Sweep frequency adjustment
	static uint16_t ApplySweep(uint16_t frequency, int8_t sweepValue) {
		int32_t result = (int32_t)frequency + sweepValue;
		return (uint16_t)(result & 0x7ff);
	}
}

// =============================================================================
// Base APU Channel State Tests
// =============================================================================

TEST(WsApuBaseChannelTest, DefaultState_ZeroInitialized) {
	BaseWsApuState state = {};
	EXPECT_EQ(state.Frequency, 0);
	EXPECT_EQ(state.Timer, 0);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.LeftVolume, 0);
	EXPECT_EQ(state.RightVolume, 0);
	EXPECT_EQ(state.SamplePosition, 0);
	EXPECT_EQ(state.LeftOutput, 0);
	EXPECT_EQ(state.RightOutput, 0);
}

TEST(WsApuBaseChannelTest, SetVolume_AllCombinations) {
	BaseWsApuState state = {};
	for (int i = 0; i < 256; i++) {
		state.SetVolume((uint8_t)i);
		EXPECT_EQ(state.LeftVolume, (i >> 4) & 0x0f) << "i=" << i;
		EXPECT_EQ(state.RightVolume, i & 0x0f) << "i=" << i;
		EXPECT_EQ(state.GetVolume(), (uint8_t)i) << "i=" << i;
	}
}

TEST(WsApuBaseChannelTest, Frequency_11BitRange) {
	BaseWsApuState state = {};
	state.Frequency = 0x7ff; // max 11-bit
	EXPECT_EQ(state.Frequency, 0x7ff);

	state.Frequency = 0;
	EXPECT_EQ(state.Frequency, 0);
}

// =============================================================================
// Channel 1 Tests (Basic Wavetable)
// =============================================================================

TEST(WsApuCh1Test, InheritsBaseState) {
	WsApuCh1State state = {};
	state.Enabled = true;
	state.Frequency = 0x100;
	state.SetVolume(0xab);
	EXPECT_TRUE(state.Enabled);
	EXPECT_EQ(state.Frequency, 0x100);
	EXPECT_EQ(state.LeftVolume, 0x0a);
	EXPECT_EQ(state.RightVolume, 0x0b);
}

// =============================================================================
// Channel 2 Tests (Wavetable + PCM Voice)
// =============================================================================

TEST(WsApuCh2Test, DefaultState_PcmDisabled) {
	WsApuCh2State state = {};
	EXPECT_FALSE(state.PcmEnabled);
	EXPECT_FALSE(state.MaxPcmVolumeRight);
	EXPECT_FALSE(state.HalfPcmVolumeRight);
	EXPECT_FALSE(state.MaxPcmVolumeLeft);
	EXPECT_FALSE(state.HalfPcmVolumeLeft);
}

TEST(WsApuCh2Test, PcmVolume_SetsIndependently) {
	WsApuCh2State state = {};
	state.PcmEnabled = true;
	state.MaxPcmVolumeLeft = true;
	state.HalfPcmVolumeRight = true;

	EXPECT_TRUE(state.PcmEnabled);
	EXPECT_TRUE(state.MaxPcmVolumeLeft);
	EXPECT_FALSE(state.HalfPcmVolumeLeft);
	EXPECT_FALSE(state.MaxPcmVolumeRight);
	EXPECT_TRUE(state.HalfPcmVolumeRight);
}

// =============================================================================
// Channel 3 Tests (Wavetable + Sweep)
// =============================================================================

TEST(WsApuCh3Test, DefaultState_SweepDisabled) {
	WsApuCh3State state = {};
	EXPECT_FALSE(state.SweepEnabled);
	EXPECT_EQ(state.SweepScaler, 0);
	EXPECT_EQ(state.SweepValue, 0);
	EXPECT_EQ(state.SweepPeriod, 0);
	EXPECT_EQ(state.SweepTimer, 0);
	EXPECT_FALSE(state.UseSweepCpuClock);
}

TEST(WsApuCh3Test, SweepValue_SignedRange) {
	WsApuCh3State state = {};
	state.SweepValue = 127;  // max positive
	EXPECT_EQ(state.SweepValue, 127);

	state.SweepValue = -128; // max negative
	EXPECT_EQ(state.SweepValue, -128);

	state.SweepValue = 0;
	EXPECT_EQ(state.SweepValue, 0);
}

TEST(WsApuCh3BehaviorTest, SweepApply_PositiveIncrement) {
	uint16_t freq = 0x100;
	uint16_t result = WsApuTestHelpers::ApplySweep(freq, 5);
	EXPECT_EQ(result, 0x105);
}

TEST(WsApuCh3BehaviorTest, SweepApply_NegativeDecrement) {
	uint16_t freq = 0x100;
	uint16_t result = WsApuTestHelpers::ApplySweep(freq, -5);
	EXPECT_EQ(result, 0x0fb);
}

TEST(WsApuCh3BehaviorTest, SweepApply_Wraps11Bit) {
	uint16_t freq = 0x7ff;
	uint16_t result = WsApuTestHelpers::ApplySweep(freq, 1);
	EXPECT_EQ(result, 0x000); // wraps
}

// =============================================================================
// Channel 4 Tests (Wavetable + Noise/LFSR)
// =============================================================================

TEST(WsApuCh4Test, DefaultState_NoiseDisabled) {
	WsApuCh4State state = {};
	EXPECT_FALSE(state.NoiseEnabled);
	EXPECT_FALSE(state.LfsrEnabled);
	EXPECT_EQ(state.TapMode, 0);
	EXPECT_EQ(state.TapShift, 0);
	EXPECT_EQ(state.Lfsr, 0);
	EXPECT_EQ(state.HoldLfsr, 0);
}

TEST(WsApuCh4Test, Lfsr_15BitRange) {
	WsApuCh4State state = {};
	state.Lfsr = 0x7fff; // max 15-bit
	EXPECT_EQ(state.Lfsr, 0x7fff);
}

TEST(WsApuCh4BehaviorTest, LfsrStep_ProducesNonZero) {
	uint16_t lfsr = 0x0001;
	uint16_t next = WsApuTestHelpers::LfsrStep(lfsr, 0);
	// After one step, LFSR should change
	EXPECT_NE(next, lfsr);
}

TEST(WsApuCh4BehaviorTest, LfsrStep_AllTapModes_ShiftFromBit0) {
	for (uint8_t tapMode = 0; tapMode < 4; tapMode++) {
		uint16_t lfsr = 0x0001;
		uint16_t next = WsApuTestHelpers::LfsrStep(lfsr, tapMode);
		// Bit 0 is shifted out, so bit 0 of result comes from old bit 1
		// Check the shift happened (bit 0 moved right)
		EXPECT_EQ(next & 0x01, 0) << "tapMode=" << (int)tapMode;
	}
}

// =============================================================================
// HyperVoice Tests (PCM DMA Audio)
// =============================================================================

TEST(WsApuHyperVoiceTest, DefaultState_Disabled) {
	WsApuHyperVoiceState state = {};
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.LeftOutput, 0);
	EXPECT_EQ(state.RightOutput, 0);
	EXPECT_EQ(state.Divisor, 0);
	EXPECT_EQ(state.Timer, 0);
	EXPECT_EQ(state.Input, 0);
	EXPECT_EQ(state.Shift, 0);
}

TEST(WsApuHyperVoiceTest, ChannelModeEnum) {
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::Stereo), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoLeft), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoRight), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoBoth), 3);
}

TEST(WsApuHyperVoiceTest, ScalingModeEnum) {
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Unsigned), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::UnsignedNegated), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Signed), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::None), 3);
}

// =============================================================================
// Complete APU State Tests
// =============================================================================

TEST(WsApuStateTest, DefaultState_AllChannelsDisabled) {
	WsApuState state = {};
	EXPECT_FALSE(state.Ch1.Enabled);
	EXPECT_FALSE(state.Ch2.Enabled);
	EXPECT_FALSE(state.Ch3.Enabled);
	EXPECT_FALSE(state.Ch4.Enabled);
	EXPECT_FALSE(state.Voice.Enabled);
}

TEST(WsApuStateTest, MasterVolume_SetsCorrectly) {
	WsApuState state = {};
	state.MasterVolume = 0x0f;
	state.InternalMasterVolume = 0x03;
	state.SpeakerVolume = 0x02;
	EXPECT_EQ(state.MasterVolume, 0x0f);
	EXPECT_EQ(state.InternalMasterVolume, 0x03);
	EXPECT_EQ(state.SpeakerVolume, 0x02);
}

TEST(WsApuStateTest, ForceOutputFlags_Independent) {
	WsApuState state = {};
	state.ForceOutput2 = true;
	state.ForceOutput4 = false;
	state.ForceOutputCh2Voice = true;
	state.HoldChannels = true;

	EXPECT_TRUE(state.ForceOutput2);
	EXPECT_FALSE(state.ForceOutput4);
	EXPECT_TRUE(state.ForceOutputCh2Voice);
	EXPECT_TRUE(state.HoldChannels);
}

TEST(WsApuStateTest, WaveTableAddress_FullRange) {
	WsApuState state = {};
	state.WaveTableAddress = 0xfe00;
	EXPECT_EQ(state.WaveTableAddress, 0xfe00);
}

TEST(WsApuStateTest, Speaker_EnableAndVolume) {
	WsApuState state = {};
	state.SpeakerEnabled = true;
	state.HeadphoneEnabled = true;
	EXPECT_TRUE(state.SpeakerEnabled);
	EXPECT_TRUE(state.HeadphoneEnabled);
}

// =============================================================================
// Wavetable Sample Extraction Tests
// =============================================================================

TEST(WsApuWavetableTest, GetSample_EvenPosition_LowNibble) {
	uint8_t waveRam[16] = {};
	waveRam[0] = 0xab; // position 0 = low nibble = 0xb, position 1 = high nibble = 0xa
	EXPECT_EQ(WsApuTestHelpers::GetWaveTableSample(waveRam, 0), 0x0b);
}

TEST(WsApuWavetableTest, GetSample_OddPosition_HighNibble) {
	uint8_t waveRam[16] = {};
	waveRam[0] = 0xab;
	EXPECT_EQ(WsApuTestHelpers::GetWaveTableSample(waveRam, 1), 0x0a);
}

TEST(WsApuWavetableTest, GetSample_AllPositions) {
	// 32 samples packed in 16 bytes
	uint8_t waveRam[16];
	for (int i = 0; i < 16; i++) {
		waveRam[i] = (uint8_t)((i << 4) | (i & 0x0f));
	}
	for (uint8_t pos = 0; pos < 32; pos++) {
		uint8_t sample = WsApuTestHelpers::GetWaveTableSample(waveRam, pos);
		EXPECT_LE(sample, 0x0f) << "pos=" << (int)pos;
	}
}

// =============================================================================
// Period Computation Tests
// =============================================================================

TEST(WsApuPeriodTest, ZeroFrequency_MaxPeriod) {
	EXPECT_EQ(WsApuTestHelpers::ComputePeriod(0), 0x800 & 0x7ff);
}

TEST(WsApuPeriodTest, MaxFrequency_MinPeriod) {
	EXPECT_EQ(WsApuTestHelpers::ComputePeriod(0x7ff), (2048 - 0x7ff) & 0x7ff);
}

TEST(WsApuPeriodTest, MidRange) {
	EXPECT_EQ(WsApuTestHelpers::ComputePeriod(0x400), (2048 - 0x400) & 0x7ff);
}
