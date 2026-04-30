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

		vdpH40.Run(488);
		vdpH32.Run(488);
		GenesisVdpState line1H40 = vdpH40.GetState();
		GenesisVdpState line1H32 = vdpH32.GetState();
		EXPECT_TRUE(line1H40.DmaActive);
		EXPECT_TRUE(line1H32.DmaActive);
		EXPECT_EQ(line1H40.Registers[19], 0x01);
		EXPECT_EQ(line1H32.Registers[19], 0x01);

		vdpH40.Run(976);
		vdpH32.Run(976);
		GenesisVdpState line2H40 = vdpH40.GetState();
		GenesisVdpState line2H32 = vdpH32.GetState();
		EXPECT_FALSE(line2H40.DmaActive);
		EXPECT_TRUE(line2H32.DmaActive);
		EXPECT_EQ(line2H40.Registers[19], 0x00);
		EXPECT_EQ(line2H32.Registers[19], 0x01);
		EXPECT_EQ(line2H40.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_NE(line2H32.StatusRegister & VdpStatus::DmaBusy, 0);
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

		vdp.Run(488);
		GenesisVdpState afterLine1 = vdp.GetState();
		EXPECT_TRUE(afterLine1.DmaActive);
		EXPECT_EQ(afterLine1.Registers[19], 0x02);
		EXPECT_NE(afterLine1.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(976);
		GenesisVdpState afterLine2 = vdp.GetState();
		EXPECT_TRUE(afterLine2.DmaActive);
		EXPECT_EQ(afterLine2.Registers[19], 0x02);
		EXPECT_NE(afterLine2.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(1952);
		GenesisVdpState afterLine4 = vdp.GetState();
		EXPECT_FALSE(afterLine4.DmaActive);
		EXPECT_EQ(afterLine4.Registers[19], 0x00);
		EXPECT_EQ(afterLine4.Registers[20], 0x00);
		EXPECT_EQ(afterLine4.StatusRegister & VdpStatus::DmaBusy, 0);
	}
}
