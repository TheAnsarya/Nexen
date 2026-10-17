#include "pch.h"
#include <vector>
#include <cstring>

// Benchmark: std::move (defeats buffer reuse) vs memcpy (preserves buffer)
// Simulates the SaveStateManager::GetVideoData hot path during rewind

static constexpr size_t FrameBufferSize = 256 * 240 * 4; // ~240KB (typical NES frame)

// ===== OLD: move transfers ownership, forces re-alloc next call =====

static void BM_VideoData_Move_Pattern(benchmark::State& state) {
	std::vector<uint8_t> decompressBuffer;
	std::vector<uint8_t> output;

	for (auto _ : state) {
		// Simulate: resize persistent buffer (re-alloc if moved away last call)
		decompressBuffer.resize(FrameBufferSize);
		// Fill with some data to simulate decompression
		memset(decompressBuffer.data(), 0x42, FrameBufferSize);

		// OLD pattern: move defeats buffer persistence
		output = std::move(decompressBuffer);

		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_VideoData_Move_Pattern);

// ===== NEW: copy preserves persistent buffer allocation =====

static void BM_VideoData_Copy_Pattern(benchmark::State& state) {
	std::vector<uint8_t> decompressBuffer;
	std::vector<uint8_t> output;

	for (auto _ : state) {
		// Simulate: resize persistent buffer (no-op after first call since we keep it)
		decompressBuffer.resize(FrameBufferSize);
		// Fill with some data to simulate decompression
		memset(decompressBuffer.data(), 0x42, FrameBufferSize);

		// NEW pattern: copy preserves _decompressBuffer allocation
		output.resize(FrameBufferSize);
		memcpy(output.data(), decompressBuffer.data(), FrameBufferSize);

		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_VideoData_Copy_Pattern);

// ===== Repeated calls (simulates rewind: ~1 state/sec for 10 seconds) =====

static void BM_VideoData_Move_RewindBurst(benchmark::State& state) {
	std::vector<uint8_t> decompressBuffer;
	std::vector<uint8_t> output;

	for (auto _ : state) {
		// Simulate 10 consecutive state loads during rewind
		for (int i = 0; i < 10; i++) {
			decompressBuffer.resize(FrameBufferSize);
			memset(decompressBuffer.data(), (uint8_t)i, FrameBufferSize);
			output = std::move(decompressBuffer);
		}
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_VideoData_Move_RewindBurst);

static void BM_VideoData_Copy_RewindBurst(benchmark::State& state) {
	std::vector<uint8_t> decompressBuffer;
	std::vector<uint8_t> output;

	for (auto _ : state) {
		// Simulate 10 consecutive state loads during rewind
		for (int i = 0; i < 10; i++) {
			decompressBuffer.resize(FrameBufferSize);
			memset(decompressBuffer.data(), (uint8_t)i, FrameBufferSize);
			output.resize(FrameBufferSize);
			memcpy(output.data(), decompressBuffer.data(), FrameBufferSize);
		}
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
}
BENCHMARK(BM_VideoData_Copy_RewindBurst);
