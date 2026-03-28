#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "Shared/Emulator.h"

TEST(ChannelFConsole, SupportedExtensionsContainChfOnly) {
	vector<string> ext = ChannelFConsole::GetSupportedExtensions();
	ASSERT_EQ(ext.size(), 1u);
	EXPECT_EQ(ext[0], ".chf");
}

TEST(ChannelFConsole, LoadRomAndRunFrameUpdatesState) {
	uint8_t romBytes[16] = {0};
	VirtualFile rom(romBytes, sizeof(romBytes), "test.chf");
	Emulator emu;
	ChannelFConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	EXPECT_EQ(console.GetConsoleType(), ConsoleType::ChannelF);
	EXPECT_EQ(console.GetRomFormat(), RomFormat::ChannelF);

	uint64_t initialClock = console.GetMasterClock();
	uint32_t initialFrame = console.GetPpuFrame().FrameCount;
	console.RunFrame();
	EXPECT_GT(console.GetMasterClock(), initialClock);
	EXPECT_EQ(console.GetPpuFrame().FrameCount, initialFrame + 1u);
}
