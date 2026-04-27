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

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, DualSh2StagingStatusReflectsMasterSlaveAndSyncPhase) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15012, 0x03);
		bus.WriteByte(0xA15013, 0x07);
		bus.WriteByte(0xA15014, 0x52);

		uint8_t status = bus.ReadByte(0xA1501A);
		uint8_t digest = bus.ReadByte(0xA1501B);

		EXPECT_EQ(status & 0x03, 0x03);
		EXPECT_EQ((status >> 2) & 0x0F, 0x07);
		EXPECT_NE(digest, 0x00u);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, DualSh2StagingDigestIsDeterministicAcrossSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15012, 0x01);
		bus.WriteByte(0xA15013, 0x03);
		bus.WriteByte(0xA15014, 0x10);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		bus.WriteByte(0xA15012, 0x03);
		bus.WriteByte(0xA15013, 0x05);
		bus.WriteByte(0xA15014, 0x80);
		uint8_t baselineStatus = bus.ReadByte(0xA1501A);
		uint8_t baselineDigest = bus.ReadByte(0xA1501B);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA15012, 0x03);
		bus.WriteByte(0xA15013, 0x05);
		bus.WriteByte(0xA15014, 0x80);

		EXPECT_EQ(bus.ReadByte(0xA1501A), baselineStatus);
		EXPECT_EQ(bus.ReadByte(0xA1501B), baselineDigest);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, CompositionStatusTracksBlendAndFrameSyncMarkers) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15012, 0x03);
		bus.WriteByte(0xA15013, 0x04);
		bus.WriteByte(0xA15016, 0x0A);
		bus.WriteByte(0xA15017, 0xB7);

		uint8_t status = bus.ReadByte(0xA1501C);
		uint8_t digest = bus.ReadByte(0xA1501D);

		EXPECT_EQ(status & 0x0F, 0x0Au);
		EXPECT_NE((status >> 6) & 0x03, 0x00u);
		EXPECT_NE(digest, 0x00u);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, CompositionDigestIsDeterministicAcrossSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15012, 0x03);
		bus.WriteByte(0xA15016, 0x04);
		bus.WriteByte(0xA15017, 0x31);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		bus.WriteByte(0xA15016, 0x0C);
		bus.WriteByte(0xA15017, 0x99);
		uint8_t baselineStatus = bus.ReadByte(0xA1501C);
		uint8_t baselineDigest = bus.ReadByte(0xA1501D);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA15016, 0x0C);
		bus.WriteByte(0xA15017, 0x99);

		EXPECT_EQ(bus.ReadByte(0xA1501C), baselineStatus);
		EXPECT_EQ(bus.ReadByte(0xA1501D), baselineDigest);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, ToolingStatusExposesCapabilitiesAndDeterministicDigest) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15008, 0x10);
		bus.WriteByte(0xA15009, 0x20);
		bus.WriteByte(0xA1500A, 0x30);
		bus.WriteByte(0xA1500B, 0x40);

		EXPECT_EQ(bus.ReadByte(0xA1501E), 0x0Fu);
		EXPECT_NE(bus.ReadByte(0xA1501F), 0x00u);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, ToolingDigestIsDeterministicAcrossSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15016, 0x03);
		bus.WriteByte(0xA15017, 0x12);
		bus.WriteByte(0xA15008, 0x08);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		bus.WriteByte(0xA15009, 0x19);
		bus.WriteByte(0xA1500A, 0x2A);
		bus.WriteByte(0xA1500B, 0x3B);
		uint8_t baselineCaps = bus.ReadByte(0xA1501E);
		uint8_t baselineDigest = bus.ReadByte(0xA1501F);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA15009, 0x19);
		bus.WriteByte(0xA1500A, 0x2A);
		bus.WriteByte(0xA1500B, 0x3B);

		EXPECT_EQ(bus.ReadByte(0xA1501E), baselineCaps);
		EXPECT_EQ(bus.ReadByte(0xA1501F), baselineDigest);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, SegaCdToolingTasCheatSignalsAreDeterministicAcrossSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA12012, 0x10);
		bus.WriteByte(0xA12014, 0x20);
		uint8_t digestBefore = bus.ReadByte(0xA1201B);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		bus.WriteByte(0xA12013, 0x35);
		bus.WriteByte(0xA12015, 0xA7);
		uint8_t baselineTas = bus.ReadByte(0xA12013);
		uint8_t baselineCheat = bus.ReadByte(0xA12015);
		uint8_t baselineDigest = bus.ReadByte(0xA1201B);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA12013, 0x35);
		bus.WriteByte(0xA12015, 0xA7);

		EXPECT_EQ(bus.ReadByte(0xA12013), baselineTas);
		EXPECT_EQ(bus.ReadByte(0xA12015), baselineCheat);
		EXPECT_EQ(bus.ReadByte(0xA1201B), baselineDigest);
		EXPECT_NE(baselineDigest, digestBefore);
	}

	TEST(Genesis32xBusOwnershipDigestScaffoldTests, M32xToolingTasCheatSignalsAreDeterministicAcrossSaveLoadReplay) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.WriteByte(0xA15016, 0x03);
		bus.WriteByte(0xA15017, 0x12);
		bus.WriteByte(0xA15008, 0x14);
		uint8_t digestBefore = bus.ReadByte(0xA1501F);
		GenesisBoundaryScaffoldSaveState checkpoint = scaffold.SaveState();

		bus.WriteByte(0xA15009, 0x4B);
		bus.WriteByte(0xA1500B, 0x9C);
		uint8_t baselineTas = bus.ReadByte(0xA15009);
		uint8_t baselineCheat = bus.ReadByte(0xA1500B);
		uint8_t baselineDigest = bus.ReadByte(0xA1501F);

		scaffold.LoadState(checkpoint);
		bus.WriteByte(0xA15009, 0x4B);
		bus.WriteByte(0xA1500B, 0x9C);

		EXPECT_EQ(bus.ReadByte(0xA15009), baselineTas);
		EXPECT_EQ(bus.ReadByte(0xA1500B), baselineCheat);
		EXPECT_EQ(bus.ReadByte(0xA1501F), baselineDigest);
		EXPECT_NE(baselineDigest, digestBefore);
	}
}
