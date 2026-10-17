#include "pch.h"
// AllocOptBench.cpp
// Benchmarks proving Phase 16.6 allocation optimizations:
// - vector reserve() vs unreserved growth
// - std::move vs copy for strings in vector
// - emplace_back vs push_back for vector constructor args
// - const char* lookup table vs std::string lookup table
// Related issue: #533 (Phase 16.6 perf)

#include "benchmark/benchmark.h"
#include <string>
#include <vector>
#include <unordered_map>

// =============================================================================
// Vector Reserve: reserve(64) vs unreserved push_back
// =============================================================================
// Simulates HdData::ToRgb() — always exactly 64 elements

static void BM_VectorGrow_NoReserve_64(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<uint32_t> buffer;
		for (int i = 0; i < 64; i++) {
			buffer.push_back(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(buffer.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_VectorGrow_NoReserve_64);

static void BM_VectorGrow_Reserve_64(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<uint32_t> buffer;
		buffer.reserve(64);
		for (int i = 0; i < 64; i++) {
			buffer.push_back(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(buffer.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_VectorGrow_Reserve_64);

// =============================================================================
// Vector<string> Reserve + Move: GameDatabase pattern (~5000 strings)
// =============================================================================
// Simulates LoadGameDb — large vector of strings without/with reserve+move

static void BM_VectorString_NoReserve_NoCopy_1000(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::string> data;
		for (int i = 0; i < 1000; i++) {
			std::string line = "B750B2EA99AE0EF79B0E1C1116E34C262BFE66CC,NES-TLROM,256,128,H,0,0";
			data.push_back(line);  // copies
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_VectorString_NoReserve_NoCopy_1000);

static void BM_VectorString_Reserve_Move_1000(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::string> data;
		data.reserve(1000);
		for (int i = 0; i < 1000; i++) {
			std::string line = "B750B2EA99AE0EF79B0E1C1116E34C262BFE66CC,NES-TLROM,256,128,H,0,0";
			data.push_back(std::move(line));  // moves — zero heap copy
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_VectorString_Reserve_Move_1000);

// Scale to 5000 (realistic GameDB size)
static void BM_VectorString_NoReserve_NoCopy_5000(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::string> data;
		for (int i = 0; i < 5000; i++) {
			std::string line = "B750B2EA99AE0EF79B0E1C1116E34C262BFE66CC,NES-TLROM,256,128,H,0,0";
			data.push_back(line);
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 5000);
}
BENCHMARK(BM_VectorString_NoReserve_NoCopy_5000);

static void BM_VectorString_Reserve_Move_5000(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::string> data;
		data.reserve(5000);
		for (int i = 0; i < 5000; i++) {
			std::string line = "B750B2EA99AE0EF79B0E1C1116E34C262BFE66CC,NES-TLROM,256,128,H,0,0";
			data.push_back(std::move(line));
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 5000);
}
BENCHMARK(BM_VectorString_Reserve_Move_5000);

// =============================================================================
// Emplace_back vs Push_back for vector<vector<uint8_t>>
// =============================================================================
// Simulates FdsLoader disk loading

static void BM_PushBack_VectorTemp(benchmark::State& state) {
	std::vector<uint8_t> sourceData(56, 0xAB);  // Simulates disk header data
	for (auto _ : state) {
		std::vector<std::vector<uint8_t>> diskHeaders;
		for (int i = 0; i < 4; i++) {  // typical: 2-4 disk sides
			diskHeaders.push_back(std::vector<uint8_t>(sourceData.begin(), sourceData.begin() + 56));
		}
		benchmark::DoNotOptimize(diskHeaders.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_PushBack_VectorTemp);

static void BM_EmplaceBack_VectorArgs(benchmark::State& state) {
	std::vector<uint8_t> sourceData(56, 0xAB);
	for (auto _ : state) {
		std::vector<std::vector<uint8_t>> diskHeaders;
		for (int i = 0; i < 4; i++) {
			diskHeaders.emplace_back(sourceData.begin(), sourceData.begin() + 56);
		}
		benchmark::DoNotOptimize(diskHeaders.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_EmplaceBack_VectorArgs);

// Empty vector: push_back(vector<uint8_t>()) vs emplace_back()
static void BM_PushBack_EmptyVector(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::vector<uint8_t>> data;
		for (int i = 0; i < 4; i++) {
			data.push_back(std::vector<uint8_t>());
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_PushBack_EmptyVector);

static void BM_EmplaceBack_EmptyVector(benchmark::State& state) {
	for (auto _ : state) {
		std::vector<std::vector<uint8_t>> data;
		for (int i = 0; i < 4; i++) {
			data.emplace_back();
		}
		benchmark::DoNotOptimize(data.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_EmplaceBack_EmptyVector);

// =============================================================================
// Const char* lookup table vs std::string lookup table
// =============================================================================
// Simulates SnesDisUtils::OpName[256] — old: string[256], new: const char*[256]

// Only testing lookup + use — the real gain is at static init (256 fewer heap allocs)
// But we can also measure per-lookup cost difference

static std::string g_stringTable[256];
static const char* g_constCharTable[256];

static void InitTables() {
	static bool initialized = false;
	if (!initialized) {
		const char* names[] = {
			"BRK", "ORA", "COP", "ORA", "TSB", "ORA", "ASL", "ORA",
			"PHP", "ORA", "ASL", "PHD", "TSB", "ORA", "ASL", "ORA"
		};
		for (int i = 0; i < 256; i++) {
			g_stringTable[i] = names[i % 16];
			g_constCharTable[i] = names[i % 16];
		}
		initialized = true;
	}
}

// Lookup + strlen on std::string table
static void BM_OpNameLookup_String(benchmark::State& state) {
	InitTables();
	for (auto _ : state) {
		size_t totalLen = 0;
		for (int i = 0; i < 256; i++) {
			totalLen += g_stringTable[i].size();
		}
		benchmark::DoNotOptimize(totalLen);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_OpNameLookup_String);

// Lookup + strlen on const char* table
static void BM_OpNameLookup_ConstChar(benchmark::State& state) {
	InitTables();
	for (auto _ : state) {
		size_t totalLen = 0;
		for (int i = 0; i < 256; i++) {
			totalLen += strlen(g_constCharTable[i]);
		}
		benchmark::DoNotOptimize(totalLen);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_OpNameLookup_ConstChar);

// =============================================================================
// std::move into map vs copy into map (HdNesPack pattern)
// =============================================================================
// Simulates BuildAdditionalTileCache: vector of structs emplaced into unordered_map

struct MockSpriteInfo {
	int32_t offsetX = 0;
	int32_t offsetY = 0;
	uint32_t tileIndex = 0;
	bool ignorePalette = false;
	uint8_t padding[3] = {};
};

static void BM_MapEmplace_CopyVector(benchmark::State& state) {
	std::vector<MockSpriteInfo> additions(8);  // typical: 4-8 additional sprites
	for (size_t i = 0; i < additions.size(); i++) {
		additions[i].tileIndex = static_cast<uint32_t>(i);
	}
	for (auto _ : state) {
		std::unordered_map<uint32_t, std::vector<MockSpriteInfo>> cache;
		// Copy pattern (old)
		auto additionsCopy = additions;
		cache.emplace(42u, additionsCopy);
		benchmark::DoNotOptimize(cache[42].data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MapEmplace_CopyVector);

static void BM_MapEmplace_MoveVector(benchmark::State& state) {
	std::vector<MockSpriteInfo> additions(8);
	for (size_t i = 0; i < additions.size(); i++) {
		additions[i].tileIndex = static_cast<uint32_t>(i);
	}
	for (auto _ : state) {
		std::unordered_map<uint32_t, std::vector<MockSpriteInfo>> cache;
		// Move pattern (new) — transfer ownership, zero copy
		auto additionsCopy = additions;  // still need to create the data
		cache.emplace(42u, std::move(additionsCopy));
		benchmark::DoNotOptimize(cache[42].data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MapEmplace_MoveVector);

// =============================================================================
// String reserve for per-frame GetTextState pattern
// =============================================================================
// Simulates BaseControlDevice::GetTextState — builds string char-by-char

static void BM_GetTextState_NoReserve(benchmark::State& state) {
	const std::string keyNames = "ABsSUDLR";  // NES standard controller
	for (auto _ : state) {
		std::string output = "";
		// Simulate coordinate output
		output += std::to_string(128) + " " + std::to_string(120) + " ";
		// Simulate button state (8 buttons)
		for (size_t i = 0; i < keyNames.size(); i++) {
			output += (i % 2 == 0) ? keyNames[i] : '.';
		}
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetTextState_NoReserve);

static void BM_GetTextState_WithReserve(benchmark::State& state) {
	const std::string keyNames = "ABsSUDLR";
	for (auto _ : state) {
		std::string output;
		output.reserve(keyNames.size() + 20);
		// Simulate coordinate output
		output += std::to_string(128) + " " + std::to_string(120) + " ";
		// Simulate button state
		for (size_t i = 0; i < keyNames.size(); i++) {
			output += (i % 2 == 0) ? keyNames[i] : '.';
		}
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetTextState_WithReserve);

// Palette reserve: HdPackLoader::LoadCustomPalette pattern
static void BM_PaletteLoad_NoReserve(benchmark::State& state) {
	std::vector<uint8_t> fileData(192);  // 64 colors × 3 bytes
	for (size_t i = 0; i < fileData.size(); i++) {
		fileData[i] = static_cast<uint8_t>(i);
	}
	for (auto _ : state) {
		std::vector<uint32_t> paletteData;
		for (size_t i = 0; i < fileData.size(); i += 3) {
			paletteData.push_back(0xFF000000 | (fileData[i] << 16) | (fileData[i + 1] << 8) | fileData[i + 2]);
		}
		benchmark::DoNotOptimize(paletteData.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_PaletteLoad_NoReserve);

static void BM_PaletteLoad_WithReserve(benchmark::State& state) {
	std::vector<uint8_t> fileData(192);
	for (size_t i = 0; i < fileData.size(); i++) {
		fileData[i] = static_cast<uint8_t>(i);
	}
	for (auto _ : state) {
		std::vector<uint32_t> paletteData;
		paletteData.reserve(fileData.size() / 3);
		for (size_t i = 0; i < fileData.size(); i += 3) {
			paletteData.push_back(0xFF000000 | (fileData[i] << 16) | (fileData[i + 1] << 8) | fileData[i + 2]);
		}
		benchmark::DoNotOptimize(paletteData.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_PaletteLoad_WithReserve);
