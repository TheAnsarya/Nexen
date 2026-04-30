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

		vdpH40.Run(41);
		vdpH32.Run(41);
		GenesisVdpState cycle41H40 = vdpH40.GetState();
		GenesisVdpState cycle41H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle41H40.DmaActive);
		EXPECT_TRUE(cycle41H32.DmaActive);
		EXPECT_EQ(cycle41H40.Registers[19], 0x00);
		EXPECT_EQ(cycle41H32.Registers[19], 0x01);
		EXPECT_EQ(cycle41H40.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_NE(cycle41H32.StatusRegister & VdpStatus::DmaBusy, 0);

		vdpH32.Run(42);
		GenesisVdpState cycle42H32 = vdpH32.GetState();
		EXPECT_FALSE(cycle42H32.DmaActive);
		EXPECT_EQ(cycle42H32.Registers[19], 0x00);
		EXPECT_EQ(cycle42H32.StatusRegister & VdpStatus::DmaBusy, 0);
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

		vdp.Run(41);
		GenesisVdpState afterCycle41 = vdp.GetState();
		EXPECT_TRUE(afterCycle41.DmaActive);
		EXPECT_EQ(afterCycle41.Registers[19], 0x02);
		EXPECT_NE(afterCycle41.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(42);
		GenesisVdpState afterCycle42 = vdp.GetState();
		EXPECT_TRUE(afterCycle42.DmaActive);
		EXPECT_EQ(afterCycle42.Registers[19], 0x01);
		EXPECT_NE(afterCycle42.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(44);
		GenesisVdpState beforeSecondSlot = vdp.GetState();
		EXPECT_TRUE(beforeSecondSlot.DmaActive);
		EXPECT_EQ(beforeSecondSlot.Registers[19], 0x01);
		EXPECT_NE(beforeSecondSlot.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(45);
		GenesisVdpState afterCycle45 = vdp.GetState();
		EXPECT_FALSE(afterCycle45.DmaActive);
		EXPECT_EQ(afterCycle45.Registers[19], 0x00);
		EXPECT_EQ(afterCycle45.Registers[20], 0x00);
		EXPECT_EQ(afterCycle45.StatusRegister & VdpStatus::DmaBusy, 0);
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

		// H40 refined external-slot schedule starts at cycle 41.
		vdp.Run(40);
		GenesisVdpState beforeFirstSlot = vdp.GetState();
		EXPECT_EQ(beforeFirstSlot.Registers[19], 0x02);
		EXPECT_TRUE(beforeFirstSlot.DmaActive);

		vdp.Run(41);
		GenesisVdpState afterFirstSlot = vdp.GetState();
		EXPECT_EQ(afterFirstSlot.Registers[19], 0x01);
		EXPECT_TRUE(afterFirstSlot.DmaActive);

		// Between slot 41 and slot 43 there should be no additional decrement.
		vdp.Run(42);
		GenesisVdpState betweenSlots = vdp.GetState();
		EXPECT_EQ(betweenSlots.Registers[19], 0x01);
		EXPECT_TRUE(betweenSlots.DmaActive);

		vdp.Run(43);
		GenesisVdpState afterSecondSlot = vdp.GetState();
		EXPECT_EQ(afterSecondSlot.Registers[19], 0x00);
		EXPECT_FALSE(afterSecondSlot.DmaActive);
		EXPECT_EQ(afterSecondSlot.StatusRegister & VdpStatus::DmaBusy, 0);
	}
}
