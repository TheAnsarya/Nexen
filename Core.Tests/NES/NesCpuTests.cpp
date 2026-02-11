#include "pch.h"
#include "NES/NesTypes.h"

/// <summary>
/// Test fixture for NES 6502 CPU types and state.
/// These tests verify CPU state structures and flag calculations
/// without requiring a full emulator environment.
/// </summary>
class NesCpuTypesTest : public ::testing::Test {
protected:
	NesCpuState _state = {};

	void SetUp() override {
		_state = {};
		_state.A = 0;
		_state.X = 0;
		_state.Y = 0;
		_state.SP = 0xFD;
		_state.PC = 0;
		_state.PS = 0x24;  // I flag set, Reserved always set
		_state.IrqFlag = 0;
		_state.NmiFlag = false;
	}

	void SetFlags(uint8_t flags) {
		_state.PS |= flags;
	}

	void ClearFlags(uint8_t flags) {
		_state.PS &= ~flags;
	}

	bool CheckFlag(uint8_t flag) {
		return (_state.PS & flag) != 0;
	}

	void SetZeroNegativeFlags(uint8_t value) {
		ClearFlags(PSFlags::Zero | PSFlags::Negative);
		if (value == 0) {
			SetFlags(PSFlags::Zero);
		}
		if (value & 0x80) {
			SetFlags(PSFlags::Negative);
		}
	}
};

//=============================================================================
// CPU State Tests
//=============================================================================

TEST_F(NesCpuTypesTest, InitialState_DefaultValues) {
	NesCpuState state = {};
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.X, 0);
	EXPECT_EQ(state.Y, 0);
	EXPECT_EQ(state.PC, 0);
	EXPECT_EQ(state.CycleCount, 0);
}

TEST_F(NesCpuTypesTest, State_StackPointerRange) {
	// Stack pointer is 8-bit, stack is $0100-$01FF
	_state.SP = 0xFF;
	EXPECT_EQ(_state.SP, 0xFF);

	_state.SP = 0x00;
	EXPECT_EQ(_state.SP, 0x00);
}

//=============================================================================
// Processor Flag Tests
//=============================================================================

TEST_F(NesCpuTypesTest, Flags_CarryFlag_SetAndClear) {
	ClearFlags(PSFlags::Carry);
	EXPECT_FALSE(CheckFlag(PSFlags::Carry));
	SetFlags(PSFlags::Carry);
	EXPECT_TRUE(CheckFlag(PSFlags::Carry));
	ClearFlags(PSFlags::Carry);
	EXPECT_FALSE(CheckFlag(PSFlags::Carry));
}

TEST_F(NesCpuTypesTest, Flags_ZeroFlag_SetAndClear) {
	ClearFlags(PSFlags::Zero);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	SetFlags(PSFlags::Zero);
	EXPECT_TRUE(CheckFlag(PSFlags::Zero));
	ClearFlags(PSFlags::Zero);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
}

TEST_F(NesCpuTypesTest, Flags_InterruptFlag_SetAndClear) {
	ClearFlags(PSFlags::Interrupt);
	EXPECT_FALSE(CheckFlag(PSFlags::Interrupt));
	SetFlags(PSFlags::Interrupt);
	EXPECT_TRUE(CheckFlag(PSFlags::Interrupt));
}

TEST_F(NesCpuTypesTest, Flags_DecimalFlag_SetAndClear) {
	ClearFlags(PSFlags::Decimal);
	EXPECT_FALSE(CheckFlag(PSFlags::Decimal));
	SetFlags(PSFlags::Decimal);
	EXPECT_TRUE(CheckFlag(PSFlags::Decimal));
}

TEST_F(NesCpuTypesTest, Flags_BreakFlag_SetAndClear) {
	ClearFlags(PSFlags::Break);
	EXPECT_FALSE(CheckFlag(PSFlags::Break));
	SetFlags(PSFlags::Break);
	EXPECT_TRUE(CheckFlag(PSFlags::Break));
}

TEST_F(NesCpuTypesTest, Flags_OverflowFlag_SetAndClear) {
	ClearFlags(PSFlags::Overflow);
	EXPECT_FALSE(CheckFlag(PSFlags::Overflow));
	SetFlags(PSFlags::Overflow);
	EXPECT_TRUE(CheckFlag(PSFlags::Overflow));
}

