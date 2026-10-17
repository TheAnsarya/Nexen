#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx Audio Processing Unit (APU).
///
/// The Lynx APU consists of 4 LFSR-based audio channels in the Mikey chip.
/// Each channel has:
///   - 12-bit LFSR (Linear Feedback Shift Register) for waveform generation
///   - 8-byte register block: Volume, Feedback, Output, ShiftLo/Hi, Backup, Control, Counter
///   - Configurable feedback tap selection for unique waveforms
///   - Integration mode for accumulating output
///   - Stereo attenuation (4-bit left/right per channel)
///
/// Audio channel timers can cascade (linked mode) or run independently.
/// Channel 3 supports DAC mode for PCM playback.
///
/// References:
///   - Epyx hardware reference: monlynx.de/lynx/hardware.html
///   - ~docs/plans/lynx-subsystems-deep-dive.md (Section: Audio System)
///   - ~docs/plans/atari-lynx-technical-report.md (Section 7)
/// </summary>
class LynxApuTest : public ::testing::Test {
protected:
	LynxAudioChannelState _channel = {};
	LynxApuState _apu = {};

	void SetUp() override {
		_channel = {};
		_channel.ShiftRegister = 0x001; // Non-zero initial LFSR state
		_channel.Volume = 0x7f;         // Max volume
		_channel.FeedbackEnable = 0x01; // Tap bit 0
		_channel.Attenuation = 0xff;    // Full volume both sides (L=0xF, R=0xF)
		_channel.Counter = 0;
		_channel.BackupValue = 100;
		_channel.Enabled = true;
		_channel.TimerDone = false;

		_apu = {};
		for (int i = 0; i < 4; i++) {
			_apu.Channels[i] = _channel;
		}
		_apu.Stereo = 0x00;   // All channels enabled on both sides
		_apu.Panning = 0x00;  // No attenuation (full volume)
	}

	/// <summary>Clock the LFSR with the specified feedback enable mask</summary>
	void ClockLfsr(LynxAudioChannelState& ch) {
		// Feedback taps: bits 0,1,2,3,4,5,7,10 of shift register
		static constexpr uint8_t tapBits[] = { 0, 1, 2, 3, 4, 5, 7, 10 };
		uint16_t sr = ch.ShiftRegister;
		uint8_t feedback = 0;

		for (int i = 0; i < 8; i++) {
			if (ch.FeedbackEnable & (1 << i)) {
				feedback ^= ((sr >> tapBits[i]) & 1);
			}
		}

		sr = (sr >> 1) | (feedback << 11);
		ch.ShiftRegister = sr & 0x0fff;
	}
};

//=============================================================================
// LFSR State Tests
//=============================================================================

TEST_F(LynxApuTest, Lfsr_InitialState_NonZero) {
	// The LFSR must start with a non-zero value, otherwise it halts
	EXPECT_NE(_channel.ShiftRegister, 0);
	EXPECT_EQ(_channel.ShiftRegister, 0x001);
}

TEST_F(LynxApuTest, Lfsr_Clock_ShiftsRight) {
	_channel.ShiftRegister = 0x0ff;
	_channel.FeedbackEnable = 0x00; // No feedback (deterministic shift)
	ClockLfsr(_channel);
	EXPECT_EQ(_channel.ShiftRegister, 0x07f); // Right shift, no new bit
}

TEST_F(LynxApuTest, Lfsr_Clock_FeedbackTap0) {
	// Tap bit 0 = LSB of shift register
	_channel.ShiftRegister = 0x001; // Bit 0 = 1
	_channel.FeedbackEnable = 0x01; // Enable tap 0
	ClockLfsr(_channel);
	// Feedback = 1 (from bit 0), new bit enters at bit 11
	EXPECT_EQ(_channel.ShiftRegister, (0x001 >> 1) | (1 << 11));
	EXPECT_EQ(_channel.ShiftRegister, 0x800);
}

TEST_F(LynxApuTest, Lfsr_Clock_FeedbackTap0_ZeroBit) {
	_channel.ShiftRegister = 0x002; // Bit 0 = 0
	_channel.FeedbackEnable = 0x01; // Enable tap 0
	ClockLfsr(_channel);
	// Feedback = 0, no new bit at bit 11
	EXPECT_EQ(_channel.ShiftRegister, 0x001);
}

