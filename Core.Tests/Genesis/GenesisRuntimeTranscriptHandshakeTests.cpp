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

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeOrderingPerturbationChangesDigestWithSameLaneCount) {
		auto runReferenceOrder = []() {
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
		};

		auto runPerturbedOrder = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			(void)memoryManager.Read8(0xA11100);
			memoryManager.Write16(0xA11100, 0x0000);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);

			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot reference = runReferenceOrder();
		RuntimeTranscriptSnapshot perturbed = runPerturbedOrder();

		EXPECT_EQ(reference.LaneCount, perturbed.LaneCount);
		EXPECT_NE(reference.LaneDigest, perturbed.LaneDigest);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeResetReadPathAccessesAreCapturedInTranscript) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA11200, 0x01);
		memoryManager.Write8(0xA11100, 0x01);

		uint32_t laneCountBefore = memoryManager.GetIoState().TranscriptLaneCount;
		(void)memoryManager.Read8(0xA11200);
		(void)memoryManager.Read16(0xA11200);

		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
		EXPECT_EQ(snapshot.LaneCount, laneCountBefore + 2);

		bool sawResetReadEntry = false;
		for (uint32_t i = 0; i < 4; i++) {
			if (snapshot.EntryAddress[i] == 0xA11200u) {
				uint8_t flags = snapshot.EntryFlags[i];
				sawResetReadEntry |= (flags & 0x86) == 0x86;
			}
		}

		EXPECT_TRUE(sawResetReadEntry);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeMarkersClassifyBusRequestAndResetChannels) {
		auto runBusRequestWrite = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);
			memoryManager.Write8(0xA11100, 0x01);
			return CaptureSnapshot(memoryManager);
		};

		auto runResetWrite = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);
			memoryManager.Write8(0xA11200, 0x01);
			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot busWrite = runBusRequestWrite();
		RuntimeTranscriptSnapshot resetWrite = runResetWrite();

		ASSERT_EQ(busWrite.LaneCount, 1u);
		ASSERT_EQ(resetWrite.LaneCount, 1u);
		EXPECT_EQ(busWrite.EntryAddress[0], 0xA11100u);
		EXPECT_EQ(resetWrite.EntryAddress[0], 0xA11200u);

		uint8_t busFlags = busWrite.EntryFlags[0];
		uint8_t resetFlags = resetWrite.EntryFlags[0];
		EXPECT_EQ(busFlags & 0x81, 0x81);
		EXPECT_EQ(busFlags & 0x04, 0x00);
		EXPECT_EQ(resetFlags & 0x85, 0x85);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeMarkersClassifyReadRoleBitsByChannel) {
		auto runBusRequestRead = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);
			(void)memoryManager.Read8(0xA11100);
			return CaptureSnapshot(memoryManager);
		};

		auto runResetRead = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);
			(void)memoryManager.Read8(0xA11200);
			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot busRead = runBusRequestRead();
		RuntimeTranscriptSnapshot resetRead = runResetRead();

		ASSERT_EQ(busRead.LaneCount, 1u);
		ASSERT_EQ(resetRead.LaneCount, 1u);
		EXPECT_EQ(busRead.EntryAddress[0], 0xA11100u);
		EXPECT_EQ(resetRead.EntryAddress[0], 0xA11200u);

		uint8_t busFlags = busRead.EntryFlags[0];
		uint8_t resetFlags = resetRead.EntryFlags[0];
		EXPECT_EQ(busFlags & 0x82, 0x82);
		EXPECT_EQ(busFlags & 0x04, 0x00);
		EXPECT_EQ(resetFlags & 0x86, 0x86);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeOddAddressReadsCaptureExpectedAddressAndFlags) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		(void)memoryManager.Read8(0xA11101);
		(void)memoryManager.Read8(0xA11201);
		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);

		ASSERT_EQ(snapshot.LaneCount, 2u);
		EXPECT_EQ(snapshot.EntryAddress[0], 0xA11101u);
		EXPECT_EQ(snapshot.EntryAddress[1], 0xA11201u);
		EXPECT_EQ(snapshot.EntryFlags[0] & 0x82, 0x82);
		EXPECT_EQ(snapshot.EntryFlags[0] & 0x04, 0x00);
		EXPECT_EQ(snapshot.EntryFlags[1] & 0x86, 0x86);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeOddAddressReadsAreDeterministicAcrossRuns) {
		auto runOddReadScenario = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			(void)memoryManager.Read8(0xA11101);
			(void)memoryManager.Read8(0xA11201);
			(void)memoryManager.Read8(0xA11101);
			(void)memoryManager.Read8(0xA11201);
			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot runA = runOddReadScenario();
		RuntimeTranscriptSnapshot runB = runOddReadScenario();

		EXPECT_EQ(runA, runB);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshake16BitAccessesCaptureExpectedEntriesAndFlags) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write16(0xA11100, 0x0100);
		memoryManager.Write16(0xA11200, 0x0000);
		(void)memoryManager.Read16(0xA11100);
		(void)memoryManager.Read16(0xA11200);

		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
		ASSERT_EQ(snapshot.LaneCount, 4u);
		EXPECT_EQ(snapshot.EntryAddress[0], 0xA11100u);
		EXPECT_EQ(snapshot.EntryAddress[1], 0xA11200u);
		EXPECT_EQ(snapshot.EntryAddress[2], 0xA11100u);
		EXPECT_EQ(snapshot.EntryAddress[3], 0xA11200u);

		EXPECT_EQ(snapshot.EntryFlags[0] & 0x81, 0x81);
		EXPECT_EQ(snapshot.EntryFlags[1] & 0x85, 0x85);
		EXPECT_EQ(snapshot.EntryFlags[2] & 0x82, 0x82);
		EXPECT_EQ(snapshot.EntryFlags[3] & 0x86, 0x86);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshake16BitScenarioIsDeterministicAcrossRuns) {
		auto run16BitScenario = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write16(0xA11100, 0x0000);
			memoryManager.Write16(0xA11200, 0x0100);
			(void)memoryManager.Read16(0xA11100);
			(void)memoryManager.Read16(0xA11200);
			memoryManager.Write16(0xA11100, 0x0100);
			memoryManager.Write16(0xA11200, 0x0000);
			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot runA = run16BitScenario();
		RuntimeTranscriptSnapshot runB = run16BitScenario();

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

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixed8And16BitHandshakeReplayPreservesTranscriptParity) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write8(0xA11100, 0x01);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read8(0xA11201);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);
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
