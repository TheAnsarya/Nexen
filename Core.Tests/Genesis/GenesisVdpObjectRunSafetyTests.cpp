#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisM68k.h"
#include "Shared/Emulator.h"

namespace {
	TEST(GenesisVdpObjectRunSafetyTests, RunOneScanlinePreservesNullDependencySafetyPreconditions) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		constexpr uint64_t kClocksPerScanline = 488;
		GenesisVdpState before = vdp.GetState();
		EXPECT_EQ(before.Registers[1] & 0x20, 0);
		EXPECT_EQ(before.Registers[0] & 0x10, 0);
		EXPECT_FALSE(before.DmaActive);

		vdp.Run(kClocksPerScanline);

		GenesisVdpState after = vdp.GetState();
		EXPECT_FALSE(after.DmaActive);
		EXPECT_EQ(after.Registers[1] & 0x20, 0);
		EXPECT_EQ(after.Registers[0] & 0x10, 0);
	}

	TEST(GenesisVdpObjectRunSafetyTests, HIntCounterUsesRuntimeCounterWhileKeepingR10Stable) {
		GenesisM68k cpu;
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, &cpu, nullptr);

		// Enable HBlank interrupt path (R0 bit 4) and set R10 counter to 2.
		vdp.WriteControlPort(0x8012);
		vdp.WriteControlPort(0x8A02);

		constexpr uint64_t kClocksPerScanline = 488;
		vdp.Run(kClocksPerScanline * 2);

		GenesisVdpState state = vdp.GetState();
		EXPECT_EQ(state.Registers[10], 0x02);
		EXPECT_EQ(state.HIntCounter, 0x00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, SetRegionTogglesPalStatusBit) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		GenesisVdpState initial = vdp.GetState();
		EXPECT_EQ(initial.StatusRegister & VdpStatus::PalMode, 0u);

		vdp.SetRegion(true);
		EXPECT_EQ(vdp.GetScreenHeight(), 240u);
		GenesisVdpState palState = vdp.GetState();
		EXPECT_NE(palState.StatusRegister & VdpStatus::PalMode, 0u);

		vdp.SetRegion(false);
		EXPECT_EQ(vdp.GetScreenHeight(), 224u);
		GenesisVdpState ntscState = vdp.GetState();
		EXPECT_EQ(ntscState.StatusRegister & VdpStatus::PalMode, 0u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, ResetAppliesStartupDefaultsForModeAndAutoIncrement) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		GenesisVdpState state = vdp.GetState();
		EXPECT_EQ(state.Registers[1], 0x04);
		EXPECT_EQ(state.Registers[12], 0x81);
		EXPECT_EQ(state.Registers[15], 0x02);
		EXPECT_NE(state.StatusRegister & VdpStatus::FifoEmpty, 0u);
		EXPECT_EQ(vdp.GetScreenWidth(), 320u);
	}

	TEST(GenesisVdpObjectRunSafetyTests, StartupDisplayEnableCanRenderFirstVisibleScanline) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		// CRAM color format is 0000BBB0GGG0RRR0; this sets full red.
		vdp.GetCramPointer()[0] = 0x000e;
		// Enable display (R1 bit 6) while preserving existing startup bits.
		vdp.WriteControlPort(0x8144);
		vdp.Run(488);

		uint16_t* frame = vdp.GetScreenBuffer(false);
		EXPECT_EQ(frame[0], 0x7c00);
	}

	TEST(GenesisVdpObjectRunSafetyTests, VBlankAndVIntStartAtFirstVBlankLineBoundary) {
		Emulator emu;
		emu.Initialize(false);
		GenesisM68k cpu;
		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, &cpu, nullptr);

		// Enable VBlank interrupt while keeping display disabled startup mode.
		vdp.WriteControlPort(0x8124);

		GenesisVdpState prev = vdp.GetState();
		bool reachedVBlankBoundary = false;
		constexpr uint32_t MaxCycles = 200000;

		for (uint32_t cycle = 1; cycle <= MaxCycles; cycle++) {
			vdp.Run(cycle);
			GenesisVdpState curr = vdp.GetState();

			if (curr.VCounter == 224 && curr.HCounter == 0) {
				reachedVBlankBoundary = true;
				EXPECT_EQ(prev.VCounter, 223u);
				EXPECT_EQ(prev.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_NE(curr.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_NE(curr.StatusRegister & VdpStatus::VIntPending, 0u);
				break;
			}

			prev = curr;
		}

		EXPECT_TRUE(reachedVBlankBoundary);
	}

	TEST(GenesisVdpObjectRunSafetyTests, FrameStartBoundaryClearsVBlankFlag) {
		Emulator emu;
		emu.Initialize(false);
		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, nullptr);

		GenesisVdpState prev = vdp.GetState();
		bool reachedFrameStartBoundary = false;
		constexpr uint32_t MaxCycles = 300000;

		for (uint32_t cycle = 1; cycle <= MaxCycles; cycle++) {
			vdp.Run(cycle);
			GenesisVdpState curr = vdp.GetState();

			if (curr.FrameCount > 0 && curr.VCounter == 0 && curr.HCounter == 0) {
				reachedFrameStartBoundary = true;
				EXPECT_EQ(prev.VCounter, 261u);
				EXPECT_NE(prev.StatusRegister & VdpStatus::VBlankFlag, 0u);
				EXPECT_EQ(curr.StatusRegister & VdpStatus::VBlankFlag, 0u);
				break;
			}

			prev = curr;
		}

		EXPECT_TRUE(reachedFrameStartBoundary);
	}
}
