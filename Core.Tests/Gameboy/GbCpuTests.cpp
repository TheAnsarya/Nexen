#include "pch.h"
#include "Gameboy/GbTypes.h"

/// <summary>
/// Test fixture for Game Boy LR35902 CPU types and state.
/// These tests verify CPU state structures and flag calculations
/// without requiring a full emulator environment.
/// </summary>
class GbCpuTypesTest : public ::testing::Test {
protected:
	GbCpuState _state = {};

	void SetUp() override {
		_state = {};
		_state.A = 0;
		_state.Flags = 0;
		_state.B = 0;
		_state.C = 0;
		_state.D = 0;
		_state.E = 0;
		_state.H = 0;
		_state.L = 0;
		_state.SP = 0xFFFE;
		_state.PC = 0x0100;  // Entry point after boot ROM
		_state.IME = false;
		_state.EiPending = false;
		_state.HaltBug = false;
		_state.Stopped = false;
	}

	// Game Boy flag manipulation
	void SetFlag(uint8_t flag) {
		_state.Flags |= flag;
	}

	void ClearFlag(uint8_t flag) {
		_state.Flags &= ~flag;
	}

	bool CheckFlag(uint8_t flag) const {
		return (_state.Flags & flag) != 0;
	}

	// Set Z and N flags based on result (typical behavior for ALU ops)
	void SetZeroFlag(uint8_t value) {
		if (value == 0) {
			SetFlag(GbCpuFlags::Zero);
		} else {
			ClearFlag(GbCpuFlags::Zero);
		}
	}
};

//=============================================================================
// CPU State Tests
//=============================================================================

TEST_F(GbCpuTypesTest, InitialState_DefaultValues) {
	GbCpuState state = {};
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.Flags, 0);
	EXPECT_EQ(state.B, 0);
	EXPECT_EQ(state.C, 0);
	EXPECT_EQ(state.D, 0);
	EXPECT_EQ(state.E, 0);
	EXPECT_EQ(state.H, 0);
	EXPECT_EQ(state.L, 0);
	EXPECT_EQ(state.PC, 0);
	EXPECT_EQ(state.SP, 0);
	EXPECT_EQ(state.CycleCount, 0);
}

TEST_F(GbCpuTypesTest, State_RegisterPairs_BC) {
	_state.B = 0x12;
	_state.C = 0x34;
	uint16_t bc = ((uint16_t)_state.B << 8) | _state.C;
	EXPECT_EQ(bc, 0x1234);
}

TEST_F(GbCpuTypesTest, State_RegisterPairs_DE) {
	_state.D = 0xAB;
	_state.E = 0xCD;
	uint16_t de = ((uint16_t)_state.D << 8) | _state.E;
	EXPECT_EQ(de, 0xABCD);
}

TEST_F(GbCpuTypesTest, State_RegisterPairs_HL) {
	_state.H = 0xFF;
	_state.L = 0x00;
	uint16_t hl = ((uint16_t)_state.H << 8) | _state.L;
	EXPECT_EQ(hl, 0xFF00);
}

TEST_F(GbCpuTypesTest, State_RegisterPairs_AF) {
	_state.A = 0x01;
	_state.Flags = 0xB0;  // Z=1, N=0, H=1, C=1, lower nibble always 0
	uint16_t af = ((uint16_t)_state.A << 8) | _state.Flags;
	EXPECT_EQ(af, 0x01B0);
}

//=============================================================================
// Flag Tests - Game Boy uses only upper 4 bits of F register
//=============================================================================

TEST_F(GbCpuTypesTest, Flags_ZeroFlag_SetAndClear) {
	ClearFlag(GbCpuFlags::Zero);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::Zero));
	SetFlag(GbCpuFlags::Zero);
	EXPECT_TRUE(CheckFlag(GbCpuFlags::Zero));
	ClearFlag(GbCpuFlags::Zero);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::Zero));
}

TEST_F(GbCpuTypesTest, Flags_AddSubFlag_SetAndClear) {
	ClearFlag(GbCpuFlags::AddSub);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::AddSub));
	SetFlag(GbCpuFlags::AddSub);
	EXPECT_TRUE(CheckFlag(GbCpuFlags::AddSub));
	ClearFlag(GbCpuFlags::AddSub);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::AddSub));
}

TEST_F(GbCpuTypesTest, Flags_HalfCarryFlag_SetAndClear) {
	ClearFlag(GbCpuFlags::HalfCarry);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::HalfCarry));
	SetFlag(GbCpuFlags::HalfCarry);
	EXPECT_TRUE(CheckFlag(GbCpuFlags::HalfCarry));
	ClearFlag(GbCpuFlags::HalfCarry);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::HalfCarry));
}