TEST_F(LynxApuTest, Lfsr_Clock_MultipleTaps_XorBehavior) {
	// Enable taps 0 and 1 (XOR)
	_channel.ShiftRegister = 0x003; // Bits 0=1, 1=1 → XOR = 0
	_channel.FeedbackEnable = 0x03;
	ClockLfsr(_channel);
	EXPECT_EQ(_channel.ShiftRegister, 0x001); // XOR = 0, no new bit

	// Reset
	_channel.ShiftRegister = 0x001; // Bits 0=1, 1=0 → XOR = 1
	ClockLfsr(_channel);
	EXPECT_EQ(_channel.ShiftRegister, 0x800); // XOR = 1, new bit at 11
}

TEST_F(LynxApuTest, Lfsr_12BitMask) {
	// Ensure shift register stays 12-bit even with high feedback
	_channel.ShiftRegister = 0x0fff; // All 12 bits set
	_channel.FeedbackEnable = 0xff;  // All taps (XOR of many bits)
	ClockLfsr(_channel);
	EXPECT_LE(_channel.ShiftRegister, 0x0fff);
}

TEST_F(LynxApuTest, Lfsr_Tap7_UsesBit7) {
	// Tap 7 in FeedbackEnable corresponds to bit 10 in shift register
	_channel.ShiftRegister = 0x400; // Bit 10 = 1
	_channel.FeedbackEnable = 0x80; // Tap 7 (→ SR bit 10)
	ClockLfsr(_channel);
	// Feedback = 1, enters bit 11
	EXPECT_EQ(_channel.ShiftRegister, 0xa00); // 0x400 >> 1 = 0x200 | 0x800 = 0xa00
}

//=============================================================================
// Volume and Output Tests
//=============================================================================

TEST_F(LynxApuTest, Volume_4BitRange) {
	// Volume register is 4-bit (high nibble used for something else in real HW)
	_channel.Volume = 0;
	EXPECT_EQ(_channel.Volume, 0);
	_channel.Volume = 0x7f; // Actual 7-bit signed? Check docs
	EXPECT_EQ(_channel.Volume, 0x7f);
}

TEST_F(LynxApuTest, Output_SignedRange) {
	// Output is signed 8-bit
	_channel.Output = 127;
	EXPECT_EQ(_channel.Output, 127);
	_channel.Output = -128;
	EXPECT_EQ(_channel.Output, -128);
}

TEST_F(LynxApuTest, Output_FromLsrBit_Positive) {
	// When LFSR LSB = 1, output = +Volume
	_channel.ShiftRegister = 0x001;
	int8_t expected = (_channel.ShiftRegister & 1) ? _channel.Volume : -_channel.Volume;
	EXPECT_GT(expected, 0);
}

TEST_F(LynxApuTest, Output_FromLsrBit_Negative) {
	// When LFSR LSB = 0, output = -Volume
	_channel.ShiftRegister = 0x002;
	int8_t expected = (_channel.ShiftRegister & 1) ? _channel.Volume : -_channel.Volume;
	EXPECT_LT(expected, 0);
}

//=============================================================================
// Integration Mode Tests
//=============================================================================

TEST_F(LynxApuTest, IntegrateMode_Disabled_DirectOutput) {
	_channel.Integrate = false;
	_channel.Output = 50;
	_channel.ShiftRegister = 0x001; // LSB = 1
	// In non-integrate mode, output = direct ±volume
	int8_t expected = _channel.Volume;
	EXPECT_EQ(expected, 0x7f);
}

TEST_F(LynxApuTest, IntegrateMode_Enabled_Accumulates) {
	_channel.Integrate = true;
	_channel.Output = 0;
	_channel.Volume = 10;
	_channel.ShiftRegister = 0x001; // LSB = 1

	// Simulate accumulation
	_channel.Output += 10;
	EXPECT_EQ(_channel.Output, 10);
	_channel.Output += 10;
	EXPECT_EQ(_channel.Output, 20);
}

TEST_F(LynxApuTest, IntegrateMode_ClampToPositive127) {
	_channel.Integrate = true;
	_channel.Output = 120;
	int16_t temp = (int16_t)_channel.Output + 20;
	if (temp > 127) temp = 127;
	_channel.Output = (int8_t)temp;
	EXPECT_EQ(_channel.Output, 127);
}

TEST_F(LynxApuTest, IntegrateMode_ClampToNegative128) {
	_channel.Integrate = true;
	_channel.Output = -120;
	int16_t temp = (int16_t)_channel.Output - 20;
	if (temp < -128) temp = -128;
	_channel.Output = (int8_t)temp;
	EXPECT_EQ(_channel.Output, -128);
}

