#include "pch.h"
#include "ChannelF/ChannelFCoreScaffold.h"
#include "ChannelF/ChannelFOpcodeTable.h"
#include "ChannelF/ChannelFBiosDatabase.h"

static void BM_ChannelFCore_RunFrame_NoInput(benchmark::State& state) {
	ChannelFCoreScaffold core;

	for (auto _ : state) {
		core.RunFrame();
		benchmark::DoNotOptimize(core.GetDeterministicState());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_RunFrame_NoInput);

static void BM_ChannelFCore_RunFrame_WithInput(benchmark::State& state) {
	ChannelFCoreScaffold core;
	core.SetPanelButtons(0x0f);
	core.SetRightController(0x55);
	core.SetLeftController(0xaa);

	for (auto _ : state) {
		core.RunFrame();
		benchmark::DoNotOptimize(core.GetDeterministicState());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_RunFrame_WithInput);

static void BM_ChannelFCore_OpcodeLookup_Defined(benchmark::State& state) {
	for (auto _ : state) {
		const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get(0x2b);
		uint8_t cycles = info.Cycles;
		benchmark::DoNotOptimize(cycles);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_OpcodeLookup_Defined);

static void BM_ChannelFCore_OpcodeLookup_Unknown(benchmark::State& state) {
	for (auto _ : state) {
		const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get(0xff);
		bool isDefined = info.IsDefined;
		benchmark::DoNotOptimize(isDefined);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_OpcodeLookup_Unknown);

static void BM_ChannelFCore_BiosVariantDetect_Luxor(benchmark::State& state) {
	const string luxorSha1 = "759e2ed31fbde4a2d8daf8b9f3e0dffebc90dae2";
	const string luxorMd5 = "95d339631d867c8f1d15a5f2ec26069d";

	for (auto _ : state) {
		ChannelFBiosVariant variant = ChannelFBiosDatabase::DetectVariant(luxorSha1, luxorMd5);
		benchmark::DoNotOptimize(variant);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_BiosVariantDetect_Luxor);

static void BM_ChannelFCore_SerializeRoundtrip(benchmark::State& state) {
	ChannelFCoreScaffold core;
	core.SetPanelButtons(0x0f);
	core.SetRightController(0x55);
	core.SetLeftController(0xaa);
	for (int i = 0; i < 60; i++) {
		core.RunFrame();
	}

	for (auto _ : state) {
		Serializer saver(1, true, SerializeFormat::Binary);
		core.Serialize(saver);

		std::stringstream ss;
		saver.SaveTo(ss);
		ss.seekg(0);

		ChannelFCoreScaffold restored;
		Serializer loader(1, false, SerializeFormat::Binary);
		loader.LoadFrom(ss);
		restored.Serialize(loader);

		benchmark::DoNotOptimize(restored.GetDeterministicState());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_SerializeRoundtrip);

static void BM_ChannelFCore_Reset(benchmark::State& state) {
	ChannelFCoreScaffold core;
	core.SetPanelButtons(0x0f);
	core.SetRightController(0xaa);
	core.SetLeftController(0x55);
	core.RunFrame();

	for (auto _ : state) {
		core.Reset();
		benchmark::DoNotOptimize(core.GetDeterministicState());
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ChannelFCore_Reset);

static void BM_ChannelFCore_OpcodeTableFullScan(benchmark::State& state) {
	for (auto _ : state) {
		uint32_t definedCount = 0;
		for (uint16_t op = 0; op < 256; op++) {
			const ChannelFOpcodeInfo& info = ChannelFOpcodeTable::Get(static_cast<uint8_t>(op));
			if (info.IsDefined) definedCount++;
		}
		benchmark::DoNotOptimize(definedCount);
	}
	state.SetItemsProcessed(state.iterations() * 256);
}
BENCHMARK(BM_ChannelFCore_OpcodeTableFullScan);
