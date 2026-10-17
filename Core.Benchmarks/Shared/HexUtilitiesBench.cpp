#include "pch.h"
#include "Utilities/HexUtilities.h"

// ===== ToHex Throughput =====

static void BM_HexUtilities_ToHex_Uint8(benchmark::State& state) {
	for (auto _ : state) {
		for (int i = 0; i < 256; i++) {
			benchmark::DoNotOptimize(HexUtilities::ToHex((uint8_t)i));
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_HexUtilities_ToHex_Uint8);

static void BM_HexUtilities_ToHex_Uint16(benchmark::State& state) {
	for (auto _ : state) {
		for (uint16_t i = 0; i < 1000; i++) {
			benchmark::DoNotOptimize(HexUtilities::ToHex(i));
		}
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_HexUtilities_ToHex_Uint16);

static void BM_HexUtilities_ToHex_Uint32_Variable(benchmark::State& state) {
	uint32_t values[] = {0x42, 0x1234, 0x123456, 0x12345678};
	for (auto _ : state) {
		for (uint32_t val : values) {
			benchmark::DoNotOptimize(HexUtilities::ToHex(val));
		}
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_HexUtilities_ToHex_Uint32_Variable);

static void BM_HexUtilities_ToHex32_Full(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex32(0xDEADBEEF));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_ToHex32_Full);

static void BM_HexUtilities_ToHex_Uint64(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex((uint64_t)0x0123456789ABCDEF));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_ToHex_Uint64);

// ===== ToHexChar (pointer, no allocation) =====

static void BM_HexUtilities_ToHexChar(benchmark::State& state) {
	for (auto _ : state) {
		for (int i = 0; i < 256; i++) {
			benchmark::DoNotOptimize(HexUtilities::ToHexChar((uint8_t)i));
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_HexUtilities_ToHexChar);

// ===== FromHex Parsing =====

static void BM_HexUtilities_FromHex_Short(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::FromHex("FF"));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_FromHex_Short);

static void BM_HexUtilities_FromHex_Long(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::FromHex("DEADBEEF"));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_FromHex_Long);

// ===== Vector hex dump (typical debugger usage) =====

static void BM_HexUtilities_ToHex_Vector_256Bytes(benchmark::State& state) {
	std::vector<uint8_t> data(256);
	for (int i = 0; i < 256; i++) data[i] = (uint8_t)i;

	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex(data));
	}
	state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(BM_HexUtilities_ToHex_Vector_256Bytes);

static void BM_HexUtilities_ToHex_Vector_4K(benchmark::State& state) {
	std::vector<uint8_t> data(4096);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)(i & 0xFF);

	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex(data));
	}
	state.SetBytesProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_HexUtilities_ToHex_Vector_4K);

// ===== ToHex20/24 (address formatting) =====

static void BM_HexUtilities_ToHex20_Address(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex20(0x7E200));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_ToHex20_Address);

static void BM_HexUtilities_ToHex24_Address(benchmark::State& state) {
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::ToHex24(0x7E2000));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_ToHex24_Address);

// ===== FromHex LUT Benchmark =====
// Benchmarks the constexpr LUT-based FromHex parsing (eliminates 3 branches per nibble)

static void BM_HexUtilities_FromHex_Batch(benchmark::State& state) {
	// Simulate typical debugger usage: parse many hex addresses
	const char* addresses[] = {"0000", "00FF", "1234", "ABCD", "FFFF", "7E2000", "DEADBEEF"};
	for (auto _ : state) {
		for (const char* addr : addresses) {
			benchmark::DoNotOptimize(HexUtilities::FromHex(addr));
		}
	}
	state.SetItemsProcessed(state.iterations() * 7);
}
BENCHMARK(BM_HexUtilities_FromHex_Batch);

static void BM_HexUtilities_FromHex_MixedCase(benchmark::State& state) {
	// LUT handles all cases uniformly — no branch per character
	for (auto _ : state) {
		benchmark::DoNotOptimize(HexUtilities::FromHex("aAbBcCdD"));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HexUtilities_FromHex_MixedCase);
