#include "pch.h"
// MetadataRecordingBench.cpp
// Benchmarks for CDL and Pansy metadata recording overhead
// Focus: realistic CDL byte-OR patterns, enabled vs disabled, and combined pipeline

#include "benchmark/benchmark.h"
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>

// =============================================================================
// Realistic CDL Recording Benchmarks
// =============================================================================
// Simulates the actual LightweightCdlRecorder: a byte array where each entry
// tracks Code/Data flags via OR operations. This is what runs per-instruction.

namespace CdlBenchFlags {
	constexpr uint8_t Code = 0x01;
	constexpr uint8_t Data = 0x02;
}

/// <summary>
/// Realistic CDL recorder matching LightweightCdlRecorder's hot path.
/// Uses a real byte array with OR operations (idempotent writes).
/// </summary>
class RealisticCdlRecorder {
public:
	std::unique_ptr<uint8_t[]> cdlData;
	uint32_t cdlSize = 0;
	bool enabled = false;

	RealisticCdlRecorder(uint32_t romSize) : cdlSize(romSize) {
		cdlData = std::make_unique<uint8_t[]>(romSize);
		std::memset(cdlData.get(), 0, romSize);
	}

	__forceinline void RecordCode(uint32_t absAddr) {
		if (enabled && absAddr < cdlSize) {
			cdlData[absAddr] |= CdlBenchFlags::Code;
		}
	}

	__forceinline void RecordData(uint32_t absAddr) {
		if (enabled && absAddr < cdlSize) {
			cdlData[absAddr] |= CdlBenchFlags::Data;
		}
	}
};

// Benchmark: CDL code marking on sequential addresses (first pass — all cold)
static void BM_CDL_RecordCode_ColdPass(benchmark::State& state) {
	constexpr uint32_t romSize = 512 * 1024;  // 512KB ROM
	RealisticCdlRecorder cdl(romSize);
	cdl.enabled = true;

	for (auto _ : state) {
		state.PauseTiming();
		std::memset(cdl.cdlData.get(), 0, romSize);
		state.ResumeTiming();

		// Simulate 10000 instructions hitting unique addresses
		for (uint32_t i = 0; i < 10000; i++) {
			cdl.RecordCode(i * 3);  // Stride of 3 (avg instruction size)
		}
	}
	state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CDL_RecordCode_ColdPass);

// Benchmark: CDL code marking on repeated addresses (steady state — all hot/idempotent)
static void BM_CDL_RecordCode_HotPass(benchmark::State& state) {
	constexpr uint32_t romSize = 512 * 1024;  // 512KB ROM
	RealisticCdlRecorder cdl(romSize);
	cdl.enabled = true;

	// Pre-warm: mark all addresses as code
	for (uint32_t i = 0; i < 10000; i++) {
		cdl.RecordCode(i * 3);
	}

	for (auto _ : state) {
		// Re-mark same addresses (OR with already-set bits — no-op effectively)
		for (uint32_t i = 0; i < 10000; i++) {
			cdl.RecordCode(i * 3);
		}
	}
	state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CDL_RecordCode_HotPass);

// Benchmark: CDL disabled check overhead (how much does the if(enabled) cost?)
static void BM_CDL_RecordCode_Disabled(benchmark::State& state) {
	constexpr uint32_t romSize = 512 * 1024;
	RealisticCdlRecorder cdl(romSize);
	cdl.enabled = false;

	for (auto _ : state) {
		for (uint32_t i = 0; i < 10000; i++) {
			cdl.RecordCode(i * 3);
		}
	}
	state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CDL_RecordCode_Disabled);

// Benchmark: CDL data marking with random addresses (cache-unfriendly pattern)
static void BM_CDL_RecordData_RandomAddresses(benchmark::State& state) {
	constexpr uint32_t romSize = 512 * 1024;
	RealisticCdlRecorder cdl(romSize);
	cdl.enabled = true;

	// Pre-generate random addresses
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint32_t> dist(0, romSize - 1);
	std::vector<uint32_t> addresses(10000);
	for (auto& addr : addresses) addr = dist(gen);

	for (auto _ : state) {
		for (uint32_t addr : addresses) {
			cdl.RecordData(addr);
		}
	}
	state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_CDL_RecordData_RandomAddresses);

