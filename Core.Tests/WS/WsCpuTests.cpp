#include "pch.h"
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan V30MZ CPU Tests
// =============================================================================
// Tests for CPU state, flags, addressing, register operations, and
// interrupt/segment behavior of the V30MZ (NEC 8086-clone).

namespace WsCpuTestHelpers {
	// V30MZ physical address calculation: segment:offset → 20-bit linear
	static uint32_t ToPhysical(uint16_t segment, uint16_t offset) {
		return ((uint32_t)segment << 4) + offset;
	}

	// Compute 16-bit flags from individual bits (mirrors WsCpuFlags::Get)
	static uint16_t PackFlags(bool carry, bool parity, bool auxCarry, bool zero,
		bool sign, bool trap, bool irq, bool direction, bool overflow, bool mode) {
		return (
			(uint8_t)carry |
			((uint8_t)parity << 2) |
			((uint8_t)auxCarry << 4) |
			((uint8_t)zero << 6) |
			((uint8_t)sign << 7) |
			((uint8_t)trap << 8) |
			((uint8_t)irq << 9) |
			((uint8_t)direction << 10) |
			((uint8_t)overflow << 11) |
			((uint8_t)mode << 15) |
			0x7002);
	}

	// 8-bit parity: true if even number of set bits
	static bool ComputeParity(uint8_t val) {
		int count = 0;
		for (int j = 0; j < 8; j++) {
			count += (val & (1 << j)) ? 1 : 0;
		}
		return (count & 1) == 0;
	}

	// 8-bit sign flag extraction
	static bool IsNegative8(uint8_t val) { return (val & 0x80) != 0; }
	static bool IsNegative16(uint16_t val) { return (val & 0x8000) != 0; }

	// Simulated 8-bit ADD with flag computation
	struct ArithResult8 {
		uint8_t Result;
		bool Carry;
		bool Zero;
		bool Sign;
		bool Overflow;
		bool AuxCarry;
		bool Parity;
	};

	static ArithResult8 Add8(uint8_t a, uint8_t b) {
		uint16_t sum = (uint16_t)a + b;
		uint8_t result = (uint8_t)sum;
		return {
			result,
			sum > 0xff,
			result == 0,
			IsNegative8(result),
			((a ^ result) & (b ^ result) & 0x80) != 0,
			((a ^ b ^ result) & 0x10) != 0,
			ComputeParity(result)
		};
	}

	// Simulated 8-bit SUB with flag computation
	static ArithResult8 Sub8(uint8_t a, uint8_t b) {
		uint16_t diff = (uint16_t)a - b;
		uint8_t result = (uint8_t)diff;
		return {
			result,
			a < b,
			result == 0,
			IsNegative8(result),
			((a ^ b) & (a ^ result) & 0x80) != 0,
			((a ^ b ^ result) & 0x10) != 0,
			ComputeParity(result)
		};
	}

	// Simulated 16-bit ADD with flag computation
	struct ArithResult16 {
		uint16_t Result;
		bool Carry;
		bool Zero;
		bool Sign;
		bool Overflow;
		bool AuxCarry;
		bool Parity;
	};

	static ArithResult16 Add16(uint16_t a, uint16_t b) {
		uint32_t sum = (uint32_t)a + b;
		uint16_t result = (uint16_t)sum;
		return {
			result,
			sum > 0xffff,
			result == 0,
			IsNegative16(result),
			((a ^ result) & (b ^ result) & 0x8000) != 0,
			((a ^ b ^ result) & 0x10) != 0,
			ComputeParity((uint8_t)result)
		};
	}
}

// =============================================================================
// CPU State Initialization Tests
// =============================================================================

TEST(WsCpuStateTest, DefaultState_ZeroInitialized) {
	WsCpuState state = {};
	EXPECT_EQ(state.CycleCount, 0u);
	EXPECT_EQ(state.CS, 0);
	EXPECT_EQ(state.IP, 0);
	EXPECT_EQ(state.SS, 0);
	EXPECT_EQ(state.SP, 0);
	EXPECT_EQ(state.BP, 0);
	EXPECT_EQ(state.DS, 0);
	EXPECT_EQ(state.ES, 0);
	EXPECT_EQ(state.SI, 0);
	EXPECT_EQ(state.DI, 0);
	EXPECT_EQ(state.AX, 0);
	EXPECT_EQ(state.BX, 0);
	EXPECT_EQ(state.CX, 0);
	EXPECT_EQ(state.DX, 0);
	EXPECT_FALSE(state.Halted);
	EXPECT_FALSE(state.PowerOff);
}

