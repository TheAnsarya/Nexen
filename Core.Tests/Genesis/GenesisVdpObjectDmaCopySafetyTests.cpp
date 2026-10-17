#include "pch.h"
#include "Genesis/GenesisVdp.h"

namespace {
	TEST(GenesisVdpObjectDmaCopySafetyTests, DmaCopyRegisterSequenceCompletesWithoutMemoryManagerDependency) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		// Mirror benchmark setup path: enable DMA and select copy mode in reg 23.
		vdp.WriteControlPort(0x8110);
		vdp.WriteControlPort(0x97c0);

		GenesisVdpState setupState = vdp.GetState();
		EXPECT_NE(setupState.Registers[1] & 0x10, 0);
		EXPECT_EQ((setupState.Registers[23] >> 6) & 0x03, 0x03);

		// Configure source/destination and transfer length, then trigger DMA.
		vdp.WriteControlPort(0x9500);
		vdp.WriteControlPort(0x9601);
		vdp.WriteControlPort(0x9300);
		vdp.WriteControlPort(0x9401);
		vdp.WriteControlPort(0x6000);
		vdp.WriteControlPort(0x0080);

		GenesisVdpState preRunState = vdp.GetState();
		EXPECT_TRUE(preRunState.DmaActive);

		// One scanline is sufficient for this simplified VDP model to process DMA.
		vdp.Run(488);

		GenesisVdpState postRunState = vdp.GetState();
		EXPECT_FALSE(postRunState.DmaActive);
		EXPECT_EQ((postRunState.Registers[23] >> 6) & 0x03, 0x03);
	}
}
