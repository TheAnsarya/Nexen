#include "pch.h"
#include "SMS/SmsTypes.h"

// =============================================================================
// SMS Z80 CPU Benchmarks
// =============================================================================

// --------------------------------------------------------------------------
// SetFlagState: Branching vs Branchless
// --------------------------------------------------------------------------

/// <summary>
/// Original branching implementation: if/else SetFlag/ClearFlag.
/// </summary>
static void BM_SmsCpu_SetFlagState_Branching(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		bool flagState = (value & 0x80) != 0;
		if (flagState) {
			cpuState.Flags |= SmsCpuFlags::Sign;
		} else {
			cpuState.Flags &= ~SmsCpuFlags::Sign;
		}
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_SetFlagState_Branching);

/// <summary>
/// Branchless implementation using two's complement trick.
/// This is the current implementation in SmsCpu.cpp.
/// </summary>
static void BM_SmsCpu_SetFlagState_Branchless(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		bool flagState = (value & 0x80) != 0;
		cpuState.Flags = (cpuState.Flags & ~SmsCpuFlags::Sign) |
			(-static_cast<uint8_t>(flagState) & SmsCpuFlags::Sign);
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_SetFlagState_Branchless);

// --------------------------------------------------------------------------
// SetStandardFlags (Sign|Zero|F5|F3|Parity): Branching vs Branchless
// --------------------------------------------------------------------------

/// <summary>
/// Simulates SetStandardFlags using branching SetFlagState calls.
/// </summary>
static void BM_SmsCpu_StandardFlags_Branching(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	// Precompute parity table
	static bool parityTable[256];
	static bool initialized = false;
	if (!initialized) {
		for (int i = 0; i < 256; i++) {
			uint8_t v = static_cast<uint8_t>(i);
			v ^= v >> 4;
			v ^= v >> 2;
			v ^= v >> 1;
			parityTable[i] = (v & 1) == 0;
		}
		initialized = true;
	}

	for (auto _ : state) {
		// Sign
		if (value & 0x80) cpuState.Flags |= SmsCpuFlags::Sign;
		else cpuState.Flags &= ~SmsCpuFlags::Sign;
		// Zero
		if (value == 0) cpuState.Flags |= SmsCpuFlags::Zero;
		else cpuState.Flags &= ~SmsCpuFlags::Zero;
		// F5
		if (value & SmsCpuFlags::F5) cpuState.Flags |= SmsCpuFlags::F5;
		else cpuState.Flags &= ~SmsCpuFlags::F5;
		// F3
		if (value & SmsCpuFlags::F3) cpuState.Flags |= SmsCpuFlags::F3;
		else cpuState.Flags &= ~SmsCpuFlags::F3;
		// Parity
		if (parityTable[value]) cpuState.Flags |= SmsCpuFlags::Parity;
		else cpuState.Flags &= ~SmsCpuFlags::Parity;

		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_StandardFlags_Branching);

/// <summary>
/// Simulates SetStandardFlags using branchless SetFlagState calls.
/// </summary>
static void BM_SmsCpu_StandardFlags_Branchless(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	// Precompute parity table
	static bool parityTable[256];
	static bool initialized = false;
	if (!initialized) {
		for (int i = 0; i < 256; i++) {
			uint8_t v = static_cast<uint8_t>(i);
			v ^= v >> 4;
			v ^= v >> 2;
			v ^= v >> 1;
			parityTable[i] = (v & 1) == 0;
		}
		initialized = true;
	}

	auto SetFlagState = [](uint8_t& flags, uint8_t flag, bool s) {
		flags = (flags & ~flag) | (-static_cast<uint8_t>(s) & flag);
	};

	for (auto _ : state) {
		SetFlagState(cpuState.Flags, SmsCpuFlags::Sign, (value & 0x80) != 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::Zero, value == 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::F5, (value & SmsCpuFlags::F5) != 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::F3, (value & SmsCpuFlags::F3) != 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::Parity, parityTable[value]);
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_StandardFlags_Branchless);

// --------------------------------------------------------------------------
// Multiple SetFlagState calls: Worst-case chained operations
// --------------------------------------------------------------------------

/// <summary>
/// Simulates a worst-case scenario with 3 consecutive SetFlagState calls
/// using branching, as seen in many SMS Z80 instructions.
/// </summary>
static void BM_SmsCpu_ChainedSetFlagState_Branching(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		// 3 flag updates (common in ALU ops)
		if (value & 0x80) cpuState.Flags |= SmsCpuFlags::Sign;
		else cpuState.Flags &= ~SmsCpuFlags::Sign;

		if (value == 0) cpuState.Flags |= SmsCpuFlags::Zero;
		else cpuState.Flags &= ~SmsCpuFlags::Zero;

		if (value & 0x01) cpuState.Flags |= SmsCpuFlags::Carry;
		else cpuState.Flags &= ~SmsCpuFlags::Carry;

		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_ChainedSetFlagState_Branching);

/// <summary>
/// Same 3 consecutive flag updates using branchless.
/// </summary>
static void BM_SmsCpu_ChainedSetFlagState_Branchless(benchmark::State& state) {
	SmsCpuState cpuState = {};
	uint8_t value = 0;

	auto SetFlagState = [](uint8_t& flags, uint8_t flag, bool s) {
		flags = (flags & ~flag) | (-static_cast<uint8_t>(s) & flag);
	};

	for (auto _ : state) {
		SetFlagState(cpuState.Flags, SmsCpuFlags::Sign, (value & 0x80) != 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::Zero, value == 0);
		SetFlagState(cpuState.Flags, SmsCpuFlags::Carry, (value & 0x01) != 0);
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SmsCpu_ChainedSetFlagState_Branchless);