TEST_F(GbCpuTypesTest, Flags_CarryFlag_SetAndClear) {
	ClearFlag(GbCpuFlags::Carry);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::Carry));
	SetFlag(GbCpuFlags::Carry);
	EXPECT_TRUE(CheckFlag(GbCpuFlags::Carry));
	ClearFlag(GbCpuFlags::Carry);
	EXPECT_FALSE(CheckFlag(GbCpuFlags::Carry));
}

TEST_F(GbCpuTypesTest, Flags_AllFlags_CorrectPositions) {
	// Game Boy flag positions are in upper nibble
	EXPECT_EQ(GbCpuFlags::Zero, 0x80);       // Bit 7
	EXPECT_EQ(GbCpuFlags::AddSub, 0x40);     // Bit 6
	EXPECT_EQ(GbCpuFlags::HalfCarry, 0x20);  // Bit 5
	EXPECT_EQ(GbCpuFlags::Carry, 0x10);      // Bit 4
}

TEST_F(GbCpuTypesTest, Flags_LowerNibble_AlwaysZero) {
	// Lower 4 bits of F register are always 0 in actual hardware
	// For this test, we just verify the flag constants don't use lower bits
	EXPECT_EQ(GbCpuFlags::Zero & 0x0F, 0);
	EXPECT_EQ(GbCpuFlags::AddSub & 0x0F, 0);
	EXPECT_EQ(GbCpuFlags::HalfCarry & 0x0F, 0);
	EXPECT_EQ(GbCpuFlags::Carry & 0x0F, 0);
}

TEST_F(GbCpuTypesTest, Flags_MultipleFlags_SetSimultaneously) {
	_state.Flags = 0;
	SetFlag(GbCpuFlags::Zero | GbCpuFlags::Carry);
	EXPECT_TRUE(CheckFlag(GbCpuFlags::Zero));
	EXPECT_TRUE(CheckFlag(GbCpuFlags::Carry));
	EXPECT_FALSE(CheckFlag(GbCpuFlags::AddSub));
	EXPECT_FALSE(CheckFlag(GbCpuFlags::HalfCarry));
}

//=============================================================================
// IRQ Source Tests
//=============================================================================

TEST_F(GbCpuTypesTest, IrqSource_VerticalBlank) {
	EXPECT_EQ(static_cast<int>(GbIrqSource::VerticalBlank), 1);
}

TEST_F(GbCpuTypesTest, IrqSource_LcdStat) {
	EXPECT_EQ(static_cast<int>(GbIrqSource::LcdStat), 2);
}

TEST_F(GbCpuTypesTest, IrqSource_Timer) {
	EXPECT_EQ(static_cast<int>(GbIrqSource::Timer), 4);
}

TEST_F(GbCpuTypesTest, IrqSource_Serial) {
	EXPECT_EQ(static_cast<int>(GbIrqSource::Serial), 8);
}

TEST_F(GbCpuTypesTest, IrqSource_Joypad) {
	EXPECT_EQ(static_cast<int>(GbIrqSource::Joypad), 16);
}

//=============================================================================
// Arithmetic Logic Tests (8-bit)
//=============================================================================

class GbCpuArithmeticTest : public ::testing::Test {
protected:
	struct AddResult {
		uint8_t result;
		bool zero;
		bool halfCarry;
		bool carry;
	};

	// ADD A, n - flags: Z 0 H C
	AddResult Add8(uint8_t a, uint8_t b) {
		uint16_t sum = (uint16_t)a + (uint16_t)b;
		uint8_t result = (uint8_t)sum;

		bool halfCarry = ((a & 0x0F) + (b & 0x0F)) > 0x0F;
		bool carry = sum > 0xFF;
		bool zero = result == 0;

		return { result, zero, halfCarry, carry };
	}

	// ADC A, n - flags: Z 0 H C
	AddResult AddWithCarry8(uint8_t a, uint8_t b, bool carryIn) {
		uint8_t c = carryIn ? 1 : 0;
		uint16_t sum = (uint16_t)a + (uint16_t)b + c;
		uint8_t result = (uint8_t)sum;

		bool halfCarry = ((a & 0x0F) + (b & 0x0F) + c) > 0x0F;
		bool carry = sum > 0xFF;
		bool zero = result == 0;

		return { result, zero, halfCarry, carry };
	}

	// SUB A, n - flags: Z 1 H C
	AddResult Sub8(uint8_t a, uint8_t b) {
		uint8_t result = a - b;

		bool halfCarry = (a & 0x0F) < (b & 0x0F);
		bool carry = a < b;
		bool zero = result == 0;

		return { result, zero, halfCarry, carry };
	}

