#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
	VirtualFile MakeRomFile(const string& fileName, const vector<uint8_t>& data) {
		return VirtualFile(data.data(), data.size(), fileName);
	}

	TEST(Atari2600MapperPhaseATests, Supports2KAnd4KFixedMapping) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom2k(2048);
		for (size_t i = 0; i < rom2k.size(); i++) {
			rom2k[i] = (uint8_t)(i & 0xFF);
		}
		VirtualFile rom2kFile = MakeRomFile("mapper-2k.a26", rom2k);
		ASSERT_EQ(console.LoadRom(rom2kFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "2k");
		EXPECT_EQ(console.DebugReadCartridge(0x1000), rom2k[0]);
		EXPECT_EQ(console.DebugReadCartridge(0x1800), rom2k[0]);
		EXPECT_EQ(console.DebugReadCartridge(0x1FFF), rom2k[0x07FF]);

		vector<uint8_t> rom4k(4096);
		for (size_t i = 0; i < rom4k.size(); i++) {
			rom4k[i] = (uint8_t)(0x80 | (i & 0x7F));
		}
		VirtualFile rom4kFile = MakeRomFile("mapper-4k.a26", rom4k);
		ASSERT_EQ(console.LoadRom(rom4kFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "4k");
		EXPECT_EQ(console.DebugReadCartridge(0x1000), rom4k[0]);
		EXPECT_EQ(console.DebugReadCartridge(0x1ABC), rom4k[0x0ABC]);
		EXPECT_EQ(console.DebugReadCartridge(0x1FFF), rom4k[0x0FFF]);
	}

	TEST(Atari2600MapperPhaseATests, SupportsF8BankSwitching) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x11);
		std::fill(rom8k.begin() + 0x1000, rom8k.begin() + 0x2000, 0x22);
		VirtualFile romFile = MakeRomFile("mapper-f8.a26", rom8k);

		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "f8");
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 1u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x22);

		console.DebugReadCartridge(0x1FF8);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 0u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x11);

		console.DebugWriteCartridge(0x1FF9, 0x00);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 1u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x22);
	}

	TEST(Atari2600MapperPhaseATests, SupportsF6BankSwitching) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom16k(16384, 0x40);
		std::fill(rom16k.begin() + 0x1000, rom16k.begin() + 0x2000, 0x41);
		std::fill(rom16k.begin() + 0x2000, rom16k.begin() + 0x3000, 0x42);
		std::fill(rom16k.begin() + 0x3000, rom16k.begin() + 0x4000, 0x43);
		VirtualFile romFile = MakeRomFile("mapper-f6.a26", rom16k);

		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "f6");
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 0u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x40);

		console.DebugReadCartridge(0x1FF7);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 1u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x41);

		console.DebugWriteCartridge(0x1FF8, 0x00);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 2u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x42);

		console.DebugWriteCartridge(0x1FF9, 0x00);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 3u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x43);
	}

	TEST(Atari2600MapperPhaseATests, BaselineHarnessDigestStableForPhaseACorpus) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<Atari2600BaselineRomCase> romSet;
		romSet.push_back({"phasea-2k.a26", vector<uint8_t>(2048, 0xEA)});
		romSet.push_back({"phasea-4k.a26", vector<uint8_t>(4096, 0xEA)});

		vector<uint8_t> f8(8192, 0xEA);
		std::fill(f8.begin() + 0x1000, f8.begin() + 0x2000, 0x00);
		romSet.push_back({"phasea-f8.a26", f8});

		vector<uint8_t> f6(16384, 0xEA);
		std::fill(f6.begin() + 0x1000, f6.begin() + 0x2000, 0x00);
		std::fill(f6.begin() + 0x2000, f6.begin() + 0x3000, 0xFF);
		std::fill(f6.begin() + 0x3000, f6.begin() + 0x4000, 0x55);
		romSet.push_back({"phasea-f6.a26", f6});

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(runA.PassCount, (int)romSet.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}
}
