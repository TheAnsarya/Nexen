#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	// Genesis NTSC timing: 488 cycles per scanline, 262 scanlines per frame.
	// Active display: scanlines 0-223, VBlank: 224-261.
	static constexpr uint32_t CyclesPerScanline = 488;
	static constexpr uint32_t ActiveScanlines = 224;

	// Helper: write a VDP register via control port (0x8Rxx format).
	static void WriteVdpRegister(GenesisPlatformBusStub& bus, uint8_t reg, uint8_t value) {
		uint8_t hi = (uint8_t)(0x80 | (reg & 0x1f));
		bus.WriteByte(0xC00004, hi);
		bus.WriteByte(0xC00005, value);
	}

	// Helper: write a VDP command word via control port.
	static void WriteVdpCommand(GenesisPlatformBusStub& bus, uint16_t word) {
		bus.WriteByte(0xC00004, (uint8_t)(word >> 8));
		bus.WriteByte(0xC00005, (uint8_t)(word & 0xff));
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, RegisterWriteDuringActiveScanlineUpdatesRegisterFile) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to scanline 10 (mid-active-display).
		scaffold.StepFrameScaffold(CyclesPerScanline * 10);
		EXPECT_EQ(scaffold.GetTimingScanline(), 10u);

		// Write reg7 (backdrop color index) = 0x2a during active display.
		WriteVdpRegister(scaffold.GetBus(), 7, 0x2a);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(7), 0x2au);

		// Write reg1 display enable during active display.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x40);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x40u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, MidFrameDisplayEnableToggleAffectsStatusBit) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to scanline 50.
		scaffold.StepFrameScaffold(CyclesPerScanline * 50);

		// Enable display → status bit 3 set.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x40);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);

		// Advance a few more scanlines while display is still active.
		scaffold.StepFrameScaffold(CyclesPerScanline * 5);

		// Disable display mid-frame → status bit 3 cleared.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x00);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) == 0);

		// Re-enable → status bit 3 set again.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x40);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, MultipleRegisterWritesWithinSingleScanlineAllApplied) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to scanline 20.
		scaffold.StepFrameScaffold(CyclesPerScanline * 20);
		uint32_t scanlineBefore = scaffold.GetTimingScanline();
		EXPECT_EQ(scanlineBefore, 20u);

		// Write multiple registers without advancing scanline.
		WriteVdpRegister(scaffold.GetBus(), 0, 0x04);
		WriteVdpRegister(scaffold.GetBus(), 1, 0x64);
		WriteVdpRegister(scaffold.GetBus(), 2, 0x30);
		WriteVdpRegister(scaffold.GetBus(), 3, 0x3c);
		WriteVdpRegister(scaffold.GetBus(), 4, 0x07);
		WriteVdpRegister(scaffold.GetBus(), 7, 0x15);
		WriteVdpRegister(scaffold.GetBus(), 10, 0x0a);
		WriteVdpRegister(scaffold.GetBus(), 15, 0x02);

		// All registers should reflect the written values.
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(0), 0x04u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x64u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(2), 0x30u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(3), 0x3cu);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(4), 0x07u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(7), 0x15u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(10), 0x0au);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(15), 0x02u);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, RegisterWriteOrderingDeterminesRenderOutput) {
		// Scenario A: render with composition inputs, then change inputs and re-render.
		GenesisM68kBoundaryScaffold scaffoldA;
		scaffoldA.Startup();
		scaffoldA.StepFrameScaffold(CyclesPerScanline * 30);
		scaffoldA.GetBus().SetRenderCompositionInputs(0x12, true, 0x08, false, 0x00, false, false, 0x00, false);
		scaffoldA.GetBus().SetScroll(0, 0);
		scaffoldA.GetBus().RenderScaffoldLine(32);
		string digestA1 = scaffoldA.GetBus().GetRenderLineDigest();
		// Change composition inputs mid-frame.
		scaffoldA.GetBus().SetRenderCompositionInputs(0x30, false, 0x1a, true, 0x00, false, false, 0x00, false);
		scaffoldA.GetBus().RenderScaffoldLine(32);
		string digestA2 = scaffoldA.GetBus().GetRenderLineDigest();

		// Different composition inputs should produce different digest.
		EXPECT_NE(digestA1, digestA2);

		// Scenario B: same operations, same order → identical digests (determinism).
		GenesisM68kBoundaryScaffold scaffoldB;
		scaffoldB.Startup();
		scaffoldB.StepFrameScaffold(CyclesPerScanline * 30);
		scaffoldB.GetBus().SetRenderCompositionInputs(0x12, true, 0x08, false, 0x00, false, false, 0x00, false);
		scaffoldB.GetBus().SetScroll(0, 0);
		scaffoldB.GetBus().RenderScaffoldLine(32);
		string digestB1 = scaffoldB.GetBus().GetRenderLineDigest();
		scaffoldB.GetBus().SetRenderCompositionInputs(0x30, false, 0x1a, true, 0x00, false, false, 0x00, false);
		scaffoldB.GetBus().RenderScaffoldLine(32);
		string digestB2 = scaffoldB.GetBus().GetRenderLineDigest();

		EXPECT_EQ(digestA1, digestB1);
		EXPECT_EQ(digestA2, digestB2);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, MidFrameScrollChangeProducesDifferentRenderDigest) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().SetRenderCompositionInputs(0x18, true, 0x05, false, 0x00, false, false, 0x00, false);

		// Render at scanline 40 with scroll (0,0).
		scaffold.StepFrameScaffold(CyclesPerScanline * 40);
		scaffold.GetBus().SetScroll(0, 0);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestBefore = scaffold.GetBus().GetRenderLineDigest();

		// Advance to scanline 80 and render with different scroll.
		scaffold.StepFrameScaffold(CyclesPerScanline * 40);
		scaffold.GetBus().SetScroll(7, 3);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestAfter = scaffold.GetBus().GetRenderLineDigest();

		EXPECT_NE(digestBefore, digestAfter);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, StatusReadClearsPendingBitDuringActiveDisplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to active display.
		scaffold.StepFrameScaffold(CyclesPerScanline * 15);

		// Write a command word to set pending bit via side effects.
		WriteVdpCommand(scaffold.GetBus(), 0x4000);

		// First status read should see sticky pending bit; second should clear it.
		uint8_t statusFirst = scaffold.GetBus().ReadByte(0xC00004);
		uint8_t statusSecond = scaffold.GetBus().ReadByte(0xC00004);

		EXPECT_TRUE((statusFirst & 0x80u) != 0);
		EXPECT_TRUE((statusSecond & 0x80u) == 0);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, CommandWordDuringActiveDisplaySetsStatusBits) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to mid-active-display.
		scaffold.StepFrameScaffold(CyclesPerScanline * 100);

		// Write a command class word.
		WriteVdpCommand(scaffold.GetBus(), 0x4abc);
		uint16_t status = scaffold.GetBus().GetVdpStatus();

		// Command class sets bit 15, clears bit 0, sets bit 1.
		EXPECT_TRUE((status & 0x8000u) != 0);
		EXPECT_TRUE((status & 0x0001u) == 0);
		EXPECT_TRUE((status & 0x0002u) != 0);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, InterleavedRegisterAndCommandWordsDuringActiveScanline) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to scanline 60.
		scaffold.StepFrameScaffold(CyclesPerScanline * 60);

		// Step 1: Enable display.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x40);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x40u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);

		// Step 2: Command word.
		WriteVdpCommand(scaffold.GetBus(), 0x4000);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x8000u) != 0);

		// Step 3: Write backdrop color register.
		WriteVdpRegister(scaffold.GetBus(), 7, 0x3f);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(7), 0x3fu);

		// Step 4: Disable display. Status bit 3 should clear even though
		// other status bits were set by the command word.
		WriteVdpRegister(scaffold.GetBus(), 1, 0x00);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) == 0);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x00u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(7), 0x3fu);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, RenderCompositionChangesBetweenScanlinesDuringActiveDisplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Advance to scanline 30 and render with composition A.
		scaffold.StepFrameScaffold(CyclesPerScanline * 30);
		scaffold.GetBus().SetRenderCompositionInputs(0x10, true, 0x00, false, 0x00, false, false, 0x00, false);
		scaffold.GetBus().SetScroll(0, 0);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestA = scaffold.GetBus().GetRenderLineDigest();

		// Advance to scanline 60 and render with composition B.
		scaffold.StepFrameScaffold(CyclesPerScanline * 30);
		scaffold.GetBus().SetRenderCompositionInputs(0x00, false, 0x20, true, 0x00, false, false, 0x00, false);
		scaffold.GetBus().SetScroll(0, 0);
		scaffold.GetBus().RenderScaffoldLine(32);
		string digestB = scaffold.GetBus().GetRenderLineDigest();

		// Different composition inputs → different digest.
		EXPECT_NE(digestA, digestB);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, RegisterWritesDuringVBlankAndActiveDisplayBothTakeEffect) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		// Write reg2 during VBlank (scanline 0 is technically the first active line,
		// but after a full frame we are in VBlank territory).
		scaffold.StepFrameScaffold(CyclesPerScanline * ActiveScanlines);
		EXPECT_GE(scaffold.GetTimingScanline(), ActiveScanlines);
		WriteVdpRegister(scaffold.GetBus(), 2, 0x38);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(2), 0x38u);

		// Wrap to next frame, advance to active display scanline 10.
		scaffold.StepFrameScaffold(CyclesPerScanline * (262 - ActiveScanlines + 10));
		EXPECT_EQ(scaffold.GetTimingScanline(), 10u);
		EXPECT_EQ(scaffold.GetTimingFrame(), 1u);

		// Reg2 still has the VBlank-written value.
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(2), 0x38u);

		// Overwrite during active display.
		WriteVdpRegister(scaffold.GetBus(), 2, 0x30);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(2), 0x30u);
	}

	TEST(GenesisVdpActiveScanlineRegisterTests, ActiveScanlineRegisterWriteFuzzIsDeterministic) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();

			scaffold.GetBus().SetRenderCompositionInputs(0x14, true, 0x08, false, 0x00, false, false, 0x00, false);

			uint32_t seed = 0xa5c3e1f7u;
			uint64_t hash = 1469598103934665603ull; // FNV offset basis

			for (uint32_t scanline = 0; scanline < 128; scanline++) {
				scaffold.StepFrameScaffold(CyclesPerScanline);

				// Pseudo-random register writes each scanline.
				seed = seed * 1664525u + 1013904223u;
				uint8_t reg = (uint8_t)((seed >> 24) & 0x1fu);
				uint8_t value = (uint8_t)((seed >> 8) & 0xffu);
				WriteVdpRegister(scaffold.GetBus(), reg, value);

				// Every 4th scanline, write a command word too.
				if ((scanline % 4u) == 0u) {
					seed = seed * 1664525u + 1013904223u;
					uint16_t cmd = (uint16_t)(0x4000u | (seed & 0x3fffu));
					WriteVdpCommand(scaffold.GetBus(), cmd);
				}

				// Every 8th scanline, change scroll and render a line.
				if ((scanline % 8u) == 0u) {
					seed = seed * 1664525u + 1013904223u;
					uint16_t sx = (uint16_t)(seed & 0x1ff);
					uint16_t sy = (uint16_t)((seed >> 12) & 0x1ff);
					scaffold.GetBus().SetScroll(sx, sy);
					scaffold.GetBus().RenderScaffoldLine(32);

					const string& digest = scaffold.GetBus().GetRenderLineDigest();
					for (uint8_t ch : digest) {
						hash ^= ch;
						hash *= 1099511628211ull;
					}
				}

				// Hash register state every scanline.
				uint16_t status = scaffold.GetBus().GetVdpStatus();
				uint8_t reg1 = scaffold.GetBus().GetVdpRegister(1);
				uint8_t regCurrent = scaffold.GetBus().GetVdpRegister(reg);

				string line = std::format("{}:{:04x}:{:02x}:{:02x}", scanline, status, reg1, regCurrent);
				for (uint8_t ch : line) {
					hash ^= ch;
					hash *= 1099511628211ull;
				}
			}

			return std::make_tuple(
				hash,
				scaffold.GetBus().GetVdpStatus(),
				scaffold.GetBus().GetVdpControlWordLatch(),
				scaffold.GetBus().GetRenderLineDigest()
			);
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(std::get<0>(runA), std::get<0>(runB));
		EXPECT_EQ(std::get<1>(runA), std::get<1>(runB));
		EXPECT_EQ(std::get<2>(runA), std::get<2>(runB));
		EXPECT_EQ(std::get<3>(runA), std::get<3>(runB));
	}
}
