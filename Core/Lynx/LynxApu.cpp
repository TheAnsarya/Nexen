#include "pch.h"
#include "Lynx/LynxApu.h"
#include "Lynx/LynxConsole.h"
#include "Shared/Emulator.h"
#include "Shared/Audio/SoundMixer.h"
#include "Utilities/Serializer.h"

LynxApu::LynxApu(Emulator* emu, LynxConsole* console)
	: _emu(emu), _console(console) {
	_soundMixer = emu->GetSoundMixer();
	_soundBuffer = std::make_unique<int16_t[]>(MaxSamples * 2); // Stereo interleaved
}

void LynxApu::Init() {
	_state = {};
	_sampleCount = 0;
	_lastSampleCycle = 0;

	// Initialize channels to default state
	for (int i = 0; i < 4; i++) {
		_state.Channels[i] = {};
		_state.Channels[i].ShiftRegister = 0x001; // Non-zero initial LFSR
		_state.Channels[i].Attenuation = 0xff;    // Full volume (both nibbles = 0xF)
	}

	_state.Stereo = 0x00;  // All channels enabled on both sides
	_state.Panning = 0x00; // No attenuation applied (full volume)
}

void LynxApu::Tick(uint64_t currentCycle) {
	// Clock each audio channel's own timer using actual CPU cycle count.
	// This matches how Mikey system timers work: elapsed = currentCycle - LastTick.
	for (int ch = 0; ch < 4; ch++) {
		TickChannelTimer(ch, currentCycle);
	}

	// Generate audio samples at the target sample rate
	while (currentCycle - _lastSampleCycle >= CpuCyclesPerSample) {
		_lastSampleCycle += CpuCyclesPerSample;
		MixOutput();
	}
}

void LynxApu::TickChannelTimer(int ch, uint64_t currentCycle) {
	LynxAudioChannelState& channel = _state.Channels[ch];

	if (!channel.Enabled || channel.TimerDone) {
		// Advance LastTick so we don't accumulate a huge delta
		channel.LastTick = currentCycle;
		return;
	}

	// Check clock source — bits 0-2 of Control register
	uint8_t clockSource = channel.Control & 0x07;
	if (clockSource == 7) {
		// Linked timer — clocked by cascade from previous channel, not master clock
		return;
	}

	uint32_t period = _prescalerPeriods[clockSource];
	if (period == 0) {
		return;
	}

	// Process all elapsed ticks using actual CPU cycle delta
	while (currentCycle - channel.LastTick >= period) {
		channel.LastTick += period;

		// Decrement counter
		channel.Counter--;

		if (channel.Counter == 0xff) { // Underflow (wrapped from 0 to 0xFF)
			channel.TimerDone = true;
			channel.Counter = channel.BackupValue;

			// Clock the LFSR (actual audio generation)
			ClockChannel(ch);

			// Cascade to next linked audio channel
			CascadeAudioChannel(ch);

			// Stop counting while done flag is set (matching Mikey HW Bug 13.6)
			break;
		}
	}
}

void LynxApu::CascadeAudioChannel(int sourceChannel) {
	int target = sourceChannel + 1;
	if (target >= 4) {
		return;
	}

	LynxAudioChannelState& channel = _state.Channels[target];

	// Only cascade if target is linked (clock source 7) and enabled
	if (!channel.Enabled || (channel.Control & 0x07) != 7 || channel.TimerDone) {
		return;
	}

	channel.Counter--;
	if (channel.Counter == 0xff) { // Underflow
		channel.TimerDone = true;
		channel.Counter = channel.BackupValue;

		ClockChannel(target);
		CascadeAudioChannel(target);
	}
}

