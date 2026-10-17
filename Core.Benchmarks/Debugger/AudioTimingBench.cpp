// AudioTimingBench.cpp
// Benchmarks for SoundMixer and timing/frame pacing hot paths

#include "pch.h"
#include "benchmark/benchmark.h"
#include <cstdint>
#include <vector>
#include <array>
#include <random>

// Dummy SoundMixer for benchmarking
class DummySoundMixer {
public:
	std::vector<int16_t> buffer;
	DummySoundMixer(size_t size) : buffer(size, 0) {}
	void Mix() {
		for (size_t i = 0; i < buffer.size(); i++) {
			buffer[i] = (int16_t)(buffer[i] + i);
		}
	}
	void Output() {
		volatile int16_t sink = 0;
		for (size_t i = 0; i < buffer.size(); i++) {
			sink ^= buffer[i];
		}
	}
};

static void BM_SoundMixer_Mix_1024(benchmark::State& state) {
	DummySoundMixer mixer(1024);
	for (auto _ : state) {
		mixer.Mix();
	}
}
BENCHMARK(BM_SoundMixer_Mix_1024);

static void BM_SoundMixer_Output_1024(benchmark::State& state) {
	DummySoundMixer mixer(1024);
	for (auto _ : state) {
		mixer.Output();
	}
}
BENCHMARK(BM_SoundMixer_Output_1024);

static void BM_SoundMixer_Mix_4096(benchmark::State& state) {
	DummySoundMixer mixer(4096);
	for (auto _ : state) {
		mixer.Mix();
	}
}
BENCHMARK(BM_SoundMixer_Mix_4096);

static void BM_SoundMixer_Output_4096(benchmark::State& state) {
	DummySoundMixer mixer(4096);
	for (auto _ : state) {
		mixer.Output();
	}
}
BENCHMARK(BM_SoundMixer_Output_4096);

// Dummy timing/frame pacing benchmark
static void BM_FrameAdvance(benchmark::State& state) {
	uint64_t frameCounter = 0;
	for (auto _ : state) {
		frameCounter++;
	}
}
BENCHMARK(BM_FrameAdvance);

static void BM_TurboMode(benchmark::State& state) {
	uint64_t turboCounter = 0;
	for (auto _ : state) {
		for (int i = 0; i < 10; i++) {
			turboCounter++;
		}
	}
}
BENCHMARK(BM_TurboMode);
