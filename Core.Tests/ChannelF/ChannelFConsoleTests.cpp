#include "pch.h"
#include "ChannelF/ChannelFConsole.h"
#include "Shared/Emulator.h"
#include "Shared/EmuSettings.h"

TEST(ChannelFConsole, SupportedExtensionsContainChfAndBin) {
	vector<string> ext = ChannelFConsole::GetSupportedExtensions();
	ASSERT_EQ(ext.size(), 2u);
	EXPECT_EQ(ext[0], ".chf");
	EXPECT_EQ(ext[1], ".bin");
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

TEST(ChannelFConsole, DefaultRegionIsNtsc) {
	uint8_t romBytes[16] = {0};
	VirtualFile rom(romBytes, sizeof(romBytes), "test.chf");
	Emulator emu;
	ChannelFConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	EXPECT_EQ(console.GetRegion(), ConsoleRegion::Ntsc);
	EXPECT_EQ(console.GetMasterClockRate(), ChannelFConsole::NtscCpuClockHz);
	EXPECT_EQ(console.GetFps(), ChannelFConsole::NtscFps);
}

TEST(ChannelFConsole, RegionOverridePalChangesTiming) {
	uint8_t romBytes[16] = {0};
	VirtualFile rom(romBytes, sizeof(romBytes), "test.chf");
	Emulator emu;
	emu.GetSettings()->GetChannelFConfig().Region = ConsoleRegion::Pal;
	ChannelFConsole console(&emu);

	ASSERT_EQ(console.LoadRom(rom), LoadRomResult::Success);
	EXPECT_EQ(console.GetRegion(), ConsoleRegion::Pal);
	EXPECT_EQ(console.GetMasterClockRate(), ChannelFConsole::PalCpuClockHz);
	EXPECT_EQ(console.GetFps(), ChannelFConsole::PalFps);
	EXPECT_EQ(console.GetPpuFrame().ScanlineCount, ChannelFConsole::PalScanlineCount);
}
