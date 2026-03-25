#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"
#include "Genesis/GenesisSmokeHarness.h"

namespace {
	vector<uint8_t> BuildNopRom(size_t size) {
		vector<uint8_t> rom(size, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4e;
			rom[i + 1] = 0x71;
		}
		return rom;
	}

	vector<GenesisCompatibilityRomCase> BuildCompatibilityCorpus() {
		vector<GenesisCompatibilityRomCase> corpus;
		corpus.push_back({"sonic-bench.bin", BuildNopRom(0x8000)});
		corpus.push_back({"jurassic-bench.bin", BuildNopRom(0x8000)});
		corpus.push_back({"generic-bench.bin", BuildNopRom(0x8000)});
		return corpus;
	}
}

static void BM_Genesis_StepFrameScaffold_OneScanline(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<uint8_t> rom = BuildNopRom(0x8000);
	scaffold.LoadRom(rom);
	scaffold.Startup();

	for (auto _ : state) {
		scaffold.StepFrameScaffold(488);
		benchmark::DoNotOptimize(scaffold.GetTimingScanline());
		benchmark::DoNotOptimize(scaffold.GetCpu().GetCycleCount());
	}

	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Genesis_StepFrameScaffold_OneScanline);

static void BM_Genesis_AudioBusWriteAndMix(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<uint8_t> rom = BuildNopRom(0x8000);
	scaffold.LoadRom(rom);
	scaffold.Startup();

	for (auto _ : state) {
		scaffold.GetBus().WriteByte(0xA04000, 0x22);
		scaffold.GetBus().WriteByte(0xA04001, 0x10);
		scaffold.GetBus().WriteByte(0xC00011, 0x90);
		scaffold.GetBus().WriteByte(0xC00011, 0x08);
		scaffold.StepFrameScaffold(144u * 4u);
		benchmark::DoNotOptimize(scaffold.GetBus().GetMixedSampleCount());
		benchmark::DoNotOptimize(scaffold.GetBus().GetMixedDigest().data());
	}

	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_Genesis_AudioBusWriteAndMix);

static void BM_Genesis_AudioCadence_AlternatingYmPsg(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<uint8_t> rom = BuildNopRom(0x8000);
	scaffold.LoadRom(rom);
	scaffold.Startup();

	uint32_t phase = 0;
	for (auto _ : state) {
		scaffold.GetBus().WriteByte(0xA04000, 0x22);
		scaffold.GetBus().WriteByte(0xA04001, (uint8_t)(0x10 + (phase & 0x1F)));
		scaffold.GetBus().WriteByte(0xA04002, 0x30);
		scaffold.GetBus().WriteByte(0xA04003, (uint8_t)(0x08 + (phase & 0x0F)));
		scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x90 | (phase & 0x0F)));
		scaffold.GetBus().WriteByte(0xC00011, (uint8_t)(0x04 + (phase & 0x07)));
		scaffold.StepFrameScaffold(128u + (phase % 5u));
		phase++;
		benchmark::DoNotOptimize(scaffold.GetBus().GetYmSampleCount());
		benchmark::DoNotOptimize(scaffold.GetBus().GetPsgSampleCount());
		benchmark::DoNotOptimize(scaffold.GetBus().GetMixedDigest().data());
	}

	state.SetItemsProcessed(state.iterations() * 6);
	state.SetLabel("audio-cadence-alternating");
}
BENCHMARK(BM_Genesis_AudioCadence_AlternatingYmPsg);

static void BM_Genesis_CompatibilityMatrix_Corpus(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<GenesisCompatibilityRomCase> corpus = BuildCompatibilityCorpus();

	for (auto _ : state) {
		GenesisCompatibilityMatrixResult result = GenesisSmokeHarness::RunCompatibilityMatrix(scaffold, corpus);
		benchmark::DoNotOptimize(result.PassCount);
		benchmark::DoNotOptimize(result.Digest.data());
	}

	state.SetItemsProcessed(state.iterations() * corpus.size());
	state.SetLabel("compatibility-matrix");
}
BENCHMARK(BM_Genesis_CompatibilityMatrix_Corpus);

static void BM_Genesis_PerformanceGate_Corpus(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<GenesisCompatibilityRomCase> corpus = BuildCompatibilityCorpus();

	for (auto _ : state) {
		GenesisPerformanceGateResult result = GenesisSmokeHarness::RunPerformanceGate(scaffold, corpus, 5000000);
		benchmark::DoNotOptimize(result.PassCount);
		benchmark::DoNotOptimize(result.Digest.data());
	}

	state.SetItemsProcessed(state.iterations() * corpus.size());
	state.SetLabel("performance-gate");
}
BENCHMARK(BM_Genesis_PerformanceGate_Corpus);

static void BM_Genesis_DmaContention_CopyBurst(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<uint8_t> rom = BuildNopRom(0x8000);
	scaffold.LoadRom(rom);

	for (auto _ : state) {
		scaffold.Startup();
		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Copy, 64);
		scaffold.StepFrameScaffold(512);
		benchmark::DoNotOptimize(scaffold.GetBus().GetDmaContentionCycles());
		benchmark::DoNotOptimize(scaffold.GetCpu().GetCycleCount());
	}

	state.SetItemsProcessed(state.iterations() * 64);
	state.SetLabel("dma-copy-contention");
}
BENCHMARK(BM_Genesis_DmaContention_CopyBurst);

static void BM_Genesis_DmaContention_FillBurst(benchmark::State& state) {
	GenesisM68kBoundaryScaffold scaffold;
	vector<uint8_t> rom = BuildNopRom(0x8000);
	scaffold.LoadRom(rom);

	for (auto _ : state) {
		scaffold.Startup();
		scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Fill, 64);
		scaffold.StepFrameScaffold(512);
		benchmark::DoNotOptimize(scaffold.GetBus().GetDmaContentionCycles());
		benchmark::DoNotOptimize(scaffold.GetCpu().GetCycleCount());
	}

	state.SetItemsProcessed(state.iterations() * 64);
	state.SetLabel("dma-fill-contention");
}
BENCHMARK(BM_Genesis_DmaContention_FillBurst);