	// SBC A, n - flags: Z 1 H C
	AddResult SubWithBorrow8(uint8_t a, uint8_t b, bool carryIn) {
		uint8_t c = carryIn ? 1 : 0;
		uint8_t result = a - b - c;

		bool halfCarry = (a & 0x0F) < ((b & 0x0F) + c);
		bool carry = (uint16_t)a < ((uint16_t)b + c);
		bool zero = result == 0;

		return { result, zero, halfCarry, carry };
	}
};

TEST_F(GbCpuArithmeticTest, Add8_ZeroPlusZero_ReturnsZero) {
	auto r = Add8(0x00, 0x00);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuArithmeticTest, Add8_OnePlusOne_ReturnsTwo) {
	auto r = Add8(0x01, 0x01);
	EXPECT_EQ(r.result, 0x02);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuArithmeticTest, Add8_HalfCarry) {
	// 0x0F + 0x01 = 0x10, half carry occurs
	auto r = Add8(0x0F, 0x01);
	EXPECT_EQ(r.result, 0x10);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuArithmeticTest, Add8_FullCarry) {
	auto r = Add8(0xFF, 0x01);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuArithmeticTest, AddWithCarry8_CarryIn_AddsOne) {
	auto r = AddWithCarry8(0x01, 0x01, true);
	EXPECT_EQ(r.result, 0x03);
}

TEST_F(GbCpuArithmeticTest, AddWithCarry8_HalfCarryFromCarryIn) {
	// 0x0F + 0x00 + 1 = 0x10, half carry occurs
	auto r = AddWithCarry8(0x0F, 0x00, true);
	EXPECT_EQ(r.result, 0x10);
	EXPECT_TRUE(r.halfCarry);
}

TEST_F(GbCpuArithmeticTest, Sub8_ZeroMinusZero_ReturnsZero) {
	auto r = Sub8(0x00, 0x00);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuArithmeticTest, Sub8_TwoMinusOne_ReturnsOne) {
	auto r = Sub8(0x02, 0x01);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.zero);
}

TEST_F(GbCpuArithmeticTest, Sub8_HalfBorrow) {
	// 0x10 - 0x01 = 0x0F, half borrow occurs
	auto r = Sub8(0x10, 0x01);
	EXPECT_EQ(r.result, 0x0F);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuArithmeticTest, Sub8_Underflow) {
	auto r = Sub8(0x00, 0x01);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuArithmeticTest, SubWithBorrow8_BorrowIn_SubtractsOne) {
	auto r = SubWithBorrow8(0x02, 0x01, true);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
}

//=============================================================================
// 16-bit Arithmetic Tests
//=============================================================================

class GbCpu16BitArithmeticTest : public ::testing::Test {
protected:
	struct Add16Result {
		uint16_t result;
		bool halfCarry;  // Carry from bit 11 to bit 12
		bool carry;      // Carry from bit 15
	};

	// ADD HL, rr - flags: - 0 H C
	Add16Result AddHL(uint16_t hl, uint16_t rr) {
		uint32_t sum = (uint32_t)hl + (uint32_t)rr;
		uint16_t result = (uint16_t)sum;

		// Half carry is from bit 11
		bool halfCarry = ((hl & 0x0FFF) + (rr & 0x0FFF)) > 0x0FFF;
		bool carry = sum > 0xFFFF;

		return { result, halfCarry, carry };
	}

	// ADD SP, e - signed immediate
	struct AddSpResult {
		uint16_t result;
		bool halfCarry;  // From bit 3
		bool carry;      // From bit 7
	};

	AddSpResult AddSP(uint16_t sp, int8_t offset) {
		uint16_t result = sp + offset;

		// Half carry and carry are computed on lower byte
		// treating the operation as unsigned
		uint8_t lo = (uint8_t)sp;
		uint8_t off = (uint8_t)offset;

		bool halfCarry = ((lo & 0x0F) + (off & 0x0F)) > 0x0F;
		bool carry = ((uint16_t)lo + (uint16_t)off) > 0xFF;

		return { result, halfCarry, carry };
	}
};

TEST_F(GbCpu16BitArithmeticTest, AddHL_ZeroPlusZero) {
	auto r = AddHL(0x0000, 0x0000);
	EXPECT_EQ(r.result, 0x0000);
	EXPECT_FALSE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpu16BitArithmeticTest, AddHL_HalfCarry) {
	// 0x0FFF + 0x0001 = 0x1000, half carry from bit 11
	auto r = AddHL(0x0FFF, 0x0001);
	EXPECT_EQ(r.result, 0x1000);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpu16BitArithmeticTest, AddHL_FullCarry) {
	auto r = AddHL(0xFFFF, 0x0001);
	EXPECT_EQ(r.result, 0x0000);
	EXPECT_TRUE(r.halfCarry);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpu16BitArithmeticTest, AddSP_PositiveOffset) {
	auto r = AddSP(0xFF00, 5);
	EXPECT_EQ(r.result, 0xFF05);
}

