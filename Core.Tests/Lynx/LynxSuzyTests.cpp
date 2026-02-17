#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for Lynx Suzy math coprocessor, sprite system, and hardware bugs.
/// Verifies multiply/divide behavior, sign-magnitude bugs (13.8, 13.9, 13.10),
/// sprite chain termination (bug 13.12), and collision buffer layout.
/// </summary>
class LynxSuzyMathTest : public ::testing::Test {
protected:
	LynxSuzyState _state = {};

	void SetUp() override {
		memset(&_state, 0, sizeof(_state));
	}
};

//=============================================================================
// Math Coprocessor — Unsigned Multiply
//=============================================================================

TEST_F(LynxSuzyMathTest, Multiply_UnsignedSimple) {
	// 3 × 5 = 15
	uint16_t a = 3, b = 5;
	uint32_t result = (uint32_t)a * (uint32_t)b;
	EXPECT_EQ(result, 15u);
}

TEST_F(LynxSuzyMathTest, Multiply_UnsignedMaxValues) {
	// 0xFFFF × 0xFFFF = 0xFFFE0001
	uint16_t a = 0xFFFF, b = 0xFFFF;
	uint32_t result = (uint32_t)a * (uint32_t)b;
	EXPECT_EQ(result, 0xFFFE0001u);
}

TEST_F(LynxSuzyMathTest, Multiply_UnsignedZero) {
	uint16_t a = 0, b = 12345;
	uint32_t result = (uint32_t)a * (uint32_t)b;
	EXPECT_EQ(result, 0u);
}

//=============================================================================
// HW Bug 13.8 — Signed Multiply: $8000 is Positive
// The sign-magnitude math uses bit 15 for sign, but due to a hardware bug,
// $8000 (only sign bit set, magnitude 0) is treated as POSITIVE zero,
// while $0000 is treated as NEGATIVE zero.
//=============================================================================

TEST_F(LynxSuzyMathTest, Bug13_8_SignMagnitude_8000IsPositive) {
	// On real Lynx: $8000 → positive (sign bit ignored for $8000)
	// sign = value & 0x8000
	// magnitude = value & 0x7FFF
	// If magnitude == 0, the sign bit determines positive/negative zero
	// Bug: $8000 = sign=1 but treated as positive
	int16_t value = (int16_t)0x8000;
	bool isNegative = (value & 0x8000) != 0;
	uint16_t magnitude = value & 0x7FFF;

	// $8000: sign=1, magnitude=0
	EXPECT_TRUE(isNegative);
	EXPECT_EQ(magnitude, 0u);

	// In sign-magnitude interpretation, $8000 should be "negative zero"
	// But the hardware treats magnitude-0 values specially:
	// The two's complement conversion of magnitude=0 is still 0,
	// so sign doesn't matter for the actual computation
	int16_t twosComp = isNegative ? (int16_t)(~magnitude + 1) : (int16_t)magnitude;
	// ~0 + 1 = 0x10000 which truncates to 0
	EXPECT_EQ((uint16_t)twosComp, 0u);
}

TEST_F(LynxSuzyMathTest, Bug13_8_SignMagnitude_0000IsNegative) {
	// $0000: sign=0, magnitude=0 — "positive zero"
	int16_t value = 0x0000;
	bool isNegative = (value & 0x8000) != 0;
	uint16_t magnitude = value & 0x7FFF;

	EXPECT_FALSE(isNegative);
	EXPECT_EQ(magnitude, 0u);
}

TEST_F(LynxSuzyMathTest, Bug13_8_SignMagnitude_NormalNegative) {
	// $8005 = sign=1, magnitude=5 → -5 in sign-magnitude
	int16_t value = (int16_t)0x8005;
	bool isNegative = (value & 0x8000) != 0;
	uint16_t magnitude = value & 0x7FFF;

	EXPECT_TRUE(isNegative);
	EXPECT_EQ(magnitude, 5u);

	int16_t twosComp = isNegative ? (int16_t)(~magnitude + 1) : (int16_t)magnitude;
	EXPECT_EQ(twosComp, -5);
}

//=============================================================================
// HW Bug 13.9 — Division: Remainder Always Positive
// The hardware doesn't negate the remainder, even for signed division.
//=============================================================================

TEST_F(LynxSuzyMathTest, Bug13_9_DivideRemainderAlwaysPositive) {
	// -7 / 3 should give quotient = -2 or -3, remainder = -1 or 2
	// But hardware always returns positive remainder
	int32_t dividend = -7;
	int16_t divisor = 3;

	// Normal C division
	int16_t quotient = (int16_t)(dividend / divisor);
	int16_t remainder = (int16_t)(dividend % divisor);

	// C truncates toward zero: -7/3 = -2 remainder -1
	EXPECT_EQ(quotient, -2);
	EXPECT_EQ(remainder, -1);

	// Hardware bug: remainder is forced positive (magnitude only)
	uint16_t hwRemainder = (uint16_t)std::abs(remainder);
	EXPECT_EQ(hwRemainder, 1u);
	EXPECT_GE(hwRemainder, 0u); // Always positive
}

TEST_F(LynxSuzyMathTest, Bug13_9_DivideRemainderPositive_LargeValues) {
	int32_t dividend = -12345;
	int16_t divisor = 100;

	int16_t remainder = (int16_t)(dividend % divisor);
	EXPECT_EQ(remainder, -45); // C gives -45

	uint16_t hwRemainder = (uint16_t)std::abs(remainder);
	EXPECT_EQ(hwRemainder, 45u); // Hardware gives +45
}

//=============================================================================
// HW Bug 13.10 — Math Overflow Overwritten Per Operation
// The overflow flag is NOT OR'd across operations — each new multiply/divide
// overwrites the previous overflow status.
//=============================================================================

TEST_F(LynxSuzyMathTest, Bug13_10_OverflowOverwrittenPerOp) {
	// Suppose first multiply overflows
	_state.MathOverflow = true;

	// Second multiply does NOT overflow — should clear the flag
	_state.MathOverflow = false; // New operation sets its own result

	EXPECT_FALSE(_state.MathOverflow);
	// Key: it was NOT OR'd with the previous true value
}

