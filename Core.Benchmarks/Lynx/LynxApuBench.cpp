#include "pch.h"
#include <array>
#include <cstring>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx Audio (APU) Benchmarks
// =============================================================================
// The Lynx APU uses 4 LFSR-based channels driven by Mikey timers.
// Audio clocking happens at the master clock rate (16 MHz), downsampled to
// the output sample rate. Each channel uses configurable feedback taps.

// -----------------------------------------------------------------------------
// LFSR (Linear Feedback Shift Register)
// -----------------------------------------------------------------------------

// Benchmark single LFSR clock step (12-bit shift register)
static void BM_LynxApu_LfsrClock_SingleStep(benchmark::State& state) {
	uint16_t shiftReg = 0x001; // Initial seed (non-zero)
	uint8_t feedbackEnable = 0x3F; // All lower 6 taps enabled

	for (auto _ : state) {
		// XOR feedback from selected taps (bits 0-11, 12 bits)
		uint8_t feedback = 0;
		if (feedbackEnable & 0x01) feedback ^= (shiftReg >> 0) & 1;
		if (feedbackEnable & 0x02) feedback ^= (shiftReg >> 1) & 1;
		if (feedbackEnable & 0x04) feedback ^= (shiftReg >> 4) & 1;
		if (feedbackEnable & 0x08) feedback ^= (shiftReg >> 5) & 1;
		if (feedbackEnable & 0x10) feedback ^= (shiftReg >> 7) & 1;
		if (feedbackEnable & 0x20) feedback ^= (shiftReg >> 10) & 1;
		if (feedbackEnable & 0x40) feedback ^= (shiftReg >> 11) & 1;
		// Shift and insert feedback
		shiftReg = ((shiftReg << 1) | feedback) & 0x0FFF;
		benchmark::DoNotOptimize(shiftReg);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_LfsrClock_SingleStep);

// Benchmark LFSR clock with branchless tap extraction
static void BM_LynxApu_LfsrClock_Branchless(benchmark::State& state) {
	uint16_t shiftReg = 0x001;
	uint8_t feedbackEnable = 0x3F;
	// Pre-compute tap positions for enabled taps
	static constexpr uint8_t tapPositions[7] = { 0, 1, 4, 5, 7, 10, 11 };

	for (auto _ : state) {
		uint8_t feedback = 0;
		for (int i = 0; i < 7; i++) {
			feedback ^= ((shiftReg >> tapPositions[i]) & 1) & ((feedbackEnable >> i) & 1);
		}
		shiftReg = ((shiftReg << 1) | feedback) & 0x0FFF;
		benchmark::DoNotOptimize(shiftReg);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_LfsrClock_Branchless);

// Benchmark LFSR clocking all 4 channels
static void BM_LynxApu_LfsrClock_All4Channels(benchmark::State& state) {
	LynxAudioChannelState channels[4] = {};
	for (int i = 0; i < 4; i++) {
		channels[i].ShiftRegister = static_cast<uint16_t>(0x001 + i * 0x111);
		channels[i].FeedbackEnable = static_cast<uint8_t>(0x3F - i * 8);
		channels[i].Volume = static_cast<uint8_t>(128 + i * 32);
		channels[i].Enabled = true;
	}

	static constexpr uint8_t tapPositions[7] = { 0, 1, 4, 5, 7, 10, 11 };

	for (auto _ : state) {
		for (int ch = 0; ch < 4; ch++) {
			if (!channels[ch].Enabled) continue;
			uint16_t sr = channels[ch].ShiftRegister;
			uint8_t fe = channels[ch].FeedbackEnable;
			uint8_t feedback = 0;
			for (int i = 0; i < 7; i++) {
				feedback ^= ((sr >> tapPositions[i]) & 1) & ((fe >> i) & 1);
			}
			channels[ch].ShiftRegister = ((sr << 1) | feedback) & 0x0FFF;
			// Output: current bit 0 shapes the waveform
			channels[ch].Output = (channels[ch].ShiftRegister & 1)
				? static_cast<int8_t>(channels[ch].Volume)
				: static_cast<int8_t>(-channels[ch].Volume);
		}
		benchmark::DoNotOptimize(channels);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_LynxApu_LfsrClock_All4Channels);

// -----------------------------------------------------------------------------
// Audio Mixing
// -----------------------------------------------------------------------------

// Benchmark 4-channel stereo mix (per sample)
static void BM_LynxApu_StereoMix(benchmark::State& state) {
	LynxAudioChannelState channels[4] = {};
	for (int i = 0; i < 4; i++) {
		channels[i].Output = static_cast<int8_t>(32 + i * 16);
		channels[i].Volume = static_cast<uint8_t>(128 + i * 32);
		channels[i].Attenuation = static_cast<uint8_t>(((15 - i) << 4) | (i * 4));
		channels[i].Enabled = true;
	}

	for (auto _ : state) {
		int16_t left = 0;
		int16_t right = 0;
		for (int ch = 0; ch < 4; ch++) {
			if (!channels[ch].Enabled) continue;
			int16_t sample = channels[ch].Output;
			left += static_cast<int16_t>((sample * (channels[ch].Attenuation & 0xf0)) / (16 * 16));
			right += static_cast<int16_t>((sample * (channels[ch].Attenuation & 0x0f)) / 16);
		}
		benchmark::DoNotOptimize(left);
		benchmark::DoNotOptimize(right);
		// Vary outputs to prevent const-folding
		channels[0].Output ^= 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_StereoMix);

// Benchmark stereo mix when all channels muted (STEREO = 0xFF fast-skip)
static void BM_LynxApu_StereoMix_AllMuted(benchmark::State& state) {
	uint8_t stereo = 0xff; // All channels muted on both sides
	int16_t buffer[2] = {};

	for (auto _ : state) {
		// Mirrors the fast-skip path in LynxApu::MixOutput()
		if (stereo == 0xff) [[unlikely]] {
			buffer[0] = 0;
			buffer[1] = 0;
		}
		benchmark::DoNotOptimize(buffer);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_StereoMix_AllMuted);

// Benchmark mono mix (comparison baseline)
static void BM_LynxApu_MonoMix(benchmark::State& state) {
	LynxAudioChannelState channels[4] = {};
	for (int i = 0; i < 4; i++) {
		channels[i].Output = static_cast<int8_t>(32 + i * 16);
		channels[i].Volume = static_cast<uint8_t>(128 + i * 32);
		channels[i].Enabled = true;
	}

	for (auto _ : state) {
		int16_t output = 0;
		for (int ch = 0; ch < 4; ch++) {
			if (!channels[ch].Enabled) continue;
			output += channels[ch].Output;
		}
		benchmark::DoNotOptimize(output);
		channels[0].Output ^= 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_MonoMix);

// Benchmark audio integration mode (channel N feeds into channel N+1)
static void BM_LynxApu_IntegrationMode(benchmark::State& state) {
	LynxAudioChannelState channels[4] = {};
	for (int i = 0; i < 4; i++) {
		channels[i].Output = static_cast<int8_t>(32);
		channels[i].Volume = static_cast<int8_t>(127);
		channels[i].Integrate = (i > 0); // Channels 1-3 integrate from previous
		channels[i].Enabled = true;
	}

	for (auto _ : state) {
		// Channel 0 generates base waveform
		int8_t prevOutput = channels[0].Output;
		for (int ch = 1; ch < 4; ch++) {
			if (channels[ch].Integrate) {
				// Integration: output = previous output + current value
				int16_t integrated = static_cast<int16_t>(prevOutput) + channels[ch].Output;
				// Clamp to int8 range
				if (integrated > 127) integrated = 127;
				if (integrated < -128) integrated = -128;
				channels[ch].Output = static_cast<int8_t>(integrated);
			}
			prevOutput = channels[ch].Output;
		}
		benchmark::DoNotOptimize(channels);
		channels[0].Output ^= 3;
	}
	state.SetItemsProcessed(state.iterations() * 3); // 3 integration ops
}
BENCHMARK(BM_LynxApu_IntegrationMode);

// -----------------------------------------------------------------------------
// Sample Generation (Buffer Fill)
// -----------------------------------------------------------------------------

// Benchmark generating a buffer of audio samples
static void BM_LynxApu_GenerateSamples(benchmark::State& state) {
	const size_t sampleCount = static_cast<size_t>(state.range(0));
	std::vector<int16_t> buffer(sampleCount * 2); // stereo

	LynxAudioChannelState channels[4] = {};
	for (int i = 0; i < 4; i++) {
		channels[i].ShiftRegister = static_cast<uint16_t>(0x001 + i * 0x111);
		channels[i].FeedbackEnable = static_cast<uint8_t>(0x3F - i * 8);
		channels[i].Volume = static_cast<int8_t>(127);
		channels[i].Attenuation = 0xff; // Full volume both sides
		channels[i].Enabled = true;
		channels[i].Counter = static_cast<uint8_t>(10 + i * 5);
		channels[i].BackupValue = channels[i].Counter;
	}

	static constexpr uint8_t tapPositions[7] = { 0, 1, 4, 5, 7, 10, 11 };

	for (auto _ : state) {
		for (size_t s = 0; s < sampleCount; s++) {
			// Clock channels
			for (int ch = 0; ch < 4; ch++) {
				if (!channels[ch].Enabled) continue;
				channels[ch].Counter--;
				if (channels[ch].Counter == 0) {
					channels[ch].Counter = channels[ch].BackupValue;
					// Clock LFSR
					uint16_t sr = channels[ch].ShiftRegister;
					uint8_t fe = channels[ch].FeedbackEnable;
					uint8_t feedback = 0;
					for (int i = 0; i < 7; i++) {
						feedback ^= ((sr >> tapPositions[i]) & 1) & ((fe >> i) & 1);
					}
					channels[ch].ShiftRegister = ((sr << 1) | feedback) & 0x0FFF;
					channels[ch].Output = (channels[ch].ShiftRegister & 1)
						? static_cast<int8_t>(channels[ch].Volume)
						: static_cast<int8_t>(-channels[ch].Volume);
				}
			}
			// Mix
			int16_t left = 0, right = 0;
			for (int ch = 0; ch < 4; ch++) {
				int16_t sample = channels[ch].Output;
				left += static_cast<int16_t>((sample * (channels[ch].Attenuation & 0xf0)) / (16 * 16));
				right += static_cast<int16_t>((sample * (channels[ch].Attenuation & 0x0f)) / 16);
			}
			buffer[s * 2] = left;
			buffer[s * 2 + 1] = right;
		}
		benchmark::DoNotOptimize(buffer.data());
	}
	state.SetItemsProcessed(state.iterations() * sampleCount);
	state.SetBytesProcessed(state.iterations() * sampleCount * 4); // stereo 16-bit
}
BENCHMARK(BM_LynxApu_GenerateSamples)->Range(256, 4096);

// Benchmark volume attenuation computation (per channel per sample)
static void BM_LynxApu_VolumeAttenuation(benchmark::State& state) {
	int8_t output = 64;
	uint8_t attenuation = 0xc8; // Left=12, Right=8

	for (auto _ : state) {
		int16_t left = static_cast<int16_t>((output * (attenuation & 0xf0)) / (16 * 16));
		int16_t right = static_cast<int16_t>((output * (attenuation & 0x0f)) / 16);
		benchmark::DoNotOptimize(left);
		benchmark::DoNotOptimize(right);
		output ^= 3;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxApu_VolumeAttenuation);

// =============================================================================
// APU Tick (Isolated Timer Processing)
// =============================================================================

// Benchmark the per-channel timer tick loop without MixOutput.
// Simulates TickChannelTimer for all 4 channels over a realistic
// cycle delta (1 frame = ~16,000 CPU cycles at ~1 MHz, ~267 ticks per channel).
static void BM_LynxApu_TickIsolated(benchmark::State& state) {
	// 4-channel timer state: counter, backupValue, lastTick, enabled
	struct ChannelTimerSim {
		uint8_t counter;
		uint8_t backupValue;
		uint64_t lastTick;
		bool enabled;
		uint8_t clockSource; // prescaler index 0-6 (7=linked)
	};

	static constexpr uint32_t prescalerPeriods[7] = {
		1, 2, 4, 8, 16, 64, 256
	};

	ChannelTimerSim channels[4] = {
		{ 0x80, 0x80, 0, true, 2 },  // channel 0: period=4
		{ 0x40, 0x40, 0, true, 3 },  // channel 1: period=8
		{ 0xc0, 0xc0, 0, true, 1 },  // channel 2: period=2
		{ 0x20, 0x20, 0, true, 4 },  // channel 3: period=16
	};

	uint64_t currentCycle = 0;
	uint8_t tickableMask = 0x0f; // all 4 enabled
	uint16_t shiftReg = 0x001;

	for (auto _ : state) {
		currentCycle += 16000; // ~1 frame of CPU cycles

		for (int ch = 0; ch < 4; ch++) {
			if (!(tickableMask & (1 << ch))) {
				channels[ch].lastTick = currentCycle;
				continue;
			}
			auto& c = channels[ch];
			if (!c.enabled || c.clockSource >= 7) {
				c.lastTick = currentCycle;
				continue;
			}
			uint32_t period = prescalerPeriods[c.clockSource];
			while (currentCycle - c.lastTick >= period) {
				c.lastTick += period;
				c.counter--;
				if (c.counter == 0xff) { // underflow
					c.counter = c.backupValue;
					// Simulate LFSR clock (the audio generation step)
					uint8_t fb = ((shiftReg >> 0) ^ (shiftReg >> 1)) & 1;
					shiftReg = ((shiftReg << 1) | fb) & 0x0FFF;
				}
			}
		}
		benchmark::DoNotOptimize(shiftReg);
		benchmark::DoNotOptimize(channels);
	}
	state.SetItemsProcessed(state.iterations() * 4); // 4 channels per tick
}
BENCHMARK(BM_LynxApu_TickIsolated);
