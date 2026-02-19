#include "pch.h"
#include <array>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx 65C02 CPU Benchmarks
// =============================================================================
// Benchmarks for WDC 65C02 CPU operations used in the Atari Lynx emulation core.
// The CPU runs at 4 MHz (16 MHz master / 4). Every CPU cycle matters.

// -----------------------------------------------------------------------------
// Flag Manipulation
// -----------------------------------------------------------------------------

// Benchmark processor status flag manipulation (most common CPU operation)
static void BM_LynxCpu_FlagManipulation(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.PS = LynxPSFlags::Reserved | LynxPSFlags::IrqDisable; // 0x24

	for (auto _ : state) {
		cpu.PS |= LynxPSFlags::Carry;
		cpu.PS &= ~LynxPSFlags::Zero;
		cpu.PS |= LynxPSFlags::Negative;
		cpu.PS &= ~LynxPSFlags::Overflow;
		benchmark::DoNotOptimize(cpu.PS);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_LynxCpu_FlagManipulation);

// Benchmark Zero/Negative flag setting (called on virtually every ALU op)
static void BM_LynxCpu_SetZeroNeg(benchmark::State& state) {
	LynxCpuState cpu = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpu.PS &= ~(LynxPSFlags::Zero | LynxPSFlags::Negative);
		if (value == 0) cpu.PS |= LynxPSFlags::Zero;
		if (value & 0x80) cpu.PS |= LynxPSFlags::Negative;
		benchmark::DoNotOptimize(cpu.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_SetZeroNeg);

// Benchmark branchless Zero/Negative flag setting (optimization comparison)
static void BM_LynxCpu_SetZeroNeg_Branchless(benchmark::State& state) {
	LynxCpuState cpu = {};
	uint8_t value = 0;

	for (auto _ : state) {
		cpu.PS &= ~(LynxPSFlags::Zero | LynxPSFlags::Negative);
		cpu.PS |= (value == 0) ? LynxPSFlags::Zero : 0;
		cpu.PS |= (value & 0x80); // Negative is bit 7, same position
		benchmark::DoNotOptimize(cpu.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_SetZeroNeg_Branchless);

// -----------------------------------------------------------------------------
// IRQ Handling
// -----------------------------------------------------------------------------

// Benchmark IRQ Break flag masking (regression: operator precedence fix)
static void BM_LynxCpu_IrqPsMask(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.PS = 0x3C; // I, D, B, R set

	for (auto _ : state) {
		// Correct: clear Break, set Reserved when pushing during IRQ
		uint8_t pushed = (cpu.PS & ~LynxPSFlags::Break) | LynxPSFlags::Reserved;
		benchmark::DoNotOptimize(pushed);
		cpu.PS ^= 0x01; // varying input
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_IrqPsMask);

// Benchmark IRQ pending check (hot path — checked every instruction)
static void BM_LynxCpu_IrqPendingCheck(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.PS = LynxPSFlags::Reserved; // I flag clear => IRQs enabled
	uint8_t irqPending = 0;

	for (auto _ : state) {
		bool shouldIrq = (irqPending != 0) && !(cpu.PS & LynxPSFlags::IrqDisable);
		benchmark::DoNotOptimize(shouldIrq);
		irqPending ^= LynxIrqSource::Timer0; // toggle
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_IrqPendingCheck);

// -----------------------------------------------------------------------------
// Stack Operations
// -----------------------------------------------------------------------------

// Benchmark stack push/pop (65C02 stack is at $0100-$01FF)
static void BM_LynxCpu_StackPushPop(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.SP = 0xFF;
	std::array<uint8_t, 256> stack{};

	for (auto _ : state) {
		// Push PC high, PC low, PS (interrupt sequence)
		stack[cpu.SP--] = 0x80;           // PCH
		stack[cpu.SP--] = 0x00;           // PCL
		stack[cpu.SP--] = cpu.PS;         // status
		// Pop PS, PC low, PC high (RTI sequence)
		cpu.PS = stack[++cpu.SP];
		uint8_t pcl = stack[++cpu.SP];
		uint8_t pch = stack[++cpu.SP];
		cpu.PC = (pch << 8) | pcl;
		benchmark::DoNotOptimize(cpu.PC);
		benchmark::DoNotOptimize(cpu.PS);
	}
	state.SetItemsProcessed(state.iterations() * 6); // 6 stack ops
}
BENCHMARK(BM_LynxCpu_StackPushPop);

// -----------------------------------------------------------------------------
// Addressing Modes
// -----------------------------------------------------------------------------

// Benchmark Zero Page addressing (most common on 65C02)
static void BM_LynxCpu_AddrMode_ZeroPage(benchmark::State& state) {
	uint8_t operand = 0x42;

	for (auto _ : state) {
		uint16_t addr = operand;
		benchmark::DoNotOptimize(addr);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_ZeroPage);

// Benchmark Zero Page,X addressing with wraparound
static void BM_LynxCpu_AddrMode_ZeroPageX(benchmark::State& state) {
	uint8_t operand = 0x42;
	uint8_t X = 0x10;

	for (auto _ : state) {
		uint16_t addr = static_cast<uint8_t>(operand + X); // wraps in ZP
		benchmark::DoNotOptimize(addr);
		operand++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_ZeroPageX);

// Benchmark Absolute addressing
static void BM_LynxCpu_AddrMode_Absolute(benchmark::State& state) {
	uint8_t lo = 0x00;
	uint8_t hi = 0xFC;

	for (auto _ : state) {
		uint16_t addr = lo | (hi << 8);
		benchmark::DoNotOptimize(addr);
		lo++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_Absolute);

// Benchmark Absolute,X with page crossing detection
static void BM_LynxCpu_AddrMode_AbsoluteX_PageCross(benchmark::State& state) {
	uint16_t base = 0xFC80;
	uint8_t X = 0;

	for (auto _ : state) {
		uint16_t addr = base + X;
		bool pageCross = ((base ^ addr) & 0xFF00) != 0;
		benchmark::DoNotOptimize(addr);
		benchmark::DoNotOptimize(pageCross);
		X++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_AbsoluteX_PageCross);

// Benchmark (Indirect),Y post-indexed addressing (sprite chain traversal pattern)
static void BM_LynxCpu_AddrMode_IndirectY(benchmark::State& state) {
	std::array<uint8_t, 256> zeroPage{};
	// Set up pointer at ZP $80 -> $2000
	zeroPage[0x80] = 0x00;
	zeroPage[0x81] = 0x20;
	uint8_t Y = 0;

	for (auto _ : state) {
		uint16_t ptr = zeroPage[0x80] | (zeroPage[0x81] << 8);
		uint16_t addr = ptr + Y;
		bool pageCross = ((ptr ^ addr) & 0xFF00) != 0;
		benchmark::DoNotOptimize(addr);
		benchmark::DoNotOptimize(pageCross);
		Y++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_IndirectY);

// 65C02-specific: (ZP) indirect without index
static void BM_LynxCpu_AddrMode_ZeroPageIndirect(benchmark::State& state) {
	std::array<uint8_t, 256> zeroPage{};
	zeroPage[0x80] = 0x00;
	zeroPage[0x81] = 0x20;
	uint8_t zpAddr = 0x80;

	for (auto _ : state) {
		uint16_t addr = zeroPage[zpAddr] | (zeroPage[static_cast<uint8_t>(zpAddr + 1)] << 8);
		benchmark::DoNotOptimize(addr);
		zpAddr += 2;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_AddrMode_ZeroPageIndirect);

// -----------------------------------------------------------------------------
// ALU Operations
// -----------------------------------------------------------------------------

// Benchmark ADC (binary mode) — the most complex flag-setting operation
static void BM_LynxCpu_ADC_Binary(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.A = 0x50;
	cpu.PS = LynxPSFlags::Reserved;
	uint8_t value = 0;

	for (auto _ : state) {
		uint16_t sum = cpu.A + value + (cpu.PS & LynxPSFlags::Carry ? 1 : 0);
		cpu.PS &= ~(LynxPSFlags::Carry | LynxPSFlags::Zero | LynxPSFlags::Overflow | LynxPSFlags::Negative);
		if (sum > 0xFF) cpu.PS |= LynxPSFlags::Carry;
		if ((sum & 0xFF) == 0) cpu.PS |= LynxPSFlags::Zero;
		if (((cpu.A ^ sum) & (value ^ sum) & 0x80) != 0) cpu.PS |= LynxPSFlags::Overflow;
		if (sum & 0x80) cpu.PS |= LynxPSFlags::Negative;
		cpu.A = static_cast<uint8_t>(sum);
		benchmark::DoNotOptimize(cpu.A);
		benchmark::DoNotOptimize(cpu.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_ADC_Binary);

// Benchmark ADC (BCD/decimal mode) — 65C02 supports decimal mode
static void BM_LynxCpu_ADC_Decimal(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.A = 0x25; // BCD 25
	cpu.PS = LynxPSFlags::Reserved | LynxPSFlags::Decimal;
	uint8_t value = 0x19; // BCD 19

	for (auto _ : state) {
		uint8_t carry = (cpu.PS & LynxPSFlags::Carry) ? 1 : 0;
		uint8_t lo = (cpu.A & 0x0F) + (value & 0x0F) + carry;
		if (lo > 9) lo += 6;
		uint8_t hi = (cpu.A >> 4) + (value >> 4) + (lo > 0x0F ? 1 : 0);
		lo &= 0x0F;
		if (hi > 9) hi += 6;
		cpu.PS &= ~(LynxPSFlags::Carry | LynxPSFlags::Zero);
		if (hi > 0x0F) cpu.PS |= LynxPSFlags::Carry;
		cpu.A = (hi << 4) | lo;
		if (cpu.A == 0) cpu.PS |= LynxPSFlags::Zero;
		benchmark::DoNotOptimize(cpu.A);
		benchmark::DoNotOptimize(cpu.PS);
		value = static_cast<uint8_t>((value + 1) & 0x99);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_ADC_Decimal);

// Benchmark CMP register comparison (frequent: sprite loop counters)
static void BM_LynxCpu_CmpRegister(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.A = 0x80;
	uint8_t value = 0;

	for (auto _ : state) {
		uint16_t result = cpu.A - value;
		cpu.PS &= ~(LynxPSFlags::Carry | LynxPSFlags::Zero | LynxPSFlags::Negative);
		if (cpu.A >= value) cpu.PS |= LynxPSFlags::Carry;
		if ((result & 0xFF) == 0) cpu.PS |= LynxPSFlags::Zero;
		if (result & 0x80) cpu.PS |= LynxPSFlags::Negative;
		benchmark::DoNotOptimize(cpu.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_CmpRegister);

// Benchmark BIT instruction (65C02 version with immediate mode)
static void BM_LynxCpu_BitTest(benchmark::State& state) {
	LynxCpuState cpu = {};
	cpu.A = 0x55;
	uint8_t value = 0;

	for (auto _ : state) {
		uint8_t result = cpu.A & value;
		cpu.PS &= ~(LynxPSFlags::Zero | LynxPSFlags::Overflow | LynxPSFlags::Negative);
		if (result == 0) cpu.PS |= LynxPSFlags::Zero;
		cpu.PS |= (value & 0xC0); // V and N from memory operand
		benchmark::DoNotOptimize(cpu.PS);
		value++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_BitTest);

// -----------------------------------------------------------------------------
// Branch Prediction Pattern
// -----------------------------------------------------------------------------

// Benchmark branch taken (typical game loop: branch always taken)
static void BM_LynxCpu_Branch_AlwaysTaken(benchmark::State& state) {
	uint16_t pc = 0x1000;
	int8_t offset = -10; // tight loop

	for (auto _ : state) {
		uint16_t newPc = pc + 2 + offset;
		bool pageCross = ((pc + 2) ^ newPc) & 0xFF00;
		// Extra cycle for page cross
		uint32_t cycles = 3 + (pageCross ? 1 : 0);
		benchmark::DoNotOptimize(newPc);
		benchmark::DoNotOptimize(cycles);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_Branch_AlwaysTaken);

// Benchmark branch with varying taken/not-taken (realistic game code)
static void BM_LynxCpu_Branch_Mixed(benchmark::State& state) {
	uint16_t pc = 0x1000;
	int8_t offset = 5;
	uint8_t counter = 0;

	for (auto _ : state) {
		bool taken = (counter & 0x03) != 0; // 75% taken
		uint32_t cycles = 2; // not taken
		if (taken) {
			uint16_t newPc = pc + 2 + offset;
			bool pageCross = ((pc + 2) ^ newPc) & 0xFF00;
			cycles = 3 + (pageCross ? 1 : 0);
			benchmark::DoNotOptimize(newPc);
		}
		benchmark::DoNotOptimize(cycles);
		counter++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCpu_Branch_Mixed);