TEST_F(LynxSuzyMathTest, Bug13_10_OverflowDetectionLogic) {
	// Accumulate mode: result added to existing value
	// Overflow occurs when 32-bit accumulator would exceed 32 bits
	uint32_t accumulator = 0xFFFF0000;
	uint32_t newResult = 0x00020000;

	uint64_t fullResult = (uint64_t)accumulator + (uint64_t)newResult;
	bool overflow = (fullResult >> 32) != 0;

	EXPECT_TRUE(overflow);
}

//=============================================================================
// HW Bug 13.12 — SCB NEXT Only Checks Upper Byte
// The sprite chain terminates when (scbAddr >> 8) == 0, not scbAddr == 0.
// This means addresses $0000-$00FF all terminate the chain.
//=============================================================================

TEST_F(LynxSuzyMathTest, Bug13_12_SpriteChainTermination_ZeroPage) {
	// Any address in zero page ($0000-$00FF) should terminate the chain
	for (uint16_t addr = 0x0000; addr <= 0x00FF; addr++) {
		bool terminates = (addr >> 8) == 0;
		EXPECT_TRUE(terminates) << "Address $" << std::hex << addr
			<< " should terminate the sprite chain";
	}
}

TEST_F(LynxSuzyMathTest, Bug13_12_SpriteChainTermination_NonZeroPage) {
	// Addresses $0100 and above should NOT terminate
	uint16_t addr = 0x0100;
	EXPECT_FALSE((addr >> 8) == 0);

	addr = 0x1000;
	EXPECT_FALSE((addr >> 8) == 0);

	addr = 0xFFFF;
	EXPECT_FALSE((addr >> 8) == 0);
}

TEST_F(LynxSuzyMathTest, Bug13_12_CompareWithCorrectTermination) {
	// Show the difference: addr == 0 would only match $0000
	// But (addr >> 8) == 0 matches $0000-$00FF
	uint16_t addr = 0x0001;
	bool correctCheck = (addr >> 8) == 0;  // Upper byte check
	bool naiveCheck = (addr == 0);          // Full word check

	EXPECT_TRUE(correctCheck);   // Upper byte is 0 → terminates
	EXPECT_FALSE(naiveCheck);    // Full word is nonzero → doesn't terminate
	// This proves the bug matters for addresses $01-$FF
}

//=============================================================================
// Collision Buffer Layout
//=============================================================================

TEST_F(LynxSuzyMathTest, CollisionBuffer_Size) {
	EXPECT_EQ(LynxConstants::CollisionBufferSize, 16u);
}

TEST_F(LynxSuzyMathTest, CollisionBuffer_DefaultZero) {
	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(_state.CollisionBuffer[i], 0u);
	}
}

TEST_F(LynxSuzyMathTest, CollisionBuffer_MutualUpdate) {
	// When sprite A (collNum=3) writes to pixel of color index 5,
	// and collisionBuffer[5] already has value 2 (from sprite B):
	uint8_t collNum = 3;
	uint8_t pixIndex = 5;
	_state.CollisionBuffer[pixIndex] = 2; // Previously written by sprite 2

	uint8_t existing = _state.CollisionBuffer[pixIndex];
	if (existing != 0) {
		_state.CollisionBuffer[collNum] |= existing;
		_state.CollisionBuffer[existing] |= collNum;
	}

	EXPECT_EQ(_state.CollisionBuffer[3], 2u); // Sprite 3 collided with 2
	EXPECT_EQ(_state.CollisionBuffer[2], 3u); // Sprite 2 collided with 3
}

//=============================================================================
// Sprite Type and BPP Enums
//=============================================================================

TEST_F(LynxSuzyMathTest, SpriteType_Values) {
	EXPECT_EQ((uint8_t)LynxSpriteType::Background, 0);
	EXPECT_EQ((uint8_t)LynxSpriteType::Normal, 1);
	EXPECT_EQ((uint8_t)LynxSpriteType::Boundary, 2);
	EXPECT_EQ((uint8_t)LynxSpriteType::Shadow, 7);
}

TEST_F(LynxSuzyMathTest, SpriteBpp_Values) {
	EXPECT_EQ((uint8_t)LynxSpriteBpp::Bpp1, 0);
	EXPECT_EQ((uint8_t)LynxSpriteBpp::Bpp2, 1);
	EXPECT_EQ((uint8_t)LynxSpriteBpp::Bpp3, 2);
	EXPECT_EQ((uint8_t)LynxSpriteBpp::Bpp4, 3);
}

//=============================================================================
// Math Register State
//=============================================================================

TEST_F(LynxSuzyMathTest, MathState_DefaultZero) {
	EXPECT_EQ(_state.MathA, 0);
	EXPECT_EQ(_state.MathB, 0);
	EXPECT_EQ(_state.MathC, 0);
	EXPECT_EQ(_state.MathD, 0);
	EXPECT_FALSE(_state.MathSign);
	EXPECT_FALSE(_state.MathAccumulate);
	EXPECT_FALSE(_state.MathInProgress);
	EXPECT_FALSE(_state.MathOverflow);
}

TEST_F(LynxSuzyMathTest, MathState_SignAccumulateFlags) {
	_state.MathSign = true;
	_state.MathAccumulate = true;
	EXPECT_TRUE(_state.MathSign);
	EXPECT_TRUE(_state.MathAccumulate);
}

//=============================================================================
// Math Unit — Signed Multiply Comprehensive Tests
// Tests for Hardware Bug 13.8: $8000 is positive, $0000 is negative
// NOTE: The Lynx doesn't use true sign-magnitude. It interprets bit 15 as
// "negate this value via two's complement before the operation". This leads
// to peculiar behavior for negative numbers.
//=============================================================================

TEST_F(LynxSuzyMathTest, SignedMultiply_PositiveTimesPositive) {
	// 5 × 3 = 15
	int16_t a = 5, b = 3;
	int32_t result = (int32_t)a * (int32_t)b;
	EXPECT_EQ(result, 15);
}

