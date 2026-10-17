#include "pch.h"
#include <array>
#include <cstring>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx Memory System Benchmarks
// =============================================================================
// The memory manager dispatches every CPU read/write through the MAPCTL overlay
// system. Address classification is the #1 hot path after timer ticking.
// Cart read with auto-increment is the primary data access pattern.

// -----------------------------------------------------------------------------
// Address Space Classification (MAPCTL)
// -----------------------------------------------------------------------------

// Benchmark address classification with all overlays enabled
static void BM_LynxMem_AddressClassify_AllVisible(benchmark::State& state) {
	LynxMemoryManagerState memState = {};
	memState.SuzySpaceVisible = true;
	memState.MikeySpaceVisible = true;
	memState.RomSpaceVisible = true;
	memState.VectorSpaceVisible = true;

	uint16_t addr = 0;

	for (auto _ : state) {
		bool isSuzy = memState.SuzySpaceVisible &&
			addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd;
		bool isMikey = memState.MikeySpaceVisible &&
			addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd;
		bool isRom = memState.RomSpaceVisible &&
			addr >= LynxConstants::BootRomBase && addr < 0xFFFA;
		bool isVector = memState.VectorSpaceVisible && addr >= 0xFFFA;

		benchmark::DoNotOptimize(isSuzy);
		benchmark::DoNotOptimize(isMikey);
		benchmark::DoNotOptimize(isRom);
		benchmark::DoNotOptimize(isVector);
		addr += 7; // stride through address space
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_AddressClassify_AllVisible);

// Benchmark address classification with no overlays (pure RAM)
static void BM_LynxMem_AddressClassify_NoneVisible(benchmark::State& state) {
	LynxMemoryManagerState memState = {};
	memState.SuzySpaceVisible = false;
	memState.MikeySpaceVisible = false;
	memState.RomSpaceVisible = false;
	memState.VectorSpaceVisible = false;

	uint16_t addr = 0;

	for (auto _ : state) {
		bool isSuzy = memState.SuzySpaceVisible &&
			addr >= LynxConstants::SuzyBase && addr <= LynxConstants::SuzyEnd;
		bool isMikey = memState.MikeySpaceVisible &&
			addr >= LynxConstants::MikeyBase && addr <= LynxConstants::MikeyEnd;
		bool isRom = memState.RomSpaceVisible &&
			addr >= LynxConstants::BootRomBase && addr < 0xFFFA;
		bool isVector = memState.VectorSpaceVisible && addr >= 0xFFFA;

		benchmark::DoNotOptimize(isSuzy);
		benchmark::DoNotOptimize(isMikey);
		benchmark::DoNotOptimize(isRom);
		benchmark::DoNotOptimize(isVector);
		addr += 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_AddressClassify_NoneVisible);

// Benchmark optimized address classification using lookup table
static void BM_LynxMem_AddressClassify_PageTable(benchmark::State& state) {
	// Pre-build page table (256 entries, one per page)
	enum class PageType : uint8_t { Ram, Suzy, Mikey, Rom, Vector };
	std::array<PageType, 256> pageTable{};
	pageTable.fill(PageType::Ram);
	pageTable[0xFC] = PageType::Suzy;
	pageTable[0xFD] = PageType::Mikey;
	pageTable[0xFE] = PageType::Rom;
	pageTable[0xFF] = PageType::Rom; // simplified

	uint16_t addr = 0;

	for (auto _ : state) {
		PageType type = pageTable[addr >> 8];
		benchmark::DoNotOptimize(type);
		addr += 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_AddressClassify_PageTable);

// -----------------------------------------------------------------------------
// RAM Read/Write
// -----------------------------------------------------------------------------

// Benchmark sequential RAM reads (typical DMA pattern)
static void BM_LynxMem_RamRead_Sequential(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::WorkRamSize> ram{};
	// Fill with pattern
	for (size_t i = 0; i < ram.size(); i++) {
		ram[i] = static_cast<uint8_t>(i * 3 + 0x42);
	}
	uint16_t addr = 0;

	for (auto _ : state) {
		uint8_t value = ram[addr];
		benchmark::DoNotOptimize(value);
		addr++;
	}
	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_RamRead_Sequential);

// Benchmark random RAM reads (game logic pattern)
static void BM_LynxMem_RamRead_Random(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::WorkRamSize> ram{};
	for (size_t i = 0; i < ram.size(); i++) {
		ram[i] = static_cast<uint8_t>(i * 7);
	}
	uint16_t addr = 0x1234;

	for (auto _ : state) {
		uint8_t value = ram[addr];
		// Pseudo-random walk through memory
		addr = (addr * 2053 + 13849) & 0xFFFF;
		benchmark::DoNotOptimize(value);
	}
	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_RamRead_Random);

// Benchmark 16-bit little-endian read from RAM (pointer loads)
static void BM_LynxMem_RamRead16_LittleEndian(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::WorkRamSize> ram{};
	for (size_t i = 0; i < ram.size() - 1; i += 2) {
		ram[i] = static_cast<uint8_t>(i & 0xFF);
		ram[i + 1] = static_cast<uint8_t>((i >> 8) & 0xFF);
	}
	uint16_t addr = 0;

	for (auto _ : state) {
		uint16_t value = ram[addr] | (ram[addr + 1] << 8);
		benchmark::DoNotOptimize(value);
		addr = (addr + 2) & 0xFFFE;
	}
	state.SetBytesProcessed(state.iterations() * 2);
}
BENCHMARK(BM_LynxMem_RamRead16_LittleEndian);

// Benchmark RAM write (sequential, DMA output pattern)
static void BM_LynxMem_RamWrite_Sequential(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::WorkRamSize> ram{};
	uint16_t addr = 0;
	uint8_t value = 0;

	for (auto _ : state) {
		ram[addr] = value;
		benchmark::DoNotOptimize(ram[addr]);
		addr++;
		value++;
	}
	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_RamWrite_Sequential);

// -----------------------------------------------------------------------------
// MAPCTL Register
// -----------------------------------------------------------------------------

// Benchmark MAPCTL update (decoding the 4 overlay bits)
static void BM_LynxMem_MapctlUpdate(benchmark::State& state) {
	LynxMemoryManagerState memState = {};
	uint8_t mapctl = 0;

	for (auto _ : state) {
		memState.Mapctl = mapctl;
		memState.SuzySpaceVisible = !(mapctl & 0x01);
		memState.MikeySpaceVisible = !(mapctl & 0x02);
		memState.VectorSpaceVisible = !(mapctl & 0x04);
		memState.RomSpaceVisible = !(mapctl & 0x08);
		benchmark::DoNotOptimize(memState);
		mapctl++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMem_MapctlUpdate);

// -----------------------------------------------------------------------------
// Cart Read with Auto-Increment
// -----------------------------------------------------------------------------

// Benchmark cart data read with address counter increment
static void BM_LynxCart_ReadAutoIncrement(benchmark::State& state) {
	constexpr uint32_t romSize = 256 * 1024; // 256 KB cart
	std::array<uint8_t, romSize> romData{};
	for (size_t i = 0; i < romSize; i++) {
		romData[i] = static_cast<uint8_t>(i * 3);
	}

	LynxCartState cart = {};
	cart.AddressCounter = 0;

	for (auto _ : state) {
		uint32_t addr = cart.AddressCounter % romSize;
		uint8_t data = romData[addr];
		cart.AddressCounter++;
		benchmark::DoNotOptimize(data);
	}
	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ReadAutoIncrement);

// Benchmark cart read with bank/page wrapping
static void BM_LynxCart_ReadBankWrapping(benchmark::State& state) {
	constexpr uint32_t pageSize = 256;
	constexpr uint32_t bankSize = pageSize * 256; // 64 KB bank
	constexpr uint32_t romSize = bankSize * 4;    // 256 KB
	std::array<uint8_t, romSize> romData{};
	for (size_t i = 0; i < romSize; i++) {
		romData[i] = static_cast<uint8_t>(i);
	}

	LynxCartState cart = {};
	cart.CurrentBank = 0;
	cart.AddressCounter = 0;

	for (auto _ : state) {
		uint32_t bankOffset = cart.CurrentBank * bankSize;
		uint32_t addr = (bankOffset + cart.AddressCounter) % romSize;
		uint8_t data = romData[addr];
		cart.AddressCounter++;
		if (cart.AddressCounter >= bankSize) {
			cart.AddressCounter = 0;
			cart.CurrentBank = (cart.CurrentBank + 1) % 4;
		}
		benchmark::DoNotOptimize(data);
	}
	state.SetBytesProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ReadBankWrapping);

// Benchmark cart address computation (modulo is expensive)
static void BM_LynxCart_AddressModulo(benchmark::State& state) {
	constexpr uint32_t romSize = 256 * 1024;
	uint32_t addr = 0;

	for (auto _ : state) {
		uint32_t effective = addr % romSize;
		benchmark::DoNotOptimize(effective);
		addr += 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_AddressModulo);

// Benchmark cart address computation (power-of-2 mask, optimization)
static void BM_LynxCart_AddressMask(benchmark::State& state) {
	constexpr uint32_t romSize = 256 * 1024;
	constexpr uint32_t romMask = romSize - 1;
	uint32_t addr = 0;

	for (auto _ : state) {
		uint32_t effective = addr & romMask;
		benchmark::DoNotOptimize(effective);
		addr += 7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_AddressMask);
