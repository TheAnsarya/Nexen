#include "pch.h"
#include <vector>
#include <cstring>
#include <numeric>
#include <algorithm>
#include "Utilities/miniz.h"

// =============================================================================
// Save State Compression Algorithm Comparison Benchmarks
// =============================================================================
// Save states are the most frequent large-buffer operations during emulation.
// The current implementation uses miniz (zlib-compatible) at MZ_BEST_SPEED (level 1).
//
// This benchmark compares:
//   1. miniz level 1 (MZ_BEST_SPEED) — current implementation
//   2. miniz level 6 (MZ_DEFAULT_LEVEL) — better ratio, more CPU
//   3. miniz level 0 (no compression / store-only) — fastest possible
//   4. Raw memcpy — theoretical floor (no compression at all)
//
// Payload sizes tested match real emulator save state sizes:
//   - 100KB: NES save state (CPU + PPU + cartridge + RAM)
//   - 200KB: Game Boy / GBA save state
//   - 400KB: SNES save state (64KB VRAM + 128KB WRAM + registers)
//
// These benchmarks measure both compression and decompression to support the
// async save state investigation in issue #422.
//
// The benchmarks use realistic payload content:
//   - Flat patterns (best-case for compressors)
//   - Pseudorandom content (worst-case for compressors, typical for VRAM)
//   - Mixed content (realistic: mostly zeros with scattered data)
// =============================================================================

// ---------------------------------------------------------------------------
// Payload Generators
// ---------------------------------------------------------------------------

namespace {
	// Generate a highly compressible payload (typical CPU/PPU state registers)
	// Large runs of zeros with occasional non-zero values
	std::vector<uint8_t> MakeCompressiblePayload(size_t size) {
		std::vector<uint8_t> data(size, 0);
		// Scatter ~5% non-zero bytes (typical for register state)
		for (size_t i = 0; i < size; i += 20) {
			data[i] = static_cast<uint8_t>((i * 37 + 0xAB) & 0xFF);
		}
		return data;
	}

	// Generate a pseudorandom payload (typical for VRAM/tileset data)
	// Low compressibility — each byte is unpredictable
	std::vector<uint8_t> MakeRandomPayload(size_t size) {
		std::vector<uint8_t> data(size);
		// Linear congruential generator (fast, deterministic)
		uint32_t lcg = 0xDEADBEEF;
		for (auto& b : data) {
			lcg = lcg * 1664525u + 1013904223u;
			b = static_cast<uint8_t>(lcg >> 24);
		}
		return data;
	}

	// Generate a mixed payload (realistic SNES state:
	// WRAM=random, VRAM=tiled patterns, registers=sparse)
	std::vector<uint8_t> MakeMixedPayload(size_t size) {
		std::vector<uint8_t> data(size, 0);
		uint32_t lcg = 0xCAFEBABE;

		// First third: random (simulates WRAM)
		for (size_t i = 0; i < size / 3; i++) {
			lcg = lcg * 1664525u + 1013904223u;
			data[i] = static_cast<uint8_t>(lcg >> 24);
		}
		// Middle third: repeating tile pattern (simulates VRAM)
		for (size_t i = size / 3; i < 2 * size / 3; i++) {
			data[i] = static_cast<uint8_t>(i & 0x1F); // 32-byte period
		}
		// Final third: sparse (simulates registers/zero-page)
		for (size_t i = 2 * size / 3; i < size; i += 16) {
			data[i] = static_cast<uint8_t>((i * 13) & 0xFF);
		}
		return data;
	}

	// Compress a buffer with miniz at the given level
	// Returns the compressed output vector (with size header for round-trip fidelity)
	std::vector<uint8_t> CompressMiniz(const std::vector<uint8_t>& input, int level) {
		mz_ulong compressedSize = mz_compressBound(static_cast<mz_ulong>(input.size()));
		std::vector<uint8_t> output(compressedSize + 8);

		// Write 8-byte header: [originalSize:4][compressedSize:4]
		uint32_t originalSize = static_cast<uint32_t>(input.size());
		memcpy(output.data(), &originalSize, 4);

		mz_ulong actualSize = compressedSize;
		compress2(output.data() + 8, &actualSize,
		          input.data(), static_cast<mz_ulong>(input.size()), level);

		uint32_t csize = static_cast<uint32_t>(actualSize);
		memcpy(output.data() + 4, &csize, 4);
		output.resize(8 + actualSize);
		return output;
	}

	// Decompress a buffer compressed by CompressMiniz
	std::vector<uint8_t> DecompressMiniz(const std::vector<uint8_t>& input) {
		uint32_t originalSize, compressedSize;
		memcpy(&originalSize,   input.data(),     4);
		memcpy(&compressedSize, input.data() + 4, 4);
		std::vector<uint8_t> output(originalSize);
		mz_ulong decompSize = originalSize;
		uncompress(output.data(), &decompSize, input.data() + 8, compressedSize);
		return output;
	}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Compression Benchmarks — NES save state size (100KB)
// ---------------------------------------------------------------------------

// Benchmark: miniz level 1 (MZ_BEST_SPEED) — current save state compressor
static void BM_SaveStateCompress_Miniz_Level1_100KB_Compressible(benchmark::State& state) {
	auto payload = MakeCompressiblePayload(100 * 1024);
	std::vector<uint8_t> output;
	output.reserve(mz_compressBound(static_cast<mz_ulong>(payload.size())) + 8);

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_BEST_SPEED);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level1_100KB_Compressible);

// Benchmark: miniz level 1 with random (low-compressibility) payload
static void BM_SaveStateCompress_Miniz_Level1_100KB_Random(benchmark::State& state) {
	auto payload = MakeRandomPayload(100 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_BEST_SPEED);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level1_100KB_Random);