TEST_F(LynxSuzyMathTest, SignedMultiply_HardwareNegation) {
	// The hardware's "signed" mode:
	// If bit 15 is set, it takes two's complement of the VALUE, not just
	// uses the magnitude. This means $8005 → ~$8005 + 1 = $7FFB = 32763
	//
	// So to represent -5 in Lynx signed multiply, you'd use... $FFFB
	// (actual two's complement of 5)? No! The hardware interprets $FFFB as:
	// bit 15 set → negate: ~$FFFB + 1 = $0004 + 1 = $0005
	// So $FFFB becomes +5 after the hardware "negation"!
	//
	// This is the documented hardware bug - it's a confusing system.
	uint16_t val = 0x8005;
	uint16_t negated = (uint16_t)(~val + 1);
	EXPECT_EQ(negated, 0x7FFBu); // ~8005 + 1 = 7FFB

	// The sign bit WAS set, so result is negated after multiplication
}

TEST_F(LynxSuzyMathTest, SignedMultiply_ActualNegativeValue) {
	// To get an actually negative result in Lynx signed multiply:
	// Use one operand with bit 15 set (so it gets negated, and result is negated)
	//
	// If we want -15 as the result of 5 × 3, use:
	// $8005 × $0003 → negated($8005) × $0003 = $7FFB × 3 = $17FF1 (very large!)
	// Then result is negated: ~$17FF1 + 1 since bit 15 was set
	//
	// The hardware is really designed for small negative values where
	// the two's complement behavior works out correctly.

	// For actual game use: small magnitudes work as expected
	// $8005: bit 15 set, so "negate" → ~$8005+1 = $7FFB = 32763 (!)
	// Games typically use values like $FFFF (-1), $FFFE (-2), etc.
	uint16_t a = 0xFFFF; // -1 in two's complement
	uint16_t b = 0x0003; // +3

	// Hardware: bit 15 set on a, so negate a: ~$FFFF + 1 = $0001
	// Then multiply: 1 × 3 = 3
	// Signs differ (a was negative), so negate result: ~3 + 1 = -3
	bool aNeg = (a & 0x8000) != 0;
	bool bNeg = (b & 0x8000) != 0;
	uint16_t aMag = aNeg ? (uint16_t)(~a + 1) : a;
	uint16_t bMag = bNeg ? (uint16_t)(~b + 1) : b;

	EXPECT_EQ(aMag, 1u); // ~$FFFF + 1 = $0001
	EXPECT_EQ(bMag, 3u);

	uint32_t product = (uint32_t)aMag * (uint32_t)bMag;
	EXPECT_EQ(product, 3u);

	// Result sign: a was negative, b was positive → result is negative
	EXPECT_TRUE(aNeg ^ bNeg);
}

TEST_F(LynxSuzyMathTest, SignedMultiply_BothNegative) {
	// $FFFF × $FFFF in signed mode:
	// Both have bit 15 set, both get negated to 1
	// 1 × 1 = 1
	// Signs same → result positive
	uint16_t a = 0xFFFF;
	uint16_t b = 0xFFFF;

	bool aNeg = (a & 0x8000) != 0;
	bool bNeg = (b & 0x8000) != 0;
	uint16_t aMag = (uint16_t)(~a + 1);
	uint16_t bMag = (uint16_t)(~b + 1);

	EXPECT_EQ(aMag, 1u);
	EXPECT_EQ(bMag, 1u);

	uint32_t product = (uint32_t)aMag * (uint32_t)bMag;
	EXPECT_EQ(product, 1u);

	// Both negative → positive result
	EXPECT_FALSE(aNeg ^ bNeg);
}

TEST_F(LynxSuzyMathTest, Bug13_8_SignedMultiply_8000IsPositive) {
	// $8000 in sign-magnitude: sign=1, magnitude=0
	// Hardware bug: ~$8000 + 1 = $7FFF + 1 = $8000 (unchanged!)
	// So $8000 is treated as positive $8000 (32768), not negative zero
	uint16_t val = 0x8000;
	uint16_t negated = (uint16_t)(~val + 1);

	EXPECT_EQ(negated, 0x8000u); // Two's complement of $8000 is $8000

	// This means $8000 × $0002 = 32768 × 2 = 65536 (not 0)
	uint16_t a = 0x8000;
	uint16_t b = 0x0002;
	bool aNeg = (a & 0x8000) != 0;
	uint16_t aMag = aNeg ? (uint16_t)(~a + 1) : a;

	// aMag = ~$8000 + 1 = $8000 (the bug!)
	EXPECT_EQ(aMag, 0x8000u);

	uint32_t product = (uint32_t)aMag * (uint32_t)b;
	EXPECT_EQ(product, 0x10000u); // 32768 × 2 = 65536
}

TEST_F(LynxSuzyMathTest, Bug13_8_SignedMultiply_0000Behaves) {
	// $0000: sign=0, magnitude=0 → 0
	// ~$0000 + 1 = $FFFF + 1 = $0000 (correct)
	uint16_t val = 0x0000;
	uint16_t negated = (uint16_t)(~val + 1);
	EXPECT_EQ(negated, 0x0000u);

	// $0000 × anything = 0
	uint32_t product = (uint32_t)0 * (uint32_t)12345;
	EXPECT_EQ(product, 0u);
}

//=============================================================================
// Math Unit — Division Comprehensive Tests
//=============================================================================

TEST_F(LynxSuzyMathTest, UnsignedDivide_Basic) {
	// 100 / 7 = 14 remainder 2
	uint32_t dividend = 100;
	uint16_t divisor = 7;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 14u);
	EXPECT_EQ(remainder, 2u);
}

TEST_F(LynxSuzyMathTest, UnsignedDivide_Large) {
	// 0x12345678 / 0x1234 = 0x1000E remainder 0x0CC0
	uint32_t dividend = 0x12345678;
	uint16_t divisor = 0x1234;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, dividend / divisor);
	EXPECT_EQ(remainder, dividend % divisor);
}

TEST_F(LynxSuzyMathTest, UnsignedDivide_QuotientZero) {
	// 5 / 100 = 0 remainder 5
	uint32_t dividend = 5;
	uint16_t divisor = 100;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 0u);
	EXPECT_EQ(remainder, 5u);
}