// Benchmark: Combined code + data marking (typical instruction + operand read pattern)
static void BM_CDL_CombinedCodeData(benchmark::State& state) {
	constexpr uint32_t romSize = 512 * 1024;
	RealisticCdlRecorder cdl(romSize);
	cdl.enabled = true;

	for (auto _ : state) {
		for (uint32_t i = 0; i < 10000; i++) {
			uint32_t pc = (i * 3) % romSize;
			cdl.RecordCode(pc);      // Instruction fetch
			cdl.RecordCode(pc + 1);  // Operand byte 1
			cdl.RecordData((pc * 7) % romSize);  // Data read from table
		}
	}
	state.SetItemsProcessed(state.iterations() * 30000);
}
BENCHMARK(BM_CDL_CombinedCodeData);

// =============================================================================
// CrossFeedFilter Benchmarks
// =============================================================================
// Compares old int16_t arithmetic (overflow-prone) vs new int32_t+clamp (correct)

// The old pattern: int16_t multiplication could overflow
static void BM_CrossFeed_OldOverflowProne(benchmark::State& state) {
	constexpr size_t sampleCount = 2048;
	std::vector<int16_t> buffer(sampleCount * 2);
	std::mt19937 gen(42);
	std::uniform_int_distribution<int16_t> dist(-20000, 20000);
	for (auto& s : buffer) s = dist(gen);
	int ratio = 30;  // 30% cross-feed

	for (auto _ : state) {
		auto workBuf = buffer;  // Copy to avoid cumulative overflow
		int16_t* stereo = workBuf.data();
		for (size_t i = 0; i < sampleCount; i++) {
			int16_t left = stereo[0];
			int16_t right = stereo[1];
			// Old pattern: int16_t arithmetic overflow on loud signals
			stereo[0] = static_cast<int16_t>(left + right * ratio / 100);
			stereo[1] = static_cast<int16_t>(right + left * ratio / 100);
			stereo += 2;
		}
		benchmark::DoNotOptimize(workBuf.data());
	}
	state.SetItemsProcessed(state.iterations() * sampleCount);
}
BENCHMARK(BM_CrossFeed_OldOverflowProne);

