#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisZ80HandoffTests, ResetReleaseBootstrapsAndStartsZ80) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		EXPECT_TRUE(scaffold.GetBus().IsZ80Bootstrapped());
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());
		EXPECT_EQ(scaffold.GetBus().GetZ80BootstrapCount(), 1u);
	}

	TEST(GenesisZ80HandoffTests, BusRequestHaltsAndReleaseResumesZ80Execution) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		scaffold.StepFrameScaffold(40);
		uint64_t runningCycles = scaffold.GetBus().GetZ80ExecutedCycles();
		EXPECT_GT(runningCycles, 0u);

		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		scaffold.StepFrameScaffold(40);
		uint64_t haltedCycles = scaffold.GetBus().GetZ80ExecutedCycles();
		EXPECT_EQ(runningCycles, haltedCycles);
		EXPECT_FALSE(scaffold.GetBus().IsZ80Running());

		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		scaffold.StepFrameScaffold(40);
		EXPECT_GT(scaffold.GetBus().GetZ80ExecutedCycles(), haltedCycles);
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());
		EXPECT_GE(scaffold.GetBus().GetZ80HandoffCount(), 2u);
	}

	TEST(GenesisZ80HandoffTests, Z80WindowAccessReflectsBusHandoffState) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		scaffold.GetBus().WriteByte(0xA11200, 0x01);

		uint8_t blockedValue = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(blockedValue, 0xFFu);

		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		uint8_t handoffValue = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(handoffValue, 0x00u);
	}
}