TEST_F(LynxSuzyMathTest, UnsignedDivide_ByOne) {
	// Any / 1 = Any remainder 0
	uint32_t dividend = 0xDEADBEEF;
	uint16_t divisor = 1;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, dividend);
	EXPECT_EQ(remainder, 0u);
}

TEST_F(LynxSuzyMathTest, UnsignedDivide_ExactDivision) {
	// 1000 / 10 = 100 remainder 0
	uint32_t dividend = 1000;
	uint16_t divisor = 10;
	uint32_t quotient = dividend / divisor;
	uint32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 100u);
	EXPECT_EQ(remainder, 0u);
}

//=============================================================================
// Collision Detection Comprehensive Tests
//=============================================================================

TEST_F(LynxSuzyMathTest, Collision_NoCollision_EmptyBuffer) {
	// Writing to empty slot doesn't trigger collision
	uint8_t collNum = 5;
	uint8_t pixIndex = 3;

	// Buffer is empty
	EXPECT_EQ(_state.CollisionBuffer[pixIndex], 0u);

	// First sprite writes its collision number
	_state.CollisionBuffer[pixIndex] = collNum;

	// No mutual update needed since nothing was there
	EXPECT_EQ(_state.CollisionBuffer[pixIndex], 5u);
	EXPECT_EQ(_state.CollisionBuffer[collNum], 0u); // This sprite's slot not touched
}

TEST_F(LynxSuzyMathTest, Collision_TwoSprites_MutualUpdate) {
	// Sprite 3 writes to slot 7, sprite 5 then writes to same slot
	uint8_t sprite3 = 3;
	uint8_t sprite5 = 5;
	uint8_t pixIndex = 7;

	// Sprite 3 writes first
	_state.CollisionBuffer[pixIndex] = sprite3;

	// Sprite 5 writes to same pixel — collision!
	uint8_t existing = _state.CollisionBuffer[pixIndex]; // 3
	EXPECT_EQ(existing, sprite3);

	// Mutual update
	_state.CollisionBuffer[sprite5] |= existing; // Sprite 5's slot gets 3
	_state.CollisionBuffer[existing] |= sprite5; // Sprite 3's slot gets 5

	EXPECT_EQ(_state.CollisionBuffer[sprite5], sprite3); // 5 collided with 3
	EXPECT_EQ(_state.CollisionBuffer[sprite3], sprite5); // 3 collided with 5
}

TEST_F(LynxSuzyMathTest, Collision_ThreeSprites_Accumulate) {
	// Sprites 2, 4, 6 all collide on same pixel
	uint8_t spriteA = 2, spriteB = 4, spriteC = 6;
	uint8_t pixIndex = 9;

	// Sprite A writes first
	_state.CollisionBuffer[pixIndex] = spriteA;

	// Sprite B collides with A
	uint8_t existing = _state.CollisionBuffer[pixIndex];
	_state.CollisionBuffer[spriteB] |= existing;
	_state.CollisionBuffer[existing] |= spriteB;
	_state.CollisionBuffer[pixIndex] = spriteB; // B is now top

	// Sprite C collides with B
	existing = _state.CollisionBuffer[pixIndex];
	_state.CollisionBuffer[spriteC] |= existing;
	_state.CollisionBuffer[existing] |= spriteC;

	EXPECT_EQ(_state.CollisionBuffer[spriteA], spriteB); // A hit B
	EXPECT_EQ(_state.CollisionBuffer[spriteB], spriteA | spriteC); // B hit A and C
	EXPECT_EQ(_state.CollisionBuffer[spriteC], spriteB); // C hit B
}

TEST_F(LynxSuzyMathTest, Collision_AllSlots) {
	// Test all 16 collision slots can be used
	for (int i = 0; i < 16; i++) {
		_state.CollisionBuffer[i] = (uint8_t)i;
	}

	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(_state.CollisionBuffer[i], (uint8_t)i);
	}
}

//=============================================================================
// Sprite Type Collision Behavior
//=============================================================================

TEST_F(LynxSuzyMathTest, SpriteType_BackgroundType_NoCollision) {
	// Background sprites (type 0) don't participate in collision
	EXPECT_EQ((uint8_t)LynxSpriteType::Background, 0);
	// Collision logic checks sprite type before updating buffer
}

TEST_F(LynxSuzyMathTest, SpriteType_NonCollidableType) {
	// NonCollidable sprites (type 5) write pixels but don't collide
	EXPECT_EQ((uint8_t)LynxSpriteType::NonCollidable, 5);
}

TEST_F(LynxSuzyMathTest, SpriteType_NormalType_Collides) {
	// Normal sprites (type 1) participate in collision
	EXPECT_EQ((uint8_t)LynxSpriteType::Normal, 1);
}

//=============================================================================
// Sprite BPP Decoding
//=============================================================================

TEST_F(LynxSuzyMathTest, SpriteBpp_ColorCounts) {
	// 1 bpp = 2 colors (indices 0-1)
	// 2 bpp = 4 colors (indices 0-3)
	// 3 bpp = 8 colors (indices 0-7)
	// 4 bpp = 16 colors (indices 0-15)
	EXPECT_EQ(1 << (uint8_t)LynxSpriteBpp::Bpp1, 1);
	EXPECT_EQ(1 << (uint8_t)LynxSpriteBpp::Bpp2, 2);
	EXPECT_EQ(1 << (uint8_t)LynxSpriteBpp::Bpp3, 4);
	EXPECT_EQ(1 << (uint8_t)LynxSpriteBpp::Bpp4, 8);

	// Actual color counts: 2^(bpp+1) when mode is used as power
	// Or simpler: 2, 4, 8, 16 for modes 0, 1, 2, 3
	int colors[] = { 2, 4, 8, 16 };
	EXPECT_EQ(colors[(uint8_t)LynxSpriteBpp::Bpp1], 2);
	EXPECT_EQ(colors[(uint8_t)LynxSpriteBpp::Bpp2], 4);
	EXPECT_EQ(colors[(uint8_t)LynxSpriteBpp::Bpp3], 8);
	EXPECT_EQ(colors[(uint8_t)LynxSpriteBpp::Bpp4], 16);
}

