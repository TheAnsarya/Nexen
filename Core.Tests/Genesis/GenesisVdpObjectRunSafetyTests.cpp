#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisM68k.h"

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
}