TEST(WsCpuStateTest, RegisterAssignment_PreservesValues) {
	WsCpuState state = {};
	state.AX = 0x1234;
	state.BX = 0x5678;
	state.CX = 0x9abc;
	state.DX = 0xdef0;
	state.SP = 0x2000;
	state.BP = 0x3000;
	state.SI = 0x4000;
	state.DI = 0x5000;

	EXPECT_EQ(state.AX, 0x1234);
	EXPECT_EQ(state.BX, 0x5678);
	EXPECT_EQ(state.CX, 0x9abc);
	EXPECT_EQ(state.DX, 0xdef0);
	EXPECT_EQ(state.SP, 0x2000);
	EXPECT_EQ(state.BP, 0x3000);
	EXPECT_EQ(state.SI, 0x4000);
	EXPECT_EQ(state.DI, 0x5000);
}

TEST(WsCpuStateTest, SegmentRegisters_FullRange) {
	WsCpuState state = {};
	state.CS = 0xffff;
	state.DS = 0xf000;
	state.ES = 0x0000;
	state.SS = 0x1000;

	EXPECT_EQ(state.CS, 0xffff);
	EXPECT_EQ(state.DS, 0xf000);
	EXPECT_EQ(state.ES, 0x0000);
	EXPECT_EQ(state.SS, 0x1000);
}

// =============================================================================
// CPU Flags Tests (WsCpuFlags Get/Set)
// =============================================================================

TEST(WsCpuFlagsTest, AllFlagsClear_ReturnsBaseValue) {
	WsCpuFlags flags = {};
	flags.Carry = false;
	flags.Parity = false;
	flags.AuxCarry = false;
	flags.Zero = false;
	flags.Sign = false;
	flags.Trap = false;
	flags.Irq = false;
	flags.Direction = false;
	flags.Overflow = false;
	flags.Mode = false;

	// Base bits 0x7002 always set
	EXPECT_EQ(flags.Get(), 0x7002);
}

TEST(WsCpuFlagsTest, AllFlagsSet_CorrectBitPositions) {
	WsCpuFlags flags = {};
	flags.Carry = true;
	flags.Parity = true;
	flags.AuxCarry = true;
	flags.Zero = true;
	flags.Sign = true;
	flags.Trap = true;
	flags.Irq = true;
	flags.Direction = true;
	flags.Overflow = true;
	flags.Mode = true;

	uint16_t packed = flags.Get();
	// 0x7002 | 0x01(C) | 0x04(P) | 0x10(A) | 0x40(Z) | 0x80(S) |
	// 0x100(T) | 0x200(I) | 0x400(D) | 0x800(O) | 0x8000(M)
	uint16_t expected = 0x7002 | 0x01 | 0x04 | 0x10 | 0x40 | 0x80 |
		0x100 | 0x200 | 0x400 | 0x800 | 0x8000;
	EXPECT_EQ(packed, expected);
}

TEST(WsCpuFlagsTest, IndividualFlag_Carry) {
	WsCpuFlags flags = {};
	flags.Carry = true;
	uint16_t packed = flags.Get();
	EXPECT_TRUE(packed & 0x01);

	WsCpuFlags rt = {};
	rt.Set(packed);
	EXPECT_TRUE(rt.Carry);
	EXPECT_FALSE(rt.Parity);
}

TEST(WsCpuFlagsTest, IndividualFlag_Overflow) {
	WsCpuFlags flags = {};
	flags.Overflow = true;
	uint16_t packed = flags.Get();
	EXPECT_TRUE(packed & 0x800);

	WsCpuFlags rt = {};
	rt.Set(packed);
	EXPECT_TRUE(rt.Overflow);
	EXPECT_FALSE(rt.Carry);
}

TEST(WsCpuFlagsTest, IndividualFlag_Mode) {
	WsCpuFlags flags = {};
	flags.Mode = true;
	uint16_t packed = flags.Get();
	EXPECT_TRUE(packed & 0x8000);

	WsCpuFlags rt = {};
	rt.Set(packed);
	EXPECT_TRUE(rt.Mode);
	EXPECT_FALSE(rt.Carry);
}

TEST(WsCpuFlagsTest, SetFromPacked_ClearsAbsentFlags) {
	WsCpuFlags flags = {};
	flags.Set(0x7002); // Base with no flags
	EXPECT_FALSE(flags.Carry);
	EXPECT_FALSE(flags.Parity);
	EXPECT_FALSE(flags.AuxCarry);
	EXPECT_FALSE(flags.Zero);
	EXPECT_FALSE(flags.Sign);
	EXPECT_FALSE(flags.Trap);
	EXPECT_FALSE(flags.Irq);
	EXPECT_FALSE(flags.Direction);
	EXPECT_FALSE(flags.Overflow);
	EXPECT_FALSE(flags.Mode);
}

