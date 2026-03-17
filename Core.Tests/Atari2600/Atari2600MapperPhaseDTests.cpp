#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
	VirtualFile MakeRomFile(const string& fileName, const vector<uint8_t>& data) {
		return VirtualFile(data.data(), data.size(), fileName);
	}

	TEST(Atari2600MapperPhaseDTests, Mapper3FSwitchesOnlyOn3FStrobes) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x00);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0800,
				rom8k.begin() + ((size_t)bank + 1) * 0x0800,
				(uint8_t)(0x70 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper-3f-strobe-accurate.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		ASSERT_EQ(console.DebugGetMapperMode(), "3f");
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x70);

		console.DebugWriteCartridge(0x0001, 0x02);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x70);

		console.DebugWriteCartridge(0x003f, 0x02);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x72);

		console.DebugWriteCartridge(0x007f, 0x03);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x73);
	}

	TEST(Atari2600MapperPhaseDTests, Mapper3FStrobesUseMirrorsDeterministically) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x00);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0800,
				rom8k.begin() + ((size_t)bank + 1) * 0x0800,
				(uint8_t)(0x20 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper3f-tigervision.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		ASSERT_EQ(console.DebugGetMapperMode(), "3f");

		console.DebugWriteCartridge(0x043f, 0x01);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x21);

		console.DebugWriteCartridge(0x083f, 0x02);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x22);

		console.DebugWriteCartridge(0x0c3f, 0x03);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x23);
	}

	TEST(Atari2600MapperPhaseDTests, BaselineHarnessDigestStableForPhaseDCorpus) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<Atari2600BaselineRomCase> romSet;
		romSet.push_back({"phased-3f-strobe.a26", vector<uint8_t>(8192, 0xEA)});
		romSet.push_back({"phased-fallback-homebrew.a26", vector<uint8_t>(16384, 0xEA)});

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(runA.PassCount, (int)romSet.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}
}
