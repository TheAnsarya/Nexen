#include "pch.h"
#include <array>
#include "NES/NesTypes.h"

// =============================================================================
// NES CPU State Benchmarks
// =============================================================================

// Benchmark flag manipulation - common operation in CPU emulation
static void BM_NesCpu_FlagManipulation(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.PS = 0x24;  // Initial: I flag + Reserved

	for (auto _ : state) {
		// Simulate typical flag updates
		cpuState.PS |= PSFlags::Carry;
		cpuState.PS &= ~PSFlags::Zero;
		cpuState.PS |= PSFlags::Negative;
		cpuState.PS &= ~PSFlags::Overflow;
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_NesCpu_FlagManipulation);

// Benchmark Zero and Negative flag calculation (very frequent operation)
static void BM_NesCpu_SetZeroNegativeFlags(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		// Clear Z and N flags
		cpuState.PS &= ~(PSFlags::Zero | PSFlags::Negative);
		// Set based on value
		if (value == 0) cpuState.PS |= PSFlags::Zero;
		if (value & 0x80) cpuState.PS |= PSFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_SetZeroNegativeFlags);

// Benchmark branchless Zero/Negative flag setting (comparison)
static void BM_NesCpu_SetZeroNegativeFlags_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Zero | PSFlags::Negative);
		cpuState.PS |= (value == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (value >> 7) << 7;  // Move bit 7 to Negative flag position
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_SetZeroNegativeFlags_Branchless);

// Benchmark stack operations (push/pop patterns)
static void BM_NesCpu_StackPushPop(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.SP = 0xFF;
	std::array<uint8_t, 256> stack{};

	for (auto _ : state) {
		// Push pattern
		stack[0x100 + cpuState.SP--] = cpuState.A;
		stack[0x100 + cpuState.SP--] = cpuState.X;
		// Pop pattern
		cpuState.X = stack[0x100 + ++cpuState.SP];
		cpuState.A = stack[0x100 + ++cpuState.SP];
		benchmark::DoNotOptimize(cpuState.SP);
		benchmark::DoNotOptimize(cpuState.A);
	}
	state.SetItemsProcessed(state.iterations() * 4);  // 4 stack operations
}
BENCHMARK(BM_NesCpu_StackPushPop);

