#include "pch.h"
#include "PCE/PceTypes.h"

//=============================================================================
// PCE CPU Flag Operations
//=============================================================================

/// <summary>
/// Tests for PCE HuC6280 CPU flag operations.
/// The PCE CPU uses the same 6502-family flag layout as the NES.
/// </summary>
class PceCpuTypesTest : public ::testing::Test {
protected:
	PceCpuState _state{};

	void SetUp() override {
		_state = {};
		_state.PS = 0;
	}

	void ClearFlags(uint8_t flags) { _state.PS &= ~flags; }
	void SetFlags(uint8_t flags) { _state.PS |= flags; }
	bool CheckFlag(uint8_t flag) { return (_state.PS & flag) == flag; }
};

TEST_F(PceCpuTypesTest, InitialState_AllClear) {
	EXPECT_EQ(_state.PS, 0);
	EXPECT_EQ(_state.A, 0);
	EXPECT_EQ(_state.X, 0);
	EXPECT_EQ(_state.Y, 0);
	EXPECT_EQ(_state.SP, 0);
	EXPECT_EQ(_state.PC, 0);
}

TEST_F(PceCpuTypesTest, FlagLayout_MatchesMos6502) {
	EXPECT_EQ(PceCpuFlags::Carry, 0x01);
	EXPECT_EQ(PceCpuFlags::Zero, 0x02);
	EXPECT_EQ(PceCpuFlags::Interrupt, 0x04);
	EXPECT_EQ(PceCpuFlags::Decimal, 0x08);
	EXPECT_EQ(PceCpuFlags::Break, 0x10);
	EXPECT_EQ(PceCpuFlags::Memory, 0x20);
	EXPECT_EQ(PceCpuFlags::Overflow, 0x40);
	EXPECT_EQ(PceCpuFlags::Negative, 0x80);
}

TEST_F(PceCpuTypesTest, SetFlag_SetsOnlyTargetFlag) {
	SetFlags(PceCpuFlags::Zero);
	EXPECT_TRUE(CheckFlag(PceCpuFlags::Zero));
	EXPECT_FALSE(CheckFlag(PceCpuFlags::Carry));
	EXPECT_FALSE(CheckFlag(PceCpuFlags::Negative));
}

TEST_F(PceCpuTypesTest, ClearFlag_ClearsOnlyTargetFlag) {
	_state.PS = 0xFF;
	ClearFlags(PceCpuFlags::Carry);
	EXPECT_FALSE(CheckFlag(PceCpuFlags::Carry));
	EXPECT_TRUE(CheckFlag(PceCpuFlags::Zero));
	EXPECT_TRUE(CheckFlag(PceCpuFlags::Negative));
}

//=============================================================================
// Before/After Comparison: Branching vs Branchless SetZeroNegativeFlags
//=============================================================================

/// <summary>
/// PCE SetZeroNegativeFlags comparison tests.
/// The callers always clear Zero|Negative before calling, so
/// SetZeroNegativeFlags only needs to SET flags (never clear).
/// Both old and new implementations are additive-only (OR-based).
/// PCE Negative = 0x80 maps directly to bit 7 of value.
/// </summary>
class PceCpuBranchlessComparisonTest : public ::testing::Test {
protected:
	/// Old implementation: branching if/else if
	static uint8_t SetZeroNeg_Branching(uint8_t ps, uint8_t value) {
		// Caller has already cleared Zero|Negative
		if (value == 0) {
			ps |= PceCpuFlags::Zero;
		} else if (value & 0x80) {
			ps |= PceCpuFlags::Negative;
		}
		return ps;
	}

	/// New implementation: branchless (matches current PceCpu.cpp)
	static uint8_t SetZeroNeg_Branchless(uint8_t ps, uint8_t value) {
		// Caller has already cleared Zero|Negative
		ps |= (value == 0) ? PceCpuFlags::Zero : 0;
		ps |= (value & PceCpuFlags::Negative);
		return ps;
	}
};

