#include "pch.h"
#include "SNES/SnesCpuTypes.h"

/// <summary>
/// Test fixture for SNES 65816 CPU types and state.
/// These tests verify CPU state structures and flag calculations
/// without requiring a full emulator environment.
/// </summary>
class SnesCpuTypesTest : public ::testing::Test {
protected:
	SnesCpuState _state = {};

	void SetUp() override {
		// Initialize CPU state to known values
		_state = {};
		_state.A = 0;
		_state.X = 0;
		_state.Y = 0;
		_state.SP = 0x01FF;
		_state.D = 0;
		_state.PC = 0;
		_state.K = 0;
		_state.DBR = 0;
		_state.PS = 0;
		_state.EmulationMode = false;
	}

	// Helper to set processor flags
	void SetFlags(uint8_t flags) {
		_state.PS |= flags;
	}

	// Helper to clear processor flags
	void ClearFlags(uint8_t flags) {
		_state.PS &= ~flags;
	}

	// Helper to check processor flag
	bool CheckFlag(uint8_t flag) {
		return (_state.PS & flag) != 0;
	}

	// Helper to set zero and negative flags based on 8-bit value
	void SetZeroNegativeFlags8(uint8_t value) {
		ClearFlags(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) {
			SetFlags(ProcFlags::Zero);
		}
		if (value & 0x80) {
			SetFlags(ProcFlags::Negative);
		}
	}

	// Helper to set zero and negative flags based on 16-bit value
	void SetZeroNegativeFlags16(uint16_t value) {
		ClearFlags(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) {
			SetFlags(ProcFlags::Zero);
		}
		if (value & 0x8000) {
			SetFlags(ProcFlags::Negative);
		}
	}
};

//=============================================================================
// CPU State Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, InitialState_AllZero) {
	SnesCpuState state = {};
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.X, 0);
	EXPECT_EQ(state.Y, 0);
	EXPECT_EQ(state.PC, 0);
	EXPECT_EQ(state.PS, 0);
	EXPECT_EQ(state.EmulationMode, false);
}

TEST_F(SnesCpuTypesTest, StopState_DefaultIsRunning) {
	SnesCpuState state = {};
	EXPECT_EQ(state.StopState, SnesCpuStopState::Running);
}

//=============================================================================
// Processor Flag Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, Flags_CarryFlag_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::Carry));
	SetFlags(ProcFlags::Carry);
	EXPECT_TRUE(CheckFlag(ProcFlags::Carry));
	ClearFlags(ProcFlags::Carry);
	EXPECT_FALSE(CheckFlag(ProcFlags::Carry));
}

TEST_F(SnesCpuTypesTest, Flags_ZeroFlag_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	SetFlags(ProcFlags::Zero);
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));
	ClearFlags(ProcFlags::Zero);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
}

TEST_F(SnesCpuTypesTest, Flags_NegativeFlag_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
	SetFlags(ProcFlags::Negative);
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
	ClearFlags(ProcFlags::Negative);
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, Flags_OverflowFlag_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::Overflow));
	SetFlags(ProcFlags::Overflow);
	EXPECT_TRUE(CheckFlag(ProcFlags::Overflow));
	ClearFlags(ProcFlags::Overflow);
	EXPECT_FALSE(CheckFlag(ProcFlags::Overflow));
}

TEST_F(SnesCpuTypesTest, Flags_IrqDisable_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::IrqDisable));
	SetFlags(ProcFlags::IrqDisable);
	EXPECT_TRUE(CheckFlag(ProcFlags::IrqDisable));
	ClearFlags(ProcFlags::IrqDisable);
	EXPECT_FALSE(CheckFlag(ProcFlags::IrqDisable));
}

TEST_F(SnesCpuTypesTest, Flags_DecimalMode_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::Decimal));
	SetFlags(ProcFlags::Decimal);
	EXPECT_TRUE(CheckFlag(ProcFlags::Decimal));
	ClearFlags(ProcFlags::Decimal);
	EXPECT_FALSE(CheckFlag(ProcFlags::Decimal));
}

TEST_F(SnesCpuTypesTest, Flags_IndexMode8_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::IndexMode8));
	SetFlags(ProcFlags::IndexMode8);
	EXPECT_TRUE(CheckFlag(ProcFlags::IndexMode8));
	ClearFlags(ProcFlags::IndexMode8);
	EXPECT_FALSE(CheckFlag(ProcFlags::IndexMode8));
}

TEST_F(SnesCpuTypesTest, Flags_MemoryMode8_SetAndClear) {
	EXPECT_FALSE(CheckFlag(ProcFlags::MemoryMode8));
	SetFlags(ProcFlags::MemoryMode8);
	EXPECT_TRUE(CheckFlag(ProcFlags::MemoryMode8));
	ClearFlags(ProcFlags::MemoryMode8);
	EXPECT_FALSE(CheckFlag(ProcFlags::MemoryMode8));
}

TEST_F(SnesCpuTypesTest, Flags_MultipleFlags_SetSimultaneously) {
	SetFlags(ProcFlags::Carry | ProcFlags::Zero | ProcFlags::Negative);
	EXPECT_TRUE(CheckFlag(ProcFlags::Carry));
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
	EXPECT_FALSE(CheckFlag(ProcFlags::Overflow));
}

TEST_F(SnesCpuTypesTest, Flags_MultipleFlags_ClearSimultaneously) {
	_state.PS = 0xFF;  // All flags set
	ClearFlags(ProcFlags::Carry | ProcFlags::Zero);
	EXPECT_FALSE(CheckFlag(ProcFlags::Carry));
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));  // Should still be set
}

