#include "pch.h"
#include <benchmark/benchmark.h>
#include <array>
#include <cstring>
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan CPU, Timer, DMA, and APU Benchmarks
// =============================================================================

// --- CPU Flag Pack/Unpack (hot path in every instruction) ---

namespace {
	uint16_t PackFlags(bool carry, bool zero, bool sign, bool overflow,
		bool auxCarry, bool parity, bool mode, bool direction, bool interrupt, bool trap) {
		uint16_t base = 0x7002;
		if (carry) base |= 0x0001;
		if (parity) base |= 0x0004;
		if (auxCarry) base |= 0x0010;
		if (zero) base |= 0x0040;
		if (sign) base |= 0x0080;
		if (trap) base |= 0x0100;
		if (interrupt) base |= 0x0200;
		if (direction) base |= 0x0400;
		if (overflow) base |= 0x0800;
		if (mode) base |= 0x8000;
		return base;
	}

	struct UnpackedFlags {
		bool carry, zero, sign, overflow, auxCarry, parity, mode, direction, interrupt, trap;
	};

	UnpackedFlags UnpackFlags(uint16_t packed) {
		return {
			(packed & 0x0001) != 0,
			(packed & 0x0040) != 0,
			(packed & 0x0080) != 0,
			(packed & 0x0800) != 0,
			(packed & 0x0010) != 0,
			(packed & 0x0004) != 0,
			(packed & 0x8000) != 0,
			(packed & 0x0400) != 0,
			(packed & 0x0200) != 0,
			(packed & 0x0100) != 0
		};
	}

	// Parity computation (hot path for every ALU op)
	bool ComputeParity(uint8_t val) {
		val ^= val >> 4;
		val ^= val >> 2;
		val ^= val >> 1;
		return (val & 1) == 0;
	}

	// 20-bit segment:offset physical address (every memory access)
	uint32_t ToPhysical(uint16_t segment, uint16_t offset) {
		return (static_cast<uint32_t>(segment) << 4) + offset;
	}
}

// --- CPU Flag Benchmarks ---