TEST_F(PceCpuBranchlessComparisonTest, Exhaustive_All256Values_AllPSStates) {
	// Test every value (0-255) with multiple initial PS register states.
	// Note: callers always clear Zero|Negative before calling, so we
	// test with those flags already cleared in initial PS.
	uint8_t psStates[] = {
		0x00,                                           // All flags clear
		PceCpuFlags::Carry,                             // Only carry
		PceCpuFlags::Interrupt | PceCpuFlags::Decimal,  // I+D
		PceCpuFlags::Carry | PceCpuFlags::Overflow,     // C+V
		0x6D,                                           // Several flags
	};

	for (uint8_t initialPS : psStates) {
		// Simulate what callers do: clear Zero|Negative first
		uint8_t clearedPS = initialPS & ~(PceCpuFlags::Zero | PceCpuFlags::Negative);

		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			uint8_t oldResult = SetZeroNeg_Branching(clearedPS, value);
			uint8_t newResult = SetZeroNeg_Branchless(clearedPS, value);

			EXPECT_EQ(oldResult, newResult)
				<< "PS mismatch for initialPS=0x" << std::hex << (int)initialPS
				<< " (cleared=0x" << (int)clearedPS << ")"
				<< " value=0x" << (int)value
				<< " old=0x" << (int)oldResult
				<< " new=0x" << (int)newResult;
		}
	}
}

TEST_F(PceCpuBranchlessComparisonTest, ZeroValue_SetsZeroFlag) {
	uint8_t ps = 0x00;
	uint8_t result = SetZeroNeg_Branchless(ps, 0x00);
	EXPECT_TRUE(result & PceCpuFlags::Zero);
	EXPECT_FALSE(result & PceCpuFlags::Negative);
}

TEST_F(PceCpuBranchlessComparisonTest, NegativeValue_SetsNegativeFlag) {
	uint8_t ps = 0x00;
	uint8_t result = SetZeroNeg_Branchless(ps, 0x80);
	EXPECT_FALSE(result & PceCpuFlags::Zero);
	EXPECT_TRUE(result & PceCpuFlags::Negative);
}

TEST_F(PceCpuBranchlessComparisonTest, PositiveNonZero_SetsNoFlags) {
	uint8_t ps = 0x00;
	uint8_t result = SetZeroNeg_Branchless(ps, 0x42);
	EXPECT_FALSE(result & PceCpuFlags::Zero);
	EXPECT_FALSE(result & PceCpuFlags::Negative);
}

TEST_F(PceCpuBranchlessComparisonTest, PreservesOtherFlags) {
	uint8_t ps = PceCpuFlags::Carry | PceCpuFlags::Overflow;
	uint8_t result = SetZeroNeg_Branchless(ps, 0x42);
	EXPECT_TRUE(result & PceCpuFlags::Carry);
	EXPECT_TRUE(result & PceCpuFlags::Overflow);
}

//=============================================================================
// Before/After Comparison: PCE CMP (Compare) Instruction
//=============================================================================

/// <summary>
/// PCE CMP comparison tests (identical logic to NES CMP, same ISA family).
/// Currently still branching in PceCpu.Instructions.cpp — these tests
/// validate correctness and serve as baseline for future branchless conversion.
/// </summary>
class PceCpuCmpComparisonTest : public ::testing::Test {
protected:
	// Current branching CMP (matches PceCpu.Instructions.cpp)
	static uint8_t CMP_Branching(uint8_t ps, uint8_t reg, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		auto result = reg - value;
		if (reg >= value) ps |= PceCpuFlags::Carry;
		if (reg == value) ps |= PceCpuFlags::Zero;
		if ((result & 0x80) == 0x80) ps |= PceCpuFlags::Negative;
		return ps;
	}

