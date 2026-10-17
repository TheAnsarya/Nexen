#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	vector<uint8_t> BuildNopRom() {
		vector<uint8_t> rom(0x4000, 0x00);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = 0x4E;
			rom[i + 1] = 0x71;
		}
		return rom;
	}

	struct GenesisCombinedSnapshot {
		uint64_t CpuCycles = 0;
		uint32_t HorizontalInterrupts = 0;
		uint32_t VerticalInterrupts = 0;
		uint32_t DmaContentionCycles = 0;
		uint32_t DmaContentionEvents = 0;
		uint64_t Z80Cycles = 0;
		uint32_t Z80Handoffs = 0;
		string RenderDigest;
		string MixedDigest;
		string YmDigest;
		string PsgDigest;
	};

	GenesisCombinedSnapshot RunCombinedReplayScenario() {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.LoadRom(BuildNopRom());
		scaffold.Startup();
		scaffold.ConfigureInterruptSchedule(true, 8, true);
		scaffold.ClearTimingEvents();

		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		bus.WriteByte(0xA04000, 0x22);
		bus.WriteByte(0xA04001, 0x21);
		bus.WriteByte(0xA04002, 0x30);
		bus.WriteByte(0xA04003, 0x0C);
		bus.WriteByte(0xC00011, 0x90);
		bus.WriteByte(0xC00011, 0x06);

		bus.SetRenderCompositionInputs(0x1A, true, 0x06, false, 0x0F, true, true, 0x20, true);
		bus.SetScroll(5, 2);
		bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 48);

		for (uint32_t step = 0; step < 24; step++) {
			if (step == 4 || step == 12) {
				bus.WriteByte(0xA11100, 0x01);
			}
			if (step == 7 || step == 16) {
				bus.WriteByte(0xA11100, 0x00);
			}

			if ((step % 6) == 0) {
				bus.WriteByte(0xC00004, 0x97);
				bus.WriteByte(0xC00005, 0x80);
				bus.WriteByte(0xC00004, 0x81);
				bus.WriteByte(0xC00005, 0x40);
			}

			scaffold.StepFrameScaffold(244);
			bus.RenderScaffoldLine(64);
		}

		GenesisCombinedSnapshot snapshot = {};
		snapshot.CpuCycles = scaffold.GetCpu().GetCycleCount();
		snapshot.HorizontalInterrupts = scaffold.GetHorizontalInterruptCount();
		snapshot.VerticalInterrupts = scaffold.GetVerticalInterruptCount();
		snapshot.DmaContentionCycles = bus.GetDmaContentionCycles();
		snapshot.DmaContentionEvents = bus.GetDmaContentionEvents();
		snapshot.Z80Cycles = bus.GetZ80ExecutedCycles();
		snapshot.Z80Handoffs = bus.GetZ80HandoffCount();
		snapshot.RenderDigest = bus.GetRenderLineDigest();
		snapshot.MixedDigest = bus.GetMixedDigest();
		snapshot.YmDigest = bus.GetYmDigest();
		snapshot.PsgDigest = bus.GetPsgDigest();
		return snapshot;
	}

	TEST(GenesisCombinedInteractionTests, DmaAndInterruptScheduleInteractionIsDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.LoadRom(BuildNopRom());
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 8, true);
			scaffold.ClearTimingEvents();

			scaffold.GetBus().BeginDmaTransfer(GenesisVdpDmaMode::Fill, 32);
			for (uint32_t i = 0; i < 300; i++) {
				scaffold.StepFrameScaffold(488);
			}

			return std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>(
				scaffold.GetHorizontalInterruptCount(),
				scaffold.GetVerticalInterruptCount(),
				scaffold.GetBus().GetDmaContentionCycles(),
				scaffold.GetBus().GetDmaContentionEvents());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_GT(std::get<0>(runA), 0u);
		EXPECT_GT(std::get<1>(runA), 0u);
		EXPECT_GT(std::get<2>(runA), 0u);
		EXPECT_GT(std::get<3>(runA), 0u);
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisCombinedInteractionTests, DmaWithZ80HandoffProducesStableExecutionState) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.LoadRom(BuildNopRom());
			scaffold.Startup();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 40);

			for (uint32_t i = 0; i < 10; i++) {
				bus.WriteByte(0xA11100, (i % 2 == 0) ? 0x01 : 0x00);
				scaffold.StepFrameScaffold(160);
			}

			return std::tuple<uint64_t, uint32_t, uint32_t, bool>(
				bus.GetZ80ExecutedCycles(),
				bus.GetZ80HandoffCount(),
				bus.GetDmaContentionEvents(),
				bus.IsZ80Running());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_GT(std::get<0>(runA), 0u);
		EXPECT_GE(std::get<1>(runA), 2u);
		EXPECT_GT(std::get<2>(runA), 0u);
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisCombinedInteractionTests, CombinedReplayKeepsAudioAndRenderDigestsStable) {
		GenesisCombinedSnapshot runA = RunCombinedReplayScenario();
		GenesisCombinedSnapshot runB = RunCombinedReplayScenario();

		EXPECT_GT(runA.HorizontalInterrupts, 0u);
		EXPECT_GT(runA.DmaContentionEvents, 0u);
		EXPECT_GT(runA.Z80Cycles, 0u);
		EXPECT_FALSE(runA.RenderDigest.empty());
		EXPECT_FALSE(runA.MixedDigest.empty());
		EXPECT_FALSE(runA.YmDigest.empty());
		EXPECT_FALSE(runA.PsgDigest.empty());

		EXPECT_EQ(runA.CpuCycles, runB.CpuCycles);
		EXPECT_EQ(runA.HorizontalInterrupts, runB.HorizontalInterrupts);
		EXPECT_EQ(runA.VerticalInterrupts, runB.VerticalInterrupts);
		EXPECT_EQ(runA.DmaContentionCycles, runB.DmaContentionCycles);
		EXPECT_EQ(runA.DmaContentionEvents, runB.DmaContentionEvents);
		EXPECT_EQ(runA.Z80Cycles, runB.Z80Cycles);
		EXPECT_EQ(runA.Z80Handoffs, runB.Z80Handoffs);
		EXPECT_EQ(runA.RenderDigest, runB.RenderDigest);
		EXPECT_EQ(runA.MixedDigest, runB.MixedDigest);
		EXPECT_EQ(runA.YmDigest, runB.YmDigest);
		EXPECT_EQ(runA.PsgDigest, runB.PsgDigest);
	}

	TEST(GenesisCombinedInteractionTests, MixedCadenceBoundaryWindowsRemainDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.LoadRom(BuildNopRom());
			scaffold.Startup();
			scaffold.ConfigureInterruptSchedule(true, 4, true);
			scaffold.ClearTimingEvents();

			auto& bus = scaffold.GetBus();
			bus.WriteByte(0xA11200, 0x01);
			bus.BeginDmaTransfer(GenesisVdpDmaMode::Copy, 56);

			const std::array<uint32_t, 12> cadence = {
				1, 487, 488, 489,
				96, 244, 732, 977,
				488, 1953, 120, 488
			};

			for (size_t step = 0; step < cadence.size(); step++) {
				if (step == 2 || step == 8) {
					bus.WriteByte(0xA11100, 0x01);
				}
				if (step == 4 || step == 10) {
					bus.WriteByte(0xA11100, 0x00);
				}

				bus.WriteByte(0xA04000, (uint8_t)(0x20u + (uint8_t)step));
				bus.WriteByte(0xA04001, (uint8_t)(0x30u + (uint8_t)step));
				bus.WriteByte(0xC00011, (uint8_t)(0x80u | (uint8_t)step));

				scaffold.StepFrameScaffold(cadence.at(step));
			}

			return std::tuple<uint64_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, string, string, string, vector<string>>(
				scaffold.GetCpu().GetCycleCount(),
				scaffold.GetHorizontalInterruptCount(),
				scaffold.GetVerticalInterruptCount(),
				bus.GetDmaContentionCycles(),
				bus.GetDmaContentionEvents(),
				bus.GetZ80ExecutedCycles(),
				bus.GetMixedDigest(),
				bus.GetYmDigest(),
				bus.GetPsgDigest(),
				scaffold.GetTimingEvents());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_GT(std::get<1>(runA), 0u);
		EXPECT_GT(std::get<3>(runA), 0u);
		EXPECT_GT(std::get<5>(runA), 0u);
		EXPECT_FALSE(std::get<6>(runA).empty());
		EXPECT_FALSE(std::get<7>(runA).empty());
		EXPECT_FALSE(std::get<8>(runA).empty());
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisCombinedInteractionTests, SaveStateReplayAcrossCadenceEdgesKeepsDeterministicDigests) {
		GenesisM68kBoundaryScaffold baseline;
		baseline.LoadRom(BuildNopRom());
		baseline.Startup();
		baseline.ConfigureInterruptSchedule(true, 2, true);
		baseline.ClearTimingEvents();

		auto& baseBus = baseline.GetBus();
		baseBus.WriteByte(0xA11200, 0x01);
		baseBus.BeginDmaTransfer(GenesisVdpDmaMode::Fill, 40);

		for (uint32_t i = 0; i < 6; i++) {
			if (i == 2) {
				baseBus.WriteByte(0xA11100, 0x01);
			}
			if (i == 4) {
				baseBus.WriteByte(0xA11100, 0x00);
			}
			baseBus.WriteByte(0xA04000, (uint8_t)(0x40u + (uint8_t)i));
			baseline.StepFrameScaffold((i % 2 == 0) ? 487 : 489);
		}

		GenesisBoundaryScaffoldSaveState checkpoint = baseline.SaveState();

		for (uint32_t i = 0; i < 10; i++) {
			baseBus.WriteByte(0xC00011, (uint8_t)(0x90u + (uint8_t)i));
			baseline.StepFrameScaffold((i % 3 == 0) ? 488 : 244);
		}

		GenesisM68kBoundaryScaffold replay;
		replay.LoadState(checkpoint);
		auto& replayBus = replay.GetBus();

		for (uint32_t i = 0; i < 10; i++) {
			replayBus.WriteByte(0xC00011, (uint8_t)(0x90u + (uint8_t)i));
			replay.StepFrameScaffold((i % 3 == 0) ? 488 : 244);
		}

		EXPECT_EQ(baseline.GetCpu().GetCycleCount(), replay.GetCpu().GetCycleCount());
		EXPECT_EQ(baseline.GetHorizontalInterruptCount(), replay.GetHorizontalInterruptCount());
		EXPECT_EQ(baseline.GetVerticalInterruptCount(), replay.GetVerticalInterruptCount());
		EXPECT_EQ(baseBus.GetDmaContentionCycles(), replayBus.GetDmaContentionCycles());
		EXPECT_EQ(baseBus.GetDmaContentionEvents(), replayBus.GetDmaContentionEvents());
		EXPECT_EQ(baseBus.GetZ80ExecutedCycles(), replayBus.GetZ80ExecutedCycles());
		EXPECT_EQ(baseBus.GetMixedDigest(), replayBus.GetMixedDigest());
		EXPECT_EQ(baseBus.GetYmDigest(), replayBus.GetYmDigest());
		EXPECT_EQ(baseBus.GetPsgDigest(), replayBus.GetPsgDigest());
		EXPECT_EQ(baseline.GetTimingEvents(), replay.GetTimingEvents());
	}
}