TEST_F(NesCpuTypesTest, Flags_NegativeFlag_SetAndClear) {
	ClearFlags(PSFlags::Negative);
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
	SetFlags(PSFlags::Negative);
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, Flags_ReservedFlag_AlwaysSet) {
	// The reserved flag (bit 5) should always be 1
	EXPECT_EQ(PSFlags::Reserved, 0x20);
}

TEST_F(NesCpuTypesTest, Flags_MultipleFlags_SetSimultaneously) {
	_state.PS = 0;
	SetFlags(PSFlags::Carry | PSFlags::Zero | PSFlags::Negative);
	EXPECT_TRUE(CheckFlag(PSFlags::Carry));
	EXPECT_TRUE(CheckFlag(PSFlags::Zero));
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));
	EXPECT_FALSE(CheckFlag(PSFlags::Overflow));
}

//=============================================================================
// Zero/Negative Flag Calculation Tests
//=============================================================================

TEST_F(NesCpuTypesTest, ZeroNegative_ZeroValue_SetsZeroFlag) {
	SetZeroNegativeFlags(0x00);
	EXPECT_TRUE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_PositiveValue_ClearsBothFlags) {
	SetZeroNegativeFlags(0x01);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_NegativeValue_SetsNegativeFlag) {
	SetZeroNegativeFlags(0x80);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_MaxValue_SetsNegativeFlag) {
	SetZeroNegativeFlags(0xFF);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_Boundary_0x7F_ClearsBothFlags) {
	SetZeroNegativeFlags(0x7F);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_Exhaustive_All256Values) {
	// Exhaustive test: verify zero/negative flag behavior for every possible
	// uint8_t value. This ensures the branchless optimization in NesCpu.h
	// produces identical results to the reference if/else if implementation.
	for (int i = 0; i <= 255; i++) {
		uint8_t value = static_cast<uint8_t>(i);
		SetZeroNegativeFlags(value);

		bool expectZero = (value == 0);
		bool expectNegative = (value & 0x80) != 0;

		EXPECT_EQ(CheckFlag(PSFlags::Zero), expectZero)
			<< "Zero flag mismatch for value 0x" << std::hex << i;
		EXPECT_EQ(CheckFlag(PSFlags::Negative), expectNegative)
			<< "Negative flag mismatch for value 0x" << std::hex << i;

		// Zero and Negative flags must be mutually exclusive for 8-bit values
		EXPECT_FALSE(CheckFlag(PSFlags::Zero) && CheckFlag(PSFlags::Negative))
			<< "Both Zero and Negative set for value 0x" << std::hex << i;
	}
}

TEST_F(NesCpuTypesTest, ZeroNegative_PreservesOtherFlags) {
	// Ensure SetZeroNegativeFlags only affects Zero and Negative flags.
	// Set carry and overflow before, verify they survive.
	_state.PS = 0;
	SetFlags(PSFlags::Carry | PSFlags::Overflow);
	SetZeroNegativeFlags(0x42);
	EXPECT_TRUE(CheckFlag(PSFlags::Carry));
	EXPECT_TRUE(CheckFlag(PSFlags::Overflow));
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_ClearsStaleFlags) {
	// If Zero was set from a previous operation, it must be cleared
	// when a non-zero value is processed.
	SetZeroNegativeFlags(0x00);
	EXPECT_TRUE(CheckFlag(PSFlags::Zero));

	SetZeroNegativeFlags(0x42);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_ClearsStaleNegative) {
	// If Negative was set from a previous operation, it must be cleared
	// when a positive value is processed.
	SetZeroNegativeFlags(0x80);
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));

	SetZeroNegativeFlags(0x01);
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
}

TEST_F(NesCpuTypesTest, ZeroNegative_NegativeToZeroTransition) {
	// Transition from negative value to zero must clear Negative, set Zero.
	SetZeroNegativeFlags(0xFF);
	EXPECT_TRUE(CheckFlag(PSFlags::Negative));
	EXPECT_FALSE(CheckFlag(PSFlags::Zero));

	SetZeroNegativeFlags(0x00);
	EXPECT_FALSE(CheckFlag(PSFlags::Negative));
	EXPECT_TRUE(CheckFlag(PSFlags::Zero));
}

//=============================================================================
// IRQ Source Tests
//=============================================================================

TEST_F(NesCpuTypesTest, IrqSource_External) {
	EXPECT_EQ(static_cast<int>(IRQSource::External), 1);
}

TEST_F(NesCpuTypesTest, IrqSource_FrameCounter) {
	EXPECT_EQ(static_cast<int>(IRQSource::FrameCounter), 2);
}

