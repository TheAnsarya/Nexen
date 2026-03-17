#include "pch.h"
#include "Genesis/GenesisM68kBoundaryScaffold.h"

namespace {
	TEST(GenesisVdpRegisterStatusTests, ControlWordUpdatesRegisterFileAndStatusSemantics) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x81);
		scaffold.GetBus().WriteByte(0xC00005, 0x40);
		EXPECT_EQ(scaffold.GetBus().GetVdpControlWordLatch(), 0x8140u);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x40u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) != 0);

		scaffold.GetBus().WriteByte(0xC00004, 0x81);
		scaffold.GetBus().WriteByte(0xC00005, 0x00);
		EXPECT_EQ(scaffold.GetBus().GetVdpRegister(1), 0x00u);
		EXPECT_TRUE((scaffold.GetBus().GetVdpStatus() & 0x0008u) == 0);
	}

	TEST(GenesisVdpRegisterStatusTests, StatusReadClearsStickyPendingBitOnHighByteRead) {
		GenesisM68kBoundaryScaffold scaffold;
		scaffold.Startup();

		scaffold.GetBus().WriteByte(0xC00004, 0x40);
		scaffold.GetBus().WriteByte(0xC00005, 0x00);

		uint8_t statusFirst = scaffold.GetBus().ReadByte(0xC00004);
		uint8_t statusSecond = scaffold.GetBus().ReadByte(0xC00004);

		EXPECT_TRUE((statusFirst & 0x80u) != 0);
		EXPECT_TRUE((statusSecond & 0x80u) == 0);
	}
}
