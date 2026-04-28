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
		uint32_t DebugLaneCount = 0;
		uint64_t DebugLaneDigest = 0;
		std::array<uint32_t, 4> DebugEntryAddress = {};
		std::array<uint8_t, 4> DebugEntryValue = {};
		std::array<uint8_t, 4> DebugEntryFlags = {};

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
			snapshot.DebugEntryAddress[i] = io.DebugTranscriptEntryAddress[i];
			snapshot.DebugEntryValue[i] = io.DebugTranscriptEntryValue[i];
			snapshot.DebugEntryFlags[i] = io.DebugTranscriptEntryFlags[i];
		}
		snapshot.DebugLaneCount = io.DebugTranscriptLaneCount;
		snapshot.DebugLaneDigest = io.DebugTranscriptLaneDigest;
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

	TEST(GenesisRuntimeTranscriptHandshakeTests, WorkRamWrite16OnOddAddressAlignsToEvenWordBoundary) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write16(0xFFFFFF, 0x1234);

		EXPECT_EQ(memoryManager.Read8(0xFFFFFE), 0x12);
		EXPECT_EQ(memoryManager.Read8(0xFFFFFF), 0x34);
		EXPECT_EQ(memoryManager.Read16(0xFFFFFE), 0x1234);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, WorkRamRead16OnOddAddressReadsAlignedEvenWord) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xFFFFFE, 0xAB);
		memoryManager.Write8(0xFFFFFF, 0xCD);

		EXPECT_EQ(memoryManager.Read16(0xFFFFFF), 0xABCD);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, IoWindowWrite16LowByteUpdatesController1ControlRegister) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write16(0xA10008, 0x12AB);

		EXPECT_EQ(memoryManager.DebugRead8(0xA10009), 0xABu);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, IoWindowWrite16LowByteUpdatesController2AndExtControlRegisters) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write16(0xA1000A, 0x34CD);
		memoryManager.Write16(0xA1000C, 0x56EF);

		EXPECT_EQ(memoryManager.DebugRead8(0xA1000B), 0xCDu);
		EXPECT_EQ(memoryManager.DebugRead8(0xA1000D), 0xEFu);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, Z80WindowRead16ReturnsConsecutiveBytesWhenBusGranted) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA11100, 0x01);
		memoryManager.Write8(0xA00000, 0x12);
		memoryManager.Write8(0xA00001, 0x34);

		EXPECT_EQ(memoryManager.Read16(0xA00000), 0x1234);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, Z80WindowWrite16WritesBothBytesWhenBusGranted) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA11100, 0x01);
		memoryManager.Write16(0xA00000, 0xABCD);

		EXPECT_EQ(memoryManager.Read8(0xA00000), 0xABu);
		EXPECT_EQ(memoryManager.Read8(0xA00001), 0xCDu);
		EXPECT_EQ(memoryManager.Read16(0xA00000), 0xABCDu);
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

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdSubCpuControlReadReflectsLatchedRunningAndBusRequestBits) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA12000, 0x03);
		uint8_t statusA = memoryManager.Read8(0xA12000);

		EXPECT_EQ(statusA & 0x03, 0x03);
		EXPECT_NE(statusA & 0xF0, 0x00);

		memoryManager.Write8(0xA12001, 0x00);
		uint8_t statusB = memoryManager.Read8(0xA12000);
		EXPECT_EQ(statusB & 0x03, 0x00);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdSubCpuControlStatusIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12000, 0x01);
			memoryManager.Write8(0xA12000, 0x03);
			(void)memoryManager.Read8(0xA12000);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12001, 0x02);
			(uint8_t)memoryManager.Read8(0xA12001);
			return memoryManager.Read8(0xA12000);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);
		uint8_t first = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);
		uint8_t replay = runTail(restored);

		EXPECT_EQ(first, replay);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdAudioCheckpointMixingProducesDeterministicStatusBytes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA12002, 0x40); // PCM L
		memoryManager.Write8(0xA12003, 0x10); // PCM R
		memoryManager.Write8(0xA12004, 0x20); // CDDA L
		memoryManager.Write8(0xA12005, 0xE0); // CDDA R (-32)

		uint8_t left = memoryManager.Read8(0xA12010);
		uint8_t right = memoryManager.Read8(0xA12011);

		EXPECT_EQ(left, 0x60u);
		EXPECT_EQ(right, 0xF0u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdAudioCheckpointStatusIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12002, 0x7F);
			memoryManager.Write8(0xA12003, 0x30);
			memoryManager.Write8(0xA12004, 0x10);
			memoryManager.Write8(0xA12005, 0x10);
			(void)memoryManager.Read8(0xA12010);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12002, 0x20);
			memoryManager.Write8(0xA12004, 0xF0);
			uint8_t left = memoryManager.Read8(0xA12010);
			uint8_t right = memoryManager.Read8(0xA12011);
			return std::tuple<uint8_t, uint8_t, RuntimeTranscriptSnapshot>(left, right, CaptureSnapshot(memoryManager));
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);

		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdToolingContractRegistersExposeDeterministicStatus) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA12012, 0x31);
		memoryManager.Write8(0xA12013, 0x42);
		memoryManager.Write8(0xA12014, 0x53);
		memoryManager.Write8(0xA12015, 0x64);

		EXPECT_EQ(memoryManager.Read8(0xA12012), 0x31u);
		EXPECT_EQ(memoryManager.Read8(0xA12013), 0x42u);
		EXPECT_EQ(memoryManager.Read8(0xA12014), 0x53u);
		EXPECT_EQ(memoryManager.Read8(0xA12015), 0x64u);
		EXPECT_EQ(memoryManager.Read8(0xA1201A), 0x0Fu);
		EXPECT_NE(memoryManager.Read8(0xA1201B), 0x00u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdToolingStatusExposesEventCountTelemetryBytes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA12012, 0x31);
		memoryManager.Write8(0xA12013, 0x42);
		memoryManager.Write8(0xA12014, 0x53);
		memoryManager.Write8(0xA12015, 0x64);

		uint16_t eventCount = (uint16_t)memoryManager.Read8(0xA12016)
			| ((uint16_t)memoryManager.Read8(0xA12017) << 8);
		EXPECT_GE(eventCount, 4u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdToolingContractStatusIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12012, 0x11);
			memoryManager.Write8(0xA12013, 0x22);
			memoryManager.Write8(0xA12014, 0x33);
			memoryManager.Write8(0xA12015, 0x44);
			(void)memoryManager.Read8(0xA1201A);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12013, 0x55);
			memoryManager.Write8(0xA12015, 0x66);
			uint8_t capabilities = memoryManager.Read8(0xA1201A);
			uint8_t digest = memoryManager.Read8(0xA1201B);
			return std::tuple<uint8_t, uint8_t, RuntimeTranscriptSnapshot>(capabilities, digest, CaptureSnapshot(memoryManager));
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);

		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugBridgeWriteUpdatesSegaCdToolingStatusBytes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA12012, 0x11);
		memoryManager.DebugWrite8(0xA12013, 0x22);
		memoryManager.DebugWrite8(0xA12014, 0x33);
		memoryManager.DebugWrite8(0xA12015, 0x44);

		EXPECT_EQ(memoryManager.DebugRead8(0xA12012), 0x11u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA12013), 0x22u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA12014), 0x33u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA12015), 0x44u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA1201A), 0x0Fu);
		EXPECT_NE(memoryManager.DebugRead8(0xA1201B), 0x00u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugBridgeReadWriteParityIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA15012, 0x01);
			memoryManager.DebugWrite8(0xA15013, 0x03);
			memoryManager.DebugWrite8(0xA15016, 0x04);
			memoryManager.DebugWrite8(0xA15017, 0x20);
			(void)memoryManager.DebugRead8(0xA1501A);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA15014, 0x8A);
			memoryManager.DebugWrite8(0xA15009, 0x41);
			memoryManager.DebugWrite8(0xA1500B, 0x6D);
			uint8_t sh2Status = memoryManager.DebugRead8(0xA1501A);
			uint8_t sh2Digest = memoryManager.DebugRead8(0xA1501B);
			uint8_t composeStatus = memoryManager.DebugRead8(0xA1501C);
			uint8_t composeDigest = memoryManager.DebugRead8(0xA1501D);
			uint8_t toolingCaps = memoryManager.DebugRead8(0xA1501E);
			uint8_t toolingDigest = memoryManager.DebugRead8(0xA1501F);
			return std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t>(
				sh2Status,
				sh2Digest,
				composeStatus,
				composeDigest,
				toolingCaps,
				toolingDigest);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);

		EXPECT_EQ(expected, replay);
		EXPECT_EQ(std::get<4>(expected), 0x0Fu);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugBridgeAccessDoesNotMutateRuntimeTranscriptLaneState) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA12012, 0xAA);
		memoryManager.Write8(0xA12013, 0xBB);

		RuntimeTranscriptSnapshot before = CaptureSnapshot(memoryManager);

		memoryManager.DebugWrite8(0xA12014, 0x33);
		(void)memoryManager.DebugRead8(0xA12014);
		(void)memoryManager.DebugRead8(0xA1201B);

		RuntimeTranscriptSnapshot after = CaptureSnapshot(memoryManager);

		EXPECT_EQ(before.LaneCount, after.LaneCount);
		EXPECT_EQ(before.LaneDigest, after.LaneDigest);
		EXPECT_GT(after.DebugLaneCount, before.DebugLaneCount);
		EXPECT_NE(after.DebugLaneDigest, before.DebugLaneDigest);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugHandshakeAccessDoesNotMutateRuntimeTranscriptLaneState) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA11100, 0x01);
		(void)memoryManager.Read8(0xA11100);

		RuntimeTranscriptSnapshot before = CaptureSnapshot(memoryManager);

		memoryManager.DebugWrite8(0xA11200, 0x01);
		(void)memoryManager.DebugRead8(0xA11100);

		RuntimeTranscriptSnapshot after = CaptureSnapshot(memoryManager);

		EXPECT_EQ(before.LaneCount, after.LaneCount);
		EXPECT_EQ(before.LaneDigest, after.LaneDigest);
		EXPECT_GT(after.DebugLaneCount, before.DebugLaneCount);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugIoControlRegistersRoundTripThroughIoState) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA10009, 0x12);
		memoryManager.DebugWrite8(0xA1000B, 0x34);
		memoryManager.DebugWrite8(0xA1000D, 0x56);

		EXPECT_EQ(memoryManager.DebugRead8(0xA10009), 0x12u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA1000B), 0x34u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA1000D), 0x56u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA10001), 0xA0u);
		EXPECT_EQ(memoryManager.DebugRead8(0xA10007), 0xFFu);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugHandshakeWindowIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA11100, 0x01);
			memoryManager.DebugWrite8(0xA11200, 0x01);
			(void)memoryManager.DebugRead8(0xA11100);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA11100, 0x00);
			memoryManager.DebugWrite8(0xA11200, 0x00);
			uint8_t busReq = memoryManager.DebugRead8(0xA11100);
			uint8_t resetRead = memoryManager.DebugRead8(0xA11200);
			RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
			return std::tuple<uint8_t, uint8_t, RuntimeTranscriptSnapshot>(busReq, resetRead, snapshot);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);

		EXPECT_EQ(expected, replay);
		EXPECT_EQ(std::get<0>(expected), 0x01u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugZ80WindowAccessHonorsHandshakeOwnershipState) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA00010, 0x5Au);
		EXPECT_EQ(memoryManager.DebugRead8(0xA00010), 0x5Au);

		memoryManager.DebugWrite8(0xA11200, 0x01);
		memoryManager.DebugWrite8(0xA11100, 0x00);
		EXPECT_EQ(memoryManager.DebugRead8(0xA00010), memoryManager.GetOpenBus());

		memoryManager.DebugWrite8(0xA11100, 0x01);
		EXPECT_EQ(memoryManager.DebugRead8(0xA00010), 0x5Au);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugZ80WindowReplayIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA00000, 0x11);
			memoryManager.DebugWrite8(0xA00001, 0x22);
			memoryManager.DebugWrite8(0xA11100, 0x01);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA00002, 0x33);
			uint8_t a = memoryManager.DebugRead8(0xA00000);
			uint8_t b = memoryManager.DebugRead8(0xA00001);
			uint8_t c = memoryManager.DebugRead8(0xA00002);
			RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
			return std::tuple<uint8_t, uint8_t, uint8_t, uint32_t, uint64_t>(a, b, c, snapshot.LaneCount, snapshot.LaneDigest);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(expected, replay);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugVdpWindowIsNullSafeWithoutAttachedVdp) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		uint8_t baselineOpenBus = memoryManager.GetOpenBus();
		memoryManager.DebugWrite8(0xC00011, 0x90);
		EXPECT_EQ(memoryManager.DebugRead8(0xC00004), baselineOpenBus);
		EXPECT_EQ(memoryManager.DebugRead8(0xC00011), baselineOpenBus);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugHandshakeTrafficIsIsolatedToDebugTranscriptLane) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA11200, 0x01);
		memoryManager.DebugWrite8(0xA11100, 0x01);
		(void)memoryManager.DebugRead8(0xA11100);
		memoryManager.DebugWrite8(0xA11100, 0x00);
		(void)memoryManager.DebugRead8(0xA11200);

		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
		EXPECT_EQ(snapshot.LaneCount, 0u);
		EXPECT_EQ(snapshot.LaneDigest, 0ull);
		EXPECT_GE(snapshot.DebugLaneCount, 5u);
		EXPECT_NE(snapshot.DebugLaneDigest, 0ull);

		bool sawDebugLaneMarker = false;
		for (uint32_t i = 0; i < 4; i++) {
			sawDebugLaneMarker |= (snapshot.DebugEntryFlags[i] & 0x40) != 0;
		}
		EXPECT_TRUE(sawDebugLaneMarker);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugBridgeTrafficTranscriptIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA12012, 0x11);
			memoryManager.DebugWrite8(0xA12013, 0x22);
			(void)memoryManager.DebugRead8(0xA1201A);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA12015, 0x55);
			(void)memoryManager.DebugRead8(0xA1201B);
			RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
			uint8_t digest = memoryManager.DebugRead8(0xA1201B);
			return std::tuple<uint8_t, uint32_t, uint64_t, uint32_t, uint64_t>(
				digest,
				snapshot.LaneCount,
				snapshot.LaneDigest,
				snapshot.DebugLaneCount,
				snapshot.DebugLaneDigest);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(expected, replay);
		EXPECT_NE(std::get<0>(expected), 0u);
		EXPECT_NE(std::get<4>(expected), 0ull);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdToolingStatusExposesDebugLaneTelemetryBytes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA10009, 0x11);
		memoryManager.DebugWrite8(0xA12012, 0x22);
		(void)memoryManager.DebugRead8(0xA1201B);

		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
		uint8_t laneCountStatus = memoryManager.Read8(0xA12018);
		uint8_t laneDigestStatus = memoryManager.Read8(0xA12019);

		EXPECT_EQ(laneCountStatus, (uint8_t)(snapshot.DebugLaneCount & 0xFF));
		EXPECT_EQ(laneDigestStatus, (uint8_t)(snapshot.DebugLaneDigest & 0xFF));
		EXPECT_NE(laneCountStatus, 0u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugTranscriptLaneCapturesIoZ80VdpAndHandshakeTraffic) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA10009, 0x12);
		memoryManager.DebugWrite8(0xA00000, 0x34);
		memoryManager.DebugWrite8(0xC00011, 0x56);
		memoryManager.DebugWrite8(0xA11100, 0x01);

		RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
		EXPECT_GE(snapshot.DebugLaneCount, 4u);
		EXPECT_NE(snapshot.DebugLaneDigest, 0ull);

		bool sawIo = false;
		bool sawZ80 = false;
		bool sawVdp = false;
		bool sawHandshake = false;
		for (uint32_t i = 0; i < 4; i++) {
			uint8_t flags = snapshot.DebugEntryFlags[i];
			sawIo |= (flags & 0x10) != 0 && (flags & 0x80) == 0;
			sawZ80 |= (flags & 0x20) != 0 && (flags & 0x10) == 0;
			sawVdp |= (flags & 0x30) == 0x30;
			sawHandshake |= (flags & 0x80) != 0;
		}

		EXPECT_TRUE(sawIo);
		EXPECT_TRUE(sawZ80);
		EXPECT_TRUE(sawVdp);
		EXPECT_TRUE(sawHandshake);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, ExpandedDebugTranscriptLaneIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA10009, 0x01);
			memoryManager.DebugWrite8(0xA00001, 0x23);
			memoryManager.DebugWrite8(0xA11100, 0x01);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xC00011, 0x80);
			memoryManager.DebugWrite8(0xA11100, 0x00);
			memoryManager.DebugWrite8(0xA1000B, 0x45);
			RuntimeTranscriptSnapshot snapshot = CaptureSnapshot(memoryManager);
			return std::tuple<uint32_t, uint64_t>(snapshot.DebugLaneCount, snapshot.DebugLaneDigest);
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(expected, replay);
		EXPECT_NE(std::get<1>(expected), 0ull);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugTranscriptLaneApiSurfacesCanBeClearedAndReused) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.DebugWrite8(0xA10009, 0x12);
		memoryManager.DebugWrite8(0xA00000, 0x34);
		memoryManager.DebugWrite8(0xA11100, 0x01);

		uint32_t laneCountBeforeClear = memoryManager.GetDebugTranscriptLaneCount();
		uint64_t laneDigestBeforeClear = memoryManager.GetDebugTranscriptLaneDigest();
		EXPECT_GE(laneCountBeforeClear, 3u);
		EXPECT_NE(laneDigestBeforeClear, 0ull);

		memoryManager.ClearDebugTranscriptLane();
		EXPECT_EQ(memoryManager.GetDebugTranscriptLaneCount(), 0u);
		EXPECT_EQ(memoryManager.GetDebugTranscriptLaneDigest(), 0ull);

		RuntimeTranscriptSnapshot snapshotAfterClear = CaptureSnapshot(memoryManager);
		EXPECT_EQ(snapshotAfterClear.DebugLaneCount, 0u);
		EXPECT_EQ(snapshotAfterClear.DebugLaneDigest, 0ull);
		for (uint32_t i = 0; i < 4; i++) {
			EXPECT_EQ(snapshotAfterClear.DebugEntryAddress[i], 0u);
			EXPECT_EQ(snapshotAfterClear.DebugEntryValue[i], 0u);
			EXPECT_EQ(snapshotAfterClear.DebugEntryFlags[i], 0u);
		}

		memoryManager.DebugWrite8(0xA12012, 0x56);
		EXPECT_EQ(memoryManager.GetDebugTranscriptLaneCount(), 1u);
		EXPECT_NE(memoryManager.GetDebugTranscriptLaneDigest(), 0ull);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, DebugTranscriptLaneClearIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA10009, 0x01);
			memoryManager.DebugWrite8(0xA00002, 0x23);
			memoryManager.DebugWrite8(0xA11100, 0x01);
			memoryManager.ClearDebugTranscriptLane();
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.DebugWrite8(0xA12015, 0x77);
			memoryManager.DebugWrite8(0xC00011, 0x80);
			return std::tuple<uint32_t, uint64_t>(memoryManager.GetDebugTranscriptLaneCount(), memoryManager.GetDebugTranscriptLaneDigest());
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(expected, replay);
		EXPECT_EQ(std::get<0>(expected), 2u);
		EXPECT_NE(std::get<1>(expected), 0ull);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xDualSh2StagingStatusReflectsControlAndSyncPhase) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA15012, 0x03);
		memoryManager.Write8(0xA15013, 0x05);
		memoryManager.Write8(0xA15014, 0x77);

		uint8_t status = memoryManager.Read8(0xA1501A);
		uint8_t digest = memoryManager.Read8(0xA1501B);

		EXPECT_EQ(status & 0x03, 0x03);
		EXPECT_EQ((status >> 2) & 0x0F, 0x05);
		EXPECT_NE(digest, 0x00u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xDualSh2StagingDigestIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15012, 0x01);
			memoryManager.Write8(0xA15013, 0x02);
			memoryManager.Write8(0xA15014, 0x10);
			(void)memoryManager.Read8(0xA1501A);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15012, 0x03);
			memoryManager.Write8(0xA15013, 0x06);
			memoryManager.Write8(0xA15014, 0x44);
			uint8_t status = memoryManager.Read8(0xA1501A);
			uint8_t digest = memoryManager.Read8(0xA1501B);
			return std::tuple<uint8_t, uint8_t, RuntimeTranscriptSnapshot>(status, digest, CaptureSnapshot(memoryManager));
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xCompositionStatusReflectsBlendAndSyncEpoch) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA15012, 0x03);
		memoryManager.Write8(0xA15013, 0x04);
		memoryManager.Write8(0xA15016, 0x09);
		memoryManager.Write8(0xA15017, 0x5A);

		uint8_t status = memoryManager.Read8(0xA1501C);
		uint8_t digest = memoryManager.Read8(0xA1501D);

		EXPECT_EQ(status & 0x0F, 0x09);
		EXPECT_NE((status >> 6) & 0x03, 0x00);
		EXPECT_NE(digest, 0x00u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xCompositionDigestIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15012, 0x01);
			memoryManager.Write8(0xA15013, 0x03);
			memoryManager.Write8(0xA15016, 0x04);
			memoryManager.Write8(0xA15017, 0x11);
			(void)memoryManager.Read8(0xA1501C);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15016, 0x0B);
			memoryManager.Write8(0xA15017, 0x77);
			uint8_t status = memoryManager.Read8(0xA1501C);
			uint8_t digest = memoryManager.Read8(0xA1501D);
			return std::tuple<uint8_t, uint8_t, RuntimeTranscriptSnapshot>(status, digest, CaptureSnapshot(memoryManager));
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xToolingContractStatusExposesCapabilitiesAndDigest) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA15008, 0x21);
		memoryManager.Write8(0xA15009, 0x32);
		memoryManager.Write8(0xA1500A, 0x43);
		memoryManager.Write8(0xA1500B, 0x54);

		EXPECT_EQ(memoryManager.Read8(0xA1501E), 0x0Fu);
		EXPECT_NE(memoryManager.Read8(0xA1501F), 0x00u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xToolingStatusExposesEventCountTelemetryBytes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA15008, 0x21);
		memoryManager.Write8(0xA15009, 0x32);
		memoryManager.Write8(0xA1500A, 0x43);
		memoryManager.Write8(0xA1500B, 0x54);

		uint16_t eventCount = (uint16_t)memoryManager.Read8(0xA15018)
			| ((uint16_t)memoryManager.Read8(0xA15019) << 8);
		EXPECT_GE(eventCount, 4u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xToolingDigestIsDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15016, 0x06);
			memoryManager.Write8(0xA15017, 0x12);
			memoryManager.Write8(0xA15008, 0x11);
			memoryManager.Write8(0xA15009, 0x22);
			(void)memoryManager.Read8(0xA1501E);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA1500A, 0x33);
			memoryManager.Write8(0xA1500B, 0x44);
			uint16_t eventCount = (uint16_t)memoryManager.Read8(0xA15018)
				| ((uint16_t)memoryManager.Read8(0xA15019) << 8);
			uint8_t caps = memoryManager.Read8(0xA1501E);
			uint8_t digest = memoryManager.Read8(0xA1501F);
			return std::tuple<uint16_t, uint8_t, uint8_t, RuntimeTranscriptSnapshot>(eventCount, caps, digest, CaptureSnapshot(memoryManager));
		};

		runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
		EXPECT_EQ(std::get<3>(expected), std::get<3>(replay));
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, SegaCdToolingTasCheatSignalsAreDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12012, 0x18);
			memoryManager.Write8(0xA12014, 0x2C);
			return memoryManager.Read8(0xA1201B);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA12013, 0x5A);
			memoryManager.Write8(0xA12015, 0xC3);
			uint8_t tas = memoryManager.Read8(0xA12013);
			uint8_t cheat = memoryManager.Read8(0xA12015);
			uint8_t digest = memoryManager.Read8(0xA1201B);
			return std::tuple<uint8_t, uint8_t, uint8_t, RuntimeTranscriptSnapshot>(tas, cheat, digest, CaptureSnapshot(memoryManager));
		};

		uint8_t digestBefore = runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
		EXPECT_EQ(std::get<3>(expected), std::get<3>(replay));
		EXPECT_NE(std::get<2>(expected), digestBefore);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, M32xToolingTasCheatSignalsAreDeterministicAcrossSerializeReplay) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15016, 0x04);
			memoryManager.Write8(0xA15017, 0x22);
			memoryManager.Write8(0xA15008, 0x0F);
			return memoryManager.Read8(0xA1501F);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA15009, 0x61);
			memoryManager.Write8(0xA1500B, 0x92);
			uint8_t tas = memoryManager.Read8(0xA15009);
			uint8_t cheat = memoryManager.Read8(0xA1500B);
			uint8_t digest = memoryManager.Read8(0xA1501F);
			return std::tuple<uint8_t, uint8_t, uint8_t, RuntimeTranscriptSnapshot>(tas, cheat, digest, CaptureSnapshot(memoryManager));
		};

		uint8_t digestBefore = runPrefix(original);
		Serializer saver(1, true, SerializeFormat::Binary);
		original.Serialize(saver);
		std::stringstream state;
		saver.SaveTo(state);
		state.seekg(0);

		auto expected = runTail(original);

		Emulator emuB;
		std::vector<uint8_t> romB(0x400000);
		GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);
		Serializer loader(1, false, SerializeFormat::Binary);
		ASSERT_TRUE(loader.LoadFrom(state));
		restored.Serialize(loader);

		auto replay = runTail(restored);
		EXPECT_EQ(std::get<0>(expected), std::get<0>(replay));
		EXPECT_EQ(std::get<1>(expected), std::get<1>(replay));
		EXPECT_EQ(std::get<2>(expected), std::get<2>(replay));
		EXPECT_EQ(std::get<3>(expected), std::get<3>(replay));
		EXPECT_NE(std::get<2>(expected), digestBefore);
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

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeA11100Read16MatchesBoth8BitStatusLanes) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		// Bus free: both byte lanes expose status bit.
		memoryManager.Write8(0xA11100, 0x00);
		EXPECT_EQ(memoryManager.Read8(0xA11100), 0x01u);
		EXPECT_EQ(memoryManager.Read8(0xA11101), 0x01u);
		EXPECT_EQ(memoryManager.Read16(0xA11100), 0x0101u);

		// Bus requested: both lanes drop to zero.
		memoryManager.Write8(0xA11100, 0x01);
		EXPECT_EQ(memoryManager.Read8(0xA11100), 0x00u);
		EXPECT_EQ(memoryManager.Read8(0xA11101), 0x00u);
		EXPECT_EQ(memoryManager.Read16(0xA11100), 0x0000u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeWrite16LowLaneCanRequestBus) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write16(0xA11100, 0x0001);

		EXPECT_EQ(memoryManager.Read8(0xA11100), 0x00u);
		EXPECT_EQ(memoryManager.Read8(0xA11101), 0x00u);
		EXPECT_EQ(memoryManager.Read16(0xA11100), 0x0000u);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeHandshakeWrite16LowLaneCanReleaseReset) {
		Emulator emu;
		std::vector<uint8_t> romData(0x400000);
		GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

		memoryManager.Write8(0xA00000, 0x5Au);
		EXPECT_EQ(memoryManager.Read8(0xA00000), 0x5Au);

		memoryManager.Write8(0xA11100, 0x00);
		memoryManager.Write16(0xA11200, 0x0001);

		// With reset released and no bus request, Z80 window should read open bus instead of RAM.
		EXPECT_EQ(memoryManager.Read8(0xA00000), 0x00u);
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

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixed8And16BitOrderingPerturbationChangesDigestWithSameLaneCount) {
		auto runReferenceOrder = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write8(0xA11100, 0x01);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read8(0xA11201);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);

			return CaptureSnapshot(memoryManager);
		};

		auto runPerturbedOrder = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);
			(void)memoryManager.Read8(0xA11201);
			(void)memoryManager.Read16(0xA11200);
			memoryManager.Write8(0xA11100, 0x00);

			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot reference = runReferenceOrder();
		RuntimeTranscriptSnapshot perturbed = runPerturbedOrder();

		EXPECT_EQ(reference.LaneCount, perturbed.LaneCount);
		EXPECT_NE(reference.LaneDigest, perturbed.LaneDigest);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixedHandshakeControlValueNoOpWritesRemainDeterministic) {
		auto runControlValueNoOpScenario = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write16(0xA11100, 0x0100);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write8(0xA11100, 0x00);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);

			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot runA = runControlValueNoOpScenario();
		RuntimeTranscriptSnapshot runB = runControlValueNoOpScenario();

		EXPECT_EQ(runA, runB);
		EXPECT_NE(runA.LaneDigest, 0ull);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixedHandshakeControlValueNoOpWritesPreserveReplayParityAfterSerialize) {
		Emulator emuA;
		std::vector<uint8_t> romA(0x400000);
		GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

		auto runPrefix = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write16(0xA11100, 0x0100);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write8(0xA11100, 0x00);
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

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixedHandshakeNoOpControlOrderingPerturbationChangesDigestWithSameLaneCount) {
		auto runReferenceOrder = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write8(0xA11100, 0x00);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);

			return CaptureSnapshot(memoryManager);
		};

		auto runPerturbedOrder = []() {
			Emulator emu;
			std::vector<uint8_t> romData(0x400000);
			GenesisMemoryManager memoryManager = CreateMemoryManager(emu, romData);

			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write16(0xA11200, 0x0000);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);
			memoryManager.Write8(0xA11100, 0x00);

			return CaptureSnapshot(memoryManager);
		};

		RuntimeTranscriptSnapshot reference = runReferenceOrder();
		RuntimeTranscriptSnapshot perturbed = runPerturbedOrder();

		EXPECT_EQ(reference.LaneCount, perturbed.LaneCount);
		EXPECT_NE(reference.LaneDigest, perturbed.LaneDigest);
	}

	TEST(GenesisRuntimeTranscriptHandshakeTests, RuntimeMixedHandshakeNoOpControlReplayParityPersistsWithLaterCheckpointShift) {
		auto runPrefixA = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write8(0xA11200, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
			memoryManager.Write8(0xA11100, 0x01);
		};

		auto runPrefixB = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write16(0xA11100, 0x0100);
			(void)memoryManager.Read8(0xA11101);
			memoryManager.Write16(0xA11100, 0x0100);
		};

		auto runTail = [](GenesisMemoryManager& memoryManager) {
			memoryManager.Write16(0xA11200, 0x0000);
			(void)memoryManager.Read16(0xA11100);
			memoryManager.Write8(0xA11100, 0x00);
			memoryManager.Write8(0xA11100, 0x00);
			(void)memoryManager.Read16(0xA11200);
		};

		auto runFullSequence = [&](GenesisMemoryManager& memoryManager) {
			runPrefixA(memoryManager);
			runPrefixB(memoryManager);
			runTail(memoryManager);
		};

		auto captureReplayAfterCheckpoint = [&](bool shiftCheckpointLater) {
			Emulator emuA;
			std::vector<uint8_t> romA(0x400000);
			GenesisMemoryManager original = CreateMemoryManager(emuA, romA);

			runPrefixA(original);
			if (shiftCheckpointLater) {
				runPrefixB(original);
			}

			Serializer saver(1, true, SerializeFormat::Binary);
			original.Serialize(saver);
			std::stringstream ss;
			saver.SaveTo(ss);
			ss.seekg(0);

			if (!shiftCheckpointLater) {
				runPrefixB(original);
			}
			runTail(original);
			RuntimeTranscriptSnapshot expected = CaptureSnapshot(original);

			Emulator emuB;
			std::vector<uint8_t> romB(0x400000);
			GenesisMemoryManager restored = CreateMemoryManager(emuB, romB);

			Serializer loader(1, false, SerializeFormat::Binary);
			EXPECT_TRUE(loader.LoadFrom(ss));
			restored.Serialize(loader);

			if (!shiftCheckpointLater) {
				runPrefixB(restored);
			}
			runTail(restored);
			RuntimeTranscriptSnapshot replay = CaptureSnapshot(restored);

			EXPECT_EQ(expected.LaneCount, replay.LaneCount);
			EXPECT_EQ(expected.LaneDigest, replay.LaneDigest);
			return replay;
		};

		Emulator emuDirect;
		std::vector<uint8_t> romDirect(0x400000);
		GenesisMemoryManager direct = CreateMemoryManager(emuDirect, romDirect);
		runFullSequence(direct);
		RuntimeTranscriptSnapshot directSnapshot = CaptureSnapshot(direct);

		RuntimeTranscriptSnapshot earlyCheckpointReplay = captureReplayAfterCheckpoint(false);
		RuntimeTranscriptSnapshot lateCheckpointReplay = captureReplayAfterCheckpoint(true);

		EXPECT_EQ(directSnapshot.LaneCount, earlyCheckpointReplay.LaneCount);
		EXPECT_EQ(directSnapshot.LaneDigest, earlyCheckpointReplay.LaneDigest);
		EXPECT_EQ(directSnapshot.LaneCount, lateCheckpointReplay.LaneCount);
		EXPECT_EQ(directSnapshot.LaneDigest, lateCheckpointReplay.LaneDigest);
		EXPECT_EQ(earlyCheckpointReplay.LaneCount, lateCheckpointReplay.LaneCount);
		EXPECT_EQ(earlyCheckpointReplay.LaneDigest, lateCheckpointReplay.LaneDigest);
	}
}