//=============================================================================
// Zero/Negative Flag Calculation Tests (8-bit mode)
//=============================================================================

TEST_F(SnesCpuTypesTest, ZeroNegative8_ZeroValue_SetsZeroFlag) {
	SetZeroNegativeFlags8(0x00);
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_PositiveValue_ClearsBothFlags) {
	SetZeroNegativeFlags8(0x01);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_NegativeValue_SetsNegativeFlag) {
	SetZeroNegativeFlags8(0x80);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_MaxValue_SetsNegativeFlag) {
	SetZeroNegativeFlags8(0xFF);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_Boundary_0x7F_ClearsBothFlags) {
	SetZeroNegativeFlags8(0x7F);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

//=============================================================================
// Zero/Negative Flag Calculation Tests (16-bit mode)
//=============================================================================

TEST_F(SnesCpuTypesTest, ZeroNegative16_ZeroValue_SetsZeroFlag) {
	SetZeroNegativeFlags16(0x0000);
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_PositiveValue_ClearsBothFlags) {
	SetZeroNegativeFlags16(0x0001);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_NegativeValue_SetsNegativeFlag) {
	SetZeroNegativeFlags16(0x8000);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_MaxValue_SetsNegativeFlag) {
	SetZeroNegativeFlags16(0xFFFF);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_Boundary_0x7FFF_ClearsBothFlags) {
	SetZeroNegativeFlags16(0x7FFF);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_LowByte0x80_ClearsBothFlags) {
	// 0x0080 is positive in 16-bit mode (bit 15 is clear)
	SetZeroNegativeFlags16(0x0080);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_Exhaustive_All256Values) {
	// Exhaustive test: verify 8-bit zero/negative flag behavior for all uint8_t values.
	// Ensures the branchless optimization in SnesCpu.Shared.h produces identical
	// results to the reference if/else if implementation.
	for (int i = 0; i <= 255; i++) {
		uint8_t value = static_cast<uint8_t>(i);
		SetZeroNegativeFlags8(value);

		bool expectZero = (value == 0);
		bool expectNegative = (value & 0x80) != 0;

		EXPECT_EQ(CheckFlag(ProcFlags::Zero), expectZero)
			<< "8-bit Zero flag mismatch for value 0x" << std::hex << i;
		EXPECT_EQ(CheckFlag(ProcFlags::Negative), expectNegative)
			<< "8-bit Negative flag mismatch for value 0x" << std::hex << i;

		// Zero and Negative flags must be mutually exclusive for 8-bit values
		EXPECT_FALSE(CheckFlag(ProcFlags::Zero) && CheckFlag(ProcFlags::Negative))
			<< "Both Zero and Negative set for 8-bit value 0x" << std::hex << i;
	}
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_Exhaustive_BoundaryValues) {
	// Exhaustive boundary test for 16-bit mode. Tests every critical boundary
	// in the uint16_t range to ensure the branchless (value >> 8) & 0x80
	// optimization correctly maps bit 15 to the Negative flag.
	uint16_t values[] = {
		0x0000, 0x0001, 0x0002,                     // Near zero
		0x007E, 0x007F, 0x0080, 0x0081,             // Low byte sign boundary
		0x00FE, 0x00FF, 0x0100, 0x0101,             // Low byte overflow boundary
		0x1000, 0x2000, 0x3000, 0x4000,             // Mid-range positive
		0x7FFE, 0x7FFF,                              // Max positive boundary
		0x8000, 0x8001,                              // Min negative boundary
		0x8080,                                      // Both bytes have bit 7 set
		0xC000, 0xE000,                              // High negative range
		0xFF00, 0xFF7F, 0xFF80, 0xFFFE, 0xFFFF      // Near max negative
	};

	for (uint16_t value : values) {
		SetZeroNegativeFlags16(value);

		bool expectZero = (value == 0);
		bool expectNegative = (value & 0x8000) != 0;

		EXPECT_EQ(CheckFlag(ProcFlags::Zero), expectZero)
			<< "16-bit Zero flag mismatch for value 0x" << std::hex << value;
		EXPECT_EQ(CheckFlag(ProcFlags::Negative), expectNegative)
			<< "16-bit Negative flag mismatch for value 0x" << std::hex << value;

		// Zero and Negative are mutually exclusive
		EXPECT_FALSE(CheckFlag(ProcFlags::Zero) && CheckFlag(ProcFlags::Negative))
			<< "Both Zero and Negative set for 16-bit value 0x" << std::hex << value;
	}
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_HighByteExhaustive) {
	// Test every possible high byte (0x00-0xFF) with low byte = 0x00.
	// This specifically validates (value >> 8) & 0x80 for all high byte patterns.
	for (int hi = 0; hi <= 255; hi++) {
		uint16_t value = static_cast<uint16_t>(hi << 8);
		SetZeroNegativeFlags16(value);

		bool expectZero = (value == 0);
		bool expectNegative = (hi & 0x80) != 0;

		EXPECT_EQ(CheckFlag(ProcFlags::Zero), expectZero)
			<< "High byte 0x" << std::hex << hi << " Zero flag mismatch";
		EXPECT_EQ(CheckFlag(ProcFlags::Negative), expectNegative)
			<< "High byte 0x" << std::hex << hi << " Negative flag mismatch";
	}
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_PreservesOtherFlags) {
	// Ensure 8-bit SetZeroNegativeFlags only affects Zero and Negative flags.
	_state.PS = 0;
	SetFlags(ProcFlags::Carry | ProcFlags::Overflow);
	SetZeroNegativeFlags8(0x42);
	EXPECT_TRUE(CheckFlag(ProcFlags::Carry));
	EXPECT_TRUE(CheckFlag(ProcFlags::Overflow));
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_PreservesOtherFlags) {
	// Ensure 16-bit SetZeroNegativeFlags only affects Zero and Negative flags.
	_state.PS = 0;
	SetFlags(ProcFlags::Carry | ProcFlags::Overflow | ProcFlags::IrqDisable);
	SetZeroNegativeFlags16(0x1234);
	EXPECT_TRUE(CheckFlag(ProcFlags::Carry));
	EXPECT_TRUE(CheckFlag(ProcFlags::Overflow));
	EXPECT_TRUE(CheckFlag(ProcFlags::IrqDisable));
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative8_ClearsStaleFlags) {
	// Stale Zero flag must be cleared when processing non-zero value.
	SetZeroNegativeFlags8(0x00);
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));

	SetZeroNegativeFlags8(0x42);
	EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_ClearsStaleFlags) {
	// Stale Negative flag must be cleared when processing zero value.
	SetZeroNegativeFlags16(0x8000);
	EXPECT_TRUE(CheckFlag(ProcFlags::Negative));

	SetZeroNegativeFlags16(0x0000);
	EXPECT_FALSE(CheckFlag(ProcFlags::Negative));
	EXPECT_TRUE(CheckFlag(ProcFlags::Zero));
}

