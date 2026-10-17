#include "pch.h"
#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <random>

// =============================================================================
// Audio DSP Benchmarks
// =============================================================================
// Benchmarks for audio signal processing operations used across all emulators.
// Audio processing is performance-critical as it runs at 44100-48000 Hz sample rates.

// -----------------------------------------------------------------------------
// Helper Functions and Constants
// -----------------------------------------------------------------------------

constexpr size_t kSmallBufferSize = 256;    // ~5ms at 48000 Hz
constexpr size_t kMediumBufferSize = 2048;  // ~42ms at 48000 Hz
constexpr size_t kLargeBufferSize = 8192;   // ~170ms at 48000 Hz

/// <summary>Generate test audio samples (sine wave)</summary>
static void GenerateSineWave(int16_t* buffer, size_t count, double frequency, double sampleRate) {
	constexpr double amplitude = 16000.0;
	const double omega = 2.0 * 3.14159265358979323846 * frequency / sampleRate;
	for (size_t i = 0; i < count; i++) {
		buffer[i] = static_cast<int16_t>(amplitude * std::sin(omega * static_cast<double>(i)));
	}
}

/// <summary>Generate stereo samples (interleaved L/R)</summary>
static void GenerateStereoSineWave(int16_t* buffer, size_t samplePairs, double freqL, double freqR, double sampleRate) {
	constexpr double amplitude = 16000.0;
	const double omegaL = 2.0 * 3.14159265358979323846 * freqL / sampleRate;
	const double omegaR = 2.0 * 3.14159265358979323846 * freqR / sampleRate;
	for (size_t i = 0; i < samplePairs; i++) {
		buffer[i * 2] = static_cast<int16_t>(amplitude * std::sin(omegaL * static_cast<double>(i)));
		buffer[i * 2 + 1] = static_cast<int16_t>(amplitude * std::sin(omegaR * static_cast<double>(i)));
	}
}

/// <summary>Generate random noise samples</summary>
[[maybe_unused]] static void GenerateNoise(int16_t* buffer, size_t count) {
	std::mt19937 rng(42);
	std::uniform_int_distribution<int16_t> dist(-16000, 16000);
	for (size_t i = 0; i < count; i++) {
		buffer[i] = dist(rng);
	}
}

// -----------------------------------------------------------------------------
// Sample Format Conversion Benchmarks
// -----------------------------------------------------------------------------

