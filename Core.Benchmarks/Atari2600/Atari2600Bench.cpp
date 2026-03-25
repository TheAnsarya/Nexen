#include "pch.h"
#include "Shared/Emulator.h"
#include "Utilities/VirtualFile.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"

namespace {
	vector<uint8_t> BuildNopRom(size_t size) {
		return vector<uint8_t>(size, 0xea);
	}

	vector<Atari2600BaselineRomCase> BuildCompatibilityCorpus() {
		vector<Atari2600BaselineRomCase> corpus;
		corpus.push_back({"bench-f8.a26", BuildNopRom(8192)});
		corpus.push_back({"bench-f6.a26", BuildNopRom(16384)});
		corpus.push_back({"bench-3f.a26", BuildNopRom(8192)});
		return corpus;
	}

	bool LoadNopRom(Atari2600Console& console, const string& name, size_t size) {
		vector<uint8_t> rom = BuildNopRom(size);
		VirtualFile romFile(rom.data(), rom.size(), name);
		return console.LoadRom(romFile) == LoadRomResult::Success;
	}

	bool LoadF8MapperBenchRom(Atari2600Console& console) {
		vector<uint8_t> rom(8192, 0x11);
		std::fill(rom.begin() + 0x1000, rom.begin() + 0x2000, 0x22);
		VirtualFile romFile(rom.data(), rom.size(), "bench-mapper-f8.a26");
		return console.LoadRom(romFile) == LoadRomResult::Success;
	}
}

static void BM_Atari2600_RunFrame_Nop4K(benchmark::State& state) {
	Emulator emu;
	Atari2600Console console(&emu);
	if (!LoadNopRom(console, "bench-runframe.a26", 4096)) {
		state.SkipWithError("failed to load Atari2600 benchmark ROM");
		return;
	}

	for (auto _ : state) {
		console.Reset();
		console.RunFrame();
		Atari2600FrameStepSummary summary = console.GetLastFrameSummary();
		benchmark::DoNotOptimize(summary.CpuCyclesThisFrame);
		benchmark::DoNotOptimize(summary.FrameCount);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Atari2600_RunFrame_Nop4K);

static void BM_Atari2600_MapperF8_HotspotReads(benchmark::State& state) {
	Emulator emu;
	Atari2600Console console(&emu);
	if (!LoadF8MapperBenchRom(console)) {
		state.SkipWithError("failed to load Atari2600 F8 benchmark ROM");
		return;
	}

	uint8_t checksum = 0;
	for (auto _ : state) {
		checksum ^= console.DebugReadCartridge(0x1ff8);
		checksum ^= console.DebugReadCartridge(0x1000);
		checksum ^= console.DebugReadCartridge(0x1ff9);
		checksum ^= console.DebugReadCartridge(0x1000);
	}

	benchmark::DoNotOptimize(checksum);
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_Atari2600_MapperF8_HotspotReads);

static void BM_Atari2600_CompatibilityMatrix_Corpus(benchmark::State& state) {
	Emulator emu;
	Atari2600Console console(&emu);
	vector<Atari2600BaselineRomCase> corpus = BuildCompatibilityCorpus();

	for (auto _ : state) {
		Atari2600CompatibilityMatrixResult result = Atari2600SmokeHarness::RunCompatibilityMatrix(console, corpus);
		benchmark::DoNotOptimize(result.PassCount);
		benchmark::DoNotOptimize(result.Digest.data());
	}

	state.SetItemsProcessed(state.iterations() * corpus.size());
	state.SetLabel("compatibility-matrix");
}
BENCHMARK(BM_Atari2600_CompatibilityMatrix_Corpus);

static void BM_Atari2600_PerformanceGate_Corpus(benchmark::State& state) {
	Emulator emu;
	Atari2600Console console(&emu);
	vector<Atari2600BaselineRomCase> corpus = BuildCompatibilityCorpus();

	for (auto _ : state) {
		Atari2600PerformanceGateResult result = Atari2600SmokeHarness::RunPerformanceGate(console, corpus, 5000000);
		benchmark::DoNotOptimize(result.PassCount);
		benchmark::DoNotOptimize(result.Digest.data());
	}

	state.SetItemsProcessed(state.iterations() * corpus.size());
	state.SetLabel("performance-gate");
}
BENCHMARK(BM_Atari2600_PerformanceGate_Corpus);