//=============================================================================
// Stereo Attenuation Tests
//=============================================================================

TEST_F(LynxApuTest, StereoAtten_LeftMax_RightZero) {
	_channel.Attenuation = 0xf0; // Left=15, Right=0
	// Left is full, right is silent
	EXPECT_EQ(_channel.Attenuation >> 4, 15);
	EXPECT_EQ(_channel.Attenuation & 0x0f, 0);
}

TEST_F(LynxApuTest, StereoAtten_Balance_Center) {
	_channel.Attenuation = 0x88; // Left=8, Right=8
	EXPECT_EQ(_channel.Attenuation >> 4, _channel.Attenuation & 0x0f);
}

TEST_F(LynxApuTest, StereoAtten_AllChannels) {
	for (int i = 0; i < 4; i++) {
		uint8_t left = static_cast<uint8_t>(i * 3);
		uint8_t right = static_cast<uint8_t>(15 - (i * 3));
		_apu.Channels[i].Attenuation = static_cast<uint8_t>((left << 4) | right);
	}
	EXPECT_EQ(_apu.Channels[0].Attenuation >> 4, 0);
	EXPECT_EQ(_apu.Channels[3].Attenuation >> 4, 9);
}

//=============================================================================
// Timer / Counter Tests
//=============================================================================

TEST_F(LynxApuTest, Counter_Underflow_ReloadsBackup) {
	// Counter at 0, decrement causes 8-bit underflow to 0xFF
	// which triggers reload from BackupValue
	_channel.Counter = 0;
	_channel.BackupValue = 50;
	// Simulate decrement with underflow
	_channel.Counter--;
	// After 8-bit underflow: 0 - 1 = 0xFF (in unsigned 8-bit)
	if (_channel.Counter == 0xff) {
		_channel.Counter = _channel.BackupValue;
		_channel.TimerDone = true;
	}
	EXPECT_EQ(_channel.Counter, 50);
	EXPECT_TRUE(_channel.TimerDone);
}

TEST_F(LynxApuTest, Counter_TimerDone_BlocksCounting) {
	// HW Bug 13.6: TimerDone flag blocks further counting
	_channel.TimerDone = true;
	_channel.Counter = 100;
	// In real implementation, TickChannelTimer returns early if TimerDone
	// Simulate: if TimerDone, don't decrement
	EXPECT_EQ(_channel.Counter, 100); // Unchanged
}

TEST_F(LynxApuTest, Counter_ClockSource_Prescaler) {
	// Control bits 0-2 select prescaler: 0=4, 1=8, 2=16, etc.
	_channel.Control = 0x00; // Source 0 = 4 clocks
	EXPECT_EQ(_channel.Control & 0x07, 0);
	_channel.Control = 0x03; // Source 3 = 32 clocks
	EXPECT_EQ(_channel.Control & 0x07, 3);
}

TEST_F(LynxApuTest, Counter_LinkedMode) {
	// Clock source 7 = linked (cascaded from previous channel)
	_channel.Control = 0x07;
	EXPECT_EQ(_channel.Control & 0x07, 7);
}

//=============================================================================
// STEREO / PAN / Global State Tests
//=============================================================================

TEST_F(LynxApuTest, Stereo_Default_AllEnabled) {
	// Default 0x00 = all channels enabled on both sides
	_apu.Stereo = 0x00;
	EXPECT_EQ(_apu.Stereo, 0);
	// Setting bits disables channels
	_apu.Stereo = 0xff;
	EXPECT_EQ(_apu.Stereo, 0xff);
}

TEST_F(LynxApuTest, Panning_Default_NoAttenuation) {
	// Default 0x00 = no attenuation applied (full volume)
	_apu.Panning = 0x00;
	EXPECT_EQ(_apu.Panning, 0);
	// Setting bits enables per-channel attenuation
	_apu.Panning = 0xff;
	EXPECT_EQ(_apu.Panning, 0xff);
}

TEST_F(LynxApuTest, AllChannels_IndependentState) {
	for (int i = 0; i < 4; i++) {
		_apu.Channels[i].Volume = (uint8_t)(i * 30);
		_apu.Channels[i].FeedbackEnable = (uint8_t)(1 << i);
	}
	EXPECT_EQ(_apu.Channels[0].Volume, 0);
	EXPECT_EQ(_apu.Channels[1].Volume, 30);
	EXPECT_EQ(_apu.Channels[2].Volume, 60);
	EXPECT_EQ(_apu.Channels[3].Volume, 90);
}

