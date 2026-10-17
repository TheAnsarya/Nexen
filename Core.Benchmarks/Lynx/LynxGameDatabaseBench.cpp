#include "pch.h"
#include <array>
#include <cstring>
#include <unordered_map>
#include <map>

// =============================================================================
// Lynx Game Database Benchmarks
// =============================================================================
// The game database provides CRC32-based lookup for game metadata:
//   - Game name
//   - Screen rotation (None, Left, Right)
//   - EEPROM type (None, 93c46, 93c56, 93c66, 93c76, 93c86)
//   - Player count (1-6)
//
// Used for auto-detecting properties for headerless ROMs and verifying
// LNX header data against known-good values.
//
// References:
//   - No-Intro DAT files
//   - LynxGameDatabase.h (database implementation)

// -----------------------------------------------------------------------------
// Test Database Entry Structure
// -----------------------------------------------------------------------------

struct GameEntry {
	uint32_t crc32;
	const char* name;
	uint8_t rotation;
	uint8_t eepromType;
	uint8_t playerCount;
};

// Sample database with ~20 entries for benchmark purposes
static constexpr GameEntry testDatabase[] = {
	{ 0x8b8de924, "California Games", 0, 0, 4 },
	{ 0x1d0dab8a, "Chip's Challenge", 0, 0, 1 },
	{ 0x45ce0898, "Klax", 2, 0, 2 },
	{ 0xe8b3b8d9, "Rygar", 0, 0, 1 },
	{ 0x12345678, "Test Game 1", 0, 1, 1 },
	{ 0x23456789, "Test Game 2", 1, 0, 2 },
	{ 0x3456789a, "Test Game 3", 0, 2, 1 },
	{ 0x456789ab, "Test Game 4", 2, 0, 4 },
	{ 0x56789abc, "Test Game 5", 0, 3, 1 },
	{ 0x6789abcd, "Test Game 6", 1, 0, 2 },
	{ 0x789abcde, "Test Game 7", 0, 4, 1 },
	{ 0x89abcdef, "Test Game 8", 2, 0, 6 },
	{ 0x9abcdef0, "Test Game 9", 0, 5, 1 },
	{ 0xabcdef01, "Test Game 10", 0, 0, 2 },
	{ 0xbcdef012, "Test Game 11", 1, 1, 1 },
	{ 0xcdef0123, "Test Game 12", 0, 0, 4 },
	{ 0xdef01234, "Test Game 13", 2, 2, 1 },
	{ 0xef012345, "Test Game 14", 0, 0, 2 },
	{ 0xf0123456, "Test Game 15", 0, 3, 1 },
	{ 0x01234567, "Test Game 16", 1, 0, 1 },
};

static constexpr size_t testDatabaseSize = sizeof(testDatabase) / sizeof(testDatabase[0]);

// -----------------------------------------------------------------------------
// Linear Search (Baseline)
// -----------------------------------------------------------------------------

// Benchmark linear search through database
static void BM_LynxGameDb_LinearSearch(benchmark::State& state) {
	uint32_t searchCrcs[] = {
		0x8b8de924, // First entry
		0x01234567, // Last entry
		0xDEADBEEF, // Not found
		0x789abcde, // Middle entry
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 4];
		const GameEntry* found = nullptr;

		for (size_t i = 0; i < testDatabaseSize; i++) {
			if (testDatabase[i].crc32 == targetCrc) {
				found = &testDatabase[i];
				break;
			}
		}

		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_LinearSearch);

// -----------------------------------------------------------------------------
// Hash Map Lookup
// -----------------------------------------------------------------------------

// Benchmark std::unordered_map lookup
static void BM_LynxGameDb_UnorderedMapLookup(benchmark::State& state) {
	// Build hash map
	std::unordered_map<uint32_t, const GameEntry*> hashMap;
	for (size_t i = 0; i < testDatabaseSize; i++) {
		hashMap[testDatabase[i].crc32] = &testDatabase[i];
	}

	uint32_t searchCrcs[] = {
		0x8b8de924, 0x01234567, 0xDEADBEEF, 0x789abcde,
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 4];
		auto it = hashMap.find(targetCrc);
		const GameEntry* found = (it != hashMap.end()) ? it->second : nullptr;
		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_UnorderedMapLookup);

// -----------------------------------------------------------------------------
// Sorted Map Lookup
// -----------------------------------------------------------------------------

// Benchmark std::map lookup (binary search tree)
static void BM_LynxGameDb_MapLookup(benchmark::State& state) {
	// Build sorted map
	std::map<uint32_t, const GameEntry*> sortedMap;
	for (size_t i = 0; i < testDatabaseSize; i++) {
		sortedMap[testDatabase[i].crc32] = &testDatabase[i];
	}

	uint32_t searchCrcs[] = {
		0x8b8de924, 0x01234567, 0xDEADBEEF, 0x789abcde,
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 4];
		auto it = sortedMap.find(targetCrc);
		const GameEntry* found = (it != sortedMap.end()) ? it->second : nullptr;
		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_MapLookup);