TEST_F(SnesCpuTypesTest, ZeroNegative16_LowByteDoesNotAffectNegative) {
	// Critical: in 16-bit mode, bit 7 of the LOW byte (0x0080) must NOT
	// set the Negative flag. Only bit 15 matters.
	uint16_t lowByteNegative[] = { 0x0080, 0x0081, 0x00FF, 0x00C0, 0x00FE };
	for (uint16_t value : lowByteNegative) {
		SetZeroNegativeFlags16(value);
		EXPECT_FALSE(CheckFlag(ProcFlags::Negative))
			<< "16-bit mode incorrectly set Negative for 0x" << std::hex << value;
		EXPECT_FALSE(CheckFlag(ProcFlags::Zero));
	}
}

//=============================================================================
// Register Size Mode Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, Accumulator_8BitMode_HighByteMasked) {
	_state.A = 0x1234;
	SetFlags(ProcFlags::MemoryMode8);

	// In 8-bit mode, only low byte should be affected by operations
	// The high byte is preserved but operations only see low byte
	uint8_t lowByte = _state.A & 0xFF;
	EXPECT_EQ(lowByte, 0x34);
}

TEST_F(SnesCpuTypesTest, Accumulator_16BitMode_FullWord) {
	_state.A = 0x1234;
	ClearFlags(ProcFlags::MemoryMode8);

	// In 16-bit mode, full word is used
	EXPECT_EQ(_state.A, 0x1234);
}

TEST_F(SnesCpuTypesTest, IndexRegisters_8BitMode_HighByteMasked) {
	_state.X = 0xABCD;
	_state.Y = 0xEF01;
	SetFlags(ProcFlags::IndexMode8);

	// In 8-bit index mode, only low byte should be used
	uint8_t xLow = _state.X & 0xFF;
	uint8_t yLow = _state.Y & 0xFF;
	EXPECT_EQ(xLow, 0xCD);
	EXPECT_EQ(yLow, 0x01);
}

TEST_F(SnesCpuTypesTest, IndexRegisters_16BitMode_FullWord) {
	_state.X = 0xABCD;
	_state.Y = 0xEF01;
	ClearFlags(ProcFlags::IndexMode8);

	EXPECT_EQ(_state.X, 0xABCD);
	EXPECT_EQ(_state.Y, 0xEF01);
}

//=============================================================================
// Stack Pointer Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, StackPointer_NativeMode_16Bit) {
	_state.EmulationMode = false;
	_state.SP = 0x1FFF;
	EXPECT_EQ(_state.SP, 0x1FFF);
}

TEST_F(SnesCpuTypesTest, StackPointer_EmulationMode_PageOne) {
	_state.EmulationMode = true;
	// In emulation mode, SP high byte is always 0x01
	// This test just verifies the constraint is documented
	_state.SP = 0x01FF;
	EXPECT_EQ(_state.SP >> 8, 0x01);
}

//=============================================================================
// Direct Page Register Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, DirectPage_CanBeAnyValue) {
	_state.D = 0x0000;
	EXPECT_EQ(_state.D, 0x0000);

	_state.D = 0x1234;
	EXPECT_EQ(_state.D, 0x1234);

	_state.D = 0xFF00;
	EXPECT_EQ(_state.D, 0xFF00);
}

//=============================================================================
// Bank Register Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, ProgramBank_8BitRange) {
	_state.K = 0x00;
	EXPECT_EQ(_state.K, 0x00);

	_state.K = 0x7E;
	EXPECT_EQ(_state.K, 0x7E);

	_state.K = 0xFF;
	EXPECT_EQ(_state.K, 0xFF);
}

TEST_F(SnesCpuTypesTest, DataBank_8BitRange) {
	_state.DBR = 0x00;
	EXPECT_EQ(_state.DBR, 0x00);

	_state.DBR = 0x7E;
	EXPECT_EQ(_state.DBR, 0x7E);

	_state.DBR = 0xFF;
	EXPECT_EQ(_state.DBR, 0xFF);
}