TEST_F(GbCpu16BitArithmeticTest, AddSP_NegativeOffset) {
	auto r = AddSP(0xFF05, -5);
	EXPECT_EQ(r.result, 0xFF00);
}

TEST_F(GbCpu16BitArithmeticTest, AddSP_HalfCarry) {
	// SP = 0xFF0F, offset = 1 -> half carry
	auto r = AddSP(0xFF0F, 1);
	EXPECT_EQ(r.result, 0xFF10);
	EXPECT_TRUE(r.halfCarry);
}

//=============================================================================
// Shift/Rotate Logic Tests (Z80-style)
//=============================================================================

class GbCpuShiftTest : public ::testing::Test {
protected:
	struct ShiftResult {
		uint8_t result;
		bool zero;
		bool carry;
	};

	// RLC - Rotate Left through Carry (bit 7 -> carry and bit 0)
	ShiftResult RLC(uint8_t value) {
		bool bit7 = (value & 0x80) != 0;
		uint8_t result = (value << 1) | (bit7 ? 1 : 0);
		return { result, result == 0, bit7 };
	}

	// RL - Rotate Left (old carry -> bit 0, bit 7 -> new carry)
	ShiftResult RL(uint8_t value, bool carryIn) {
		bool bit7 = (value & 0x80) != 0;
		uint8_t result = (value << 1) | (carryIn ? 1 : 0);
		return { result, result == 0, bit7 };
	}

	// RRC - Rotate Right through Carry (bit 0 -> carry and bit 7)
	ShiftResult RRC(uint8_t value) {
		bool bit0 = (value & 0x01) != 0;
		uint8_t result = (value >> 1) | (bit0 ? 0x80 : 0);
		return { result, result == 0, bit0 };
	}

	// RR - Rotate Right (old carry -> bit 7, bit 0 -> new carry)
	ShiftResult RR(uint8_t value, bool carryIn) {
		bool bit0 = (value & 0x01) != 0;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0);
		return { result, result == 0, bit0 };
	}

	// SLA - Shift Left Arithmetic (bit 0 = 0)
	ShiftResult SLA(uint8_t value) {
		bool bit7 = (value & 0x80) != 0;
		uint8_t result = value << 1;
		return { result, result == 0, bit7 };
	}

	// SRA - Shift Right Arithmetic (bit 7 preserved)
	ShiftResult SRA(uint8_t value) {
		bool bit0 = (value & 0x01) != 0;
		bool bit7 = (value & 0x80) != 0;
		uint8_t result = (value >> 1) | (bit7 ? 0x80 : 0);
		return { result, result == 0, bit0 };
	}

	// SRL - Shift Right Logical (bit 7 = 0)
	ShiftResult SRL(uint8_t value) {
		bool bit0 = (value & 0x01) != 0;
		uint8_t result = value >> 1;
		return { result, result == 0, bit0 };
	}

	// SWAP - Swap nibbles
	ShiftResult SWAP(uint8_t value) {
		uint8_t result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
		return { result, result == 0, false };
	}
};

