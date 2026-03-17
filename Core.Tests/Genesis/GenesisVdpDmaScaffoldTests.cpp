#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpDmaScaffoldTests, TracksVdpWindowReadWriteActivity) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00000, 0x12);
		scaffold.GetBus().WriteByte(0xC00001, 0x34);
		uint8_t valueHi = scaffold.GetBus().ReadByte(0xC00000);
		uint8_t valueLo = scaffold.GetBus().ReadByte(0xC00001);

		EXPECT_TRUE(scaffold.GetBus().WasVdpWindowAccessed());
		EXPECT_EQ(scaffold.GetBus().GetVdpWriteCount(), 2u);
		EXPECT_EQ(scaffold.GetBus().GetVdpReadCount(), 2u);
		EXPECT_EQ(scaffold.GetBus().GetLastVdpAddress(), 0xC00001u);
		EXPECT_EQ(valueHi, 0x12u);
		EXPECT_EQ(valueLo, 0x34u);
		EXPECT_EQ(scaffold.GetBus().GetVdpDataPortLatch(), 0x1234u);
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
