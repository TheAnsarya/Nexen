#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisMemoryMapOwnershipTests, DecodeOwnerRoutesCoreAddressWindows) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0x000120), GenesisBusOwner::Rom);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA00010), GenesisBusOwner::Z80);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA10002), GenesisBusOwner::Io);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA14101), GenesisBusOwner::Io);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xC00004), GenesisBusOwner::Vdp);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xFF1234), GenesisBusOwner::WorkRam);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0x800000), GenesisBusOwner::OpenBus);
	}

	TEST(GenesisMemoryMapOwnershipTests, IoWindowReadWriteUsesDedicatedOwnershipPath) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xA10003, 0x5A);
		uint8_t value = scaffold.GetBus().ReadByte(0xA10003);

		EXPECT_TRUE(scaffold.GetBus().WasIoWindowAccessed());
		EXPECT_EQ(scaffold.GetBus().GetIoWriteCount(), 1u);
		EXPECT_EQ(scaffold.GetBus().GetIoReadCount(), 1u);
		EXPECT_EQ(value, 0x5Au);
	}

	TEST(GenesisMemoryMapOwnershipTests, OpenBusAccessReturnsFFAndTracksOwnershipViolations) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0x800000, 0x11);
		uint8_t value = scaffold.GetBus().ReadByte(0x800000);

		EXPECT_EQ(value, 0x11u);
		EXPECT_EQ(scaffold.GetBus().GetOpenBusWriteCount(), 1u);
		EXPECT_EQ(scaffold.GetBus().GetOpenBusReadCount(), 1u);
	}

	TEST(GenesisMemoryMapOwnershipTests, WorkRamWindowMirrorsOnLow16Bits) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xFF1234, 0xA6);
		uint8_t mirrored = scaffold.GetBus().ReadByte(0x1FF1234);

		EXPECT_EQ(mirrored, 0xA6u);
		EXPECT_EQ(scaffold.GetBus().GetWorkRamWriteCount(), 1u);
		EXPECT_EQ(scaffold.GetBus().GetWorkRamReadCount(), 1u);
	}

	TEST(GenesisMemoryMapOwnershipTests, Z80OwnershipTransitionsPreserveDecodeAndToggleAccessSemantics) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA00000), GenesisBusOwner::Z80);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA11100), GenesisBusOwner::Io);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA11200), GenesisBusOwner::Io);

		scaffold.GetBus().WriteByte(0xA11200, 0x01);
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());

		uint8_t blockedRead = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(blockedRead, 0xFFu);

		scaffold.GetBus().WriteByte(0xA11100, 0x01);
		EXPECT_FALSE(scaffold.GetBus().IsZ80Running());
		uint8_t handoffRead = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(handoffRead, 0x00u);

		scaffold.GetBus().WriteByte(0xA11100, 0x00);
		EXPECT_TRUE(scaffold.GetBus().IsZ80Running());
		uint8_t resumedRead = scaffold.GetBus().ReadByte(0xA00000);
		EXPECT_EQ(resumedRead, 0xFFu);

		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA00000), GenesisBusOwner::Z80);
		EXPECT_GE(scaffold.GetBus().GetZ80HandoffCount(), 2u);
		EXPECT_GE(scaffold.GetBus().GetZ80ReadCount(), 3u);
	}

	TEST(GenesisMemoryMapOwnershipTests, Z80OwnershipTransitionDigestIsDeterministicAcrossRepeatedRuns) {
		auto runScenario = []() {
			GenesisM68kBoundaryScaffold scaffold;
			scaffold.Startup();

			scaffold.GetBus().WriteByte(0xA11200, 0x01);
			scaffold.StepFrameScaffold(32);
			uint64_t z80CyclesRunning = scaffold.GetBus().GetZ80ExecutedCycles();

			scaffold.GetBus().WriteByte(0xA11100, 0x01);
			scaffold.StepFrameScaffold(32);
			uint64_t z80CyclesHeld = scaffold.GetBus().GetZ80ExecutedCycles();

			scaffold.GetBus().WriteByte(0xA11100, 0x00);
			scaffold.StepFrameScaffold(32);
			uint64_t z80CyclesResumed = scaffold.GetBus().GetZ80ExecutedCycles();

			uint8_t blockedRead = scaffold.GetBus().ReadByte(0xA00000);
			scaffold.GetBus().WriteByte(0xA11100, 0x01);
			uint8_t handoffRead = scaffold.GetBus().ReadByte(0xA00000);

			return std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint32_t, uint8_t, uint8_t>(
				z80CyclesRunning,
				z80CyclesHeld,
				z80CyclesResumed,
				scaffold.GetBus().GetZ80HandoffCount(),
				scaffold.GetBus().GetZ80ReadCount(),
				blockedRead,
				handoffRead);
		};

		auto runA = runScenario();
		auto runB = runScenario();

		EXPECT_EQ(runA, runB);
		EXPECT_GT(std::get<0>(runA), 0u);
		EXPECT_EQ(std::get<0>(runA), std::get<1>(runA));
		EXPECT_GT(std::get<2>(runA), std::get<1>(runA));
		EXPECT_EQ(std::get<5>(runA), 0xFFu);
		EXPECT_EQ(std::get<6>(runA), 0x00u);
	}
}
