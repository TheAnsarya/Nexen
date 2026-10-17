#include "pch.h"
#include "Shared/Emulator.h"
#include "Utilities/VirtualFile.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Atari2600/Atari2600Tia.h"
#include "Atari2600/Atari2600Riot.h"
#include "Atari2600/Atari2600Bus.h"
#include "Atari2600/Atari2600Mapper.h"

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

// --- Subsystem-level benchmarks ---

static void BM_Atari2600_TiaStepCpuCycles(benchmark::State& state) {
	Atari2600Tia tia;
	tia.Reset();
	uint32_t cyclesPerIteration = Atari2600Constants::CpuCyclesPerFrame;

	for (auto _ : state) {
		tia.BeginFrameCapture();
		tia.StepCpuCycles(cyclesPerIteration);
		benchmark::DoNotOptimize(tia.GetState().TotalColorClocks);
	}

	state.SetItemsProcessed(state.iterations() * cyclesPerIteration);
	state.SetLabel("tia-step-cpu-cycles");
}
BENCHMARK(BM_Atari2600_TiaStepCpuCycles);

static void BM_Atari2600_TiaAudioStep(benchmark::State& state) {
	Atari2600Tia tia;
	tia.Reset();

	// Configure channels with non-zero volume for realistic workload
	tia.WriteRegister(0x15, 0x02); // AUDC0 = tone
	tia.WriteRegister(0x16, 0x0F); // AUDF0 = freq 15
	tia.WriteRegister(0x17, 0x0A); // AUDV0 = vol 10
	tia.WriteRegister(0x18, 0x01); // AUDC1 = square
	tia.WriteRegister(0x19, 0x08); // AUDF1 = freq 8
	tia.WriteRegister(0x1A, 0x06); // AUDV1 = vol 6

	uint32_t cyclesPerIteration = Atari2600Constants::CpuCyclesPerFrame;

	for (auto _ : state) {
		tia.BeginFrameCapture();
		tia.StepCpuCycles(cyclesPerIteration);
		auto audioData = tia.GetAudioBuffer();
		benchmark::DoNotOptimize(audioData.data());
		benchmark::DoNotOptimize(audioData.size());
	}

	state.SetItemsProcessed(state.iterations() * cyclesPerIteration);
	state.SetLabel("tia-audio-step");
}
BENCHMARK(BM_Atari2600_TiaAudioStep);

static void BM_Atari2600_RiotTimerStep(benchmark::State& state) {
	Atari2600Riot riot;
	riot.Reset();

	// Configure timer with TIM64T (divider = 64)
	riot.WriteRegister(0x0296, 0xFF); // TIM64T = 255

	uint32_t cyclesPerIteration = Atari2600Constants::CpuCyclesPerFrame;

	for (auto _ : state) {
		riot.StepCpuCycles(cyclesPerIteration);
		benchmark::DoNotOptimize(riot.ReadRegister(0x0284)); // Read timer
	}

	state.SetItemsProcessed(state.iterations() * cyclesPerIteration);
	state.SetLabel("riot-timer-step");
}
BENCHMARK(BM_Atari2600_RiotTimerStep);

static void BM_Atari2600_BusDispatch(benchmark::State& state) {
	Emulator emu;
	Atari2600Console console(&emu);

	vector<uint8_t> rom(4096, 0xea);
	VirtualFile romFile(rom.data(), rom.size(), "bench-bus.a26");
	if (console.LoadRom(romFile) != LoadRomResult::Success) {
		state.SkipWithError("failed to load Atari2600 benchmark ROM");
		return;
	}

	// Mix of address regions: cartridge, RIOT, TIA
	static constexpr uint16_t addresses[] = {
		0x1000, 0x1100, 0x1200, 0x1FFF, // Cartridge (most common)
		0x0080, 0x00FF,                   // RIOT RAM
		0x0000, 0x000D,                   // TIA
	};
	static constexpr size_t addrCount = sizeof(addresses) / sizeof(addresses[0]);

	uint8_t checksum = 0;
	for (auto _ : state) {
		for (size_t j = 0; j < addrCount; j++) {
			checksum ^= console.DebugRead(addresses[j]);
		}
	}

	benchmark::DoNotOptimize(checksum);
	state.SetItemsProcessed(state.iterations() * addrCount);
	state.SetLabel("bus-dispatch");
}
BENCHMARK(BM_Atari2600_BusDispatch);
