#include "pch.h"
#include <array>
#include <random>

// =============================================================================
// Virtual Dispatch Overhead Benchmarks
// =============================================================================
// These benchmarks measure the overhead of virtual function calls in the
// memory handler dispatch path, which is the hottest path in emulation.
//
// Context: Both SNES and NES route every CPU memory read/write through a
// virtual function call on IMemoryHandler or INesMemoryHandler. These calls
// happen millions of times per second. This benchmark suite measures the
// actual cost of virtual dispatch vs. direct calls to quantify the overhead.
//
// Related: GitHub issue #526 - SNES/NES memory handler devirtualization

// -----------------------------------------------------------------------------
// Simulated handler interfaces (standalone, no emulator dependencies)
// -----------------------------------------------------------------------------

// Simulates IMemoryHandler (SNES) with the same virtual dispatch pattern
class IMockHandler {
public:
	virtual ~IMockHandler() = default;
	virtual uint8_t Read(uint32_t addr) = 0;
	virtual void Write(uint32_t addr, uint8_t value) = 0;
};

// Simulates RamHandler (SNES) - trivial read/write
class MockRamHandler final : public IMockHandler {
	uint8_t* _ram;
	uint32_t _mask;
public:
	MockRamHandler(uint8_t* ram, uint32_t mask) : _ram(ram), _mask(mask) {}
	uint8_t Read(uint32_t addr) override { return _ram[addr & _mask]; }
	void Write(uint32_t addr, uint8_t value) override { _ram[addr & _mask] = value; }
};

// Simulates RomHandler (SNES) - read-only
class MockRomHandler final : public IMockHandler {
	const uint8_t* _rom;
	uint32_t _mask;
public:
	MockRomHandler(const uint8_t* rom, uint32_t mask) : _rom(rom), _mask(mask) {}
	uint8_t Read(uint32_t addr) override { return _rom[addr & _mask]; }
	void Write([[maybe_unused]] uint32_t addr, [[maybe_unused]] uint8_t value) override { /* ROM: no-op */ }
};

// Simulates RegisterHandler (SNES) - more complex read/write
class MockRegisterHandler final : public IMockHandler {
	std::array<uint8_t, 256> _regs{};
public:
	uint8_t Read(uint32_t addr) override {
		uint8_t reg = static_cast<uint8_t>(addr);
		// Simulate side-effect register read
		uint8_t val = _regs[reg];
		_regs[reg] |= 0x80;  // Set "read" flag
		return val;
	}
	void Write(uint32_t addr, uint8_t value) override {
		_regs[static_cast<uint8_t>(addr)] = value;
	}
};

// Handler type tag for devirtualized fast-path simulation
enum class HandlerType : uint8_t {
	Ram = 0,
	Rom = 1,
	Register = 2,
	Other = 3
};

// Tagged handler wrapper for type-tagged fast-path benchmarks
struct TaggedHandler {
	IMockHandler* handler;
	HandlerType type;
};

// =============================================================================
// SNES-style dispatch benchmarks (4096-entry handler table)
// =============================================================================