	// Future branchless CMP (bit-shift pattern, matches current PceCpu.Instructions.cpp)
	static uint8_t CMP_Branchless(uint8_t ps, uint8_t reg, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		auto result = reg - value;
		// Carry=0x01 maps directly to bool(reg >= value)
		ps |= static_cast<uint8_t>(reg >= value);
		// Zero=0x02, shift bool left by 1
		ps |= static_cast<uint8_t>(reg == value) << 1;
		// Negative=0x80 maps directly to result bit 7
		ps |= (result & PceCpuFlags::Negative);
		return ps;
	}
};

TEST_F(PceCpuCmpComparisonTest, Exhaustive_All256x256) {
	for (int r = 0; r < 256; r++) {
		for (int v = 0; v < 256; v++) {
			uint8_t reg = static_cast<uint8_t>(r);
			uint8_t val = static_cast<uint8_t>(v);
			uint8_t oldPS = CMP_Branching(0x00, reg, val);
			uint8_t newPS = CMP_Branchless(0x00, reg, val);
			ASSERT_EQ(oldPS, newPS)
				<< "CMP mismatch: reg=0x" << std::hex << (int)reg
				<< " value=0x" << (int)val;
		}
	}
}

TEST_F(PceCpuCmpComparisonTest, Equal_SetsZeroAndCarry) {
	uint8_t ps = CMP_Branchless(0x00, 0x42, 0x42);
	EXPECT_TRUE(ps & PceCpuFlags::Zero);
	EXPECT_TRUE(ps & PceCpuFlags::Carry);
	EXPECT_FALSE(ps & PceCpuFlags::Negative);
}

TEST_F(PceCpuCmpComparisonTest, RegGreater_SetsCarry) {
	uint8_t ps = CMP_Branchless(0x00, 0x50, 0x30);
	EXPECT_FALSE(ps & PceCpuFlags::Zero);
	EXPECT_TRUE(ps & PceCpuFlags::Carry);
}

TEST_F(PceCpuCmpComparisonTest, RegSmaller_SetsNegative) {
	uint8_t ps = CMP_Branchless(0x00, 0x30, 0x50);
	EXPECT_FALSE(ps & PceCpuFlags::Zero);
	EXPECT_FALSE(ps & PceCpuFlags::Carry);
	EXPECT_TRUE(ps & PceCpuFlags::Negative);  // 0x30-0x50 = 0xE0 (bit 7 set)
}

//=============================================================================
// Before/After Comparison: PCE ASL/LSR/ROL/ROR Shift Operations
//=============================================================================

/// <summary>
/// PCE shift comparison tests. These instructions clear C+Z+N before calling
/// SetZeroNegativeFlags, so the branchless path is exercised through the
/// Z/N flag setting. Tests validate the full instruction behavior.
/// </summary>
class PceCpuShiftComparisonTest : public ::testing::Test {
protected:
	struct ShiftResult {
		uint8_t result;
		uint8_t ps;
	};

