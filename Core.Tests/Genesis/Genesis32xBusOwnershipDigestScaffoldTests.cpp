#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	void RunOwnershipScenario(GenesisM68kBoundaryScaffold& scaffold) {
		auto& bus = scaffold.GetBus();
		bus.WriteByte(0xA11200, 0x01);
		(void)bus.ReadByte(0xA11200);
		bus.WriteByte(0xA11100, 0x01);
		(void)bus.ReadByte(0xA00000);
		bus.WriteByte(0xA11100, 0x00);
		(void)bus.ReadByte(0xA00000);
		bus.WriteByte(0xC00004, 0x80);
		(void)bus.ReadByte(0xC00004);
		bus.WriteByte(0xFF1234, 0x7A);
		(void)bus.ReadByte(0xFF1234);
		(void)bus.ReadByte(0x800000);
		scaffold.StepFrameScaffold(96);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, OwnershipTraceDigestIsDeterministicAcrossRepeatedRuns) {
		auto run = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();
			scaffold.GetBus().ClearOwnershipTrace();
			RunOwnershipScenario(scaffold);
			return std::tuple<uint32_t, string, uint32_t>(
				scaffold.GetBus().GetOwnershipTraceCount(),
				scaffold.GetBus().GetOwnershipTraceDigest(),
				scaffold.GetBus().GetZ80HandoffCount());
		};

		auto runA = run();
		auto runB = run();

		EXPECT_EQ(runA, runB);
		EXPECT_GT(std::get<0>(runA), 0u);
		EXPECT_FALSE(std::get<1>(runA).empty());
		EXPECT_GE(std::get<2>(runA), 2u);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, OwnershipTraceClearResetsDigestAndCounter) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		RunOwnershipScenario(scaffold);
		EXPECT_GT(bus.GetOwnershipTraceCount(), 0u);
		EXPECT_FALSE(bus.GetOwnershipTraceDigest().empty());

		bus.ClearOwnershipTrace();
		EXPECT_EQ(bus.GetOwnershipTraceCount(), 0u);
		EXPECT_TRUE(bus.GetOwnershipTraceDigest().empty());

		(void)bus.ReadByte(0x000100);
		EXPECT_EQ(bus.GetOwnershipTraceCount(), 1u);
		EXPECT_FALSE(bus.GetOwnershipTraceDigest().empty());
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, OwnershipTraceDigestReplayMatchesAfterSaveLoad) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();
		bus.ClearOwnershipTrace();

		RunOwnershipScenario(scaffold);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		RunOwnershipScenario(scaffold);
		string baselineDigest = bus.GetOwnershipTraceDigest();
		uint32_t baselineCount = bus.GetOwnershipTraceCount();

		scaffold.LoadState(checkpoint);
		RunOwnershipScenario(scaffold);

		EXPECT_EQ(bus.GetOwnershipTraceDigest(), baselineDigest);
		EXPECT_EQ(bus.GetOwnershipTraceCount(), baselineCount);
	}
}