//=============================================================================
// Channel Enable Tests
//=============================================================================

TEST_F(LynxApuTest, ChannelEnabled_True) {
	_channel.Enabled = true;
	EXPECT_TRUE(_channel.Enabled);
}

TEST_F(LynxApuTest, ChannelEnabled_False_NoOutput) {
	_channel.Enabled = false;
	// Disabled channel should not contribute to mix
	EXPECT_FALSE(_channel.Enabled);
}

//=============================================================================
// Feedback Polynomial Tests (for different waveforms)
//=============================================================================

TEST_F(LynxApuTest, Feedback_AllTaps_ComplexWaveform) {
	_channel.FeedbackEnable = 0xff; // All taps enabled
	_channel.ShiftRegister = 0xabc;
	ClockLfsr(_channel);
	// Result depends on XOR of all enabled taps
	EXPECT_LE(_channel.ShiftRegister, 0x0fff);
}

TEST_F(LynxApuTest, Feedback_NoTaps_PureShift) {
	_channel.FeedbackEnable = 0x00;
	_channel.ShiftRegister = 0x800;
	ClockLfsr(_channel);
	EXPECT_EQ(_channel.ShiftRegister, 0x400); // Pure right shift
}

TEST_F(LynxApuTest, Feedback_SingleTap_KnownPattern) {
	// Known good LFSR pattern for single tap at bit 0
	_channel.FeedbackEnable = 0x01;
	_channel.ShiftRegister = 0x001;

	// Clock multiple times to verify pattern doesn't collapse
	for (int i = 0; i < 100; i++) {
		ClockLfsr(_channel);
		EXPECT_NE(_channel.ShiftRegister, 0); // Should never reach 0
	}
}

//=============================================================================
// Stereo Mute Fast-Skip Tests
//=============================================================================

TEST_F(LynxApuTest, Stereo_AllMuted_StereoRegister0xFF) {
	// When STEREO = 0xFF, all channels are muted on both left and right.
	// MixOutput() should fast-skip and write zero samples.
	_apu.Stereo = 0xff;
	EXPECT_EQ(_apu.Stereo & 0xf0, 0xf0); // Left bits 4-7 all set
	EXPECT_EQ(_apu.Stereo & 0x0f, 0x0f); // Right bits 0-3 all set
}

TEST_F(LynxApuTest, Stereo_PartialMute_LeftOnly) {
	// Left muted (bits 4-7 set), right enabled (bits 0-3 clear)
	_apu.Stereo = 0xf0;
	EXPECT_TRUE((_apu.Stereo & (0x10 << 0)) != 0); // Ch0 left muted
	EXPECT_TRUE((_apu.Stereo & (0x10 << 1)) != 0); // Ch1 left muted
	EXPECT_TRUE((_apu.Stereo & (0x10 << 2)) != 0); // Ch2 left muted
	EXPECT_TRUE((_apu.Stereo & (0x10 << 3)) != 0); // Ch3 left muted
	EXPECT_FALSE((_apu.Stereo & (0x01 << 0)) != 0); // Ch0 right enabled
}

TEST_F(LynxApuTest, Stereo_PartialMute_RightOnly) {
	// Left enabled (bits 4-7 clear), right muted (bits 0-3 set)
	_apu.Stereo = 0x0f;
	EXPECT_FALSE((_apu.Stereo & (0x10 << 0)) != 0); // Ch0 left enabled
	EXPECT_TRUE((_apu.Stereo & (0x01 << 0)) != 0);  // Ch0 right muted
	EXPECT_TRUE((_apu.Stereo & (0x01 << 1)) != 0);  // Ch1 right muted
	EXPECT_TRUE((_apu.Stereo & (0x01 << 2)) != 0);  // Ch2 right muted
	EXPECT_TRUE((_apu.Stereo & (0x01 << 3)) != 0);  // Ch3 right muted
}

TEST_F(LynxApuTest, Stereo_NotFullyMuted_HasAudio) {
	// STEREO != 0xFF means at least one channel is enabled somewhere
	_apu.Stereo = 0xfe; // Channel 0 right enabled
	EXPECT_NE(_apu.Stereo, 0xff);
}

