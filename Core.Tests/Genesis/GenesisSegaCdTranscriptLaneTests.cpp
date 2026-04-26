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
		EXPECT_NE(lane[0].find("seq=1 role=SCD-G1-CMD op=W addr=a12002 value=34"), string::npos);
		EXPECT_NE(lane[1].find("seq=2 role=SCD-G1-RSP op=W addr=a12012 value=56"), string::npos);
		EXPECT_NE(lane[2].find("seq=3 role=SCD-G1-CMD op=R addr=a12002 value=34"), string::npos);
		EXPECT_NE(lane[3].find("seq=4 role=SCD-G1-RSP op=R addr=a12012 value=56"), string::npos);
		EXPECT_FALSE(bus.GetCommandResponseLaneDigest().empty());
	}

	TEST(GenesisSegaCdTranscriptLaneTests, ExpandedBridgeWindowsA130AndA140AreTrackedDeterministically) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();
		auto& bus = scaffold.GetBus();

		bus.ClearCommandResponseLane();
		bus.WriteByte(0xA13001, 0x91);
		bus.WriteByte(0xA13011, 0xA1);
		bus.WriteByte(0xA14002, 0x92);
		bus.WriteByte(0xA14012, 0xA2);
		EXPECT_EQ(bus.ReadByte(0xA13001), 0x91u);
		EXPECT_EQ(bus.ReadByte(0xA13011), 0xA1u);
		EXPECT_EQ(bus.ReadByte(0xA14002), 0x92u);
		EXPECT_EQ(bus.ReadByte(0xA14012), 0xA2u);

		const vector<string>& lane = bus.GetCommandResponseLane();
		ASSERT_EQ(bus.GetCommandResponseLaneCount(), 8u);
		ASSERT_EQ(lane.size(), 8u);
		EXPECT_NE(lane[0].find("role=SCD-G2-CMD op=W addr=a13001 value=91"), string::npos);
		EXPECT_NE(lane[1].find("role=SCD-G2-RSP op=W addr=a13011 value=a1"), string::npos);
		EXPECT_NE(lane[2].find("role=SCD-G3-CMD op=W addr=a14002 value=92"), string::npos);
		EXPECT_NE(lane[3].find("role=SCD-G3-RSP op=W addr=a14012 value=a2"), string::npos);
		EXPECT_NE(lane[4].find("role=SCD-G2-CMD op=R addr=a13001 value=91"), string::npos);
		EXPECT_NE(lane[5].find("role=SCD-G2-RSP op=R addr=a13011 value=a1"), string::npos);
		EXPECT_NE(lane[6].find("role=SCD-G3-CMD op=R addr=a14002 value=92"), string::npos);
		EXPECT_NE(lane[7].find("role=SCD-G3-RSP op=R addr=a14012 value=a2"), string::npos);
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
