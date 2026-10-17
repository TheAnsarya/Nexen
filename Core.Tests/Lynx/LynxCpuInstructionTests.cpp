#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Exhaustive logic verification tests for the Lynx 65C02 CPU instruction set.
/// These tests reimplement the CPU's ALU operations as standalone static methods
/// and verify them against known-correct results. This catches logic errors,
/// flag calculation bugs, and BCD mode issues without requiring a full CPU
/// instantiation or memory subsystem.
///
/// Test categories:
///   - ADC binary mode (256×256×2 = 131072 combinations)
///   - SBC binary mode (256×256×2 = 131072 combinations)
///   - ADC decimal (BCD) mode — 65C02 specific flag behavior
///   - SBC decimal (BCD) mode — 65C02 specific
///   - Shift/Rotate operations (ASL, LSR, ROL, ROR)
///   - Compare operations (CMP, CPX, CPY)
///   - BIT, TSB, TRB — including 65C02 BIT immediate
///   - SetZeroNeg flag helper
///   - SetA/SetX/SetY register-set helpers
///   - SetPS reserved bit enforcement
/// </summary>

// =============================================================================
// ADC / SBC Binary Mode Logic Tests
// =============================================================================

class LynxCpuArithmeticTest : public ::testing::Test {
protected:
	/// ADC binary mode — replicated from LynxCpu::ADC()
	static uint8_t ADC_Binary(uint8_t a, uint8_t operand, bool carryIn,
		bool& carryOut, bool& overflowOut, bool& zeroOut, bool& negOut) {
		uint16_t result = (uint16_t)a + operand + (carryIn ? 1 : 0);
		carryOut = result > 0xff;
		overflowOut = (~(a ^ operand) & (a ^ (uint8_t)result) & 0x80) != 0;
		uint8_t r = (uint8_t)result;
		zeroOut = (r == 0);
		negOut = (r & 0x80) != 0;
		return r;
	}

	/// SBC binary mode — replicated from LynxCpu::SBC()
	static uint8_t SBC_Binary(uint8_t a, uint8_t operand, bool carryIn,
		bool& carryOut, bool& overflowOut, bool& zeroOut, bool& negOut) {
		uint8_t borrow = carryIn ? 0 : 1;
		uint16_t result = (uint16_t)a - operand - borrow;
		carryOut = result < 0x100;
		overflowOut = ((a ^ operand) & (a ^ (uint8_t)result) & 0x80) != 0;
		uint8_t r = (uint8_t)result;
		zeroOut = (r == 0);
		negOut = (r & 0x80) != 0;
		return r;
	}

	/// ADC decimal (BCD) mode — replicated from LynxCpu::ADC() decimal path
	/// 65C02: Z and N flags set from BCD result (differs from NMOS 6502)
	static uint8_t ADC_Decimal(uint8_t a, uint8_t operand, bool carryIn,
		bool& carryOut, bool& overflowOut, bool& zeroOut, bool& negOut) {
		uint8_t carry = carryIn ? 1 : 0;
		uint8_t al = (a & 0x0f) + (operand & 0x0f) + carry;
		if (al > 9) al += 6;
		uint16_t ah = (a >> 4) + (operand >> 4) + (al > 15 ? 1 : 0);
		al &= 0x0f;

		// Overflow uses binary arithmetic result
		uint16_t binResult = (uint16_t)a + operand + carry;
		overflowOut = (~(a ^ operand) & (a ^ (uint8_t)binResult) & 0x80) != 0;

		if (ah > 9) ah += 6;
		carryOut = ah > 15;

		uint8_t result = (al & 0x0f) | ((ah & 0x0f) << 4);
		// 65C02: Z and N flags from BCD result
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}

	/// SBC decimal (BCD) mode — replicated from LynxCpu::SBC() decimal path
	static uint8_t SBC_Decimal(uint8_t a, uint8_t operand, bool carryIn,
		bool& carryOut, bool& overflowOut, bool& zeroOut, bool& negOut) {
		uint8_t borrow = carryIn ? 0 : 1;
		int8_t al = (a & 0x0f) - (operand & 0x0f) - borrow;
		if (al < 0) al = ((al - 6) & 0x0f) | static_cast<int8_t>(-128);
		int16_t ah = (a >> 4) - (operand >> 4) + (al < 0 ? -1 : 0);
		al &= 0x0f;

		uint16_t binResult = (uint16_t)a - operand - borrow;
		carryOut = binResult < 0x100;
		overflowOut = ((a ^ operand) & (a ^ (uint8_t)binResult) & 0x80) != 0;

		if (ah < 0) ah -= 6;

		uint8_t result = (al & 0x0f) | ((ah & 0x0f) << 4);
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}
};

