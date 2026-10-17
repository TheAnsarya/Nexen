#include "pch.h"
#include "Shared/Emulator.h"
#include "Atari2600/Atari2600Console.h"
#include "Shared/MemoryType.h"
#include "Utilities/VirtualFile.h"

namespace {
	VirtualFile MakeRomFile(const string& fileName, const vector<uint8_t>& data) {
		return VirtualFile(data.data(), data.size(), fileName);
	}

	TEST(Atari2600IntegrationWiringTests, AbsoluteAddressMapsToAtariMemoryTypesAndDeterministicOffsets) {
		Emulator emu;
		Atari2600Console console(&emu);

		vector<uint8_t> rom8k(8192, 0x11);
		std::fill(rom8k.begin() + 0x1000, rom8k.begin() + 0x2000, 0x22);
		VirtualFile romFile = MakeRomFile("atari-mapping-f8.a26", rom8k);
		ASSERT_EQ(console.LoadRom(romFile), LoadRomResult::Success);

		AddressInfo cartridgeAddress = {0x1000, MemoryType::NesMemory};
		AddressInfo firstBankAddress = console.GetAbsoluteAddress(cartridgeAddress);
		ASSERT_EQ(firstBankAddress.Type, MemoryType::Atari2600PrgRom);
		EXPECT_EQ(firstBankAddress.Address, 0x1000);

		console.DebugReadCartridge(0x1FF8);
		AddressInfo secondBankAddress = console.GetAbsoluteAddress(cartridgeAddress);
		ASSERT_EQ(secondBankAddress.Type, MemoryType::Atari2600PrgRom);
		EXPECT_EQ(secondBankAddress.Address, 0x0000);

		AddressInfo riotRamAddress = {0x008F, MemoryType::NesMemory};
		AddressInfo mappedRiotRam = console.GetAbsoluteAddress(riotRamAddress);
		EXPECT_EQ(mappedRiotRam.Type, MemoryType::Atari2600Ram);
		EXPECT_EQ(mappedRiotRam.Address, 0x0F);

		AddressInfo riotRegisterAddress = {0x0283, MemoryType::NesMemory};
		AddressInfo mappedRiotRegister = console.GetAbsoluteAddress(riotRegisterAddress);
		EXPECT_EQ(mappedRiotRegister.Type, MemoryType::Atari2600Ram);
		EXPECT_EQ(mappedRiotRegister.Address, 0x03);

		AddressInfo tiaRegisterAddress = {0x0015, MemoryType::NesMemory};
		AddressInfo mappedTiaRegister = console.GetAbsoluteAddress(tiaRegisterAddress);
		EXPECT_EQ(mappedTiaRegister.Type, MemoryType::Atari2600TiaRegisters);
		EXPECT_EQ(mappedTiaRegister.Address, 0x15);
	}

	TEST(Atari2600IntegrationWiringTests, ProgramCounterAbsoluteAddressStaysWithinRomBoundsDeterministically) {
		Emulator emuA;
		Atari2600Console consoleA(&emuA);
		vector<uint8_t> rom(4096, 0xEA);
		VirtualFile romFileA = MakeRomFile("atari-pc-abs-a.a26", rom);
		ASSERT_EQ(consoleA.LoadRom(romFileA), LoadRomResult::Success);
		AddressInfo pcAddressA = consoleA.GetPcAbsoluteAddress();

		Emulator emuB;
		Atari2600Console consoleB(&emuB);
		VirtualFile romFileB = MakeRomFile("atari-pc-abs-b.a26", rom);
		ASSERT_EQ(consoleB.LoadRom(romFileB), LoadRomResult::Success);
		AddressInfo pcAddressB = consoleB.GetPcAbsoluteAddress();

		EXPECT_EQ(pcAddressA.Type, MemoryType::Atari2600PrgRom);
		EXPECT_EQ(pcAddressB.Type, MemoryType::Atari2600PrgRom);
		EXPECT_GE(pcAddressA.Address, 0);
		EXPECT_GE(pcAddressB.Address, 0);
		EXPECT_LT(pcAddressA.Address, (int32_t)rom.size());
		EXPECT_LT(pcAddressB.Address, (int32_t)rom.size());
		EXPECT_EQ(pcAddressA.Address, pcAddressB.Address);
	}
}
