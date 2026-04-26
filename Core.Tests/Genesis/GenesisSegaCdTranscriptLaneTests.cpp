#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisSegaCdTranscriptLaneTests, CommandAndResponseWindowsCaptureOrderedLaneEntries) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.ClearCommandResponseLane();
		bus.WriteByte(0xA12002, 0x34);
		bus.WriteByte(0xA12012, 0x56);
		EXPECT_EQ(bus.ReadByte(0xA12002), 0x34u);
		EXPECT_EQ(bus.ReadByte(0xA12012), 0x56u);

		const vector<string>& lane = bus.GetCommandResponseLane();
		ASSERT_EQ(bus.GetCommandResponseLaneCount(), 4u);
		ASSERT_EQ(lane.size(), 4u);
		EXPECT_NE(lane[0].find("seq=1 role=SCD-CMD op=W addr=a12002 value=34"), string::npos);
		EXPECT_NE(lane[1].find("seq=2 role=SCD-RSP op=W addr=a12012 value=56"), string::npos);
		EXPECT_NE(lane[2].find("seq=3 role=SCD-CMD op=R addr=a12002 value=34"), string::npos);
		EXPECT_NE(lane[3].find("seq=4 role=SCD-RSP op=R addr=a12012 value=56"), string::npos);
		EXPECT_FALSE(bus.GetCommandResponseLaneDigest().empty());
	}

	TEST(GenesisSegaCdTranscriptLaneTests, SegaCdTranscriptDigestIsDeterministicAcrossRuns) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();
			auto& bus = scaffold.GetBus();
			bus.ClearCommandResponseLane();

			for (uint8_t i = 0; i < 8; i++) {
				bus.WriteByte((uint32_t)(0xA12000 + (i & 0x0F)), (uint8_t)(0x20 + i));
				bus.ReadByte((uint32_t)(0xA12010 + (i & 0x0F)));
				bus.WriteByte((uint32_t)(0xA12010 + (i & 0x0F)), (uint8_t)(0x80 + i));
				bus.ReadByte((uint32_t)(0xA12000 + (i & 0x0F)));
			}

			return std::tuple<uint32_t, string, vector<string>>(
				bus.GetCommandResponseLaneCount(),
				bus.GetCommandResponseLaneDigest(),
				bus.GetCommandResponseLane());
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_FALSE(std::get<1>(runA).empty());
		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisSegaCdTranscriptLaneTests, SaveStateReplayPreservesSegaCdTranscriptProgression) {
		GenesisM68kBoundaryScaffold firstRun;
		firstRun.Startup();
		auto& firstBus = firstRun.GetBus();
		firstBus.ClearCommandResponseLane();

		firstBus.WriteByte(0xA12003, 0x44);
		firstBus.WriteByte(0xA12013, 0x66);
		EXPECT_EQ(firstBus.ReadByte(0xA12003), 0x44u);
		GenesisBoundaryScaffoldSaveState checkpoint = firstRun.SaveState();

		firstBus.WriteByte(0xA12004, 0x55);
		EXPECT_EQ(firstBus.ReadByte(0xA12013), 0x66u);
		EXPECT_EQ(firstBus.ReadByte(0xA12004), 0x55u);

		GenesisM68kBoundaryScaffold replayRun;
		replayRun.LoadState(checkpoint);
		auto& replayBus = replayRun.GetBus();

		replayBus.WriteByte(0xA12004, 0x55);
		EXPECT_EQ(replayBus.ReadByte(0xA12013), 0x66u);
		EXPECT_EQ(replayBus.ReadByte(0xA12004), 0x55u);

		EXPECT_EQ(firstBus.GetCommandResponseLaneCount(), replayBus.GetCommandResponseLaneCount());
		EXPECT_EQ(firstBus.GetCommandResponseLaneDigest(), replayBus.GetCommandResponseLaneDigest());
		EXPECT_EQ(firstBus.GetCommandResponseLane(), replayBus.GetCommandResponseLane());
	}
}
