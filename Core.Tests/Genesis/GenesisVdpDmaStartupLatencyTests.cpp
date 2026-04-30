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
		vdp.WriteControlPort(0x8150);
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

		vdpH40.Run(19);
		vdpH32.Run(19);
		GenesisVdpState cycle19H40 = vdpH40.GetState();
		GenesisVdpState cycle19H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle19H40.DmaActive);
		EXPECT_TRUE(cycle19H32.DmaActive);
		EXPECT_EQ(cycle19H40.Registers[19], 0x00);
		EXPECT_EQ(cycle19H32.Registers[19], 0x01);
		EXPECT_EQ(cycle19H40.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_NE(cycle19H32.StatusRegister & VdpStatus::DmaBusy, 0);

		vdpH32.Run(23);
		GenesisVdpState cycle23H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle23H32.DmaActive);
		EXPECT_EQ(cycle23H32.Registers[19], 0x00);
		EXPECT_EQ(cycle23H32.StatusRegister & VdpStatus::DmaBusy, 0);
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

		vdp.Run(21);
		GenesisVdpState afterCycle21 = vdp.GetState();
		EXPECT_TRUE(afterCycle21.DmaActive);
		EXPECT_EQ(afterCycle21.Registers[19], 0x02);
		EXPECT_NE(afterCycle21.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(23);
		GenesisVdpState afterCycle23 = vdp.GetState();
		EXPECT_TRUE(afterCycle23.DmaActive);
		EXPECT_EQ(afterCycle23.Registers[19], 0x01);
		EXPECT_NE(afterCycle23.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(57);
		GenesisVdpState afterCycle57 = vdp.GetState();
		EXPECT_FALSE(afterCycle57.DmaActive);
		EXPECT_EQ(afterCycle57.Registers[19], 0x00);
		EXPECT_EQ(afterCycle57.Registers[20], 0x00);
		EXPECT_EQ(afterCycle57.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, ActiveDisplayBusDmaAdvancesOnlyOnExternalSlots) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, true, 0x02);

		// H40 slot table starts at cycle 18. Ensure no progress before the slot.
		vdp.Run(17);
		GenesisVdpState beforeFirstSlot = vdp.GetState();
		EXPECT_EQ(beforeFirstSlot.Registers[19], 0x02);
		EXPECT_TRUE(beforeFirstSlot.DmaActive);

		vdp.Run(19);
		GenesisVdpState afterFirstSlot = vdp.GetState();
		EXPECT_EQ(afterFirstSlot.Registers[19], 0x01);
		EXPECT_TRUE(afterFirstSlot.DmaActive);

		// Between slot 18 and slot 54 there should be no additional decrement.
		vdp.Run(53);
		GenesisVdpState betweenSlots = vdp.GetState();
		EXPECT_EQ(betweenSlots.Registers[19], 0x01);
		EXPECT_TRUE(betweenSlots.DmaActive);

		vdp.Run(55);
		GenesisVdpState afterSecondSlot = vdp.GetState();
		EXPECT_EQ(afterSecondSlot.Registers[19], 0x00);
		EXPECT_FALSE(afterSecondSlot.DmaActive);
		EXPECT_EQ(afterSecondSlot.StatusRegister & VdpStatus::DmaBusy, 0);
	}
}
