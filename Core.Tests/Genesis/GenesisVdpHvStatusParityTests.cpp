#include "pch.h"
#include "Genesis/GenesisVdp.h"

namespace {
	static void WriteReg(GenesisVdp& vdp, uint8_t reg, uint8_t value) {
		vdp.WriteControlPort((uint16_t)(0x8000u | ((uint16_t)reg << 8) | value));
	}

	static uint8_t ReadHCounter(GenesisVdp& vdp) {
		return (uint8_t)(vdp.ReadHVCounter() & 0x00ffu);
	}

	static uint8_t ReadVCounter(GenesisVdp& vdp) {
		return (uint8_t)(vdp.ReadHVCounter() >> 8);
	}

	TEST(GenesisVdpHvStatusParityTests, HCounterSequenceShowsH40WrapDiscontinuityNearHsync) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 12, 0x81); // H40
		WriteReg(vdp, 1, 0x44);  // display on

		std::vector<uint8_t> captured = {};
		captured.reserve(64);

		for (uint32_t cycle = 0; cycle < 488; cycle++) {
			captured.push_back(ReadHCounter(vdp));
			vdp.Run(cycle + 1u);
		}

		bool foundHsyncJump = false;
		for (size_t i = 1; i < captured.size(); i++) {
			uint8_t prev = captured[i - 1];
			uint8_t cur = captured[i];
			if (prev == 0xb6u && cur == 0xe5u) {
				foundHsyncJump = true;
				break;
			}
		}

		EXPECT_TRUE(foundHsyncJump);
	}

	TEST(GenesisVdpHvStatusParityTests, HCounterSequenceShowsH32WrapDiscontinuityNearHsync) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 12, 0x80); // H32
		WriteReg(vdp, 1, 0x44);  // display on

		std::vector<uint8_t> captured = {};
		captured.reserve(489);

		for (uint32_t cycle = 0; cycle < 489; cycle++) {
			captured.push_back(ReadHCounter(vdp));
			vdp.Run(cycle + 1u);
		}

		bool foundHsyncJump = false;
		for (size_t i = 1; i < captured.size(); i++) {
			uint8_t prev = captured[i - 1];
			uint8_t cur = captured[i];
			if (prev == 0x93u && cur == 0xe9u) {
				foundHsyncJump = true;
				break;
			}
		}

		EXPECT_TRUE(foundHsyncJump);
	}

	TEST(GenesisVdpHvStatusParityTests, HVCounterLatchCapturesStableValueWhenR0LatchBitSet) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 12, 0x81);
		WriteReg(vdp, 1, 0x44);

		vdp.Run(120);
		uint16_t beforeLatch = vdp.ReadHVCounter();

		WriteReg(vdp, 0, 0x02);
		uint16_t latchedA = vdp.ReadHVCounter();

		vdp.Run(220);
		uint16_t latchedB = vdp.ReadHVCounter();

		EXPECT_EQ(latchedA, latchedB);
		EXPECT_EQ(latchedA, beforeLatch);

		WriteReg(vdp, 0, 0x00);
		uint16_t liveAfterClear = vdp.ReadHVCounter();
		EXPECT_NE(liveAfterClear, latchedA);
	}

	TEST(GenesisVdpHvStatusParityTests, NtscVCounterUsesNonLinearWrapAfterLine234) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		vdp.SetRegion(false);

		WriteReg(vdp, 1, 0x44);

		uint64_t lineClock = 0;
		for (uint32_t line = 0; line < 236; line++) {
			lineClock += 488;
			vdp.Run(lineClock);
		}

		uint8_t vCounterAfterWrapPoint = ReadVCounter(vdp);
		EXPECT_EQ(vCounterAfterWrapPoint, 0xe5u);
	}

	TEST(GenesisVdpHvStatusParityTests, PalV28VCounterUsesNonLinearWrapAfterLine258) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		vdp.SetRegion(true);

		WriteReg(vdp, 1, 0x44); // V28

		uint64_t lineClock = 0;
		for (uint32_t line = 0; line < 260; line++) {
			lineClock += 488;
			vdp.Run(lineClock);
		}

		uint8_t vCounterAfterWrapPoint = ReadVCounter(vdp);
		EXPECT_EQ(vCounterAfterWrapPoint, 0xcau);
	}

	TEST(GenesisVdpHvStatusParityTests, PalV30VCounterUsesNonLinearWrapAfterLine266) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		vdp.SetRegion(true);

		WriteReg(vdp, 1, 0x4c); // display on + V30

		uint64_t lineClock = 0;
		for (uint32_t line = 0; line < 268; line++) {
			lineClock += 488;
			vdp.Run(lineClock);
		}

		uint8_t vCounterAfterWrapPoint = ReadVCounter(vdp);
		EXPECT_EQ(vCounterAfterWrapPoint, 0xd2u);
	}

	TEST(GenesisVdpHvStatusParityTests, StatusReadClearsVIntAndSpriteStickyFlags) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x64); // display on + VINT enable

		uint64_t lineClock = 0;
		for (uint32_t line = 0; line < 225; line++) {
			lineClock += 488;
			vdp.Run(lineClock);
		}

		uint16_t statusFirst = vdp.ReadControlPort();
		uint16_t statusSecond = vdp.ReadControlPort();

		EXPECT_NE(statusFirst & VdpStatus::VIntPending, 0u);
		EXPECT_EQ(statusSecond & VdpStatus::VIntPending, 0u);
		EXPECT_EQ(statusSecond & VdpStatus::SprOverflow, 0u);
		EXPECT_EQ(statusSecond & VdpStatus::SprCollision, 0u);
	}

	TEST(GenesisVdpHvStatusParityTests, StatusReadReflectsFifoEmptyAndFifoFullTransitions) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);  // display on
		WriteReg(vdp, 12, 0x81); // H40

		// Set VRAM write mode.
		vdp.WriteControlPort(0x4000);
		vdp.WriteControlPort(0x0000);

		// Prime enough writes during active display to fill FIFO.
		vdp.WriteDataPort(0x1111);
		vdp.WriteDataPort(0x2222);
		vdp.WriteDataPort(0x3333);
		vdp.WriteDataPort(0x4444);

		uint16_t statusFull = vdp.ReadControlPort();
		EXPECT_NE(statusFull & VdpStatus::FifoFull, 0u);
		EXPECT_EQ(statusFull & VdpStatus::FifoEmpty, 0u);

		// Drain over one visible scanline worth of slots.
		vdp.Run(488);
		uint16_t statusDrained = vdp.ReadControlPort();
		EXPECT_EQ(statusDrained & VdpStatus::FifoFull, 0u);
		EXPECT_NE(statusDrained & VdpStatus::FifoEmpty, 0u);
	}

	TEST(GenesisVdpHvStatusParityTests, HCounterAtScanlineStartUsesLineOriginSlot) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 12, 0x81);
		WriteReg(vdp, 1, 0x44);

		uint8_t h0 = ReadHCounter(vdp);
		EXPECT_EQ(h0, 165u);

		WriteReg(vdp, 12, 0x80);
		vdp.Reset(false);
		WriteReg(vdp, 12, 0x80);
		WriteReg(vdp, 1, 0x44);

		h0 = ReadHCounter(vdp);
		EXPECT_EQ(h0, 133u);
	}
}