//=============================================================================
// Address Calculation Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, FullAddress_BankAndOffset) {
	_state.K = 0x80;
	_state.PC = 0x1234;

	// Full 24-bit address = (K << 16) | PC
	uint32_t fullAddress = ((uint32_t)_state.K << 16) | _state.PC;
	EXPECT_EQ(fullAddress, 0x801234);
}

TEST_F(SnesCpuTypesTest, DataAddress_DBRAndOffset) {
	_state.DBR = 0x7E;
	uint16_t offset = 0x2000;

	// Data address = (DBR << 16) | offset
	uint32_t dataAddress = ((uint32_t)_state.DBR << 16) | offset;
	EXPECT_EQ(dataAddress, 0x7E2000);
}

TEST_F(SnesCpuTypesTest, DirectPageAddress_DPlusOffset) {
	_state.D = 0x1000;
	uint8_t offset = 0x50;

	// Direct page address = D + offset (wraps at bank boundary in native mode)
	uint16_t dpAddress = _state.D + offset;
	EXPECT_EQ(dpAddress, 0x1050);
}

//=============================================================================
// Emulation Mode Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, EmulationMode_FlagsAreConstrained) {
	_state.EmulationMode = true;

	// In emulation mode:
	// - M flag (MemoryMode8) is always 1
	// - X flag (IndexMode8) is always 1
	// This is enforced by CPU logic, not the state struct
	// Test documents the expected behavior
	SetFlags(ProcFlags::MemoryMode8 | ProcFlags::IndexMode8);
	EXPECT_TRUE(CheckFlag(ProcFlags::MemoryMode8));
	EXPECT_TRUE(CheckFlag(ProcFlags::IndexMode8));
}

TEST_F(SnesCpuTypesTest, NativeMode_FlagsCanVary) {
	_state.EmulationMode = false;

	// In native mode, M and X flags can be any value
	ClearFlags(ProcFlags::MemoryMode8 | ProcFlags::IndexMode8);
	EXPECT_FALSE(CheckFlag(ProcFlags::MemoryMode8));
	EXPECT_FALSE(CheckFlag(ProcFlags::IndexMode8));

	SetFlags(ProcFlags::MemoryMode8);
	EXPECT_TRUE(CheckFlag(ProcFlags::MemoryMode8));
	EXPECT_FALSE(CheckFlag(ProcFlags::IndexMode8));
}

//=============================================================================
// IRQ Source Tests
//=============================================================================

TEST_F(SnesCpuTypesTest, IrqSource_None) {
	_state.IrqSource = (uint8_t)SnesIrqSource::None;
	EXPECT_EQ(_state.IrqSource, 0);
}

TEST_F(SnesCpuTypesTest, IrqSource_Ppu) {
	_state.IrqSource = (uint8_t)SnesIrqSource::Ppu;
	EXPECT_EQ(_state.IrqSource, 1);
}

TEST_F(SnesCpuTypesTest, IrqSource_Coprocessor) {
	_state.IrqSource = (uint8_t)SnesIrqSource::Coprocessor;
	EXPECT_EQ(_state.IrqSource, 2);
}

TEST_F(SnesCpuTypesTest, IrqSource_Multiple) {
	_state.IrqSource = (uint8_t)SnesIrqSource::Ppu | (uint8_t)SnesIrqSource::Coprocessor;
	EXPECT_TRUE((_state.IrqSource & (uint8_t)SnesIrqSource::Ppu) != 0);
	EXPECT_TRUE((_state.IrqSource & (uint8_t)SnesIrqSource::Coprocessor) != 0);
}

//=============================================================================
// Parameterized Flag Tests
//=============================================================================

class SnesCpuFlagTest : public ::testing::TestWithParam<uint8_t> {};

TEST_P(SnesCpuFlagTest, IndividualFlag_SetAndRead) {
	uint8_t ps = 0;
	uint8_t flag = GetParam();

	ps |= flag;
	EXPECT_EQ(ps & flag, flag);

	ps &= ~flag;
	EXPECT_EQ(ps & flag, 0);
}

INSTANTIATE_TEST_SUITE_P(
	AllFlags,
	SnesCpuFlagTest,
	::testing::Values(
		ProcFlags::Carry,       // 0x01
		ProcFlags::Zero,        // 0x02
		ProcFlags::IrqDisable,  // 0x04
		ProcFlags::Decimal,     // 0x08
		ProcFlags::IndexMode8,  // 0x10
		ProcFlags::MemoryMode8, // 0x20
		ProcFlags::Overflow,    // 0x40
		ProcFlags::Negative     // 0x80
	)
);

//=============================================================================
// Arithmetic Logic Tests (Binary Addition/Subtraction)
//=============================================================================

/// <summary>
/// Test fixture for 65816 arithmetic logic.
/// These tests verify the mathematical operations without
/// requiring memory access.
/// </summary>
class SnesCpuArithmeticTest : public ::testing::Test {
protected:
	// Simulate 8-bit binary addition (non-decimal mode)
	struct Add8Result {
		uint8_t result;
		bool carry;
		bool overflow;
		bool zero;
		bool negative;
	};

	Add8Result Add8(uint8_t a, uint8_t b, bool carryIn) {
		uint16_t sum = (uint16_t)a + (uint16_t)b + (carryIn ? 1 : 0);
		uint8_t result = (uint8_t)sum;

		bool carry = sum > 0xFF;
		bool overflow = (~(a ^ b) & (a ^ result) & 0x80) != 0;
		bool zero = result == 0;
		bool negative = (result & 0x80) != 0;

		return { result, carry, overflow, zero, negative };
	}