// Baseline: Virtual dispatch through handler table (current SNES behavior)
static void BM_VirtualDispatch_SnesStyleRead(benchmark::State& state) {
	constexpr size_t kTableSize = 4096;
	constexpr size_t kRamSize = 0x20000;  // 128KB WRAM
	constexpr size_t kRomSize = 0x400000; // 4MB ROM

	std::vector<uint8_t> ram(kRamSize, 0x42);
	std::vector<uint8_t> rom(kRomSize, 0xAB);

	// Create handlers
	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);

	// Build handler table mimicking SNES memory map
	// Banks 00-3F: lower half ROM, upper half RAM/IO
	// Banks 80-BF: mirror of 00-3F
	// Banks 40-7D/C0-FF: ROM
	std::array<IMockHandler*, kTableSize> handlers{};
	for (size_t i = 0; i < kTableSize; i++) {
		uint8_t bank = static_cast<uint8_t>(i >> 4);
		uint8_t page = static_cast<uint8_t>(i & 0xF);

		if (bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF)) {
			// Lower banks: pages 0-7 = RAM/IO, pages 8-F = ROM
			handlers[i] = (page < 8) ? static_cast<IMockHandler*>(&ramHandler)
			                         : static_cast<IMockHandler*>(&romHandler);
		} else {
			// Upper banks: all ROM
			handlers[i] = &romHandler;
		}
	}

	// Pre-generate realistic address pattern: mostly ROM reads with some RAM
	constexpr size_t kAccessCount = 4096;
	std::array<uint32_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	// 80% ROM reads (bank 80+, page 8+), 20% RAM reads (bank 00-3F, page 0-7)
	std::uniform_int_distribution<uint32_t> romDist(0x808000, 0xBFFFFF);
	std::uniform_int_distribution<uint32_t> ramDist(0x000000, 0x3F7FFF);
	std::bernoulli_distribution isRom(0.80);
	for (auto& addr : addresses) {
		addr = isRom(gen) ? romDist(gen) : ramDist(gen);
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (auto addr : addresses) {
			IMockHandler* handler = handlers[addr >> 12];
			sum += handler->Read(addr);  // Virtual call
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_VirtualDispatch_SnesStyleRead);

// Direct call: same logic but call RamHandler/RomHandler directly (no virtual)
static void BM_DirectCall_SnesStyleRead(benchmark::State& state) {
	constexpr size_t kTableSize = 4096;
	constexpr size_t kRamSize = 0x20000;
	constexpr size_t kRomSize = 0x400000;

	std::vector<uint8_t> ram(kRamSize, 0x42);
	std::vector<uint8_t> rom(kRomSize, 0xAB);

	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);

	// Build tagged handler table
	std::array<TaggedHandler, kTableSize> handlers{};
	for (size_t i = 0; i < kTableSize; i++) {
		uint8_t bank = static_cast<uint8_t>(i >> 4);
		uint8_t page = static_cast<uint8_t>(i & 0xF);

		if (bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF)) {
			if (page < 8) {
				handlers[i] = {&ramHandler, HandlerType::Ram};
			} else {
				handlers[i] = {&romHandler, HandlerType::Rom};
			}
		} else {
			handlers[i] = {&romHandler, HandlerType::Rom};
		}
	}

	constexpr size_t kAccessCount = 4096;
	std::array<uint32_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint32_t> romDist(0x808000, 0xBFFFFF);
	std::uniform_int_distribution<uint32_t> ramDist(0x000000, 0x3F7FFF);
	std::bernoulli_distribution isRom(0.80);
	for (auto& addr : addresses) {
		addr = isRom(gen) ? romDist(gen) : ramDist(gen);
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (auto addr : addresses) {
			auto& tagged = handlers[addr >> 12];
			// Type-tagged fast path: no virtual call for common types
			switch (tagged.type) {
				case HandlerType::Ram:
					sum += static_cast<MockRamHandler*>(tagged.handler)->MockRamHandler::Read(addr);
					break;
				case HandlerType::Rom:
					sum += static_cast<MockRomHandler*>(tagged.handler)->MockRomHandler::Read(addr);
					break;
				default:
					sum += tagged.handler->Read(addr);  // Fallback to virtual
					break;
			}
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_DirectCall_SnesStyleRead);

// =============================================================================
// NES-style dispatch benchmarks (65536-entry handler table)
// =============================================================================

// Simulates INesMemoryHandler (NES) for per-address dispatch
class IMockNesHandler {
public:
	virtual ~IMockNesHandler() = default;
	virtual uint8_t ReadRam(uint16_t addr) = 0;
	virtual void WriteRam(uint16_t addr, uint8_t value) = 0;
};

class MockNesRamHandler final : public IMockNesHandler {
	uint8_t* _ram;
	uint16_t _mask;
public:
	MockNesRamHandler(uint8_t* ram, uint16_t mask) : _ram(ram), _mask(mask) {}
	uint8_t ReadRam(uint16_t addr) override { return _ram[addr & _mask]; }
	void WriteRam(uint16_t addr, uint8_t value) override { _ram[addr & _mask] = value; }
};

class MockNesMapperHandler final : public IMockNesHandler {
	// Simulates BaseMapper page-table lookup
	std::array<const uint8_t*, 256> _pages{};
	const uint8_t* _rom;
public:
	MockNesMapperHandler(const uint8_t* rom, size_t romSize) : _rom(rom) {
		// Set up simple page table (8KB banks mirrored)
		for (size_t i = 0; i < 256; i++) {
			_pages[i] = rom + ((i * 256) % romSize);
		}
	}
	uint8_t ReadRam(uint16_t addr) override {
		return _pages[addr >> 8][static_cast<uint8_t>(addr)];
	}
	void WriteRam(uint16_t /*addr*/, uint8_t /*value*/) override {
		// Mapper register write (side effects)
	}
};

class MockNesOpenBusHandler final : public IMockNesHandler {
	uint8_t _openBus = 0;
public:
	uint8_t ReadRam(uint16_t /*addr*/) override { return _openBus; }
	void WriteRam(uint16_t /*addr*/, uint8_t value) override { _openBus = value; }
};

// Baseline: Virtual dispatch through 64K handler table (current NES behavior)
static void BM_VirtualDispatch_NesStyleRead(benchmark::State& state) {
	constexpr size_t kRamSize = 2048;   // 2KB internal RAM
	constexpr size_t kRomSize = 0x8000; // 32KB PRG-ROM

	std::vector<uint8_t> ram(kRamSize, 0x42);
	std::vector<uint8_t> rom(kRomSize, 0xAB);

	MockNesRamHandler ramHandler(ram.data(), 0x07FF);
	MockNesMapperHandler mapperHandler(rom.data(), kRomSize);
	MockNesOpenBusHandler openBusHandler;

	// Build 64K handler table mimicking NES memory map
	std::array<IMockNesHandler*, 65536> handlers{};
	for (uint32_t addr = 0; addr < 65536; addr++) {
		if (addr < 0x2000) {
			handlers[addr] = &ramHandler;        // $0000-$1FFF: RAM (mirrored)
		} else if (addr < 0x4020) {
			handlers[addr] = &openBusHandler;    // $2000-$401F: I/O (simplified)
		} else if (addr >= 0x8000) {
			handlers[addr] = &mapperHandler;     // $8000-$FFFF: PRG-ROM
		} else {
			handlers[addr] = &openBusHandler;    // $4020-$7FFF: expansion
		}
	}

	// Pre-generate realistic NES access pattern
	// ~60% PRG-ROM reads (instruction fetch), ~25% RAM, ~15% I/O
	constexpr size_t kAccessCount = 4096;
	std::array<uint16_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint16_t> romDist(0x8000, 0xFFFF);
	std::uniform_int_distribution<uint16_t> ramDist(0x0000, 0x1FFF);
	std::uniform_int_distribution<uint16_t> ioDist(0x2000, 0x401F);
	std::uniform_int_distribution<int> typeDist(0, 99);
	for (auto& addr : addresses) {
		int r = typeDist(gen);
		if (r < 60) addr = romDist(gen);
		else if (r < 85) addr = ramDist(gen);
		else addr = ioDist(gen);
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (auto addr : addresses) {
			sum += handlers[addr]->ReadRam(addr);  // Virtual call
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_VirtualDispatch_NesStyleRead);

// Direct call with mapper fast-path check (proposed NES optimization)
static void BM_DirectCall_NesStyleRead(benchmark::State& state) {
	constexpr size_t kRamSize = 2048;
	constexpr size_t kRomSize = 0x8000;

	std::vector<uint8_t> ram(kRamSize, 0x42);
	std::vector<uint8_t> rom(kRomSize, 0xAB);

	MockNesRamHandler ramHandler(ram.data(), 0x07FF);
	MockNesMapperHandler mapperHandler(rom.data(), kRomSize);
	MockNesOpenBusHandler openBusHandler;

	std::array<IMockNesHandler*, 65536> handlers{};
	for (uint32_t addr = 0; addr < 65536; addr++) {
		if (addr < 0x2000) {
			handlers[addr] = &ramHandler;
		} else if (addr < 0x4020) {
			handlers[addr] = &openBusHandler;
		} else if (addr >= 0x8000) {
			handlers[addr] = &mapperHandler;
		} else {
			handlers[addr] = &openBusHandler;
		}
	}

	constexpr size_t kAccessCount = 4096;
	std::array<uint16_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint16_t> romDist(0x8000, 0xFFFF);
	std::uniform_int_distribution<uint16_t> ramDist(0x0000, 0x1FFF);
	std::uniform_int_distribution<uint16_t> ioDist(0x2000, 0x401F);
	std::uniform_int_distribution<int> typeDist(0, 99);
	for (auto& addr : addresses) {
		int r = typeDist(gen);
		if (r < 60) addr = romDist(gen);
		else if (r < 85) addr = ramDist(gen);
		else addr = ioDist(gen);
	}

	// Cache the mapper pointer for fast-path comparison
	IMockNesHandler* mapper = &mapperHandler;
	IMockNesHandler* ramh = &ramHandler;

	for (auto _ : state) {
		uint32_t sum = 0;
		for (auto addr : addresses) {
			IMockNesHandler* h = handlers[addr];
			// Fast-path: check if it's the mapper (most common)
			if (h == mapper) [[likely]] {
				sum += static_cast<MockNesMapperHandler*>(h)->MockNesMapperHandler::ReadRam(addr);
			} else if (h == ramh) {
				sum += static_cast<MockNesRamHandler*>(h)->MockNesRamHandler::ReadRam(addr);
			} else {
				sum += h->ReadRam(addr);  // Virtual fallback
			}
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_DirectCall_NesStyleRead);

// =============================================================================
// Micro-benchmarks: pure virtual call overhead measurement
// =============================================================================

// Warm branch predictor: single handler type (best case for virtual dispatch)
static void BM_VirtualDispatch_SingleType(benchmark::State& state) {
	std::vector<uint8_t> ram(4096, 0x42);
	MockRamHandler handler(ram.data(), 0xFFF);
	IMockHandler* p = &handler;

	for (auto _ : state) {
		uint32_t sum = 0;
		for (int i = 0; i < 1024; i++) {
			sum += p->Read(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_VirtualDispatch_SingleType);

// Direct call baseline for comparison
static void BM_DirectCall_SingleType(benchmark::State& state) {
	std::vector<uint8_t> ram(4096, 0x42);
	MockRamHandler handler(ram.data(), 0xFFF);

	for (auto _ : state) {
		uint32_t sum = 0;
		for (int i = 0; i < 1024; i++) {
			sum += handler.Read(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_DirectCall_SingleType);

// Cold branch predictor: alternating handler types (worst case)
static void BM_VirtualDispatch_AlternatingTypes(benchmark::State& state) {
	std::vector<uint8_t> ram(4096, 0x42);
	std::vector<uint8_t> rom(4096, 0xAB);

	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);

	// Alternate between two types to defeat branch prediction
	std::array<IMockHandler*, 1024> handlers{};
	for (size_t i = 0; i < 1024; i++) {
		handlers[i] = (i & 1) ? static_cast<IMockHandler*>(&romHandler)
		                      : static_cast<IMockHandler*>(&ramHandler);
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (size_t i = 0; i < 1024; i++) {
			sum += handlers[i]->Read(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_VirtualDispatch_AlternatingTypes);

// Three-type mix: RAM, ROM, Register (realistic SNES pattern)
static void BM_VirtualDispatch_ThreeTypes(benchmark::State& state) {
	std::vector<uint8_t> ram(4096, 0x42);
	std::vector<uint8_t> rom(4096, 0xAB);

	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);
	MockRegisterHandler regHandler;

	// 70% ROM, 20% RAM, 10% Register (realistic SNES distribution)
	std::array<IMockHandler*, 1024> handlers{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<int> dist(0, 99);
	for (size_t i = 0; i < 1024; i++) {
		int r = dist(gen);
		if (r < 70) handlers[i] = &romHandler;
		else if (r < 90) handlers[i] = &ramHandler;
		else handlers[i] = &regHandler;
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (size_t i = 0; i < 1024; i++) {
			sum += handlers[i]->Read(static_cast<uint32_t>(i));
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_VirtualDispatch_ThreeTypes);

// =============================================================================
// Write path benchmarks
// =============================================================================

// Virtual dispatch write (SNES-style)
static void BM_VirtualDispatch_SnesStyleWrite(benchmark::State& state) {
	constexpr size_t kTableSize = 4096;
	constexpr size_t kRamSize = 0x20000;
	constexpr size_t kRomSize = 0x400000;

	std::vector<uint8_t> ram(kRamSize, 0);
	std::vector<uint8_t> rom(kRomSize, 0);

	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);

	std::array<IMockHandler*, kTableSize> handlers{};
	for (size_t i = 0; i < kTableSize; i++) {
		uint8_t bank = static_cast<uint8_t>(i >> 4);
		uint8_t page = static_cast<uint8_t>(i & 0xF);
		if (bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF)) {
			handlers[i] = (page < 8) ? static_cast<IMockHandler*>(&ramHandler)
			                         : static_cast<IMockHandler*>(&romHandler);
		} else {
			handlers[i] = &romHandler;
		}
	}

	// Writes go mostly to RAM (writes to ROM are no-ops)
	constexpr size_t kAccessCount = 4096;
	std::array<uint32_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint32_t> ramDist(0x000000, 0x3F7FFF);
	std::uniform_int_distribution<uint32_t> romDist(0x808000, 0xBFFFFF);
	std::bernoulli_distribution isRam(0.70);  // 70% RAM writes
	for (auto& addr : addresses) {
		addr = isRam(gen) ? ramDist(gen) : romDist(gen);
	}

	for (auto _ : state) {
		for (auto addr : addresses) {
			IMockHandler* handler = handlers[addr >> 12];
			handler->Write(addr, 0x42);  // Virtual call
		}
		benchmark::DoNotOptimize(ram.data());
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_VirtualDispatch_SnesStyleWrite);

// Direct call write (SNES-style with type tag)
static void BM_DirectCall_SnesStyleWrite(benchmark::State& state) {
	constexpr size_t kTableSize = 4096;
	constexpr size_t kRamSize = 0x20000;
	constexpr size_t kRomSize = 0x400000;

	std::vector<uint8_t> ram(kRamSize, 0);
	std::vector<uint8_t> rom(kRomSize, 0);

	MockRamHandler ramHandler(ram.data(), 0xFFF);
	MockRomHandler romHandler(rom.data(), 0xFFF);

	std::array<TaggedHandler, kTableSize> handlers{};
	for (size_t i = 0; i < kTableSize; i++) {
		uint8_t bank = static_cast<uint8_t>(i >> 4);
		uint8_t page = static_cast<uint8_t>(i & 0xF);
		if (bank <= 0x3F || (bank >= 0x80 && bank <= 0xBF)) {
			if (page < 8) {
				handlers[i] = {&ramHandler, HandlerType::Ram};
			} else {
				handlers[i] = {&romHandler, HandlerType::Rom};
			}
		} else {
			handlers[i] = {&romHandler, HandlerType::Rom};
		}
	}

	constexpr size_t kAccessCount = 4096;
	std::array<uint32_t, kAccessCount> addresses{};
	std::mt19937 gen(42);
	std::uniform_int_distribution<uint32_t> ramDist(0x000000, 0x3F7FFF);
	std::uniform_int_distribution<uint32_t> romDist(0x808000, 0xBFFFFF);
	std::bernoulli_distribution isRam(0.70);
	for (auto& addr : addresses) {
		addr = isRam(gen) ? ramDist(gen) : romDist(gen);
	}

	for (auto _ : state) {
		for (auto addr : addresses) {
			auto& tagged = handlers[addr >> 12];
			switch (tagged.type) {
				case HandlerType::Ram:
					static_cast<MockRamHandler*>(tagged.handler)->MockRamHandler::Write(addr, 0x42);
					break;
				case HandlerType::Rom:
					// ROM write is no-op, skip entirely
					break;
				default:
					tagged.handler->Write(addr, 0x42);
					break;
			}
		}
		benchmark::DoNotOptimize(ram.data());
	}
	state.SetItemsProcessed(state.iterations() * kAccessCount);
}
BENCHMARK(BM_DirectCall_SnesStyleWrite);