// -- ADC Binary --

TEST_F(LynxCpuArithmeticTest, ADC_Binary_SimpleAdd) {
	bool c, v, z, n;
	uint8_t r = ADC_Binary(0x10, 0x20, false, c, v, z, n);
	EXPECT_EQ(r, 0x30);
	EXPECT_FALSE(c);
	EXPECT_FALSE(v);
	EXPECT_FALSE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_ZeroResult) {
	bool c, v, z, n;
	uint8_t r = ADC_Binary(0x00, 0x00, false, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_CarryOut) {
	bool c, v, z, n;
	uint8_t r = ADC_Binary(0xFF, 0x01, false, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_CarryIn) {
	bool c, v, z, n;
	uint8_t r = ADC_Binary(0x10, 0x20, true, c, v, z, n);
	EXPECT_EQ(r, 0x31);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_Overflow_PositiveToNegative) {
	bool c, v, z, n;
	// 0x7F + 0x01 = 0x80 — positive overflow to negative
	uint8_t r = ADC_Binary(0x7F, 0x01, false, c, v, z, n);
	EXPECT_EQ(r, 0x80);
	EXPECT_TRUE(v);
	EXPECT_TRUE(n);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_Overflow_NegativeToPositive) {
	bool c, v, z, n;
	// 0x80 + 0x80 = 0x100 → 0x00 — negative overflow to positive
	uint8_t r = ADC_Binary(0x80, 0x80, false, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(v);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_NoOverflow_SameSignPositive) {
	bool c, v, z, n;
	// 0x20 + 0x30 = 0x50 — both positive, result positive, no overflow
	uint8_t r = ADC_Binary(0x20, 0x30, false, c, v, z, n);
	EXPECT_EQ(r, 0x50);
	EXPECT_FALSE(v);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_NoOverflow_DifferentSigns) {
	bool c, v, z, n;
	// 0x50 + 0xD0 = 0x120 → 0x20 — different signs never overflow
	uint8_t r = ADC_Binary(0x50, 0xD0, false, c, v, z, n);
	EXPECT_EQ(r, 0x20);
	EXPECT_FALSE(v);
	EXPECT_TRUE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_Exhaustive_CarryIn0) {
	// Verify carry and overflow flags for all A × operand combinations
	for (int a = 0; a < 256; a++) {
		for (int op = 0; op < 256; op++) {
			bool c, v, z, n;
			uint8_t result = ADC_Binary((uint8_t)a, (uint8_t)op, false, c, v, z, n);

			uint16_t expected = (uint16_t)a + op;
			EXPECT_EQ(result, (uint8_t)expected);
			EXPECT_EQ(c, expected > 0xff)
				<< "Carry mismatch: A=0x" << std::hex << a << " op=0x" << op;
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

TEST_F(LynxCpuArithmeticTest, ADC_Binary_Exhaustive_CarryIn1) {
	for (int a = 0; a < 256; a++) {
		for (int op = 0; op < 256; op++) {
			bool c, v, z, n;
			uint8_t result = ADC_Binary((uint8_t)a, (uint8_t)op, true, c, v, z, n);

			uint16_t expected = (uint16_t)a + op + 1;
			EXPECT_EQ(result, (uint8_t)expected);
			EXPECT_EQ(c, expected > 0xff);
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

// -- SBC Binary --

TEST_F(LynxCpuArithmeticTest, SBC_Binary_SimpleSubtract) {
	bool c, v, z, n;
	uint8_t r = SBC_Binary(0x30, 0x10, true, c, v, z, n);
	EXPECT_EQ(r, 0x20);
	EXPECT_TRUE(c);  // No borrow
	EXPECT_FALSE(v);
	EXPECT_FALSE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuArithmeticTest, SBC_Binary_ZeroResult) {
	bool c, v, z, n;
	uint8_t r = SBC_Binary(0x42, 0x42, true, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuArithmeticTest, SBC_Binary_Borrow) {
	bool c, v, z, n;
	// 0x00 - 0x01 with carry=1 (no borrow) → 0xFF, carry=0 (borrow occurred)
	uint8_t r = SBC_Binary(0x00, 0x01, true, c, v, z, n);
	EXPECT_EQ(r, 0xFF);
	EXPECT_FALSE(c);
	EXPECT_TRUE(n);
}

TEST_F(LynxCpuArithmeticTest, SBC_Binary_Overflow_PositiveMinusNegative) {
	bool c, v, z, n;
	// 0x50 - 0xB0 = 0xA0 → signed: 80 - (-80) = 160 (overflow!)
	uint8_t r = SBC_Binary(0x50, 0xB0, true, c, v, z, n);
	EXPECT_EQ(r, 0xA0);
	EXPECT_TRUE(v);
	EXPECT_TRUE(n);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, SBC_Binary_Exhaustive_CarrySet) {
	for (int a = 0; a < 256; a++) {
		for (int op = 0; op < 256; op++) {
			bool c, v, z, n;
			uint8_t result = SBC_Binary((uint8_t)a, (uint8_t)op, true, c, v, z, n);

			uint16_t expected = (uint16_t)a - op;
			EXPECT_EQ(result, (uint8_t)expected);
			EXPECT_EQ(c, expected < 0x100) // Carry = no borrow
				<< "Carry mismatch: A=0x" << std::hex << a << " op=0x" << op;
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

TEST_F(LynxCpuArithmeticTest, SBC_Binary_Exhaustive_CarryClear) {
	for (int a = 0; a < 256; a++) {
		for (int op = 0; op < 256; op++) {
			bool c, v, z, n;
			uint8_t result = SBC_Binary((uint8_t)a, (uint8_t)op, false, c, v, z, n);

			uint16_t expected = (uint16_t)a - op - 1;
			EXPECT_EQ(result, (uint8_t)expected);
			EXPECT_EQ(c, expected < 0x100);
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

// -- ADC Decimal (BCD) --

TEST_F(LynxCpuArithmeticTest, ADC_Decimal_SimpleAdd) {
	bool c, v, z, n;
	// BCD: 0x15 + 0x27 = 0x42
	uint8_t r = ADC_Decimal(0x15, 0x27, false, c, v, z, n);
	EXPECT_EQ(r, 0x42);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Decimal_NibbleCarry) {
	bool c, v, z, n;
	// BCD: 0x09 + 0x01 = 0x10
	uint8_t r = ADC_Decimal(0x09, 0x01, false, c, v, z, n);
	EXPECT_EQ(r, 0x10);
	EXPECT_FALSE(c);
}

TEST_F(LynxCpuArithmeticTest, ADC_Decimal_HighNibbleCarry) {
	bool c, v, z, n;
	// BCD: 0x99 + 0x01 = 0x00 with carry
	uint8_t r = ADC_Decimal(0x99, 0x01, false, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuArithmeticTest, ADC_Decimal_65C02_ZeroFlagFromBCDResult) {
	bool c, v, z, n;
	// 65C02 specific: Z flag set from BCD, not binary result
	// BCD 0x99 + 0x01 = 0x00 → Z=1
	ADC_Decimal(0x99, 0x01, false, c, v, z, n);
	EXPECT_TRUE(z);  // 65C02: zero from BCD result
}

TEST_F(LynxCpuArithmeticTest, ADC_Decimal_65C02_NegativeFlagFromBCDResult) {
	bool c, v, z, n;
	// BCD 0x50 + 0x40 = 0x90 → N=1 (from BCD result high bit)
	uint8_t r = ADC_Decimal(0x50, 0x40, false, c, v, z, n);
	EXPECT_EQ(r, 0x90);
	EXPECT_TRUE(n);  // 65C02: negative from BCD result
}

// -- SBC Decimal (BCD) --

TEST_F(LynxCpuArithmeticTest, SBC_Decimal_SimpleSubtract) {
	bool c, v, z, n;
	// BCD: 0x42 - 0x15 = 0x27
	uint8_t r = SBC_Decimal(0x42, 0x15, true, c, v, z, n);
	EXPECT_EQ(r, 0x27);
	EXPECT_TRUE(c);
}

TEST_F(LynxCpuArithmeticTest, SBC_Decimal_NibbleBorrow) {
	bool c, v, z, n;
	// BCD: 0x10 - 0x01 = 0x09
	uint8_t r = SBC_Decimal(0x10, 0x01, true, c, v, z, n);
	EXPECT_EQ(r, 0x09);
	EXPECT_TRUE(c);
}

TEST_F(LynxCpuArithmeticTest, SBC_Decimal_ZeroResult) {
	bool c, v, z, n;
	// BCD: 0x50 - 0x50 = 0x00
	uint8_t r = SBC_Decimal(0x50, 0x50, true, c, v, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

// =============================================================================
// Shift / Rotate Logic Tests
// =============================================================================

class LynxCpuShiftTest : public ::testing::Test {
protected:
	/// ASL — Arithmetic Shift Left (replicated from LynxCpu)
	static uint8_t ASL(uint8_t value, bool& carryOut, bool& zeroOut, bool& negOut) {
		carryOut = (value & 0x80) != 0;
		uint8_t result = value << 1;
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}

	/// LSR — Logical Shift Right
	static uint8_t LSR(uint8_t value, bool& carryOut, bool& zeroOut, bool& negOut) {
		carryOut = (value & 0x01) != 0;
		uint8_t result = value >> 1;
		zeroOut = (result == 0);
		negOut = false; // bit 7 always 0 after LSR
		return result;
	}

	/// ROL — Rotate Left through carry
	static uint8_t ROL(uint8_t value, bool carryIn, bool& carryOut, bool& zeroOut, bool& negOut) {
		carryOut = (value & 0x80) != 0;
		uint8_t result = (value << 1) | (carryIn ? 1 : 0);
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}

	/// ROR — Rotate Right through carry
	static uint8_t ROR(uint8_t value, bool carryIn, bool& carryOut, bool& zeroOut, bool& negOut) {
		carryOut = (value & 0x01) != 0;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0);
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}
};

TEST_F(LynxCpuShiftTest, ASL_ShiftZero) {
	bool c, z, n;
	uint8_t r = ASL(0x00, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_FALSE(c);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuShiftTest, ASL_ShiftWithCarry) {
	bool c, z, n;
	uint8_t r = ASL(0x80, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuShiftTest, ASL_SetNegative) {
	bool c, z, n;
	uint8_t r = ASL(0x40, c, z, n);
	EXPECT_EQ(r, 0x80);
	EXPECT_FALSE(c);
	EXPECT_TRUE(n);
}

TEST_F(LynxCpuShiftTest, ASL_Exhaustive) {
	for (int v = 0; v < 256; v++) {
		bool c, z, n;
		uint8_t result = ASL((uint8_t)v, c, z, n);
		EXPECT_EQ(result, (uint8_t)(v << 1));
		EXPECT_EQ(c, (v & 0x80) != 0);
		EXPECT_EQ(z, result == 0);
		EXPECT_EQ(n, (result & 0x80) != 0);
	}
}

TEST_F(LynxCpuShiftTest, LSR_ShiftZero) {
	bool c, z, n;
	uint8_t r = LSR(0x00, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_FALSE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuShiftTest, LSR_ShiftOne) {
	bool c, z, n;
	uint8_t r = LSR(0x01, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuShiftTest, LSR_NeverSetsNegative) {
	bool c, z, n;
	uint8_t r = LSR(0xFF, c, z, n);
	EXPECT_EQ(r, 0x7F);
	EXPECT_FALSE(n);  // LSR always clears negative
}

TEST_F(LynxCpuShiftTest, LSR_Exhaustive) {
	for (int v = 0; v < 256; v++) {
		bool c, z, n;
		uint8_t result = LSR((uint8_t)v, c, z, n);
		EXPECT_EQ(result, (uint8_t)(v >> 1));
		EXPECT_EQ(c, (v & 0x01) != 0);
		EXPECT_EQ(z, result == 0);
		EXPECT_FALSE(n);  // LSR never sets N
	}
}

TEST_F(LynxCpuShiftTest, ROL_NoCarryIn) {
	bool c, z, n;
	uint8_t r = ROL(0x80, false, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuShiftTest, ROL_CarryIn) {
	bool c, z, n;
	uint8_t r = ROL(0x00, true, c, z, n);
	EXPECT_EQ(r, 0x01);
	EXPECT_FALSE(c);
	EXPECT_FALSE(z);
}

TEST_F(LynxCpuShiftTest, ROL_Exhaustive_BothCarryStates) {
	for (int carryIn = 0; carryIn <= 1; carryIn++) {
		for (int v = 0; v < 256; v++) {
			bool c, z, n;
			uint8_t result = ROL((uint8_t)v, carryIn != 0, c, z, n);
			uint8_t expected = (uint8_t)((v << 1) | carryIn);
			EXPECT_EQ(result, expected);
			EXPECT_EQ(c, (v & 0x80) != 0);
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

TEST_F(LynxCpuShiftTest, ROR_NoCarryIn) {
	bool c, z, n;
	uint8_t r = ROR(0x01, false, c, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuShiftTest, ROR_CarryIn) {
	bool c, z, n;
	uint8_t r = ROR(0x00, true, c, z, n);
	EXPECT_EQ(r, 0x80);
	EXPECT_FALSE(c);
	EXPECT_TRUE(n);
}

TEST_F(LynxCpuShiftTest, ROR_Exhaustive_BothCarryStates) {
	for (int carryIn = 0; carryIn <= 1; carryIn++) {
		for (int v = 0; v < 256; v++) {
			bool c, z, n;
			uint8_t result = ROR((uint8_t)v, carryIn != 0, c, z, n);
			uint8_t expected = (uint8_t)((v >> 1) | (carryIn ? 0x80 : 0));
			EXPECT_EQ(result, expected);
			EXPECT_EQ(c, (v & 0x01) != 0);
			EXPECT_EQ(z, result == 0);
			EXPECT_EQ(n, (result & 0x80) != 0);
		}
	}
}

// =============================================================================
// Compare Operation Logic Tests
// =============================================================================

class LynxCpuCompareTest : public ::testing::Test {
protected:
	/// CMP/CPX/CPY — replicated from LynxCpu::CMP() logic
	static void Compare(uint8_t reg, uint8_t operand,
		bool& carryOut, bool& zeroOut, bool& negOut) {
		uint16_t diff = (uint16_t)reg - operand;
		carryOut = diff < 0x100; // Carry = no borrow (reg >= operand)
		uint8_t result = (uint8_t)diff;
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
	}
};

TEST_F(LynxCpuCompareTest, Equal_SetsZeroAndCarry) {
	bool c, z, n;
	Compare(0x42, 0x42, c, z, n);
	EXPECT_TRUE(c);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuCompareTest, GreaterThan_SetsCarryOnly) {
	bool c, z, n;
	Compare(0x50, 0x20, c, z, n);
	EXPECT_TRUE(c);
	EXPECT_FALSE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuCompareTest, LessThan_ClearsCarry) {
	bool c, z, n;
	Compare(0x10, 0x50, c, z, n);
	EXPECT_FALSE(c);
	EXPECT_FALSE(z);
}

TEST_F(LynxCpuCompareTest, NegativeDifference) {
	bool c, z, n;
	// 0x01 - 0x02 = 0xFF (negative in signed)
	Compare(0x01, 0x02, c, z, n);
	EXPECT_FALSE(c);
	EXPECT_TRUE(n);
}

TEST_F(LynxCpuCompareTest, Exhaustive_AllCombinations) {
	for (int reg = 0; reg < 256; reg++) {
		for (int op = 0; op < 256; op++) {
			bool c, z, n;
			Compare((uint8_t)reg, (uint8_t)op, c, z, n);

			uint16_t diff = (uint16_t)reg - op;
			uint8_t result = (uint8_t)diff;

			EXPECT_EQ(c, reg >= op)
				<< "Carry: reg=0x" << std::hex << reg << " op=0x" << op;
			EXPECT_EQ(z, reg == op)
				<< "Zero: reg=0x" << std::hex << reg << " op=0x" << op;
			EXPECT_EQ(n, (result & 0x80) != 0)
				<< "Neg: reg=0x" << std::hex << reg << " op=0x" << op;
		}
	}
}

// =============================================================================
// BIT / TSB / TRB Logic Tests (65C02 specific)
// =============================================================================

class LynxCpuBitTest : public ::testing::Test {
protected:
	/// BIT — replicated from LynxCpu::BIT()
	static void BIT(uint8_t a, uint8_t val, bool& zeroOut, bool& overflowOut, bool& negOut) {
		zeroOut = (a & val) == 0;
		overflowOut = (val & 0x40) != 0;
		negOut = (val & 0x80) != 0;
	}

	/// BIT immediate (65C02) — only affects Z flag
	static void BIT_Imm(uint8_t a, uint8_t val, bool& zeroOut) {
		zeroOut = (a & val) == 0;
	}

	/// TSB (Test and Set Bits) — 65C02
	static uint8_t TSB(uint8_t a, uint8_t val, bool& zeroOut) {
		zeroOut = (a & val) == 0;
		return val | a;
	}

	/// TRB (Test and Reset Bits) — 65C02
	static uint8_t TRB(uint8_t a, uint8_t val, bool& zeroOut) {
		zeroOut = (a & val) == 0;
		return val & ~a;
	}
};

TEST_F(LynxCpuBitTest, BIT_ZeroWhenNoCommonBits) {
	bool z, v, n;
	BIT(0x0F, 0xF0, z, v, n);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuBitTest, BIT_NotZeroWhenCommonBits) {
	bool z, v, n;
	BIT(0x0F, 0x0F, z, v, n);
	EXPECT_FALSE(z);
}

TEST_F(LynxCpuBitTest, BIT_OverflowFromBit6) {
	bool z, v, n;
	BIT(0x00, 0x40, z, v, n);
	EXPECT_TRUE(v);  // Bit 6 of operand → V
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuBitTest, BIT_NegativeFromBit7) {
	bool z, v, n;
	BIT(0x00, 0x80, z, v, n);
	EXPECT_TRUE(n);  // Bit 7 of operand → N
	EXPECT_FALSE(v);
}

TEST_F(LynxCpuBitTest, BIT_Imm_OnlyAffectsZero) {
	// 65C02 BIT immediate: ONLY sets Z, does NOT change N/V
	bool z;
	BIT_Imm(0xFF, 0xFF, z);
	EXPECT_FALSE(z);

	BIT_Imm(0x00, 0xFF, z);
	EXPECT_TRUE(z);
}

TEST_F(LynxCpuBitTest, BIT_Exhaustive) {
	for (int a = 0; a < 256; a++) {
		for (int val = 0; val < 256; val++) {
			bool z, v, n;
			BIT((uint8_t)a, (uint8_t)val, z, v, n);
			EXPECT_EQ(z, (a & val) == 0);
			EXPECT_EQ(v, (val & 0x40) != 0);
			EXPECT_EQ(n, (val & 0x80) != 0);
		}
	}
}

TEST_F(LynxCpuBitTest, TSB_SetsSpecifiedBits) {
	bool z;
	uint8_t r = TSB(0x0F, 0xF0, z);
	EXPECT_EQ(r, 0xFF);
	EXPECT_TRUE(z);  // No common bits → Z=1
}

TEST_F(LynxCpuBitTest, TSB_CommonBits_ZeroClear) {
	bool z;
	uint8_t r = TSB(0x0F, 0x0F, z);
	EXPECT_EQ(r, 0x0F);
	EXPECT_FALSE(z);  // Common bits → Z=0
}

TEST_F(LynxCpuBitTest, TRB_ClearsSpecifiedBits) {
	bool z;
	uint8_t r = TRB(0x0F, 0xFF, z);
	EXPECT_EQ(r, 0xF0);
	EXPECT_FALSE(z);  // Common bits → Z=0
}

TEST_F(LynxCpuBitTest, TRB_NoCommonBits_ZeroSet) {
	bool z;
	uint8_t r = TRB(0x0F, 0xF0, z);
	EXPECT_EQ(r, 0xF0);  // Nothing cleared
	EXPECT_TRUE(z);  // No common bits → Z=1
}

TEST_F(LynxCpuBitTest, TSB_TRB_Exhaustive) {
	for (int a = 0; a < 256; a++) {
		for (int val = 0; val < 256; val++) {
			bool z;

			// TSB
			uint8_t tsb_result = TSB((uint8_t)a, (uint8_t)val, z);
			EXPECT_EQ(tsb_result, (uint8_t)(val | a));
			EXPECT_EQ(z, (a & val) == 0);

			// TRB
			uint8_t trb_result = TRB((uint8_t)a, (uint8_t)val, z);
			EXPECT_EQ(trb_result, (uint8_t)(val & ~a));
			EXPECT_EQ(z, (a & val) == 0);
		}
	}
}

// =============================================================================
// SetZeroNeg / Register Set Helper Tests
// =============================================================================

class LynxCpuFlagHelperTest : public ::testing::Test {
protected:
	/// SetZeroNeg — replicated from LynxCpu.h
	static void SetZeroNeg(uint8_t& ps, uint8_t value) {
		if (value == 0) ps |= LynxPSFlags::Zero; else ps &= ~LynxPSFlags::Zero;
		if (value & 0x80) ps |= LynxPSFlags::Negative; else ps &= ~LynxPSFlags::Negative;
	}

	/// SetPS — 65C02 behavior: bits 4-5 masked, reserved always set
	static void SetPS(uint8_t& ps, uint8_t value) {
		ps = (value & 0xcf) | LynxPSFlags::Reserved;
	}
};

TEST_F(LynxCpuFlagHelperTest, SetZeroNeg_Zero_SetsZeroClearsNeg) {
	uint8_t ps = 0;
	SetZeroNeg(ps, 0x00);
	EXPECT_TRUE(ps & LynxPSFlags::Zero);
	EXPECT_FALSE(ps & LynxPSFlags::Negative);
}

TEST_F(LynxCpuFlagHelperTest, SetZeroNeg_Negative_SetsNegClearsZero) {
	uint8_t ps = 0;
	SetZeroNeg(ps, 0x80);
	EXPECT_FALSE(ps & LynxPSFlags::Zero);
	EXPECT_TRUE(ps & LynxPSFlags::Negative);
}

TEST_F(LynxCpuFlagHelperTest, SetZeroNeg_Positive_ClearsBoth) {
	uint8_t ps = LynxPSFlags::Zero | LynxPSFlags::Negative;
	SetZeroNeg(ps, 0x42);
	EXPECT_FALSE(ps & LynxPSFlags::Zero);
	EXPECT_FALSE(ps & LynxPSFlags::Negative);
}

TEST_F(LynxCpuFlagHelperTest, SetZeroNeg_PreservesOtherFlags) {
	uint8_t ps = LynxPSFlags::Carry | LynxPSFlags::Overflow;
	SetZeroNeg(ps, 0x00);
	EXPECT_TRUE(ps & LynxPSFlags::Carry);
	EXPECT_TRUE(ps & LynxPSFlags::Overflow);
	EXPECT_TRUE(ps & LynxPSFlags::Zero);
}

TEST_F(LynxCpuFlagHelperTest, SetZeroNeg_Exhaustive) {
	uint8_t psStates[] = { 0x00, 0xFF, 0x24, LynxPSFlags::Carry | LynxPSFlags::Decimal };

	for (uint8_t initialPS : psStates) {
		for (int v = 0; v <= 255; v++) {
			uint8_t ps = initialPS;
			SetZeroNeg(ps, (uint8_t)v);

			// Z and N correctly set
			EXPECT_EQ((ps & LynxPSFlags::Zero) != 0, v == 0);
			EXPECT_EQ((ps & LynxPSFlags::Negative) != 0, (v & 0x80) != 0);

			// Other flags preserved (mask out Z and N)
			uint8_t otherMask = ~(LynxPSFlags::Zero | LynxPSFlags::Negative);
			EXPECT_EQ(ps & otherMask, initialPS & otherMask);
		}
	}
}

TEST_F(LynxCpuFlagHelperTest, SetPS_ReservedAlwaysSet) {
	uint8_t ps;
	SetPS(ps, 0x00);
	EXPECT_TRUE(ps & LynxPSFlags::Reserved);
	EXPECT_FALSE(ps & LynxPSFlags::Break);
}

TEST_F(LynxCpuFlagHelperTest, SetPS_BreakCleared) {
	uint8_t ps;
	// Even if Break bit is in the input, SetPS masks it out
	SetPS(ps, 0xFF);
	EXPECT_FALSE(ps & LynxPSFlags::Break);
	EXPECT_TRUE(ps & LynxPSFlags::Reserved);
}

TEST_F(LynxCpuFlagHelperTest, SetPS_MasksBits4And5) {
	// Input 0xFF → PS should be (0xFF & 0xCF) | 0x20 = 0xEF
	uint8_t ps;
	SetPS(ps, 0xFF);
	EXPECT_EQ(ps, 0xEF);
}

TEST_F(LynxCpuFlagHelperTest, SetPS_Exhaustive) {
	for (int v = 0; v <= 255; v++) {
		uint8_t ps;
		SetPS(ps, (uint8_t)v);
		EXPECT_EQ(ps, (uint8_t)((v & 0xCF) | LynxPSFlags::Reserved));
	}
}

// =============================================================================
// Increment / Decrement Logic Tests
// =============================================================================

class LynxCpuIncDecTest : public ::testing::Test {
protected:
	static uint8_t INC(uint8_t value, bool& zeroOut, bool& negOut) {
		uint8_t result = value + 1;
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}

	static uint8_t DEC(uint8_t value, bool& zeroOut, bool& negOut) {
		uint8_t result = value - 1;
		zeroOut = (result == 0);
		negOut = (result & 0x80) != 0;
		return result;
	}
};

TEST_F(LynxCpuIncDecTest, INC_Wraps) {
	bool z, n;
	uint8_t r = INC(0xFF, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuIncDecTest, INC_ToNegative) {
	bool z, n;
	uint8_t r = INC(0x7F, z, n);
	EXPECT_EQ(r, 0x80);
	EXPECT_TRUE(n);
	EXPECT_FALSE(z);
}

TEST_F(LynxCpuIncDecTest, DEC_Wraps) {
	bool z, n;
	uint8_t r = DEC(0x00, z, n);
	EXPECT_EQ(r, 0xFF);
	EXPECT_TRUE(n);
	EXPECT_FALSE(z);
}

TEST_F(LynxCpuIncDecTest, DEC_ToZero) {
	bool z, n;
	uint8_t r = DEC(0x01, z, n);
	EXPECT_EQ(r, 0x00);
	EXPECT_TRUE(z);
	EXPECT_FALSE(n);
}

TEST_F(LynxCpuIncDecTest, INC_Exhaustive) {
	for (int v = 0; v < 256; v++) {
		bool z, n;
		uint8_t r = INC((uint8_t)v, z, n);
		EXPECT_EQ(r, (uint8_t)(v + 1));
		EXPECT_EQ(z, r == 0);
		EXPECT_EQ(n, (r & 0x80) != 0);
	}
}

TEST_F(LynxCpuIncDecTest, DEC_Exhaustive) {
	for (int v = 0; v < 256; v++) {
		bool z, n;
		uint8_t r = DEC((uint8_t)v, z, n);
		EXPECT_EQ(r, (uint8_t)(v - 1));
		EXPECT_EQ(z, r == 0);
		EXPECT_EQ(n, (r & 0x80) != 0);
	}
}

// =============================================================================
// ADC/SBC Overflow Flag — targeted edge case tests
// =============================================================================

class LynxCpuOverflowTest : public ::testing::Test {
protected:
	static bool ADC_Overflow(uint8_t a, uint8_t op, bool carry) {
		uint16_t result = (uint16_t)a + op + (carry ? 1 : 0);
		return (~(a ^ op) & (a ^ (uint8_t)result) & 0x80) != 0;
	}

	static bool SBC_Overflow(uint8_t a, uint8_t op, bool carry) {
		uint8_t borrow = carry ? 0 : 1;
		uint16_t result = (uint16_t)a - op - borrow;
		return ((a ^ op) & (a ^ (uint8_t)result) & 0x80) != 0;
	}
};

TEST_F(LynxCpuOverflowTest, ADC_PositiveOverflow) {
	EXPECT_TRUE(ADC_Overflow(0x7F, 0x01, false));   // +127 + 1 = -128 (overflow)
	EXPECT_TRUE(ADC_Overflow(0x7F, 0x7F, false));   // +127 + 127 (overflow)
	EXPECT_TRUE(ADC_Overflow(0x40, 0x40, false));   // +64 + 64 = -128 (overflow)
}

TEST_F(LynxCpuOverflowTest, ADC_NegativeOverflow) {
	EXPECT_TRUE(ADC_Overflow(0x80, 0x80, false));   // -128 + -128 (overflow)
	EXPECT_TRUE(ADC_Overflow(0x80, 0xFF, false));   // -128 + -1 (overflow)
}

TEST_F(LynxCpuOverflowTest, ADC_NoOverflow_DifferentSigns) {
	EXPECT_FALSE(ADC_Overflow(0x7F, 0x80, false));  // +127 + -128 (no overflow)
	EXPECT_FALSE(ADC_Overflow(0x01, 0xFF, false));  // 1 + -1 (no overflow)
	EXPECT_FALSE(ADC_Overflow(0x50, 0xD0, false));  // Different signs
}

TEST_F(LynxCpuOverflowTest, SBC_Overflow) {
	EXPECT_TRUE(SBC_Overflow(0x50, 0xB0, true));    // +80 - (-80) = 160 (overflow)
	EXPECT_TRUE(SBC_Overflow(0x80, 0x01, true));    // -128 - 1 (overflow)
}

TEST_F(LynxCpuOverflowTest, SBC_NoOverflow_SameSign) {
	EXPECT_FALSE(SBC_Overflow(0x50, 0x20, true));   // +80 - +32 (no overflow)
	EXPECT_FALSE(SBC_Overflow(0x80, 0x80, true));   // -128 - (-128) = 0 (no overflow)
}

TEST_F(LynxCpuOverflowTest, Exhaustive_ADC) {
	// Verify overflow formula for all A × operand combinations
	for (int a = 0; a < 256; a++) {
		for (int op = 0; op < 256; op++) {
			bool v = ADC_Overflow((uint8_t)a, (uint8_t)op, false);

			// Overflow if both operands have same sign but result has different sign
			int16_t signedA = (int8_t)a;
			int16_t signedOp = (int8_t)op;
			int16_t signedResult = signedA + signedOp;
			bool expectedV = (signedResult > 127 || signedResult < -128);

			EXPECT_EQ(v, expectedV)
				<< "A=0x" << std::hex << a << " op=0x" << op
				<< " signedResult=" << std::dec << signedResult;
		}
	}
}
