#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
	VirtualFile MakeRomFile(const string& fileName, const vector<uint8_t>& data) {
		return VirtualFile(data.data(), data.size(), fileName);
	}

	TEST(Atari2600MapperPhaseCTests, Supports3FTigervisionWindowSwitching) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x00);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0800,
				rom8k.begin() + ((size_t)bank + 1) * 0x0800,
				(uint8_t)(0x40 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper-3f.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "3f");

		// Default maps bank 0 at $1000-$17ff and fixed last bank at $1800-$1fff.
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x40);
		EXPECT_EQ(console.DebugReadCartridge(0x1800), 0x43);

		console.DebugWriteCartridge(0x003f, 0x02);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x42);
		EXPECT_EQ(console.DebugReadCartridge(0x1800), 0x43);
	}

	TEST(Atari2600MapperPhaseCTests, RareFallbackUsesValueSelectedBank) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom16k(16384, 0x00);
		for (uint8_t bank = 0; bank < 4; bank++) {
			std::fill(
				rom16k.begin() + (size_t)bank * 0x1000,
				rom16k.begin() + ((size_t)bank + 1) * 0x1000,
				(uint8_t)(0x60 + bank));
		}

		VirtualFile romFile = MakeRomFile("rare-homebrew-cart.a26", rom16k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "fallback");

		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x60);

		console.DebugWriteCartridge(0x1ff8, 0x03);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x63);

		console.DebugWriteCartridge(0x1ffa, 0x09);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x61);
	}

	TEST(Atari2600MapperPhaseCTests, BaselineHarnessDigestStableForPhaseCCorpus) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<Atari2600BaselineRomCase> romSet;
		romSet.push_back({"phasec-3f.a26", vector<uint8_t>(8192, 0xEA)});
		romSet.push_back({"phasec-rare-homebrew.a26", vector<uint8_t>(16384, 0xEA)});

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(runA.PassCount, (int)romSet.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}
}