TEST(WsCpuFlagsTest, GetSetRoundTrip_AllCombinations) {
	// Test all 1024 combinations of 10 flags
	for (int i = 0; i < 1024; i++) {
		WsCpuFlags flags = {};
		flags.Carry = (i & 1) != 0;
		flags.Parity = (i & 2) != 0;
		flags.AuxCarry = (i & 4) != 0;
		flags.Zero = (i & 8) != 0;
		flags.Sign = (i & 16) != 0;
		flags.Trap = (i & 32) != 0;
		flags.Irq = (i & 64) != 0;
		flags.Direction = (i & 128) != 0;
		flags.Overflow = (i & 256) != 0;
		flags.Mode = (i & 512) != 0;

		uint16_t packed = flags.Get();

		WsCpuFlags rt = {};
		rt.Set(packed);

		EXPECT_EQ(rt.Carry, flags.Carry) << "i=" << i;
		EXPECT_EQ(rt.Parity, flags.Parity) << "i=" << i;
		EXPECT_EQ(rt.AuxCarry, flags.AuxCarry) << "i=" << i;
		EXPECT_EQ(rt.Zero, flags.Zero) << "i=" << i;
		EXPECT_EQ(rt.Sign, flags.Sign) << "i=" << i;
		EXPECT_EQ(rt.Trap, flags.Trap) << "i=" << i;
		EXPECT_EQ(rt.Irq, flags.Irq) << "i=" << i;
		EXPECT_EQ(rt.Direction, flags.Direction) << "i=" << i;
		EXPECT_EQ(rt.Overflow, flags.Overflow) << "i=" << i;
		EXPECT_EQ(rt.Mode, flags.Mode) << "i=" << i;
	}
}

// =============================================================================
// Physical Address Calculation Tests
// =============================================================================

TEST(WsCpuAddressingTest, SegmentOffset_ZeroBase) {
	EXPECT_EQ(WsCpuTestHelpers::ToPhysical(0x0000, 0x0000), 0x00000u);
}

TEST(WsCpuAddressingTest, SegmentOffset_SimpleCalculation) {
	// 0x1000:0x0100 → 0x10000 + 0x0100 = 0x10100
	EXPECT_EQ(WsCpuTestHelpers::ToPhysical(0x1000, 0x0100), 0x10100u);
}

TEST(WsCpuAddressingTest, SegmentOffset_TypicalReset) {
	// V30MZ reset: CS=0xffff, IP=0x0000 → 0xffff0
	EXPECT_EQ(WsCpuTestHelpers::ToPhysical(0xffff, 0x0000), 0xffff0u);
}

TEST(WsCpuAddressingTest, SegmentOffset_Wrap20Bit) {
	// 0xffff:0xffff → 0xffff0 + 0xffff = 0x10ffef (wraps on real hardware)
	uint32_t addr = WsCpuTestHelpers::ToPhysical(0xffff, 0xffff);
	EXPECT_EQ(addr, 0x10ffefu);
}

TEST(WsCpuAddressingTest, SegmentOffset_StackExample) {
	// SS:SP stack access: SS=0x0000, SP=0x2000 → 0x02000
	EXPECT_EQ(WsCpuTestHelpers::ToPhysical(0x0000, 0x2000), 0x02000u);
}

TEST(WsCpuAddressingTest, SegmentOffset_BootRomArea) {
	// WS boot ROM at top of address space: 0xfe00:0x0000 → 0xfe000
	EXPECT_EQ(WsCpuTestHelpers::ToPhysical(0xfe00, 0x0000), 0xfe000u);
}

// =============================================================================
// Arithmetic Flag Computation Tests
// =============================================================================

TEST(WsCpuArithTest, Add8_ZeroResult) {
	auto r = WsCpuTestHelpers::Add8(0x00, 0x00);
	EXPECT_EQ(r.Result, 0);
	EXPECT_TRUE(r.Zero);
	EXPECT_FALSE(r.Carry);
	EXPECT_FALSE(r.Sign);
	EXPECT_TRUE(r.Parity); // 0 has even parity
}

TEST(WsCpuArithTest, Add8_CarryOut) {
	auto r = WsCpuTestHelpers::Add8(0xff, 0x01);
	EXPECT_EQ(r.Result, 0x00);
	EXPECT_TRUE(r.Carry);
	EXPECT_TRUE(r.Zero);
}

TEST(WsCpuArithTest, Add8_SignedOverflow) {
	// 0x7f + 0x01 = 0x80 → positive + positive = negative (overflow)
	auto r = WsCpuTestHelpers::Add8(0x7f, 0x01);
	EXPECT_EQ(r.Result, 0x80);
	EXPECT_TRUE(r.Overflow);
	EXPECT_TRUE(r.Sign);
	EXPECT_FALSE(r.Zero);
	EXPECT_FALSE(r.Carry);
}

