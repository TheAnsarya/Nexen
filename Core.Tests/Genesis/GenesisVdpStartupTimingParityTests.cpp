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
		return (uint64_t)line * 488ull;
	}

	TEST(GenesisVdpStartupTimingParityTests, VBlankFlagIsDeferredOnFirstVBlankLine) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);

		uint64_t vblankLineStart = LineStartCycle(224);
		vdp.Run(vblankLineStart);
		uint16_t statusAtLineStart = ReadStatus(vdp);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VBlankFlag, 0u);

		vdp.Run(vblankLineStart + 64ull);
		uint16_t statusAfterDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterDelay & VdpStatus::VBlankFlag, 0u);
	}

	TEST(GenesisVdpStartupTimingParityTests, VIntPendingAppearsAtDeferredVBlankPoint) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x64); // display + VINT enable

		uint64_t vblankLineStart = LineStartCycle(224);
		vdp.Run(vblankLineStart);
		uint16_t statusAtLineStart = ReadStatus(vdp);
		EXPECT_EQ(statusAtLineStart & VdpStatus::VIntPending, 0u);

		vdp.Run(vblankLineStart + 128ull);
		uint16_t statusAfterDelay = ReadStatus(vdp);
		EXPECT_NE(statusAfterDelay & VdpStatus::VIntPending, 0u);

		uint16_t statusAfterAckRead = ReadStatus(vdp);
		EXPECT_EQ(statusAfterAckRead & VdpStatus::VIntPending, 0u);
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
