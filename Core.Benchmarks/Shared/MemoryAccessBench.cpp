#include "pch.h"
#include <array>
#include <random>

// =============================================================================
// Memory Access Pattern Benchmarks
// =============================================================================
// These benchmarks test memory access patterns common in emulation.
// They help identify cache efficiency and memory bandwidth bottlenecks.

// Benchmark sequential memory reads (best-case for cache)
static void BM_Memory_SequentialRead(benchmark::State& state) {
	const size_t size = state.range(0);
	std::vector<uint8_t> memory(size, 0x42);
	
	for (auto _ : state) {
		uint32_t sum = 0;
		for (size_t i = 0; i < size; i++) {
			sum += memory[i];
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_Memory_SequentialRead)->Range(2048, 1 << 20);  // 2KB to 1MB

// Benchmark sequential memory writes
static void BM_Memory_SequentialWrite(benchmark::State& state) {
	const size_t size = state.range(0);
	std::vector<uint8_t> memory(size);
	
	for (auto _ : state) {
		for (size_t i = 0; i < size; i++) {
			memory[i] = static_cast<uint8_t>(i);
		}
		benchmark::DoNotOptimize(memory.data());
	}
	state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_Memory_SequentialWrite)->Range(2048, 1 << 20);

// Benchmark random memory reads (worst-case for cache)
static void BM_Memory_RandomRead(benchmark::State& state) {
	const size_t size = state.range(0);
	std::vector<uint8_t> memory(size, 0x42);
	std::vector<uint32_t> indices(1024);
	
	// Pre-generate random indices
	std::mt19937 gen(42);  // Fixed seed for reproducibility
	std::uniform_int_distribution<uint32_t> dist(0, static_cast<uint32_t>(size - 1));
	for (auto& idx : indices) {
		idx = dist(gen);
	}
	
	for (auto _ : state) {
		uint32_t sum = 0;
		for (auto idx : indices) {
			sum += memory[idx];
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * indices.size());
}
BENCHMARK(BM_Memory_RandomRead)->Range(2048, 1 << 20);

// Benchmark NES-style memory access with mirroring
static void BM_Memory_NesMirroredRead(benchmark::State& state) {
	// NES has 2KB internal RAM mirrored to 8KB
	std::array<uint8_t, 2048> ram{};
	std::fill(ram.begin(), ram.end(), static_cast<uint8_t>(0x42));
	
	for (auto _ : state) {
		uint32_t sum = 0;
		// Access full 8KB range (includes mirrors)
		for (uint16_t addr = 0; addr < 0x2000; addr++) {
			sum += ram[addr & 0x07FF];  // Mirror mask
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * 0x2000);
}
BENCHMARK(BM_Memory_NesMirroredRead);

// Benchmark SNES-style memory bank access
static void BM_Memory_SnesBankAccess(benchmark::State& state) {
	// Simulate SNES memory banking
	std::array<std::vector<uint8_t>, 256> banks;
	for (auto& bank : banks) {
		bank.resize(0x10000, 0x42);  // 64KB per bank
	}
	
	for (auto _ : state) {
		uint32_t sum = 0;
		// Access across banks (typical ROM access pattern)
		for (uint8_t bank = 0x80; bank < 0x90; bank++) {
			for (uint16_t offset = 0x8000; offset < 0x8100; offset++) {
				sum += banks[bank][offset];
			}
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * 16 * 256);  // 16 banks * 256 bytes
}
BENCHMARK(BM_Memory_SnesBankAccess);

// Benchmark memory mapping lookup (pointer table approach)
static void BM_Memory_PointerTableLookup(benchmark::State& state) {
	// Simulate memory handler pointer table (common emulator pattern)
	std::array<uint8_t, 2048> ram{};
	std::array<uint8_t, 32768> rom{};
	std::fill(ram.begin(), ram.end(), static_cast<uint8_t>(0x11));
	std::fill(rom.begin(), rom.end(), static_cast<uint8_t>(0x22));
	
	// Page table: 256 pages of 256 bytes each (64KB address space)
	std::array<uint8_t*, 256> pageTable{};
	
	// Map pages (NES-like layout)
	for (int i = 0; i < 8; i++) {
		pageTable[i] = ram.data() + (i & 7) * 256;  // $0000-$1FFF (mirrored)
	}
	for (int i = 0x80; i < 0x100; i++) {
		pageTable[i] = rom.data() + (i - 0x80) * 256;  // $8000-$FFFF
	}
	
	for (auto _ : state) {
		uint32_t sum = 0;
		// Mix of RAM and ROM accesses
		for (int i = 0; i < 1000; i++) {
			uint16_t addr1 = 0x0000 + (i & 0xFF);  // RAM
			uint16_t addr2 = 0x8000 + (i & 0x7FFF);  // ROM
			
			if (pageTable[addr1 >> 8]) sum += pageTable[addr1 >> 8][addr1 & 0xFF];
			if (pageTable[addr2 >> 8]) sum += pageTable[addr2 >> 8][addr2 & 0xFF];
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 2000);
}
BENCHMARK(BM_Memory_PointerTableLookup);

// Benchmark 16-bit memory access (little endian read)
static void BM_Memory_Read16_LittleEndian(benchmark::State& state) {
	std::array<uint8_t, 65536> memory{};
	for (size_t i = 0; i < memory.size(); i++) {
		memory[i] = static_cast<uint8_t>(i);
	}
	
	for (auto _ : state) {
		uint32_t sum = 0;
		for (uint16_t addr = 0; addr < 0x8000; addr += 2) {
			// Little-endian 16-bit read
			uint16_t value = memory[addr] | (static_cast<uint16_t>(memory[addr + 1]) << 8);
			sum += value;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * 0x8000);
}
BENCHMARK(BM_Memory_Read16_LittleEndian);

// Benchmark 16-bit memory access (memcpy approach)
static void BM_Memory_Read16_Memcpy(benchmark::State& state) {
	std::array<uint8_t, 65536> memory{};
	for (size_t i = 0; i < memory.size(); i++) {
		memory[i] = static_cast<uint8_t>(i);
	}
	
	for (auto _ : state) {
		uint32_t sum = 0;
		for (uint16_t addr = 0; addr < 0x8000; addr += 2) {
			uint16_t value;
			std::memcpy(&value, &memory[addr], sizeof(value));
			sum += value;
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * 0x8000);
}
BENCHMARK(BM_Memory_Read16_Memcpy);

// Benchmark PPU-like VRAM access pattern
static void BM_Memory_VramAccessPattern(benchmark::State& state) {
	// VRAM is typically accessed in patterns (nametable rows, pattern table reads)
	std::array<uint8_t, 16384> vram{};
	std::fill(vram.begin(), vram.end(), static_cast<uint8_t>(0x42));
	
	for (auto _ : state) {
		uint32_t sum = 0;
		// Simulate nametable row read (32 tiles per row)
		for (int row = 0; row < 30; row++) {
			uint16_t ntBase = static_cast<uint16_t>(0x2000 + row * 32);
			for (int col = 0; col < 32; col++) {
				uint8_t tileIndex = vram[(ntBase + col) & 0x3FFF];
				
				// Pattern table lookup (8 bytes per tile plane)
				uint16_t patternBase = tileIndex * 16;
				for (int y = 0; y < 8; y++) {
					sum += vram[patternBase + y];  // Plane 0
					sum += vram[patternBase + y + 8];  // Plane 1
				}
			}
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Memory_VramAccessPattern);

// Benchmark DMA-like block transfer
static void BM_Memory_DmaTransfer(benchmark::State& state) {
	const size_t size = state.range(0);
	std::vector<uint8_t> src(size, 0x42);
	std::vector<uint8_t> dst(size);
	
	for (auto _ : state) {
		std::memcpy(dst.data(), src.data(), size);
		benchmark::DoNotOptimize(dst.data());
	}
	state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_Memory_DmaTransfer)->Range(256, 1 << 16);  // 256 bytes to 64KB

// Benchmark OAM-like sprite memory access (scattered reads)
static void BM_Memory_OamAccessPattern(benchmark::State& state) {
	// OAM is 256 bytes, accessed as 64 4-byte entries
	std::array<uint8_t, 256> oam{};
	for (size_t i = 0; i < oam.size(); i++) {
		oam[i] = static_cast<uint8_t>(i);
	}
	
	for (auto _ : state) {
		uint32_t visibleCount = 0;
		// Check all 64 sprites for visibility
		for (int sprite = 0; sprite < 64; sprite++) {
			uint8_t y = oam[sprite * 4 + 0];
			uint8_t tile = oam[sprite * 4 + 1];
			uint8_t attr = oam[sprite * 4 + 2];
			uint8_t x = oam[sprite * 4 + 3];
			
			// Simple visibility check
			if (y < 240) {
				visibleCount++;
				benchmark::DoNotOptimize(tile);
				benchmark::DoNotOptimize(attr);
				benchmark::DoNotOptimize(x);
			}
		}
		benchmark::DoNotOptimize(visibleCount);
	}
	state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_Memory_OamAccessPattern);