TEST_F(GbCpuShiftTest, RLC_RotatesLeft) {
	auto r = RLC(0x80);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuShiftTest, RLC_Zero_SetsZeroFlag) {
	auto r = RLC(0x00);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, RL_WithCarryIn) {
	auto r = RL(0x00, true);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, RL_SetsCarryFromBit7) {
	auto r = RL(0x80, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuShiftTest, RRC_RotatesRight) {
	auto r = RRC(0x01);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuShiftTest, RR_WithCarryIn) {
	auto r = RR(0x00, true);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SLA_ShiftsLeft) {
	auto r = SLA(0x40);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SLA_SetsCarry) {
	auto r = SLA(0x80);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuShiftTest, SRA_PreservesSign) {
	auto r = SRA(0x80);
	EXPECT_EQ(r.result, 0xC0);  // Sign bit preserved
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SRA_ShiftsRight) {
	auto r = SRA(0x02);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SRL_ShiftsRight_ClearsMSB) {
	auto r = SRL(0x80);
	EXPECT_EQ(r.result, 0x40);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SRL_SetsCarry) {
	auto r = SRL(0x01);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuShiftTest, SWAP_SwapsNibbles) {
	auto r = SWAP(0x12);
	EXPECT_EQ(r.result, 0x21);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuShiftTest, SWAP_ZeroValue) {
	auto r = SWAP(0x00);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
}

//=============================================================================
// BIT/SET/RES Tests (CB-prefixed instructions)
//=============================================================================

class GbCpuBitTest : public ::testing::Test {
protected:
	struct BitResult {
		bool zero;  // Z flag = complement of tested bit
	};

	BitResult BIT(uint8_t value, uint8_t bit) {
		return { (value & (1 << bit)) == 0 };
	}

	uint8_t SET(uint8_t value, uint8_t bit) {
		return value | (1 << bit);
	}

	uint8_t RES(uint8_t value, uint8_t bit) {
		return value & ~(1 << bit);
	}
};

TEST_F(GbCpuBitTest, BIT_Bit0_Set_ClearsZero) {
	auto r = BIT(0x01, 0);
	EXPECT_FALSE(r.zero);
}

TEST_F(GbCpuBitTest, BIT_Bit0_Clear_SetsZero) {
	auto r = BIT(0x00, 0);
	EXPECT_TRUE(r.zero);
}

TEST_F(GbCpuBitTest, BIT_Bit7_Set_ClearsZero) {
	auto r = BIT(0x80, 7);
	EXPECT_FALSE(r.zero);
}

TEST_F(GbCpuBitTest, BIT_Bit7_Clear_SetsZero) {
	auto r = BIT(0x7F, 7);
	EXPECT_TRUE(r.zero);
}

TEST_F(GbCpuBitTest, SET_Bit0) {
	uint8_t result = SET(0x00, 0);
	EXPECT_EQ(result, 0x01);
}

TEST_F(GbCpuBitTest, SET_Bit7) {
	uint8_t result = SET(0x00, 7);
	EXPECT_EQ(result, 0x80);
}

TEST_F(GbCpuBitTest, SET_AlreadySet) {
	uint8_t result = SET(0xFF, 4);
	EXPECT_EQ(result, 0xFF);
}

TEST_F(GbCpuBitTest, RES_Bit0) {
	uint8_t result = RES(0xFF, 0);
	EXPECT_EQ(result, 0xFE);
}

TEST_F(GbCpuBitTest, RES_Bit7) {
	uint8_t result = RES(0xFF, 7);
	EXPECT_EQ(result, 0x7F);
}

TEST_F(GbCpuBitTest, RES_AlreadyClear) {
	uint8_t result = RES(0x00, 4);
	EXPECT_EQ(result, 0x00);
}

//=============================================================================
// Compare Logic Tests
//=============================================================================

class GbCpuCompareTest : public ::testing::Test {
protected:
	struct CompareResult {
		bool zero;
		bool halfCarry;
		bool carry;
	};

	// CP A, n - same as SUB but result discarded
	CompareResult Compare(uint8_t a, uint8_t n) {
		uint8_t result = a - n;
		return {
			result == 0,
			(a & 0x0F) < (n & 0x0F),
			a < n
		};
	}
};

TEST_F(GbCpuCompareTest, Compare_Equal_SetsZero) {
	auto r = Compare(0x42, 0x42);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.halfCarry);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuCompareTest, Compare_Greater_ClearsCarry) {
	auto r = Compare(0x50, 0x40);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuCompareTest, Compare_Less_SetsCarry) {
	auto r = Compare(0x40, 0x50);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuCompareTest, Compare_HalfBorrow) {
	auto r = Compare(0x10, 0x01);
	EXPECT_TRUE(r.halfCarry);
}

//=============================================================================
// Bitwise Logic Tests
//=============================================================================

class GbCpuBitwiseTest : public ::testing::Test {
protected:
	struct LogicResult {
		uint8_t result;
		bool zero;
		bool halfCarry;  // H flag set for AND, cleared for OR/XOR
	};

	LogicResult AND(uint8_t a, uint8_t b) {
		uint8_t result = a & b;
		return { result, result == 0, true };  // H=1 for AND
	}

	LogicResult OR(uint8_t a, uint8_t b) {
		uint8_t result = a | b;
		return { result, result == 0, false };  // H=0 for OR
	}

	LogicResult XOR(uint8_t a, uint8_t b) {
		uint8_t result = a ^ b;
		return { result, result == 0, false };  // H=0 for XOR
	}
};

TEST_F(GbCpuBitwiseTest, AND_MasksCorrectly) {
	auto r = AND(0xF0, 0x0F);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.halfCarry);
}

TEST_F(GbCpuBitwiseTest, AND_PreservesBits) {
	auto r = AND(0xFF, 0x0F);
	EXPECT_EQ(r.result, 0x0F);
}

TEST_F(GbCpuBitwiseTest, OR_CombinesBits) {
	auto r = OR(0xF0, 0x0F);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.halfCarry);
}

