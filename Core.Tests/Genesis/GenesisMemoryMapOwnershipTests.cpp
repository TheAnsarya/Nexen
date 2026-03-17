#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisMemoryMapOwnershipTests, DecodeOwnerRoutesCoreAddressWindows) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0x000120), GenesisBusOwner::Rom);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA00010), GenesisBusOwner::Z80);
		EXPECT_EQ(scaffold.GetBus().GetOwnerForAddress(0xA10002), GenesisBusOwner::Io);
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

		EXPECT_EQ(value, 0xFFu);
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
}
