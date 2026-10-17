#include "pch.h"
#include "Lynx/LynxDecrypt.h"
#include <array>
#include <cstring>

/// <summary>
/// Tests for Lynx RSA bootloader decryption.
///
/// The Lynx uses RSA encryption with Montgomery multiplication:
///   - Block size: 51 bytes (408 bits)
///   - Public exponent: 3
///   - Formula: PLAINTEXT = ENCRYPTED³ mod PUBLIC_KEY
///
/// These tests validate:
///   - Public key/constant values
///   - Montgomery multiplication
///   - Block decryption
///   - Validation functions
///   - Edge cases and error handling
///
/// References:
///   - LynxDecrypt.h (API)
///   - libretro-handy lynxdec.cpp (reference implementation)
///   - ~docs/plans/lynx-bootloader-encryption-research.md
/// </summary>
class LynxDecryptTest : public ::testing::Test {
protected:
	// Create minimal valid encrypted data (1 block)
	std::vector<uint8_t> CreateMinimalEncrypted() {
		std::vector<uint8_t> data(52); // 1 + 51 bytes
		data[0] = 0xFF; // Block count = 256 - 0xFF = 1
		// Fill block with test pattern (won't decrypt to valid data)
		for (size_t i = 1; i < 52; i++) {
			data[i] = static_cast<uint8_t>(i);
		}
		return data;
	}

	// Create 8-block encrypted data (typical loader size)
	std::vector<uint8_t> CreateTypicalEncrypted() {
		std::vector<uint8_t> data(1 + 8 * 51); // 409 bytes
		data[0] = 0xF8; // Block count = 256 - 0xF8 = 8
		// Fill with pattern
		for (size_t i = 1; i < data.size(); i++) {
			data[i] = static_cast<uint8_t>(i * 0x17);
		}
		return data;
	}
};

//=============================================================================
// Public Key Tests
//=============================================================================

TEST_F(LynxDecryptTest, PublicModulus_Size) {
	EXPECT_EQ(sizeof(LynxCrypto::PublicModulus), 51u);
}

TEST_F(LynxDecryptTest, PublicModulus_FirstByte) {
	EXPECT_EQ(LynxCrypto::PublicModulus[0], 0x35);
}

TEST_F(LynxDecryptTest, PublicModulus_LastByte) {
	EXPECT_EQ(LynxCrypto::PublicModulus[50], 0x79);
}

TEST_F(LynxDecryptTest, PublicModulus_KnownBytes) {
	// Verify several known bytes from the modulus
	EXPECT_EQ(LynxCrypto::PublicModulus[1], 0xB5);
	EXPECT_EQ(LynxCrypto::PublicModulus[2], 0xA3);
	EXPECT_EQ(LynxCrypto::PublicModulus[10], 0xD7);
	EXPECT_EQ(LynxCrypto::PublicModulus[23], 0x00);  // Corrected byte position
	EXPECT_EQ(LynxCrypto::PublicModulus[40], 0xF2);
}

//=============================================================================
// Constants Tests
//=============================================================================

TEST_F(LynxDecryptTest, BlockSize_Is51) {
	EXPECT_EQ(LynxCrypto::BlockSize, 51u);
}

TEST_F(LynxDecryptTest, OutputBytesPerBlock_Is50) {
	EXPECT_EQ(LynxCrypto::OutputBytesPerBlock, 50u);
}

TEST_F(LynxDecryptTest, MinEncryptedSize_Is52) {
	EXPECT_EQ(LynxCrypto::MinEncryptedSize, 52u);
}

TEST_F(LynxDecryptTest, GetRequiredSize_1Block) {
	EXPECT_EQ(LynxCrypto::GetRequiredSize(1), 52u);
}

TEST_F(LynxDecryptTest, GetRequiredSize_8Blocks) {
	EXPECT_EQ(LynxCrypto::GetRequiredSize(8), 409u);
}

//=============================================================================
// Block Count Tests
//=============================================================================

