#include "pch.h"
#include "ChannelF/ChannelFTypes.h"
#include "ChannelF/ChannelFDefaultVideoFilter.h"

// ============================================================================
// Video filter palette lookup benchmarks
// ============================================================================

static void BM_ChannelFVideo_PaletteLookup_Single(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t color = ChannelFDefaultVideoFilter::ChannelFPalette[2];
		benchmark::DoNotOptimize(color);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFVideo_PaletteLookup_Single);

static void BM_ChannelFVideo_PaletteLookup_AllFour(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t c0 = ChannelFDefaultVideoFilter::ChannelFPalette[0];
		uint32_t c1 = ChannelFDefaultVideoFilter::ChannelFPalette[1];
		uint32_t c2 = ChannelFDefaultVideoFilter::ChannelFPalette[2];
		uint32_t c3 = ChannelFDefaultVideoFilter::ChannelFPalette[3];
		benchmark::DoNotOptimize(c0 ^ c1 ^ c2 ^ c3);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_ChannelFVideo_PaletteLookup_AllFour);

// ============================================================================
// Full-frame VRAM-to-ARGB conversion benchmark
// ============================================================================

static void BM_ChannelFVideo_FullFrameConvert(benchmark::State& state) {
	// Simulate VRAM (128x64 pixels, 1 byte per pixel, 2-bit color index)
	alignas(64) uint16_t vram[128 * 64];
	for (uint32_t i = 0; i < 128 * 64; i++) {
		vram[i] = static_cast<uint16_t>(i & 0x03);
	}

	alignas(64) uint32_t output[128 * 64];

	for (auto _ : state) {
		for (uint32_t i = 0; i < 128 * 64; i++) {
			output[i] = ChannelFDefaultVideoFilter::ChannelFPalette[vram[i] & 0x03];
		}
		benchmark::DoNotOptimize(output[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 128 * 64 * sizeof(uint32_t));
}
BENCHMARK(BM_ChannelFVideo_FullFrameConvert);

// ============================================================================
// VRAM index computation benchmark
// ============================================================================

static void BM_ChannelFVideo_VramIndexCalc(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t total = 0;
		for (uint8_t y = 0; y < 64; y++) {
			for (uint8_t x = 0; x < 128; x++) {
				total += static_cast<uint32_t>(y) * 128u + x;
			}
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 128 * 64);
}
BENCHMARK(BM_ChannelFVideo_VramIndexCalc);

// ============================================================================
// Audio square wave period computation benchmark
// ============================================================================

static void BM_ChannelFAudio_PeriodCalc_AllFreqs(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t total = 0;
		for (uint16_t freq = 1; freq < 256; freq++) {
			uint32_t period = ChannelFConstants::CpuClockHz / (2u * freq);
			total += period;
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 255);
}
BENCHMARK(BM_ChannelFAudio_PeriodCalc_AllFreqs);

// ============================================================================
// Input encoding benchmark
// ============================================================================

static void BM_ChannelFInput_EncodeController(benchmark::State& state) {
	// Benchmark active-low encoding for all 256 button combinations
	for (auto _ : state) {
		uint32_t checksum = 0;
		for (uint16_t combo = 0; combo < 256; combo++) {
			uint8_t value = 0xff;
			if (combo & 0x01) value &= ~0x01;
			if (combo & 0x02) value &= ~0x02;
			if (combo & 0x04) value &= ~0x04;
			if (combo & 0x08) value &= ~0x08;
			if (combo & 0x10) value &= ~0x10;
			if (combo & 0x20) value &= ~0x20;
			if (combo & 0x40) value &= ~0x40;
			if (combo & 0x80) value &= ~0x80;
			checksum += value;
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_ChannelFInput_EncodeController);

// ============================================================================
// State struct copy benchmark
// ============================================================================

static void BM_ChannelFState_CopyFull(benchmark::State& state) {
	ChannelFState src = {};
	src.Cpu.PC0 = 0x0800;
	src.Cpu.A = 0x42;
	src.Cpu.CycleCount = 12345;
	src.Video.Color = 2;
	src.Audio.SoundEnabled = true;
	src.Ports.Port4 = 0x55;

	for (auto _ : state) {
		ChannelFState dst = src;
		benchmark::DoNotOptimize(dst.Cpu.PC0);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * sizeof(ChannelFState));
}
BENCHMARK(BM_ChannelFState_CopyFull);

static void BM_ChannelFState_CopyCpuOnly(benchmark::State& state) {
	ChannelFCpuState src = {};
	src.PC0 = 0x0800;
	src.A = 0x42;
	src.CycleCount = 12345;
	for (int i = 0; i < 64; i++) src.Scratchpad[i] = static_cast<uint8_t>(i);

	for (auto _ : state) {
		ChannelFCpuState dst = src;
		benchmark::DoNotOptimize(dst.PC0);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * sizeof(ChannelFCpuState));
}
BENCHMARK(BM_ChannelFState_CopyCpuOnly);

// ============================================================================
// Scratchpad access pattern benchmarks
// ============================================================================

static void BM_ChannelFCpu_ScratchpadSequentialRead(benchmark::State& state) {
	ChannelFCpuState cpu = {};
	for (int i = 0; i < 64; i++) cpu.Scratchpad[i] = static_cast<uint8_t>(i * 3 + 7);

	for (auto _ : state) {
		uint32_t sum = 0;
		for (int i = 0; i < 64; i++) {
			sum += cpu.Scratchpad[i];
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_ChannelFCpu_ScratchpadSequentialRead);

static void BM_ChannelFCpu_ScratchpadISARAccess(benchmark::State& state) {
	ChannelFCpuState cpu = {};
	for (int i = 0; i < 64; i++) cpu.Scratchpad[i] = static_cast<uint8_t>(i);

	// Simulate ISAR-based indirect access pattern
	for (auto _ : state) {
		uint32_t sum = 0;
		uint8_t isar = 0;
		for (int i = 0; i < 256; i++) {
			sum += cpu.Scratchpad[isar & 0x3f];
			isar = (isar + 1) & 0x3f;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_ChannelFCpu_ScratchpadISARAccess);