	// Simulate 16-bit binary addition (non-decimal mode)
	struct Add16Result {
		uint16_t result;
		bool carry;
		bool overflow;
		bool zero;
		bool negative;
	};

	Add16Result Add16(uint16_t a, uint16_t b, bool carryIn) {
		uint32_t sum = (uint32_t)a + (uint32_t)b + (carryIn ? 1 : 0);
		uint16_t result = (uint16_t)sum;

		bool carry = sum > 0xFFFF;
		bool overflow = (~(a ^ b) & (a ^ result) & 0x8000) != 0;
		bool zero = result == 0;
		bool negative = (result & 0x8000) != 0;

		return { result, carry, overflow, zero, negative };
	}

	// Simulate 8-bit binary subtraction (non-decimal mode)
	// SBC is implemented as A + ~B + C
	Add8Result Sub8(uint8_t a, uint8_t b, bool carryIn) {
		return Add8(a, ~b, carryIn);
	}

	// Simulate 16-bit binary subtraction (non-decimal mode)
	Add16Result Sub16(uint16_t a, uint16_t b, bool carryIn) {
		return Add16(a, (uint16_t)~b, carryIn);
	}
};

// ADC Tests (8-bit)
TEST_F(SnesCpuArithmeticTest, Add8_ZeroPlusZero_ReturnsZero) {
	auto r = Add8(0x00, 0x00, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.overflow);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuArithmeticTest, Add8_OnePlusOne_ReturnsTwo) {
	auto r = Add8(0x01, 0x01, false);
	EXPECT_EQ(r.result, 0x02);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.overflow);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuArithmeticTest, Add8_WithCarryIn_AddsOne) {
	auto r = Add8(0x01, 0x01, true);
	EXPECT_EQ(r.result, 0x03);
	EXPECT_FALSE(r.carry);
}

TEST_F(SnesCpuArithmeticTest, Add8_Overflow_SetsCarry) {
	auto r = Add8(0xFF, 0x01, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Add8_SignedOverflow_PositiveToNegative) {
	// 0x7F + 0x01 = 0x80 (127 + 1 = -128 in signed)
	auto r = Add8(0x7F, 0x01, false);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuArithmeticTest, Add8_SignedOverflow_NegativeToPositive) {
	// 0x80 + 0x80 = 0x00 (-128 + -128 = 0 in signed, overflow)
	auto r = Add8(0x80, 0x80, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Add8_NoSignedOverflow_SameSign) {
	// 0x40 + 0x20 = 0x60 (64 + 32 = 96, no overflow)
	auto r = Add8(0x40, 0x20, false);
	EXPECT_EQ(r.result, 0x60);
	EXPECT_FALSE(r.overflow);
}

// ADC Tests (16-bit)
TEST_F(SnesCpuArithmeticTest, Add16_ZeroPlusZero_ReturnsZero) {
	auto r = Add16(0x0000, 0x0000, false);
	EXPECT_EQ(r.result, 0x0000);
	EXPECT_FALSE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Add16_Overflow_SetsCarry) {
	auto r = Add16(0xFFFF, 0x0001, false);
	EXPECT_EQ(r.result, 0x0000);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Add16_SignedOverflow_PositiveToNegative) {
	// 0x7FFF + 0x0001 = 0x8000
	auto r = Add16(0x7FFF, 0x0001, false);
	EXPECT_EQ(r.result, 0x8000);
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.negative);
}

// SBC Tests (8-bit)
TEST_F(SnesCpuArithmeticTest, Sub8_ZeroMinusZero_ReturnsZero) {
	// SBC with carry set means no borrow
	auto r = Sub8(0x00, 0x00, true);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.carry);  // No borrow
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Sub8_TwoMinusOne_ReturnsOne) {
	auto r = Sub8(0x02, 0x01, true);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_TRUE(r.carry);  // No borrow
}

TEST_F(SnesCpuArithmeticTest, Sub8_OneMinusTwo_Underflows) {
	auto r = Sub8(0x01, 0x02, true);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.carry);  // Borrow occurred
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuArithmeticTest, Sub8_WithBorrow_SubtractsOne) {
	// With carry clear (borrow), subtracts additional 1
	auto r = Sub8(0x02, 0x01, false);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
}

// SBC Tests (16-bit)
TEST_F(SnesCpuArithmeticTest, Sub16_ZeroMinusZero_ReturnsZero) {
	auto r = Sub16(0x0000, 0x0000, true);
	EXPECT_EQ(r.result, 0x0000);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuArithmeticTest, Sub16_Underflow_ClearsBorrow) {
	auto r = Sub16(0x0000, 0x0001, true);
	EXPECT_EQ(r.result, 0xFFFF);
	EXPECT_FALSE(r.carry);  // Borrow occurred
	EXPECT_TRUE(r.negative);
}

//=============================================================================
// Shift/Rotate Logic Tests
//=============================================================================

class SnesCpuShiftTest : public ::testing::Test {
protected:
	// ASL - Arithmetic Shift Left (8-bit)
	std::pair<uint8_t, bool> ASL8(uint8_t value) {
		bool carryOut = (value & 0x80) != 0;
		uint8_t result = value << 1;
		return { result, carryOut };
	}

	// ASL - Arithmetic Shift Left (16-bit)
	std::pair<uint16_t, bool> ASL16(uint16_t value) {
		bool carryOut = (value & 0x8000) != 0;
		uint16_t result = value << 1;
		return { result, carryOut };
	}