static void BM_WsCpu_FlagPack_AllCombinations(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t checksum = 0;
		for (uint16_t combo = 0; combo < 1024; combo++) {
			uint16_t packed = PackFlags(
				combo & 1, (combo >> 1) & 1, (combo >> 2) & 1, (combo >> 3) & 1,
				(combo >> 4) & 1, (combo >> 5) & 1, (combo >> 6) & 1, (combo >> 7) & 1,
				(combo >> 8) & 1, (combo >> 9) & 1);
			checksum += packed;
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_WsCpu_FlagPack_AllCombinations);

static void BM_WsCpu_FlagUnpack_AllCombinations(benchmark::State& state) {
	// Pre-pack all 1024 flag combos
	std::array<uint16_t, 1024> packed;
	for (uint16_t combo = 0; combo < 1024; combo++) {
		packed[combo] = PackFlags(
			combo & 1, (combo >> 1) & 1, (combo >> 2) & 1, (combo >> 3) & 1,
			(combo >> 4) & 1, (combo >> 5) & 1, (combo >> 6) & 1, (combo >> 7) & 1,
			(combo >> 8) & 1, (combo >> 9) & 1);
	}

	for (auto _ : state) {
		uint32_t checksum = 0;
		for (uint16_t combo = 0; combo < 1024; combo++) {
			auto flags = UnpackFlags(packed[combo]);
			checksum += flags.carry + flags.zero + flags.sign;
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_WsCpu_FlagUnpack_AllCombinations);

static void BM_WsCpu_FlagRoundtrip(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t checksum = 0;
		for (uint16_t combo = 0; combo < 1024; combo++) {
			uint16_t packed = PackFlags(
				combo & 1, (combo >> 1) & 1, (combo >> 2) & 1, (combo >> 3) & 1,
				(combo >> 4) & 1, (combo >> 5) & 1, (combo >> 6) & 1, (combo >> 7) & 1,
				(combo >> 8) & 1, (combo >> 9) & 1);
			auto flags = UnpackFlags(packed);
			checksum += flags.carry + flags.parity + flags.overflow;
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_WsCpu_FlagRoundtrip);

// --- Parity Computation Benchmark ---

static void BM_WsCpu_ParityCompute_AllBytes(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t count = 0;
		for (uint16_t i = 0; i < 256; i++) {
			if (ComputeParity(static_cast<uint8_t>(i))) count++;
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_WsCpu_ParityCompute_AllBytes);

static void BM_WsCpu_ParityTable_Lookup(benchmark::State& state) {
	// Pre-build table
	std::array<bool, 256> table;
	for (int i = 0; i < 256; i++) {
		table[i] = ComputeParity(static_cast<uint8_t>(i));
	}

	for (auto _ : state) {
		uint32_t count = 0;
		for (uint16_t i = 0; i < 256; i++) {
			if (table[i]) count++;
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_WsCpu_ParityTable_Lookup);

// --- Segment:Offset Address Computation Benchmark ---

static void BM_WsCpu_SegmentOffset_PhysicalAddr(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t checksum = 0;
		for (uint16_t seg = 0; seg < 256; seg++) {
			for (uint16_t off = 0; off < 256; off++) {
				checksum += ToPhysical(static_cast<uint16_t>(seg << 8), static_cast<uint16_t>(off << 8));
			}
		}
		benchmark::DoNotOptimize(checksum);
	}
	state.SetItemsProcessed(state.iterations() * 256 * 256);
}
BENCHMARK(BM_WsCpu_SegmentOffset_PhysicalAddr);

// --- Timer Tick Simulation Benchmark ---

namespace {
	struct SimpleTimer {
		uint16_t counter;
		uint16_t reload;
		bool enabled;
		bool autoReload;

		uint16_t Tick() {
			if (!enabled || counter == 0) return counter;
			counter--;
			if (counter == 0 && autoReload) {
				counter = reload;
			}
			return counter;
		}
	};
}

static void BM_WsTimer_TickSimulation_1000Ticks(benchmark::State& state) {
	for (auto _ : state) {
		SimpleTimer timer = { 1000, 1000, true, true };
		uint32_t total = 0;
		for (int i = 0; i < 1000; i++) {
			total += timer.Tick();
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_WsTimer_TickSimulation_1000Ticks);

static void BM_WsTimer_TickSimulation_OneShot(benchmark::State& state) {
	for (auto _ : state) {
		SimpleTimer timer = { 500, 0, true, false };
		uint32_t ticks = 0;
		while (timer.counter > 0) {
			timer.Tick();
			ticks++;
		}
		benchmark::DoNotOptimize(ticks);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_WsTimer_TickSimulation_OneShot);

// --- APU Period Computation Benchmark ---

namespace {
	uint32_t ApuComputePeriod(uint16_t freq) {
		return (2048 - (freq & 0x7ff));
	}

	uint8_t GetWaveTableSample(const uint8_t* waveData, uint8_t sampleIndex) {
		uint8_t byteIndex = sampleIndex >> 1;
		return (sampleIndex & 1) ? (waveData[byteIndex] >> 4) : (waveData[byteIndex] & 0x0f);
	}

	uint16_t LfsrStep(uint16_t lfsr, uint8_t tapMode) {
		uint16_t tap;
		switch (tapMode & 0x03) {
			case 0: tap = ((lfsr >> 7) ^ (lfsr >> 0)) & 1; break;
			case 1: tap = ((lfsr >> 10) ^ (lfsr >> 3)) & 1; break;
			case 2: tap = ((lfsr >> 13) ^ (lfsr >> 0)) & 1; break;
			default: tap = ((lfsr >> 4) ^ (lfsr >> 0)) & 1; break;
		}
		lfsr = (lfsr >> 1) | (tap << 14);
		return lfsr;
	}
}

static void BM_WsApu_PeriodCompute_AllFreqs(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t total = 0;
		for (uint16_t freq = 0; freq < 2048; freq++) {
			total += ApuComputePeriod(freq);
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 2048);
}
BENCHMARK(BM_WsApu_PeriodCompute_AllFreqs);

static void BM_WsApu_WaveTableSample_FullTable(benchmark::State& state) {
	uint8_t waveData[16] = {};
	for (int i = 0; i < 16; i++) waveData[i] = static_cast<uint8_t>(i * 17);

	for (auto _ : state) {
		uint32_t total = 0;
		for (uint8_t s = 0; s < 32; s++) {
			total += GetWaveTableSample(waveData, s);
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 32);
}
BENCHMARK(BM_WsApu_WaveTableSample_FullTable);

static void BM_WsApu_LfsrStep_1000Steps(benchmark::State& state) {
	for (auto _ : state) {
		uint16_t lfsr = 0x7fff;
		for (int i = 0; i < 1000; i++) {
			lfsr = LfsrStep(lfsr, 0);
		}
		benchmark::DoNotOptimize(lfsr);
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_WsApu_LfsrStep_1000Steps);

static void BM_WsApu_LfsrStep_AllTapModes(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t total = 0;
		for (uint8_t tap = 0; tap < 4; tap++) {
			uint16_t lfsr = 0x7fff;
			for (int i = 0; i < 250; i++) {
				lfsr = LfsrStep(lfsr, tap);
			}
			total += lfsr;
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_WsApu_LfsrStep_AllTapModes);

// --- State Copy Benchmarks ---

static void BM_WsState_CopyFull(benchmark::State& state) {
	WsState src = {};
	src.Cpu.AX = 0x1234;
	src.Cpu.BX = 0x5678;
	src.Cpu.CycleCount = 999999;
	src.Ppu.Scanline = 144;

	for (auto _ : state) {
		WsState dst = src;
		benchmark::DoNotOptimize(dst.Cpu.AX);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * sizeof(WsState));
}
BENCHMARK(BM_WsState_CopyFull);

static void BM_WsState_CopyCpuOnly(benchmark::State& state) {
	WsCpuState src = {};
	src.AX = 0x1234;
	src.BX = 0x5678;
	src.CycleCount = 999999;
	src.CS = 0xf000;
	src.IP = 0x0100;

	for (auto _ : state) {
		WsCpuState dst = src;
		benchmark::DoNotOptimize(dst.AX);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * sizeof(WsCpuState));
}
BENCHMARK(BM_WsState_CopyCpuOnly);

// --- EEPROM Address Masking Benchmark ---

namespace {
	uint16_t EepromAddressMask(uint16_t addr, uint8_t sizeKind) {
		switch (sizeKind) {
			case 0: return addr & 0x003f; // 128B = 6-bit
			case 1: return addr & 0x01ff; // 1KB = 9-bit
			case 2: return addr & 0x03ff; // 2KB = 10-bit
			default: return addr & 0x003f;
		}
	}
}

static void BM_WsEeprom_AddressMask_AllSizes(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t total = 0;
		for (uint8_t size = 0; size < 3; size++) {
			for (uint16_t addr = 0; addr < 1024; addr++) {
				total += EepromAddressMask(addr, size);
			}
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 3 * 1024);
}
BENCHMARK(BM_WsEeprom_AddressMask_AllSizes);