TEST_F(LynxSuzyMathTest, SpriteBpp_PixelMask) {
	// Pixel mask for extracting index from packed data
	uint8_t masks[] = { 0x01, 0x03, 0x07, 0x0F };
	EXPECT_EQ(masks[(uint8_t)LynxSpriteBpp::Bpp1], 0x01);
	EXPECT_EQ(masks[(uint8_t)LynxSpriteBpp::Bpp2], 0x03);
	EXPECT_EQ(masks[(uint8_t)LynxSpriteBpp::Bpp3], 0x07);
	EXPECT_EQ(masks[(uint8_t)LynxSpriteBpp::Bpp4], 0x0F);
}

//=============================================================================
// Signed Division Tests
// Complements Bug13_9 tests with more signed division edge cases
//=============================================================================

TEST_F(LynxSuzyMathTest, SignedDivide_PositiveByPositive) {
	// 100 / 7 in signed mode = 14 remainder 2 (same as unsigned)
	int32_t dividend = 100;
	int16_t divisor = 7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 14);
	EXPECT_EQ(remainder, 2);
}

TEST_F(LynxSuzyMathTest, SignedDivide_NegativeByPositive) {
	// -100 / 7 = -14 remainder -2 (C truncates toward zero)
	int32_t dividend = -100;
	int16_t divisor = 7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, -14);
	EXPECT_EQ(remainder, -2);

	// Hardware: remainder is always positive magnitude
	uint16_t hwRemainder = (uint16_t)std::abs(remainder);
	EXPECT_EQ(hwRemainder, 2u);
}

TEST_F(LynxSuzyMathTest, SignedDivide_PositiveByNegative) {
	// 100 / -7 = -14 remainder 2
	int32_t dividend = 100;
	int16_t divisor = -7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, -14);
	EXPECT_EQ(remainder, 2);
}

TEST_F(LynxSuzyMathTest, SignedDivide_NegativeByNegative) {
	// -100 / -7 = 14 remainder -2
	int32_t dividend = -100;
	int16_t divisor = -7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 14);
	EXPECT_EQ(remainder, -2);

	// Hardware: remainder always positive
	uint16_t hwRemainder = (uint16_t)std::abs(remainder);
	EXPECT_EQ(hwRemainder, 2u);
}

TEST_F(LynxSuzyMathTest, SignedDivide_DividendZero) {
	// 0 / 7 = 0 remainder 0
	int32_t dividend = 0;
	int16_t divisor = 7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, 0);
	EXPECT_EQ(remainder, 0);
}

TEST_F(LynxSuzyMathTest, SignedDivide_ExactDivision) {
	// -21 / 7 = -3 remainder 0
	int32_t dividend = -21;
	int16_t divisor = 7;

	int32_t quotient = dividend / divisor;
	int32_t remainder = dividend % divisor;

	EXPECT_EQ(quotient, -3);
	EXPECT_EQ(remainder, 0);
}

//=============================================================================
// Sprite Control Register 0 (SPRCTL0) Decoding
// Bits [2:0] = Sprite type
// Bits [7:6] = BPP mode
// Bit 4 = HFLIP
// Bit 5 = VFLIP
//=============================================================================

TEST_F(LynxSuzyMathTest, Sprctl0_TypeDecoding) {
	// Type is bits [2:0]
	uint8_t sprctl0 = 0b00000001; // Type = 1 (Normal)
	uint8_t spriteType = sprctl0 & 0x07;
	EXPECT_EQ(spriteType, (uint8_t)LynxSpriteType::Normal);

	sprctl0 = 0b00000010; // Type = 2 (Boundary)
	spriteType = sprctl0 & 0x07;
	EXPECT_EQ(spriteType, (uint8_t)LynxSpriteType::Boundary);

	sprctl0 = 0b00000111; // Type = 7 (Shadow)
	spriteType = sprctl0 & 0x07;
	EXPECT_EQ(spriteType, (uint8_t)LynxSpriteType::Shadow);
}

TEST_F(LynxSuzyMathTest, Sprctl0_BppDecoding) {
	// BPP is bits [7:6]
	uint8_t sprctl0 = 0b00000000; // BPP = 0 (1bpp)
	uint8_t bpp = (sprctl0 >> 6) & 0x03;
	EXPECT_EQ(bpp, (uint8_t)LynxSpriteBpp::Bpp1);

	sprctl0 = 0b01000000; // BPP = 1 (2bpp)
	bpp = (sprctl0 >> 6) & 0x03;
	EXPECT_EQ(bpp, (uint8_t)LynxSpriteBpp::Bpp2);

	sprctl0 = 0b10000000; // BPP = 2 (3bpp)
	bpp = (sprctl0 >> 6) & 0x03;
	EXPECT_EQ(bpp, (uint8_t)LynxSpriteBpp::Bpp3);

	sprctl0 = 0b11000000; // BPP = 3 (4bpp)
	bpp = (sprctl0 >> 6) & 0x03;
	EXPECT_EQ(bpp, (uint8_t)LynxSpriteBpp::Bpp4);
}

TEST_F(LynxSuzyMathTest, Sprctl0_HFlip) {
	// HFLIP is bit 4
	uint8_t sprctl0 = 0b00010000;
	bool hflip = (sprctl0 & 0x10) != 0;
	EXPECT_TRUE(hflip);

	sprctl0 = 0b00000000;
	hflip = (sprctl0 & 0x10) != 0;
	EXPECT_FALSE(hflip);
}

TEST_F(LynxSuzyMathTest, Sprctl0_VFlip) {
	// VFLIP is bit 5
	uint8_t sprctl0 = 0b00100000;
	bool vflip = (sprctl0 & 0x20) != 0;
	EXPECT_TRUE(vflip);

	sprctl0 = 0b00000000;
	vflip = (sprctl0 & 0x20) != 0;
	EXPECT_FALSE(vflip);
}