// The new pattern: int32_t arithmetic + clamp (correct, no overflow)
static void BM_CrossFeed_NewInt32Clamp(benchmark::State& state) {
	constexpr size_t sampleCount = 2048;
	std::vector<int16_t> buffer(sampleCount * 2);
	std::mt19937 gen(42);
	std::uniform_int_distribution<int16_t> dist(-20000, 20000);
	for (auto& s : buffer) s = dist(gen);
	int ratio = 30;

	for (auto _ : state) {
		auto workBuf = buffer;
		int16_t* stereo = workBuf.data();
		for (size_t i = 0; i < sampleCount; i++) {
			int16_t left = stereo[0];
			int16_t right = stereo[1];
			// New pattern: int32_t prevents overflow, clamp ensures range
			int32_t newLeft = (int32_t)left + (int32_t)right * ratio / 100;
			int32_t newRight = (int32_t)right + (int32_t)left * ratio / 100;
			stereo[0] = (int16_t)std::clamp(newLeft, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
			stereo[1] = (int16_t)std::clamp(newRight, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
			stereo += 2;
		}
		benchmark::DoNotOptimize(workBuf.data());
	}
	state.SetItemsProcessed(state.iterations() * sampleCount);
}
BENCHMARK(BM_CrossFeed_NewInt32Clamp);

// =============================================================================
// NotificationManager Dispatch Benchmarks
// =============================================================================
// Compares old pattern (copy vector + cleanup) vs new pattern (iterate under lock)

// Old pattern: copy the listener list, iterate copy, then cleanup expired entries
static void BM_Notification_OldCopyAndCleanup(benchmark::State& state) {
	constexpr int listenerCount = 20;
	constexpr int notificationsPerFrame = 5;

	// Simulate weak_ptr vector with shared_ptr backing
	std::vector<std::shared_ptr<int>> owners(listenerCount);
	std::vector<std::weak_ptr<int>> listeners(listenerCount);
	for (int i = 0; i < listenerCount; i++) {
		owners[i] = std::make_shared<int>(i);
		listeners[i] = owners[i];
	}
	// Expire some listeners (simulate UI elements destroyed)
	owners[3].reset();
	owners[7].reset();
	owners[15].reset();

	for (auto _ : state) {
		for (int n = 0; n < notificationsPerFrame; n++) {
			// OLD: copy vector, then iterate, then cleanup
			auto copy = listeners;
			for (auto& wp : copy) {
				if (auto sp = wp.lock()) {
					benchmark::DoNotOptimize(*sp);
				}
			}
			// Cleanup expired
			listeners.erase(
				std::remove_if(listeners.begin(), listeners.end(),
					[](const std::weak_ptr<int>& wp) { return wp.expired(); }),
				listeners.end()
			);
			// Re-add expired ones for next iteration
			listeners.resize(listenerCount);
		}
	}
	state.SetItemsProcessed(state.iterations() * notificationsPerFrame * listenerCount);
}
BENCHMARK(BM_Notification_OldCopyAndCleanup);

// New pattern: iterate under lock (index-based), no copy, cleanup at registration time
static void BM_Notification_NewIterateUnderLock(benchmark::State& state) {
	constexpr int listenerCount = 20;
	constexpr int notificationsPerFrame = 5;

	std::vector<std::shared_ptr<int>> owners(listenerCount);
	std::vector<std::weak_ptr<int>> listeners(listenerCount);
	for (int i = 0; i < listenerCount; i++) {
		owners[i] = std::make_shared<int>(i);
		listeners[i] = owners[i];
	}
	owners[3].reset();
	owners[7].reset();
	owners[15].reset();

	for (auto _ : state) {
		for (int n = 0; n < notificationsPerFrame; n++) {
			// NEW: iterate with index under lock — no copy, skip expired
			for (size_t i = 0; i < listeners.size(); i++) {
				if (auto sp = listeners[i].lock()) {
					benchmark::DoNotOptimize(*sp);
				}
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * notificationsPerFrame * listenerCount);
}
BENCHMARK(BM_Notification_NewIterateUnderLock);

// =============================================================================
// MessageManager Single-Lookup Benchmarks
// =============================================================================
// Compares old count()+operator[] (2 lookups) vs new find()+iterator (1 lookup)

static void BM_MessageManager_OldDoubleLookup(benchmark::State& state) {
	std::unordered_map<std::string, std::string> resources;
	for (int i = 0; i < 200; i++) {
		resources["key_" + std::to_string(i)] = "Localized value for key " + std::to_string(i);
	}

	// Keys to look up (mix of hits and misses)
	std::vector<std::string> keys;
	for (int i = 0; i < 50; i++) {
		keys.push_back("key_" + std::to_string(i * 3));       // Hits
		keys.push_back("missing_" + std::to_string(i));        // Misses
	}

	for (auto _ : state) {
		for (const auto& key : keys) {
			// OLD: count() + operator[] — two hash lookups
			if (resources.count(key) > 0) {
				benchmark::DoNotOptimize(resources[key]);
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * keys.size());
}
BENCHMARK(BM_MessageManager_OldDoubleLookup);

static void BM_MessageManager_NewSingleLookup(benchmark::State& state) {
	std::unordered_map<std::string, std::string> resources;
	for (int i = 0; i < 200; i++) {
		resources["key_" + std::to_string(i)] = "Localized value for key " + std::to_string(i);
	}

	std::vector<std::string> keys;
	for (int i = 0; i < 50; i++) {
		keys.push_back("key_" + std::to_string(i * 3));
		keys.push_back("missing_" + std::to_string(i));
	}

	for (auto _ : state) {
		for (const auto& key : keys) {
			// NEW: find() + iterator — single hash lookup
			auto it = resources.find(key);
			if (it != resources.end()) {
				benchmark::DoNotOptimize(it->second);
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * keys.size());
}
BENCHMARK(BM_MessageManager_NewSingleLookup);

