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
