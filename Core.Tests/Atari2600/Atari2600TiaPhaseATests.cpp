#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

namespace {
	void LoadNopRom(Atari2600Console& console, const string& name) {
		vector<uint8_t> rom(4096, 0xEA);
		VirtualFile romFile(rom.data(), rom.size(), name);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
	}

	TEST(Atari2600TiaPhaseATests, WsyncAdvancesScanlineOnNextCpuCycleBoundary) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.RequestWsync();
		console.StepCpuCycles(1);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Scanline, 1u);
		EXPECT_EQ(tiaState.ColorClock, 3u);
		EXPECT_EQ(tiaState.WsyncCount, 1u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveStrobeAppliesSingleBlankingBurst) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x002A, 0x00);
		EXPECT_TRUE(console.GetTiaState().HmovePending);
		EXPECT_EQ(console.GetTiaState().HmoveStrobeCount, 1u);

		console.StepCpuCycles(1);
		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_FALSE(tiaState.HmovePending);
		EXPECT_EQ(tiaState.HmoveApplyCount, 1u);
		EXPECT_EQ(tiaState.ColorClock, 11u);
		EXPECT_EQ(tiaState.TotalColorClocks, 11u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveClearStrobeCancelsPendingBurst) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x002A, 0x00);
		console.DebugWriteCartridge(0x002B, 0x00);
		EXPECT_FALSE(console.GetTiaState().HmovePending);

		console.StepCpuCycles(1);
		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_FALSE(tiaState.HmovePending);
		EXPECT_EQ(tiaState.HmoveApplyCount, 0u);
		EXPECT_EQ(tiaState.ColorClock, 3u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveAndWsyncRemainDeterministicAcrossFrames) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		for (uint32_t i = 0; i < 6; i++) {
			console.RequestWsync();
			console.DebugWriteCartridge(0x002A, 0x00);
			console.StepCpuCycles(1);
		}

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Scanline, 6u);
		EXPECT_EQ(tiaState.WsyncCount, 6u);
		EXPECT_EQ(tiaState.HmoveStrobeCount, 6u);
		EXPECT_EQ(tiaState.HmoveApplyCount, 6u);
		EXPECT_EQ(tiaState.ColorClock, 11u);
	}

	TEST(Atari2600TiaPhaseATests, CtrlpfBitsPopulateReflectScoreAndPriorityFlags) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x000A, 0x07);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_TRUE(tiaState.PlayfieldReflect);
		EXPECT_TRUE(tiaState.PlayfieldScoreMode);
		EXPECT_TRUE(tiaState.PlayfieldPriority);
		EXPECT_EQ(console.DebugReadCartridge(0x000A), 0x07u);
	}

	TEST(Atari2600TiaPhaseATests, PlayfieldRegisterWritesLatchPerScanline) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x000D, 0x10);
		console.StepCpuCycles(76);
		console.DebugWriteCartridge(0x000D, 0x20);

		Atari2600ScanlineRenderState line0 = console.DebugGetScanlineRenderState(0);
		Atari2600ScanlineRenderState line1 = console.DebugGetScanlineRenderState(1);
		EXPECT_EQ(line0.Playfield0, 0x10u);
		EXPECT_EQ(line1.Playfield0, 0x20u);
	}

	TEST(Atari2600TiaPhaseATests, ColorClockWrapCarriesToNextScanlineAtBoundary) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(76); // 76 * 3 = 228 color clocks => one full scanline.

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Scanline, 1u);
		EXPECT_EQ(tiaState.ColorClock, 0u);
		EXPECT_EQ(tiaState.TotalColorClocks, 228u);
	}

	TEST(Atari2600TiaPhaseATests, WsyncNearWrapPerformsSingleCarryThenClockAdvance) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(75); // color clock 225 on scanline 0.
		console.RequestWsync();
		console.StepCpuCycles(1);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Scanline, 1u);
		EXPECT_EQ(tiaState.ColorClock, 3u);
		EXPECT_EQ(tiaState.WsyncCount, 1u);
		EXPECT_EQ(tiaState.TotalColorClocks, 228u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveAtBoundaryDoesNotDoubleWrapScanline) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(75); // color clock 225.
		console.RequestHmove();
		console.StepCpuCycles(1);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Scanline, 1u);
		EXPECT_EQ(tiaState.ColorClock, 8u);
		EXPECT_EQ(tiaState.HmoveStrobeCount, 1u);
		EXPECT_EQ(tiaState.HmoveApplyCount, 1u);
		EXPECT_EQ(tiaState.TotalColorClocks, 236u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveAfterCycle73DefersUntilNextScanline) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(74); // color clock 222.
		console.RequestHmove();
		console.StepCpuCycles(1);

		Atari2600TiaState deferredState = console.GetTiaState();
		EXPECT_TRUE(deferredState.HmovePending);
		EXPECT_EQ(deferredState.HmoveApplyCount, 0u);
		EXPECT_EQ(deferredState.Scanline, 0u);
		EXPECT_EQ(deferredState.ColorClock, 225u);

		console.StepCpuCycles(1);
		Atari2600TiaState appliedState = console.GetTiaState();
		EXPECT_FALSE(appliedState.HmovePending);
		EXPECT_EQ(appliedState.HmoveApplyCount, 1u);
		EXPECT_EQ(appliedState.Scanline, 1u);
		EXPECT_EQ(appliedState.ColorClock, 8u);
	}

	TEST(Atari2600TiaPhaseATests, HmoveAtCycle73AppliesImmediately) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(73); // color clock 219.
		console.RequestHmove();
		console.StepCpuCycles(1);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_FALSE(tiaState.HmovePending);
		EXPECT_EQ(tiaState.HmoveApplyCount, 1u);
		EXPECT_EQ(tiaState.Scanline, 1u);
		EXPECT_EQ(tiaState.ColorClock, 2u);
	}

	TEST(Atari2600TiaPhaseATests, CollisionRegistersLatchAndCxclrClears) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "tia-collision-latches.a26");

		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);
		console.DebugWriteCartridge(0x001B, 0xFF);
		console.DebugWriteCartridge(0x001C, 0xFF);
		console.DebugWriteCartridge(0x001D, 0x02);
		console.DebugWriteCartridge(0x001E, 0x02);
		console.DebugWriteCartridge(0x001F, 0x02);
		console.DebugWriteCartridge(0x0010, 0x00);
		console.DebugWriteCartridge(0x0011, 0x00);
		console.DebugWriteCartridge(0x0012, 0x00);
		console.DebugWriteCartridge(0x0013, 0x00);
		console.DebugWriteCartridge(0x0014, 0x00);

		console.RunFrame();

		EXPECT_EQ(console.DebugReadCartridge(0x0000), 0xC0u); // cxm0p
		EXPECT_EQ(console.DebugReadCartridge(0x0001), 0xC0u); // cxm1p
		EXPECT_EQ(console.DebugReadCartridge(0x0002), 0xC0u); // cxp0fb
		EXPECT_EQ(console.DebugReadCartridge(0x0003), 0xC0u); // cxp1fb
		EXPECT_EQ(console.DebugReadCartridge(0x0004), 0xC0u); // cxm0fb
		EXPECT_EQ(console.DebugReadCartridge(0x0005), 0xC0u); // cxm1fb
		EXPECT_EQ(console.DebugReadCartridge(0x0006), 0x80u); // cxblpf
		EXPECT_EQ(console.DebugReadCartridge(0x0007), 0xC0u); // cxppmm

		console.DebugWriteCartridge(0x002C, 0x00); // cxclr
		for (uint16_t i = 0; i < 8; i++) {
			EXPECT_EQ(console.DebugReadCartridge(i), 0u);
		}
	}

	TEST(Atari2600TiaPhaseATests, CollisionLatchesPersistUntilCxclr) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "tia-collision-persist.a26");

		console.DebugWriteCartridge(0x000D, 0xF0);
		console.DebugWriteCartridge(0x000E, 0xFF);
		console.DebugWriteCartridge(0x000F, 0xFF);
		console.DebugWriteCartridge(0x001B, 0xFF);
		console.DebugWriteCartridge(0x001C, 0xFF);
		console.DebugWriteCartridge(0x001D, 0x02);
		console.DebugWriteCartridge(0x001E, 0x02);
		console.DebugWriteCartridge(0x001F, 0x02);
		console.DebugWriteCartridge(0x0010, 0x00);
		console.DebugWriteCartridge(0x0011, 0x00);
		console.DebugWriteCartridge(0x0012, 0x00);
		console.DebugWriteCartridge(0x0013, 0x00);
		console.DebugWriteCartridge(0x0014, 0x00);

		console.RunFrame();
		uint8_t latchedCxm0p = console.DebugReadCartridge(0x0000);
		EXPECT_NE(latchedCxm0p, 0u);

		console.DebugWriteCartridge(0x000D, 0x00);
		console.DebugWriteCartridge(0x000E, 0x00);
		console.DebugWriteCartridge(0x000F, 0x00);
		console.DebugWriteCartridge(0x001B, 0x00);
		console.DebugWriteCartridge(0x001C, 0x00);
		console.DebugWriteCartridge(0x001D, 0x00);
		console.DebugWriteCartridge(0x001E, 0x00);
		console.DebugWriteCartridge(0x001F, 0x00);
		console.RunFrame();

		EXPECT_EQ(console.DebugReadCartridge(0x0000), latchedCxm0p);

		console.DebugWriteCartridge(0x002C, 0x00);
		EXPECT_EQ(console.DebugReadCartridge(0x0000), 0u);
	}
}
