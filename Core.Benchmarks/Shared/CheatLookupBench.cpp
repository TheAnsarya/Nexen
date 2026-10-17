#include "pch.h"
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <random>

// Benchmark: sorted vector binary search vs unordered_map for cheat lookup
// Simulates the CheatManager::ApplyCheat hot path with typical cheat counts (1-10)

struct BenchCheatCode {
	uint32_t Address = 0;
	int16_t Compare = -1;
	uint8_t Value = 0;
};

// ===== Baseline: unordered_map lookup (OLD implementation) =====

static void BM_CheatLookup_UnorderedMap_1Cheat(benchmark::State& state) {
	std::unordered_map<uint32_t, BenchCheatCode> cheats;
	cheats[0x1234] = {0x1234, -1, 0xFF};

	// Simulate bank-hit addresses (256-byte NES bank containing the cheat)
	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = cheats.find(addrs[idx]);
		benchmark::DoNotOptimize(it);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_UnorderedMap_1Cheat);

static void BM_CheatLookup_UnorderedMap_5Cheats(benchmark::State& state) {
	std::unordered_map<uint32_t, BenchCheatCode> cheats;
	cheats[0x1234] = {0x1234, -1, 0xFF};
	cheats[0x1250] = {0x1250, -1, 0x10};
	cheats[0x1280] = {0x1280, -1, 0x20};
	cheats[0x12A0] = {0x12A0, -1, 0x30};
	cheats[0x12E0] = {0x12E0, -1, 0x40};

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = cheats.find(addrs[idx]);
		benchmark::DoNotOptimize(it);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_UnorderedMap_5Cheats);

static void BM_CheatLookup_UnorderedMap_10Cheats(benchmark::State& state) {
	std::unordered_map<uint32_t, BenchCheatCode> cheats;
	for (uint32_t i = 0; i < 10; i++) {
		uint32_t addr = 0x1200 + i * 16;
		cheats[addr] = {addr, -1, (uint8_t)i};
	}

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = cheats.find(addrs[idx]);
		benchmark::DoNotOptimize(it);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_UnorderedMap_10Cheats);

// ===== New: sorted vector binary search (NEW implementation) =====

static void BM_CheatLookup_SortedVector_1Cheat(benchmark::State& state) {
	std::vector<std::pair<uint32_t, BenchCheatCode>> cheats;
	cheats.emplace_back(0x1234, BenchCheatCode{0x1234, -1, 0xFF});

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = std::lower_bound(cheats.begin(), cheats.end(), addrs[idx],
			[](const std::pair<uint32_t, BenchCheatCode>& p, uint32_t a) { return p.first < a; });
		benchmark::DoNotOptimize(it != cheats.end() && it->first == addrs[idx]);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_SortedVector_1Cheat);

static void BM_CheatLookup_SortedVector_5Cheats(benchmark::State& state) {
	std::vector<std::pair<uint32_t, BenchCheatCode>> cheats;
	cheats.emplace_back(0x1234, BenchCheatCode{0x1234, -1, 0xFF});
	cheats.emplace_back(0x1250, BenchCheatCode{0x1250, -1, 0x10});
	cheats.emplace_back(0x1280, BenchCheatCode{0x1280, -1, 0x20});
	cheats.emplace_back(0x12A0, BenchCheatCode{0x12A0, -1, 0x30});
	cheats.emplace_back(0x12E0, BenchCheatCode{0x12E0, -1, 0x40});

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = std::lower_bound(cheats.begin(), cheats.end(), addrs[idx],
			[](const std::pair<uint32_t, BenchCheatCode>& p, uint32_t a) { return p.first < a; });
		benchmark::DoNotOptimize(it != cheats.end() && it->first == addrs[idx]);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_SortedVector_5Cheats);

static void BM_CheatLookup_SortedVector_10Cheats(benchmark::State& state) {
	std::vector<std::pair<uint32_t, BenchCheatCode>> cheats;
	for (uint32_t i = 0; i < 10; i++) {
		uint32_t addr = 0x1200 + i * 16;
		cheats.emplace_back(addr, BenchCheatCode{addr, -1, (uint8_t)i});
	}

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) addrs.push_back(i);

	size_t idx = 0;
	for (auto _ : state) {
		auto it = std::lower_bound(cheats.begin(), cheats.end(), addrs[idx],
			[](const std::pair<uint32_t, BenchCheatCode>& p, uint32_t a) { return p.first < a; });
		benchmark::DoNotOptimize(it != cheats.end() && it->first == addrs[idx]);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_SortedVector_10Cheats);

// ===== Miss case: lookup address not in cheats (most common hot-path case) =====

static void BM_CheatLookup_UnorderedMap_Miss(benchmark::State& state) {
	std::unordered_map<uint32_t, BenchCheatCode> cheats;
	cheats[0x1234] = {0x1234, -1, 0xFF};

	// Addresses that are NOT in the cheat map (miss case — most common in real emulation)
	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) {
		if (i != 0x1234) addrs.push_back(i);
	}

	size_t idx = 0;
	for (auto _ : state) {
		auto it = cheats.find(addrs[idx]);
		benchmark::DoNotOptimize(it == cheats.end());
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_UnorderedMap_Miss);

static void BM_CheatLookup_SortedVector_Miss(benchmark::State& state) {
	std::vector<std::pair<uint32_t, BenchCheatCode>> cheats;
	cheats.emplace_back(0x1234, BenchCheatCode{0x1234, -1, 0xFF});

	std::vector<uint32_t> addrs;
	for (uint32_t i = 0x1200; i < 0x1300; i++) {
		if (i != 0x1234) addrs.push_back(i);
	}

	size_t idx = 0;
	for (auto _ : state) {
		auto it = std::lower_bound(cheats.begin(), cheats.end(), addrs[idx],
			[](const std::pair<uint32_t, BenchCheatCode>& p, uint32_t a) { return p.first < a; });
		benchmark::DoNotOptimize(it == cheats.end() || it->first != addrs[idx]);
		idx = (idx + 1) % addrs.size();
	}
}
BENCHMARK(BM_CheatLookup_SortedVector_Miss);