// Benchmark: miniz level 6 (default) — better ratio, more CPU
static void BM_SaveStateCompress_Miniz_Level6_100KB_Compressible(benchmark::State& state) {
	auto payload = MakeCompressiblePayload(100 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_DEFAULT_LEVEL);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level6_100KB_Compressible);

// Benchmark: miniz level 0 (store-only — no compression)
static void BM_SaveStateCompress_Miniz_Level0_100KB(benchmark::State& state) {
	auto payload = MakeMixedPayload(100 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_NO_COMPRESSION);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level0_100KB);

// Benchmark: Raw memcpy baseline — no compression, just buffer copy
// This is the theoretical floor: what could async compression save us?
static void BM_SaveStateCompress_Memcpy_Baseline_100KB(benchmark::State& state) {
	auto payload = MakeMixedPayload(100 * 1024);
	std::vector<uint8_t> output(payload.size());

	for (auto _ : state) {
		memcpy(output.data(), payload.data(), payload.size());
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
}
BENCHMARK(BM_SaveStateCompress_Memcpy_Baseline_100KB);

// ---------------------------------------------------------------------------
// Compression Benchmarks — SNES save state size (400KB)
// ---------------------------------------------------------------------------

// Benchmark: miniz level 1, SNES-sized (400KB) compressible payload
static void BM_SaveStateCompress_Miniz_Level1_400KB_Compressible(benchmark::State& state) {
	auto payload = MakeCompressiblePayload(400 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_BEST_SPEED);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level1_400KB_Compressible);

// Benchmark: miniz level 1, SNES-sized (400KB) mixed payload
static void BM_SaveStateCompress_Miniz_Level1_400KB_Mixed(benchmark::State& state) {
	auto payload = MakeMixedPayload(400 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_BEST_SPEED);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level1_400KB_Mixed);

// Benchmark: miniz level 6, SNES-sized (400KB) mixed payload
static void BM_SaveStateCompress_Miniz_Level6_400KB_Mixed(benchmark::State& state) {
	auto payload = MakeMixedPayload(400 * 1024);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, MZ_DEFAULT_LEVEL);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
	state.counters["CompressedBytes"] = benchmark::Counter(
		static_cast<double>(output.size()),
		benchmark::Counter::kAvgIterations);
}
BENCHMARK(BM_SaveStateCompress_Miniz_Level6_400KB_Mixed);

// Benchmark: Raw memcpy baseline — 400KB
static void BM_SaveStateCompress_Memcpy_Baseline_400KB(benchmark::State& state) {
	auto payload = MakeMixedPayload(400 * 1024);
	std::vector<uint8_t> output(payload.size());

	for (auto _ : state) {
		memcpy(output.data(), payload.data(), payload.size());
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
}
BENCHMARK(BM_SaveStateCompress_Memcpy_Baseline_400KB);

// ---------------------------------------------------------------------------
// Decompression Benchmarks (async issue #422: background decompress for load)
// ---------------------------------------------------------------------------

// Benchmark: Decompress 100KB compressible state (miniz level 1 → decompress)
static void BM_SaveStateDecompress_Miniz_Level1_100KB_Compressible(benchmark::State& state) {
	auto payload = MakeCompressiblePayload(100 * 1024);
	auto compressed = CompressMiniz(payload, MZ_BEST_SPEED);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = DecompressMiniz(compressed);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
}
BENCHMARK(BM_SaveStateDecompress_Miniz_Level1_100KB_Compressible);

// Benchmark: Decompress 100KB random state (low compressibility → fast decompress)
static void BM_SaveStateDecompress_Miniz_Level1_100KB_Random(benchmark::State& state) {
	auto payload = MakeRandomPayload(100 * 1024);
	auto compressed = CompressMiniz(payload, MZ_BEST_SPEED);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = DecompressMiniz(compressed);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
}
BENCHMARK(BM_SaveStateDecompress_Miniz_Level1_100KB_Random);

// Benchmark: Decompress 400KB mixed SNES state (miniz level 1)
static void BM_SaveStateDecompress_Miniz_Level1_400KB_Mixed(benchmark::State& state) {
	auto payload = MakeMixedPayload(400 * 1024);
	auto compressed = CompressMiniz(payload, MZ_BEST_SPEED);
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = DecompressMiniz(compressed);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());
}
BENCHMARK(BM_SaveStateDecompress_Miniz_Level1_400KB_Mixed);

// ---------------------------------------------------------------------------
// Compression Ratio Comparison — Single Pass Measurement
// ---------------------------------------------------------------------------
// Run one iteration each to measure compressed size ratio for each level
// (reported via state.counters["Ratio"]).

static void BM_SaveStateCompress_RatioProfile_AllLevels(benchmark::State& state) {
	const int level = static_cast<int>(state.range(0));
	auto payload = MakeMixedPayload(200 * 1024); // 200KB representative payload
	std::vector<uint8_t> output;

	for (auto _ : state) {
		output = CompressMiniz(payload, level);
		benchmark::DoNotOptimize(output.data());
	}
	state.SetBytesProcessed(state.iterations() * payload.size());

	// Report compression ratio (compressed / original)
	double ratio = static_cast<double>(output.size()) / static_cast<double>(payload.size());
	state.counters["Ratio"] = benchmark::Counter(
		ratio,
		benchmark::Counter::kAvgIterations);
	state.counters["CompressedKB"] = benchmark::Counter(
		static_cast<double>(output.size()) / 1024.0,
		benchmark::Counter::kAvgIterations);
}
// Test levels 0, 1, 3, 6, 9 to profile speed vs ratio tradeoff
BENCHMARK(BM_SaveStateCompress_RatioProfile_AllLevels)->Arg(0)->Arg(1)->Arg(3)->Arg(6)->Arg(9);