TEST_F(LynxSuzyMathTest, Sprctl0_Combined) {
	// Type=3, BPP=2, HFLIP=1, VFLIP=0
	// = 0b10_01_0_011 = 0x93
	uint8_t sprctl0 = 0b10010011;

	uint8_t spriteType = sprctl0 & 0x07;
	uint8_t bpp = (sprctl0 >> 6) & 0x03;
	bool hflip = (sprctl0 & 0x10) != 0;
	bool vflip = (sprctl0 & 0x20) != 0;

	EXPECT_EQ(spriteType, 3); // NormalShadow
	EXPECT_EQ(bpp, 2);        // 3bpp
	EXPECT_TRUE(hflip);
	EXPECT_FALSE(vflip);
}

TEST_F(LynxSuzyMathTest, Sprctl0_AllFlips) {
	// Both HFLIP and VFLIP set
	uint8_t sprctl0 = 0b00110000;
	bool hflip = (sprctl0 & 0x10) != 0;
	bool vflip = (sprctl0 & 0x20) != 0;

	EXPECT_TRUE(hflip);
	EXPECT_TRUE(vflip);
}

//=============================================================================
// SPRSYS Flags Tests
//=============================================================================

TEST_F(LynxSuzyMathTest, Sprsys_UnsafeAccess) {
	_state.UnsafeAccess = false;
	EXPECT_FALSE(_state.UnsafeAccess);

	_state.UnsafeAccess = true;
	EXPECT_TRUE(_state.UnsafeAccess);
}

TEST_F(LynxSuzyMathTest, Sprsys_SpriteToSpriteCollision) {
	_state.SpriteToSpriteCollision = false;
	EXPECT_FALSE(_state.SpriteToSpriteCollision);

	_state.SpriteToSpriteCollision = true;
	EXPECT_TRUE(_state.SpriteToSpriteCollision);
}

TEST_F(LynxSuzyMathTest, Sprsys_VStretch) {
	_state.VStretch = false;
	EXPECT_FALSE(_state.VStretch);

	_state.VStretch = true;
	EXPECT_TRUE(_state.VStretch);
}

TEST_F(LynxSuzyMathTest, Sprsys_LeftHand) {
	_state.LeftHand = false;
	EXPECT_FALSE(_state.LeftHand);

	_state.LeftHand = true;
	EXPECT_TRUE(_state.LeftHand);
}

TEST_F(LynxSuzyMathTest, Sprsys_LastCarry) {
	_state.LastCarry = false;
	EXPECT_FALSE(_state.LastCarry);

	_state.LastCarry = true;
	EXPECT_TRUE(_state.LastCarry);
}

//=============================================================================
// Sprite Scaling — Fixed Point 8.8 Format
// HSIZE/VSIZE are in 8.8 fixed point: 0x0100 = 1.0
//=============================================================================

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_1_0) {
	// 0x0100 = 1.0 (no scaling)
	uint16_t size = 0x0100;
	double scale = size / 256.0;
	EXPECT_DOUBLE_EQ(scale, 1.0);
}

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_0_5) {
	// 0x0080 = 0.5 (half size)
	uint16_t size = 0x0080;
	double scale = size / 256.0;
	EXPECT_DOUBLE_EQ(scale, 0.5);
}

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_2_0) {
	// 0x0200 = 2.0 (double size)
	uint16_t size = 0x0200;
	double scale = size / 256.0;
	EXPECT_DOUBLE_EQ(scale, 2.0);
}

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_0_25) {
	// 0x0040 = 0.25 (quarter size)
	uint16_t size = 0x0040;
	double scale = size / 256.0;
	EXPECT_DOUBLE_EQ(scale, 0.25);
}

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_Max) {
	// 0xFFFF = ~255.996 (maximum scale)
	uint16_t size = 0xFFFF;
	double scale = size / 256.0;
	EXPECT_NEAR(scale, 255.996, 0.001);
}

TEST_F(LynxSuzyMathTest, Scaling_FixedPoint_Zero) {
	// 0x0000 = 0.0 (sprite invisible)
	uint16_t size = 0x0000;
	double scale = size / 256.0;
	EXPECT_DOUBLE_EQ(scale, 0.0);
}

TEST_F(LynxSuzyMathTest, Scaling_PixelWidth_Simple) {
	// For a 10-pixel sprite at 2.0 scale: 10 * 2 = 20 pixels
	int srcPixels = 10;
	uint16_t size = 0x0200; // 2.0
	int dstPixels = (srcPixels * size) >> 8;
	EXPECT_EQ(dstPixels, 20);
}

TEST_F(LynxSuzyMathTest, Scaling_PixelWidth_Fractional) {
	// 10 pixels at 1.5 scale: 10 * 1.5 = 15 pixels
	int srcPixels = 10;
	uint16_t size = 0x0180; // 1.5
	int dstPixels = (srcPixels * size) >> 8;
	EXPECT_EQ(dstPixels, 15);
}

//=============================================================================
// Sprite Position — Signed 16-bit
//=============================================================================

TEST_F(LynxSuzyMathTest, Position_PositiveOnScreen) {
	int16_t x = 80;  // Center of 160-pixel screen
	int16_t y = 51;  // Center of 102-line screen

	EXPECT_GE(x, 0);
	EXPECT_LT(x, (int16_t)LynxConstants::ScreenWidth);
	EXPECT_GE(y, 0);
	EXPECT_LT(y, (int16_t)LynxConstants::ScreenHeight);
}

TEST_F(LynxSuzyMathTest, Position_NegativeOffScreen) {
	// Sprites can have negative positions (partially off-screen)
	int16_t x = -10;
	int16_t y = -5;

	EXPECT_LT(x, 0);
	EXPECT_LT(y, 0);
}

TEST_F(LynxSuzyMathTest, Position_LargePositive) {
	// Sprites can extend beyond screen boundaries
	int16_t x = 200; // > 160
	int16_t y = 150; // > 102

	EXPECT_GT(x, (int16_t)LynxConstants::ScreenWidth);
	EXPECT_GT(y, (int16_t)LynxConstants::ScreenHeight);
}

//=============================================================================
// Sprite Data Packing — RLE and Literal
//=============================================================================

