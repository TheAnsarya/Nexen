#include "pch.h"
#include "Utilities/FastString.h"

// ===== Single Char Writes =====

static void BM_FastString_Write_SingleChar_100(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs;
		for (int i = 0; i < 100; i++) {
			fs.Write('A');
		}
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_FastString_Write_SingleChar_100);

// ===== C-String Writes =====

static void BM_FastString_Write_CString_Short(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs;
		fs.Write("Hello");
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_Write_CString_Short);

static void BM_FastString_Write_CString_Medium(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs;
		fs.Write("LDA $2000,X ; Load accumulator indexed");
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_Write_CString_Medium);

// ===== Delimiter Pattern (common in debugger output) =====

static void BM_FastString_Delimiter_10Items(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs;
		for (int i = 0; i < 10; i++) {
			fs.Delimiter(", ");
			fs.Write("Item");
		}
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations() * 10);
}
BENCHMARK(BM_FastString_Delimiter_10Items);

// ===== Lowercase Mode =====

static void BM_FastString_Lowercase_Off(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs(false);
		fs.Write("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_Lowercase_Off);

static void BM_FastString_Lowercase_On(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs(true);
		fs.Write("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_Lowercase_On);

// ===== Comparison: FastString vs std::string =====

static void BM_FastString_BuildAddress(benchmark::State& state) {
	// Typical debugger address formatting: "$7E:2000"
	for (auto _ : state) {
		FastString fs;
		fs.Write('$');
		fs.Write("7E", 2);
		fs.Write(':');
		fs.Write("2000", 4);
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_BuildAddress);

static void BM_StdString_BuildAddress(benchmark::State& state) {
	// Same operation with std::string for comparison
	for (auto _ : state) {
		std::string s;
		s += '$';
		s += "7E";
		s += ':';
		s += "2000";
		benchmark::DoNotOptimize(s.c_str());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdString_BuildAddress);

// ===== WriteSafe vs Write =====

static void BM_FastString_WriteSafe_100(benchmark::State& state) {
	for (auto _ : state) {
		FastString fs;
		for (int i = 0; i < 100; i++) {
			fs.WriteSafe('A');
		}
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_FastString_WriteSafe_100);

// ===== Reset and Reuse =====

static void BM_FastString_ResetAndReuse(benchmark::State& state) {
	FastString fs;
	for (auto _ : state) {
		fs.Reset();
		fs.Write("Instruction: ");
		fs.Write("LDA $2000");
		benchmark::DoNotOptimize(fs.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_ResetAndReuse);

// ===== Copy from another FastString =====

static void BM_FastString_Write_FastString(benchmark::State& state) {
	FastString source;
	source.Write("Source data to copy over");

	for (auto _ : state) {
		FastString dest;
		dest.Write("Prefix: ");
		dest.Write(source);
		benchmark::DoNotOptimize(dest.ToString());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FastString_Write_FastString);
