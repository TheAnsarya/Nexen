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

	TEST(Atari2600MapperPhaseATests, F8InvalidOrderHotspotSequenceKeepsDeterministicBankState) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x11);
		std::fill(rom8k.begin() + 0x1000, rom8k.begin() + 0x2000, 0x22);
		VirtualFile romFile = MakeRomFile("mapper-f8-invalid-order.a26", rom8k);

		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		ASSERT_EQ(console.DebugGetMapperMode(), "f8");

		struct Step {
			uint16_t Addr;
			uint8_t ExpectedBank;
			uint8_t ExpectedValue;
		};

		const std::array<Step, 6> steps = {{
			{0x1ff8, 0, 0x11},
			{0x1ff8, 0, 0x11},
			{0x1ff7, 0, 0x11},
			{0x1ff9, 1, 0x22},
			{0x1ff9, 1, 0x22},
			{0x1ff6, 1, 0x22},
		}};

		for (size_t i = 0; i < steps.size(); i++) {
			const Step& step = steps[i];
			SCOPED_TRACE(string("mapper=f8;step=") + std::to_string(i)
				+ ";addr=" + std::to_string(step.Addr));

			console.DebugReadCartridge(step.Addr);
			uint8_t bank = console.DebugGetMapperBankIndex();
			uint8_t value = console.DebugReadCartridge(0x1000);
			EXPECT_EQ(bank, step.ExpectedBank) << "mapper=f8;field=bank;step=" << i;
			EXPECT_EQ(value, step.ExpectedValue) << "mapper=f8;field=value;step=" << i;
		}
	}

	TEST(Atari2600MapperPhaseATests, F6InvalidOrderHotspotSequenceKeepsDeterministicBankState) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom16k(16384, 0x40);
		std::fill(rom16k.begin() + 0x1000, rom16k.begin() + 0x2000, 0x41);
		std::fill(rom16k.begin() + 0x2000, rom16k.begin() + 0x3000, 0x42);
		std::fill(rom16k.begin() + 0x3000, rom16k.begin() + 0x4000, 0x43);
		VirtualFile romFile = MakeRomFile("mapper-f6-invalid-order.a26", rom16k);

		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		ASSERT_EQ(console.DebugGetMapperMode(), "f6");

		struct Step {
			uint16_t Addr;
			uint8_t ExpectedBank;
			uint8_t ExpectedValue;
		};

		const std::array<Step, 6> steps = {{
			{0x1ff9, 3, 0x43},
			{0x1ff4, 3, 0x43},
			{0x1ff6, 0, 0x40},
			{0x1ff8, 2, 0x42},
			{0x1ff7, 1, 0x41},
			{0x1ff5, 1, 0x41},
		}};

		for (size_t i = 0; i < steps.size(); i++) {
			const Step& step = steps[i];
			SCOPED_TRACE(string("mapper=f6;step=") + std::to_string(i)
				+ ";addr=" + std::to_string(step.Addr));

			console.DebugReadCartridge(step.Addr);
			uint8_t bank = console.DebugGetMapperBankIndex();
			uint8_t value = console.DebugReadCartridge(0x1000);
			EXPECT_EQ(bank, step.ExpectedBank) << "mapper=f6;field=bank;step=" << i;
			EXPECT_EQ(value, step.ExpectedValue) << "mapper=f6;field=value;step=" << i;
		}
	}

	TEST(Atari2600MapperPhaseATests, BaselineHarnessDigestStableForPhaseACorpus) {
		Emulator emu;
		Atari2600Console console(&emu);

		auto makeNopRom = [](size_t size) -> vector<uint8_t> {
			vector<uint8_t> rom(size, 0xEA);
			for (size_t offset = 0x1000; offset <= size; offset += 0x1000) {
				rom[offset - 3] = 0x4C;
				rom[offset - 2] = 0x00;
				rom[offset - 1] = 0x10;
			}
			if (size < 0x1000) {
				rom[size - 3] = 0x4C;
				rom[size - 2] = 0x00;
				rom[size - 1] = 0x10;
			}
			return rom;
		};

		vector<Atari2600BaselineRomCase> romSet;
		romSet.push_back({"phasea-2k.a26", makeNopRom(2048)});
		romSet.push_back({"phasea-4k.a26", makeNopRom(4096)});
		romSet.push_back({"phasea-f8.a26", makeNopRom(8192)});
		romSet.push_back({"phasea-f6.a26", makeNopRom(16384)});

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(runA.PassCount, (int)romSet.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}
}