TEST_F(GbCpuBitwiseTest, XOR_FlipsBits) {
	auto r = XOR(0xFF, 0xF0);
	EXPECT_EQ(r.result, 0x0F);
}

TEST_F(GbCpuBitwiseTest, XOR_SameValue_ReturnsZero) {
	auto r = XOR(0x42, 0x42);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
}

//=============================================================================
// Increment/Decrement Tests
//=============================================================================

class GbCpuIncDecTest : public ::testing::Test {
protected:
	struct IncDecResult {
		uint8_t result;
		bool zero;
		bool halfCarry;
	};

	// INC r - flags: Z 0 H -
	IncDecResult INC(uint8_t value) {
		uint8_t result = value + 1;
		return {
			result,
			result == 0,
			(value & 0x0F) == 0x0F  // Half carry when lower nibble overflows
		};
	}

	// DEC r - flags: Z 1 H -
	IncDecResult DEC(uint8_t value) {
		uint8_t result = value - 1;
		return {
			result,
			result == 0,
			(value & 0x0F) == 0x00  // Half borrow when lower nibble underflows
		};
	}
};

TEST_F(GbCpuIncDecTest, INC_ZeroToOne) {
	auto r = INC(0x00);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.halfCarry);
}

TEST_F(GbCpuIncDecTest, INC_0xFFWrapsToZero) {
	auto r = INC(0xFF);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_TRUE(r.halfCarry);
}

TEST_F(GbCpuIncDecTest, INC_HalfCarry) {
	auto r = INC(0x0F);
	EXPECT_EQ(r.result, 0x10);
	EXPECT_TRUE(r.halfCarry);
}

TEST_F(GbCpuIncDecTest, DEC_OneToZero) {
	auto r = DEC(0x01);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.halfCarry);
}

TEST_F(GbCpuIncDecTest, DEC_ZeroWrapsTo0xFF) {
	auto r = DEC(0x00);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.halfCarry);
}

TEST_F(GbCpuIncDecTest, DEC_HalfBorrow) {
	auto r = DEC(0x10);
	EXPECT_EQ(r.result, 0x0F);
	EXPECT_TRUE(r.halfCarry);
}

//=============================================================================
// DAA (Decimal Adjust Accumulator) Tests
//=============================================================================

class GbCpuDaaTest : public ::testing::Test {
protected:
	struct DaaResult {
		uint8_t result;
		bool carry;
	};

	// DAA - Decimal adjust after addition or subtraction
	DaaResult DAA(uint8_t a, bool addSub, bool halfCarry, bool carry) {
		uint8_t result = a;
		uint8_t adjustment = 0;
		bool newCarry = carry;

		if (!addSub) {
			// After addition
			if (halfCarry || (result & 0x0F) > 9) {
				adjustment |= 0x06;
			}
			if (carry || result > 0x99) {
				adjustment |= 0x60;
				newCarry = true;
			}
			result += adjustment;
		} else {
			// After subtraction
			if (halfCarry) {
				result -= 0x06;
			}
			if (carry) {
				result -= 0x60;
			}
		}

		return { result, newCarry };
	}
};

TEST_F(GbCpuDaaTest, DAA_AfterAddition_NoAdjustment) {
	// 0x12 is valid BCD
	auto r = DAA(0x12, false, false, false);
	EXPECT_EQ(r.result, 0x12);
	EXPECT_FALSE(r.carry);
}

TEST_F(GbCpuDaaTest, DAA_AfterAddition_LowerNibbleAdjust) {
	// 0x0A needs adjustment -> 0x10
	auto r = DAA(0x0A, false, false, false);
	EXPECT_EQ(r.result, 0x10);
}

