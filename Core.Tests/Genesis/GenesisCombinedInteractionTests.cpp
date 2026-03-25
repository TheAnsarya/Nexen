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
}
