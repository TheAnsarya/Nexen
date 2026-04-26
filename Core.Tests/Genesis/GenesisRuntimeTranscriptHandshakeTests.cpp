#include "pch.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"

namespace {
	struct RuntimeTranscriptSnapshot {
		uint32_t LaneCount = 0;
		uint64_t LaneDigest = 0;
		std::array<uint32_t, 4> EntryAddress = {};
		std::array<uint8_t, 4> EntryValue = {};
		std::array<uint8_t, 4> EntryFlags = {};

		bool operator==(const RuntimeTranscriptSnapshot&) const = default;
	};

	GenesisMemoryManager CreateMemoryManager(Emulator& emu, std::vector<uint8_t>& romData) {
		emu.Initialize(false);

		GenesisMemoryManager memoryManager;
		memoryManager.Init(&emu, nullptr, romData, nullptr, nullptr, nullptr);
		return memoryManager;
	}

	RuntimeTranscriptSnapshot CaptureSnapshot(const GenesisMemoryManager& memoryManager) {
		GenesisIoState io = memoryManager.GetIoState();

		RuntimeTranscriptSnapshot snapshot = {};
		snapshot.LaneCount = io.TranscriptLaneCount;
		snapshot.LaneDigest = io.TranscriptLaneDigest;
		for (uint32_t i = 0; i < 4; i++) {
			snapshot.EntryAddress[i] = io.TranscriptEntryAddress[i];
			snapshot.EntryValue[i] = io.TranscriptEntryValue[i];
			snapshot.EntryFlags[i] = io.TranscriptEntryFlags[i];
		}
		return snapshot;
	}

	RuntimeTranscriptSnapshot RunHandshakeScenario() {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA11200, 0x01);
		memoryManager.Write8(0xA11100, 0x01);
		(void)memoryManager.Read8(0xA11100);
		memoryManager.Write16(0xA11200, 0x0000);
		memoryManager.Write16(0xA11100, 0x0000);
		(void)memoryManager.Read16(0xA11100);

		return CaptureSnapshot(memoryManager);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeTrafficContributesToTranscriptDigest) {
		RuntimeTranscriptSnapshot snapshot = RunHandshakeScenario();

		EXPECT_EQ(snapshot.LaneCount, 6u);
		EXPECT_NE(snapshot.LaneDigest, 0ull);

		bool sawHandshakeMarker = false;
		bool sawResetChannelMarker = false;
		for (uint32_t i = 0; i < 4; i++) {
			sawHandshakeMarker |= (snapshot.EntryFlags[i] & 0x80) != 0;
			sawResetChannelMarker |= (snapshot.EntryFlags[i] & 0x84) == 0x84;
		}

		EXPECT_TRUE(sawHandshakeMarker);
		EXPECT_TRUE(sawResetChannelMarker);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeTranscriptDigestIsDeterministicAcrossRuns) {
		RuntimeTranscriptSnapshot runA = RunHandshakeScenario();
		RuntimeTranscriptSnapshot runB = RunHandshakeScenario();

		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeTranscriptPreservesReplayAfterSerializeRoundtrip) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			(void)memoryManager.Read8(0xA11100);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write16(0xA11200, 0x0000);
			memoryManager.Write16(0xA11100, 0x0000);
			(void)memoryManager.Read16(0xA11100);
		};

		runPrefix(original);

		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream ss;
		saver.SaveTo(ss);
		ss.seekg(0);

		runTail(original);
		RuntimeTranscriptSnapshot expected = CaptureSnapshot(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);

		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(ss));
		restored.Serialize(loader);

		runTail(restored);
		RuntimeTranscriptSnapshot replay = CaptureSnapshot(restored);

		EXPECT_EQ(expected.LaneCount, replay.LaneCount);
		EXPECT_EQ(expected.LaneDigest, replay.LaneDigest);
	}
}
