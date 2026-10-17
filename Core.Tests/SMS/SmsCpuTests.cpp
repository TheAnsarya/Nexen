#include "pch.h"
#include "SMS/SmsTypes.h"

//=============================================================================
// SMS CPU Flag Operations
//=============================================================================

/// <summary>
/// Tests for SMS Z80 CPU flag operations.
/// The SMS Z80 has a different flag layout from 6502-family CPUs:
/// Sign=0x80, Zero=0x40, F5=0x20, HalfCarry=0x10, F3=0x08, Parity=0x04, AddSub=0x02, Carry=0x01
/// </summary>
class SmsCpuTypesTest : public ::testing::Test {
protected:
	SmsCpuState _state{};

	void SetUp() override {
		_state = {};
		_state.Flags = 0;
		_state.FlagsChanged = 0;
	}

	void SetFlag(uint8_t flag) { _state.Flags |= flag; }
	void ClearFlag(uint8_t flag) { _state.Flags &= ~flag; }
	bool CheckFlag(uint8_t flag) { return (_state.Flags & flag) == flag; }
};

TEST_F(SmsCpuTypesTest, InitialState_AllClear) {
	EXPECT_EQ(_state.Flags, 0);
	EXPECT_EQ(_state.A, 0);
	EXPECT_EQ(_state.B, 0);
	EXPECT_EQ(_state.C, 0);
	EXPECT_EQ(_state.D, 0);
	EXPECT_EQ(_state.E, 0);
	EXPECT_EQ(_state.H, 0);
	EXPECT_EQ(_state.L, 0);
}

TEST_F(SmsCpuTypesTest, FlagLayout_MatchesZ80) {
	EXPECT_EQ(SmsCpuFlags::Carry, 0x01);
	EXPECT_EQ(SmsCpuFlags::AddSub, 0x02);
	EXPECT_EQ(SmsCpuFlags::Parity, 0x04);
	EXPECT_EQ(SmsCpuFlags::F3, 0x08);
	EXPECT_EQ(SmsCpuFlags::HalfCarry, 0x10);
	EXPECT_EQ(SmsCpuFlags::F5, 0x20);
	EXPECT_EQ(SmsCpuFlags::Zero, 0x40);
	EXPECT_EQ(SmsCpuFlags::Sign, 0x80);
}

TEST_F(SmsCpuTypesTest, SetFlag_SetsOnlyTargetFlag) {
	SetFlag(SmsCpuFlags::Zero);
	EXPECT_TRUE(CheckFlag(SmsCpuFlags::Zero));
	EXPECT_FALSE(CheckFlag(SmsCpuFlags::Carry));
	EXPECT_FALSE(CheckFlag(SmsCpuFlags::Sign));
}

TEST_F(SmsCpuTypesTest, ClearFlag_ClearsOnlyTargetFlag) {
	_state.Flags = 0xFF;
	ClearFlag(SmsCpuFlags::Carry);
	EXPECT_FALSE(CheckFlag(SmsCpuFlags::Carry));
	EXPECT_TRUE(CheckFlag(SmsCpuFlags::Zero));
	EXPECT_TRUE(CheckFlag(SmsCpuFlags::Sign));
}

//=============================================================================
// Before/After Comparison: Branching vs Branchless SetFlagState
//=============================================================================

/// <summary>
/// SMS SetFlagState comparison tests.
/// Old: if (state) { SetFlag(flag); } else { ClearFlag(flag); }
/// New: flags = (flags & ~flag) | (-static_cast<uint8_t>(state) & flag)
/// The branchless version uses the two's complement trick: -true = 0xFF, -false = 0x00.
/// </summary>
class SmsCpuSetFlagStateComparisonTest : public ::testing::Test {
protected:
	/// Old implementation: branching if/else
	static uint8_t SetFlagState_Branching(uint8_t flags, uint8_t flag, bool state) {
		if (state) {
			flags |= flag;
		} else {
			flags &= ~flag;
		}
		return flags;
	}

	/// New implementation: branchless (matches current SmsCpu.cpp)
	static uint8_t SetFlagState_Branchless(uint8_t flags, uint8_t flag, bool state) {
		return (flags & ~flag) | (-static_cast<uint8_t>(state) & flag);
	}
};