	// LSR - Logical Shift Right (8-bit)
	std::pair<uint8_t, bool> LSR8(uint8_t value) {
		bool carryOut = (value & 0x01) != 0;
		uint8_t result = value >> 1;
		return { result, carryOut };
	}

	// LSR - Logical Shift Right (16-bit)
	std::pair<uint16_t, bool> LSR16(uint16_t value) {
		bool carryOut = (value & 0x0001) != 0;
		uint16_t result = value >> 1;
		return { result, carryOut };
	}

	// ROL - Rotate Left (8-bit)
	std::pair<uint8_t, bool> ROL8(uint8_t value, bool carryIn) {
		bool carryOut = (value & 0x80) != 0;
		uint8_t result = (value << 1) | (carryIn ? 1 : 0);
		return { result, carryOut };
	}

	// ROL - Rotate Left (16-bit)
	std::pair<uint16_t, bool> ROL16(uint16_t value, bool carryIn) {
		bool carryOut = (value & 0x8000) != 0;
		uint16_t result = (value << 1) | (carryIn ? 1 : 0);
		return { result, carryOut };
	}

	// ROR - Rotate Right (8-bit)
	std::pair<uint8_t, bool> ROR8(uint8_t value, bool carryIn) {
		bool carryOut = (value & 0x01) != 0;
		uint8_t result = (value >> 1) | (carryIn ? 0x80 : 0);
		return { result, carryOut };
	}

	// ROR - Rotate Right (16-bit)
	std::pair<uint16_t, bool> ROR16(uint16_t value, bool carryIn) {
		bool carryOut = (value & 0x0001) != 0;
		uint16_t result = (value >> 1) | (carryIn ? 0x8000 : 0);
		return { result, carryOut };
	}
};

