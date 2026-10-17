#include "pch.h"
#include "Genesis/GenesisVdp.h"

namespace {
	static void WriteReg(GenesisVdp& vdp, uint8_t reg, uint8_t value) {
		vdp.WriteControlPort((uint16_t)(0x8000u | ((uint16_t)reg << 8) | value));
	}

	static uint16_t ReadStatus(GenesisVdp& vdp) {
		return vdp.ReadControlPort();
	}

	static uint64_t LineStartCycle(uint32_t line) {
		uint64_t cycle = 0;
		uint8_t remainder = 0;
		uint16_t lineCycles = 488;
		for (uint32_t i = 0; i < line; i++) {
			cycle += lineCycles;
			remainder = (uint8_t)(remainder + 4u);
			lineCycles = 488;
			if (remainder >= 7u) {
				remainder = (uint8_t)(remainder - 7u);
				lineCycles = 489;
			}
		}
		return cycle;
	}

	TEST(GenesisVdpStartupTimingParityTests, VBlankFlagIsDeferredOnFirstVBlankLine) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);

		uint64_t vblankLineStart = LineStartCycle(224);
		vdp.Run(vblankLineStart);
		GenesisVdpState stateAtLineStart = vdp.GetState();
		EXPECT_EQ(stateAtLineStart.VCounter, 224u);
		EXPECT_EQ(stateAtLineStart.HCounter, 0u);
		uint16_t statusAtLineStart = ReadStatus(vdp);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VBlankFlag, 0u);

		vdp.Run(vblankLineStart + 64ull);
		GenesisVdpState stateAfterDelay = vdp.GetState();
		EXPECT_EQ(stateAfterDelay.VCounter, 224u);
		uint16_t statusAfterDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterDelay & VdpStatus::VBlankFlag, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, VIntPendingAppearsAtDeferredVBlankPoint) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x64); // display + VINT enable

		uint64_t vblankLineStart = LineStartCycle(224);
		vdp.Run(vblankLineStart);
		GenesisVdpState stateAtLineStart = vdp.GetState();
		EXPECT_EQ(stateAtLineStart.VCounter, 224u);
		EXPECT_EQ(stateAtLineStart.HCounter, 0u);
		uint16_t statusAtLineStart = ReadStatus(vdp);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VIntPending, 0u);

		vdp.Run(vblankLineStart + 128ull);
		GenesisVdpState stateAfterDelay = vdp.GetState();
		EXPECT_EQ(stateAfterDelay.VCounter, 224u);
		uint16_t statusAfterDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterDelay & VdpStatus::VIntPending, 0u);

		uint16_t statusAfterAckRead = ReadStatus(vdp);
		EXPECT_EQ(statusAfterAckRead & VdpStatus::VIntPending, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, PalModeFirstVBlankLineUsesDeferredVBlankAndVIntSetPoints) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		vdp.SetRegion(true);

		WriteReg(vdp, 1, 0x6C); // display + VINT enable + V30 (240-line active)

		uint64_t vblankLineStartPal = LineStartCycle(240);
		vdp.Run(vblankLineStartPal);
		GenesisVdpState stateAtPalLineStart = vdp.GetState();
		EXPECT_EQ(stateAtPalLineStart.VCounter, 240u);
		EXPECT_EQ(stateAtPalLineStart.HCounter, 0u);
		uint16_t statusAtLineStart = ReadStatus(vdp);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VBlankFlag, 0u);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VIntPending, 0u);

		vdp.Run(vblankLineStartPal + 64ull);
		uint16_t statusAfterVblankDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterVblankDelay & VdpStatus::VBlankFlag, 0u);

		vdp.Run(vblankLineStartPal + 128ull);
		uint16_t statusAfterVintDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterVintDelay & VdpStatus::VIntPending, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, HBlankFlagTransitionsWithinVisibleLine) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);

		vdp.Run(300ull);
		uint16_t activeStatus = ReadStatus(vdp);
		EXPECT_EQ(activeStatus & VdpStatus::HBlanking, 0u);

		vdp.Run(430ull);
		uint16_t hblankStatus = ReadStatus(vdp);
		EXPECT_NE(hblankStatus & VdpStatus::HBlanking, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, HBlankFlagClearsAtNextLineStart) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);

		vdp.Run(430ull);
		uint16_t hblankStatus = ReadStatus(vdp);
		EXPECT_NE(hblankStatus & VdpStatus::HBlanking, 0u);

		vdp.Run(488ull);
		uint16_t nextLineStatus = ReadStatus(vdp);
		EXPECT_EQ(nextLineStatus & VdpStatus::HBlanking, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, HIntCounterTicksAtHBlankBoundariesWhenInterruptMasked) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);
		WriteReg(vdp, 0, 0x00); // HINT disabled
		WriteReg(vdp, 10, 0x02);

		vdp.Run(430ull);
		GenesisVdpState first = vdp.GetState();
		EXPECT_EQ(first.HIntCounter, 1u);

		vdp.Run(918ull);
		GenesisVdpState second = vdp.GetState();
		EXPECT_EQ(second.HIntCounter, 0u);

		vdp.Run(1406ull);
		GenesisVdpState third = vdp.GetState();
		EXPECT_EQ(third.HIntCounter, 2u);
	}

	TEST(GenesisVdpStartupTimingParityTests, HIntCounterReloadsAfterZeroAtHBlankBoundary) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);
		WriteReg(vdp, 10, 0x01);

		vdp.Run(430ull);
		GenesisVdpState first = vdp.GetState();
		EXPECT_EQ(first.HIntCounter, 0u);

		vdp.Run(918ull);
		GenesisVdpState second = vdp.GetState();
		EXPECT_EQ(second.HIntCounter, 1u);

		vdp.Run(1406ull);
		GenesisVdpState third = vdp.GetState();
		EXPECT_EQ(third.HIntCounter, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, VIntPendingStaysLatchedWhenAcknowledgeOccursWhileMasked) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44); // VINT disabled
		vdp.Run(LineStartCycle(224) + 128ull);

		GenesisVdpState beforeAck = vdp.GetState();
		EXPECT_NE(beforeAck.StatusRegister & VdpStatus::VIntPending, 0u);

		vdp.AcknowledgeInterrupt(6);
		GenesisVdpState afterMaskedAck = vdp.GetState();
		EXPECT_NE(afterMaskedAck.StatusRegister & VdpStatus::VIntPending, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, VIntPendingCanBeConsumedAfterEnableEdgeAndAcknowledge) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44); // VINT disabled
		vdp.Run(LineStartCycle(224) + 128ull);
		GenesisVdpState latchedWhileMasked = vdp.GetState();
		EXPECT_NE(latchedWhileMasked.StatusRegister & VdpStatus::VIntPending, 0u);

		WriteReg(vdp, 1, 0x64); // enable VINT after pending is already set
		vdp.AcknowledgeInterrupt(6);

		GenesisVdpState afterAckEnabled = vdp.GetState();
		EXPECT_EQ(afterAckEnabled.StatusRegister & VdpStatus::VIntPending, 0u);
	}

}
