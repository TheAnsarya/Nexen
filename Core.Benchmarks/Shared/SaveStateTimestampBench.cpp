#include "pch.h"
#include "Shared/SaveStateManager.h"
#include <string>

// =============================================================================
// SaveStateManager::ParseTimestampFromFilename Benchmark
// =============================================================================
// Benchmarks timestamp parsing from save state filenames.
// This is called per-file during save state directory scanning (UI refresh).

static void BM_ParseTimestamp_NexenSave(benchmark::State& state) {
	const std::string filename = "Dragon_Quest_III_2026-07-15_14-30-00.nexen-save";
	for (auto _ : state) {
		time_t result = SaveStateManager::ParseTimestampFromFilename(filename);
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseTimestamp_NexenSave);

static void BM_ParseTimestamp_Mss(benchmark::State& state) {
	const std::string filename = "LegacyRom_2025-06-15_09-45-30.mss";
	for (auto _ : state) {
		time_t result = SaveStateManager::ParseTimestampFromFilename(filename);
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseTimestamp_Mss);

static void BM_ParseTimestamp_Invalid(benchmark::State& state) {
	const std::string filename = "TestRom_1.nexen-save";
	for (auto _ : state) {
		time_t result = SaveStateManager::ParseTimestampFromFilename(filename);
		benchmark::DoNotOptimize(result);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ParseTimestamp_Invalid);

static void BM_ParseTimestamp_BatchScan(benchmark::State& state) {
	// Simulate scanning a directory with 50 timestamped files
	std::vector<std::string> filenames;
	for (int i = 0; i < 50; i++) {
		filenames.push_back(std::format("Game_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}.nexen-save",
			2026, (i % 12) + 1, (i % 28) + 1, i % 24, i % 60, i % 60));
	}

	for (auto _ : state) {
		for (const auto& fn : filenames) {
			time_t result = SaveStateManager::ParseTimestampFromFilename(fn);
			benchmark::DoNotOptimize(result);
		}
	}
	state.SetItemsProcessed(state.iterations() * filenames.size());
}
BENCHMARK(BM_ParseTimestamp_BatchScan);
