#include "pch.h"
#include "Genesis/GenesisVdp.h"

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
}
