#include "pch.h"
#include <array>
#include <functional>

// =============================================================================
// SNES Memory Manager Dispatch Benchmarks
// =============================================================================
// Measures the overhead of different dispatch mechanisms for CPU timing calls
// in SnesMemoryManager::Read() and Write(). The function pointer dispatch was
// replaced with an inline switch for better branch prediction and inlining.
//
// Key insight: _cpuSpeed has only 3 values (6, 8, 12) and rarely changes.
// A switch on this value is highly predictable by the branch predictor,
// while a member function pointer requires indirect call prediction.
// =============================================================================

namespace {
	// Simulate the Exec() work (2 master clock ticks)
	struct FakeExecState {
		uint64_t masterClock = 0;
		uint16_t hClock = 0;
		uint8_t cpuSpeed = 8;
	};

	__forceinline void SimExec(FakeExecState& s) {
		s.masterClock += 2;
		s.hClock += 2;
		benchmark::DoNotOptimize(s.masterClock);
	}

	// Templated IncMasterClock (matches real implementation)
	template <uint8_t clocks>
	__forceinline void SimIncMasterClock(FakeExecState& s) {
		if constexpr (clocks == 2) {
			SimExec(s);
		} else if constexpr (clocks == 4) {
			SimExec(s); SimExec(s);
		} else if constexpr (clocks == 6) {
			SimExec(s); SimExec(s); SimExec(s);
		} else if constexpr (clocks == 8) {
			SimExec(s); SimExec(s); SimExec(s); SimExec(s);
		} else if constexpr (clocks == 12) {
			SimExec(s); SimExec(s); SimExec(s);
			SimExec(s); SimExec(s); SimExec(s);
		}
	}

	// Function pointer type (matches original Func typedef)
	using ExecFunc = void(*)(FakeExecState&);

	// Wrapper functions for function pointer dispatch
	void ExecRead_Speed6(FakeExecState& s) { SimIncMasterClock<2>(s); }
	void ExecRead_Speed8(FakeExecState& s) { SimIncMasterClock<4>(s); }
	void ExecRead_Speed12(FakeExecState& s) { SimIncMasterClock<8>(s); }

	void ExecWrite_Speed6(FakeExecState& s) { SimIncMasterClock<6>(s); }
	void ExecWrite_Speed8(FakeExecState& s) { SimIncMasterClock<8>(s); }
	void ExecWrite_Speed12(FakeExecState& s) { SimIncMasterClock<12>(s); }
} // anonymous namespace

// -----------------------------------------------------------------------------
// Function Pointer Dispatch (OLD approach)
// -----------------------------------------------------------------------------
// Simulates the original (this->*_execRead)() call pattern

static void BM_SnesMemMgr_FuncPtrDispatch_Read(benchmark::State& state) {
	FakeExecState s;
	ExecFunc readFn = ExecRead_Speed8;  // Most common speed

	for (auto _ : state) {
		readFn(s);
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_FuncPtrDispatch_Read);

static void BM_SnesMemMgr_FuncPtrDispatch_Write(benchmark::State& state) {
	FakeExecState s;
	ExecFunc writeFn = ExecWrite_Speed8;

	for (auto _ : state) {
		writeFn(s);
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_FuncPtrDispatch_Write);

// Function pointer with varying targets (simulates speed mode changes)
static void BM_SnesMemMgr_FuncPtrDispatch_Mixed(benchmark::State& state) {
	FakeExecState s;
	ExecFunc readFns[] = { ExecRead_Speed6, ExecRead_Speed8, ExecRead_Speed12 };
	size_t idx = 0;

	for (auto _ : state) {
		// Cycle through speed modes (worst case for branch predictor)
		readFns[idx](s);
		idx = (idx + 1) % 3;
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_FuncPtrDispatch_Mixed);

// -----------------------------------------------------------------------------
// Switch Dispatch (NEW approach)
// -----------------------------------------------------------------------------
// Simulates the ExecReadTiming() / ExecWriteTiming() inline switch

static void BM_SnesMemMgr_SwitchDispatch_Read(benchmark::State& state) {
	FakeExecState s;

	for (auto _ : state) {
		switch (s.cpuSpeed) {
			case 8: [[likely]] SimIncMasterClock<4>(s); break;
			case 6: SimIncMasterClock<2>(s); break;
			case 12: SimIncMasterClock<8>(s); break;
		}
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_SwitchDispatch_Read);

static void BM_SnesMemMgr_SwitchDispatch_Write(benchmark::State& state) {
	FakeExecState s;

	for (auto _ : state) {
		switch (s.cpuSpeed) {
			case 8: [[likely]] SimIncMasterClock<8>(s); break;
			case 6: SimIncMasterClock<6>(s); break;
			case 12: SimIncMasterClock<12>(s); break;
		}
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_SwitchDispatch_Write);

// Switch with varying speed (simulates real-world speed changes)
static void BM_SnesMemMgr_SwitchDispatch_Mixed(benchmark::State& state) {
	FakeExecState s;
	uint8_t speeds[] = { 6, 8, 12 };
	size_t idx = 0;

	for (auto _ : state) {
		s.cpuSpeed = speeds[idx];
		switch (s.cpuSpeed) {
			case 8: [[likely]] SimIncMasterClock<4>(s); break;
			case 6: SimIncMasterClock<2>(s); break;
			case 12: SimIncMasterClock<8>(s); break;
		}
		idx = (idx + 1) % 3;
		benchmark::DoNotOptimize(s.masterClock);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_SwitchDispatch_Mixed);

// -----------------------------------------------------------------------------
// Simulated Full Read Path
// -----------------------------------------------------------------------------
// Simulates the overhead of a Read() call including timing dispatch,
// handler lookup (array index), and direct read path

static void BM_SnesMemMgr_SimulatedReadPath(benchmark::State& state) {
	FakeExecState s;
	alignas(64) uint8_t fakeRam[0x1000] = {};
	for (int i = 0; i < 0x1000; i++) fakeRam[i] = static_cast<uint8_t>(i);

	// Simulate handler lookup table (just an array of pointers like _handlers[])
	uint8_t* handlerPtrs[0x100] = {};
	for (int i = 0; i < 0x100; i++) handlerPtrs[i] = fakeRam;

	uint32_t addr = 0x7E0000;

	for (auto _ : state) {
		// 1. Timing dispatch (switch)
		switch (s.cpuSpeed) {
			case 8: [[likely]] SimIncMasterClock<4>(s); break;
			case 6: SimIncMasterClock<2>(s); break;
			case 12: SimIncMasterClock<8>(s); break;
		}

		// 2. Handler lookup (O(1) array)
		uint8_t* handler = handlerPtrs[(addr >> 12) & 0xFF];

		// 3. Direct read
		uint8_t value = handler[addr & 0xFFF];
		benchmark::DoNotOptimize(value);

		addr = (addr + 1) & 0x7FFFFF;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesMemMgr_SimulatedReadPath);