	// Branching reference implementations
	static ShiftResult ASL_Branching(uint8_t ps, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x80) ps |= PceCpuFlags::Carry;
		uint8_t result = value << 1;
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult LSR_Branching(uint8_t ps, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x01) ps |= PceCpuFlags::Carry;
		uint8_t result = value >> 1;
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult ROL_Branching(uint8_t ps, uint8_t value) {
		bool carryIn = (ps & PceCpuFlags::Carry) != 0;
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x80) ps |= PceCpuFlags::Carry;
		uint8_t result = (value << 1) | (carryIn ? 0x01 : 0x00);
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult ROR_Branching(uint8_t ps, uint8_t value) {
		bool carryIn = (ps & PceCpuFlags::Carry) != 0;
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		if (value & 0x01) ps |= PceCpuFlags::Carry;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0x00);
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	// Branchless implementations (match current PceCpu.Instructions.cpp)
	static ShiftResult ASL_Branchless(uint8_t ps, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		// Branchless: bit 7 → Carry (bit 0)
		ps |= (value >> 7);
		uint8_t result = value << 1;
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult LSR_Branchless(uint8_t ps, uint8_t value) {
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		// Branchless: bit 0 maps directly to Carry (bit 0)
		ps |= (value & PceCpuFlags::Carry);
		uint8_t result = value >> 1;
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult ROL_Branchless(uint8_t ps, uint8_t value) {
		bool carryIn = (ps & PceCpuFlags::Carry) != 0;
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		// Branchless: bit 7 → Carry (bit 0)
		ps |= (value >> 7);
		uint8_t result = (value << 1) | static_cast<uint8_t>(carryIn);
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}

	static ShiftResult ROR_Branchless(uint8_t ps, uint8_t value) {
		bool carryIn = (ps & PceCpuFlags::Carry) != 0;
		ps &= ~(PceCpuFlags::Carry | PceCpuFlags::Negative | PceCpuFlags::Zero);
		// Branchless: bit 0 maps directly to Carry (bit 0)
		ps |= (value & PceCpuFlags::Carry);
		uint8_t result = (value >> 1) | (static_cast<uint8_t>(carryIn) << 7);
		ps |= (result == 0) ? PceCpuFlags::Zero : 0;
		ps |= (result & PceCpuFlags::Negative);
		return { result, ps };
	}
};

TEST_F(PceCpuShiftComparisonTest, ASL_Exhaustive256) {
	for (int v = 0; v <= 255; v++) {
		uint8_t value = static_cast<uint8_t>(v);
		auto ref = ASL_Branching(0x00, value);
		auto branchless = ASL_Branchless(0x00, value);
		EXPECT_EQ(ref.result, branchless.result)
			<< "ASL result mismatch for value=0x" << std::hex << v;
		EXPECT_EQ(ref.ps, branchless.ps)
			<< "ASL flags mismatch for value=0x" << std::hex << v;
	}
}

TEST_F(PceCpuShiftComparisonTest, LSR_Exhaustive256) {
	for (int v = 0; v <= 255; v++) {
		uint8_t value = static_cast<uint8_t>(v);
		auto ref = LSR_Branching(0x00, value);
		auto branchless = LSR_Branchless(0x00, value);
		EXPECT_EQ(ref.result, branchless.result)
			<< "LSR result mismatch for value=0x" << std::hex << v;
		EXPECT_EQ(ref.ps, branchless.ps)
			<< "LSR flags mismatch for value=0x" << std::hex << v;
	}
}

TEST_F(PceCpuShiftComparisonTest, ROL_Exhaustive256x2) {
	for (int carryIn = 0; carryIn <= 1; carryIn++) {
		uint8_t initialPS = carryIn ? PceCpuFlags::Carry : 0;
		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			auto ref = ROL_Branching(initialPS, value);
			auto branchless = ROL_Branchless(initialPS, value);
			EXPECT_EQ(ref.result, branchless.result)
				<< "ROL result mismatch for value=0x" << std::hex << v
				<< " carry=" << carryIn;
			EXPECT_EQ(ref.ps, branchless.ps)
				<< "ROL flags mismatch for value=0x" << std::hex << v
				<< " carry=" << carryIn;
		}
	}
}

TEST_F(PceCpuShiftComparisonTest, ROR_Exhaustive256x2) {
	for (int carryIn = 0; carryIn <= 1; carryIn++) {
		uint8_t initialPS = carryIn ? PceCpuFlags::Carry : 0;
		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			auto ref = ROR_Branching(initialPS, value);
			auto branchless = ROR_Branchless(initialPS, value);
			EXPECT_EQ(ref.result, branchless.result)
				<< "ROR result mismatch for value=0x" << std::hex << v
				<< " carry=" << carryIn;
			EXPECT_EQ(ref.ps, branchless.ps)
				<< "ROR flags mismatch for value=0x" << std::hex << v
				<< " carry=" << carryIn;
		}
	}
}

//=============================================================================
// Before/After Comparison: PCE BIT Instruction
//=============================================================================

class PceCpuBitComparisonTest : public ::testing::Test {
protected:
	// Branching reference
	static uint8_t BIT_Branching(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		if ((acc & value) == 0) ps |= PceCpuFlags::Zero;
		if (value & 0x40) ps |= PceCpuFlags::Overflow;
		if (value & 0x80) ps |= PceCpuFlags::Negative;
		return ps;
	}