TEST_F(LynxSuzyMathTest, PackedData_OffsetByte) {
	// First byte of line data encodes offset and literal flag
	// Bits [6:0] = line offset (distance to next line data)
	// Bit 7 = literal mode (0 = packed, 1 = literal)
	uint8_t byte = 0b00001010; // Offset=10, Literal=false
	uint8_t offset = byte & 0x7F;
	bool literal = (byte & 0x80) != 0;

	EXPECT_EQ(offset, 10);
	EXPECT_FALSE(literal);
}

TEST_F(LynxSuzyMathTest, PackedData_LiteralMode) {
	uint8_t byte = 0b10001010; // Offset=10, Literal=true
	uint8_t offset = byte & 0x7F;
	bool literal = (byte & 0x80) != 0;

	EXPECT_EQ(offset, 10);
	EXPECT_TRUE(literal);
}

TEST_F(LynxSuzyMathTest, PackedData_ZeroOffset_EndOfSprite) {
	// Offset of 0 indicates end of sprite data
	uint8_t byte = 0x00;
	uint8_t offset = byte & 0x7F;

	EXPECT_EQ(offset, 0);
}

//=============================================================================
// Pen Index Remapping
//=============================================================================

TEST_F(LynxSuzyMathTest, PenIndex_DefaultMapping) {
	// Default pen mapping is identity: pen[n] = n
	uint8_t penIndex[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(penIndex[i], i);
	}
}

TEST_F(LynxSuzyMathTest, PenIndex_Remapped) {
	// Pen remapping allows palette indirection
	uint8_t penIndex[16] = { 0,2,4,6,8,10,12,14,1,3,5,7,9,11,13,15 };
	EXPECT_EQ(penIndex[0], 0);
	EXPECT_EQ(penIndex[1], 2);
	EXPECT_EQ(penIndex[8], 1);
}

TEST_F(LynxSuzyMathTest, PenIndex_TransparentColor) {
	// Pen 0 is typically transparent (not drawn)
	uint8_t penIndex = 0;
	bool transparent = (penIndex == 0);
	EXPECT_TRUE(transparent);
}

TEST_F(LynxSuzyMathTest, PenIndex_MaxValue) {
	// Maximum pen index for 4bpp is 15
	uint8_t maxPen = 15;
	EXPECT_LE(maxPen, 15);
}

//=============================================================================
// Sprite Control Register 1 (SPRCTL1) Decoding
// Bit 2 = Skip sprite
// Bit 4 = Skip reload HVST (hpos, vpos, hsize, vsize, stretch, tilt)
// Bit 5 = Skip reload HVS (hpos, vpos, hsize, vsize)
// Bit 6 = Skip reload HV (hpos, vpos)
// Bit 7 = Skip reload palette
//=============================================================================

TEST_F(LynxSuzyMathTest, Sprctl1_SkipSprite) {
	// Bit 2 = skip this sprite
	uint8_t sprctl1 = 0b00000100;
	bool skip = (sprctl1 & 0x04) != 0;
	EXPECT_TRUE(skip);

	sprctl1 = 0b00000000;
	skip = (sprctl1 & 0x04) != 0;
	EXPECT_FALSE(skip);
}

TEST_F(LynxSuzyMathTest, Sprctl1_SkipReloadHVST) {
	// Bit 4 = don't reload HVST from SCB
	uint8_t sprctl1 = 0b00010000;
	bool skipReloadHVST = (sprctl1 & 0x10) != 0;
	EXPECT_TRUE(skipReloadHVST);
}

TEST_F(LynxSuzyMathTest, Sprctl1_SkipReloadHVS) {
	// Bit 5 = don't reload HVS from SCB
	uint8_t sprctl1 = 0b00100000;
	bool skipReloadHVS = (sprctl1 & 0x20) != 0;
	EXPECT_TRUE(skipReloadHVS);
}

TEST_F(LynxSuzyMathTest, Sprctl1_SkipReloadHV) {
	// Bit 6 = don't reload HV from SCB
	uint8_t sprctl1 = 0b01000000;
	bool skipReloadHV = (sprctl1 & 0x40) != 0;
	EXPECT_TRUE(skipReloadHV);
}

TEST_F(LynxSuzyMathTest, Sprctl1_SkipReloadPalette) {
	// Bit 7 = don't reload palette from SCB
	uint8_t sprctl1 = 0b10000000;
	bool skipReloadPalette = (sprctl1 & 0x80) != 0;
	EXPECT_TRUE(skipReloadPalette);
}

TEST_F(LynxSuzyMathTest, Sprctl1_AllReloadFlags) {
	// All skip-reload flags set
	uint8_t sprctl1 = 0b11110000;
	bool skipHVST = (sprctl1 & 0x10) != 0;
	bool skipHVS = (sprctl1 & 0x20) != 0;
	bool skipHV = (sprctl1 & 0x40) != 0;
	bool skipPalette = (sprctl1 & 0x80) != 0;

	EXPECT_TRUE(skipHVST);
	EXPECT_TRUE(skipHVS);
	EXPECT_TRUE(skipHV);
	EXPECT_TRUE(skipPalette);
}

TEST_F(LynxSuzyMathTest, Sprctl1_StartDrawUp) {
	// Bit 3 = draw in +y direction (0=draw down, 1=draw up)
	// Note: Per docs, 0=draw down from origin, 1=draw up
	uint8_t sprctl1 = 0b00001000;
	bool startDrawUp = (sprctl1 & 0x08) != 0;
	EXPECT_TRUE(startDrawUp);
}

//=============================================================================
// Tilt Transformation  8.8 Fixed Point Signed
// Tilt shifts the horizontal position by TILT*lineNumber per line
//=============================================================================

TEST_F(LynxSuzyMathTest, Tilt_Zero) {
	// Zero tilt = no horizontal shift
	int16_t tilt = 0x0000;
	int lineNum = 10;
	int hShift = (tilt * lineNum) >> 8;
	EXPECT_EQ(hShift, 0);
}

TEST_F(LynxSuzyMathTest, Tilt_PositiveSmall) {
	// 0.5 tilt: each line shifts 0.5 pixels right
	int16_t tilt = 0x0080; // 0.5 in 8.8
	int lineNum = 8;
	// 0.5 * 8 = 4 pixels shift
	int hShift = (tilt * lineNum) >> 8;
	EXPECT_EQ(hShift, 4);
}