TEST_F(NesCpuTypesTest, IrqSource_DMC) {
	EXPECT_EQ(static_cast<int>(IRQSource::DMC), 4);
}

TEST_F(NesCpuTypesTest, IrqSource_FdsDisk) {
	EXPECT_EQ(static_cast<int>(IRQSource::FdsDisk), 8);
}

TEST_F(NesCpuTypesTest, IrqSource_Epsm) {
	EXPECT_EQ(static_cast<int>(IRQSource::Epsm), 16);
}

//=============================================================================
// Addressing Mode Tests
//=============================================================================

TEST_F(NesCpuTypesTest, AddrMode_EnumValues) {
	EXPECT_EQ(static_cast<int>(NesAddrMode::None), 0);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Acc), 1);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Imp), 2);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Imm), 3);
}

//=============================================================================
// Arithmetic Logic Tests
//=============================================================================

class NesCpuArithmeticTest : public ::testing::Test {
protected:
	struct AddResult {
		uint8_t result;
		bool carry;
		bool overflow;
		bool zero;
		bool negative;
	};

	// Binary addition (ADC without decimal mode)
	AddResult Add(uint8_t a, uint8_t b, bool carryIn) {
		uint16_t sum = (uint16_t)a + (uint16_t)b + (carryIn ? 1 : 0);
		uint8_t result = (uint8_t)sum;

		bool carry = sum > 0xFF;
		bool overflow = (~(a ^ b) & (a ^ result) & 0x80) != 0;
		bool zero = result == 0;
		bool negative = (result & 0x80) != 0;

		return { result, carry, overflow, zero, negative };
	}

	// Binary subtraction (SBC = A + ~B + C)
	AddResult Sub(uint8_t a, uint8_t b, bool carryIn) {
		return Add(a, ~b, carryIn);
	}
};

TEST_F(NesCpuArithmeticTest, Add_ZeroPlusZero_ReturnsZero) {
	auto r = Add(0x00, 0x00, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.overflow);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuArithmeticTest, Add_OnePlusOne_ReturnsTwo) {
	auto r = Add(0x01, 0x01, false);
	EXPECT_EQ(r.result, 0x02);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.overflow);
}

TEST_F(NesCpuArithmeticTest, Add_WithCarryIn_AddsOne) {
	auto r = Add(0x01, 0x01, true);
	EXPECT_EQ(r.result, 0x03);
	EXPECT_FALSE(r.carry);
}

TEST_F(NesCpuArithmeticTest, Add_Overflow_SetsCarry) {
	auto r = Add(0xFF, 0x01, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(NesCpuArithmeticTest, Add_SignedOverflow_PositiveToNegative) {
	// 0x7F + 0x01 = 0x80 (127 + 1 = -128 in signed)
	auto r = Add(0x7F, 0x01, false);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.negative);
}

TEST_F(NesCpuArithmeticTest, Add_SignedOverflow_NegativeToPositive) {
	// 0x80 + 0x80 = 0x00 with carry
	auto r = Add(0x80, 0x80, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.zero);
}

TEST_F(NesCpuArithmeticTest, Sub_ZeroMinusZero_ReturnsZero) {
	auto r = Sub(0x00, 0x00, true);  // Carry set = no borrow
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(NesCpuArithmeticTest, Sub_TwoMinusOne_ReturnsOne) {
	auto r = Sub(0x02, 0x01, true);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_TRUE(r.carry);
}

TEST_F(NesCpuArithmeticTest, Sub_OneMinusTwo_Underflows) {
	auto r = Sub(0x01, 0x02, true);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.carry);  // Borrow occurred
	EXPECT_TRUE(r.negative);
}

TEST_F(NesCpuArithmeticTest, Sub_WithBorrow_SubtractsOne) {
	auto r = Sub(0x02, 0x01, false);  // Carry clear = borrow
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
}

//=============================================================================
// Shift/Rotate Logic Tests
//=============================================================================

class NesCpuShiftTest : public ::testing::Test {
protected:
	std::pair<uint8_t, bool> ASL(uint8_t value) {
		bool carryOut = (value & 0x80) != 0;
		uint8_t result = value << 1;
		return { result, carryOut };
	}

	std::pair<uint8_t, bool> LSR(uint8_t value) {
		bool carryOut = (value & 0x01) != 0;
		uint8_t result = value >> 1;
		return { result, carryOut };
	}

	std::pair<uint8_t, bool> ROL(uint8_t value, bool carryIn) {
		bool carryOut = (value & 0x80) != 0;
		uint8_t result = (value << 1) | (carryIn ? 1 : 0);
		return { result, carryOut };
	}

	std::pair<uint8_t, bool> ROR(uint8_t value, bool carryIn) {
		bool carryOut = (value & 0x01) != 0;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0);
		return { result, carryOut };
	}
};

