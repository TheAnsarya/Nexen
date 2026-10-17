#include "pch.h"
#include <array>
#include "SNES/SnesCpuTypes.h"

// =============================================================================
// SNES CPU State Benchmarks (65816 Processor)
// =============================================================================
// The SNES uses a 65816 CPU which can operate in both 8-bit (emulation) and
// 16-bit (native) modes, making flag and register handling more complex than 6502.

// -----------------------------------------------------------------------------
// Flag Manipulation Benchmarks
// -----------------------------------------------------------------------------

// Benchmark flag manipulation - 65816 has more flag complexity than 6502
static void BM_SnesCpu_FlagManipulation(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PS = 0x24;  // Initial: I flag + Reserved
	cpuState.EmulationMode = false;

	for (auto _ : state) {
		// Simulate typical flag updates
		cpuState.PS |= ProcFlags::Carry;
		cpuState.PS &= ~ProcFlags::Zero;
		cpuState.PS |= ProcFlags::Negative;
		cpuState.PS &= ~ProcFlags::Overflow;
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_SnesCpu_FlagManipulation);

// Benchmark REP/SEP flag modification (unique to 65816)
// These instructions can set/clear multiple flags at once
static void BM_SnesCpu_REP_SEP_Flags(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PS = 0x30;  // M and X flags set (8-bit mode)

	for (auto _ : state) {
		// REP #$30 - Reset M and X flags (switch to 16-bit)
		cpuState.PS &= ~0x30;
		benchmark::DoNotOptimize(cpuState.PS);

		// SEP #$30 - Set M and X flags (switch to 8-bit)
		cpuState.PS |= 0x30;
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesCpu_REP_SEP_Flags);

// Benchmark mode checking (critical for 8/16-bit operations)
static void BM_SnesCpu_ModeCheck(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PS = 0;
	uint8_t psValues[] = {0x00, 0x10, 0x20, 0x30};  // Various mode combinations
	size_t idx = 0;

	for (auto _ : state) {
		cpuState.PS = psValues[idx++ & 3];
		bool is8BitMemory = (cpuState.PS & ProcFlags::MemoryMode8) != 0;
		bool is8BitIndex = (cpuState.PS & ProcFlags::IndexMode8) != 0;
		benchmark::DoNotOptimize(is8BitMemory);
		benchmark::DoNotOptimize(is8BitIndex);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesCpu_ModeCheck);

// -----------------------------------------------------------------------------
// Zero/Negative Flag Calculation
// -----------------------------------------------------------------------------

// Benchmark Zero and Negative flag calculation (8-bit mode)
static void BM_SnesCpu_SetZeroNegativeFlags_8Bit(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PS = ProcFlags::MemoryMode8;  // 8-bit mode
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) cpuState.PS |= ProcFlags::Zero;
		if (value & 0x80) cpuState.PS |= ProcFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_SetZeroNegativeFlags_8Bit);

// Benchmark Zero and Negative flag calculation (16-bit mode)
static void BM_SnesCpu_SetZeroNegativeFlags_16Bit(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PS = 0;  // 16-bit mode
	uint16_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) cpuState.PS |= ProcFlags::Zero;
		if (value & 0x8000) cpuState.PS |= ProcFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_SetZeroNegativeFlags_16Bit);

// Branchless flag calculation (comparison)
static void BM_SnesCpu_SetZeroNegativeFlags_Branchless(benchmark::State& state) {
	SnesCpuState cpuState = {};
	uint16_t value = 0;

	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Negative);
		cpuState.PS |= (value == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= (value >> 15) << 7;  // Move bit 15 to Negative flag position
		benchmark::DoNotOptimize(cpuState.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_SetZeroNegativeFlags_Branchless);

// -----------------------------------------------------------------------------
// Stack Operations (16-bit stack pointer)
// -----------------------------------------------------------------------------

// Benchmark stack operations with 16-bit stack pointer
static void BM_SnesCpu_StackPushPop_16Bit(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.SP = 0x1FFF;  // SNES stack starts at $01FF in emulation, $1FFF in native
	cpuState.A = 0x1234;
	cpuState.X = 0x5678;
	std::array<uint8_t, 0x2000> stack{};

	for (auto _ : state) {
		// Push 16-bit accumulator (low byte first, then high)
		stack[cpuState.SP--] = static_cast<uint8_t>(cpuState.A >> 8);
		stack[cpuState.SP--] = static_cast<uint8_t>(cpuState.A);
		// Push 16-bit X
		stack[cpuState.SP--] = static_cast<uint8_t>(cpuState.X >> 8);
		stack[cpuState.SP--] = static_cast<uint8_t>(cpuState.X);

		// Pop 16-bit X
		cpuState.X = stack[++cpuState.SP];
		cpuState.X |= static_cast<uint16_t>(stack[++cpuState.SP]) << 8;
		// Pop 16-bit A
		cpuState.A = stack[++cpuState.SP];
		cpuState.A |= static_cast<uint16_t>(stack[++cpuState.SP]) << 8;

		benchmark::DoNotOptimize(cpuState.SP);
		benchmark::DoNotOptimize(cpuState.A);
	}
	state.SetItemsProcessed(state.iterations() * 8);  // 8 byte operations
}
BENCHMARK(BM_SnesCpu_StackPushPop_16Bit);

// Stack with emulation mode restriction (wraps at $01FF)
static void BM_SnesCpu_StackPushPop_EmulationMode(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.SP = 0x01FF;
	cpuState.EmulationMode = true;
	cpuState.A = 0x42;
	std::array<uint8_t, 0x200> stack{};

	for (auto _ : state) {
		// Push with wrap at page 1
		stack[cpuState.SP] = static_cast<uint8_t>(cpuState.A);
		cpuState.SP = 0x0100 | ((cpuState.SP - 1) & 0xFF);  // Wrap within page 1

		// Pop with wrap
		cpuState.SP = 0x0100 | ((cpuState.SP + 1) & 0xFF);
		cpuState.A = stack[cpuState.SP];

		benchmark::DoNotOptimize(cpuState.SP);
		benchmark::DoNotOptimize(cpuState.A);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesCpu_StackPushPop_EmulationMode);

// -----------------------------------------------------------------------------
// Address Mode Calculations (24-bit addressing)
// -----------------------------------------------------------------------------

// Benchmark Direct Page addressing with D register offset
static void BM_SnesCpu_AddrMode_DirectPage(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.D = 0x2100;  // Direct page register
	uint8_t operand = 0x42;

	for (auto _ : state) {
		uint16_t addr = cpuState.D + operand;
		benchmark::DoNotOptimize(addr);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_AddrMode_DirectPage);

// Benchmark 24-bit Absolute Long addressing
static void BM_SnesCpu_AddrMode_AbsoluteLong(benchmark::State& state) {
	uint8_t lowByte = 0x42;
	uint8_t midByte = 0x80;
	uint8_t bankByte = 0x7E;

	for (auto _ : state) {
		uint32_t addr = lowByte | (static_cast<uint32_t>(midByte) << 8) |
						(static_cast<uint32_t>(bankByte) << 16);
		benchmark::DoNotOptimize(addr);
		lowByte++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_AddrMode_AbsoluteLong);

// Benchmark Absolute Indexed addressing with DBR
static void BM_SnesCpu_AddrMode_AbsoluteIndexed(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.DBR = 0x7E;  // Data Bank Register
	cpuState.X = 0x0010;
	uint16_t baseAddr = 0x8000;

	for (auto _ : state) {
		uint32_t addr = ((static_cast<uint32_t>(cpuState.DBR) << 16) | baseAddr) + cpuState.X;
		benchmark::DoNotOptimize(addr);
		baseAddr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_AddrMode_AbsoluteIndexed);

// Benchmark Stack Relative addressing (unique to 65816)
static void BM_SnesCpu_AddrMode_StackRelative(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.SP = 0x1FF0;
	uint8_t offset = 0x05;

	for (auto _ : state) {
		uint16_t addr = cpuState.SP + offset;
		benchmark::DoNotOptimize(addr);
		offset++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_AddrMode_StackRelative);

// Benchmark Indirect Long addressing [dp]
static void BM_SnesCpu_AddrMode_IndirectLong(benchmark::State& state) {
	std::array<uint8_t, 256> directPage{};
	directPage[0x42] = 0x00;  // Low byte
	directPage[0x43] = 0x80;  // Mid byte
	directPage[0x44] = 0x7E;  // Bank byte

	SnesCpuState cpuState = {};
	cpuState.D = 0;
	uint8_t dpAddr = 0x42;

	for (auto _ : state) {
		uint32_t addr = directPage[cpuState.D + dpAddr] |
						(static_cast<uint32_t>(directPage[cpuState.D + dpAddr + 1]) << 8) |
						(static_cast<uint32_t>(directPage[cpuState.D + dpAddr + 2]) << 16);
		benchmark::DoNotOptimize(addr);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_AddrMode_IndirectLong);

// -----------------------------------------------------------------------------
// Instruction Patterns
// -----------------------------------------------------------------------------

// Benchmark ADC instruction (16-bit mode)
static void BM_SnesCpu_Instruction_ADC_16Bit(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0x4000;
	cpuState.PS = 0;  // 16-bit mode, no carry
	uint16_t operand = 0x3000;

	for (auto _ : state) {
		uint32_t result = cpuState.A + operand + (cpuState.PS & ProcFlags::Carry);

		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Zero | ProcFlags::Negative | ProcFlags::Overflow);
		if (result > 0xFFFF) cpuState.PS |= ProcFlags::Carry;
		if ((result & 0xFFFF) == 0) cpuState.PS |= ProcFlags::Zero;
		if (result & 0x8000) cpuState.PS |= ProcFlags::Negative;
		if (~(cpuState.A ^ operand) & (cpuState.A ^ result) & 0x8000) {
			cpuState.PS |= ProcFlags::Overflow;
		}

		cpuState.A = static_cast<uint16_t>(result);
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.PS);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_ADC_16Bit);

// Benchmark XBA instruction (exchange B and A bytes)
static void BM_SnesCpu_Instruction_XBA(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0x1234;

	for (auto _ : state) {
		// XBA swaps high and low bytes of accumulator
		cpuState.A = ((cpuState.A & 0xFF) << 8) | ((cpuState.A >> 8) & 0xFF);

		// Flags are set based on new low byte only
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if ((cpuState.A & 0xFF) == 0) cpuState.PS |= ProcFlags::Zero;
		if (cpuState.A & 0x80) cpuState.PS |= ProcFlags::Negative;

		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_XBA);

// Benchmark TCS/TSC (transfer A to/from stack pointer)
static void BM_SnesCpu_Instruction_TCS_TSC(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0x1FF0;
	cpuState.SP = 0x01FF;

	for (auto _ : state) {
		// TCS - Transfer A to SP (no flags affected)
		cpuState.SP = cpuState.A;
		benchmark::DoNotOptimize(cpuState.SP);

		// TSC - Transfer SP to A (sets Z and N)
		cpuState.A = cpuState.SP;
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if (cpuState.A == 0) cpuState.PS |= ProcFlags::Zero;
		if (cpuState.A & 0x8000) cpuState.PS |= ProcFlags::Negative;
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.PS);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_SnesCpu_Instruction_TCS_TSC);

// Benchmark MVN/MVP block move pattern
static void BM_SnesCpu_Instruction_BlockMove(benchmark::State& state) {
	std::array<uint8_t, 0x10000> srcBank{};
	std::array<uint8_t, 0x10000> dstBank{};
	std::fill(srcBank.begin(), srcBank.end(), static_cast<uint8_t>(0xAA));

	SnesCpuState cpuState = {};
	cpuState.A = 0x00FF;  // Transfer 256 bytes (A+1)
	cpuState.X = 0x1000;  // Source
	cpuState.Y = 0x2000;  // Destination

	for (auto _ : state) {
		// Simulate one byte of block move
		dstBank[cpuState.Y] = srcBank[cpuState.X];
		cpuState.X++;
		cpuState.Y++;
		cpuState.A--;
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.X);
		benchmark::DoNotOptimize(cpuState.Y);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_BlockMove);

// Benchmark branch with 16-bit program counter
static void BM_SnesCpu_Instruction_Branch(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PC = 0x8000;
	cpuState.K = 0x00;  // Program Bank
	cpuState.PS = ProcFlags::Zero;  // BEQ will be taken
	int8_t offset = 10;

	for (auto _ : state) {
		bool taken = (cpuState.PS & ProcFlags::Zero) != 0;
		if (taken) {
			cpuState.PC = static_cast<uint16_t>(cpuState.PC + offset);
		}
		benchmark::DoNotOptimize(cpuState.PC);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_Branch);

// Benchmark BRL (Branch Relative Long - 16-bit offset)
static void BM_SnesCpu_Instruction_BRL(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.PC = 0x8000;
	int16_t offset = 0x1000;  // 16-bit offset

	for (auto _ : state) {
		cpuState.PC = static_cast<uint16_t>(cpuState.PC + offset);
		benchmark::DoNotOptimize(cpuState.PC);
		offset = -offset;  // Alternate forward/backward
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_BRL);

// Benchmark PEI/PEA (Push Effective Indirect/Absolute)
static void BM_SnesCpu_Instruction_PushEffective(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.SP = 0x1FFF;
	std::array<uint8_t, 0x2000> stack{};
	uint16_t effectiveAddr = 0x8000;

	for (auto _ : state) {
		// PEA - Push 16-bit value onto stack
		stack[cpuState.SP--] = static_cast<uint8_t>(effectiveAddr >> 8);
		stack[cpuState.SP--] = static_cast<uint8_t>(effectiveAddr);

		// Restore SP for next iteration
		cpuState.SP += 2;

		benchmark::DoNotOptimize(cpuState.SP);
		effectiveAddr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Instruction_PushEffective);

// =============================================================================
// Phase 2: Branchless vs Branching Comparisons
// =============================================================================

// --- Compare 8-bit ---

static void BM_SnesCpu_Compare8_Branching(benchmark::State& state) {
	SnesCpuState cpuState = {};
	uint8_t reg = 0, value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero);
		if (reg >= value) cpuState.PS |= ProcFlags::Carry;
		uint8_t result = reg - value;
		if (result == 0) cpuState.PS |= ProcFlags::Zero;
		if (result & 0x80) cpuState.PS |= ProcFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		reg += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Compare8_Branching);

static void BM_SnesCpu_Compare8_Branchless(benchmark::State& state) {
	SnesCpuState cpuState = {};
	uint8_t reg = 0, value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero);
		uint8_t result = reg - value;
		cpuState.PS |= (reg >= value) ? ProcFlags::Carry : 0;
		cpuState.PS |= (result == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		reg += 7; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Compare8_Branchless);

// --- Add8 Overflow+Carry ---

static void BM_SnesCpu_Add8_Branching(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		uint32_t result = (cpuState.A & 0xFF) + value + (cpuState.PS & ProcFlags::Carry);
		if (~(cpuState.A ^ value) & (cpuState.A ^ result) & 0x80) {
			cpuState.PS |= ProcFlags::Overflow;
		} else {
			cpuState.PS &= ~ProcFlags::Overflow;
		}
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero);
		cpuState.PS |= ((uint8_t)result == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= ((uint8_t)result & 0x80);
		if (result > 0xFF) cpuState.PS |= ProcFlags::Carry;
		cpuState.A = (cpuState.A & 0xFF00) | (uint8_t)result;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Add8_Branching);

static void BM_SnesCpu_Add8_Branchless(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		uint32_t result = (cpuState.A & 0xFF) + value + (cpuState.PS & ProcFlags::Carry);
		uint8_t overflowFlag = (~(cpuState.A ^ value) & (cpuState.A ^ result) & 0x80) ? ProcFlags::Overflow : 0;
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero | ProcFlags::Overflow);
		cpuState.PS |= ((uint8_t)result == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= ((uint8_t)result & 0x80);
		cpuState.PS |= overflowFlag;
		cpuState.PS |= (result > 0xFF) ? ProcFlags::Carry : 0;
		cpuState.A = (cpuState.A & 0xFF00) | (uint8_t)result;
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(cpuState.A);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_Add8_Branchless);

// --- ShiftLeft 8-bit ---

static void BM_SnesCpu_ShiftLeft8_Branching(benchmark::State& state) {
	SnesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero);
		if (value & 0x80) cpuState.PS |= ProcFlags::Carry;
		uint8_t result = value << 1;
		cpuState.PS |= (result == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_ShiftLeft8_Branching);

static void BM_SnesCpu_ShiftLeft8_Branchless(benchmark::State& state) {
	SnesCpuState cpuState = {};
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Carry | ProcFlags::Negative | ProcFlags::Zero);
		cpuState.PS |= (value >> 7) & ProcFlags::Carry;
		uint8_t result = value << 1;
		cpuState.PS |= (result == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= (result & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		benchmark::DoNotOptimize(result);
		value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_ShiftLeft8_Branchless);

// --- TestBits 8-bit ---

static void BM_SnesCpu_TestBits8_Branching(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Overflow | ProcFlags::Negative);
		if (((uint8_t)cpuState.A & value) == 0) cpuState.PS |= ProcFlags::Zero;
		if (value & 0x40) cpuState.PS |= ProcFlags::Overflow;
		if (value & 0x80) cpuState.PS |= ProcFlags::Negative;
		benchmark::DoNotOptimize(cpuState.PS);
		cpuState.A = (cpuState.A + 7) & 0xFF; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_TestBits8_Branching);

static void BM_SnesCpu_TestBits8_Branchless(benchmark::State& state) {
	SnesCpuState cpuState = {};
	cpuState.A = 0;
	uint8_t value = 0;
	for (auto _ : state) {
		cpuState.PS &= ~(ProcFlags::Zero | ProcFlags::Overflow | ProcFlags::Negative);
		cpuState.PS |= (((uint8_t)cpuState.A & value) == 0) ? ProcFlags::Zero : 0;
		cpuState.PS |= (value & 0x40);
		cpuState.PS |= (value & 0x80);
		benchmark::DoNotOptimize(cpuState.PS);
		cpuState.A = (cpuState.A + 7) & 0xFF; value += 13;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SnesCpu_TestBits8_Branchless);

