#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpDmaScaffoldTests, TracksVdpWindowReadWriteActivity) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x55);
		uint8_t value = scaffold.GetBus().ReadByte(0xC00004);

		EXPECT_TRUE(scaffold.GetBus().WasVdpWindowAccessed());
		EXPECT_EQ(scaffold.GetBus().GetVdpWriteCount(), 1u);
		EXPECT_EQ(scaffold.GetBus().GetVdpReadCount(), 1u);
		EXPECT_EQ(scaffold.GetBus().GetLastVdpAddress(), 0xC00004u);
		EXPECT_EQ(value, 0x55);
	}

	TEST(GenesisVdpDmaScaffoldTests, ControlPortWriteCanLatchDmaRequest) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		EXPECT_FALSE(scaffold.GetBus().WasDmaRequested());

		scaffold.GetBus().WriteByte(0xC00004, 0x40);
		EXPECT_FALSE(scaffold.GetBus().WasDmaRequested());

		scaffold.GetBus().WriteByte(0xC00005, 0x80);
		EXPECT_TRUE(scaffold.GetBus().WasDmaRequested());
		EXPECT_EQ(scaffold.GetBus().GetLastVdpAddress(), 0xC00005u);
		EXPECT_EQ(scaffold.GetBus().GetLastVdpValue(), 0x80u);
	}
}