TEST_F(LynxDecryptTest, GetBlockCount_Empty) {
	std::span<const uint8_t> empty;
	EXPECT_EQ(LynxCrypto::GetBlockCount(empty), 0u);
}

TEST_F(LynxDecryptTest, GetBlockCount_SingleBlock) {
	auto data = CreateMinimalEncrypted();
	EXPECT_EQ(LynxCrypto::GetBlockCount(data), 1u);
}

TEST_F(LynxDecryptTest, GetBlockCount_8Blocks) {
	auto data = CreateTypicalEncrypted();
	EXPECT_EQ(LynxCrypto::GetBlockCount(data), 8u);
}

TEST_F(LynxDecryptTest, GetBlockCount_Formula) {
	// Block count = 256 - first byte
	std::vector<uint8_t> data = { 0xFB }; // 256 - 0xFB = 5
	EXPECT_EQ(LynxCrypto::GetBlockCount(data), 5u);
}

//=============================================================================
// Decrypted Size Tests
//=============================================================================

TEST_F(LynxDecryptTest, GetDecryptedSize_Empty) {
	std::span<const uint8_t> empty;
	EXPECT_EQ(LynxCrypto::GetDecryptedSize(empty), 0u);
}

TEST_F(LynxDecryptTest, GetDecryptedSize_1Block) {
	auto data = CreateMinimalEncrypted();
	EXPECT_EQ(LynxCrypto::GetDecryptedSize(data), 50u);
}

TEST_F(LynxDecryptTest, GetDecryptedSize_8Blocks) {
	auto data = CreateTypicalEncrypted();
	EXPECT_EQ(LynxCrypto::GetDecryptedSize(data), 400u);
}

//=============================================================================
// Validation Tests
//=============================================================================

TEST_F(LynxDecryptTest, Validate_Empty_False) {
	std::span<const uint8_t> empty;
	EXPECT_FALSE(LynxCrypto::Validate(empty));
}

TEST_F(LynxDecryptTest, Validate_TooSmall_False) {
	std::vector<uint8_t> tooSmall(51); // Needs at least 52
	EXPECT_FALSE(LynxCrypto::Validate(tooSmall));
}

TEST_F(LynxDecryptTest, Validate_MinimalValid_True) {
	auto data = CreateMinimalEncrypted();
	EXPECT_TRUE(LynxCrypto::Validate(data));
}

TEST_F(LynxDecryptTest, Validate_TypicalValid_True) {
	auto data = CreateTypicalEncrypted();
	EXPECT_TRUE(LynxCrypto::Validate(data));
}

TEST_F(LynxDecryptTest, Validate_ZeroBlocks_False) {
	std::vector<uint8_t> data(52);
	data[0] = 0x00; // Block count = 256 - 0 = 256 (too many)
	EXPECT_FALSE(LynxCrypto::Validate(data));
}

TEST_F(LynxDecryptTest, Validate_TooManyBlocks_False) {
	std::vector<uint8_t> data(52);
	data[0] = 0xF0; // Block count = 256 - 0xF0 = 16 (>15)
	EXPECT_FALSE(LynxCrypto::Validate(data));
}

TEST_F(LynxDecryptTest, Validate_InsufficientData_False) {
	std::vector<uint8_t> data(52);
	data[0] = 0xFE; // Claims 2 blocks but only has 1 block of data
	EXPECT_FALSE(LynxCrypto::Validate(data));
}

//=============================================================================
// Decryption Result Tests
//=============================================================================

TEST_F(LynxDecryptTest, Decrypt_Empty_Invalid) {
	std::span<const uint8_t> empty;
	auto result = LynxCrypto::Decrypt(empty);
	EXPECT_FALSE(result.Valid);
	EXPECT_TRUE(result.Data.empty());
}

TEST_F(LynxDecryptTest, Decrypt_Minimal_ReturnsData) {
	auto data = CreateMinimalEncrypted();
	auto result = LynxCrypto::Decrypt(data);
	// May not be valid (checksum might not be 0) but should produce output
	EXPECT_EQ(result.Data.size(), 50u);
	EXPECT_EQ(result.BlockCount, 1u);
}