// Benchmark converting float samples to int16
static void BM_Audio_FloatToInt16(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<float> input(count);
	std::vector<int16_t> output(count);

	// Generate normalized float samples
	for (size_t i = 0; i < count; i++) {
		input[i] = std::sin(static_cast<float>(i) * 0.01f);
	}

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			output[i] = static_cast<int16_t>(input[i] * 32767.0f);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(float));
}
BENCHMARK(BM_Audio_FloatToInt16)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark converting int16 samples to float
static void BM_Audio_Int16ToFloat(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> input(count);
	std::vector<float> output(count);

	GenerateSineWave(input.data(), count, 440.0, 48000.0);

	for (auto _ : state) {
		constexpr float scale = 1.0f / 32768.0f;
		for (size_t i = 0; i < count; i++) {
			output[i] = static_cast<float>(input[i]) * scale;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_Int16ToFloat)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Stereo Processing Benchmarks
// -----------------------------------------------------------------------------

// Benchmark stereo to mono downmix
static void BM_Audio_StereoToMono(benchmark::State& state) {
	const size_t samplePairs = static_cast<size_t>(state.range(0));
	std::vector<int16_t> stereo(samplePairs * 2);
	std::vector<int16_t> mono(samplePairs);

	GenerateStereoSineWave(stereo.data(), samplePairs, 440.0, 880.0, 48000.0);

	for (auto _ : state) {
		for (size_t i = 0; i < samplePairs; i++) {
			// Average L and R channels
			mono[i] = static_cast<int16_t>((static_cast<int32_t>(stereo[i * 2]) +
			                                 static_cast<int32_t>(stereo[i * 2 + 1])) >> 1);
		}
		benchmark::DoNotOptimize(mono.data());
	}
	state.SetBytesProcessed(state.iterations() * samplePairs * 2 * sizeof(int16_t));
}
BENCHMARK(BM_Audio_StereoToMono)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark mono to stereo duplication
static void BM_Audio_MonoToStereo(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> mono(count);
	std::vector<int16_t> stereo(count * 2);

	GenerateSineWave(mono.data(), count, 440.0, 48000.0);

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			stereo[i * 2] = mono[i];
			stereo[i * 2 + 1] = mono[i];
		}
		benchmark::DoNotOptimize(stereo.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_MonoToStereo)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark stereo panning (apply volume to L/R independently)
static void BM_Audio_StereoPanning(benchmark::State& state) {
	const size_t samplePairs = static_cast<size_t>(state.range(0));
	std::vector<int16_t> buffer(samplePairs * 2);

	GenerateStereoSineWave(buffer.data(), samplePairs, 440.0, 880.0, 48000.0);

	// Volume: 75% left, 100% right
	constexpr int32_t volL = 192;  // 0.75 * 256
	constexpr int32_t volR = 256;  // 1.0 * 256

	for (auto _ : state) {
		for (size_t i = 0; i < samplePairs; i++) {
			int32_t left = buffer[i * 2];
			int32_t right = buffer[i * 2 + 1];
			buffer[i * 2] = static_cast<int16_t>((left * volL) >> 8);
			buffer[i * 2 + 1] = static_cast<int16_t>((right * volR) >> 8);
		}
		benchmark::DoNotOptimize(buffer.data());
	}
	state.SetBytesProcessed(state.iterations() * samplePairs * 2 * sizeof(int16_t));
}
BENCHMARK(BM_Audio_StereoPanning)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Sample Mixing Benchmarks
// -----------------------------------------------------------------------------

// Benchmark mixing two audio channels (additive)
static void BM_Audio_MixTwoChannels(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> ch1(count);
	std::vector<int16_t> ch2(count);
	std::vector<int16_t> output(count);

	GenerateSineWave(ch1.data(), count, 440.0, 48000.0);
	GenerateSineWave(ch2.data(), count, 880.0, 48000.0);

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t mixed = static_cast<int32_t>(ch1[i]) + static_cast<int32_t>(ch2[i]);
			// Clamp to int16 range
			if (mixed > 32767) mixed = 32767;
			if (mixed < -32768) mixed = -32768;
			output[i] = static_cast<int16_t>(mixed);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * 2 * sizeof(int16_t));
}
BENCHMARK(BM_Audio_MixTwoChannels)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark mixing 8 channels (like SNES DSP voices)
static void BM_Audio_Mix8Channels(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::array<std::vector<int16_t>, 8> channels;
	std::vector<int16_t> output(count);

	for (int ch = 0; ch < 8; ch++) {
		channels[ch].resize(count);
		GenerateSineWave(channels[ch].data(), count, 220.0 * (ch + 1), 48000.0);
	}

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t mixed = 0;
			for (int ch = 0; ch < 8; ch++) {
				mixed += channels[ch][i];
			}
			// Clamp
			if (mixed > 32767) mixed = 32767;
			if (mixed < -32768) mixed = -32768;
			output[i] = static_cast<int16_t>(mixed);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * 8 * sizeof(int16_t));
}
BENCHMARK(BM_Audio_Mix8Channels)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize);

// Benchmark mixing with volume scaling (weighted mix)
static void BM_Audio_MixWithVolume(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> ch1(count);
	std::vector<int16_t> ch2(count);
	std::vector<int16_t> output(count);

	GenerateSineWave(ch1.data(), count, 440.0, 48000.0);
	GenerateSineWave(ch2.data(), count, 880.0, 48000.0);

	// 8.8 fixed-point volumes (256 = 1.0)
	constexpr int32_t vol1 = 204;  // 0.8
	constexpr int32_t vol2 = 128;  // 0.5

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t mixed = ((static_cast<int32_t>(ch1[i]) * vol1) >> 8) +
			                ((static_cast<int32_t>(ch2[i]) * vol2) >> 8);
			// Clamp
			if (mixed > 32767) mixed = 32767;
			if (mixed < -32768) mixed = -32768;
			output[i] = static_cast<int16_t>(mixed);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * 2 * sizeof(int16_t));
}
BENCHMARK(BM_Audio_MixWithVolume)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Resampling Benchmarks (Hermite Interpolation)
// -----------------------------------------------------------------------------