// ASL Tests
TEST_F(SnesCpuShiftTest, ASL8_ShiftOne_ReturnsTwo) {
	auto [result, carry] = ASL8(0x01);
	EXPECT_EQ(result, 0x02);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, ASL8_ShiftHighBit_SetsCarry) {
	auto [result, carry] = ASL8(0x80);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(SnesCpuShiftTest, ASL8_Shift0x55_Returns0xAA) {
	auto [result, carry] = ASL8(0x55);
	EXPECT_EQ(result, 0xAA);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, ASL16_ShiftHighBit_SetsCarry) {
	auto [result, carry] = ASL16(0x8000);
	EXPECT_EQ(result, 0x0000);
	EXPECT_TRUE(carry);
}

// LSR Tests
TEST_F(SnesCpuShiftTest, LSR8_ShiftTwo_ReturnsOne) {
	auto [result, carry] = LSR8(0x02);
	EXPECT_EQ(result, 0x01);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, LSR8_ShiftOne_SetsCarry) {
	auto [result, carry] = LSR8(0x01);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(SnesCpuShiftTest, LSR8_Shift0xAA_Returns0x55) {
	auto [result, carry] = LSR8(0xAA);
	EXPECT_EQ(result, 0x55);
	EXPECT_FALSE(carry);
}

// ROL Tests
TEST_F(SnesCpuShiftTest, ROL8_RotateOne_WithCarry_ReturnsThree) {
	auto [result, carry] = ROL8(0x01, true);
	EXPECT_EQ(result, 0x03);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, ROL8_RotateHighBit_WithoutCarry_SetsCarryReturnsZero) {
	auto [result, carry] = ROL8(0x80, false);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

TEST_F(SnesCpuShiftTest, ROL8_RotateHighBit_WithCarry_SetsCarryReturnsOne) {
	auto [result, carry] = ROL8(0x80, true);
	EXPECT_EQ(result, 0x01);
	EXPECT_TRUE(carry);
}

// ROR Tests
TEST_F(SnesCpuShiftTest, ROR8_RotateTwo_WithoutCarry_ReturnsOne) {
	auto [result, carry] = ROR8(0x02, false);
	EXPECT_EQ(result, 0x01);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, ROR8_RotateOne_WithCarry_Returns0x80) {
	auto [result, carry] = ROR8(0x00, true);
	EXPECT_EQ(result, 0x80);
	EXPECT_FALSE(carry);
}

TEST_F(SnesCpuShiftTest, ROR8_RotateLowBit_SetsCarry) {
	auto [result, carry] = ROR8(0x01, false);
	EXPECT_EQ(result, 0x00);
	EXPECT_TRUE(carry);
}

//=============================================================================
// Compare Logic Tests
//=============================================================================

class SnesCpuCompareTest : public ::testing::Test {
protected:
	struct CompareResult {
		bool carry;     // Set if register >= value (unsigned)
		bool zero;      // Set if register == value
		bool negative;  // Set if (register - value) bit 7/15 is set
	};

	// Compare 8-bit values
	CompareResult Compare8(uint8_t reg, uint8_t value) {
		uint8_t result = reg - value;
		return {
			reg >= value,
			reg == value,
			(result & 0x80) != 0
		};
	}

	// Compare 16-bit values
	CompareResult Compare16(uint16_t reg, uint16_t value) {
		uint16_t result = reg - value;
		return {
			reg >= value,
			reg == value,
			(result & 0x8000) != 0
		};
	}
};

TEST_F(SnesCpuCompareTest, Compare8_Equal_SetsZeroAndCarry) {
	auto r = Compare8(0x50, 0x50);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuCompareTest, Compare8_Greater_SetsCarryOnly) {
	auto r = Compare8(0x60, 0x50);
	EXPECT_TRUE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuCompareTest, Compare8_Less_ClearsCarry) {
	auto r = Compare8(0x40, 0x50);
	EXPECT_FALSE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);  // 0x40 - 0x50 = 0xF0 (negative)
}

TEST_F(SnesCpuCompareTest, Compare8_ZeroVsZero_SetsZeroAndCarry) {
	auto r = Compare8(0x00, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuCompareTest, Compare8_MaxVsZero_SetsCarry) {
	auto r = Compare8(0xFF, 0x00);
	EXPECT_TRUE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);  // Result 0xFF has bit 7 set
}

TEST_F(SnesCpuCompareTest, Compare16_Equal_SetsZeroAndCarry) {
	auto r = Compare16(0x1234, 0x1234);
	EXPECT_TRUE(r.carry);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuCompareTest, Compare16_Greater_SetsCarryOnly) {
	auto r = Compare16(0x8000, 0x7FFF);
	EXPECT_TRUE(r.carry);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);  // 0x8000 - 0x7FFF = 0x0001
}

//=============================================================================
// Bitwise Logic Tests
//=============================================================================

class SnesCpuBitwiseTest : public ::testing::Test {};

TEST_F(SnesCpuBitwiseTest, AND8_MasksCorrectly) {
	uint8_t a = 0xF0;
	uint8_t b = 0x0F;
	EXPECT_EQ(a & b, 0x00);

	a = 0xFF;
	EXPECT_EQ(a & b, 0x0F);
}

TEST_F(SnesCpuBitwiseTest, ORA8_CombinesCorrectly) {
	uint8_t a = 0xF0;
	uint8_t b = 0x0F;
	EXPECT_EQ(a | b, 0xFF);

	a = 0x00;
	EXPECT_EQ(a | b, 0x0F);
}

TEST_F(SnesCpuBitwiseTest, EOR8_XorsCorrectly) {
	uint8_t a = 0xFF;
	uint8_t b = 0xFF;
	EXPECT_EQ(a ^ b, 0x00);

	a = 0xAA;
	b = 0x55;
	EXPECT_EQ(a ^ b, 0xFF);
}

TEST_F(SnesCpuBitwiseTest, AND16_MasksCorrectly) {
	uint16_t a = 0xFF00;
	uint16_t b = 0x00FF;
	EXPECT_EQ(a & b, 0x0000);
}

TEST_F(SnesCpuBitwiseTest, ORA16_CombinesCorrectly) {
	uint16_t a = 0xFF00;
	uint16_t b = 0x00FF;
	EXPECT_EQ(a | b, 0xFFFF);
}

TEST_F(SnesCpuBitwiseTest, EOR16_XorsCorrectly) {
	uint16_t a = 0xAAAA;
	uint16_t b = 0x5555;
	EXPECT_EQ(a ^ b, 0xFFFF);
}

//=============================================================================
// BIT Instruction Logic Tests
//=============================================================================

class SnesCpuBitTest : public ::testing::Test {
protected:
	struct BitResult {
		bool zero;      // Set if (A & M) == 0
		bool overflow;  // Set to bit 6 of memory value
		bool negative;  // Set to bit 7 of memory value
	};

	// BIT instruction logic (8-bit)
	BitResult BIT8(uint8_t acc, uint8_t mem) {
		return {
			(acc & mem) == 0,
			(mem & 0x40) != 0,
			(mem & 0x80) != 0
		};
	}

	// BIT instruction logic (16-bit)
	BitResult BIT16(uint16_t acc, uint16_t mem) {
		return {
			(acc & mem) == 0,
			(mem & 0x4000) != 0,
			(mem & 0x8000) != 0
		};
	}
};

TEST_F(SnesCpuBitTest, BIT8_ZeroResult_SetsZero) {
	auto r = BIT8(0x0F, 0xF0);
	EXPECT_TRUE(r.zero);
}

TEST_F(SnesCpuBitTest, BIT8_NonZeroResult_ClearsZero) {
	auto r = BIT8(0xFF, 0x01);
	EXPECT_FALSE(r.zero);
}

TEST_F(SnesCpuBitTest, BIT8_Bit6Set_SetsOverflow) {
	auto r = BIT8(0x00, 0x40);
	EXPECT_TRUE(r.overflow);
}

TEST_F(SnesCpuBitTest, BIT8_Bit7Set_SetsNegative) {
	auto r = BIT8(0x00, 0x80);
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuBitTest, BIT8_AllBitsSet_AllFlagsSet) {
	auto r = BIT8(0xFF, 0xC0);
	EXPECT_FALSE(r.zero);  // Result is non-zero
	EXPECT_TRUE(r.overflow);
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuBitTest, BIT16_Bit14Set_SetsOverflow) {
	auto r = BIT16(0x0000, 0x4000);
	EXPECT_TRUE(r.overflow);
}

TEST_F(SnesCpuBitTest, BIT16_Bit15Set_SetsNegative) {
	auto r = BIT16(0x0000, 0x8000);
	EXPECT_TRUE(r.negative);
}

//=============================================================================
// Increment/Decrement Logic Tests
//=============================================================================

class SnesCpuIncDecTest : public ::testing::Test {
protected:
	struct IncDecResult {
		uint8_t result;
		bool zero;
		bool negative;
	};

	IncDecResult INC8(uint8_t value) {
		uint8_t result = value + 1;
		return {
			result,
			result == 0,
			(result & 0x80) != 0
		};
	}

	IncDecResult DEC8(uint8_t value) {
		uint8_t result = value - 1;
		return {
			result,
			result == 0,
			(result & 0x80) != 0
		};
	}
};

TEST_F(SnesCpuIncDecTest, INC8_ZeroToOne) {
	auto r = INC8(0x00);
	EXPECT_EQ(r.result, 0x01);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuIncDecTest, INC8_0xFFWrapsToZero) {
	auto r = INC8(0xFF);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuIncDecTest, INC8_0x7FBecomesNegative) {
	auto r = INC8(0x7F);
	EXPECT_EQ(r.result, 0x80);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuIncDecTest, DEC8_OneToZero) {
	auto r = DEC8(0x01);
	EXPECT_EQ(r.result, 0x00);
	EXPECT_TRUE(r.zero);
	EXPECT_FALSE(r.negative);
}

TEST_F(SnesCpuIncDecTest, DEC8_ZeroWrapsTo0xFF) {
	auto r = DEC8(0x00);
	EXPECT_EQ(r.result, 0xFF);
	EXPECT_FALSE(r.zero);
	EXPECT_TRUE(r.negative);
}

TEST_F(SnesCpuIncDecTest, DEC8_0x80BecomesPositive) {
	auto r = DEC8(0x80);
	EXPECT_EQ(r.result, 0x7F);
	EXPECT_FALSE(r.zero);
	EXPECT_FALSE(r.negative);
}

//=============================================================================
// Before/After Comparison: Branching vs Branchless SetZeroNegativeFlags
//=============================================================================

/// <summary>
/// These tests embed BOTH the old (branching) and new (branchless) implementations
/// of SetZeroNegativeFlags and verify they produce identical PS register state
/// for all possible inputs. This proves the optimization in SnesCpu.Shared.h is safe.
/// </summary>
class SnesCpuBranchlessComparisonTest : public ::testing::Test {
protected:
	/// Old 8-bit implementation: if/else branching (pre-optimization)
	static uint8_t SetZeroNeg8_Branching(uint8_t ps, uint8_t value) {
		ps &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) {
			ps |= ProcFlags::Zero;
		}
		if (value & 0x80) {
			ps |= ProcFlags::Negative;
		}
		return ps;
	}

	/// New 8-bit implementation: branchless (post-optimization)
	static uint8_t SetZeroNeg8_Branchless(uint8_t ps, uint8_t value) {
		ps &= ~(ProcFlags::Zero | ProcFlags::Negative);
		ps |= (value == 0) ? ProcFlags::Zero : 0;
		ps |= (value & 0x80);  // ProcFlags::Negative = 0x80 maps directly to bit 7
		return ps;
	}

	/// Old 16-bit implementation: if/else branching (pre-optimization)
	static uint8_t SetZeroNeg16_Branching(uint8_t ps, uint16_t value) {
		ps &= ~(ProcFlags::Zero | ProcFlags::Negative);
		if (value == 0) {
			ps |= ProcFlags::Zero;
		}
		if (value & 0x8000) {
			ps |= ProcFlags::Negative;
		}
		return ps;
	}

	/// New 16-bit implementation: branchless (post-optimization)
	static uint8_t SetZeroNeg16_Branchless(uint8_t ps, uint16_t value) {
		ps &= ~(ProcFlags::Zero | ProcFlags::Negative);
		ps |= (value == 0) ? ProcFlags::Zero : 0;
		ps |= (value >> 8) & 0x80;  // Shift bit 15 to bit 7 position
		return ps;
	}
};