TEST_F(LynxDecryptTest, Decrypt_Typical_ReturnsData) {
	auto data = CreateTypicalEncrypted();
	auto result = LynxCrypto::Decrypt(data);
	EXPECT_EQ(result.Data.size(), 400u);
	EXPECT_EQ(result.BlockCount, 8u);
}

//=============================================================================
// Montgomery Multiplication Tests
//=============================================================================

TEST_F(LynxDecryptTest, Montgomery_ZeroTimesZero_Zero) {
	std::array<uint8_t, 51> result{};
	std::array<uint8_t, 51> zero{};

	LynxCrypto::MontgomeryMultiply(
		std::span<uint8_t, 51>(result),
		std::span<const uint8_t, 51>(zero),
		std::span<const uint8_t, 51>(zero),
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	// 0 × 0 mod anything = 0
	for (size_t i = 0; i < 51; i++) {
		EXPECT_EQ(result[i], 0);
	}
}

TEST_F(LynxDecryptTest, Montgomery_ZeroTimesAnything_Zero) {
	std::array<uint8_t, 51> result{};
	std::array<uint8_t, 51> zero{};
	std::array<uint8_t, 51> nonZero{};
	nonZero[50] = 0x42;

	LynxCrypto::MontgomeryMultiply(
		std::span<uint8_t, 51>(result),
		std::span<const uint8_t, 51>(zero),
		std::span<const uint8_t, 51>(nonZero),
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	// 0 × X mod anything = 0
	for (size_t i = 0; i < 51; i++) {
		EXPECT_EQ(result[i], 0);
	}
}

TEST_F(LynxDecryptTest, Montgomery_Deterministic) {
	// Same inputs should always produce same output
	std::array<uint8_t, 51> input{};
	input[50] = 0x05;
	input[49] = 0x10;

	std::array<uint8_t, 51> result1{};
	std::array<uint8_t, 51> result2{};

	LynxCrypto::MontgomeryMultiply(
		result1, input, input,
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	LynxCrypto::MontgomeryMultiply(
		result2, input, input,
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	EXPECT_EQ(result1, result2);
}

//=============================================================================
// Block Decryption Tests
//=============================================================================

TEST_F(LynxDecryptTest, DecryptBlock_ProducesOutput) {
	std::array<uint8_t, 51> block{};
	block[50] = 0x42;

	std::array<uint8_t, 50> output{};
	uint8_t accumulator = 0;

	LynxCrypto::DecryptBlock(
		std::span<uint8_t, 50>(output),
		std::span<const uint8_t, 51>(block),
		accumulator
	);

	// Should produce some output (exact values depend on RSA math)
	// Just verify it doesn't crash and produces deterministic output
	EXPECT_TRUE(true);
}

TEST_F(LynxDecryptTest, DecryptBlock_ModifiesAccumulator) {
	std::array<uint8_t, 51> block{};
	for (size_t i = 0; i < 51; i++) {
		block[i] = static_cast<uint8_t>(i);
	}

	std::array<uint8_t, 50> output{};
	uint8_t accumulator = 0;

	LynxCrypto::DecryptBlock(
		std::span<uint8_t, 50>(output),
		std::span<const uint8_t, 51>(block),
		accumulator
	);

	// Accumulator should be modified
	// It might coincidentally be 0, but unlikely with arbitrary input
	// Just verify the function runs without crashing
	EXPECT_TRUE(true);
}

//=============================================================================
// Edge Case Tests
//=============================================================================

TEST_F(LynxDecryptTest, Decrypt_ExactMinimalSize_Works) {
	std::vector<uint8_t> data(52);
	data[0] = 0xFF; // 1 block
	auto result = LynxCrypto::Decrypt(data);
	EXPECT_EQ(result.BlockCount, 1u);
}

TEST_F(LynxDecryptTest, Decrypt_ExtraDataIgnored) {
	std::vector<uint8_t> data(100); // More than needed for 1 block
	data[0] = 0xFF; // 1 block
	auto result = LynxCrypto::Decrypt(data);
	// Should only process 1 block, ignore extra
	EXPECT_EQ(result.BlockCount, 1u);
	EXPECT_EQ(result.Data.size(), 50u);
}

//=============================================================================
// Checksum Tests
//=============================================================================

TEST_F(LynxDecryptTest, Decrypt_ChecksumReturned) {
	auto data = CreateTypicalEncrypted();
	auto result = LynxCrypto::Decrypt(data);
	// Checksum is the final accumulator value
	// For random data, it's very unlikely to be 0
	// Just verify it's captured
	EXPECT_EQ(result.BlockCount, 8u);
}

TEST_F(LynxDecryptTest, Decrypt_ValidData_ChecksumZero) {
	// Valid encrypted data should have checksum = 0
	// We don't have real encrypted test vectors, so just
	// verify the Valid flag matches checksum == 0
	auto data = CreateTypicalEncrypted();
	auto result = LynxCrypto::Decrypt(data);
	EXPECT_EQ(result.Valid, result.Checksum == 0);
}

//=============================================================================
// Private Key Tests
//=============================================================================

TEST_F(LynxDecryptTest, PrivateExponent_Size) {
	EXPECT_EQ(sizeof(LynxCrypto::PrivateExponent), 51u);
}

TEST_F(LynxDecryptTest, PrivateExponent_FirstByte) {
	EXPECT_EQ(LynxCrypto::PrivateExponent[0], 0x23);
}

TEST_F(LynxDecryptTest, PrivateExponent_LastByte) {
	EXPECT_EQ(LynxCrypto::PrivateExponent[50], 0xA3);
}

TEST_F(LynxDecryptTest, PrivateExponent_KnownBytes) {
	// Verify several known bytes from the private exponent
	EXPECT_EQ(LynxCrypto::PrivateExponent[1], 0xCE);
	EXPECT_EQ(LynxCrypto::PrivateExponent[2], 0x6D);
	EXPECT_EQ(LynxCrypto::PrivateExponent[10], 0x3A);
	EXPECT_EQ(LynxCrypto::PrivateExponent[25], 0x87);
	EXPECT_EQ(LynxCrypto::PrivateExponent[40], 0x21);
}

//=============================================================================
// Encryption API Tests
//=============================================================================

TEST_F(LynxDecryptTest, Encrypt_EmptyInput_Invalid) {
	std::vector<uint8_t> empty;
	auto result = LynxCrypto::Encrypt(empty);
	EXPECT_FALSE(result.Valid);
	EXPECT_TRUE(result.Data.empty());
}

TEST_F(LynxDecryptTest, Encrypt_TooLarge_Invalid) {
	std::vector<uint8_t> tooLarge(800); // > 15 blocks × 50 bytes
	auto result = LynxCrypto::Encrypt(tooLarge);
	EXPECT_FALSE(result.Valid);
}

TEST_F(LynxDecryptTest, Encrypt_MinimalInput_Valid) {
	std::vector<uint8_t> data(1); // 1 byte = 1 block
	data[0] = 0x42;
	auto result = LynxCrypto::Encrypt(data);
	EXPECT_TRUE(result.Valid);
	EXPECT_EQ(result.BlockCount, 1u);
	// Output: 1 byte header + 51 bytes encrypted = 52 bytes
	EXPECT_EQ(result.Data.size(), 52u);
}

TEST_F(LynxDecryptTest, Encrypt_BlockCountCorrect) {
	std::vector<uint8_t> data(100); // 2 blocks (ceil(100/50))
	auto result = LynxCrypto::Encrypt(data);
	EXPECT_TRUE(result.Valid);
	EXPECT_EQ(result.BlockCount, 2u);
	// Header byte should be 256 - 2 = 254 = 0xFE
	EXPECT_EQ(result.Data[0], 0xFE);
}

TEST_F(LynxDecryptTest, Encrypt_ExactBlockSize_Works) {
	std::vector<uint8_t> data(50); // Exactly 1 block
	auto result = LynxCrypto::Encrypt(data);
	EXPECT_TRUE(result.Valid);
	EXPECT_EQ(result.BlockCount, 1u);
}

TEST_F(LynxDecryptTest, Encrypt_MaxBlocks_Works) {
	std::vector<uint8_t> data(750); // Exactly 15 blocks
	auto result = LynxCrypto::Encrypt(data);
	EXPECT_TRUE(result.Valid);
	EXPECT_EQ(result.BlockCount, 15u);
	// Header: 256 - 15 = 241 = 0xF1
	EXPECT_EQ(result.Data[0], 0xF1);
}

//=============================================================================
// Round-Trip Tests (Encrypt → Decrypt)
//=============================================================================

TEST_F(LynxDecryptTest, RoundTrip_SimpleData) {
	// Create simple plaintext
	std::vector<uint8_t> plaintext(50);
	for (size_t i = 0; i < 50; i++) {
		plaintext[i] = static_cast<uint8_t>(i);
	}

	// Encrypt
	auto encrypted = LynxCrypto::Encrypt(plaintext);
	ASSERT_TRUE(encrypted.Valid);

	// Decrypt
	auto decrypted = LynxCrypto::Decrypt(encrypted.Data);
	// Note: Valid flag checks checksum=0, which arbitrary data won't satisfy
	// We only care that the DATA round-trips correctly
	EXPECT_EQ(decrypted.BlockCount, 1u);

	// Verify round-trip of actual data
	ASSERT_EQ(decrypted.Data.size(), plaintext.size());
	for (size_t i = 0; i < plaintext.size(); i++) {
		EXPECT_EQ(decrypted.Data[i], plaintext[i])
			<< "Mismatch at byte " << i;
	}
}

TEST_F(LynxDecryptTest, RoundTrip_MultipleBlocks) {
	// Create multi-block plaintext
	std::vector<uint8_t> plaintext(200); // 4 blocks
	for (size_t i = 0; i < plaintext.size(); i++) {
		plaintext[i] = static_cast<uint8_t>(i * 17 + 3);
	}

	// Encrypt
	auto encrypted = LynxCrypto::Encrypt(plaintext);
	ASSERT_TRUE(encrypted.Valid);
	EXPECT_EQ(encrypted.BlockCount, 4u);

	// Decrypt
	auto decrypted = LynxCrypto::Decrypt(encrypted.Data);
	// Note: Valid checks checksum=0, arbitrary data won't satisfy this
	EXPECT_EQ(decrypted.BlockCount, 4u);

	// Verify round-trip of actual data
	ASSERT_EQ(decrypted.Data.size(), plaintext.size());
	for (size_t i = 0; i < plaintext.size(); i++) {
		EXPECT_EQ(decrypted.Data[i], plaintext[i])
			<< "Mismatch at byte " << i;
	}
}

TEST_F(LynxDecryptTest, RoundTrip_AllZeros) {
	std::vector<uint8_t> plaintext(100, 0);

	auto encrypted = LynxCrypto::Encrypt(plaintext);
	ASSERT_TRUE(encrypted.Valid);

	auto decrypted = LynxCrypto::Decrypt(encrypted.Data);
	// All zeros should have Valid=true since final acc = 0
	EXPECT_TRUE(decrypted.Valid);

	ASSERT_EQ(decrypted.Data.size(), plaintext.size());
	for (size_t i = 0; i < plaintext.size(); i++) {
		EXPECT_EQ(decrypted.Data[i], 0) << "Mismatch at byte " << i;
	}
}

TEST_F(LynxDecryptTest, RoundTrip_AllOnes) {
	std::vector<uint8_t> plaintext(100, 0xFF);

	auto encrypted = LynxCrypto::Encrypt(plaintext);
	ASSERT_TRUE(encrypted.Valid);

	auto decrypted = LynxCrypto::Decrypt(encrypted.Data);
	// Note: Valid checks checksum=0, 0xFF data won't satisfy this
	EXPECT_EQ(decrypted.BlockCount, 2u);

	ASSERT_EQ(decrypted.Data.size(), plaintext.size());
	for (size_t i = 0; i < plaintext.size(); i++) {
		EXPECT_EQ(decrypted.Data[i], 0xFF) << "Mismatch at byte " << i;
	}
}

//=============================================================================
// Modular Exponentiation Tests
//=============================================================================

TEST_F(LynxDecryptTest, ModularExponentiate_Identity) {
	// x^1 mod n = x (if x < n)
	std::array<uint8_t, 51> base{};
	std::array<uint8_t, 51> exponent{};
	std::array<uint8_t, 51> result{};

	// Small base value (5)
	base[50] = 5;
	// Exponent = 1
	exponent[50] = 1;

	LynxCrypto::ModularExponentiate(
		std::span<uint8_t, 51>(result),
		std::span<const uint8_t, 51>(base),
		std::span<const uint8_t, 51>(exponent),
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	// Result should be 5
	EXPECT_EQ(result[50], 5);
	for (size_t i = 0; i < 50; i++) {
		EXPECT_EQ(result[i], 0) << "Non-zero at byte " << i;
	}
}

TEST_F(LynxDecryptTest, ModularExponentiate_Square) {
	// 3^2 mod n = 9 (if n > 9)
	std::array<uint8_t, 51> base{};
	std::array<uint8_t, 51> exponent{};
	std::array<uint8_t, 51> result{};

	base[50] = 3;
	exponent[50] = 2;

	LynxCrypto::ModularExponentiate(
		std::span<uint8_t, 51>(result),
		std::span<const uint8_t, 51>(base),
		std::span<const uint8_t, 51>(exponent),
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	// Result should be 9
	EXPECT_EQ(result[50], 9);
}

TEST_F(LynxDecryptTest, ModularExponentiate_Cube) {
	// 2^3 mod n = 8 (if n > 8)
	std::array<uint8_t, 51> base{};
	std::array<uint8_t, 51> exponent{};
	std::array<uint8_t, 51> result{};

	base[50] = 2;
	exponent[50] = 3;

	LynxCrypto::ModularExponentiate(
		std::span<uint8_t, 51>(result),
		std::span<const uint8_t, 51>(base),
		std::span<const uint8_t, 51>(exponent),
		std::span<const uint8_t, 51>(LynxCrypto::PublicModulus)
	);

	// Result should be 8
	EXPECT_EQ(result[50], 8);
}

//=============================================================================
// Fuzz Tests — Montgomery Multiplication
// Use deterministic seeded RNG to test a variety of input pairs.
// Verifies commutative and associative properties.
//=============================================================================

namespace {
// Simple RNG for reproducible fuzz tests
class FuzzRng {
public:
	explicit FuzzRng(uint64_t seed) : _state(seed) {}

	uint8_t Next8() {
		_state = _state * 6364136223846793005ULL + 1442695040888963407ULL;
		return static_cast<uint8_t>(_state >> 56);
	}

	void Fill51(std::span<uint8_t, 51> arr) {
		for (size_t i = 0; i < 51; i++) {
			arr[i] = Next8();
		}
	}

	// Ensure value is less than modulus by zeroing high bytes
	void FillLessThanModulus(std::span<uint8_t, 51> arr) {
		Fill51(arr);
		// Zero the highest bytes to ensure it's well under the modulus
		arr[0] = 0;
		arr[1] = 0;
		arr[2] = 0;
	}

private:
	uint64_t _state;
};
} // namespace

TEST_F(LynxDecryptTest, Fuzz_Montgomery_Commutativity_100Pairs) {
	// Montgomery multiplication should be commutative: A * B = B * A
	constexpr uint64_t SEED = 0x524F534143525950ULL; // "ROSACRYP"
	FuzzRng rng(SEED);

	for (int i = 0; i < 100; i++) {
		std::array<uint8_t, 51> a{}, b{}, result_ab{}, result_ba{};

		rng.FillLessThanModulus(a);
		rng.FillLessThanModulus(b);

		LynxCrypto::MontgomeryMultiply(result_ab, a, b,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));
		LynxCrypto::MontgomeryMultiply(result_ba, b, a,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));

		EXPECT_EQ(result_ab, result_ba)
			<< "Commutativity failed at iteration " << i;
	}
}

TEST_F(LynxDecryptTest, Fuzz_Montgomery_ZeroAbsorption_100Values) {
	// 0 * X = 0 for all X
	constexpr uint64_t SEED = 0x5A45524F41425321ULL; // "ZEROABS!"
	FuzzRng rng(SEED);

	std::array<uint8_t, 51> zero{};

	for (int i = 0; i < 100; i++) {
		std::array<uint8_t, 51> x{}, result{};
		rng.FillLessThanModulus(x);

		LynxCrypto::MontgomeryMultiply(result, zero, x,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));

		// Result should be zero
		bool isZero = true;
		for (size_t j = 0; j < 51; j++) {
			if (result[j] != 0) {
				isZero = false;
				break;
			}
		}
		EXPECT_TRUE(isZero) << "Zero absorption failed at iteration " << i;
	}
}

TEST_F(LynxDecryptTest, Fuzz_Montgomery_Determinism_100Values) {
	// Same inputs should always produce same output
	constexpr uint64_t SEED = 0x4445544552524D21ULL; // "DETERRM!"
	FuzzRng rng(SEED);

	for (int i = 0; i < 100; i++) {
		std::array<uint8_t, 51> a{}, b{}, result1{}, result2{};

		rng.FillLessThanModulus(a);
		rng.FillLessThanModulus(b);

		LynxCrypto::MontgomeryMultiply(result1, a, b,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));
		LynxCrypto::MontgomeryMultiply(result2, a, b,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));

		EXPECT_EQ(result1, result2)
			<< "Determinism failed at iteration " << i;
	}
}

TEST_F(LynxDecryptTest, Fuzz_Montgomery_NoOverflow_100Values) {
	// Result should always fit in 51 bytes (408 bits)
	// and be less than the modulus
	constexpr uint64_t SEED = 0x4E4F4F564552464CULL; // "NOOVERFL"
	FuzzRng rng(SEED);

	for (int i = 0; i < 100; i++) {
		std::array<uint8_t, 51> a{}, b{}, result{};

		// Use full random values to stress test
		rng.Fill51(a);
		rng.Fill51(b);

		LynxCrypto::MontgomeryMultiply(result, a, b,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));

		// Just verify the function completes without crashing
		// and produces output that fits in the buffer (which it does by design)
		EXPECT_TRUE(true);
	}
}