// Benchmark linear interpolation resampling (simple baseline)
static void BM_Audio_ResampleLinear(benchmark::State& state) {
	const size_t inputCount = static_cast<size_t>(state.range(0));
	const double ratio = 44100.0 / 48000.0;  // NES to output rate
	const size_t outputCount = static_cast<size_t>(inputCount / ratio) + 1;

	std::vector<int16_t> input(inputCount);
	std::vector<int16_t> output(outputCount);

	GenerateSineWave(input.data(), inputCount, 440.0, 44100.0);

	for (auto _ : state) {
		double pos = 0.0;
		for (size_t i = 0; i < outputCount && static_cast<size_t>(pos) + 1 < inputCount; i++) {
			size_t idx = static_cast<size_t>(pos);
			double frac = pos - idx;

			// Linear interpolation
			int32_t s0 = input[idx];
			int32_t s1 = input[idx + 1];
			output[i] = static_cast<int16_t>(s0 + static_cast<int32_t>((s1 - s0) * frac));

			pos += ratio;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * inputCount * sizeof(int16_t));
}
BENCHMARK(BM_Audio_ResampleLinear)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark Hermite interpolation (used by Nexen)
static void BM_Audio_ResampleHermite(benchmark::State& state) {
	const size_t inputCount = static_cast<size_t>(state.range(0));
	const double ratio = 32040.0 / 48000.0;  // SNES to output rate
	const size_t outputCount = static_cast<size_t>(inputCount / ratio) + 1;

	std::vector<int16_t> input(inputCount + 4);  // Extra samples for interpolation
	std::vector<int16_t> output(outputCount);

	GenerateSineWave(input.data(), inputCount + 4, 440.0, 32040.0);

	for (auto _ : state) {
		double pos = 2.0;  // Start with 2 samples history
		for (size_t i = 0; i < outputCount && static_cast<size_t>(pos) + 1 < inputCount; i++) {
			size_t idx = static_cast<size_t>(pos);
			double mu = pos - idx;

			// Hermite 4-point interpolation
			double s0 = input[idx - 1];
			double s1 = input[idx];
			double s2 = input[idx + 1];
			double s3 = input[idx + 2];

			double mu2 = mu * mu;
			double mu3 = mu2 * mu;

			double a0 = -0.5 * s0 + 1.5 * s1 - 1.5 * s2 + 0.5 * s3;
			double a1 = s0 - 2.5 * s1 + 2.0 * s2 - 0.5 * s3;
			double a2 = -0.5 * s0 + 0.5 * s2;
			double a3 = s1;

			double result = a0 * mu3 + a1 * mu2 + a2 * mu + a3;

			// Clamp
			if (result > 32767.0) result = 32767.0;
			if (result < -32768.0) result = -32768.0;
			output[i] = static_cast<int16_t>(result);

			pos += ratio;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * inputCount * sizeof(int16_t));
}
BENCHMARK(BM_Audio_ResampleHermite)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// FIR Filter Benchmarks (SNES DSP Echo)
// -----------------------------------------------------------------------------

// Benchmark 8-tap FIR filter (SNES echo)
static void BM_Audio_Fir8Tap(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> input(count + 8);
	std::vector<int16_t> output(count);

	GenerateSineWave(input.data(), count + 8, 440.0, 32040.0);

	// SNES-style FIR coefficients (8-bit signed)
	const int8_t fir[8] = { 127, 0, 0, 0, 0, 0, 0, 0 };  // Simple pass-through

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t sum = 0;
			for (int t = 0; t < 8; t++) {
				sum += static_cast<int32_t>(input[i + t]) * fir[t];
			}
			sum >>= 7;  // Normalize
			if (sum > 32767) sum = 32767;
			if (sum < -32768) sum = -32768;
			output[i] = static_cast<int16_t>(sum);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_Fir8Tap)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark echo effect with feedback
static void BM_Audio_EchoWithFeedback(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	const size_t delaySize = 2048;  // ~64ms at 32040 Hz

	std::vector<int16_t> input(count);
	std::vector<int16_t> output(count);
	std::vector<int16_t> delayLine(delaySize);

	GenerateSineWave(input.data(), count, 440.0, 32040.0);

	constexpr int32_t feedback = 96;  // ~37.5% feedback
	constexpr int32_t wetMix = 64;    // ~25% wet
	constexpr int32_t dryMix = 192;   // ~75% dry

	for (auto _ : state) {
		std::fill(delayLine.begin(), delayLine.end(), static_cast<int16_t>(0));
		size_t delayPos = 0;

		for (size_t i = 0; i < count; i++) {
			// Read from delay line
			int32_t delayed = delayLine[delayPos];

			// Write to delay line with feedback
			int32_t toDelay = input[i] + ((delayed * feedback) >> 8);
			if (toDelay > 32767) toDelay = 32767;
			if (toDelay < -32768) toDelay = -32768;
			delayLine[delayPos] = static_cast<int16_t>(toDelay);

			// Mix dry and wet
			int32_t mixed = ((input[i] * dryMix) >> 8) + ((delayed * wetMix) >> 8);
			if (mixed > 32767) mixed = 32767;
			if (mixed < -32768) mixed = -32768;
			output[i] = static_cast<int16_t>(mixed);

			delayPos = (delayPos + 1) % delaySize;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_EchoWithFeedback)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// SNES BRR Decoding Benchmarks
// -----------------------------------------------------------------------------

// Benchmark BRR block decoding (SNES sample format)
static void BM_Audio_DecodeBrrBlock(benchmark::State& state) {
	// BRR: 9 bytes per block, decodes to 16 samples
	constexpr size_t blocksPerIteration = 128;  // 2048 samples

	// Generate test BRR data (header + 8 bytes per block)
	std::array<uint8_t, 9 * blocksPerIteration> brrData;
	for (size_t b = 0; b < blocksPerIteration; b++) {
		brrData[b * 9] = 0x00;  // Header: filter=0, range=0
		for (int i = 1; i < 9; i++) {
			brrData[b * 9 + i] = static_cast<uint8_t>((b + i) & 0xff);  // Fake sample data
		}
	}

	std::array<int16_t, 16 * blocksPerIteration> output;

	for (auto _ : state) {
		int16_t prev1 = 0, prev2 = 0;

		for (size_t b = 0; b < blocksPerIteration; b++) {
			uint8_t header = brrData[b * 9];
			int shift = header >> 4;
			int filter = (header >> 2) & 3;

			for (int i = 0; i < 8; i++) {
				uint8_t byte = brrData[b * 9 + 1 + i];

				// Decode two 4-bit samples per byte
				for (int n = 0; n < 2; n++) {
					int32_t sample;
					if (n == 0) {
						sample = static_cast<int8_t>(byte) >> 4;  // High nibble (signed)
					} else {
						sample = static_cast<int8_t>(byte << 4) >> 4;  // Low nibble (signed)
					}

					// Apply shift
					sample <<= shift;
					if (shift > 12) sample = (sample < 0) ? -0x800 : 0x7ff;

					// Apply filter
					switch (filter) {
						case 0: break;  // No filter
						case 1: sample += prev1 + (-prev1 >> 4); break;
						case 2: sample += (prev1 << 1) + (-((prev1 << 1) + prev1) >> 5) - prev2 + (prev2 >> 4); break;
						case 3: sample += (prev1 << 1) + (-(prev1 + (prev1 << 2) + (prev1 << 3)) >> 6) - prev2 + ((prev2 + (prev2 << 1)) >> 4); break;
					}

					// Clamp to 15-bit signed
					if (sample > 0x3fff) sample = 0x3fff;
					if (sample < -0x4000) sample = -0x4000;

					output[b * 16 + i * 2 + n] = static_cast<int16_t>(sample << 1);
					prev2 = prev1;
					prev1 = static_cast<int16_t>(sample);
				}
			}
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * 9 * blocksPerIteration);
}
BENCHMARK(BM_Audio_DecodeBrrBlock);

// -----------------------------------------------------------------------------
// NES APU Simulation Benchmarks
// -----------------------------------------------------------------------------

// Benchmark square wave generation (NES pulse channels)
static void BM_Audio_SquareWaveGen(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> output(count);

	// NES APU square wave parameters
	uint16_t period = 447;  // ~A440
	uint8_t dutyCycle = 2;  // 50% duty
	int16_t volume = 8000;

	// Duty cycle patterns
	static const uint8_t dutyTable[4] = { 0x01, 0x03, 0x0F, 0xFC };
	uint8_t dutyPattern = dutyTable[dutyCycle];

	for (auto _ : state) {
		uint16_t timer = period;
		uint8_t sequencePos = 0;

		for (size_t i = 0; i < count; i++) {
			// Simulate APU clocking
			if (timer == 0) {
				timer = period;
				sequencePos = (sequencePos + 1) & 7;
			}
			timer--;

			// Output based on duty cycle
			bool high = (dutyPattern >> sequencePos) & 1;
			output[i] = high ? volume : -volume;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_SquareWaveGen)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark triangle wave generation (NES triangle channel)
static void BM_Audio_TriangleWaveGen(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> output(count);

	uint16_t period = 447;
	int16_t amplitude = 8000;

	// Triangle sequence (32 steps)
	static const int8_t triangleTable[32] = {
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};

	for (auto _ : state) {
		uint16_t timer = period;
		uint8_t sequencePos = 0;

		for (size_t i = 0; i < count; i++) {
			if (timer == 0) {
				timer = period;
				sequencePos = (sequencePos + 1) & 31;
			}
			timer--;

			// Scale to full range
			output[i] = static_cast<int16_t>((triangleTable[sequencePos] - 8) * amplitude / 8);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_TriangleWaveGen)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark noise generation (NES noise channel LFSR)
static void BM_Audio_NoiseGen(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> output(count);

	uint16_t period = 4;
	int16_t volume = 8000;
	bool modeFlag = false;  // Long mode

	for (auto _ : state) {
		uint16_t timer = period;
		uint16_t shiftReg = 1;

		for (size_t i = 0; i < count; i++) {
			if (timer == 0) {
				timer = period;

				// LFSR feedback
				uint16_t feedback;
				if (modeFlag) {
					feedback = ((shiftReg & 1) ^ ((shiftReg >> 6) & 1));
				} else {
					feedback = ((shiftReg & 1) ^ ((shiftReg >> 1) & 1));
				}
				shiftReg = (shiftReg >> 1) | (feedback << 14);
			}
			timer--;

			output[i] = (shiftReg & 1) ? volume : -volume;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_NoiseGen)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Low-Pass Filter Benchmarks
// -----------------------------------------------------------------------------

// Benchmark one-pole low-pass filter
static void BM_Audio_OnePoleFilter(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> input(count);
	std::vector<int16_t> output(count);

	GenerateSineWave(input.data(), count, 440.0, 48000.0);

	// Low-pass coefficient (0-256 range, higher = more filtering)
	constexpr int32_t alpha = 26;  // ~10% cutoff

	for (auto _ : state) {
		int32_t prevSample = 0;

		for (size_t i = 0; i < count; i++) {
			// y[n] = y[n-1] + alpha * (x[n] - y[n-1])
			int32_t diff = input[i] - prevSample;
			prevSample = prevSample + ((diff * alpha) >> 8);
			output[i] = static_cast<int16_t>(prevSample);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_OnePoleFilter)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark biquad filter (used for EQ)
static void BM_Audio_BiquadFilter(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> input(count);
	std::vector<int16_t> output(count);

	GenerateSineWave(input.data(), count, 440.0, 48000.0);

	// Biquad coefficients (16.16 fixed-point)
	constexpr int32_t b0 = 3277;   // ~0.05
	constexpr int32_t b1 = 6554;   // ~0.10
	constexpr int32_t b2 = 3277;   // ~0.05
	constexpr int32_t a1 = -85197; // ~-1.3
	constexpr int32_t a2 = 26214;  // ~0.4

	for (auto _ : state) {
		int32_t x1 = 0, x2 = 0;
		int32_t y1 = 0, y2 = 0;

		for (size_t i = 0; i < count; i++) {
			int32_t x0 = input[i];

			// y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
			int32_t y0 = ((b0 * x0) >> 16) + ((b1 * x1) >> 16) + ((b2 * x2) >> 16)
			           - ((a1 * y1) >> 16) - ((a2 * y2) >> 16);

			if (y0 > 32767) y0 = 32767;
			if (y0 < -32768) y0 = -32768;
			output[i] = static_cast<int16_t>(y0);

			x2 = x1;
			x1 = x0;
			y2 = y1;
			y1 = y0;
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_BiquadFilter)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Envelope Benchmarks
// -----------------------------------------------------------------------------

// Benchmark ADSR envelope generation
static void BM_Audio_AdsrEnvelope(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int16_t> output(count);

	// ADSR parameters (in samples)
	constexpr size_t attack = 100;
	constexpr size_t decay = 200;
	constexpr size_t release = 500;
	constexpr int32_t sustainLevel = 200;  // Out of 256
	const size_t sustainEnd = count - release;

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t envelope;

			if (i < attack) {
				// Attack: ramp up
				envelope = static_cast<int32_t>((i * 256) / attack);
			} else if (i < attack + decay) {
				// Decay: ramp down to sustain
				size_t decayPos = i - attack;
				envelope = 256 - static_cast<int32_t>(((256 - sustainLevel) * decayPos) / decay);
			} else if (i < sustainEnd) {
				// Sustain: hold level
				envelope = sustainLevel;
			} else {
				// Release: ramp down
				size_t releasePos = i - sustainEnd;
				envelope = sustainLevel - static_cast<int32_t>((sustainLevel * releasePos) / release);
				if (envelope < 0) envelope = 0;
			}

			output[i] = static_cast<int16_t>(envelope);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int16_t));
}
BENCHMARK(BM_Audio_AdsrEnvelope)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// -----------------------------------------------------------------------------
// Clipping and Saturation Benchmarks
// -----------------------------------------------------------------------------

// Benchmark hard clipping
static void BM_Audio_HardClip(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int32_t> input(count);
	std::vector<int16_t> output(count);

	// Generate samples that overflow int16
	for (size_t i = 0; i < count; i++) {
		input[i] = static_cast<int32_t>(50000.0 * std::sin(static_cast<double>(i) * 0.01));
	}

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			int32_t sample = input[i];
			if (sample > 32767) sample = 32767;
			if (sample < -32768) sample = -32768;
			output[i] = static_cast<int16_t>(sample);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int32_t));
}
BENCHMARK(BM_Audio_HardClip)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// Benchmark soft clipping (tanh-like)
static void BM_Audio_SoftClip(benchmark::State& state) {
	const size_t count = static_cast<size_t>(state.range(0));
	std::vector<int32_t> input(count);
	std::vector<int16_t> output(count);

	for (size_t i = 0; i < count; i++) {
		input[i] = static_cast<int32_t>(50000.0 * std::sin(static_cast<double>(i) * 0.01));
	}

	for (auto _ : state) {
		for (size_t i = 0; i < count; i++) {
			// Polynomial soft clip approximation
			double x = static_cast<double>(input[i]) / 32768.0;
			double y;
			if (x >= 1.0) {
				y = 1.0;
			} else if (x <= -1.0) {
				y = -1.0;
			} else {
				// Soft clip: y = 1.5x - 0.5x^3
				y = 1.5 * x - 0.5 * x * x * x;
			}
			output[i] = static_cast<int16_t>(y * 32767.0);
		}
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * count * sizeof(int32_t));
}
BENCHMARK(BM_Audio_SoftClip)->Arg(kSmallBufferSize)->Arg(kMediumBufferSize)->Arg(kLargeBufferSize);

// ===== Equalizer Band Gains: vector vs std::array =====

// Reference: vector<double> construction (heap allocation per call)
static void BM_Audio_EqGains_Vector(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<double> bandGains = {
			0.0, 1.0, 2.0, -1.0, 0.5, -0.5, 1.5, -1.5, 0.0, 0.0,
			0.0, 1.0, 2.0, -1.0, 0.5, -0.5, 1.5, -1.5, 0.0, 0.0
		};
		benchmark::DoNotOptimize(bandGains.data());
	}
}
BENCHMARK(BM_Audio_EqGains_Vector);

// Optimized: std::array<double, 20> (stack allocation, no heap)
static void BM_Audio_EqGains_Array(benchmark::State& state) {
	for (auto _ : state) {
		std::array<double, 20> bandGains = {
			0.0, 1.0, 2.0, -1.0, 0.5, -0.5, 1.5, -1.5, 0.0, 0.0,
			0.0, 1.0, 2.0, -1.0, 0.5, -0.5, 1.5, -1.5, 0.0, 0.0
		};
		benchmark::DoNotOptimize(bandGains.data());
	}
}
BENCHMARK(BM_Audio_EqGains_Array);