// -----------------------------------------------------------------------------
// Binary Search (Sorted Array)
// -----------------------------------------------------------------------------

// Benchmark binary search on pre-sorted array
static void BM_LynxGameDb_BinarySearch(benchmark::State& state) {
	// Pre-sorted CRCs for binary search
	uint32_t sortedCrcs[testDatabaseSize];
	for (size_t i = 0; i < testDatabaseSize; i++) {
		sortedCrcs[i] = testDatabase[i].crc32;
	}
	std::sort(sortedCrcs, sortedCrcs + testDatabaseSize);

	uint32_t searchCrcs[] = {
		0x8b8de924, 0x01234567, 0xDEADBEEF, 0x789abcde,
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 4];
		bool found = std::binary_search(sortedCrcs, sortedCrcs + testDatabaseSize, targetCrc);
		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_BinarySearch);

// -----------------------------------------------------------------------------
// Entry Field Access
// -----------------------------------------------------------------------------

// Benchmark name field access
static void BM_LynxGameDb_AccessName(benchmark::State& state) {
	size_t idx = 0;

	for (auto _ : state) {
		const char* name = testDatabase[idx % testDatabaseSize].name;
		benchmark::DoNotOptimize(name);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_AccessName);

// Benchmark rotation field access
static void BM_LynxGameDb_AccessRotation(benchmark::State& state) {
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t rotation = testDatabase[idx % testDatabaseSize].rotation;
		benchmark::DoNotOptimize(rotation);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_AccessRotation);

// Benchmark EEPROM type field access
static void BM_LynxGameDb_AccessEepromType(benchmark::State& state) {
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t eepromType = testDatabase[idx % testDatabaseSize].eepromType;
		benchmark::DoNotOptimize(eepromType);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_AccessEepromType);

// Benchmark player count field access
static void BM_LynxGameDb_AccessPlayerCount(benchmark::State& state) {
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t playerCount = testDatabase[idx % testDatabaseSize].playerCount;
		benchmark::DoNotOptimize(playerCount);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_AccessPlayerCount);

// -----------------------------------------------------------------------------
// Entry Count
// -----------------------------------------------------------------------------

// Benchmark database size query
static void BM_LynxGameDb_GetEntryCount(benchmark::State& state) {
	for (auto _ : state) {
		size_t count = testDatabaseSize;
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_GetEntryCount);

// -----------------------------------------------------------------------------
// Miss Rate Analysis
// -----------------------------------------------------------------------------

// Benchmark search with high miss rate (50%)
static void BM_LynxGameDb_MixedHitMiss(benchmark::State& state) {
	uint32_t searchCrcs[] = {
		0x8b8de924, // Hit
		0xAAAAAAAA, // Miss
		0x45ce0898, // Hit
		0xBBBBBBBB, // Miss
		0x789abcde, // Hit
		0xCCCCCCCC, // Miss
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 6];
		const GameEntry* found = nullptr;

		for (size_t i = 0; i < testDatabaseSize; i++) {
			if (testDatabase[i].crc32 == targetCrc) {
				found = &testDatabase[i];
				break;
			}
		}

		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_MixedHitMiss);

// Benchmark search with all misses
static void BM_LynxGameDb_AllMiss(benchmark::State& state) {
	uint32_t searchCrcs[] = {
		0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD,
	};
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t targetCrc = searchCrcs[idx % 4];
		const GameEntry* found = nullptr;

		for (size_t i = 0; i < testDatabaseSize; i++) {
			if (testDatabase[i].crc32 == targetCrc) {
				found = &testDatabase[i];
				break;
			}
		}

		benchmark::DoNotOptimize(found);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_AllMiss);

// -----------------------------------------------------------------------------
// Combined Lookup + Access
// -----------------------------------------------------------------------------

// Benchmark full lookup cycle (search + field access)
static void BM_LynxGameDb_FullLookupCycle(benchmark::State& state) {
	uint32_t targetCrc = 0x45ce0898; // Klax

	for (auto _ : state) {
		const GameEntry* found = nullptr;

		for (size_t i = 0; i < testDatabaseSize; i++) {
			if (testDatabase[i].crc32 == targetCrc) {
				found = &testDatabase[i];
				break;
			}
		}

		if (found) {
			const char* name = found->name;
			uint8_t rotation = found->rotation;
			uint8_t eepromType = found->eepromType;
			uint8_t playerCount = found->playerCount;

			benchmark::DoNotOptimize(name);
			benchmark::DoNotOptimize(rotation);
			benchmark::DoNotOptimize(eepromType);
			benchmark::DoNotOptimize(playerCount);
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxGameDb_FullLookupCycle);

