#include "pch.h"
#include "Lynx/Debugger/LynxDisUtils.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Benchmarks for LynxDisUtils — the Lynx (65C02) disassembly utility functions.
///
/// These measure the debugger's hot paths for instruction analysis:
///   - GetOpSize: called for every instruction during disassembly
///   - IsUnconditionalJump / IsConditionalJump: called for control flow analysis
///   - IsJumpToSub / IsReturnInstruction: called for call stack tracking
///   - GetOpName: called for trace logging and disassembly display
/// </summary>

// =============================================================================
// GetOpSize Benchmarks
// =============================================================================

static void BM_LynxDisUtils_GetOpSize_Sequential256(benchmark::State& state) {
	// Measure GetOpSize for all 256 opcodes in sequence
	for (auto _ : state) {
		uint32_t total = 0;
		for (int op = 0; op < 256; op++) {
			total += LynxDisUtils::GetOpSize(static_cast<uint8_t>(op));
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxDisUtils_GetOpSize_Sequential256);

static void BM_LynxDisUtils_GetOpSize_CommonOpcodes(benchmark::State& state) {
	// Measure GetOpSize for commonly-used opcodes in typical Lynx code
	// LDA/STA/LDX/STX/JSR/RTS/JMP/BEQ/BNE/CMP/AND/ORA
	static constexpr uint8_t commonOps[] = {
		0xa9, 0xa5, 0xad, 0xbd, // LDA variants
		0x85, 0x8d, 0x9d,       // STA variants
		0xa2, 0xa0,             // LDX/LDY imm
		0x20, 0x60,             // JSR/RTS
		0x4c, 0x6c,             // JMP variants
		0xf0, 0xd0, 0x90, 0xb0, // Branch instructions
		0xc9, 0xe0, 0xc0,       // CMP/CPX/CPY
		0x29, 0x09, 0x49,       // AND/ORA/EOR
		0x48, 0x68,             // PHA/PLA
		0xe8, 0xca,             // INX/DEX
		0xea,                   // NOP
	};
	static constexpr size_t opCount = sizeof(commonOps) / sizeof(commonOps[0]);

	for (auto _ : state) {
		uint32_t total = 0;
		for (size_t i = 0; i < opCount; i++) {
			total += LynxDisUtils::GetOpSize(commonOps[i]);
		}
		benchmark::DoNotOptimize(total);
	}
	state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(opCount));
}
BENCHMARK(BM_LynxDisUtils_GetOpSize_CommonOpcodes);

// =============================================================================
// Jump Classification Benchmarks
// =============================================================================

static void BM_LynxDisUtils_IsUnconditionalJump_All256(benchmark::State& state) {
	for (auto _ : state) {
		int count = 0;
		for (int op = 0; op < 256; op++) {
			if (LynxDisUtils::IsUnconditionalJump(static_cast<uint8_t>(op))) {
				count++;
			}
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxDisUtils_IsUnconditionalJump_All256);

static void BM_LynxDisUtils_IsConditionalJump_All256(benchmark::State& state) {
	for (auto _ : state) {
		int count = 0;
		for (int op = 0; op < 256; op++) {
			if (LynxDisUtils::IsConditionalJump(static_cast<uint8_t>(op))) {
				count++;
			}
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxDisUtils_IsConditionalJump_All256);

static void BM_LynxDisUtils_FullClassification_All256(benchmark::State& state) {
	// Combined: classify every opcode as unconditional/conditional/sub/return/other
	for (auto _ : state) {
		int unconditional = 0, conditional = 0, sub = 0, ret = 0;
		for (int op = 0; op < 256; op++) {
			uint8_t opcode = static_cast<uint8_t>(op);
			if (LynxDisUtils::IsUnconditionalJump(opcode)) unconditional++;
			if (LynxDisUtils::IsConditionalJump(opcode)) conditional++;
			if (LynxDisUtils::IsJumpToSub(opcode)) sub++;
			if (LynxDisUtils::IsReturnInstruction(opcode)) ret++;
		}
		benchmark::DoNotOptimize(unconditional);
		benchmark::DoNotOptimize(conditional);
		benchmark::DoNotOptimize(sub);
		benchmark::DoNotOptimize(ret);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxDisUtils_FullClassification_All256);

// =============================================================================
// GetOpName Benchmarks
// =============================================================================

static void BM_LynxDisUtils_GetOpName_All256(benchmark::State& state) {
	for (auto _ : state) {
		for (int op = 0; op < 256; op++) {
			auto name = LynxDisUtils::GetOpName(static_cast<uint8_t>(op));
			benchmark::DoNotOptimize(name);
		}
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxDisUtils_GetOpName_All256);
