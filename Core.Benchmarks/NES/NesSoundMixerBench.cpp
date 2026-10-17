// NesSoundMixerBench.cpp
// Benchmarks for NesSoundMixer::EndFrame optimizations (Phase 13)
// Compares: full 220KB memset vs selective zeroing, sort+unique vs dedup-at-insert

#include "pch.h"
#include "benchmark/benchmark.h"
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <random>

static constexpr uint32_t MaxChannelCount = 11;
static constexpr uint32_t CycleLength = 10000;

// Simulate a typical NES frame: ~300 unique timestamps across 11 channels
// with some timestamps shared between channels (total pushes ~500)
static std::vector<uint32_t> GenerateFrameTimestamps(std::mt19937& rng, int uniqueCount, int totalPushes) {
	std::uniform_int_distribution<uint32_t> dist(0, CycleLength - 1);
	std::vector<uint32_t> unique_ts;
	unique_ts.reserve(uniqueCount);
	for (int i = 0; i < uniqueCount; i++) {
		unique_ts.push_back(dist(rng));
	}

	// Simulate multiple channels pushing overlapping timestamps
	std::vector<uint32_t> all_ts;
	all_ts.reserve(totalPushes);
	std::uniform_int_distribution<int> idx_dist(0, uniqueCount - 1);
	for (int i = 0; i < totalPushes; i++) {
		all_ts.push_back(unique_ts[idx_dist(rng)]);
	}
	return all_ts;
}

// OLD: Full 220KB memset
static void BM_EndFrame_FullMemset(benchmark::State& state) {
	int16_t channelOutput[MaxChannelCount][CycleLength] = {};
	for (auto _ : state) {
		memset(channelOutput, 0, sizeof(channelOutput));
		benchmark::DoNotOptimize(channelOutput);
	}
}
BENCHMARK(BM_EndFrame_FullMemset);

// NEW: Selective zeroing (only used timestamps)
static void BM_EndFrame_SelectiveZero(benchmark::State& state) {
	int16_t channelOutput[MaxChannelCount][CycleLength] = {};
	std::mt19937 rng(42);
	auto timestamps = GenerateFrameTimestamps(rng, 300, 500);

	// Sort + unique to get the unique set
	std::sort(timestamps.begin(), timestamps.end());
	timestamps.erase(std::unique(timestamps.begin(), timestamps.end()), timestamps.end());

	// Fill some data so zero is meaningful
	for (uint32_t stamp : timestamps) {
		for (uint32_t ch = 0; ch < MaxChannelCount; ch++) {
			channelOutput[ch][stamp] = 1;
		}
	}

	for (auto _ : state) {
		for (size_t i = 0, len = timestamps.size(); i < len; i++) {
			uint32_t stamp = timestamps[i];
			for (uint32_t j = 0; j < MaxChannelCount; j++) {
				channelOutput[j][stamp] = 0;
			}
		}
		benchmark::DoNotOptimize(channelOutput);
	}
}
BENCHMARK(BM_EndFrame_SelectiveZero);

// OLD: push_back all + sort + unique
static void BM_Timestamps_PushSortUnique(benchmark::State& state) {
	std::mt19937 rng(42);
	auto frame_pushes = GenerateFrameTimestamps(rng, 300, 500);

	for (auto _ : state) {
		std::vector<uint32_t> timestamps;
		timestamps.reserve(512);
		for (uint32_t t : frame_pushes) {
			timestamps.push_back(t);
		}
		std::sort(timestamps.begin(), timestamps.end());
		timestamps.erase(std::unique(timestamps.begin(), timestamps.end()), timestamps.end());
		benchmark::DoNotOptimize(timestamps.data());
		benchmark::DoNotOptimize(timestamps.size());
	}
}
BENCHMARK(BM_Timestamps_PushSortUnique);

// NEW: dedup-at-insert + sort (smaller vector, no unique needed)
static void BM_Timestamps_DedupInsertSort(benchmark::State& state) {
	std::mt19937 rng(42);
	auto frame_pushes = GenerateFrameTimestamps(rng, 300, 500);

	for (auto _ : state) {
		bool used[CycleLength] = {};
		std::vector<uint32_t> timestamps;
		timestamps.reserve(512);
		for (uint32_t t : frame_pushes) {
			if (!used[t]) {
				used[t] = true;
				timestamps.push_back(t);
			}
		}
		std::sort(timestamps.begin(), timestamps.end());
		benchmark::DoNotOptimize(timestamps.data());
		benchmark::DoNotOptimize(timestamps.size());
	}
}
BENCHMARK(BM_Timestamps_DedupInsertSort);

// Benchmark for StringUtilities::GetNthSegment vs Split()[n]
static void BM_Split_ThenIndex(benchmark::State& state) {
	std::string comment = "Line 0\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5";
	int targetLine = 3;
	for (auto _ : state) {
		// Simulate Split then index
		std::vector<std::string> result;
		size_t index = 0;
		size_t lastIndex = 0;
		std::string_view sv(comment);
		while ((index = sv.find('\n', index)) != std::string_view::npos) {
			result.push_back(std::string(sv.substr(lastIndex, index - lastIndex)));
			index++;
			lastIndex = index;
		}
		result.push_back(std::string(sv.substr(lastIndex)));
		benchmark::DoNotOptimize(result[targetLine].data());
	}
}
BENCHMARK(BM_Split_ThenIndex);

// NEW: GetNthSegment - find Nth delimiter without full split
static void BM_GetNthSegment(benchmark::State& state) {
	std::string comment = "Line 0\nLine 1\nLine 2\nLine 3\nLine 4\nLine 5";
	int targetLine = 3;
	for (auto _ : state) {
		std::string_view sv(comment);
		size_t start = 0;
		for (int i = 0; i < targetLine; i++) {
			size_t pos = sv.find('\n', start);
			start = pos + 1;
		}
		size_t end = sv.find('\n', start);
		std::string result(sv.substr(start, end - start));
		benchmark::DoNotOptimize(result.data());
	}
}
BENCHMARK(BM_GetNthSegment);
