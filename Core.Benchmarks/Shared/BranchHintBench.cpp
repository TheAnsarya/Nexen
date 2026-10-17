#include "pch.h"
#include <array>
#include <cstdint>

// =============================================================================
// Branch Hint Benchmarks — [[likely]]/[[unlikely]] attribute effects
// =============================================================================
// Tests the performance impact of branch prediction hints on hot-path patterns
// found in CPU emulation cores and memory managers.
//
// Each pattern is tested both WITHOUT (Old) and WITH (New) branch hints.
// The "Old" functions use plain if/else; "New" add [[likely]]/[[unlikely]].
// =============================================================================

// ---------------------------------------------------------------------------
// 1. ProcessPendingDma pattern: early-return guard (NES CPU)
//    Called on EVERY NES memory read. _needHalt is almost always false,
//    so the early return is the hot path.
// ---------------------------------------------------------------------------

static volatile bool g_needHalt = false;  // Almost always false in practice

__declspec(noinline)
static void ProcessPendingDma_Old(bool needHalt, uint16_t addr, int& counter) {
	if (!needHalt) {
		return;
	}
	// Cold path — simulate DMA work
	counter += static_cast<int>(addr);
}

__declspec(noinline)
static void ProcessPendingDma_New(bool needHalt, uint16_t addr, int& counter) {
	if (!needHalt) [[likely]] {
		return;
	}
	// Cold path — simulate DMA work
	counter += static_cast<int>(addr);
}