void LynxApu::ClockChannel(int ch) {
	LynxAudioChannelState& channel = _state.Channels[ch];

	// Clock the 12-bit LFSR (linear feedback shift register)
	// Feedback taps are selected by FeedbackEnable register
	// The feedback value is XOR of selected shift register bits
	uint16_t sr = channel.ShiftRegister;
	uint8_t feedback = 0;

	// Each bit in FeedbackEnable selects a tap on the shift register
	// Taps: bits 0,1,2,3,4,5,7,10 of the shift register correspond to
	// bits 0,1,2,3,4,5,6,7 of the feedback enable register
	static constexpr uint8_t tapBits[] = { 0, 1, 2, 3, 4, 5, 7, 10 };
	for (int i = 0; i < 8; i++) {
		if (channel.FeedbackEnable & (1 << i)) {
			feedback ^= ((sr >> tapBits[i]) & 1);
		}
	}

	// Shift register: shift right, new bit enters at bit 11
	sr = (sr >> 1) | (feedback << 11);
	channel.ShiftRegister = sr & 0x0fff; // Keep 12 bits

	// Output value depends on the low bit of the shift register
	// In integration mode, the output accumulates
	if (channel.Integrate) {
		channel.Output += (sr & 1) ? channel.Volume : static_cast<int8_t>(-channel.Volume);
		// Clamp to signed 8-bit range
		if (channel.Output > 127) channel.Output = 127;
		if (channel.Output < -128) channel.Output = -128;
	} else {
		channel.Output = (sr & 1) ? channel.Volume : static_cast<int8_t>(-channel.Volume);
	}
}

void LynxApu::MixOutput() {
	int32_t leftSum = 0;
	int32_t rightSum = 0;

	// Mixing logic matches Handy's UpdateSound():
	// - STEREO register: per-channel enable (0 = enabled, 1 = disabled)
	//   Bits 7-4 control left side, bits 3-0 control right side
	// - PAN register: per-channel attenuation enable (same bit layout)
	//   When PAN bit is set, ATTEN attenuation is applied
	//   When PAN bit is clear, channel plays at full volume
	// - ATTEN registers: per-channel stereo attenuation
	//   Upper nibble = left (0-15), lower nibble = right (0-15)
	for (int ch = 0; ch < 4; ch++) {
		LynxAudioChannelState& channel = _state.Channels[ch];
		int32_t sample = channel.Output;

		// Left channel: enabled when STEREO bit (0x10 << ch) is NOT set
		if (!(_state.Stereo & (0x10 << ch))) {
			if (_state.Panning & (0x10 << ch)) {
				// PAN enabled: apply left attenuation (upper nibble)
				leftSum += (sample * (channel.Attenuation & 0xf0)) / (16 * 16);
			} else {
				// PAN disabled: full volume
				leftSum += sample;
			}
		}

		// Right channel: enabled when STEREO bit (0x01 << ch) is NOT set
		if (!(_state.Stereo & (0x01 << ch))) {
			if (_state.Panning & (0x01 << ch)) {
				// PAN enabled: apply right attenuation (lower nibble)
				rightSum += (sample * (channel.Attenuation & 0x0f)) / 16;
			} else {
				// PAN disabled: full volume
				rightSum += sample;
			}
		}
	}

	// Scale to 16-bit range — 4 channels of ±127 = ±508, ×64 ≈ ±32512
	leftSum = std::clamp(leftSum * 64, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX));
	rightSum = std::clamp(rightSum * 64, static_cast<int32_t>(INT16_MIN), static_cast<int32_t>(INT16_MAX));

	_soundBuffer[_sampleCount * 2] = static_cast<int16_t>(leftSum);
	_soundBuffer[_sampleCount * 2 + 1] = static_cast<int16_t>(rightSum);
	_sampleCount++;

	if (_sampleCount >= MaxSamples) {
		PlayQueuedAudio();
	}
}

void LynxApu::PlayQueuedAudio() {
	_soundMixer->PlayAudioBuffer(_soundBuffer.get(), _sampleCount, SampleRate);
	_sampleCount = 0;
}

void LynxApu::EndFrame() {
	if (_sampleCount > 0) {
		PlayQueuedAudio();
	}
}