TEST_F(SmsCpuSetFlagStateComparisonTest, Exhaustive_AllFlags_AllStates_AllBools) {
	// Test every combination of initial flags, target flag, and boolean state
	uint8_t testFlags[] = {
		SmsCpuFlags::Carry,
		SmsCpuFlags::AddSub,
		SmsCpuFlags::Parity,
		SmsCpuFlags::F3,
		SmsCpuFlags::HalfCarry,
		SmsCpuFlags::F5,
		SmsCpuFlags::Zero,
		SmsCpuFlags::Sign
	};

	// Test with every possible initial flags register value (0-255)
	for (int initial = 0; initial <= 255; initial++) {
		uint8_t initialFlags = static_cast<uint8_t>(initial);

		for (uint8_t flag : testFlags) {
			for (int b = 0; b <= 1; b++) {
				bool state = (b != 0);
				uint8_t oldResult = SetFlagState_Branching(initialFlags, flag, state);
				uint8_t newResult = SetFlagState_Branchless(initialFlags, flag, state);

				EXPECT_EQ(oldResult, newResult)
					<< "SetFlagState mismatch:"
					<< " initial=0x" << std::hex << (int)initialFlags
					<< " flag=0x" << (int)flag
					<< " state=" << state
					<< " old=0x" << (int)oldResult
					<< " new=0x" << (int)newResult;
			}
		}
	}
}

TEST_F(SmsCpuSetFlagStateComparisonTest, SetFlag_WhenClear_SetsIt) {
	uint8_t result = SetFlagState_Branchless(0x00, SmsCpuFlags::Zero, true);
	EXPECT_EQ(result & SmsCpuFlags::Zero, SmsCpuFlags::Zero);
}

TEST_F(SmsCpuSetFlagStateComparisonTest, SetFlag_WhenAlreadySet_KeepsIt) {
	uint8_t result = SetFlagState_Branchless(SmsCpuFlags::Zero, SmsCpuFlags::Zero, true);
	EXPECT_EQ(result & SmsCpuFlags::Zero, SmsCpuFlags::Zero);
}

TEST_F(SmsCpuSetFlagStateComparisonTest, ClearFlag_WhenSet_ClearsIt) {
	uint8_t result = SetFlagState_Branchless(SmsCpuFlags::Zero, SmsCpuFlags::Zero, false);
	EXPECT_EQ(result & SmsCpuFlags::Zero, 0);
}

TEST_F(SmsCpuSetFlagStateComparisonTest, ClearFlag_WhenAlreadyClear_KeepsItClear) {
	uint8_t result = SetFlagState_Branchless(0x00, SmsCpuFlags::Zero, false);
	EXPECT_EQ(result & SmsCpuFlags::Zero, 0);
}

TEST_F(SmsCpuSetFlagStateComparisonTest, PreservesOtherFlags_WhenSetting) {
	uint8_t initial = SmsCpuFlags::Carry | SmsCpuFlags::Sign;
	uint8_t result = SetFlagState_Branchless(initial, SmsCpuFlags::Zero, true);
	EXPECT_TRUE(result & SmsCpuFlags::Carry);
	EXPECT_TRUE(result & SmsCpuFlags::Sign);
	EXPECT_TRUE(result & SmsCpuFlags::Zero);
}

TEST_F(SmsCpuSetFlagStateComparisonTest, PreservesOtherFlags_WhenClearing) {
	uint8_t initial = SmsCpuFlags::Carry | SmsCpuFlags::Sign | SmsCpuFlags::Zero;
	uint8_t result = SetFlagState_Branchless(initial, SmsCpuFlags::Zero, false);
	EXPECT_TRUE(result & SmsCpuFlags::Carry);
	EXPECT_TRUE(result & SmsCpuFlags::Sign);
	EXPECT_FALSE(result & SmsCpuFlags::Zero);
}

//=============================================================================
// Before/After Comparison: SMS SetStandardFlags
//=============================================================================

/// <summary>
/// Tests for the SMS SetStandardFlags template function.
/// This function conditionally calls SetFlagState for Sign, Zero, F5, F3, and Parity.
/// Since SetFlagState is now branchless, we validate that SetStandardFlags
/// produces correct results for all 256 input values.
/// </summary>
class SmsCpuStandardFlagsTest : public ::testing::Test {
protected:
	/// Parity lookup: true if even number of set bits
	static bool CheckParity(uint8_t value) {
		value ^= value >> 4;
		value ^= value >> 2;
		value ^= value >> 1;
		return (value & 1) == 0;  // Even parity
	}

	/// Simulate SetStandardFlags<Sign|Zero|Parity|F3|F5> using branching SetFlagState
	static uint8_t StandardFlags_Branching(uint8_t flags, uint8_t value) {
		// Sign
		if (value & 0x80) flags |= SmsCpuFlags::Sign;
		else flags &= ~SmsCpuFlags::Sign;

		// Zero
		if (value == 0) flags |= SmsCpuFlags::Zero;
		else flags &= ~SmsCpuFlags::Zero;

		// F5 (bit 5 of value)
		if (value & SmsCpuFlags::F5) flags |= SmsCpuFlags::F5;
		else flags &= ~SmsCpuFlags::F5;

		// F3 (bit 3 of value)
		if (value & SmsCpuFlags::F3) flags |= SmsCpuFlags::F3;
		else flags &= ~SmsCpuFlags::F3;

		// Parity (even parity)
		if (CheckParity(value)) flags |= SmsCpuFlags::Parity;
		else flags &= ~SmsCpuFlags::Parity;

		return flags;
	}

