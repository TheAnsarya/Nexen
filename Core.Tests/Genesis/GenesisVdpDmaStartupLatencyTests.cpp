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

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaSourceWritebackPreservesR23LatchedValueAfterFirstSlot) {
		vector<uint8_t> rom((size_t)0x900000, 0);
		rom[0x000000] = 0x11;
		rom[0x000001] = 0x22;
		rom[0x800000] = 0xaa;
		rom[0x800001] = 0xbb;

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8110); // DMA enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9301); // length low = 1 word
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9740); // src high includes bit6, bus DMA mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState postStep = vdp.GetState();

		EXPECT_EQ(postStep.Registers[23], 0x40);
		EXPECT_EQ(postStep.Registers[21], 0x01);
		EXPECT_EQ(postStep.Registers[22], 0x00);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaSourceWrapsInsideCurrent128KbWindow) {
		vector<uint8_t> rom((size_t)0x30000, 0);
		rom[0x01FFFE] = 0x12;
		rom[0x01FFFF] = 0x34;
		rom[0x000000] = 0x56;
		rom[0x000001] = 0x78;
		rom[0x020000] = 0x56;
		rom[0x020001] = 0x78;

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9302); // length low = 2 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x95ff); // src low (word addr low)
		vdp.WriteControlPort(0x96ff); // src mid (word addr mid)
		vdp.WriteControlPort(0x9700); // src high / mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_EQ(afterFirst.Registers[21], 0x00);
		EXPECT_EQ(afterFirst.Registers[22], 0x00);
		EXPECT_EQ(afterFirst.Registers[23], 0x00);
		uint8_t r23 = afterFirst.Registers[23];

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_EQ(afterSecond.Registers[21], 0x01);
		EXPECT_EQ(afterSecond.Registers[22], 0x00);
		EXPECT_EQ(afterSecond.Registers[23], r23);
		EXPECT_FALSE(afterSecond.DmaActive);

		uint8_t* vram = vdp.GetVramPointer();
		EXPECT_EQ(vram[0x0000], 0x12);
		EXPECT_EQ(vram[0x0001], 0x34);
		EXPECT_EQ(vram[0x0002], 0x56);
		EXPECT_EQ(vram[0x0003], 0x78);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaR23LatchedValueStaysStableAcrossMultipleSlots) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9302); // length low = 2 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9741); // src high low7 includes 0x41, mode bits select bus DMA
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_EQ(afterFirst.Registers[23], 0x41);
		EXPECT_EQ(afterFirst.Registers[21], 0x01);
		EXPECT_EQ(afterFirst.Registers[22], 0x00);

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_EQ(afterSecond.Registers[23], 0x41);
		EXPECT_EQ(afterSecond.Registers[21], 0x02);
		EXPECT_EQ(afterSecond.Registers[22], 0x00);
		EXPECT_FALSE(afterSecond.DmaActive);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaNonZeroR23LatchRemainsStableWhenSourceWindowWraps) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9302); // length low = 2 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x95ff); // src low near 128KB window edge
		vdp.WriteControlPort(0x96ff); // src mid near 128KB window edge
		vdp.WriteControlPort(0x9741); // non-zero source-high latch + bus DMA mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_EQ(afterFirst.Registers[23], 0x41);
		EXPECT_EQ(afterFirst.Registers[21], 0x00);
		EXPECT_EQ(afterFirst.Registers[22], 0x00);

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_EQ(afterSecond.Registers[23], 0x41);
		EXPECT_EQ(afterSecond.Registers[21], 0x01);
		EXPECT_EQ(afterSecond.Registers[22], 0x00);
		EXPECT_FALSE(afterSecond.DmaActive);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaMidTransferR23WriteAppliesWithoutBreakingProgression) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9302); // length low = 2 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9741); // non-zero source-high latch + bus DMA mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_TRUE(afterFirst.DmaActive);
		EXPECT_EQ(afterFirst.Registers[23], 0x41);
		EXPECT_EQ(afterFirst.Registers[19], 0x01);

		// Attempt to rewrite R23 during active DMA.
		vdp.WriteControlPort(0x97c0);

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_FALSE(afterSecond.DmaActive);
		EXPECT_EQ(afterSecond.Registers[23], 0xc0);
		EXPECT_EQ(afterSecond.Registers[21], 0x02);
		EXPECT_EQ(afterSecond.Registers[22], 0x00);
		EXPECT_EQ(afterSecond.Registers[19], 0x00);
		EXPECT_EQ(afterSecond.Registers[20], 0x00);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaCompletionClearsLengthAndBusyBitsAtBoundary) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9301); // length low = 1 word
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9741); // non-zero source-high latch + bus DMA mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState completed = vdp.GetState();
		EXPECT_FALSE(completed.DmaActive);
		EXPECT_EQ(completed.Registers[19], 0x00);
		EXPECT_EQ(completed.Registers[20], 0x00);
		EXPECT_EQ(completed.StatusRegister & VdpStatus::DmaBusy, 0);
		EXPECT_EQ(completed.Registers[23], 0x41);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaRepeatedMidTransferR23WritesKeepThreeSlotProgressionDeterministic) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9303); // length low = 3 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9741); // initial non-zero source-high latch
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_TRUE(afterFirst.DmaActive);
		EXPECT_EQ(afterFirst.Registers[19], 0x02);
		EXPECT_EQ(afterFirst.Registers[21], 0x01);
		EXPECT_EQ(afterFirst.Registers[23], 0x41);

		vdp.WriteControlPort(0x97c0);

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_TRUE(afterSecond.DmaActive);
		EXPECT_EQ(afterSecond.Registers[19], 0x01);
		EXPECT_EQ(afterSecond.Registers[21], 0x02);
		EXPECT_EQ(afterSecond.Registers[23], 0xc0);

		vdp.WriteControlPort(0x9742);

		vdp.Run(46);
		GenesisVdpState afterThird = vdp.GetState();
		EXPECT_FALSE(afterThird.DmaActive);
		EXPECT_EQ(afterThird.Registers[19], 0x00);
		EXPECT_EQ(afterThird.Registers[20], 0x00);
		EXPECT_EQ(afterThird.Registers[21], 0x03);
		EXPECT_EQ(afterThird.Registers[22], 0x00);
		EXPECT_EQ(afterThird.Registers[23], 0x42);
		EXPECT_EQ(afterThird.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, BusDmaLengthThreeCompletionBoundaryClearsBusyAndLengthExactlyOnThirdSlot) {
		vector<uint8_t> rom((size_t)0x200000, 0);

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);

		vdp.WriteControlPort(0x8150); // DMA + display enable
		vdp.WriteControlPort(0x8c01); // H40
		vdp.WriteControlPort(0x8f02); // auto-increment
		vdp.WriteControlPort(0x9303); // length low = 3 words
		vdp.WriteControlPort(0x9400); // length high
		vdp.WriteControlPort(0x9500); // src low
		vdp.WriteControlPort(0x9600); // src mid
		vdp.WriteControlPort(0x9741); // non-zero source-high latch + bus DMA mode
		vdp.WriteControlPort(0x4000); // addr low + mode low
		vdp.WriteControlPort(0x0080); // start DMA

		vdp.Run(41);
		GenesisVdpState afterFirst = vdp.GetState();
		EXPECT_TRUE(afterFirst.DmaActive);
		EXPECT_EQ(afterFirst.Registers[19], 0x02);
		EXPECT_NE(afterFirst.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(43);
		GenesisVdpState afterSecond = vdp.GetState();
		EXPECT_TRUE(afterSecond.DmaActive);
		EXPECT_EQ(afterSecond.Registers[19], 0x01);
		EXPECT_NE(afterSecond.StatusRegister & VdpStatus::DmaBusy, 0);

		vdp.Run(46);
		GenesisVdpState afterThird = vdp.GetState();
		EXPECT_FALSE(afterThird.DmaActive);
		EXPECT_EQ(afterThird.Registers[19], 0x00);
		EXPECT_EQ(afterThird.Registers[20], 0x00);
		EXPECT_EQ(afterThird.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, H40LateLineExternalSlotsRemainSlotGated) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, true, 0x0e);

		vdp.Run(460);
		GenesisVdpState beforeLateSlots = vdp.GetState();
		EXPECT_EQ(beforeLateSlots.Registers[19], 0x05);
		EXPECT_TRUE(beforeLateSlots.DmaActive);

		vdp.Run(461);
		GenesisVdpState after461 = vdp.GetState();
		EXPECT_EQ(after461.Registers[19], 0x04);
		EXPECT_TRUE(after461.DmaActive);

		vdp.Run(462);
		GenesisVdpState after462 = vdp.GetState();
		EXPECT_EQ(after462.Registers[19], 0x04);
		EXPECT_TRUE(after462.DmaActive);

		vdp.Run(463);
		GenesisVdpState after463 = vdp.GetState();
		EXPECT_EQ(after463.Registers[19], 0x03);
		EXPECT_TRUE(after463.DmaActive);

		vdp.Run(464);
		GenesisVdpState after464 = vdp.GetState();
		EXPECT_EQ(after464.Registers[19], 0x03);
		EXPECT_TRUE(after464.DmaActive);

		vdp.Run(465);
		GenesisVdpState after465 = vdp.GetState();
		EXPECT_EQ(after465.Registers[19], 0x02);
		EXPECT_TRUE(after465.DmaActive);

		vdp.Run(467);
		GenesisVdpState before468 = vdp.GetState();
		EXPECT_EQ(before468.Registers[19], 0x02);
		EXPECT_TRUE(before468.DmaActive);

		vdp.Run(468);
		GenesisVdpState after468 = vdp.GetState();
		EXPECT_EQ(after468.Registers[19], 0x01);
		EXPECT_TRUE(after468.DmaActive);

		vdp.Run(471);
		GenesisVdpState before472 = vdp.GetState();
		EXPECT_EQ(before472.Registers[19], 0x01);
		EXPECT_TRUE(before472.DmaActive);

		vdp.Run(472);
		GenesisVdpState after472 = vdp.GetState();
		EXPECT_EQ(after472.Registers[19], 0x00);
		EXPECT_FALSE(after472.DmaActive);
		EXPECT_EQ(after472.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, H32LateLineExternalSlotsRemainSlotGated) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, false, 0x0e);

		vdp.Run(453);
		GenesisVdpState beforeLateSlots = vdp.GetState();
		EXPECT_EQ(beforeLateSlots.Registers[19], 0x05);
		EXPECT_TRUE(beforeLateSlots.DmaActive);

		vdp.Run(454);
		GenesisVdpState after454 = vdp.GetState();
		EXPECT_EQ(after454.Registers[19], 0x04);
		EXPECT_TRUE(after454.DmaActive);

		vdp.Run(456);
		GenesisVdpState before457 = vdp.GetState();
		EXPECT_EQ(before457.Registers[19], 0x04);
		EXPECT_TRUE(before457.DmaActive);

		vdp.Run(457);
		GenesisVdpState after457 = vdp.GetState();
		EXPECT_EQ(after457.Registers[19], 0x03);
		EXPECT_TRUE(after457.DmaActive);

		vdp.Run(459);
		GenesisVdpState before460 = vdp.GetState();
		EXPECT_EQ(before460.Registers[19], 0x03);
		EXPECT_TRUE(before460.DmaActive);

		vdp.Run(460);
		GenesisVdpState after460 = vdp.GetState();
		EXPECT_EQ(after460.Registers[19], 0x02);
		EXPECT_TRUE(after460.DmaActive);

		vdp.Run(461);
		GenesisVdpState before462 = vdp.GetState();
		EXPECT_EQ(before462.Registers[19], 0x02);
		EXPECT_TRUE(before462.DmaActive);

		vdp.Run(462);
		GenesisVdpState after462 = vdp.GetState();
		EXPECT_EQ(after462.Registers[19], 0x01);
		EXPECT_TRUE(after462.DmaActive);

		vdp.Run(467);
		GenesisVdpState before468 = vdp.GetState();
		EXPECT_EQ(before468.Registers[19], 0x01);
		EXPECT_TRUE(before468.DmaActive);

		vdp.Run(468);
		GenesisVdpState after468 = vdp.GetState();
		EXPECT_EQ(after468.Registers[19], 0x00);
		EXPECT_FALSE(after468.DmaActive);
		EXPECT_EQ(after468.StatusRegister & VdpStatus::DmaBusy, 0);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, H40MidLineExternalSlotsRemainSlotGated) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, true, 0x09);

		vdp.Run(81);
		GenesisVdpState before82 = vdp.GetState();
		EXPECT_EQ(before82.Registers[19], 0x05);
		EXPECT_TRUE(before82.DmaActive);

		vdp.Run(82);
		GenesisVdpState after82 = vdp.GetState();
		EXPECT_EQ(after82.Registers[19], 0x04);
		EXPECT_TRUE(after82.DmaActive);

		vdp.Run(84);
		GenesisVdpState before85 = vdp.GetState();
		EXPECT_EQ(before85.Registers[19], 0x04);
		EXPECT_TRUE(before85.DmaActive);

		vdp.Run(85);
		GenesisVdpState after85 = vdp.GetState();
		EXPECT_EQ(after85.Registers[19], 0x03);
		EXPECT_TRUE(after85.DmaActive);

		vdp.Run(87);
		GenesisVdpState before88 = vdp.GetState();
		EXPECT_EQ(before88.Registers[19], 0x03);
		EXPECT_TRUE(before88.DmaActive);

		vdp.Run(88);
		GenesisVdpState after88 = vdp.GetState();
		EXPECT_EQ(after88.Registers[19], 0x02);
		EXPECT_TRUE(after88.DmaActive);

		vdp.Run(89);
		GenesisVdpState before90 = vdp.GetState();
		EXPECT_EQ(before90.Registers[19], 0x02);
		EXPECT_TRUE(before90.DmaActive);

		vdp.Run(90);
		GenesisVdpState after90 = vdp.GetState();
		EXPECT_EQ(after90.Registers[19], 0x01);
		EXPECT_TRUE(after90.DmaActive);

		vdp.Run(92);
		GenesisVdpState before93 = vdp.GetState();
		EXPECT_EQ(before93.Registers[19], 0x01);
		EXPECT_TRUE(before93.DmaActive);

		vdp.Run(93);
		GenesisVdpState after93 = vdp.GetState();
		EXPECT_EQ(after93.Registers[19], 0x00);
		EXPECT_FALSE(after93.DmaActive);
	}

	TEST(GenesisVdpDmaStartupLatencyTests, H32MidLineExternalSlotsRemainSlotGated) {
		vector<uint8_t> rom = BuildDmaSourceRom();

		Emulator emu;
		emu.Initialize(false);
		GenesisMemoryManager mm;
		mm.Init(&emu, nullptr, rom, nullptr, nullptr, nullptr);

		GenesisVdp vdp;
		vdp.Init(&emu, nullptr, nullptr, &mm);
		ConfigureBusDmaTransfer(vdp, false, 0x09);

		vdp.Run(73);
		GenesisVdpState before74 = vdp.GetState();
		EXPECT_EQ(before74.Registers[19], 0x05);
		EXPECT_TRUE(before74.DmaActive);

		vdp.Run(74);
		GenesisVdpState after74 = vdp.GetState();
		EXPECT_EQ(after74.Registers[19], 0x04);
		EXPECT_TRUE(after74.DmaActive);

		vdp.Run(76);
		GenesisVdpState before77 = vdp.GetState();
		EXPECT_EQ(before77.Registers[19], 0x04);
		EXPECT_TRUE(before77.DmaActive);

		vdp.Run(77);
		GenesisVdpState after77 = vdp.GetState();
		EXPECT_EQ(after77.Registers[19], 0x03);
		EXPECT_TRUE(after77.DmaActive);

		vdp.Run(79);
		GenesisVdpState before80 = vdp.GetState();
		EXPECT_EQ(before80.Registers[19], 0x03);
		EXPECT_TRUE(before80.DmaActive);

		vdp.Run(80);
		GenesisVdpState after80 = vdp.GetState();
		EXPECT_EQ(after80.Registers[19], 0x02);
		EXPECT_TRUE(after80.DmaActive);

		vdp.Run(81);
		GenesisVdpState before82 = vdp.GetState();
		EXPECT_EQ(before82.Registers[19], 0x02);
		EXPECT_TRUE(before82.DmaActive);

		vdp.Run(82);
		GenesisVdpState after82 = vdp.GetState();
		EXPECT_EQ(after82.Registers[19], 0x01);
		EXPECT_TRUE(after82.DmaActive);

		vdp.Run(84);
		GenesisVdpState before85 = vdp.GetState();
		EXPECT_EQ(before85.Registers[19], 0x01);
		EXPECT_TRUE(before85.DmaActive);

		vdp.Run(85);
		GenesisVdpState after85 = vdp.GetState();
		EXPECT_EQ(after85.Registers[19], 0x00);
		EXPECT_FALSE(after85.DmaActive);
	}
}