TEST(WsCpuArithTest, Add8_AuxCarry) {
	// 0x0f + 0x01 → half-carry from bit 3 to bit 4
	auto r = WsCpuTestHelpers::Add8(0x0f, 0x01);
	EXPECT_TRUE(r.AuxCarry);
	EXPECT_EQ(r.Result, 0x10);
}

TEST(WsCpuArithTest, Sub8_ZeroResult) {
	auto r = WsCpuTestHelpers::Sub8(0x42, 0x42);
	EXPECT_EQ(r.Result, 0);
	EXPECT_TRUE(r.Zero);
	EXPECT_FALSE(r.Carry); // no borrow
}

TEST(WsCpuArithTest, Sub8_Borrow) {
	auto r = WsCpuTestHelpers::Sub8(0x00, 0x01);
	EXPECT_EQ(r.Result, 0xff);
	EXPECT_TRUE(r.Carry); // borrow
	EXPECT_TRUE(r.Sign);
}

TEST(WsCpuArithTest, Sub8_SignedOverflow) {
	// 0x80 - 0x01 = 0x7f → negative - positive = positive (overflow)
	auto r = WsCpuTestHelpers::Sub8(0x80, 0x01);
	EXPECT_EQ(r.Result, 0x7f);
	EXPECT_TRUE(r.Overflow);
	EXPECT_FALSE(r.Sign);
}

TEST(WsCpuArithTest, Add16_CarryOut) {
	auto r = WsCpuTestHelpers::Add16(0xffff, 0x0001);
	EXPECT_EQ(r.Result, 0x0000);
	EXPECT_TRUE(r.Carry);
	EXPECT_TRUE(r.Zero);
}

TEST(WsCpuArithTest, Add16_NoCarry) {
	auto r = WsCpuTestHelpers::Add16(0x1234, 0x5678);
	EXPECT_EQ(r.Result, 0x68ac);
	EXPECT_FALSE(r.Carry);
}

// =============================================================================
// Parity Tests
// =============================================================================

TEST(WsCpuParityTest, Table_AllBytesChecked) {
	WsCpuParityTable table;
	for (int i = 0; i < 256; i++) {
		bool expected = WsCpuTestHelpers::ComputeParity((uint8_t)i);
		EXPECT_EQ(table.CheckParity((uint8_t)i), expected) << "byte=" << i;
	}
}

TEST(WsCpuParityTest, SpecificValues) {
	EXPECT_TRUE(WsCpuTestHelpers::ComputeParity(0x00)); // 0 bits
	EXPECT_FALSE(WsCpuTestHelpers::ComputeParity(0x01)); // 1 bit
	EXPECT_TRUE(WsCpuTestHelpers::ComputeParity(0x03)); // 2 bits
	EXPECT_TRUE(WsCpuTestHelpers::ComputeParity(0x0f)); // 4 bits → even
	EXPECT_TRUE(WsCpuTestHelpers::ComputeParity(0xff)); // 8 bits → even
}

// =============================================================================
// Constants Sanity Tests
// =============================================================================

TEST(WsCpuConstantsTest, ScreenDimensions) {
	EXPECT_EQ(WsConstants::ScreenWidth, 224u);
	EXPECT_EQ(WsConstants::ScreenHeight, 144u);
	EXPECT_EQ(WsConstants::ClocksPerScanline, 256u);
	EXPECT_EQ(WsConstants::ScanlineCount, 159u);
}

TEST(WsCpuConstantsTest, PixelCountDerivedValues) {
	EXPECT_EQ(WsConstants::PixelCount, 224u * 144u);
	EXPECT_EQ(WsConstants::MaxPixelCount, 224u * (144u + 13u));
}

// =============================================================================
// Halted / PowerOff State Tests
// =============================================================================

TEST(WsCpuHaltTest, Halted_SetsCorrectly) {
	WsCpuState state = {};
	state.Halted = true;
	EXPECT_TRUE(state.Halted);
	EXPECT_FALSE(state.PowerOff);
}

TEST(WsCpuHaltTest, PowerOff_SetsCorrectly) {
	WsCpuState state = {};
	state.PowerOff = true;
	EXPECT_TRUE(state.PowerOff);
	EXPECT_FALSE(state.Halted);
}

// =============================================================================
// Segment Override Enum Tests
// =============================================================================

TEST(WsCpuSegmentTest, EnumValues_MatchExpected) {
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::Default), 0u);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::ES), 1u);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::SS), 2u);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::CS), 3u);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::DS), 4u);
}