static void BM_ProcessPendingDma_Old(benchmark::State& state) {
	int counter = 0;
	bool halt = g_needHalt;  // false
	for (auto _ : state) {
		ProcessPendingDma_Old(halt, 0x8000, counter);
		benchmark::DoNotOptimize(counter);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProcessPendingDma_Old);

static void BM_ProcessPendingDma_New(benchmark::State& state) {
	int counter = 0;
	bool halt = g_needHalt;  // false
	for (auto _ : state) {
		ProcessPendingDma_New(halt, 0x8000, counter);
		benchmark::DoNotOptimize(counter);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ProcessPendingDma_New);

// ---------------------------------------------------------------------------
// 2. CheckForInterrupts pattern: two rare branches (SNES CPU)
//    Called every SNES CPU instruction. NMI and IRQ are very rare.
// ---------------------------------------------------------------------------

struct FakeInterruptState {
	bool NeedNmi = false;
	uint8_t PrevIrqSource = 0;
	int nmiCount = 0;
	int irqCount = 0;
};

__declspec(noinline)
static void CheckForInterrupts_Old(FakeInterruptState& s) {
	if (s.NeedNmi) {
		s.NeedNmi = false;
		s.nmiCount++;
	} else if (s.PrevIrqSource) {
		s.irqCount++;
	}
}

__declspec(noinline)
static void CheckForInterrupts_New(FakeInterruptState& s) {
	if (s.NeedNmi) [[unlikely]] {
		s.NeedNmi = false;
		s.nmiCount++;
	} else if (s.PrevIrqSource) [[unlikely]] {
		s.irqCount++;
	}
}

static void BM_CheckForInterrupts_Old(benchmark::State& state) {
	FakeInterruptState s;
	for (auto _ : state) {
		CheckForInterrupts_Old(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CheckForInterrupts_Old);

static void BM_CheckForInterrupts_New(benchmark::State& state) {
	FakeInterruptState s;
	for (auto _ : state) {
		CheckForInterrupts_New(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CheckForInterrupts_New);

// ---------------------------------------------------------------------------
// 3. Memory handler null-check pattern (SNES MemoryManager)
//    Handler is almost always non-null; null = unmapped/open bus.
// ---------------------------------------------------------------------------

struct FakeHandler {
	uint8_t value = 0x42;
};

__declspec(noinline)
static uint8_t MemRead_Old(FakeHandler* handler, uint8_t openBus) {
	if (handler) {
		return handler->value;
	} else {
		return openBus;
	}
}

__declspec(noinline)
static uint8_t MemRead_New(FakeHandler* handler, uint8_t openBus) {
	if (handler) [[likely]] {
		return handler->value;
	} else {
		return openBus;
	}
}

static void BM_MemHandlerCheck_Old(benchmark::State& state) {
	FakeHandler handler;
	FakeHandler* ptr = &handler;  // Non-null (common case)
	for (auto _ : state) {
		auto v = MemRead_Old(ptr, 0xFF);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemHandlerCheck_Old);

static void BM_MemHandlerCheck_New(benchmark::State& state) {
	FakeHandler handler;
	FakeHandler* ptr = &handler;  // Non-null (common case)
	for (auto _ : state) {
		auto v = MemRead_New(ptr, 0xFF);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MemHandlerCheck_New);

// ---------------------------------------------------------------------------
// 4. GBA ldmGlitch pattern (GbaCpu Read/Write/Idle)
//    _ldmGlitch is almost always 0; it's a hardware bug workaround.
// ---------------------------------------------------------------------------

__declspec(noinline)
static uint32_t GbaRead_Old(uint8_t ldmGlitch, uint32_t memValue) {
	if (ldmGlitch) {
		ldmGlitch--;
	}
	return memValue;
}

__declspec(noinline)
static uint32_t GbaRead_New(uint8_t ldmGlitch, uint32_t memValue) {
	if (ldmGlitch) [[unlikely]] {
		ldmGlitch--;
	}
	return memValue;
}

static void BM_GbaLdmGlitch_Old(benchmark::State& state) {
	uint8_t glitch = 0;
	for (auto _ : state) {
		auto v = GbaRead_Old(glitch, 0x12345678);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaLdmGlitch_Old);

static void BM_GbaLdmGlitch_New(benchmark::State& state) {
	uint8_t glitch = 0;
	for (auto _ : state) {
		auto v = GbaRead_New(glitch, 0x12345678);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaLdmGlitch_New);

// ---------------------------------------------------------------------------
// 5. HaltCounter pattern (GB CPU Exec)
//    CPU is usually running (HaltCounter == 0); halt is rare.
// ---------------------------------------------------------------------------

__declspec(noinline)
static void GbExec_Old(uint32_t haltCounter, int& instrCount, int& haltCount) {
	if (haltCounter) {
		haltCount++;
	} else {
		instrCount++;
	}
}

__declspec(noinline)
static void GbExec_New(uint32_t haltCounter, int& instrCount, int& haltCount) {
	if (haltCounter) [[unlikely]] {
		haltCount++;
	} else {
		instrCount++;
	}
}

static void BM_GbHaltCheck_Old(benchmark::State& state) {
	int instr = 0, halt = 0;
	for (auto _ : state) {
		GbExec_Old(0, instr, halt);  // Common case: not halted
		benchmark::DoNotOptimize(instr);
		benchmark::DoNotOptimize(halt);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbHaltCheck_Old);

static void BM_GbHaltCheck_New(benchmark::State& state) {
	int instr = 0, halt = 0;
	for (auto _ : state) {
		GbExec_New(0, instr, halt);  // Common case: not halted
		benchmark::DoNotOptimize(instr);
		benchmark::DoNotOptimize(halt);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbHaltCheck_New);

// ---------------------------------------------------------------------------
// 6. PPU cycle range pattern (NES PPU ProcessScanlineImpl)
//    cycle <= 256 is the main rendering path (cycles 1-256 of 341 total).
//    ~75% of calls take this branch.
// ---------------------------------------------------------------------------

__declspec(noinline)
static void PpuCycleDispatch_Old(uint16_t cycle, int& renderCount, int& spriteCount, int& tileCount) {
	if (cycle <= 256) {
		renderCount++;
	} else if (cycle >= 257 && cycle <= 320) {
		spriteCount++;
	} else if (cycle >= 321 && cycle <= 336) {
		tileCount++;
	}
}

__declspec(noinline)
static void PpuCycleDispatch_New(uint16_t cycle, int& renderCount, int& spriteCount, int& tileCount) {
	if (cycle <= 256) [[likely]] {
		renderCount++;
	} else if (cycle >= 257 && cycle <= 320) {
		spriteCount++;
	} else if (cycle >= 321 && cycle <= 336) {
		tileCount++;
	}
}

static void BM_PpuCycleDispatch_Old(benchmark::State& state) {
	int render = 0, sprite = 0, tile = 0;
	uint16_t cycle = 1;
	for (auto _ : state) {
		PpuCycleDispatch_Old(cycle, render, sprite, tile);
		cycle = (cycle % 341) + 1;  // Cycle through 1-341 like real PPU
		benchmark::DoNotOptimize(render);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PpuCycleDispatch_Old);

static void BM_PpuCycleDispatch_New(benchmark::State& state) {
	int render = 0, sprite = 0, tile = 0;
	uint16_t cycle = 1;
	for (auto _ : state) {
		PpuCycleDispatch_New(cycle, render, sprite, tile);
		cycle = (cycle % 341) + 1;  // Cycle through 1-341 like real PPU
		benchmark::DoNotOptimize(render);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PpuCycleDispatch_New);

// ---------------------------------------------------------------------------
// 7. Rendering-enabled guard pattern (NES PPU LoadTileInfo)
//    Rendering is enabled for the vast majority of visible scanlines.
// ---------------------------------------------------------------------------

__declspec(noinline)
static void LoadTileInfo_Old(bool renderingEnabled, uint16_t cycle, int& tileLoads) {
	if (renderingEnabled) {
		switch (cycle & 0x07) {
			case 1: tileLoads++; break;
			case 3: tileLoads += 2; break;
			case 5: tileLoads += 3; break;
			case 7: tileLoads += 4; break;
		}
	}
}

__declspec(noinline)
static void LoadTileInfo_New(bool renderingEnabled, uint16_t cycle, int& tileLoads) {
	if (renderingEnabled) [[likely]] {
		switch (cycle & 0x07) {
			case 1: tileLoads++; break;
			case 3: tileLoads += 2; break;
			case 5: tileLoads += 3; break;
			case 7: tileLoads += 4; break;
		}
	}
}

static void BM_LoadTileInfo_Old(benchmark::State& state) {
	int tileLoads = 0;
	uint16_t cycle = 1;
	for (auto _ : state) {
		LoadTileInfo_Old(true, cycle, tileLoads);  // Common case: rendering enabled
		cycle = (cycle % 256) + 1;
		benchmark::DoNotOptimize(tileLoads);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LoadTileInfo_Old);

static void BM_LoadTileInfo_New(benchmark::State& state) {
	int tileLoads = 0;
	uint16_t cycle = 1;
	for (auto _ : state) {
		LoadTileInfo_New(true, cycle, tileLoads);  // Common case: rendering enabled
		cycle = (cycle % 256) + 1;
		benchmark::DoNotOptimize(tileLoads);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LoadTileInfo_New);

// ---------------------------------------------------------------------------
// 8. Pipeline reload pattern (GBA CPU Exec)
//    ReloadRequested is only true after branches/jumps (~15-20% of instrs).
// ---------------------------------------------------------------------------

__declspec(noinline)
static void GbaExecPipeline_Old(bool reloadRequested, int& reloadCount, int& normalCount) {
	if (reloadRequested) {
		reloadCount++;
	}
	normalCount++;
}

__declspec(noinline)
static void GbaExecPipeline_New(bool reloadRequested, int& reloadCount, int& normalCount) {
	if (reloadRequested) [[unlikely]] {
		reloadCount++;
	}
	normalCount++;
}

static void BM_GbaPipelineReload_Old(benchmark::State& state) {
	int reload = 0, normal = 0;
	for (auto _ : state) {
		GbaExecPipeline_Old(false, reload, normal);  // Common case: no reload
		benchmark::DoNotOptimize(normal);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaPipelineReload_Old);

static void BM_GbaPipelineReload_New(benchmark::State& state) {
	int reload = 0, normal = 0;
	for (auto _ : state) {
		GbaExecPipeline_New(false, reload, normal);  // Common case: no reload
		benchmark::DoNotOptimize(normal);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaPipelineReload_New);

// ---------------------------------------------------------------------------
// 9. GB register check pattern (GbMemoryManager Read)
//    Most reads go to ROM/RAM, not registers. Register reads are uncommon.
// ---------------------------------------------------------------------------

__declspec(noinline)
static uint8_t GbMemRead_Old(bool isRegister, const uint8_t* readPtr, uint8_t addr) {
	if (isRegister) {
		// Simulate register read (cold path)
		return addr ^ 0x55;
	} else if (readPtr) {
		return readPtr[addr];
	}
	return 0;
}

__declspec(noinline)
static uint8_t GbMemRead_New(bool isRegister, const uint8_t* readPtr, uint8_t addr) {
	if (isRegister) [[unlikely]] {
		// Simulate register read (cold path)
		return addr ^ 0x55;
	} else if (readPtr) [[likely]] {
		return readPtr[addr];
	}
	return 0;
}

static void BM_GbMemRead_Old(benchmark::State& state) {
	std::array<uint8_t, 256> ram{};
	ram.fill(0xAB);
	uint8_t addr = 0;
	for (auto _ : state) {
		auto v = GbMemRead_Old(false, ram.data(), addr);
		benchmark::DoNotOptimize(v);
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbMemRead_Old);

static void BM_GbMemRead_New(benchmark::State& state) {
	std::array<uint8_t, 256> ram{};
	ram.fill(0xAB);
	uint8_t addr = 0;
	for (auto _ : state) {
		auto v = GbMemRead_New(false, ram.data(), addr);
		benchmark::DoNotOptimize(v);
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbMemRead_New);

// ---------------------------------------------------------------------------
// 10. Composite hot-path simulation: NES CPU instruction cycle
//     Simulates the per-instruction overhead of ProcessPendingDma +
//     CheckForInterrupts + memory handler lookup combined.
// ---------------------------------------------------------------------------

__declspec(noinline)
static uint8_t SimulateNesInstruction_Old(
	bool needHalt, bool needNmi, uint8_t irqSource,
	FakeHandler* handler, uint8_t openBus
) {
	// ProcessPendingDma
	if (!needHalt) {
		// fast path
	} else {
		return 0;  // would do DMA
	}

	// CheckForInterrupts
	if (needNmi) {
		return 0xFF;  // NMI
	} else if (irqSource) {
		return 0xFE;  // IRQ
	}

	// Memory read
	if (handler) {
		return handler->value;
	} else {
		return openBus;
	}
}

__declspec(noinline)
static uint8_t SimulateNesInstruction_New(
	bool needHalt, bool needNmi, uint8_t irqSource,
	FakeHandler* handler, uint8_t openBus
) {
	// ProcessPendingDma
	if (!needHalt) [[likely]] {
		// fast path
	} else {
		return 0;  // would do DMA
	}

	// CheckForInterrupts
	if (needNmi) [[unlikely]] {
		return 0xFF;  // NMI
	} else if (irqSource) [[unlikely]] {
		return 0xFE;  // IRQ
	}

	// Memory read
	if (handler) [[likely]] {
		return handler->value;
	} else {
		return openBus;
	}
}

static void BM_NesInstructionComposite_Old(benchmark::State& state) {
	FakeHandler handler;
	for (auto _ : state) {
		auto v = SimulateNesInstruction_Old(false, false, 0, &handler, 0xFF);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesInstructionComposite_Old);

static void BM_NesInstructionComposite_New(benchmark::State& state) {
	FakeHandler handler;
	for (auto _ : state) {
		auto v = SimulateNesInstruction_New(false, false, 0, &handler, 0xFF);
		benchmark::DoNotOptimize(v);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesInstructionComposite_New);
