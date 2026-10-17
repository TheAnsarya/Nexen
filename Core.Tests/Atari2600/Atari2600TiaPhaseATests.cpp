#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Utilities/VirtualFile.h"

namespace {
	void LoadNopRom(Atari2600Console& console, const string& name) {
		vector<uint8_t> rom(4096, 0xEA);
		// Place JMP $1000 at end of ROM so CPU loops instead of wrapping into TIA/RIOT space
		rom[0x0FFD] = 0x4C; // JMP abs
		rom[0x0FFE] = 0x00; // low byte of $1000
		rom[0x0FFF] = 0x10; // high byte of $1000
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
		// TIA reads at $0A return INPT2 (addr & 0x0F), not CTRLPF. State verified above.
	}

	TEST(Atari2600TiaPhaseATests, CtrlpfBallSizeBitsPopulateDecodedWidth) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x000A, 0x30);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.BallSize, 8u);
		// TIA reads at $0A return INPT2, not CTRLPF. State verified above.

		console.DebugWriteCartridge(0x000A, 0x10);
		tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.BallSize, 2u);
	}

	TEST(Atari2600TiaPhaseATests, RefpBitsPopulatePerScanlineReflectionState) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x000B, 0x08);
		console.DebugWriteCartridge(0x000C, 0x08);

		Atari2600ScanlineRenderState line0 = console.DebugGetScanlineRenderState(0);
		EXPECT_TRUE(line0.Player0Reflect);
		EXPECT_TRUE(line0.Player1Reflect);
		// TIA reads at $0B/$0C return INPT3/INPT4 (addr & 0x0F), not REFP0/REFP1 write values.
		// Reflection state verified via DebugGetScanlineRenderState above.
	}

	TEST(Atari2600TiaPhaseATests, Resmp0TracksPlayer0PositionWhenEnabled) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0028, 0x02); // resmp0 on
		// Note: TIA reads at $28 return INPT0 (addr & 0x0F = $08), not RESMP0 write value.
		// Verify RESMP0 state via GetTiaState() instead.

		console.StepCpuCycles(8);                  // color clock 24
		console.DebugWriteCartridge(0x0010, 0x00); // resp0 => x=24
		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Player0X, 24u);
		EXPECT_EQ(tiaState.Missile0X, 24u);

		console.StepCpuCycles(8);                  // color clock 48
		console.DebugWriteCartridge(0x0010, 0x00); // resp0 => x=48
		tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Player0X, 48u);
		EXPECT_EQ(tiaState.Missile0X, 48u);
	}

	TEST(Atari2600TiaPhaseATests, Resmp0OffAllowsIndependentMissilePosition) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0028, 0x02); // lock on
		console.DebugWriteCartridge(0x0028, 0x00); // lock off
		// Note: TIA reads at $28 return INPT0 (addr & 0x0F = $08), not RESMP0 write value.
		// Verify RESMP0 state via position checks below.

		console.DebugWriteCartridge(0x0010, 0x00); // resp0 at color clock 0 => x=0
		console.StepCpuCycles(8);                  // color clock 24
		console.DebugWriteCartridge(0x0012, 0x00); // resm0 => x=24

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Player0X, 0u);
		EXPECT_EQ(tiaState.Missile0X, 24u);
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

	TEST(Atari2600TiaPhaseATests, ColorClockCarryStressMatrixMatchesExpectedWrapMath) {
		for (uint32_t preCycles = 70; preCycles <= 78; preCycles++) {
			for (uint32_t burstCycles = 1; burstCycles <= 4; burstCycles++) {
				Emulator emu;
				Atari2600Console console(&emu);

				console.Reset();
				console.StepCpuCycles(preCycles);
				console.StepCpuCycles(burstCycles);

				uint32_t totalColorClocks = (preCycles + burstCycles) * 3;
				uint32_t expectedScanline = totalColorClocks / 228;
				uint32_t expectedColorClock = totalColorClocks % 228;

				Atari2600TiaState tiaState = console.GetTiaState();
				EXPECT_EQ(tiaState.Scanline, expectedScanline);
				EXPECT_EQ(tiaState.ColorClock, expectedColorClock);
				EXPECT_EQ(tiaState.TotalColorClocks, totalColorClocks);
			}
		}
	}

	TEST(Atari2600TiaPhaseATests, ColorClockCarryStressMatrixIsDeterministicAcrossRuns) {
		auto runMatrix = []() {
			std::vector<std::pair<uint32_t, uint32_t>> samples;
			for (uint32_t preCycles = 72; preCycles <= 76; preCycles++) {
				Emulator emu;
				Atari2600Console console(&emu);
				console.Reset();

				console.StepCpuCycles(preCycles);
				console.RequestWsync();
				console.StepCpuCycles(1);
				Atari2600TiaState afterWsync = console.GetTiaState();
				samples.emplace_back(afterWsync.Scanline, afterWsync.ColorClock);

				console.DebugWriteCartridge(0x002A, 0x00);
				console.StepCpuCycles(1);
				Atari2600TiaState afterHmove = console.GetTiaState();
				samples.emplace_back(afterHmove.Scanline, afterHmove.ColorClock);
			}
			return samples;
		};

		auto runA = runMatrix();
		auto runB = runMatrix();
		EXPECT_EQ(runA, runB);
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

	TEST(Atari2600TiaPhaseATests, WsyncHmoveNarrowWindowMatrixAssertsApplyGateAndCarry) {
		for (uint32_t preCycles = 70; preCycles <= 76; preCycles++) {
			SCOPED_TRACE(string("pre_cycles=") + std::to_string(preCycles));

			Emulator emu;
			Atari2600Console console(&emu);
			console.Reset();

			console.StepCpuCycles(preCycles);
			console.RequestHmove();
			console.StepCpuCycles(1);

			Atari2600TiaState state = console.GetTiaState();
			bool expectImmediateApply = preCycles != 74;

			EXPECT_EQ(state.HmoveApplyCount, expectImmediateApply ? 1u : 0u)
				<< "pre_cycles=" << preCycles << ";scanline=" << state.Scanline << ";clock=" << state.ColorClock;
			EXPECT_EQ(state.HmovePending, !expectImmediateApply)
				<< "pre_cycles=" << preCycles << ";pending=" << state.HmovePending;

			if (!expectImmediateApply) {
				console.StepCpuCycles(1);
				Atari2600TiaState deferredApplied = console.GetTiaState();
				EXPECT_EQ(deferredApplied.HmoveApplyCount, 1u)
					<< "pre_cycles=" << preCycles << ";deferred_scanline=" << deferredApplied.Scanline;
				EXPECT_FALSE(deferredApplied.HmovePending);
			}
		}
	}

	TEST(Atari2600TiaPhaseATests, WsyncHmoveOrderingDigestIsDeterministicAcrossRuns) {
		auto mixDigest = [](uint64_t hash, uint64_t value) {
			hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
			return hash;
		};

		auto runDigest = [&]() {
			uint64_t digest = 0xcbf29ce484222325ULL;

			for (uint32_t preCycles = 70; preCycles <= 76; preCycles++) {
				for (uint32_t ordering = 0; ordering < 2; ordering++) {
					Emulator emu;
					Atari2600Console console(&emu);
					console.Reset();

					console.StepCpuCycles(preCycles);
					if (ordering == 0) {
						console.RequestWsync();
						console.RequestHmove();
					} else {
						console.RequestHmove();
						console.RequestWsync();
					}

					console.StepCpuCycles(1);
					console.StepCpuCycles(1);

					Atari2600TiaState state = console.GetTiaState();
					digest = mixDigest(digest, preCycles);
					digest = mixDigest(digest, ordering);
					digest = mixDigest(digest, state.Scanline);
					digest = mixDigest(digest, state.ColorClock);
					digest = mixDigest(digest, state.WsyncCount);
					digest = mixDigest(digest, state.HmoveStrobeCount);
					digest = mixDigest(digest, state.HmoveApplyCount);
					digest = mixDigest(digest, state.TotalColorClocks);
				}
			}

			return digest;
		};

		uint64_t runA = runDigest();
		uint64_t runB = runDigest();
		EXPECT_EQ(runA, runB);
		EXPECT_NE(runA, 0u);
	}

	TEST(Atari2600TiaPhaseATests, WsyncHmoveOrderingMatrixKeepsStableJitterWindows) {
		auto runOrdering = [](uint32_t preCycles, bool wsyncFirst) {
			Emulator emu;
			Atari2600Console console(&emu);
			console.Reset();

			console.StepCpuCycles(preCycles);
			if (wsyncFirst) {
				console.RequestWsync();
				console.RequestHmove();
			} else {
				console.RequestHmove();
				console.RequestWsync();
			}

			console.StepCpuCycles(1);
			return console.GetTiaState();
		};

		for (uint32_t preCycles = 72; preCycles <= 76; preCycles++) {
			SCOPED_TRACE(string("pre_cycles=") + std::to_string(preCycles));
			Atari2600TiaState wsyncThenHmove = runOrdering(preCycles, true);
			Atari2600TiaState hmoveThenWsync = runOrdering(preCycles, false);

			EXPECT_EQ(wsyncThenHmove.Scanline, hmoveThenWsync.Scanline)
				<< "pre_cycles=" << preCycles << ";field=scanline";
			EXPECT_EQ(wsyncThenHmove.ColorClock, hmoveThenWsync.ColorClock)
				<< "pre_cycles=" << preCycles << ";field=color_clock";
			EXPECT_EQ(wsyncThenHmove.HmoveApplyCount, hmoveThenWsync.HmoveApplyCount)
				<< "pre_cycles=" << preCycles << ";field=hmove_apply";
			EXPECT_EQ(wsyncThenHmove.WsyncCount, 1u)
				<< "pre_cycles=" << preCycles << ";field=wsync_count";
			EXPECT_EQ(hmoveThenWsync.WsyncCount, 1u)
				<< "pre_cycles=" << preCycles << ";field=wsync_count_mirrored";
		}
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

	TEST(Atari2600TiaPhaseATests, Refp0ChangesPlayerPlayfieldCollisionEdge) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "tia-refp-collision.a26");

		console.DebugWriteCartridge(0x0010, 0x00); // resp0 => x=0
		console.DebugWriteCartridge(0x000D, 0x10); // pf0 left-most playfield bit at x=0
		console.DebugWriteCartridge(0x000E, 0x00);
		console.DebugWriteCartridge(0x000F, 0x00);
		console.DebugWriteCartridge(0x001B, 0x80); // left-most player bit only

		console.DebugWriteCartridge(0x000B, 0x00); // refp0 off
		console.RunFrame();
		EXPECT_TRUE((console.DebugReadCartridge(0x0002) & 0x80) != 0); // cxp0fb p0-pf

		console.DebugWriteCartridge(0x002C, 0x00); // cxclr
		console.DebugWriteCartridge(0x000B, 0x08); // refp0 on
		console.RunFrame();
		EXPECT_EQ(console.DebugReadCartridge(0x0002), 0u);
	}

	TEST(Atari2600TiaPhaseATests, NusizMissileCopyModeCanLatchCxm0p) {
		Emulator emu;
		Atari2600Console console(&emu);
		LoadNopRom(console, "tia-collision-nusiz-copy.a26");

		console.DebugWriteCartridge(0x001B, 0x00); // grp0 off
		console.DebugWriteCartridge(0x001C, 0xFF); // grp1 on at default x=96
		console.DebugWriteCartridge(0x001D, 0x02); // enam0
		console.DebugWriteCartridge(0x0004, 0x04); // nusiz0: copies at 0 and +64

		console.RunFrame();

		EXPECT_TRUE((console.DebugReadCartridge(0x0000) & 0x80) != 0); // cxm0p missile0-player1
	}

	TEST(Atari2600TiaPhaseATests, HmxxRegistersApplyDisplacementOnHmove) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0020, 0x10); // hmp0 +1
		console.DebugWriteCartridge(0x0021, 0x20); // hmp1 +2
		console.DebugWriteCartridge(0x0022, 0x30); // hmm0 +3
		console.DebugWriteCartridge(0x0023, 0x40); // hmm1 +4
		console.DebugWriteCartridge(0x0024, 0xE0); // hmbl -2
		console.DebugWriteCartridge(0x002A, 0x00); // hmove
		console.StepCpuCycles(1);

		Atari2600TiaState tiaState = console.GetTiaState();
		EXPECT_EQ(tiaState.Player0X, 25u);
		EXPECT_EQ(tiaState.Player1X, 98u);
		EXPECT_EQ(tiaState.Missile0X, 35u);
		EXPECT_EQ(tiaState.Missile1X, 108u);
		EXPECT_EQ(tiaState.BallX, 78u);
	}

	TEST(Atari2600TiaPhaseATests, HmxxPositiveDisplacementWrapsAtScreenEdge) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.StepCpuCycles(53);                // color clock 159
		console.DebugWriteCartridge(0x0010, 0x00); // resp0 => x=159
		console.DebugWriteCartridge(0x0020, 0x20); // hmp0 +2
		console.DebugWriteCartridge(0x002A, 0x00); // hmove
		console.StepCpuCycles(1);

		EXPECT_EQ(console.GetTiaState().Player0X, 1u);
	}

	TEST(Atari2600TiaPhaseATests, HmclrClearsMotionRegistersBeforeHmove) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0020, 0x30); // hmp0 +3
		console.DebugWriteCartridge(0x002B, 0x00); // hmclr
		console.DebugWriteCartridge(0x002A, 0x00); // hmove
		console.StepCpuCycles(1);

		EXPECT_EQ(console.GetTiaState().Player0X, 24u);
	}

	TEST(Atari2600TiaPhaseATests, VdelPlayerUsesPreviousScanlineGraphics) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0025, 0x01); // vdelp0 on
		console.DebugWriteCartridge(0x001B, 0xF0); // grp0 on scanline 0
		console.StepCpuCycles(76);                 // advance to scanline 1
		console.DebugWriteCartridge(0x001B, 0x0F); // current grp0 differs

		Atari2600ScanlineRenderState delayedState = console.DebugGetScanlineRenderState(1);
		EXPECT_EQ(delayedState.Player0Graphics, 0xF0u);

		console.DebugWriteCartridge(0x0025, 0x00); // vdelp0 off
		Atari2600ScanlineRenderState immediateState = console.DebugGetScanlineRenderState(1);
		EXPECT_EQ(immediateState.Player0Graphics, 0x0Fu);
	}

	TEST(Atari2600TiaPhaseATests, VdelBallUsesPreviousScanlineEnableState) {
		Emulator emu;
		Atari2600Console console(&emu);

		console.Reset();
		console.DebugWriteCartridge(0x0027, 0x01); // vdelbl on
		console.DebugWriteCartridge(0x001F, 0x02); // enabl on scanline 0
		console.StepCpuCycles(76);                 // advance to scanline 1
		console.DebugWriteCartridge(0x001F, 0x00); // disable current ball

		Atari2600ScanlineRenderState delayedState = console.DebugGetScanlineRenderState(1);
		EXPECT_TRUE(delayedState.BallEnabled);

		console.DebugWriteCartridge(0x0027, 0x00); // vdelbl off
		Atari2600ScanlineRenderState immediateState = console.DebugGetScanlineRenderState(1);
		EXPECT_FALSE(immediateState.BallEnabled);
	}

	TEST(Atari2600TiaPhaseATests, VdelGraphicsCollisionLatchesRemainDeterministic) {
		auto runScenario = []() {
			Emulator emu;
			Atari2600Console console(&emu);
			LoadNopRom(console, "tia-vdel-collision.a26");

			console.DebugWriteCartridge(0x000D, 0xF0);
			console.DebugWriteCartridge(0x000E, 0xFF);
			console.DebugWriteCartridge(0x000F, 0xFF);
			console.DebugWriteCartridge(0x0010, 0x00); // resp0 at x=0
			console.DebugWriteCartridge(0x0025, 0x01); // vdelp0 enabled
			console.DebugWriteCartridge(0x001B, 0xFF); // scanline 0 player visible
			console.StepCpuCycles(76);                 // delayed latch captures 0xFF
			console.DebugWriteCartridge(0x001B, 0x00); // current scanline player hidden
			console.RunFrame();

			return console.DebugReadCartridge(0x0002); // cxp0fb
		};

		uint8_t runA = runScenario();
		uint8_t runB = runScenario();
		EXPECT_EQ(runA, runB);
	}
}
