#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "Genesis/GenesisConsole.h"
#include "Shared/Emulator.h"

namespace {
	void StampSegaMarker(vector<uint8_t>& data) {
		if (data.size() >= 0x104) {
			data[0x100] = 'S';
			data[0x101] = 'E';
			data[0x102] = 'G';
			data[0x103] = 'A';
		}
	}

}

TEST(BinRomRouting, ChannelFRejectsGenesisHeaderBinAsUnknownType) {
	Emulator emu;
	ChannelFConsole channelF(&emu);
	vector<uint8_t> romData(0x40000, 0);
	StampSegaMarker(romData);
	VirtualFile rom(romData.data(), romData.size(), "sonic.bin");

	EXPECT_EQ(channelF.LoadRom(rom), LoadRomResult::UnknownType);
}

TEST(BinRomRouting, GenesisAcceptsSegaHeaderBin) {
	Emulator emu;
	GenesisConsole genesis(&emu);
	vector<uint8_t> romData(0x40000, 0);
	StampSegaMarker(romData);
	VirtualFile rom(romData.data(), romData.size(), "sonic.bin");

	ASSERT_EQ(genesis.LoadRom(rom), LoadRomResult::Success);
	EXPECT_EQ(genesis.GetRomFormat(), RomFormat::Genesis);
}

TEST(BinRomRouting, ChannelFAcceptsSmallBinProfile) {
	Emulator emu;
	ChannelFConsole channelF(&emu);
	vector<uint8_t> romData(0x0800, 0);
	VirtualFile rom(romData.data(), romData.size(), "cart.bin");

	ASSERT_EQ(channelF.LoadRom(rom), LoadRomResult::Success);
	EXPECT_EQ(channelF.GetRomFormat(), RomFormat::ChannelF);
}
