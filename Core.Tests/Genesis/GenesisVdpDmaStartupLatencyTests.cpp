#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"

namespace {
	vector<uint8_t> BuildDmaSourceRom(size_t size = 0x2000) {
		vector<uint8_t> rom(size, 0);
		for (size_t i = 0; i + 1 < rom.size(); i += 2) {
			rom[i] = (uint8_t)((i >> 1) & 0xff);
			rom[i + 1] = (uint8_t)(0xff - ((i >> 1) & 0xff));
		}
		return rom;
	}

	void ConfigureBusDmaTransfer(GenesisVdp& vdp, bool h40Mode, uint8_t lengthLow) {
		vdp.WriteControlPort(0x8110);
		vdp.WriteControlPort((uint16_t)(h40Mode ? 0x8c01 : 0x8c00));
		vdp.WriteControlPort(0x8f02);
		vdp.WriteControlPort((uint16_t)(0x9300 | lengthLow));
		vdp.WriteControlPort(0x9400);
		vdp.WriteControlPort(0x9500);
		vdp.WriteControlPort(0x9600);
		vdp.WriteControlPort(0x9700);
		vdp.WriteControlPort(0x4000);
		vdp.WriteControlPort(0x0080);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, H40CompletesEarlierThanH32ForBusDmaStartupDelay) {
		vector<uint8_t> romA = BuildDmaSourceRom();
		vector<uint8_t> romB = BuildDmaSourceRom();

		Emulator emuA;
		emuA.Initialize(false);
		GenesisMemoryManager mmA;
		mmA.Init(&emuA, nullptr, romA, nullptr, nullptr, nullptr);
		GenesisVdp vdpH40;
		vdpH40.Init(&emuA, nullptr, nullptr, &mmA);
		ConfigureBusDmaTransfer(vdpH40, true, 0x01);

		Emulator emuB;
		emuB.Initialize(false);
		GenesisMemoryManager mmB;
		mmB.Init(&emuB, nullptr, romB, nullptr, nullptr, nullptr);
		GenesisVdp vdpH32;
		vdpH32.Init(&emuB, nullptr, nullptr, &mmB);
		ConfigureBusDmaTransfer(vdpH32, false, 0x01);

		GenesisVdpState preH40 = vdpH40.GetState();
		GenesisVdpState preH32 = vdpH32.GetState();
		EXPECT_TRUE(preH40.DmaActive);
		EXPECT_TRUE(preH32.DmaActive);
		EXPECT_NE(preH40.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_NE(preH32.StatusRegister & VdpStatus::DmaBusy, 0);

		vdpH40.Run(12);
		vdpH32.Run(12);
		GenesisVdpState cycle12H40 = vdpH40.GetState();
		GenesisVdpState cycle12H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle12H40.DmaActive);
		EXPECT_TRUE(cycle12H32.DmaActive);
		EXPECT_EQ(cycle12H40.Registers[19], 0x00);
		EXPECT_EQ(cycle12H32.Registers[19], 0x01);
		EXPECT_EQ(cycle12H40.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_NE(cycle12H32.StatusRegister & VdpStatus::DmaBusy, 0);

		vdpH32.Run(15);
		GenesisVdpState cycle15H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle15H32.DmaActive);
		EXPECT_EQ(cycle15H32.Registers[19], 0x00);
		EXPECT_EQ(cycle15H32.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, RemainingLengthAndBusyBitTransitionDeterministicallyAcrossDelay) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, false, 0x02);

		GenesisVdpState start = vdp.GetState();
		EXPECT_TRUE(start.DmaActive);
		EXPECT_EQ(start.Registers[19], 0x02);
		EXPECT_NE(start.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(8);
		GenesisVdpState afterCycle8 = vdp.GetState();
		EXPECT_TRUE(afterCycle8.DmaActive);
		EXPECT_EQ(afterCycle8.Registers[19], 0x02);
		EXPECT_NE(afterCycle8.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(15);
		GenesisVdpState afterCycle15 = vdp.GetState();
		EXPECT_TRUE(afterCycle15.DmaActive);
		EXPECT_EQ(afterCycle15.Registers[19], 0x01);
		EXPECT_NE(afterCycle15.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(21);
		GenesisVdpState afterCycle21 = vdp.GetState();
		EXPECT_FALSE(afterCycle21.DmaActive);
		EXPECT_EQ(afterCycle21.Registers[19], 0x00);
		EXPECT_EQ(afterCycle21.Registers[20], 0x00);
		EXPECT_EQ(afterCycle21.StatusRegister & VdpStatus::DmaBusy, 0);
	}
}