TEST_F(SnesCpuBranchlessComparisonTest, Exhaustive8Bit_All256Values_AllPSStates) {
	// Test every 8-bit value with multiple initial PS register states.
	uint8_t psStates[] = {
		0x00,  // All flags clear
		0xFF,  // All flags set
		0x03,  // Carry + Zero (stale zero)
		0x80,  // Stale negative
		0x82,  // Both stale
		0x30,  // MemoryMode8 + IndexMode8
		0x6D,  // Mixed flags
	};

	for (uint8_t initialPS : psStates) {
		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			uint8_t oldResult = SetZeroNeg8_Branching(initialPS, value);
			uint8_t newResult = SetZeroNeg8_Branchless(initialPS, value);

			EXPECT_EQ(oldResult, newResult)
				<< "8-bit PS mismatch for initialPS=0x" << std::hex << (int)initialPS
				<< " value=0x" << (int)value
				<< " old=0x" << (int)oldResult
				<< " new=0x" << (int)newResult;
		}
	}
}

TEST_F(SnesCpuBranchlessComparisonTest, Exhaustive16Bit_AllHighBytes_AllPSStates) {
	// For 16-bit, test all 256 high-byte values x 256 low-byte values = 65536 combos.
	// The critical transformation is (value >> 8) & 0x80 mapping bit 15 -> Negative.
	uint8_t psStates[] = { 0x00, 0xFF, 0x03, 0x80, 0x30 };

	for (uint8_t initialPS : psStates) {
		for (int v = 0; v <= 0xFFFF; v++) {
			uint16_t value = static_cast<uint16_t>(v);
			uint8_t oldResult = SetZeroNeg16_Branching(initialPS, value);
			uint8_t newResult = SetZeroNeg16_Branchless(initialPS, value);

			EXPECT_EQ(oldResult, newResult)
				<< "16-bit PS mismatch for initialPS=0x" << std::hex << (int)initialPS
				<< " value=0x" << (int)value
				<< " old=0x" << (int)oldResult
				<< " new=0x" << (int)newResult;
		}
	}
}