// Benchmark address mode calculations - Zero Page
static void BM_NesCpu_AddrMode_ZeroPage(benchmark::State& state) {
	uint8_t operand = 0x42;

	for (auto _ : state) {
		uint16_t addr = operand;
		benchmark::DoNotOptimize(addr);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_AddrMode_ZeroPage);

// Benchmark address mode calculations - Zero Page X
static void BM_NesCpu_AddrMode_ZeroPageX(benchmark::State& state) {
	uint8_t operand = 0x42;
	uint8_t X = 0x10;

	for (auto _ : state) {
		uint16_t addr = static_cast<uint8_t>(operand + X);  // Wraps in zero page
		benchmark::DoNotOptimize(addr);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_AddrMode_ZeroPageX);

// Benchmark address mode calculations - Absolute
static void BM_NesCpu_AddrMode_Absolute(benchmark::State& state) {
	uint8_t lowByte = 0x42;
	uint8_t highByte = 0x80;

	for (auto _ : state) {
		uint16_t addr = lowByte | (static_cast<uint16_t>(highByte) << 8);
		benchmark::DoNotOptimize(addr);
		lowByte++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_AddrMode_Absolute);

// Benchmark address mode calculations - Absolute X with page crossing check
static void BM_NesCpu_AddrMode_AbsoluteX_PageCross(benchmark::State& state) {
	uint16_t baseAddr = 0x80F0;
	uint8_t X = 0x20;

	for (auto _ : state) {
		uint16_t addr = baseAddr + X;
		bool pageCrossed = ((baseAddr & 0xFF00) != (addr & 0xFF00));
		benchmark::DoNotOptimize(addr);
		benchmark::DoNotOptimize(pageCrossed);
		baseAddr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_AddrMode_AbsoluteX_PageCross);

// Benchmark indirect indexed (Y) - most complex common mode
static void BM_NesCpu_AddrMode_IndirectY(benchmark::State& state) {
	std::array<uint8_t, 256> zeroPage{};
	zeroPage[0x42] = 0x00;
	zeroPage[0x43] = 0x80;
	uint8_t zpAddr = 0x42;
	uint8_t Y = 0x10;

	for (auto _ : state) {
		uint16_t baseAddr = zeroPage[zpAddr] | (static_cast<uint16_t>(zeroPage[zpAddr + 1]) << 8);
		uint16_t addr = baseAddr + Y;
		bool pageCrossed = ((baseAddr & 0xFF00) != (addr & 0xFF00));
		benchmark::DoNotOptimize(addr);
		benchmark::DoNotOptimize(pageCrossed);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_AddrMode_IndirectY);

// Benchmark ADC instruction pattern (8-bit with carry)
static void BM_NesCpu_Instruction_ADC(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0x40;
	cpuState.PS = 0;
	uint8_t operand = 0x30;

	for (auto _ : state) {
		uint16_t result = cpuState.A + operand + (cpuState.PS & PSFlags::Carry);

		// Set flags
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Zero | PSFlags::Negative | PSFlags::Overflow);
		if (result > 0xFF) cpuState.PS |= PSFlags::Carry;
		if ((result & 0xFF) == 0) cpuState.PS |= PSFlags::Zero;
		if (result & 0x80) cpuState.PS |= PSFlags::Negative;
		// Overflow: sign of result differs from sign of both inputs
		if (~(cpuState.A ^ operand) & (cpuState.A ^ result) & 0x80) {
			cpuState.PS |= PSFlags::Overflow;
		}

		cpuState.A = static_cast<uint8_t>(result);
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.PS);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_Instruction_ADC);

// Benchmark branch instruction pattern
static void BM_NesCpu_Instruction_Branch(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.PC = 0x8000;
	cpuState.PS = PSFlags::Zero;  // BEQ will be taken
	int8_t offset = 10;

	for (auto _ : state) {
		bool taken = (cpuState.PS & PSFlags::Zero) != 0;
		if (taken) {
			uint16_t oldPage = cpuState.PC & 0xFF00;
			cpuState.PC += offset;
			bool pageCrossed = (cpuState.PC & 0xFF00) != oldPage;
			benchmark::DoNotOptimize(pageCrossed);
		}
		benchmark::DoNotOptimize(cpuState.PC);
		cpuState.PC++;  // Reset for next iteration
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_Instruction_Branch);

// Benchmark typical read-modify-write pattern (INC, DEC, shifts)
static void BM_NesCpu_Instruction_RMW_Pattern(benchmark::State& state) {
	uint8_t memory = 0x42;
	NesCpuState cpuState = {};

	for (auto _ : state) {
		// Read
		uint8_t value = memory;
		// Modify
		value++;
		// Write
		memory = value;
		// Set flags
		cpuState.PS &= ~(PSFlags::Zero | PSFlags::Negative);
		if (value == 0) cpuState.PS |= PSFlags::Zero;
		if (value & 0x80) cpuState.PS |= PSFlags::Negative;
		benchmark::DoNotOptimize(memory);
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_Instruction_RMW_Pattern);

// =============================================================================
// Phase 2: Branchless vs Branching Comparisons
// =============================================================================

// --- CMP ---

static void BM_NesCpu_CMP_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t reg = 0, value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		auto result = reg - value;
		if (reg >= value) cpuState.PS |= PSFlags::Carry;
		if (reg == value) cpuState.PS |= PSFlags::Zero;
		if ((result & 0x80) == 0x80) cpuState.PS |= PSFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		reg += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_CMP_Branching);

static void BM_NesCpu_CMP_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t reg = 0, value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		auto result = reg - value;
		cpuState.PS |= (reg >= value) ? PSFlags::Carry : 0;
		cpuState.PS |= ((uint8_t)result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		reg += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_CMP_Branchless);

// --- ADD (ADC) ---

static void BM_NesCpu_ADD_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		uint16_t result = (uint16_t)cpuState.A + (uint16_t)value + (cpuState.PS & PSFlags::Carry);
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Overflow | PSFlags::Zero);
		cpuState.PS |= ((uint8_t)result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= ((uint8_t)result & 0x80);
		if (~(cpuState.A ^ value) & (cpuState.A ^ result) & 0x80) cpuState.PS |= PSFlags::Overflow;
		if (result > 0xFF) cpuState.PS |= PSFlags::Carry;
		cpuState.A = (uint8_t)result;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ADD_Branching);

static void BM_NesCpu_ADD_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		uint16_t result = (uint16_t)cpuState.A + (uint16_t)value + (cpuState.PS & PSFlags::Carry);
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Overflow | PSFlags::Zero);
		cpuState.PS |= ((uint8_t)result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= ((uint8_t)result & 0x80);
		cpuState.PS |= (~(cpuState.A ^ value) & (cpuState.A ^ result) & 0x80) ? PSFlags::Overflow : 0;
		cpuState.PS |= (result > 0xFF) ? PSFlags::Carry : 0;
		cpuState.A = (uint8_t)result;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ADD_Branchless);

// --- ADD (ADC) Truly Branchless: bit shift instead of ternary ---

static void BM_NesCpu_ADD_TrulyBranchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		uint16_t result = (uint16_t)cpuState.A + (uint16_t)value + (cpuState.PS & PSFlags::Carry);
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Overflow | PSFlags::Zero);
		cpuState.PS |= ((uint8_t)result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= ((uint8_t)result & 0x80);
		// Shift overflow bit (0x80) to Overflow position (0x40) — no ternary
		cpuState.PS |= ((~(cpuState.A ^ value) & (cpuState.A ^ result) & 0x80) >> 1);
		// High byte of result is 0 or 1 = Carry
		cpuState.PS |= static_cast<uint8_t>(result >> 8);
		cpuState.A = (uint8_t)result;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ADD_TrulyBranchless);

// --- BIT ---

static void BM_NesCpu_BIT_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Zero | PSFlags::Overflow | PSFlags::Negative);
		if ((cpuState.A & value) == 0) cpuState.PS |= PSFlags::Zero;
		if (value & 0x40) cpuState.PS |= PSFlags::Overflow;
		if (value & 0x80) cpuState.PS |= PSFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		cpuState.A += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_BIT_Branching);