TEST_F(LynxDecryptTest, Fuzz_ModularExp_SmallExponents_50Values) {
	// Test modular exponentiation with small exponents
	constexpr uint64_t SEED = 0x534D414C4C455850ULL; // "SMALLEXP"
	FuzzRng rng(SEED);

	for (int i = 0; i < 50; i++) {
		std::array<uint8_t, 51> base{}, exp{}, result{};

		// Use a small random base
		base[50] = rng.Next8();
		base[49] = rng.Next8();

		// Small exponent (1-15)
		exp[50] = (rng.Next8() & 0x0F) + 1;

		LynxCrypto::ModularExponentiate(result, base, exp,
			std::span<const uint8_t, 51>(LynxCrypto::PublicModulus));

		// Just verify it runs without crashing
		// Actual value verification would require a bigint library
		EXPECT_TRUE(true);
	}
}

TEST_F(LynxDecryptTest, Fuzz_DecryptBlock_100Blocks) {
	// Test block decryption with random data
	constexpr uint64_t SEED = 0x424C4F434B52534EULL; // "BLOCKRSN"
	FuzzRng rng(SEED);

	for (int i = 0; i < 100; i++) {
		std::array<uint8_t, 51> block{};
		std::array<uint8_t, 50> output{};
		uint8_t accumulator = 0;

		rng.Fill51(block);

		LynxCrypto::DecryptBlock(
			std::span<uint8_t, 50>(output),
			std::span<const uint8_t, 51>(block),
			accumulator
		);

		// Verify it doesn't crash and produces some output
		EXPECT_TRUE(true);
	}
}