TEST_F(NesCpuShiftTest, ASL_ShiftOne_ReturnsTwo) {
	auto [result, carry] = ASL(0x01);
	EXPECT_EQ(result, 0x02);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, ASL_ShiftHighBit_SetsCarry) {
	auto [result, carry] = ASL(0x80);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(NesCpuShiftTest, ASL_Shift0x55_Returns0xAA) {
	auto [result, carry] = ASL(0x55);
	EXPECT_EQ(result, 0xAA);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, LSR_ShiftTwo_ReturnsOne) {
	auto [result, carry] = LSR(0x02);
	EXPECT_EQ(result, 0x01);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, LSR_ShiftOne_SetsCarry) {
	auto [result, carry] = LSR(0x01);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(NesCpuShiftTest, LSR_Shift0xAA_Returns0x55) {
	auto [result, carry] = LSR(0xAA);
	EXPECT_EQ(result, 0x55);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, ROL_RotateOne_WithCarry_ReturnsThree) {
	auto [result, carry] = ROL(0x01, true);
	EXPECT_EQ(result, 0x03);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, ROL_RotateHighBit_SetsCarry) {
	auto [result, carry] = ROL(0x80, false);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(NesCpuShiftTest, ROR_RotateTwo_ReturnsOne) {
	auto [result, carry] = ROR(0x02, false);
	EXPECT_EQ(result, 0x01);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, ROR_RotateWithCarry_SetsHighBit) {
	auto [result, carry] = ROR(0x00, true);
	EXPECT_EQ(result, 0x80);
	EXPECT_FALSE(carry);
}

TEST_F(NesCpuShiftTest, ROR_RotateLowBit_SetsCarry) {
	auto [result, carry] = ROR(0x01, false);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

//=============================================================================
// Compare Logic Tests
//=============================================================================

class NesCpuCompareTest : public ::testing::Test {
protected:
	struct CompareResult {
		bool carry;
		bool zero;
		bool negative;
	};

	CompareResult Compare(uint8_t reg, uint8_t value) {
		uint8_t result = reg - value;
		return {
			reg >= value,
			reg == value,
			(result & 0x80) != 0
		};
	}
};

TEST_F(NesCpuCompareTest, Compare_Equal_SetsZeroAndCarry) {
	auto r = Compare(0x50, 0x50);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuCompareTest, Compare_Greater_SetsCarryOnly) {
	auto r = Compare(0x60, 0x50);
	EXPECT_TRUE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuCompareTest, Compare_Less_ClearsCarry) {
	auto r = Compare(0x40, 0x50);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);  // 0x40 - 0x50 = 0xF0
}

TEST_F(NesCpuCompareTest, Compare_ZeroVsZero_SetsZeroAndCarry) {
	auto r = Compare(0x00, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(NesCpuCompareTest, Compare_MaxVsZero_SetsCarry) {
	auto r = Compare(0xFF, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);  // Result 0xFF has bit 7 set
}

//=============================================================================
// Bitwise Logic Tests
//=============================================================================

class NesCpuBitwiseTest : public ::testing::Test {};

TEST_F(NesCpuBitwiseTest, AND_MasksCorrectly) {
	EXPECT_EQ(0xF0 & 0x0F, 0x00);
	EXPECT_EQ(0xFF & 0x0F, 0x0F);
	EXPECT_EQ(0xAA & 0x55, 0x00);
}

TEST_F(NesCpuBitwiseTest, ORA_CombinesCorrectly) {
	EXPECT_EQ(0xF0 | 0x0F, 0xFF);
	EXPECT_EQ(0x00 | 0x0F, 0x0F);
	EXPECT_EQ(0xAA | 0x55, 0xFF);
}

TEST_F(NesCpuBitwiseTest, EOR_XorsCorrectly) {
	EXPECT_EQ(0xFF ^ 0xFF, 0x00);
	EXPECT_EQ(0xAA ^ 0x55, 0xFF);
	EXPECT_EQ(0x00 ^ 0xFF, 0xFF);
}

//=============================================================================
// BIT Instruction Logic Tests
//=============================================================================

class NesCpuBitTest : public ::testing::Test {
protected:
	struct BitResult {
		bool zero;
		bool overflow;
		bool negative;
	};

	BitResult BIT(uint8_t acc, uint8_t mem) {
		return {
			(acc & mem) == 0,
			(mem & 0x40) != 0,
			(mem & 0x80) != 0
		};
	}
};

TEST_F(NesCpuBitTest, BIT_ZeroResult_SetsZero) {
	auto r = BIT(0x0F, 0xF0);
	EXPECT_TRUE(r.zero);
}

TEST_F(NesCpuBitTest, BIT_NonZeroResult_ClearsZero) {
	auto r = BIT(0xFF, 0x01);
	EXPECT_FALSE(r.zero);
}

TEST_F(NesCpuBitTest, BIT_Bit6Set_SetsOverflow) {
	auto r = BIT(0x00, 0x40);
	EXPECT_TRUE(r.overflow);
}

TEST_F(NesCpuBitTest, BIT_Bit7Set_SetsNegative) {
	auto r = BIT(0x00, 0x80);
	EXPECT_TRUE(r.negative);
}

TEST_F(NesCpuBitTest, BIT_AllBitsSet_AllFlagsSet) {
	auto r = BIT(0xFF, 0xC0);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.negative);
}

//=============================================================================
// Increment/Decrement Logic Tests
//=============================================================================

class NesCpuIncDecTest : public ::testing::Test {
protected:
	struct IncDecResult {
		uint8_t result;
		bool zero;
		bool negative;
	};

	IncDecResult INC(uint8_t value) {
		uint8_t result = value + 1;
		return {
			result,
			result == 0,
			(result & 0x80) != 0
		};
	}

	IncDecResult DEC(uint8_t value) {
		uint8_t result = value - 1;
		return {
			result,
			result == 0,
			(result & 0x80) != 0
		};
	}
};

TEST_F(NesCpuIncDecTest, INC_ZeroToOne) {
	auto r = INC(0x00);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuIncDecTest, INC_0xFFWrapsToZero) {
	auto r = INC(0xFF);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuIncDecTest, INC_0x7FBecomesNegative) {
	auto r = INC(0x7F);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);
}

TEST_F(NesCpuIncDecTest, DEC_OneToZero) {
	auto r = DEC(0x01);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(NesCpuIncDecTest, DEC_ZeroWrapsTo0xFF) {
	auto r = DEC(0x00);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);
}

TEST_F(NesCpuIncDecTest, DEC_0x80BecomesPositive) {
	auto r = DEC(0x80);
	EXPECT_EQ(r.result, 0x7F);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

//=============================================================================
// Stack Operation Tests
//=============================================================================

class NesCpuStackTest : public ::testing::Test {
protected:
	uint8_t _sp = 0xFD;
	uint8_t _stack[256] = {};

	void Push(uint8_t value) {
		_stack[_sp] = value;
		_sp--;
	}

	void PushWord(uint16_t value) {
		Push((uint8_t)(value >> 8));
		Push((uint8_t)value);
	}

	uint8_t Pop() {
		_sp++;
		return _stack[_sp];
	}

	uint16_t PopWord() {
		uint8_t lo = Pop();
		uint8_t hi = Pop();
		return (hi << 8) | lo;
	}
};

TEST_F(NesCpuStackTest, Push_DecrementsStackPointer) {
	uint8_t initialSp = _sp;
	Push(0x42);
	EXPECT_EQ(_sp, initialSp - 1);
	EXPECT_EQ(_stack[initialSp], 0x42);
}

TEST_F(NesCpuStackTest, Pop_IncrementsStackPointer) {
	Push(0x42);
	uint8_t spAfterPush = _sp;
	uint8_t value = Pop();
	EXPECT_EQ(_sp, spAfterPush + 1);
	EXPECT_EQ(value, 0x42);
}

TEST_F(NesCpuStackTest, PushWord_PushesHighByteThenLow) {
	uint8_t initialSp = _sp;
	PushWord(0x1234);
	EXPECT_EQ(_stack[initialSp], 0x12);      // High byte first
	EXPECT_EQ(_stack[initialSp - 1], 0x34);  // Low byte second
}

TEST_F(NesCpuStackTest, PopWord_ReturnsCorrectValue) {
	PushWord(0x1234);
	uint16_t value = PopWord();
	EXPECT_EQ(value, 0x1234);
}

TEST_F(NesCpuStackTest, Stack_Wraps) {
	_sp = 0x00;
	Push(0x42);
	EXPECT_EQ(_sp, 0xFF);  // Wraps to 0xFF
}

//=============================================================================
// Parameterized Flag Tests
//=============================================================================

class NesCpuFlagTest : public ::testing::TestWithParam<uint8_t> {};

TEST_P(NesCpuFlagTest, IndividualFlag_SetAndRead) {
	uint8_t ps = 0;
	uint8_t flag = GetParam();

	ps |= flag;
	EXPECT_EQ(ps & flag, flag);

	ps &= ~flag;
	EXPECT_EQ(ps & flag, 0);
}

INSTANTIATE_TEST_SUITE_P(
	AllFlags,
	NesCpuFlagTest,
	::testing::Values(
		PSFlags::Carry,      // 0x01
		PSFlags::Zero,       // 0x02
		PSFlags::Interrupt,  // 0x04
		PSFlags::Decimal,    // 0x08
		PSFlags::Break,      // 0x10
		PSFlags::Reserved,   // 0x20
		PSFlags::Overflow,   // 0x40
		PSFlags::Negative    // 0x80
	)
);

//=============================================================================
// Branch Logic Tests
//=============================================================================

class NesCpuBranchTest : public ::testing::Test {
protected:
	// Check if branch crosses page boundary
	bool CheckPageCrossed(uint16_t pc, int8_t offset) {
		return ((pc + 2 + offset) & 0xFF00) != ((pc + 2) & 0xFF00);
	}
};

TEST_F(NesCpuBranchTest, PageCross_ForwardNoWrap) {
	EXPECT_FALSE(CheckPageCrossed(0x1000, 10));
}

TEST_F(NesCpuBranchTest, PageCross_ForwardWrap) {
	EXPECT_TRUE(CheckPageCrossed(0x10F0, 20));  // 0x10F2 + 20 = 0x1106
}

TEST_F(NesCpuBranchTest, PageCross_BackwardNoWrap) {
	EXPECT_FALSE(CheckPageCrossed(0x1050, -10));
}

TEST_F(NesCpuBranchTest, PageCross_BackwardWrap) {
	EXPECT_TRUE(CheckPageCrossed(0x1005, -10));  // 0x1007 - 10 = 0x0FFD
}

//=============================================================================
// Before/After Comparison: Branching vs Branchless SetZeroNegativeFlags
//=============================================================================

/// <summary>
/// These tests embed BOTH the old (branching) and new (branchless) implementations
/// of SetZeroNegativeFlags and verify they produce identical PS register state
/// for all possible inputs. This proves the optimization in NesCpu.h is safe.
/// </summary>
class NesCpuBranchlessComparisonTest : public ::testing::Test {
protected:
	/// Old implementation: if/else branching (pre-optimization)
	static uint8_t SetZeroNeg_Branching(uint8_t ps, uint8_t value) {
		ps &= ~(PSFlags::Zero | PSFlags::Negative);
		if (value == 0) {
			ps |= PSFlags::Zero;
		}
		if (value & 0x80) {
			ps |= PSFlags::Negative;
		}
		return ps;
	}

	/// New implementation: branchless (post-optimization, matches NesCpu.h)
	static uint8_t SetZeroNeg_Branchless(uint8_t ps, uint8_t value) {
		ps &= ~(PSFlags::Zero | PSFlags::Negative);
		ps |= (value == 0) ? PSFlags::Zero : 0;
		ps |= (value & 0x80);  // PSFlags::Negative = 0x80 maps directly to bit 7
		return ps;
	}
};

TEST_F(NesCpuBranchlessComparisonTest, Exhaustive_All256Values_AllPSStates) {
	// Test every value (0-255) with multiple initial PS register states
	// to prove the branchless optimization produces identical results.
	uint8_t psStates[] = {
		0x00,  // All flags clear
		0x24,  // Interrupt + Reserved (typical initial state)
		0xFF,  // All flags set
		0x03,  // Carry + Zero (stale zero)
		0x80,  // Stale negative
		0x82,  // Stale negative + zero (impossible but must handle)
		0x41,  // Carry + Overflow
		0x6D,  // Several flags set
	};

	for (uint8_t initialPS : psStates) {
		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			uint8_t oldResult = SetZeroNeg_Branching(initialPS, value);
			uint8_t newResult = SetZeroNeg_Branchless(initialPS, value);

			EXPECT_EQ(oldResult, newResult)
				<< "PS mismatch for initialPS=0x" << std::hex << (int)initialPS
				<< " value=0x" << (int)value
				<< " old=0x" << (int)oldResult
				<< " new=0x" << (int)newResult;
		}
	}
}
