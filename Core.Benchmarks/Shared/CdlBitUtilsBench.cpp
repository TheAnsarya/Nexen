#include "pch.h"
#include <benchmark/benchmark.h>
#include <cstring>
#include <memory>
#include "Debugger/DebugTypes.h"
#include "Utilities/BitUtilities.h"

// =============================================================================
// Lightweight CDL Recorder Benchmarks
// =============================================================================
// Benchmarks for CDL data management operations:
// - Statistics computation (hot path during UI refresh)
// - Flag read/write throughput
// - Data buffer operations
// - File I/O (save/load)

namespace {
	// Simulate CDL operations without IConsole dependency
	struct CdlBenchState {
		std::unique_ptr<uint8_t[]> data;
		uint32_t size;

		CdlBenchState(uint32_t romSize) : size(romSize) {
			data = std::make_unique<uint8_t[]>(romSize);
			memset(data.get(), 0, romSize);
		}
	};
} // namespace

// =============================================================================
// Statistics Computation
// =============================================================================

static void BM_CdlStatistics_16KB(benchmark::State& state) {
	const uint32_t size = 16384;
	CdlBenchState cdl(size);

	// Realistic CDL data: ~30% code, ~20% data, ~5% both, rest unmarked
	for (uint32_t i = 0; i < size; i++) {
		if (i % 10 < 3) cdl.data[i] = CdlFlags::Code;
		else if (i % 10 < 5) cdl.data[i] = CdlFlags::Data;
		else if (i % 10 == 5) cdl.data[i] = CdlFlags::Code | CdlFlags::Data;
	}

	for (auto _ : state) {
		uint32_t codeSize = 0, dataSize = 0, bothSize = 0;
		for (uint32_t i = 0; i < size; i++) {
			uint32_t isCode = (uint32_t)(cdl.data[i] & CdlFlags::Code);
			uint32_t isData = (uint32_t)(cdl.data[i] & CdlFlags::Data) >> 1;
			codeSize += isCode;
			dataSize += isData;
			bothSize += isCode & isData;
		}
		dataSize -= bothSize;

		benchmark::DoNotOptimize(codeSize);
		benchmark::DoNotOptimize(dataSize);
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlStatistics_16KB);

static void BM_CdlStatistics_256KB(benchmark::State& state) {
	const uint32_t size = 262144;
	CdlBenchState cdl(size);

	for (uint32_t i = 0; i < size; i++) {
		cdl.data[i] = static_cast<uint8_t>(i & 0x03); // 0, 1, 2, 3 pattern
	}

	for (auto _ : state) {
		uint32_t codeSize = 0, dataSize = 0, bothSize = 0;
		for (uint32_t i = 0; i < size; i++) {
			uint32_t isCode = (uint32_t)(cdl.data[i] & CdlFlags::Code);
			uint32_t isData = (uint32_t)(cdl.data[i] & CdlFlags::Data) >> 1;
			codeSize += isCode;
			dataSize += isData;
			bothSize += isCode & isData;
		}
		dataSize -= bothSize;
		benchmark::DoNotOptimize(codeSize);
		benchmark::DoNotOptimize(dataSize);
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlStatistics_256KB);

static void BM_CdlStatistics_1MB(benchmark::State& state) {
	const uint32_t size = 1048576;
	CdlBenchState cdl(size);

	for (uint32_t i = 0; i < size; i++) {
		cdl.data[i] = static_cast<uint8_t>((i * 7) & 0x0f);
	}

	for (auto _ : state) {
		uint32_t codeSize = 0, dataSize = 0, bothSize = 0;
		for (uint32_t i = 0; i < size; i++) {
			uint32_t isCode = (uint32_t)(cdl.data[i] & CdlFlags::Code);
			uint32_t isData = (uint32_t)(cdl.data[i] & CdlFlags::Data) >> 1;
			codeSize += isCode;
			dataSize += isData;
			bothSize += isCode & isData;
		}
		dataSize -= bothSize;
		benchmark::DoNotOptimize(codeSize);
		benchmark::DoNotOptimize(dataSize);
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlStatistics_1MB);

// =============================================================================
// CDL Flag Read Throughput
// =============================================================================

static void BM_CdlFlagRead_Sequential(benchmark::State& state) {
	const uint32_t size = 65536;
	CdlBenchState cdl(size);
	memset(cdl.data.get(), CdlFlags::Code, size);

	for (auto _ : state) {
		uint8_t acc = 0;
		for (uint32_t i = 0; i < size; i++) {
			acc |= cdl.data[i];
		}
		benchmark::DoNotOptimize(acc);
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlFlagRead_Sequential);

static void BM_CdlFlagWrite_OrAccumulate(benchmark::State& state) {
	const uint32_t size = 65536;
	CdlBenchState cdl(size);

	for (auto _ : state) {
		for (uint32_t i = 0; i < size; i++) {
			cdl.data[i] |= CdlFlags::Code;
		}
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlFlagWrite_OrAccumulate);

// =============================================================================
// Buffer Copy (SetCdlData / GetCdlData)
// =============================================================================

static void BM_CdlBufferCopy_16KB(benchmark::State& state) {
	const uint32_t size = 16384;
	auto src = std::make_unique<uint8_t[]>(size);
	auto dst = std::make_unique<uint8_t[]>(size);
	memset(src.get(), 0xaa, size);

	for (auto _ : state) {
		memcpy(dst.get(), src.get(), size);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlBufferCopy_16KB);

static void BM_CdlBufferCopy_1MB(benchmark::State& state) {
	const uint32_t size = 1048576;
	auto src = std::make_unique<uint8_t[]>(size);
	auto dst = std::make_unique<uint8_t[]>(size);
	memset(src.get(), 0xaa, size);

	for (auto _ : state) {
		memcpy(dst.get(), src.get(), size);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlBufferCopy_1MB);

// =============================================================================
// Reset (memset to zero)
// =============================================================================

static void BM_CdlReset_256KB(benchmark::State& state) {
	const uint32_t size = 262144;
	CdlBenchState cdl(size);
	memset(cdl.data.get(), 0xff, size);

	for (auto _ : state) {
		memset(cdl.data.get(), 0, size);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(int64_t(state.iterations()) * size);
}
BENCHMARK(BM_CdlReset_256KB);

// =============================================================================
// BitUtilities Benchmarks
// =============================================================================

static void BM_BitUtilities_GetBits_Throughput(benchmark::State& state) {
	uint32_t values[256];
	for (int i = 0; i < 256; i++) {
		values[i] = static_cast<uint32_t>(i * 0x01010101);
	}

	for (auto _ : state) {
		uint32_t acc = 0;
		for (int i = 0; i < 256; i++) {
			acc += BitUtilities::GetBits<0>(values[i]);
			acc += BitUtilities::GetBits<8>(values[i]);
			acc += BitUtilities::GetBits<16>(values[i]);
			acc += BitUtilities::GetBits<24>(values[i]);
		}
		benchmark::DoNotOptimize(acc);
	}
	state.SetItemsProcessed(int64_t(state.iterations()) * 256 * 4);
}
BENCHMARK(BM_BitUtilities_GetBits_Throughput);

static void BM_BitUtilities_SetBits_Throughput(benchmark::State& state) {
	uint32_t dst[256] = {};

	for (auto _ : state) {
		for (int i = 0; i < 256; i++) {
			BitUtilities::SetBits<0>(dst[i], static_cast<uint8_t>(i));
			BitUtilities::SetBits<8>(dst[i], static_cast<uint8_t>(i ^ 0xff));
			BitUtilities::SetBits<16>(dst[i], static_cast<uint8_t>(i >> 1));
			BitUtilities::SetBits<24>(dst[i], static_cast<uint8_t>(i << 1));
		}
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(int64_t(state.iterations()) * 256 * 4);
}
BENCHMARK(BM_BitUtilities_SetBits_Throughput);