	/// Simulate SetStandardFlags using branchless SetFlagState
	static uint8_t StandardFlags_Branchless(uint8_t flags, uint8_t value) {
		auto SetFlagState = [](uint8_t f, uint8_t flag, bool state) -> uint8_t {
			return (f & ~flag) | (-static_cast<uint8_t>(state) & flag);
		};

		flags = SetFlagState(flags, SmsCpuFlags::Sign, (value & 0x80) != 0);
		flags = SetFlagState(flags, SmsCpuFlags::Zero, value == 0);
		flags = SetFlagState(flags, SmsCpuFlags::F5, (value & SmsCpuFlags::F5) != 0);
		flags = SetFlagState(flags, SmsCpuFlags::F3, (value & SmsCpuFlags::F3) != 0);
		flags = SetFlagState(flags, SmsCpuFlags::Parity, CheckParity(value));
		return flags;
	}
};

TEST_F(SmsCpuStandardFlagsTest, Exhaustive_All256Values_AllInitialFlags) {
	uint8_t flagStates[] = {
		0x00,  // All clear
		0xFF,  // All set
		SmsCpuFlags::Carry | SmsCpuFlags::HalfCarry,  // Arithmetic flags
		SmsCpuFlags::Sign | SmsCpuFlags::Zero,         // S+Z stale
		0x55,  // Alternating bits
		0xAA,  // Alternating bits (other)
	};

	for (uint8_t initialFlags : flagStates) {
		for (int v = 0; v <= 255; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			uint8_t oldResult = StandardFlags_Branching(initialFlags, value);
			uint8_t newResult = StandardFlags_Branchless(initialFlags, value);

			EXPECT_EQ(oldResult, newResult)
				<< "StandardFlags mismatch:"
				<< " initial=0x" << std::hex << (int)initialFlags
				<< " value=0x" << (int)value
				<< " old=0x" << (int)oldResult
				<< " new=0x" << (int)newResult;
		}
	}
}

TEST_F(SmsCpuStandardFlagsTest, ZeroValue_SetsZeroAndParity) {
	uint8_t flags = StandardFlags_Branchless(0x00, 0x00);
	EXPECT_TRUE(flags & SmsCpuFlags::Zero);
	EXPECT_TRUE(flags & SmsCpuFlags::Parity);   // 0 has even parity
	EXPECT_FALSE(flags & SmsCpuFlags::Sign);
	EXPECT_FALSE(flags & SmsCpuFlags::F3);
	EXPECT_FALSE(flags & SmsCpuFlags::F5);
}

TEST_F(SmsCpuStandardFlagsTest, NegativeValue_SetsSign) {
	uint8_t flags = StandardFlags_Branchless(0x00, 0x80);
	EXPECT_TRUE(flags & SmsCpuFlags::Sign);
	EXPECT_FALSE(flags & SmsCpuFlags::Zero);
}

TEST_F(SmsCpuStandardFlagsTest, OddParity_ClearsParity) {
	// 0x01 has 1 bit set => odd parity => Parity flag clear
	uint8_t flags = StandardFlags_Branchless(0x00, 0x01);
	EXPECT_FALSE(flags & SmsCpuFlags::Parity);
}

TEST_F(SmsCpuStandardFlagsTest, EvenParity_SetsParity) {
	// 0x03 has 2 bits set => even parity => Parity flag set
	uint8_t flags = StandardFlags_Branchless(0x00, 0x03);
	EXPECT_TRUE(flags & SmsCpuFlags::Parity);
}

TEST_F(SmsCpuStandardFlagsTest, UndocumentedFlags_CopyFromValue) {
	// F3 (bit 3) and F5 (bit 5) are copied from the value
	uint8_t flags = StandardFlags_Branchless(0x00, 0x28);  // bits 3 and 5 set
	EXPECT_TRUE(flags & SmsCpuFlags::F3);
	EXPECT_TRUE(flags & SmsCpuFlags::F5);
}

//=============================================================================
// Before/After Comparison: SMS UpdateLogicalOpFlags (merged ClearFlag)
//=============================================================================

