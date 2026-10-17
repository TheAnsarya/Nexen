#include "pch.h"
#include <array>
#include "GBA/GbaTypes.h"

// =============================================================================
// GBA ARM7TDMI CPU State Benchmarks
// =============================================================================
// The GBA uses an ARM7TDMI processor which supports both 32-bit ARM mode and
// 16-bit Thumb mode. The CPU has a 3-stage pipeline and multiple processor modes.

// -----------------------------------------------------------------------------
// Flag Manipulation Benchmarks
// -----------------------------------------------------------------------------

// Benchmark CPSR flag manipulation
static void BM_GbaCpu_FlagManipulation(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Mode = GbaCpuMode::User;
	cpsr.Thumb = false;
	
	for (auto _ : state) {
		cpsr.Negative = true;
		cpsr.Zero = false;
		cpsr.Carry = true;
		cpsr.Overflow = false;
		benchmark::DoNotOptimize(cpsr);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbaCpu_FlagManipulation);

// Benchmark CPSR to uint32 conversion (common operation)
static void BM_GbaCpu_CpsrToInt32(benchmark::State& state) {
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
BENCHMARK(BM_GbaCpu_CpsrToInt32);

// Benchmark condition code checking (14 possible conditions)
static void BM_GbaCpu_ConditionCheck(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Negative = true;
	cpsr.Zero = false;
	cpsr.Carry = true;
	cpsr.Overflow = false;
	uint8_t condCode = 0;
	
	for (auto _ : state) {
		bool result = false;
		switch (condCode & 0x0F) {
			case 0x0: result = cpsr.Zero; break;                           // EQ
			case 0x1: result = !cpsr.Zero; break;                          // NE
			case 0x2: result = cpsr.Carry; break;                          // CS/HS
			case 0x3: result = !cpsr.Carry; break;                         // CC/LO
			case 0x4: result = cpsr.Negative; break;                       // MI
			case 0x5: result = !cpsr.Negative; break;                      // PL
			case 0x6: result = cpsr.Overflow; break;                       // VS
			case 0x7: result = !cpsr.Overflow; break;                      // VC
			case 0x8: result = cpsr.Carry && !cpsr.Zero; break;            // HI
			case 0x9: result = !cpsr.Carry || cpsr.Zero; break;            // LS
			case 0xA: result = cpsr.Negative == cpsr.Overflow; break;      // GE
			case 0xB: result = cpsr.Negative != cpsr.Overflow; break;      // LT
			case 0xC: result = !cpsr.Zero && (cpsr.Negative == cpsr.Overflow); break; // GT
			case 0xD: result = cpsr.Zero || (cpsr.Negative != cpsr.Overflow); break;  // LE
			case 0xE: result = true; break;                                // AL
			case 0xF: result = false; break;                               // NV (reserved)
		}
		benchmark::DoNotOptimize(result);
		condCode++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ConditionCheck);

// -----------------------------------------------------------------------------
// ARM Mode Instruction Benchmarks
// -----------------------------------------------------------------------------

// Benchmark ARM data processing (ADD with flags)
static void BM_GbaCpu_ARM_DataProcessing_ADD(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 0x40000000;
	cpuState.R[1] = 0x30000000;
	
	for (auto _ : state) {
		uint64_t result64 = static_cast<uint64_t>(cpuState.R[0]) + cpuState.R[1];
		uint32_t result = static_cast<uint32_t>(result64);
		
		// Update flags
		cpuState.CPSR.Negative = (result & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		cpuState.CPSR.Carry = result64 > 0xFFFFFFFF;
		cpuState.CPSR.Overflow = (~(cpuState.R[0] ^ cpuState.R[1]) & (cpuState.R[0] ^ result) & 0x80000000) != 0;
		
		cpuState.R[2] = result;
		benchmark::DoNotOptimize(cpuState.R[2]);
		benchmark::DoNotOptimize(cpuState.CPSR);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_DataProcessing_ADD);

// Benchmark ARM barrel shifter operations
static void BM_GbaCpu_ARM_BarrelShifter_LSL(benchmark::State& state) {
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
BENCHMARK(BM_GbaCpu_ARM_BarrelShifter_LSL);

// Benchmark ARM barrel shifter - Rotate Right
static void BM_GbaCpu_ARM_BarrelShifter_ROR(benchmark::State& state) {
	uint32_t value = 0x12345678;
	uint8_t shift = 0;
	
	for (auto _ : state) {
		uint32_t result;
		uint8_t rotateAmount = shift & 0x1F;
		if (rotateAmount == 0) {
			result = value;
		} else {
			result = (value >> rotateAmount) | (value << (32 - rotateAmount));
		}
		benchmark::DoNotOptimize(result);
		shift++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_BarrelShifter_ROR);

// Benchmark ARM multiply instruction
static void BM_GbaCpu_ARM_Multiply(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 123;
	cpuState.R[1] = 456;
	
	for (auto _ : state) {
		uint32_t result = cpuState.R[0] * cpuState.R[1];
		cpuState.CPSR.Negative = (result & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		cpuState.R[2] = result;
		benchmark::DoNotOptimize(cpuState.R[2]);
		cpuState.R[0]++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_Multiply);

// Benchmark ARM multiply-accumulate
static void BM_GbaCpu_ARM_MultiplyAccumulate(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 123;
	cpuState.R[1] = 456;
	cpuState.R[2] = 1000;
	
	for (auto _ : state) {
		uint32_t result = cpuState.R[0] * cpuState.R[1] + cpuState.R[2];
		cpuState.CPSR.Negative = (result & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		cpuState.R[3] = result;
		benchmark::DoNotOptimize(cpuState.R[3]);
		cpuState.R[0]++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_MultiplyAccumulate);

// Benchmark ARM long multiply (64-bit result)
static void BM_GbaCpu_ARM_MultiplyLong(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 0x12345678;
	cpuState.R[1] = 0x9ABCDEF0;
	
	for (auto _ : state) {
		uint64_t result = static_cast<uint64_t>(cpuState.R[0]) * cpuState.R[1];
		cpuState.R[2] = static_cast<uint32_t>(result);         // Low
		cpuState.R[3] = static_cast<uint32_t>(result >> 32);   // High
		cpuState.CPSR.Negative = (cpuState.R[3] & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		benchmark::DoNotOptimize(cpuState.R[2]);
		benchmark::DoNotOptimize(cpuState.R[3]);
		cpuState.R[0]++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_MultiplyLong);

// Benchmark ARM branch with link
static void BM_GbaCpu_ARM_BranchLink(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[15] = 0x08000000;  // PC
	int32_t offset = 0x100;
	
	for (auto _ : state) {
		// BL instruction: save return address, then branch
		cpuState.R[14] = cpuState.R[15] - 4;  // LR = PC - 4
		cpuState.R[15] = cpuState.R[15] + offset;
		benchmark::DoNotOptimize(cpuState.R[14]);
		benchmark::DoNotOptimize(cpuState.R[15]);
		offset = -offset;  // Alternate direction
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ARM_BranchLink);

// -----------------------------------------------------------------------------
// Thumb Mode Instruction Benchmarks
// -----------------------------------------------------------------------------

// Benchmark Thumb ADD immediate
static void BM_GbaCpu_Thumb_AddImmediate(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 100;
	uint8_t imm8 = 50;
	
	for (auto _ : state) {
		uint32_t result = cpuState.R[0] + imm8;
		cpuState.CPSR.Negative = (result & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		cpuState.CPSR.Carry = result < cpuState.R[0];
		cpuState.R[0] = result;
		benchmark::DoNotOptimize(cpuState.R[0]);
		imm8++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_Thumb_AddImmediate);

// Benchmark Thumb shifted register (LSL, LSR, ASR)
static void BM_GbaCpu_Thumb_ShiftedRegister(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[0] = 0x12345678;
	uint8_t shift = 0;
	
	for (auto _ : state) {
		uint8_t shiftAmount = shift & 0x1F;
		uint32_t result = cpuState.R[0] << shiftAmount;
		cpuState.CPSR.Negative = (result & 0x80000000) != 0;
		cpuState.CPSR.Zero = result == 0;
		if (shiftAmount > 0) {
			cpuState.CPSR.Carry = (cpuState.R[0] >> (32 - shiftAmount)) & 1;
		}
		cpuState.R[1] = result;
		benchmark::DoNotOptimize(cpuState.R[1]);
		shift++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_Thumb_ShiftedRegister);

// Benchmark Thumb high register operations (MOV, ADD, CMP with R8-R15)
static void BM_GbaCpu_Thumb_HighRegisterOp(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[8] = 0x12345678;
	cpuState.R[0] = 0;
	
	for (auto _ : state) {
		// MOV Rd, Hs (move high register to low)
		cpuState.R[0] = cpuState.R[8];
		// ADD Rd, Hs
		cpuState.R[1] = cpuState.R[0] + cpuState.R[8];
		benchmark::DoNotOptimize(cpuState.R[0]);
		benchmark::DoNotOptimize(cpuState.R[1]);
		cpuState.R[8]++;
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbaCpu_Thumb_HighRegisterOp);

// Benchmark Thumb push/pop multiple
static void BM_GbaCpu_Thumb_PushPop(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[13] = 0x03007F00;  // SP
	cpuState.R[0] = 0x11111111;
	cpuState.R[1] = 0x22222222;
	cpuState.R[2] = 0x33333333;
	cpuState.R[14] = 0x08001234;  // LR
	std::array<uint8_t, 0x100> stack{};
	
	for (auto _ : state) {
		// PUSH {R0-R2, LR}
		uint32_t sp = cpuState.R[13];
		sp -= 4; std::memcpy(&stack[sp & 0xFF], &cpuState.R[14], 4);
		sp -= 4; std::memcpy(&stack[sp & 0xFF], &cpuState.R[2], 4);
		sp -= 4; std::memcpy(&stack[sp & 0xFF], &cpuState.R[1], 4);
		sp -= 4; std::memcpy(&stack[sp & 0xFF], &cpuState.R[0], 4);
		cpuState.R[13] = sp;
		
		// POP {R0-R2, PC}
		std::memcpy(&cpuState.R[0], &stack[sp & 0xFF], 4); sp += 4;
		std::memcpy(&cpuState.R[1], &stack[sp & 0xFF], 4); sp += 4;
		std::memcpy(&cpuState.R[2], &stack[sp & 0xFF], 4); sp += 4;
		std::memcpy(&cpuState.R[15], &stack[sp & 0xFF], 4); sp += 4;
		cpuState.R[13] = sp;
		
		benchmark::DoNotOptimize(cpuState.R[13]);
	}
	state.SetItemsProcessed(state.iterations() * 8);  // 8 memory operations
}
BENCHMARK(BM_GbaCpu_Thumb_PushPop);

// Benchmark Thumb conditional branch
static void BM_GbaCpu_Thumb_ConditionalBranch(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[15] = 0x08000100;
	cpuState.CPSR.Zero = true;
	int8_t offset = 10;
	
	for (auto _ : state) {
		// BEQ offset
		if (cpuState.CPSR.Zero) {
			cpuState.R[15] = static_cast<uint32_t>(cpuState.R[15] + (offset << 1));
		}
		benchmark::DoNotOptimize(cpuState.R[15]);
		offset = -offset;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_Thumb_ConditionalBranch);

// Benchmark Thumb BL (long branch with link - 2 instructions)
static void BM_GbaCpu_Thumb_BranchLinkLong(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[15] = 0x08000100;
	uint32_t targetOffset = 0x1000;
	
	for (auto _ : state) {
		// First instruction: LR = PC + (offset_high << 12)
		cpuState.R[14] = cpuState.R[15] + ((targetOffset >> 11) << 12);
		// Second instruction: temp = LR + (offset_low << 1), LR = PC | 1, PC = temp
		uint32_t temp = cpuState.R[14] + ((targetOffset & 0x7FF) << 1);
		cpuState.R[14] = (cpuState.R[15] - 2) | 1;
		cpuState.R[15] = temp;
		benchmark::DoNotOptimize(cpuState.R[14]);
		benchmark::DoNotOptimize(cpuState.R[15]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_Thumb_BranchLinkLong);

// -----------------------------------------------------------------------------
// ARM/Thumb Mode Switching Benchmarks
// -----------------------------------------------------------------------------

// Benchmark mode switch ARM → Thumb (BX instruction)
static void BM_GbaCpu_ModeSwitch_ArmToThumb(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.CPSR.Thumb = false;
	cpuState.R[0] = 0x08000101;  // Odd address = Thumb mode
	
	for (auto _ : state) {
		// BX Rm
		cpuState.CPSR.Thumb = (cpuState.R[0] & 1) != 0;
		cpuState.R[15] = cpuState.R[0] & ~1u;
		benchmark::DoNotOptimize(cpuState.CPSR.Thumb);
		benchmark::DoNotOptimize(cpuState.R[15]);
		cpuState.R[0] ^= 1;  // Toggle between ARM/Thumb
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ModeSwitch_ArmToThumb);

// Benchmark mode switch Thumb → ARM
static void BM_GbaCpu_ModeSwitch_ThumbToArm(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.CPSR.Thumb = true;
	cpuState.R[0] = 0x08000100;  // Even address = ARM mode
	
	for (auto _ : state) {
		// BX Rm
		cpuState.CPSR.Thumb = (cpuState.R[0] & 1) != 0;
		cpuState.R[15] = cpuState.R[0] & ~1u;
		benchmark::DoNotOptimize(cpuState.CPSR.Thumb);
		benchmark::DoNotOptimize(cpuState.R[15]);
		cpuState.R[0] ^= 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_ModeSwitch_ThumbToArm);

// -----------------------------------------------------------------------------
// Pipeline Benchmarks
// -----------------------------------------------------------------------------

// Benchmark pipeline state update
static void BM_GbaCpu_PipelineUpdate(benchmark::State& state) {
	GbaCpuPipeline pipe = {};
	pipe.Fetch.Address = 0x08000000;
	pipe.Fetch.OpCode = 0xE3A00000;
	pipe.Decode.Address = 0x08000004;
	pipe.Decode.OpCode = 0xE3A01001;
	pipe.Execute.Address = 0x08000008;
	pipe.Execute.OpCode = 0xE0800001;
	
	for (auto _ : state) {
		// Pipeline advance
		pipe.Execute = pipe.Decode;
		pipe.Decode = pipe.Fetch;
		pipe.Fetch.Address += 4;
		pipe.Fetch.OpCode = 0xE1A00000;  // MOV R0, R0 (NOP)
		benchmark::DoNotOptimize(pipe);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_PipelineUpdate);

// Benchmark pipeline flush (branch taken)
static void BM_GbaCpu_PipelineFlush(benchmark::State& state) {
	GbaCpuPipeline pipe = {};
	pipe.ReloadRequested = false;
	uint32_t newPC = 0x08001000;
	
	for (auto _ : state) {
		// Branch causes pipeline flush
		pipe.ReloadRequested = true;
		pipe.Fetch.Address = newPC;
		pipe.Fetch.OpCode = 0;
		pipe.Decode.Address = 0;
		pipe.Decode.OpCode = 0;
		pipe.Execute.Address = 0;
		pipe.Execute.OpCode = 0;
		benchmark::DoNotOptimize(pipe);
		newPC += 4;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaCpu_PipelineFlush);

// -----------------------------------------------------------------------------
// Processor Mode Benchmarks
// -----------------------------------------------------------------------------

// Benchmark CPU mode check
static void BM_GbaCpu_ModeCheck(benchmark::State& state) {
	GbaCpuFlags cpsr = {};
	cpsr.Mode = GbaCpuMode::User;
	
	for (auto _ : state) {
		bool isPrivileged = cpsr.Mode != GbaCpuMode::User;
		bool canAccessSpsr = cpsr.Mode != GbaCpuMode::User && cpsr.Mode != GbaCpuMode::System;
		benchmark::DoNotOptimize(isPrivileged);
		benchmark::DoNotOptimize(canAccessSpsr);
		// Cycle through modes
		switch (cpsr.Mode) {
			case GbaCpuMode::User: cpsr.Mode = GbaCpuMode::Fiq; break;
			case GbaCpuMode::Fiq: cpsr.Mode = GbaCpuMode::Irq; break;
			case GbaCpuMode::Irq: cpsr.Mode = GbaCpuMode::Supervisor; break;
			case GbaCpuMode::Supervisor: cpsr.Mode = GbaCpuMode::Abort; break;
			case GbaCpuMode::Abort: cpsr.Mode = GbaCpuMode::Undefined; break;
			case GbaCpuMode::Undefined: cpsr.Mode = GbaCpuMode::System; break;
			case GbaCpuMode::System: cpsr.Mode = GbaCpuMode::User; break;
		}
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbaCpu_ModeCheck);

// Benchmark banked register access simulation
static void BM_GbaCpu_BankedRegisterAccess(benchmark::State& state) {
	GbaCpuState cpuState = {};
	cpuState.R[13] = 0x03007F00;  // User SP
	cpuState.R[14] = 0x08000000;  // User LR
	cpuState.IrqRegs[0] = 0x03007E00;  // IRQ SP
	cpuState.IrqRegs[1] = 0x08001000;  // IRQ LR
	cpuState.CPSR.Mode = GbaCpuMode::User;
	
	for (auto _ : state) {
		// Simulate mode switch User → IRQ (save and restore banked regs)
		uint32_t savedSP = cpuState.R[13];
		uint32_t savedLR = cpuState.R[14];
		cpuState.UserRegs[5] = savedSP;  // Bank user SP
		cpuState.UserRegs[6] = savedLR;  // Bank user LR
		cpuState.R[13] = cpuState.IrqRegs[0];
		cpuState.R[14] = cpuState.IrqRegs[1];
		cpuState.CPSR.Mode = GbaCpuMode::Irq;
		
		// Switch back
		cpuState.IrqRegs[0] = cpuState.R[13];
		cpuState.IrqRegs[1] = cpuState.R[14];
		cpuState.R[13] = cpuState.UserRegs[5];
		cpuState.R[14] = cpuState.UserRegs[6];
		cpuState.CPSR.Mode = GbaCpuMode::User;
		
		benchmark::DoNotOptimize(cpuState.R[13]);
		benchmark::DoNotOptimize(cpuState.R[14]);
	}
	state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_GbaCpu_BankedRegisterAccess);

