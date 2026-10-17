#include "pch.h"
#include <array>
#include "Gameboy/GbTypes.h"

// =============================================================================
// Game Boy CPU State Benchmarks (Sharp LR35902 / SM83)
// =============================================================================
// The Game Boy CPU is a hybrid between Intel 8080 and Z80, with unique
// characteristics including combined AF register, unique flag layout,
// and 16-bit register pairs (BC, DE, HL).

// -----------------------------------------------------------------------------
// Flag Manipulation Benchmarks
// -----------------------------------------------------------------------------

// Benchmark flag manipulation - GB has unique flag layout in upper nibble of F
static void BM_GbCpu_FlagManipulation(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.Flags = 0x00;

	for (auto _ : state) {
		cpuState.Flags |= GbCpuFlags::Carry;
		cpuState.Flags &= ~GbCpuFlags::Zero;
		cpuState.Flags |= GbCpuFlags::HalfCarry;
		cpuState.Flags &= ~GbCpuFlags::AddSub;
		benchmark::DoNotOptimize(cpuState.Flags);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbCpu_FlagManipulation);

// Benchmark Zero flag calculation (most frequent operation)
static void BM_GbCpu_SetZeroFlag(benchmark::State& state) {
	GbCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.Flags &= ~GbCpuFlags::Zero;
		if (value == 0) cpuState.Flags |= GbCpuFlags::Zero;
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_SetZeroFlag);

// Benchmark branchless zero flag setting
static void BM_GbCpu_SetZeroFlag_Branchless(benchmark::State& state) {
	GbCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpuState.Flags &= ~GbCpuFlags::Zero;
		cpuState.Flags |= ((value == 0) ? GbCpuFlags::Zero : 0);
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_SetZeroFlag_Branchless);

// Benchmark Half-Carry flag calculation (unique to GB/Z80)
// Half-carry is set when there's a carry from bit 3 to bit 4
static void BM_GbCpu_HalfCarryCalculation(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x0F;
	uint8_t operand = 0x01;

	for (auto _ : state) {
		cpuState.Flags &= ~GbCpuFlags::HalfCarry;
		if (((cpuState.A & 0x0F) + (operand & 0x0F)) > 0x0F) {
			cpuState.Flags |= GbCpuFlags::HalfCarry;
		}
		benchmark::DoNotOptimize(cpuState.Flags);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_HalfCarryCalculation);

// Branchless half-carry calculation
static void BM_GbCpu_HalfCarryCalculation_Branchless(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x0F;
	uint8_t operand = 0x01;

	for (auto _ : state) {
		cpuState.Flags &= ~GbCpuFlags::HalfCarry;
		cpuState.Flags |= ((((cpuState.A & 0x0F) + (operand & 0x0F)) > 0x0F) ? GbCpuFlags::HalfCarry : 0);
		benchmark::DoNotOptimize(cpuState.Flags);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_HalfCarryCalculation_Branchless);

// Benchmark SetFlagState - branching version (original)
static void BM_GbCpu_SetFlagState_Branching(benchmark::State& state) {
	GbCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		// Simulate typical ALU pattern: set Zero, HalfCarry, Carry flags
		uint8_t flags = cpuState.Flags;
		bool zeroResult = (value == 0);
		bool halfCarry = ((value & 0x0F) == 0x0F);
		bool carry = (value > 0x7F);

		// Branching SetFlagState
		if (zeroResult) { flags |= GbCpuFlags::Zero; } else { flags &= ~GbCpuFlags::Zero; }
		if (halfCarry) { flags |= GbCpuFlags::HalfCarry; } else { flags &= ~GbCpuFlags::HalfCarry; }
		if (carry) { flags |= GbCpuFlags::Carry; } else { flags &= ~GbCpuFlags::Carry; }

		cpuState.Flags = flags;
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(BM_GbCpu_SetFlagState_Branching);

// Benchmark SetFlagState - branchless version (new)
static void BM_GbCpu_SetFlagState_Branchless(benchmark::State& state) {
	GbCpuState cpuState = {};
	uint8_t value = 0;

	for (auto _ : state) {
		// Same ALU pattern as branching version
		uint8_t flags = cpuState.Flags;
		bool zeroResult = (value == 0);
		bool halfCarry = ((value & 0x0F) == 0x0F);
		bool carry = (value > 0x7F);

		// Branchless SetFlagState
		flags = (flags & ~GbCpuFlags::Zero) | (-static_cast<uint8_t>(zeroResult) & GbCpuFlags::Zero);
		flags = (flags & ~GbCpuFlags::HalfCarry) | (-static_cast<uint8_t>(halfCarry) & GbCpuFlags::HalfCarry);
		flags = (flags & ~GbCpuFlags::Carry) | (-static_cast<uint8_t>(carry) & GbCpuFlags::Carry);

		cpuState.Flags = flags;
		benchmark::DoNotOptimize(cpuState.Flags);
		value++;
	}
	state.SetItemsProcessed(state.iterations() * 3);
}
BENCHMARK(BM_GbCpu_SetFlagState_Branchless);

// -----------------------------------------------------------------------------
// Register Pair Operations
// -----------------------------------------------------------------------------

// Benchmark 16-bit register pair read (BC, DE, HL)
static void BM_GbCpu_RegisterPairRead(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.H = 0x12;
	cpuState.L = 0x34;

	for (auto _ : state) {
		uint16_t hl = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		benchmark::DoNotOptimize(hl);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_RegisterPairRead);

// Benchmark 16-bit register pair write
static void BM_GbCpu_RegisterPairWrite(benchmark::State& state) {
	GbCpuState cpuState = {};
	uint16_t value = 0x1234;

	for (auto _ : state) {
		cpuState.H = static_cast<uint8_t>(value >> 8);
		cpuState.L = static_cast<uint8_t>(value);
		benchmark::DoNotOptimize(cpuState.H);
		benchmark::DoNotOptimize(cpuState.L);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_RegisterPairWrite);

// Benchmark INC/DEC on register pair (affects no flags!)
static void BM_GbCpu_RegisterPairIncDec(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.H = 0x12;
	cpuState.L = 0xFF;

	for (auto _ : state) {
		// INC HL (16-bit increment)
		uint16_t hl = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		hl++;
		cpuState.H = static_cast<uint8_t>(hl >> 8);
		cpuState.L = static_cast<uint8_t>(hl);

		// DEC HL
		hl = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		hl--;
		cpuState.H = static_cast<uint8_t>(hl >> 8);
		cpuState.L = static_cast<uint8_t>(hl);

		benchmark::DoNotOptimize(cpuState.H);
		benchmark::DoNotOptimize(cpuState.L);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbCpu_RegisterPairIncDec);

// -----------------------------------------------------------------------------
// Stack Operations
// -----------------------------------------------------------------------------

// Benchmark PUSH/POP (16-bit stack operations)
static void BM_GbCpu_StackPushPop(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.SP = 0xFFFE;
	cpuState.B = 0x12;
	cpuState.C = 0x34;
	std::array<uint8_t, 0x10000> memory{};

	for (auto _ : state) {
		// PUSH BC (high byte first, then low)
		memory[--cpuState.SP] = cpuState.B;
		memory[--cpuState.SP] = cpuState.C;

		// POP BC (low byte first, then high)
		cpuState.C = memory[cpuState.SP++];
		cpuState.B = memory[cpuState.SP++];

		benchmark::DoNotOptimize(cpuState.SP);
		benchmark::DoNotOptimize(cpuState.B);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbCpu_StackPushPop);

// Benchmark PUSH AF (special - lower nibble of F is always 0)
static void BM_GbCpu_StackPushAF(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.SP = 0xFFFE;
	cpuState.A = 0x42;
	cpuState.Flags = 0xB0;  // Z=1, N=0, H=1, C=1
	std::array<uint8_t, 0x10000> memory{};

	for (auto _ : state) {
		// PUSH AF (lower nibble of F is masked to 0)
		memory[--cpuState.SP] = cpuState.A;
		memory[--cpuState.SP] = cpuState.Flags & 0xF0;

		// POP AF
		cpuState.Flags = memory[cpuState.SP++] & 0xF0;
		cpuState.A = memory[cpuState.SP++];

		benchmark::DoNotOptimize(cpuState.SP);
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbCpu_StackPushAF);

// -----------------------------------------------------------------------------
// Address Calculations
// -----------------------------------------------------------------------------

// Benchmark (HL) addressing (most common memory access mode)
static void BM_GbCpu_AddrMode_HL_Indirect(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.H = 0xC0;
	cpuState.L = 0x00;

	for (auto _ : state) {
		uint16_t addr = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		benchmark::DoNotOptimize(addr);
		cpuState.L++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_AddrMode_HL_Indirect);

// Benchmark (HL+) and (HL-) auto-increment/decrement
static void BM_GbCpu_AddrMode_HL_AutoIncDec(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.H = 0xC0;
	cpuState.L = 0x00;

	for (auto _ : state) {
		// LDI: (HL+)
		uint16_t hl = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		uint16_t addr = hl++;
		cpuState.H = static_cast<uint8_t>(hl >> 8);
		cpuState.L = static_cast<uint8_t>(hl);
		benchmark::DoNotOptimize(addr);

		// LDD: (HL-)
		hl = (static_cast<uint16_t>(cpuState.H) << 8) | cpuState.L;
		addr = hl--;
		cpuState.H = static_cast<uint8_t>(hl >> 8);
		cpuState.L = static_cast<uint8_t>(hl);
		benchmark::DoNotOptimize(addr);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbCpu_AddrMode_HL_AutoIncDec);

// Benchmark high memory page addressing (FF00+n and FF00+C)
static void BM_GbCpu_AddrMode_HighMemory(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.C = 0x44;  // Common: 0xFF44 = LY register
	uint8_t offset = 0x00;

	for (auto _ : state) {
		// LDH A, (n) - High memory page
		uint16_t addr = 0xFF00 | offset;
		benchmark::DoNotOptimize(addr);

		// LDH A, (C) - High memory via C register
		addr = 0xFF00 | cpuState.C;
		benchmark::DoNotOptimize(addr);

		offset++;
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbCpu_AddrMode_HighMemory);

// Benchmark SP+r8 offset (unique signed offset addressing)
static void BM_GbCpu_AddrMode_SP_Offset(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.SP = 0xFFF0;
	int8_t offset = -16;

	for (auto _ : state) {
		uint16_t addr = static_cast<uint16_t>(cpuState.SP + offset);
		benchmark::DoNotOptimize(addr);
		offset++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_AddrMode_SP_Offset);

// -----------------------------------------------------------------------------
// Instruction Patterns
// -----------------------------------------------------------------------------

// Benchmark ADD instruction with all flags
static void BM_GbCpu_Instruction_ADD(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x40;
	uint8_t operand = 0x30;

	for (auto _ : state) {
		uint16_t result = cpuState.A + operand;

		cpuState.Flags = 0;  // Clear all flags (including AddSub)
		if ((result & 0xFF) == 0) cpuState.Flags |= GbCpuFlags::Zero;
		if (result > 0xFF) cpuState.Flags |= GbCpuFlags::Carry;
		if (((cpuState.A & 0x0F) + (operand & 0x0F)) > 0x0F) {
			cpuState.Flags |= GbCpuFlags::HalfCarry;
		}

		cpuState.A = static_cast<uint8_t>(result);
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_ADD);

// Benchmark SUB instruction (sets AddSub flag)
static void BM_GbCpu_Instruction_SUB(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x80;
	uint8_t operand = 0x10;

	for (auto _ : state) {
		int16_t result = cpuState.A - operand;

		cpuState.Flags = GbCpuFlags::AddSub;  // Always set for subtraction
		if ((result & 0xFF) == 0) cpuState.Flags |= GbCpuFlags::Zero;
		if (result < 0) cpuState.Flags |= GbCpuFlags::Carry;
		if ((cpuState.A & 0x0F) < (operand & 0x0F)) {
			cpuState.Flags |= GbCpuFlags::HalfCarry;
		}

		cpuState.A = static_cast<uint8_t>(result);
		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_SUB);

// Benchmark DAA (Decimal Adjust Accumulator - BCD correction)
static void BM_GbCpu_Instruction_DAA(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x15;
	cpuState.Flags = 0;

	for (auto _ : state) {
		uint8_t correction = 0;
		bool setCarry = false;

		if (!(cpuState.Flags & GbCpuFlags::AddSub)) {
			// After addition
			if ((cpuState.Flags & GbCpuFlags::Carry) || cpuState.A > 0x99) {
				correction |= 0x60;
				setCarry = true;
			}
			if ((cpuState.Flags & GbCpuFlags::HalfCarry) || (cpuState.A & 0x0F) > 0x09) {
				correction |= 0x06;
			}
			cpuState.A += correction;
		} else {
			// After subtraction
			if (cpuState.Flags & GbCpuFlags::Carry) {
				correction |= 0x60;
				setCarry = true;
			}
			if (cpuState.Flags & GbCpuFlags::HalfCarry) {
				correction |= 0x06;
			}
			cpuState.A -= correction;
		}

		cpuState.Flags &= ~(GbCpuFlags::Zero | GbCpuFlags::HalfCarry);
		if (cpuState.A == 0) cpuState.Flags |= GbCpuFlags::Zero;
		if (setCarry) cpuState.Flags |= GbCpuFlags::Carry;

		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_DAA);

// Benchmark conditional relative branch (JR cc, e)
static void BM_GbCpu_Instruction_JR_Conditional(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.PC = 0x0150;
	cpuState.Flags = GbCpuFlags::Zero;
	int8_t offset = 10;

	for (auto _ : state) {
		// JR Z, offset (taken when Zero flag is set)
		bool taken = (cpuState.Flags & GbCpuFlags::Zero) != 0;
		if (taken) {
			cpuState.PC = static_cast<uint16_t>(cpuState.PC + offset);
		}
		benchmark::DoNotOptimize(cpuState.PC);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_JR_Conditional);

// Benchmark bit test instruction (BIT b, r)
static void BM_GbCpu_Instruction_BIT(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x55;  // 01010101 - alternating bits
	uint8_t bitNum = 0;

	for (auto _ : state) {
		// BIT n, A - Test bit n of A
		bool bitSet = (cpuState.A & (1 << (bitNum & 7))) != 0;

		cpuState.Flags &= ~GbCpuFlags::Zero;
		cpuState.Flags &= ~GbCpuFlags::AddSub;
		cpuState.Flags |= GbCpuFlags::HalfCarry;
		if (!bitSet) cpuState.Flags |= GbCpuFlags::Zero;

		benchmark::DoNotOptimize(cpuState.Flags);
		bitNum++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_BIT);

// Benchmark rotate instructions (RLC, RRC, RL, RR)
static void BM_GbCpu_Instruction_Rotate(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x85;  // 10000101

	for (auto _ : state) {
		// RLC A - Rotate Left Circular
		bool carry = (cpuState.A & 0x80) != 0;
		cpuState.A = static_cast<uint8_t>((cpuState.A << 1) | (cpuState.A >> 7));

		cpuState.Flags = 0;
		if (cpuState.A == 0) cpuState.Flags |= GbCpuFlags::Zero;
		if (carry) cpuState.Flags |= GbCpuFlags::Carry;

		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_Rotate);

// Benchmark SWAP instruction (unique to GB - swap nibbles)
static void BM_GbCpu_Instruction_SWAP(benchmark::State& state) {
	GbCpuState cpuState = {};
	cpuState.A = 0x12;

	for (auto _ : state) {
		cpuState.A = static_cast<uint8_t>((cpuState.A >> 4) | (cpuState.A << 4));

		cpuState.Flags = 0;
		if (cpuState.A == 0) cpuState.Flags |= GbCpuFlags::Zero;

		benchmark::DoNotOptimize(cpuState.A);
		benchmark::DoNotOptimize(cpuState.Flags);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbCpu_Instruction_SWAP);

