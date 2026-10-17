#include "pch.h"
#include "PCE/PceTypes.h"

// =============================================================================
// PCE HuC6280 CPU Benchmarks
// =============================================================================

// --------------------------------------------------------------------------
// SetZeroNegativeFlags: Branching vs Branchless
// --------------------------------------------------------------------------

/// <summary>
/// Original branching implementation (if/else if pattern).
/// </summary>
static void BM_PceCpu_SetZeroNeg_Branching(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Zero | PceCpuFlags::Negative);
		if (value == 0) {
			cpuState.PS |= PceCpuFlags::Zero;
		} else if (value & 0x80) {
			cpuState.PS |= PceCpuFlags::Negative;
		}
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_SetZeroNeg_Branching);

/// <summary>
/// Branchless implementation using conditional OR + direct bitmask.
/// This is the current implementation in PceCpu.cpp.
/// </summary>
static void BM_PceCpu_SetZeroNeg_Branchless(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Zero | PceCpuFlags::Negative);
		cpuState.PS |= (value == 0) ? PceCpuFlags::Zero : 0;
		cpuState.PS |= (value & PceCpuFlags::Negative);
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_SetZeroNeg_Branchless);

// --------------------------------------------------------------------------
// CMP: Branching vs Branchless
// --------------------------------------------------------------------------

/// <summary>
/// CMP with branching (current implementation in PceCpu.Instructions.cpp).
/// </summary>
static void BM_PceCpu_CMP_Branching(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t reg = 0;
	uint8_t value = 0x42;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		auto result = reg - value;
		if (reg >= value) cpuState.PS |= PceCpuFlags::Carry;
		if (reg == value) cpuState.PS |= PceCpuFlags::Zero;
		if ((result & 0x80) == 0x80) cpuState.PS |= PceCpuFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		reg++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_CMP_Branching);

/// <summary>
/// CMP with branchless flag setting (potential future optimization).
/// </summary>
static void BM_PceCpu_CMP_Branchless(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t reg = 0;
	uint8_t value = 0x42;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		auto result = reg - value;
		cpuState.PS |= (reg >= value) ? PceCpuFlags::Carry : 0;
		cpuState.PS |= ((uint8_t)result == 0) ? PceCpuFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		reg++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_CMP_Branchless);

// --------------------------------------------------------------------------
// ASL (Arithmetic Shift Left): Branching vs Branchless Z/N
// --------------------------------------------------------------------------

static void BM_PceCpu_ASL_Branching(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x80) cpuState.PS |= PceCpuFlags::Carry;
		uint8_t result = value << 1;
		if (result == 0) cpuState.PS |= PceCpuFlags::Zero;
		else if (result & 0x80) cpuState.PS |= PceCpuFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_ASL_Branching);

static void BM_PceCpu_ASL_Branchless(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x80) cpuState.PS |= PceCpuFlags::Carry;
		uint8_t result = value << 1;
		cpuState.PS |= (result == 0) ? PceCpuFlags::Zero : 0;
		cpuState.PS |= (result & PceCpuFlags::Negative);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_ASL_Branchless);

// --------------------------------------------------------------------------
// SetRegister: Full flow (clear + set flags + assign)
// --------------------------------------------------------------------------

static void BM_PceCpu_SetRegister(benchmark::State& state) {
	PceCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		// Simulates SetRegister flow with branchless SetZeroNegativeFlags
		cpuState.PS &= ~(PceCpuFlags::Zero | PceCpuFlags::Negative);
		cpuState.PS |= (value == 0) ? PceCpuFlags::Zero : 0;
		cpuState.PS |= (value & PceCpuFlags::Negative);
		cpuState.A = value;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PceCpu_SetRegister);
