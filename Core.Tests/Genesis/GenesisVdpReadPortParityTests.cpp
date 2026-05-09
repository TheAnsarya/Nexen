#include "pch.h"
#include "Genesis/GenesisVdp.h"

namespace {
	static void WriteReg(GenesisVdp& vdp, uint8_t reg, uint8_t value) {
		vdp.WriteControlPort((uint16_t)(0x8000u | ((uint16_t)reg << 8) | value));
	}

	static void SetDataPortCommand(GenesisVdp& vdp, uint8_t modeLow, uint16_t address) {
		uint16_t first = (uint16_t)(((uint16_t)(modeLow & 0x03u) << 14) | (address & 0x3fffu));
		uint16_t second = (uint16_t)((address >> 14) & 0x0003u);
		vdp.WriteControlPort(first);
		vdp.WriteControlPort(second);
	}

	static void SetDataPortWriteVram(GenesisVdp& vdp, uint16_t address) {
		// CD3..0 = 0001 (VRAM write)
		uint16_t first = (uint16_t)(0x4000u | (address & 0x3fffu));
		uint16_t second = (uint16_t)(0x0000u | ((address >> 14) & 0x0003u));
		vdp.WriteControlPort(first);
		vdp.WriteControlPort(second);
	}

	static void SetDataPortReadVram(GenesisVdp& vdp, uint16_t address) {
		SetDataPortCommand(vdp, 0x00, address);
	}

	static void SetDataPortReadVsram(GenesisVdp& vdp, uint16_t address) {
		SetDataPortCommand(vdp, 0x00, address);
		// Force read mode select bits to VSRAM read by writing high CD bits in second word.
		uint16_t first = (uint16_t)(0x0000u | (address & 0x3fffu));
		uint16_t second = (uint16_t)(0x0010u | ((address >> 14) & 0x0003u)); // sets access mode 0x04
		vdp.WriteControlPort(first);
		vdp.WriteControlPort(second);
	}

	static void SetDataPortReadCram(GenesisVdp& vdp, uint16_t address) {
		SetDataPortCommand(vdp, 0x00, address);
		uint16_t first = (uint16_t)(0x0000u | (address & 0x3fffu));
		uint16_t second = (uint16_t)(0x0020u | ((address >> 14) & 0x0003u)); // sets access mode 0x08
		vdp.WriteControlPort(first);
		vdp.WriteControlPort(second);
	}

	TEST(GenesisVdpReadPortParityTests, VramReadUsesReadAheadBufferSemantics) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		WriteReg(vdp, 15, 0x02);

		SetDataPortWriteVram(vdp, 0x0000);
		vdp.WriteDataPort(0x1234);
		vdp.WriteDataPort(0xabcd);

		SetDataPortReadVram(vdp, 0x0000);
		uint16_t first = vdp.ReadDataPort();
		uint16_t second = vdp.ReadDataPort();
		uint16_t third = vdp.ReadDataPort();

		// Command finalization primes the first word into the buffer.
		EXPECT_EQ(first, 0x1234u);
		EXPECT_EQ(second, 0xabcdu);
		EXPECT_EQ(third, 0x0000u);
	}

	TEST(GenesisVdpReadPortParityTests, VramReadAdvancesByConfiguredAutoincrement) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);
		WriteReg(vdp, 15, 0x04);

		SetDataPortWriteVram(vdp, 0x0000);
		vdp.WriteDataPort(0x1001);
		vdp.WriteDataPort(0x2002);
		vdp.WriteDataPort(0x3003);

		SetDataPortReadVram(vdp, 0x0000);
		uint16_t w0 = vdp.ReadDataPort();
		uint16_t w1 = vdp.ReadDataPort();
		uint16_t w2 = vdp.ReadDataPort();

		EXPECT_EQ(w0, 0x1001u);
		EXPECT_EQ(w1, 0x2002u);
		EXPECT_EQ(w2, 0x3003u);
	}

	TEST(GenesisVdpReadPortParityTests, CramReadUsesReadAheadBufferAfterModeSwitch) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 15, 0x02);
		vdp.GetCramPointer()[0] = 0x00ee;
		vdp.GetCramPointer()[1] = 0x0444;

		SetDataPortReadCram(vdp, 0x0000);
		uint16_t first = vdp.ReadDataPort();
		uint16_t second = vdp.ReadDataPort();
		uint16_t third = vdp.ReadDataPort();

		EXPECT_EQ(first, 0x00eeu);
		EXPECT_EQ(second, 0x0444u);
		EXPECT_EQ(third, 0x0000u);
	}

	TEST(GenesisVdpReadPortParityTests, VsramReadUsesReadAheadBufferAfterModeSwitch) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 15, 0x02);
		GenesisVdpState state = vdp.GetState();
		state.Vsram[0] = 0x0011;
		state.Vsram[1] = 0x0222;

		// Seed VSRAM through writes so internal storage is authoritative.
		vdp.WriteControlPort(0x4000);
		vdp.WriteControlPort(0x0014); // VSRAM write mode
		vdp.WriteDataPort(0x0011);
		vdp.WriteDataPort(0x0222);

		SetDataPortReadVsram(vdp, 0x0000);
		uint16_t first = vdp.ReadDataPort();
		uint16_t second = vdp.ReadDataPort();
		uint16_t third = vdp.ReadDataPort();

		EXPECT_EQ(first & 0x07ffu, 0x0011u);
		EXPECT_EQ(second & 0x07ffu, 0x0222u);
		EXPECT_EQ(third & 0x07ffu, 0x0000u);
	}

	TEST(GenesisVdpReadPortParityTests, ControlPortProgrammingUpdatesCommandStateForDebugTools) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x4012);
		vdp.WriteControlPort(0x0001);
		GenesisVdpState state = vdp.GetState();
		EXPECT_NE(state.AddressRegister, 0u);
		EXPECT_NE(state.CodeRegister, 0u);

		vdp.WriteControlPort(0x1234);
		vdp.WriteControlPort(0x0020);
		state = vdp.GetState();
		EXPECT_NE(state.CodeRegister & 0x0fu, 0u);
	}

	TEST(GenesisVdpReadPortParityTests, StatusReadClearsPendingControlWritePairState) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		vdp.WriteControlPort(0x1234);
		GenesisVdpState pending = vdp.GetState();
		EXPECT_TRUE(pending.WritePending);

		(void)vdp.ReadControlPort();
		GenesisVdpState afterRead = vdp.GetState();
		EXPECT_FALSE(afterRead.WritePending);
	}

	TEST(GenesisVdpReadPortParityTests, FifoStatusBitsTrackQueuedWritesAcrossReadCycles) {
		GenesisVdp vdp;
		vdp.Init(nullptr, nullptr, nullptr, nullptr);

		WriteReg(vdp, 1, 0x44);
		SetDataPortWriteVram(vdp, 0x2000);

		vdp.WriteDataPort(0xaaaa);
		vdp.WriteDataPort(0xbbbb);
		vdp.WriteDataPort(0xcccc);
		vdp.WriteDataPort(0xdddd);

		uint16_t fullStatus = vdp.ReadControlPort();
		EXPECT_NE(fullStatus & VdpStatus::FifoFull, 0u);
		EXPECT_EQ(fullStatus & VdpStatus::FifoEmpty, 0u);

		vdp.Run(400);
		uint16_t midStatus = vdp.ReadControlPort();
		EXPECT_EQ(midStatus & VdpStatus::FifoFull, 0u);

		vdp.Run(488);
		uint16_t emptyStatus = vdp.ReadControlPort();
		EXPECT_NE(emptyStatus & VdpStatus::FifoEmpty, 0u);
	}
}
