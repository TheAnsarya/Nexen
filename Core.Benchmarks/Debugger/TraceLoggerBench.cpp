#include "pch.h"
// TraceLoggerBench.cpp
// Benchmarks proving Phase 16.6 optimizations for trace logging string operations.
// Related issue: #533 (Phase 16.6 perf: eliminate temp string allocs)
//
// Tests: std::string(N, char) temporaries vs append(N, char) / insert()
// These patterns are called per-instruction when trace logging is active.

#include "benchmark/benchmark.h"
#include <string>

// =============================================================================
// BaseTraceLogger: std::string(N, ' ') vs append(N, ' ')
// =============================================================================
// Old pattern: output += std::string(N, ' ')
//   - Allocates a temporary std::string on the heap
//   - Appends it to output (may trigger another allocation)
//   - Destroys the temporary (heap free)
//
// New pattern: output.append(N, ' ')
//   - Directly appends chars to existing buffer — zero temp allocations

// Benchmark: OLD pattern — std::string(N, ' ') + operator+=
static void BM_TraceLogger_PadWithTemp_Small(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(256);
		// Simulate WriteDisassembly indent + alignment (small padding: 4-8 chars)
		output += std::string(4, ' ');       // indent
		output += "LDA $2000,X";
		output += std::string(8, ' ');       // alignment padding
		output += "= $FF";
		output += std::string(6, ' ');       // final alignment
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_PadWithTemp_Small);

// Benchmark: NEW pattern — append(N, ' ')
static void BM_TraceLogger_PadWithAppend_Small(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(256);
		// Same operations using append — zero temporaries
		output.append(4, ' ');               // indent
		output += "LDA $2000,X";
		output.append(8, ' ');               // alignment padding
		output += "= $FF";
		output.append(6, ' ');               // final alignment
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_PadWithAppend_Small);

// Benchmark: OLD pattern — larger padding (trace logging with deep call stacks)
static void BM_TraceLogger_PadWithTemp_Large(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(512);
		// Deep indent (32 chars) + wide alignment (20 chars) + status padding (12 chars)
		output += std::string(32, ' ');
		output += "JSR SubRoutine";
		output += std::string(20, ' ');
		output += "NVmxDIZC";
		output += std::string(12, ' ');
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_PadWithTemp_Large);

// Benchmark: NEW pattern — larger padding with append
static void BM_TraceLogger_PadWithAppend_Large(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(512);
		output.append(32, ' ');
		output += "JSR SubRoutine";
		output.append(20, ' ');
		output += "NVmxDIZC";
		output.append(12, ' ');
		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_PadWithAppend_Large);

// =============================================================================
// WriteIntValue: string prepend with zero-padding
// =============================================================================
// Old: str = std::string(N - str.size(), '0') + str
//   - Creates temp string of '0's
//   - Concatenates temp + str into NEW string
//   - Assigns result back to str (another allocation)
//
// New: str.insert(0, N - str.size(), '0')
//   - Shifts existing content and inserts in-place — one operation

static void BM_TraceLogger_ZeroPad_WithTemp(benchmark::State& state) {
	for (auto _ : state) {
		std::string str = "FF";
		// Pad to 4 chars with leading zeros (old pattern)
		str = std::string(4 - str.size(), '0') + str;
		benchmark::DoNotOptimize(str.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_ZeroPad_WithTemp);

static void BM_TraceLogger_ZeroPad_WithInsert(benchmark::State& state) {
	for (auto _ : state) {
		std::string str = "FF";
		// Pad to 4 chars with leading zeros (new pattern)
		str.insert(0, 4 - str.size(), '0');
		benchmark::DoNotOptimize(str.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_ZeroPad_WithInsert);

// Pad wider value (6-char hex address padded to 8)
static void BM_TraceLogger_ZeroPad8_WithTemp(benchmark::State& state) {
	for (auto _ : state) {
		std::string str = "00FF00";
		str = std::string(8 - str.size(), '0') + str;
		benchmark::DoNotOptimize(str.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_ZeroPad8_WithTemp);

static void BM_TraceLogger_ZeroPad8_WithInsert(benchmark::State& state) {
	for (auto _ : state) {
		std::string str = "00FF00";
		str.insert(0, 8 - str.size(), '0');
		benchmark::DoNotOptimize(str.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_ZeroPad8_WithInsert);

// =============================================================================
// Full trace row simulation (realistic workload)
// =============================================================================
// Simulates a complete trace logger row with PC + disassembly + registers + flags

static void BM_TraceLogger_FullRow_OldPattern(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(256);

		// PC (zero-padded hex)
		std::string pc = "8000";
		if (pc.size() < 6) {
			pc = std::string(6 - pc.size(), '0') + pc;
		}
		output += pc;
		output += "  ";

		// Indent (typical: 4 spaces for 2-deep call stack)
		output += std::string(4, ' ');

		// Disassembly
		output += "LDA $2000,X";

		// Alignment to min width of 30
		if (output.size() < 42) {
			output += std::string(42 - output.size(), ' ');
		}

		// Effective address
		output += " [$002000]";

		// Memory value
		output += "= $FF";

		// Register padding
		std::string regA = "FF";
		regA = std::string(2 - regA.size(), '0') + regA;
		output += " A:" + regA;

		// Status flags padding
		output += " ";
		output += "NVmxDIZC";
		output += std::string(10 - 8, ' ');

		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_FullRow_OldPattern);

static void BM_TraceLogger_FullRow_NewPattern(benchmark::State& state) {
	for (auto _ : state) {
		std::string output;
		output.reserve(256);

		// PC (zero-padded hex)
		std::string pc = "8000";
		if (pc.size() < 6) {
			pc.insert(0, 6 - pc.size(), '0');
		}
		output += pc;
		output += "  ";

		// Indent
		output.append(4, ' ');

		// Disassembly
		output += "LDA $2000,X";

		// Alignment to min width of 30
		if (output.size() < 42) {
			output.append(42 - output.size(), ' ');
		}

		// Effective address
		output += " [$002000]";

		// Memory value
		output += "= $FF";

		// Register padding
		std::string regA = "FF";
		if (regA.size() < 2) {
			regA.insert(0, 2 - regA.size(), '0');
		}
		output += " A:";
		output += regA;

		// Status flags padding
		output += " ";
		output += "NVmxDIZC";
		output.append(10 - 8, ' ');

		benchmark::DoNotOptimize(output.data());
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TraceLogger_FullRow_NewPattern);