	// Branchless (matches current PceCpu.Instructions.cpp)
	static uint8_t BIT_Branchless(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		ps |= static_cast<uint8_t>((acc & value) == 0) << 1;
		ps |= (value & (PceCpuFlags::Overflow | PceCpuFlags::Negative));
		return ps;
	}
};

TEST_F(PceCpuBitComparisonTest, Exhaustive_256x256) {
	for (int a = 0; a < 256; a++) {
		for (int v = 0; v < 256; v++) {
			uint8_t acc = static_cast<uint8_t>(a);
			uint8_t val = static_cast<uint8_t>(v);
			uint8_t ref = BIT_Branching(0x00, acc, val);
			uint8_t branchless = BIT_Branchless(0x00, acc, val);
			ASSERT_EQ(ref, branchless)
				<< "BIT mismatch: acc=0x" << std::hex << a
				<< " value=0x" << v;
		}
	}
}

TEST_F(PceCpuBitComparisonTest, ZeroResult_SetsZeroFlag) {
	uint8_t ps = BIT_Branchless(0x00, 0x0F, 0xF0);
	EXPECT_TRUE(ps & PceCpuFlags::Zero);
}

TEST_F(PceCpuBitComparisonTest, Bit6Set_SetsOverflow) {
	uint8_t ps = BIT_Branchless(0x00, 0xFF, 0x40);
	EXPECT_TRUE(ps & PceCpuFlags::Overflow);
	EXPECT_FALSE(ps & PceCpuFlags::Negative);
}

TEST_F(PceCpuBitComparisonTest, Bit7Set_SetsNegative) {
	uint8_t ps = BIT_Branchless(0x00, 0xFF, 0x80);
	EXPECT_TRUE(ps & PceCpuFlags::Negative);
	EXPECT_FALSE(ps & PceCpuFlags::Overflow);
}

TEST_F(PceCpuBitComparisonTest, PreservesOtherFlags) {
	uint8_t ps = BIT_Branchless(PceCpuFlags::Carry | PceCpuFlags::Decimal, 0xFF, 0x01);
	EXPECT_TRUE(ps & PceCpuFlags::Carry);
	EXPECT_TRUE(ps & PceCpuFlags::Decimal);
}

//=============================================================================
// Before/After Comparison: PCE TSB/TRB/TST Instructions
//=============================================================================

class PceCpuTestBitComparisonTest : public ::testing::Test {
protected:
	// TSB: V+N from original value, Z from (value | A)
	static uint8_t TSB_Branching(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		if (value & 0x40) ps |= PceCpuFlags::Overflow;
		if (value & 0x80) ps |= PceCpuFlags::Negative;
		uint8_t result = value | acc;
		if (result == 0) ps |= PceCpuFlags::Zero;
		return ps;
	}

	static uint8_t TSB_Branchless(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		ps |= (value & (PceCpuFlags::Overflow | PceCpuFlags::Negative));
		uint8_t result = value | acc;
		ps |= static_cast<uint8_t>(result == 0) << 1;
		return ps;
	}

	// TRB: V+N from original value, Z from (value & ~A)
	static uint8_t TRB_Branching(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		if (value & 0x40) ps |= PceCpuFlags::Overflow;
		if (value & 0x80) ps |= PceCpuFlags::Negative;
		uint8_t result = value & ~acc;
		if (result == 0) ps |= PceCpuFlags::Zero;
		return ps;
	}

	static uint8_t TRB_Branchless(uint8_t ps, uint8_t acc, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		ps |= (value & (PceCpuFlags::Overflow | PceCpuFlags::Negative));
		uint8_t result = value & ~acc;
		ps |= static_cast<uint8_t>(result == 0) << 1;
		return ps;
	}