uint8_t LynxApu::ReadRegister(uint8_t addr) {
	// $FD20-$FD3F: Channel registers (4 channels × 8 bytes)
	if (addr < 0x20) {
		int ch = (addr >> 3) & 0x03;
		int reg = addr & 0x07;
		LynxAudioChannelState& channel = _state.Channels[ch];

		switch (reg) {
			case 0: return static_cast<uint8_t>(channel.Volume);
			case 1: return channel.FeedbackEnable;
			case 2: return static_cast<uint8_t>(channel.Output);
			case 3: return static_cast<uint8_t>(channel.ShiftRegister & 0xff);
			case 4: return static_cast<uint8_t>(channel.ShiftRegister >> 8);
			case 5: return channel.BackupValue;
			case 6: return channel.Control;
			case 7: return channel.Counter;
		}
	}

	// $FD40-$FD43: ATTEN_A-D — per-channel stereo attenuation (nibble-packed L/R)
	if (addr >= 0x20 && addr <= 0x23) {
		return _state.Channels[addr - 0x20].Attenuation;
	}

	// $FD48: MPAN — per-channel panning enable
	if (addr == 0x28) {
		return _state.Panning;
	}

	// $FD50: MSTEREO — per-channel stereo enable bitmask
	if (addr == 0x30) {
		return _state.Stereo;
	}

	return 0;
}

void LynxApu::WriteRegister(uint8_t addr, uint8_t value) {
	// $FD20-$FD3F: Channel registers
	if (addr < 0x20) {
		int ch = (addr >> 3) & 0x03;
		int reg = addr & 0x07;
		LynxAudioChannelState& channel = _state.Channels[ch];

		switch (reg) {
			case 0: channel.Volume = static_cast<int8_t>(value); break; // Full 8-bit signed volume
			case 1: channel.FeedbackEnable = value; break;
			case 2: channel.Output = static_cast<int8_t>(value); break;
			case 3: channel.ShiftRegister = (channel.ShiftRegister & 0xf00) | value; break;
			case 4: channel.ShiftRegister = (channel.ShiftRegister & 0x0ff) | ((value & 0x0f) << 8); break;
			case 5: channel.BackupValue = value; break;
			case 6:
				channel.Control = value;
				channel.Enabled = (value & 0x08) != 0; // Bit 3: enable count
				channel.Integrate = (value & 0x20) != 0; // Bit 5: integration mode
				// Bit 6: reset timer done — self-clearing
				if (value & 0x40) {
					channel.TimerDone = false;
					channel.Counter = channel.BackupValue;
				}
				break;
			case 7: channel.Counter = value; break;
		}
		return;
	}

	// $FD40-$FD43: ATTEN_A-D — per-channel stereo attenuation (nibble-packed L/R)
	if (addr >= 0x20 && addr <= 0x23) {
		_state.Channels[addr - 0x20].Attenuation = value;
		return;
	}

	// $FD48: MPAN — per-channel panning enable
	if (addr == 0x28) {
		_state.Panning = value;
		return;
	}

	// $FD50: MSTEREO — per-channel stereo enable bitmask
	if (addr == 0x30) {
		_state.Stereo = value;
		return;
	}
}

void LynxApu::Serialize(Serializer& s) {
	for (int i = 0; i < 4; i++) {
		SVI(_state.Channels[i].Volume);
		SVI(_state.Channels[i].FeedbackEnable);
		SVI(_state.Channels[i].Output);
		SVI(_state.Channels[i].ShiftRegister);
		SVI(_state.Channels[i].BackupValue);
		SVI(_state.Channels[i].Control);
		SVI(_state.Channels[i].Counter);
		SVI(_state.Channels[i].Attenuation);
		SVI(_state.Channels[i].Integrate);
		SVI(_state.Channels[i].Enabled);
		SVI(_state.Channels[i].TimerDone);
		SVI(_state.Channels[i].LastTick);
	}
	SV(_state.Stereo);
	SV(_state.Panning);

	SV(_sampleCount);
	SV(_lastSampleCycle);
}
