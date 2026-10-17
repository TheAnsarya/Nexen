#include "pch.h"
#include <array>
#include "GBA/GbaTypes.h"

// =============================================================================
// GBA ARM7TDMI CPU State Benchmarks
// =============================================================================
// The GBA uses an ARM7TDMI processor which supports both 32-bit ARM mode and
// 16-bit Thumb mode. Key features: 3-stage pipeline, conditional execution,
// barrel shifter, and fast multiply.

// -----------------------------------------------------------------------------
// ARM Mode Flag Operations
// -----------------------------------------------------------------------------

// Benchmark CPSR flag manipulation
static void BM_GbaArm_FlagManipulation(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Mode = GbaCpuMode::User;
	cpsr.Negative = false;
	cpsr.Zero = false;
	cpsr.Carry = false;
	cpsr.Overflow = false;
	
	for (auto _ : state) {
		cpsr.Negative = true;
		cpsr.Zero = false;
		cpsr.Carry = true;
		cpsr.Overflow = false;
		benchmark::DoNotOptimize(cpsr);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbaArm_FlagManipulation);

// Benchmark CPSR to uint32 conversion (common for MSR/MRS instructions)
static void BM_GbaArm_CpsrToInt32(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Mode = GbaCpuMode::User;
	cpsr.Negative = true;
	cpsr.Zero = false;
	cpsr.Carry = true;
	cpsr.Overflow = false;
	cpsr.IrqDisable = true;
	cpsr.FiqDisable = false;
	cpsr.Thumb = false;
	
	for (auto _ : state) {
		uint32_t value = cpsr.ToInt32();
		benchmark::DoNotOptimize(value);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_CpsrToInt32);

// Benchmark condition code checking (ARM conditional execution)
static void BM_GbaArm_ConditionCheck(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Negative = false;
	cpsr.Zero = true;
	cpsr.Carry = true;
	cpsr.Overflow = false;
	uint32_t condCode = 0;
	
	for (auto _ : state) {
		bool execute = false;
		switch (condCode & 0xF) {
			case 0x0: execute = cpsr.Zero; break;                    // EQ
			case 0x1: execute = !cpsr.Zero; break;                   // NE
			case 0x2: execute = cpsr.Carry; break;                   // CS
			case 0x3: execute = !cpsr.Carry; break;                  // CC
			case 0x4: execute = cpsr.Negative; break;                // MI
			case 0x5: execute = !cpsr.Negative; break;               // PL
			case 0x6: execute = cpsr.Overflow; break;                // VS
			case 0x7: execute = !cpsr.Overflow; break;               // VC
			case 0x8: execute = cpsr.Carry && !cpsr.Zero; break;     // HI
			case 0x9: execute = !cpsr.Carry || cpsr.Zero; break;     // LS
			case 0xA: execute = cpsr.Negative == cpsr.Overflow; break; // GE
			case 0xB: execute = cpsr.Negative != cpsr.Overflow; break; // LT
			case 0xC: execute = !cpsr.Zero && (cpsr.Negative == cpsr.Overflow); break; // GT
			case 0xD: execute = cpsr.Zero || (cpsr.Negative != cpsr.Overflow); break;  // LE
			case 0xE: execute = true; break;                         // AL
			case 0xF: execute = false; break;                        // NV (reserved)
		}
		benchmark::DoNotOptimize(execute);
		condCode++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ConditionCheck);

// -----------------------------------------------------------------------------
// Barrel Shifter Operations
// -----------------------------------------------------------------------------

// Benchmark LSL (Logical Shift Left)
static void BM_GbaArm_ShiftLsl(benchmark::State& state) {
	uint32_t value = 0x12345678;
	uint8_t shift = 0;
	bool carry = false;
	
	for (auto _ : state) {
		uint32_t result;
		if (shift == 0) {
			result = value;
		} else if (shift < 32) {
			carry = (value >> (32 - shift)) & 1;
			result = value << shift;
		} else if (shift == 32) {
			carry = value & 1;
			result = 0;
		} else {
			carry = false;
			result = 0;
		}
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(carry);
		shift = (shift + 1) & 0x3F;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ShiftLsl);

// Benchmark LSR (Logical Shift Right)
static void BM_GbaArm_ShiftLsr(benchmark::State& state) {
	uint32_t value = 0x12345678;
	uint8_t shift = 1;
	bool carry = false;
	
	for (auto _ : state) {
		uint32_t result;
		if (shift == 0) {
			result = value;
		} else if (shift < 32) {
			carry = (value >> (shift - 1)) & 1;
			result = value >> shift;
		} else if (shift == 32) {
			carry = (value >> 31) & 1;
			result = 0;
		} else {
			carry = false;
			result = 0;
		}
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(carry);
		shift = ((shift + 1) & 0x3F) | 1;  // Keep non-zero
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ShiftLsr);

// Benchmark ASR (Arithmetic Shift Right)
static void BM_GbaArm_ShiftAsr(benchmark::State& state) {
	uint32_t value = 0x80000000;  // Negative number
	uint8_t shift = 1;
	bool carry = false;
	
	for (auto _ : state) {
		uint32_t result;
		if (shift == 0) {
			result = value;
		} else if (shift < 32) {
			carry = (value >> (shift - 1)) & 1;
			result = static_cast<uint32_t>(static_cast<int32_t>(value) >> shift);
		} else {
			carry = (value >> 31) & 1;
			result = (value & 0x80000000) ? 0xFFFFFFFF : 0;
		}
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(carry);
		shift = ((shift + 1) & 0x3F) | 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ShiftAsr);

// Benchmark ROR (Rotate Right)
static void BM_GbaArm_ShiftRor(benchmark::State& state) {
	uint32_t value = 0x12345678;
	uint8_t shift = 1;
	bool carry = false;
	
	for (auto _ : state) {
		uint32_t result;
		uint8_t s = shift & 0x1F;
		if (s == 0) {
			result = value;
			carry = (value >> 31) & 1;
		} else {
			result = (value >> s) | (value << (32 - s));
			carry = (value >> (s - 1)) & 1;
		}
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(carry);
		shift = ((shift + 1) & 0x1F) | 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ShiftRor);

// -----------------------------------------------------------------------------
// ALU Operations
// -----------------------------------------------------------------------------

// Benchmark ADD with flags
static void BM_GbaArm_Add(benchmark::State& state) {
	uint32_t op1 = 0x7FFFFFFF;
	uint32_t op2 = 1;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint64_t result64 = static_cast<uint64_t>(op1) + op2;
		uint32_t result = static_cast<uint32_t>(result64);
		
		flags.Negative = (result & 0x80000000) != 0;
		flags.Zero = result == 0;
		flags.Carry = result64 > 0xFFFFFFFF;
		flags.Overflow = (~(op1 ^ op2) & (op1 ^ result) & 0x80000000) != 0;
		
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(flags);
		op1++;
		op2++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_Add);

// Benchmark ADC (Add with Carry)
static void BM_GbaArm_Adc(benchmark::State& state) {
	uint32_t op1 = 0x7FFFFFFF;
	uint32_t op2 = 0;
	bool carryIn = true;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint64_t result64 = static_cast<uint64_t>(op1) + op2 + (carryIn ? 1 : 0);
		uint32_t result = static_cast<uint32_t>(result64);
		
		flags.Negative = (result & 0x80000000) != 0;
		flags.Zero = result == 0;
		flags.Carry = result64 > 0xFFFFFFFF;
		flags.Overflow = (~(op1 ^ op2) & (op1 ^ result) & 0x80000000) != 0;
		
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(flags);
		op1++;
		carryIn = !carryIn;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_Adc);

// Benchmark SUB with flags
static void BM_GbaArm_Sub(benchmark::State& state) {
	uint32_t op1 = 0x80000000;
	uint32_t op2 = 1;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint32_t result = op1 - op2;
		
		flags.Negative = (result & 0x80000000) != 0;
		flags.Zero = result == 0;
		flags.Carry = op1 >= op2;  // No borrow
		flags.Overflow = ((op1 ^ op2) & (op1 ^ result) & 0x80000000) != 0;
		
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(flags);
		op1++;
		op2++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_Sub);

// -----------------------------------------------------------------------------
// Multiply Operations
// -----------------------------------------------------------------------------

// Benchmark MUL (32x32 -> 32)
static void BM_GbaArm_Multiply(benchmark::State& state) {
	uint32_t rm = 0x1234;
	uint32_t rs = 0x5678;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint32_t result = rm * rs;
		flags.Negative = (result & 0x80000000) != 0;
		flags.Zero = result == 0;
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(flags);
		rm++;
		rs++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_Multiply);

// Benchmark UMULL (unsigned 32x32 -> 64)
static void BM_GbaArm_MultiplyLongUnsigned(benchmark::State& state) {
	uint32_t rm = 0xFFFFFFFF;
	uint32_t rs = 0xFFFFFFFF;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint64_t result = static_cast<uint64_t>(rm) * rs;
		uint32_t rdHi = static_cast<uint32_t>(result >> 32);
		uint32_t rdLo = static_cast<uint32_t>(result);
		flags.Negative = (rdHi & 0x80000000) != 0;
		flags.Zero = result == 0;
		benchmark::DoNotOptimize(rdHi);
		benchmark::DoNotOptimize(rdLo);
		benchmark::DoNotOptimize(flags);
		rm--;
		rs--;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_MultiplyLongUnsigned);

// Benchmark SMULL (signed 32x32 -> 64)
static void BM_GbaArm_MultiplyLongSigned(benchmark::State& state) {
	int32_t rm = -12345;
	int32_t rs = 67890;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		int64_t result = static_cast<int64_t>(rm) * rs;
		uint32_t rdHi = static_cast<uint32_t>(result >> 32);
		uint32_t rdLo = static_cast<uint32_t>(result);
		flags.Negative = (rdHi & 0x80000000) != 0;
		flags.Zero = result == 0;
		benchmark::DoNotOptimize(rdHi);
		benchmark::DoNotOptimize(rdLo);
		benchmark::DoNotOptimize(flags);
		rm++;
		rs++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_MultiplyLongSigned);

// -----------------------------------------------------------------------------
// Thumb Mode Operations
// -----------------------------------------------------------------------------

// Benchmark Thumb mode flag check
static void BM_GbaThumb_ModeCheck(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Thumb = true;
	
	for (auto _ : state) {
		bool isThumb = cpsr.Thumb;
		uint32_t pcIncrement = isThumb ? 2 : 4;
		benchmark::DoNotOptimize(isThumb);
		benchmark::DoNotOptimize(pcIncrement);
		cpsr.Thumb = !cpsr.Thumb;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaThumb_ModeCheck);

// Benchmark Thumb high register access (R8-R15)
static void BM_GbaThumb_HighRegisterAccess(benchmark::State& state) {
	GbaCpuState cpuState = {};
	for (int i = 0; i < 16; i++) cpuState.R[i] = static_cast<uint32_t>(i * 0x1000);
	
	for (auto _ : state) {
		// Thumb MOV Rd, Hs / MOV Hd, Rs patterns
		uint32_t r8 = cpuState.R[8];
		uint32_t r0 = cpuState.R[0];
		cpuState.R[8] = r0;
		cpuState.R[0] = r8;
		benchmark::DoNotOptimize(cpuState.R[0]);
		benchmark::DoNotOptimize(cpuState.R[8]);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbaThumb_HighRegisterAccess);

// Benchmark Thumb ADD/SUB small immediate (3-bit)
static void BM_GbaThumb_AddSubImm3(benchmark::State& state) {
	uint32_t rd = 100;
	uint8_t imm3 = 0;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		uint32_t result = rd + imm3;
		flags.Negative = (result & 0x80000000) != 0;
		flags.Zero = result == 0;
		flags.Carry = result < rd;  // Simplified
		benchmark::DoNotOptimize(result);
		benchmark::DoNotOptimize(flags);
		imm3 = (imm3 + 1) & 0x7;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaThumb_AddSubImm3);

// Benchmark Thumb MOV/CMP/ADD/SUB immediate (8-bit)
static void BM_GbaThumb_MovCmpAddSubImm8(benchmark::State& state) {
	uint32_t rd = 0;
	uint8_t imm8 = 0;
	GbaCpuFlags flags = {};
	
	for (auto _ : state) {
		// MOV Rd, #imm8
		rd = imm8;
		flags.Negative = false;
		flags.Zero = rd == 0;
		benchmark::DoNotOptimize(rd);
		benchmark::DoNotOptimize(flags);
		imm8++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaThumb_MovCmpAddSubImm8);

// -----------------------------------------------------------------------------
// Pipeline Operations
// -----------------------------------------------------------------------------

// Benchmark pipeline state update
static void BM_GbaArm_PipelineUpdate(benchmark::State& state) {
	GbaCpuPipeline pipeline = {};
	pipeline.Fetch.Address = 0x08000000;
	pipeline.Fetch.OpCode = 0xE3A00001;
	pipeline.Decode.Address = 0x08000004;
	pipeline.Decode.OpCode = 0xE3A01002;
	pipeline.Execute.Address = 0x08000008;
	pipeline.Execute.OpCode = 0xE0800001;
	
	for (auto _ : state) {
		// Simulate pipeline advance
		pipeline.Execute = pipeline.Decode;
		pipeline.Decode = pipeline.Fetch;
		pipeline.Fetch.Address += 4;
		pipeline.Fetch.OpCode = static_cast<uint32_t>(pipeline.Fetch.Address);  // Simulated fetch
		benchmark::DoNotOptimize(pipeline);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_PipelineUpdate);

// Benchmark pipeline flush (branch taken)
static void BM_GbaArm_PipelineFlush(benchmark::State& state) {
	GbaCpuPipeline pipeline = {};
	uint32_t branchTarget = 0x08001000;
	
	for (auto _ : state) {
		// Pipeline flush on branch
		pipeline.ReloadRequested = true;
		pipeline.Fetch.Address = branchTarget;
		pipeline.Fetch.OpCode = 0;  // Will be fetched
		pipeline.Decode.Address = 0;
		pipeline.Decode.OpCode = 0;
		pipeline.Execute.Address = 0;
		pipeline.Execute.OpCode = 0;
		benchmark::DoNotOptimize(pipeline);
		branchTarget += 4;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_PipelineFlush);

// -----------------------------------------------------------------------------
// Mode Switching
// -----------------------------------------------------------------------------

// Benchmark CPU mode switch (e.g., User -> IRQ)
static void BM_GbaArm_ModeSwitch(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.CPSR.Mode = GbaCpuMode::User;
	for (int i = 0; i < 16; i++) cpuState.R[i] = static_cast<uint32_t>(i * 0x1000);
	
	for (auto _ : state) {
		// Simulate mode switch to IRQ
		GbaCpuMode oldMode = cpuState.CPSR.Mode;
		
		// Save banked registers (simplified)
		if (oldMode == GbaCpuMode::User) {
			cpuState.UserRegs[0] = cpuState.R[8];
			cpuState.UserRegs[1] = cpuState.R[9];
		}
		
		// Switch mode
		cpuState.CPSR.Mode = GbaCpuMode::Irq;
		cpuState.CPSR.IrqDisable = true;
		
		// Load banked registers
		cpuState.R[13] = cpuState.IrqRegs[0];
		cpuState.R[14] = cpuState.IrqRegs[1];
		
		benchmark::DoNotOptimize(cpuState);
		
		// Switch back
		cpuState.CPSR.Mode = GbaCpuMode::User;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ModeSwitch);

// Benchmark ARM/Thumb mode switch (BX instruction)
static void BM_GbaArm_ThumbSwitch(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.CPSR.Thumb = false;
	cpuState.R[15] = 0x08000000;
	uint32_t targetAddr = 0x08001001;  // Bit 0 = 1 means Thumb mode
	
	for (auto _ : state) {
		// BX Rm - Branch and Exchange
		bool newThumbMode = (targetAddr & 1) != 0;
		cpuState.CPSR.Thumb = newThumbMode;
		cpuState.R[15] = targetAddr & ~1u;
		
		benchmark::DoNotOptimize(cpuState.CPSR.Thumb);
		benchmark::DoNotOptimize(cpuState.R[15]);
		
		// Alternate between ARM and Thumb targets
		targetAddr ^= 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_ThumbSwitch);

// -----------------------------------------------------------------------------
// Memory Access Patterns
// -----------------------------------------------------------------------------

// Benchmark LDR/STR address calculation
static void BM_GbaArm_LoadStoreAddr(benchmark::State& state) {
	uint32_t base = 0x03000000;  // IWRAM
	uint32_t offset = 0x100;
	bool preIndex = true;
	bool addOffset = true;
	
	for (auto _ : state) {
		uint32_t addr;
		if (preIndex) {
			addr = addOffset ? (base + offset) : (base - offset);
		} else {
			addr = base;
		}
		benchmark::DoNotOptimize(addr);
		offset = (offset + 4) & 0xFFF;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaArm_LoadStoreAddr);

// Benchmark LDM/STM multiple register transfer
static void BM_GbaArm_LoadStoreMultiple(benchmark::State& state) {
	std::array<uint32_t, 0x1000> memory{};
	GbaCpuState cpuState = {};
	cpuState.R[13] = 0x800;  // SP pointing to middle of memory
	uint16_t regList = 0x40FF;  // R0-R7, LR
	
	for (auto _ : state) {
		// STMFD (push) - count registers and calculate addresses
		int regCount = 0;
		for (int i = 0; i < 16; i++) {
			if (regList & (1 << i)) {
				regCount++;
			}
		}
		
		uint32_t addr = cpuState.R[13] - (regCount * 4);
		
		// Store registers
		for (int i = 0; i < 16; i++) {
			if (regList & (1 << i)) {
				memory[addr / 4] = cpuState.R[i];
				addr += 4;
			}
		}
		
		cpuState.R[13] -= regCount * 4;
		benchmark::DoNotOptimize(cpuState.R[13]);
		benchmark::DoNotOptimize(memory[0x1F0]);
		
		cpuState.R[13] += regCount * 4;  // Reset for next iteration
	}
	state.SetItemsProcessed(state.iterations() * 9);  // 9 registers
}
BENCHMARK(BM_GbaArm_LoadStoreMultiple);

