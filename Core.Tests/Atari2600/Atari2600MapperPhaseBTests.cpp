#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Atari2600/Atari2600SmokeHarness.h"
#include "Utilities/VirtualFile.h"

namespace {
	VirtualFile MakeRomFile(const string& fileName, const vector<uint8_t>& data) {
		return VirtualFile(data.data(), data.size(), fileName);
	}

	TEST(Atari2600MapperPhaseBTests, SupportsF4BankSwitching) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom32k(32768, 0x30);
		for (uint8_t bank = 0; bank < 8; bank++) {
			std::fill(
				rom32k.begin() + (size_t)bank * 0x1000,
				rom32k.begin() + ((size_t)bank + 1) * 0x1000,
				(uint8_t)(0x30 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper-f4.a26", rom32k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "f4");

		console.DebugReadCartridge(0x1FF4);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 0u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x30);

		console.DebugWriteCartridge(0x1FFB, 0x00);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 7u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x37);
	}

	TEST(Atari2600MapperPhaseBTests, SupportsFEBankSwitching) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0xA1);
		std::fill(rom8k.begin() + 0x1000, rom8k.begin() + 0x2000, 0xB2);

		VirtualFile romFile = MakeRomFile("mapper-fe.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "fe");


		console.DebugReadCartridge(0x1FFF);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 1u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0xB2);

		console.DebugWriteCartridge(0x1FFE, 0x00);
		EXPECT_EQ(console.DebugGetMapperBankIndex(), 0u);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0xA1);
	}

	TEST(Atari2600MapperPhaseBTests, SupportsE0SegmentedWindows) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x00);
		for (uint8_t bank = 0; bank < 8; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0400,
				rom8k.begin() + ((size_t)bank + 1) * 0x0400,
				(uint8_t)(0x10 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper-e0.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		EXPECT_EQ(console.DebugGetMapperMode(), "e0");
		// Initial mapping: segment banks 4/5/6 and fixed bank 7.
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x14);
		EXPECT_EQ(console.DebugReadCartridge(0x1400), 0x15);
		EXPECT_EQ(console.DebugReadCartridge(0x1800), 0x16);
		EXPECT_EQ(console.DebugReadCartridge(0x1C00), 0x17);

		console.DebugReadCartridge(0x1FE2);
		EXPECT_EQ(console.DebugReadCartridge(0x1000), 0x12);

		console.DebugWriteCartridge(0x1FEB, 0x00);
		EXPECT_EQ(console.DebugReadCartridge(0x1400), 0x13);

		console.DebugWriteCartridge(0x1FF0, 0x00);
		EXPECT_EQ(console.DebugReadCartridge(0x1800), 0x10);
		EXPECT_EQ(console.DebugReadCartridge(0x1C00), 0x17);
	}

	TEST(Atari2600MapperPhaseBTests, E0InvalidOrderHotspotSequenceKeepsSegmentStability) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x00);
		for (uint8_t bank = 0; bank < 8; bank++) {
			std::fill(
				rom8k.begin() + (size_t)bank * 0x0400,
				rom8k.begin() + ((size_t)bank + 1) * 0x0400,
				(uint8_t)(0x10 + bank));
		}

		VirtualFile romFile = MakeRomFile("mapper-e0-invalid-order.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);
		ASSERT_EQ(console.DebugGetMapperMode(), "e0");

		struct Step {
			uint16_t Addr;
			uint8_t ExpectedLo;
			uint8_t ExpectedMid;
			uint8_t ExpectedHi;
		};

		const std::array<Step, 6> steps = {{
			{0x1ff7, 0x14, 0x15, 0x17},
			{0x1fe1, 0x11, 0x15, 0x17},
			{0x1ff5, 0x11, 0x15, 0x15},
			{0x1fee, 0x11, 0x16, 0x15},
			{0x1fe3, 0x13, 0x16, 0x15},
			{0x1ffa, 0x13, 0x16, 0x15},
		}};

		for (size_t i = 0; i < steps.size(); i++) {
			const Step& step = steps[i];
			SCOPED_TRACE(string("mapper=e0;step=") + std::to_string(i)
				+ ";addr=" + std::to_string(step.Addr));

			console.DebugReadCartridge(step.Addr);
			uint8_t lo = console.DebugReadCartridge(0x1000);
			uint8_t mid = console.DebugReadCartridge(0x1400);
			uint8_t hi = console.DebugReadCartridge(0x1800);

			EXPECT_EQ(lo, step.ExpectedLo) << "mapper=e0;field=lo;step=" << i;
			EXPECT_EQ(mid, step.ExpectedMid) << "mapper=e0;field=mid;step=" << i;
			EXPECT_EQ(hi, step.ExpectedHi) << "mapper=e0;field=hi;step=" << i;
		}
	}

	TEST(Atari2600MapperPhaseBTests, BaselineHarnessDigestStableForPhaseBCorpus) {
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
		romSet.push_back({"phaseb-f4.a26", makeNopRom(32768)});
		romSet.push_back({"phaseb-fe.a26", makeNopRom(8192)});
		romSet.push_back({"phaseb-e0.a26", makeNopRom(8192)});

		Atari2600BaselineRomSetResult runA = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);
		Atari2600BaselineRomSetResult runB = Atari2600SmokeHarness::RunBaselineRomSet(console, romSet);

		EXPECT_EQ(runA.PassCount, (int)romSet.size());
		EXPECT_EQ(runA.FailCount, 0);
		EXPECT_FALSE(runA.Digest.empty());
		EXPECT_EQ(runA.Digest, runB.Digest);
	}
}