	// TST: V+N from original test value, Z from (value & immediate)
	static uint8_t TST_Branching(uint8_t ps, uint8_t imm, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		if (value & 0x40) ps |= PceCpuFlags::Overflow;
		if (value & 0x80) ps |= PceCpuFlags::Negative;
		uint8_t result = value & imm;
		if (result == 0) ps |= PceCpuFlags::Zero;
		return ps;
	}

	static uint8_t TST_Branchless(uint8_t ps, uint8_t imm, uint8_t value) {
		ps &= ~(PceCpuFlags::Zero | PceCpuFlags::Overflow | PceCpuFlags::Negative);
		ps |= (value & (PceCpuFlags::Overflow | PceCpuFlags::Negative));
		uint8_t result = value & imm;
		ps |= static_cast<uint8_t>(result == 0) << 1;
		return ps;
	}
};

TEST_F(PceCpuTestBitComparisonTest, TSB_Exhaustive_256x256) {
	for (int a = 0; a < 256; a++) {
		for (int v = 0; v < 256; v++) {
			uint8_t acc = static_cast<uint8_t>(a);
			uint8_t val = static_cast<uint8_t>(v);
			ASSERT_EQ(TSB_Branching(0x00, acc, val), TSB_Branchless(0x00, acc, val))
				<< "TSB mismatch: acc=0x" << std::hex << a << " value=0x" << v;
		}
	}
}

TEST_F(PceCpuTestBitComparisonTest, TRB_Exhaustive_256x256) {
	for (int a = 0; a < 256; a++) {
		for (int v = 0; v < 256; v++) {
			uint8_t acc = static_cast<uint8_t>(a);
			uint8_t val = static_cast<uint8_t>(v);
			ASSERT_EQ(TRB_Branching(0x00, acc, val), TRB_Branchless(0x00, acc, val))
				<< "TRB mismatch: acc=0x" << std::hex << a << " value=0x" << v;
		}
	}
}

TEST_F(PceCpuTestBitComparisonTest, TST_Exhaustive_256x256) {
	for (int a = 0; a < 256; a++) {
		for (int v = 0; v < 256; v++) {
			uint8_t imm = static_cast<uint8_t>(a);
			uint8_t val = static_cast<uint8_t>(v);
			ASSERT_EQ(TST_Branching(0x00, imm, val), TST_Branchless(0x00, imm, val))
				<< "TST mismatch: imm=0x" << std::hex << a << " value=0x" << v;
		}
	}
}

TEST_F(PceCpuTestBitComparisonTest, TSB_VNFromOriginal_ZFromResult) {
	// V+N should come from original value, not from result
	uint8_t ps = TSB_Branchless(0x00, 0xFF, 0xC0);  // value=0xC0 (V+N set), result=0xFF
	EXPECT_TRUE(ps & PceCpuFlags::Overflow);
	EXPECT_TRUE(ps & PceCpuFlags::Negative);
	EXPECT_FALSE(ps & PceCpuFlags::Zero);
}

TEST_F(PceCpuTestBitComparisonTest, TRB_ZeroResult) {
	// TRB: value & ~A. If A has all bits of value set, result is 0 → Zero
	uint8_t ps = TRB_Branchless(0x00, 0xFF, 0x42);  // value=0x42, ~A=0x00, result=0x00
	EXPECT_TRUE(ps & PceCpuFlags::Zero);
	EXPECT_TRUE(ps & PceCpuFlags::Overflow);  // bit 6 of 0x42
	EXPECT_FALSE(ps & PceCpuFlags::Negative);
}

TEST_F(PceCpuTestBitComparisonTest, TST_MaskAndTest) {
	// TST: value & immediate. Tests immediate mask against memory value
	uint8_t ps = TST_Branchless(0x00, 0x80, 0xC0);  // value=0xC0, imm=0x80, result=0x80
	EXPECT_FALSE(ps & PceCpuFlags::Zero);   // 0xC0 & 0x80 = 0x80, not zero
	EXPECT_TRUE(ps & PceCpuFlags::Negative); // bit 7 of 0xC0
	EXPECT_TRUE(ps & PceCpuFlags::Overflow); // bit 6 of 0xC0
}