static void BM_NesCpu_BIT_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Zero | PSFlags::Overflow | PSFlags::Negative);
		cpuState.PS |= ((cpuState.A & value) == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (value & 0x40);
		cpuState.PS |= (value & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		cpuState.A += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_BIT_Branchless);

// --- ASL ---

static void BM_NesCpu_ASL_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		if (value & 0x80) cpuState.PS |= PSFlags::Carry;
		uint8_t result = value << 1;
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ASL_Branching);

static void BM_NesCpu_ASL_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		cpuState.PS |= (value >> 7);
		uint8_t result = value << 1;
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ASL_Branchless);

// --- LSR ---

static void BM_NesCpu_LSR_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		if (value & 0x01) cpuState.PS |= PSFlags::Carry;
		uint8_t result = value >> 1;
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_LSR_Branching);

static void BM_NesCpu_LSR_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		cpuState.PS |= (value & 0x01);
		uint8_t result = value >> 1;
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_LSR_Branchless);

// --- ROL ---

static void BM_NesCpu_ROL_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		bool carryIn = cpuState.PS & PSFlags::Carry;
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		if (value & 0x80) cpuState.PS |= PSFlags::Carry;
		uint8_t result = (value << 1) | (carryIn ? 0x01 : 0x00);
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ROL_Branching);

static void BM_NesCpu_ROL_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		bool carryIn = cpuState.PS & PSFlags::Carry;
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		cpuState.PS |= (value >> 7);
		uint8_t result = (value << 1) | (carryIn ? 0x01 : 0x00);
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ROL_Branchless);

// --- ROR ---

static void BM_NesCpu_ROR_Branching(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		bool carryIn = cpuState.PS & PSFlags::Carry;
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		if (value & 0x01) cpuState.PS |= PSFlags::Carry;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0x00);
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ROR_Branching);

static void BM_NesCpu_ROR_Branchless(benchmark::State& state) {
	NesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		bool carryIn = cpuState.PS & PSFlags::Carry;
		cpuState.PS &= ~(PSFlags::Carry | PSFlags::Negative | PSFlags::Zero);
		cpuState.PS |= (value & 0x01);
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0x00);
		cpuState.PS |= (result == 0) ? PSFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_NesCpu_ROR_Branchless);

