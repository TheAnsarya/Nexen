#include "pch.h"
#include "Genesis/GenesisVdp.h"

namespace {
	TEST(GenesisVdpObjectDmaSafetyTests, DmaFillRegisterSequenceCompletesWithoutMemoryManagerDependency) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		// Mirror the benchmark setup path: enable DMA and select fill mode in reg 23.
		vdp.WriteControlPort(0x8110);
		vdp.WriteControlPort(0x9780);

		GenesisVdpState setupState = vdp.GetState();
		EXPECT_EQ((setupState.Registers[23] >> 6) & 0x03, 0x02);

		// Configure transfer length and trigger DMA via two-word control write.
		vdp.WriteControlPort(0x8d00);
		vdp.WriteControlPort(0x9300);
		vdp.WriteControlPort(0x9402);
		vdp.WriteControlPort(0x4000);
		vdp.WriteControlPort(0x0080);

		GenesisVdpState preRunState = vdp.GetState();
		EXPECT_TRUE(preRunState.DmaActive);

		// One scanline is sufficient for this simplified VDP model to process DMA.
		vdp.Run(488);

		GenesisVdpState postRunState = vdp.GetState();
		EXPECT_FALSE(postRunState.DmaActive);
		EXPECT_EQ((postRunState.Registers[23] >> 6) & 0x03, 0x02);
	}
}
