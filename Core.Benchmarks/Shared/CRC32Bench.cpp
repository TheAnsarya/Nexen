#include "pch.h"
#include "Utilities/CRC32.h"

// ===== Small Buffer (typical ROM header) =====

static void BM_CRC32_SmallBuffer_16B(benchmark::State& state) {
	uint8_t data[16];
	memset(data, 0x42, sizeof(data));
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data, 16));
	}
	state.SetBytesProcessed(state.iterations() * 16);
}
BENCHMARK(BM_CRC32_SmallBuffer_16B);

// ===== Medium Buffer (save state chunk) =====

static void BM_CRC32_MediumBuffer_4KB(benchmark::State& state) {
	std::vector<uint8_t> data(4096);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)(i & 0xFF);
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data.data(), data.size()));
	}
	state.SetBytesProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_CRC32_MediumBuffer_4KB);

// ===== Large Buffer (full ROM CRC) =====

static void BM_CRC32_LargeBuffer_512KB(benchmark::State& state) {
	std::vector<uint8_t> data(512 * 1024);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)((i * 37) & 0xFF);
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data.data(), data.size()));
	}
	state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CRC32_LargeBuffer_512KB);

static void BM_CRC32_LargeBuffer_2MB(benchmark::State& state) {
	std::vector<uint8_t> data(2 * 1024 * 1024);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)((i * 37) & 0xFF);
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data.data(), data.size()));
	}
	state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CRC32_LargeBuffer_2MB);

// ===== Very Large Buffer (full SNES ROM: 4-6MB) =====

static void BM_CRC32_VeryLargeBuffer_4MB(benchmark::State& state) {
	std::vector<uint8_t> data(4 * 1024 * 1024);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)((i * 37) & 0xFF);
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data.data(), data.size()));
	}
	state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CRC32_VeryLargeBuffer_4MB);

// ===== Vector Overload =====

static void BM_CRC32_Vector_512KB(benchmark::State& state) {
	std::vector<uint8_t> data(512 * 1024);
	for (size_t i = 0; i < data.size(); i++) data[i] = (uint8_t)((i * 37) & 0xFF);
	for (auto _ : state) {
		benchmark::DoNotOptimize(CRC32::GetCRC(data));
	}
	state.SetBytesProcessed(state.iterations() * data.size());
}
BENCHMARK(BM_CRC32_Vector_512KB);