TEST_F(GbCpuDaaTest, DAA_AfterAddition_UpperNibbleAdjust) {
	// 0xA0 needs adjustment -> 0x00 with carry
	auto r = DAA(0xA0, false, false, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
}

TEST_F(GbCpuDaaTest, DAA_AfterSubtraction_WithHalfCarry) {
	// After subtraction with half borrow
	auto r = DAA(0x0F, true, true, false);
	EXPECT_EQ(r.result, 0x09);
}

//=============================================================================
// Stack Operation Tests
//=============================================================================

class GbCpuStackTest : public ::testing::Test {
protected:
	uint16_t _sp = 0xFFFE;
	uint8_t _memory[0x10000] = {};

	void Push8(uint8_t value) {
		_sp--;
		_memory[_sp] = value;
	}

	void Push16(uint16_t value) {
		Push8((uint8_t)(value >> 8));
		Push8((uint8_t)value);
	}

	uint8_t Pop8() {
		return _memory[_sp++];
	}

	uint16_t Pop16() {
		uint8_t lo = Pop8();
		uint8_t hi = Pop8();
		return ((uint16_t)hi << 8) | lo;
	}
};

TEST_F(GbCpuStackTest, Push8_DecrementsStackPointer) {
	uint16_t initialSp = _sp;
	Push8(0x42);
	EXPECT_EQ(_sp, initialSp - 1);
	EXPECT_EQ(_memory[_sp], 0x42);
}

TEST_F(GbCpuStackTest, Pop8_IncrementsStackPointer) {
	Push8(0x42);
	uint16_t spAfterPush = _sp;
	uint8_t value = Pop8();
	EXPECT_EQ(_sp, spAfterPush + 1);
	EXPECT_EQ(value, 0x42);
}

TEST_F(GbCpuStackTest, Push16_PushesHighByteThenLow) {
	uint16_t initialSp = _sp;
	Push16(0x1234);
	// SP decremented by 2
	EXPECT_EQ(_sp, initialSp - 2);
	// High byte at higher address (pushed first)
	EXPECT_EQ(_memory[initialSp - 1], 0x12);
	// Low byte at lower address (pushed second)
	EXPECT_EQ(_memory[initialSp - 2], 0x34);
}

TEST_F(GbCpuStackTest, Pop16_ReturnsCorrectValue) {
	Push16(0x1234);
	uint16_t value = Pop16();
	EXPECT_EQ(value, 0x1234);
}

TEST_F(GbCpuStackTest, Stack_RoundTrip) {
	uint16_t original = 0xABCD;
	Push16(original);
	uint16_t retrieved = Pop16();
	EXPECT_EQ(retrieved, original);
}

//=============================================================================
// Parameterized Flag Tests
//=============================================================================

class GbCpuFlagTest : public ::testing::TestWithParam<uint8_t> {};

TEST_P(GbCpuFlagTest, IndividualFlag_SetAndRead) {
	uint8_t flags = 0;
	uint8_t flag = GetParam();

	flags |= flag;
	EXPECT_EQ(flags & flag, flag);

	flags &= ~flag;
	EXPECT_EQ(flags & flag, 0);
}

INSTANTIATE_TEST_SUITE_P(
	AllFlags,
	GbCpuFlagTest,
	::testing::Values(
		GbCpuFlags::Zero,       // 0x80
		GbCpuFlags::AddSub,     // 0x40
		GbCpuFlags::HalfCarry,  // 0x20
		GbCpuFlags::Carry       // 0x10
	)
);

//=============================================================================
// Interrupt Tests
//=============================================================================

class GbCpuInterruptTest : public ::testing::Test {
protected:
	uint8_t _ie = 0;  // Interrupt Enable (0xFFFF)
	uint8_t _if = 0;  // Interrupt Flag (0xFF0F)
	bool _ime = false;

	bool InterruptPending() const {
		return (_ie & _if & 0x1F) != 0;
	}

	uint8_t GetHighestPriorityInterrupt() const {
		uint8_t pending = _ie & _if & 0x1F;
		if (pending & 0x01) return 0x01;  // VBlank
		if (pending & 0x02) return 0x02;  // LCD STAT
		if (pending & 0x04) return 0x04;  // Timer
		if (pending & 0x08) return 0x08;  // Serial
		if (pending & 0x10) return 0x10;  // Joypad
		return 0;
	}

	uint16_t GetInterruptVector(uint8_t interrupt) const {
		switch (interrupt) {
			case 0x01: return 0x0040;  // VBlank
			case 0x02: return 0x0048;  // LCD STAT
			case 0x04: return 0x0050;  // Timer
			case 0x08: return 0x0058;  // Serial
			case 0x10: return 0x0060;  // Joypad
			default: return 0;
		}
	}
};

TEST_F(GbCpuInterruptTest, NoPending_WhenNoneEnabled) {
	_if = 0x1F;  // All requested
	_ie = 0x00;  // None enabled
	EXPECT_FALSE(InterruptPending());
}

TEST_F(GbCpuInterruptTest, NoPending_WhenNoneRequested) {
	_if = 0x00;  // None requested
	_ie = 0x1F;  // All enabled
	EXPECT_FALSE(InterruptPending());
}

TEST_F(GbCpuInterruptTest, Pending_WhenEnabledAndRequested) {
	_if = 0x01;  // VBlank requested
	_ie = 0x01;  // VBlank enabled
	EXPECT_TRUE(InterruptPending());
}

TEST_F(GbCpuInterruptTest, Priority_VBlankHighest) {
	_if = 0x1F;  // All requested
	_ie = 0x1F;  // All enabled
	EXPECT_EQ(GetHighestPriorityInterrupt(), 0x01);
}

TEST_F(GbCpuInterruptTest, Priority_TimerWhenVBlankDisabled) {
	_if = 0x1F;  // All requested
	_ie = 0x1C;  // VBlank and LCD disabled
	EXPECT_EQ(GetHighestPriorityInterrupt(), 0x04);  // Timer
}

TEST_F(GbCpuInterruptTest, Vectors_CorrectAddresses) {
	EXPECT_EQ(GetInterruptVector(0x01), 0x0040);  // VBlank
	EXPECT_EQ(GetInterruptVector(0x02), 0x0048);  // LCD STAT
	EXPECT_EQ(GetInterruptVector(0x04), 0x0050);  // Timer
	EXPECT_EQ(GetInterruptVector(0x08), 0x0058);  // Serial
	EXPECT_EQ(GetInterruptVector(0x10), 0x0060);  // Joypad
}

//=============================================================================
// SetFlagState Branchless Comparison Tests
//=============================================================================
// Exhaustive comparison between old branching SetFlagState and new branchless
// to verify identical behavior for every possible input combination.

class GbCpuSetFlagStateComparisonTest : public ::testing::Test {
protected:
	// Old branching implementation (original code)
	static uint8_t SetFlagState_Branching(uint8_t flags, uint8_t flag, bool state) {
		if (state) {
			flags |= flag;
		} else {
			flags &= ~flag;
		}
		return flags;
	}

	// New branchless implementation (matches GbCpu.cpp)
	static uint8_t SetFlagState_Branchless(uint8_t flags, uint8_t flag, bool state) {
		return (flags & ~flag) | (-static_cast<uint8_t>(state) & flag);
	}
};

TEST_F(GbCpuSetFlagStateComparisonTest, Exhaustive_AllFlags_AllInputs) {
	// Test all 4 GB CPU flags
	constexpr uint8_t flags[] = {
		GbCpuFlags::Zero,      // 0x80
		GbCpuFlags::AddSub,    // 0x40
		GbCpuFlags::HalfCarry, // 0x20
		GbCpuFlags::Carry      // 0x10
	};

	// Test every possible initial Flags register value (256) x each flag x set/clear
	for (uint8_t flag : flags) {
		for (int initial = 0; initial < 256; initial++) {
			uint8_t initFlags = static_cast<uint8_t>(initial);

			// Test setting the flag (state = true)
			uint8_t branching_set = SetFlagState_Branching(initFlags, flag, true);
			uint8_t branchless_set = SetFlagState_Branchless(initFlags, flag, true);
			ASSERT_EQ(branching_set, branchless_set)
				<< "Mismatch setting flag 0x" << std::hex << (int)flag
				<< " with initial Flags=0x" << (int)initFlags;

			// Test clearing the flag (state = false)
			uint8_t branching_clear = SetFlagState_Branching(initFlags, flag, false);
			uint8_t branchless_clear = SetFlagState_Branchless(initFlags, flag, false);
			ASSERT_EQ(branching_clear, branchless_clear)
				<< "Mismatch clearing flag 0x" << std::hex << (int)flag
				<< " with initial Flags=0x" << (int)initFlags;
		}
	}
}

TEST_F(GbCpuSetFlagStateComparisonTest, MultiFlag_ALU_Pattern) {
	// Test the typical ALU pattern: setting Zero, HalfCarry, Carry in sequence
	// This mimics how ADD, SUB, CP etc. call SetFlagState multiple times
	for (int initial = 0; initial < 256; initial++) {
		for (int value = 0; value < 256; value++) {
			uint8_t initFlags = static_cast<uint8_t>(initial);
			uint8_t val = static_cast<uint8_t>(value);

			// Simulate ADD-like flag computation
			bool zeroResult = (val == 0);
			bool halfCarry = ((val & 0x0F) > 0x09);
			bool carry = (val > 0x7F);

			// Apply branching version
			uint8_t branching = initFlags;
			branching = SetFlagState_Branching(branching, GbCpuFlags::Zero, zeroResult);
			branching = SetFlagState_Branching(branching, GbCpuFlags::HalfCarry, halfCarry);
			branching = SetFlagState_Branching(branching, GbCpuFlags::Carry, carry);

			// Apply branchless version
			uint8_t branchless = initFlags;
			branchless = SetFlagState_Branchless(branchless, GbCpuFlags::Zero, zeroResult);
			branchless = SetFlagState_Branchless(branchless, GbCpuFlags::HalfCarry, halfCarry);
			branchless = SetFlagState_Branchless(branchless, GbCpuFlags::Carry, carry);

			ASSERT_EQ(branching, branchless)
				<< "Multi-flag mismatch: initial=0x" << std::hex << (int)initFlags
				<< " value=0x" << (int)val;
		}
	}
}
