#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

namespace {
	struct Atari2600EdgeSnapshot {
		Atari2600RiotState Riot;
		Atari2600TiaState Tia;
		uint8_t MapperBank = 0;
		uint8_t MapperSample = 0;
		string MapperMode;
		uint8_t RiotInterruptMirror = 0;
		uint8_t RiotUnderflowMirror = 0;
	};

	Atari2600EdgeSnapshot RunCombinedEdgeScenario(const vector<uint8_t>& romData, const string& romName) {
		Emulator emu;
		Atari2600Console console(&emu);

		VirtualFile romFile(romData.data(), romData.size(), romName);
		EXPECT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		console.Reset();

		console.DebugWriteCartridge(0x0082, 0x0F);
		console.DebugWriteCartridge(0x0180, 0xA5);
		console.DebugWriteCartridge(0x0184, 0x01);

		for (uint8_t i = 0; i < 8; i++) {
			console.DebugWriteCartridge(0x0042, 0x00);
			console.DebugWriteCartridge(0x006A, 0x00);
			console.DebugWriteCartridge((uint16_t)(0x13FF + (i * 0x0400)), (uint8_t)(i + 1));
			console.StepCpuCycles(1);
		}
		console.DebugWriteCartridge(0x17FF, 0x03);

		Atari2600EdgeSnapshot snapshot = {};
		snapshot.Riot = console.GetRiotState();
		snapshot.Tia = console.GetTiaState();
		snapshot.MapperBank = console.DebugGetMapperBankIndex();
		snapshot.MapperSample = console.DebugReadCartridge(0x1000);
		snapshot.MapperMode = console.DebugGetMapperMode();
		snapshot.RiotInterruptMirror = console.DebugReadCartridge(0x0186);
		snapshot.RiotUnderflowMirror = console.DebugReadCartridge(0x0187);
		return snapshot;
	}

	TEST(Atari2600DeterministicEdgeCaseTests, RiotTimerDividerAndMirrorRegistersAreDeterministic) {
		auto runScenario = []() {
			Emulator emu;
			Atari2600Console console(&emu);

			console.Reset();
			console.DebugWriteCartridge(0x0282, 0x0F);
			console.DebugWriteCartridge(0x0280, 0xA5);
			console.DebugWriteCartridge(0x0286, 0x01);

			console.StepCpuCycles(64);
			EXPECT_FALSE(console.GetRiotState().TimerUnderflow);
			console.StepCpuCycles(64);

			Atari2600RiotState state = console.GetRiotState();
			EXPECT_EQ(console.DebugReadCartridge(0x0280), 0x05u);
			EXPECT_EQ(console.DebugReadCartridge(0x0282), 0x0Fu);
			EXPECT_EQ(console.DebugReadCartridge(0x0286), 0x01u);
			EXPECT_EQ(console.DebugReadCartridge(0x0287), 0x01u);
			return state;
		};

		Atari2600RiotState runA = runScenario();
		Atari2600RiotState runB = runScenario();

		EXPECT_TRUE(runA.TimerUnderflow);
		EXPECT_TRUE(runA.InterruptFlag);
		EXPECT_EQ(runA.TimerDivider, 64u);
		EXPECT_EQ(runA.InterruptEdgeCount, 1u);
		EXPECT_EQ(runA.Timer, runB.Timer);
		EXPECT_EQ(runA.TimerUnderflow, runB.TimerUnderflow);
		EXPECT_EQ(runA.InterruptFlag, runB.InterruptFlag);
		EXPECT_EQ(runA.InterruptEdgeCount, runB.InterruptEdgeCount);
		EXPECT_EQ(runA.CpuCycles, runB.CpuCycles);
	}

	TEST(Atari2600DeterministicEdgeCaseTests, TiaWsyncHmoveMirrorsKeepStableTimingState) {
		auto runScenario = []() {
			Emulator emu;
			Atari2600Console console(&emu);

			console.Reset();
			console.DebugWriteCartridge(0x0049, 0x2A);
			for (uint32_t i = 0; i < 16; i++) {
				console.DebugWriteCartridge(0x0042, 0x00);
				console.DebugWriteCartridge(0x006A, 0x00);
				console.StepCpuCycles(1);
			}

			Atari2600TiaState state = console.GetTiaState();
			// TIA read address $09 = INPT1 (paddle input, not connected = 0x80)
			// Note: TIA reads use addr & 0x0F, separate from write registers
			EXPECT_EQ(console.DebugReadCartridge(0x0009), 0x80u);
			// Verify the write register was actually set correctly via state
			EXPECT_EQ(state.ColorBackground, 0x2Au);
			return state;
		};

		Atari2600TiaState runA = runScenario();
		Atari2600TiaState runB = runScenario();

		EXPECT_EQ(runA.Scanline, 16u);
		EXPECT_EQ(runA.WsyncCount, 16u);
		EXPECT_EQ(runA.HmoveStrobeCount, 16u);
		EXPECT_EQ(runA.HmoveApplyCount, 16u);
		EXPECT_EQ(runA.ColorClock, 11u);
		EXPECT_EQ(runA.TotalColorClocks, runB.TotalColorClocks);
		EXPECT_EQ(runA.Scanline, runB.Scanline);
		EXPECT_EQ(runA.WsyncCount, runB.WsyncCount);
		EXPECT_EQ(runA.HmoveApplyCount, runB.HmoveApplyCount);
		EXPECT_EQ(runA.ColorClock, runB.ColorClock);
	}

	TEST(Atari2600DeterministicEdgeCaseTests, CombinedRiotTiaMapperEdgeSequenceIsDeterministic) {
		vector<uint8_t> fallbackRom(16384, 0x00);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				fallbackRom.begin() + (size_t)bank * 0x1000,
				fallbackRom.begin() + ((size_t)bank + 1) * 0x1000,
				(uint8_t)(0x90 + bank));
		}

		Atari2600EdgeSnapshot runA = RunCombinedEdgeScenario(fallbackRom, "rare-homebrew-edge.a26");
		Atari2600EdgeSnapshot runB = RunCombinedEdgeScenario(fallbackRom, "rare-homebrew-edge.a26");

		EXPECT_EQ(runA.MapperMode, "fallback");
		EXPECT_EQ(runA.MapperBank, 3u);
		EXPECT_EQ(runA.Tia.WsyncCount, 8u);
		EXPECT_EQ(runA.Tia.HmoveApplyCount, 8u);
		EXPECT_GT(runA.Riot.CpuCycles, 0u);

		EXPECT_EQ(runA.MapperBank, runB.MapperBank);
		EXPECT_EQ(runA.MapperSample, runB.MapperSample);
		EXPECT_EQ(runA.Riot.Timer, runB.Riot.Timer);
		EXPECT_EQ(runA.Riot.InterruptEdgeCount, runB.Riot.InterruptEdgeCount);
		EXPECT_EQ(runA.Tia.Scanline, runB.Tia.Scanline);
		EXPECT_EQ(runA.Tia.TotalColorClocks, runB.Tia.TotalColorClocks);
		EXPECT_EQ(runA.RiotInterruptMirror, runB.RiotInterruptMirror);
		EXPECT_EQ(runA.RiotUnderflowMirror, runB.RiotUnderflowMirror);
	}
}