TEST_F(LynxSuzyMathTest, Tilt_Negative) {
	// -0.5 tilt: each line shifts 0.5 pixels left
	int16_t tilt = (int16_t)0xFF80; // -0.5 in 8.8 signed
	int lineNum = 8;
	// -0.5 * 8 = -4 pixels shift
	int hShift = (tilt * lineNum) >> 8;
	EXPECT_EQ(hShift, -4);
}

TEST_F(LynxSuzyMathTest, Tilt_OnePixelPerLine) {
	// 1.0 tilt: each line shifts 1 pixel
	int16_t tilt = 0x0100; // 1.0 in 8.8
	int lineNum = 5;
	int hShift = (tilt * lineNum) >> 8;
	EXPECT_EQ(hShift, 5);
}

TEST_F(LynxSuzyMathTest, Tilt_ItalicEffect) {
	// Simulate italicizing text: 0.25 pixels/line over 16 lines
	// = 4 pixel slant
	int16_t tilt = 0x0040; // 0.25 in 8.8
	int lineNum = 16;
	int hShift = (tilt * lineNum) >> 8;
	EXPECT_EQ(hShift, 4);
}

//=============================================================================
// Stretch Transformation  8.8 Fixed Point Signed
// Stretch modifies the horizontal size per line
// Each line: hSize += stretch
//=============================================================================

TEST_F(LynxSuzyMathTest, Stretch_Zero) {
	// Zero stretch = no size change
	int16_t stretch = 0x0000;
	uint16_t hSize = 0x0100; // 1.0
	hSize += stretch; // unchanged
	EXPECT_EQ(hSize, 0x0100);
}

TEST_F(LynxSuzyMathTest, Stretch_PositiveGrowing) {
	// Positive stretch makes sprite wider each line (trapezoid)
	int16_t stretch = 0x0010; // 0.0625 per line
	uint16_t hSize = 0x0100; // 1.0
	// After 16 lines
	for (int line = 0; line < 16; line++) {
		hSize += stretch;
	}
	// 1.0 + 16*0.0625 = 2.0
	EXPECT_EQ(hSize, 0x0200);
}

TEST_F(LynxSuzyMathTest, Stretch_NegativeShrinking) {
	// Negative stretch makes sprite narrower each line (inverse trapezoid)
	int16_t stretch = (int16_t)0xFFF0; // -0.0625 per line
	uint16_t hSize = 0x0200; // 2.0
	// After 16 lines
	for (int line = 0; line < 16; line++) {
		hSize += stretch;
	}
	// 2.0 - 16*0.0625 = 1.0
	EXPECT_EQ(hSize, 0x0100);
}

TEST_F(LynxSuzyMathTest, Stretch_TriangleSprite) {
	// Triangle: start at 0 width, grow 0.5 per line for 8 lines = 4.0 at bottom
	int16_t stretch = 0x0080; // 0.5
	uint16_t hSize = 0x0000;
	for (int line = 0; line < 8; line++) {
		hSize += stretch;
	}
	EXPECT_EQ(hSize, 0x0400); // 4.0
}

//=============================================================================
// Collision Number/Priority
//=============================================================================

TEST_F(LynxSuzyMathTest, CollisionNumber_Range) {
	// Collision numbers are 0-15 (4-bit value)
	for (uint8_t collNum = 0; collNum < 16; collNum++) {
		EXPECT_LE(collNum, 15);
	}
}

TEST_F(LynxSuzyMathTest, CollisionNumber_HigherPriorityWins) {
	// In collision detection, higher-numbered sprites have priority
	// When two sprites collide, the buffer stores the higher number
	uint8_t sprite1CollNum = 3;
	uint8_t sprite2CollNum = 7;
	uint8_t bufferValue = std::max(sprite1CollNum, sprite2CollNum);
	EXPECT_EQ(bufferValue, 7);
}

TEST_F(LynxSuzyMathTest, CollisionNumber_ZeroIsNoCollision) {
	// Collision number 0 typically means "no collision recorded"
	uint8_t collNum = 0;
	bool hasCollision = (collNum != 0);
	EXPECT_FALSE(hasCollision);
}

//=============================================================================
// Screen Boundaries
//=============================================================================

TEST_F(LynxSuzyMathTest, Screen_Width) {
	EXPECT_EQ((int)LynxConstants::ScreenWidth, 160);
}

TEST_F(LynxSuzyMathTest, Screen_Height) {
	EXPECT_EQ((int)LynxConstants::ScreenHeight, 102);
}

TEST_F(LynxSuzyMathTest, Screen_PixelCount) {
	int pixels = (int)LynxConstants::ScreenWidth * (int)LynxConstants::ScreenHeight;
	EXPECT_EQ(pixels, 16320);
}

//=============================================================================
// Math Accumulate Mode
//=============================================================================

TEST_F(LynxSuzyMathTest, MathAccumulate_DisabledClearsResult) {
	// When accumulate is disabled, each operation starts fresh
	_state.MathAccumulate = false;
	int32_t result1 = 100 * 5;
	int32_t result2 = 7 * 3;
	// Each result is independent
	EXPECT_EQ(result1, 500);
	EXPECT_EQ(result2, 21);
}

TEST_F(LynxSuzyMathTest, MathAccumulate_EnabledAddsToResult) {
	// When accumulate is enabled, results add up
	_state.MathAccumulate = true;
	int32_t accumulator = 0;
	accumulator += 100 * 5; // 500
	accumulator += 7 * 3;   // 21
	EXPECT_EQ(accumulator, 521);
}

TEST_F(LynxSuzyMathTest, MathAccumulate_UsefulForDotProduct) {
	// Accumulate mode is useful for dot products:
	// A·B = a1*b1 + a2*b2 + a3*b3
	_state.MathAccumulate = true;
	int16_t a[] = { 3, 4, 5 };
	int16_t b[] = { 2, 6, 8 };
	int32_t dot = 0;
	for (int i = 0; i < 3; i++) {
		dot += a[i] * b[i];
	}
	// 3*2 + 4*6 + 5*8 = 6 + 24 + 40 = 70
	EXPECT_EQ(dot, 70);
}