/// <summary>
/// Tests verifying that the merged ClearFlag optimization in UpdateLogicalOpFlags
/// produces identical results to calling 3 separate ClearFlag calls.
/// After SetStandardFlags<0xEC>, AddSub/Carry/HalfCarry must be cleared.
/// </summary>
class SmsCpuLogicalOpFlagsTest : public ::testing::Test {
protected:
	// Parity lookup table for test reference
	static bool CheckParity(uint8_t value) {
		value ^= value >> 4;
		value ^= value >> 2;
		value ^= value >> 1;
		return (value & 1) == 0;
	}

	// Old: 3 separate ClearFlag calls
	static uint8_t LogicalOpFlags_Separate(uint8_t flags, uint8_t value) {
		// SetStandardFlags<0xEC> sets Sign, Zero, F5, F3, Parity
		// Sign=0x80
		flags = (flags & ~SmsCpuFlags::Sign) | (-static_cast<uint8_t>((value & 0x80) != 0) & SmsCpuFlags::Sign);
		// Zero=0x40
		flags = (flags & ~SmsCpuFlags::Zero) | (-static_cast<uint8_t>(value == 0) & SmsCpuFlags::Zero);
		// F5=0x20
		flags = (flags & ~SmsCpuFlags::F5) | (-static_cast<uint8_t>((value & SmsCpuFlags::F5) != 0) & SmsCpuFlags::F5);
		// F3=0x08
		flags = (flags & ~SmsCpuFlags::F3) | (-static_cast<uint8_t>((value & SmsCpuFlags::F3) != 0) & SmsCpuFlags::F3);
		// Parity=0x04
		flags = (flags & ~SmsCpuFlags::Parity) | (-static_cast<uint8_t>(CheckParity(value)) & SmsCpuFlags::Parity);
		// 3 separate clears
		flags &= ~SmsCpuFlags::AddSub;
		flags &= ~SmsCpuFlags::Carry;
		flags &= ~SmsCpuFlags::HalfCarry;
		return flags;
	}

	// New: single merged AND mask
	static uint8_t LogicalOpFlags_Merged(uint8_t flags, uint8_t value) {
		// SetStandardFlags<0xEC>
		flags = (flags & ~SmsCpuFlags::Sign) | (-static_cast<uint8_t>((value & 0x80) != 0) & SmsCpuFlags::Sign);
		flags = (flags & ~SmsCpuFlags::Zero) | (-static_cast<uint8_t>(value == 0) & SmsCpuFlags::Zero);
		flags = (flags & ~SmsCpuFlags::F5) | (-static_cast<uint8_t>((value & SmsCpuFlags::F5) != 0) & SmsCpuFlags::F5);
		flags = (flags & ~SmsCpuFlags::F3) | (-static_cast<uint8_t>((value & SmsCpuFlags::F3) != 0) & SmsCpuFlags::F3);
		flags = (flags & ~SmsCpuFlags::Parity) | (-static_cast<uint8_t>(CheckParity(value)) & SmsCpuFlags::Parity);
		// Single merged clear
		flags &= ~(SmsCpuFlags::AddSub | SmsCpuFlags::Carry | SmsCpuFlags::HalfCarry);
		return flags;
	}
};

TEST_F(SmsCpuLogicalOpFlagsTest, Exhaustive_256Values_AllFlagStates) {
	uint8_t flagStates[] = { 0x00, 0xFF, 0x13, 0x55, 0xAA, 0xEC };
	for (uint8_t initial : flagStates) {
		for (int v = 0; v < 256; v++) {
			uint8_t value = static_cast<uint8_t>(v);
			uint8_t separate = LogicalOpFlags_Separate(initial, value);
			uint8_t merged = LogicalOpFlags_Merged(initial, value);
			ASSERT_EQ(separate, merged)
				<< "LogicalOpFlags mismatch: flags=0x" << std::hex << (int)initial
				<< " value=0x" << (int)value;
		}
	}
}

TEST_F(SmsCpuLogicalOpFlagsTest, AddSubCarryHalfCarry_AlwaysCleared) {
	// Even if all flags start set, AddSub/Carry/HalfCarry must end cleared
	for (int v = 0; v < 256; v++) {
		uint8_t value = static_cast<uint8_t>(v);
		uint8_t result = LogicalOpFlags_Merged(0xFF, value);
		EXPECT_FALSE(result & SmsCpuFlags::AddSub) << "AddSub not cleared for value=0x" << std::hex << v;
		EXPECT_FALSE(result & SmsCpuFlags::Carry) << "Carry not cleared for value=0x" << std::hex << v;
		EXPECT_FALSE(result & SmsCpuFlags::HalfCarry) << "HalfCarry not cleared for value=0x" << std::hex << v;
	}
}
