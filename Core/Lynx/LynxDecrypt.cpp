#include "pch.h"
#include "Lynx/LynxDecrypt.h"
#include <array>
#include <cstring>

/// @file LynxDecrypt.cpp
/// @brief Atari Lynx RSA bootloader decryption implementation.
///
/// Ported from libretro-handy `lynxdec.cpp` (zlib license).
/// Original author: K. Wilkins
///
/// **Algorithm:**
/// The Lynx boot ROM uses Montgomery multiplication to compute
/// RSA decryption without hardware division. The formula is:
///   `PLAINTEXT = ENCRYPTED³ mod PUBLIC_KEY`
///
/// Montgomery multiplication computes `L = M × N mod modulus` using
/// only additions, subtractions, and shifts — crucial for the 4 MHz 65C02.
///
/// **References:**
///   - libretro-handy: https://github.com/libretro/libretro-handy
///   - Montgomery multiplication: https://en.wikipedia.org/wiki/Montgomery_modular_multiplication

namespace LynxCrypto {

// ============================================================================
// Big Integer Arithmetic Helpers
// ============================================================================

namespace {

/// <summary>
/// Double a big-endian integer (multiply by 2 / shift left by 1).
/// </summary>
/// <param name="value">Buffer to modify in-place.</param>
/// <param name="length">Number of bytes.</param>
void DoubleValue(uint8_t* value, size_t length) {
	int carry = 0;
	for (int i = static_cast<int>(length) - 1; i >= 0; i--) {
		int tmp = 2 * value[i] + carry;
		value[i] = static_cast<uint8_t>(tmp & 0xFF);
		carry = tmp >> 8;
	}
}

/// <summary>
/// Subtract: result -= subtrahend.
/// </summary>
/// <param name="result">Buffer to modify in-place.</param>
/// <param name="subtrahend">Value to subtract.</param>
/// <param name="length">Number of bytes.</param>
/// <returns>True if no borrow (result >= 0), false if borrow occurred.</returns>
bool SubtractValue(uint8_t* result, const uint8_t* subtrahend, size_t length) {
	int borrow = 0;
	for (int i = static_cast<int>(length) - 1; i >= 0; i--) {
		int tmp = result[i] - subtrahend[i] - borrow;
		if (tmp < 0) {
			result[i] = static_cast<uint8_t>(tmp + 256);
			borrow = 1;
		} else {
			result[i] = static_cast<uint8_t>(tmp);
			borrow = 0;
		}
	}
	return borrow == 0; // True if no borrow
}

/// <summary>
/// Add: result += addend.
/// </summary>
/// <param name="result">Buffer to modify in-place.</param>
/// <param name="addend">Value to add.</param>
/// <param name="length">Number of bytes.</param>
void AddValue(uint8_t* result, const uint8_t* addend, size_t length) {
	int carry = 0;
	for (int i = static_cast<int>(length) - 1; i >= 0; i--) {
		int tmp = result[i] + addend[i] + carry;
		carry = (tmp >= 256) ? 1 : 0;
		result[i] = static_cast<uint8_t>(tmp & 0xFF);
	}
}

} // anonymous namespace

// ============================================================================
// Montgomery Multiplication
// ============================================================================

void MontgomeryMultiply(
	std::span<uint8_t, BlockSize> result,
	std::span<const uint8_t, BlockSize> multiplicand,
	std::span<const uint8_t, BlockSize> multiplier,
	std::span<const uint8_t, BlockSize> modulus
) {
	// Initialize result to 0
	std::memset(result.data(), 0, BlockSize);

	// Process each byte of the multiplier (MSB first)
	for (size_t i = 0; i < BlockSize; i++) {
		uint8_t nByte = multiplier[i];

		// Process each bit of the byte (MSB first)
		for (int j = 0; j < 8; j++) {
			// L = L × 2
			DoubleValue(result.data(), BlockSize);

			bool bit = (nByte & 0x80) != 0;
			nByte <<= 1;

			if (bit) {
				// L += M
				AddValue(result.data(), multiplicand.data(), BlockSize);
			}

			// Normalize: subtract modulus while L >= modulus
			// After double+add, L < 3*modulus, so at most 2 subtractions needed
			for (int k = 0; k < 2; k++) {
				std::array<uint8_t, BlockSize> backup;
				std::memcpy(backup.data(), result.data(), BlockSize);
				bool noBorrow = SubtractValue(result.data(), modulus.data(), BlockSize);
				if (!noBorrow) {
					// Borrow occurred — result was < modulus, restore and stop
					std::memcpy(result.data(), backup.data(), BlockSize);
					break;
				}
			}
		}
	}
}

// ============================================================================
// Block Decryption
// ============================================================================

void DecryptBlock(
	std::span<uint8_t, OutputBytesPerBlock> output,
	std::span<const uint8_t, BlockSize> block,
	uint8_t& accumulator
) {
	// Reverse byte order before RSA operation
	// Reference: cleaned.c decrypt_block() reverses before cubing
	std::array<uint8_t, BlockSize> reversed;
	for (size_t i = 0; i < BlockSize; i++) {
		reversed[i] = block[BlockSize - 1 - i];
	}

	// Compute B² = B × B mod PublicModulus
	std::array<uint8_t, BlockSize> squared;
	MontgomeryMultiply(
		std::span<uint8_t, BlockSize>(squared),
		std::span<const uint8_t, BlockSize>(reversed),
		std::span<const uint8_t, BlockSize>(reversed),
		std::span<const uint8_t, BlockSize>(PublicModulus)
	);

	// Compute B³ = B × B² mod PublicModulus
	std::array<uint8_t, BlockSize> cubed;
	MontgomeryMultiply(
		std::span<uint8_t, BlockSize>(cubed),
		std::span<const uint8_t, BlockSize>(reversed),
		std::span<const uint8_t, BlockSize>(squared),
		std::span<const uint8_t, BlockSize>(PublicModulus)
	);

	// Apply post-decryption obfuscation
	// Skip byte 0 of the decrypted block (it's metadata)
	// Output 50 bytes with running accumulator
	for (size_t j = 0; j < OutputBytesPerBlock; j++) {
		accumulator = static_cast<uint8_t>(accumulator + cubed[j + 1]);
		output[j] = accumulator;
	}
}

// ============================================================================
// High-Level Decryption API
// ============================================================================

size_t GetBlockCount(std::span<const uint8_t> encrypted) {
	if (encrypted.empty()) {
		return 0;
	}
	// Block count = 256 - first byte
	return 256 - encrypted[0];
}

size_t GetDecryptedSize(std::span<const uint8_t> encrypted) {
	size_t blocks = GetBlockCount(encrypted);
	return blocks * OutputBytesPerBlock;
}

bool Validate(std::span<const uint8_t> encrypted) {
	if (encrypted.size() < MinEncryptedSize) {
		return false;
	}

	size_t blocks = GetBlockCount(encrypted);
	if (blocks == 0 || blocks > 15) {
		// Typical loaders use 5-8 blocks; >15 is suspicious
		return false;
	}

	size_t requiredSize = GetRequiredSize(blocks);
	if (encrypted.size() < requiredSize) {
		return false;
	}

	return true;
}

DecryptResult Decrypt(std::span<const uint8_t> encrypted) {
	DecryptResult result;

	if (!Validate(encrypted)) {
		return result;
	}

	size_t blocks = GetBlockCount(encrypted);
	result.BlockCount = blocks;
	result.Data.resize(blocks * OutputBytesPerBlock);

	uint8_t accumulator = 0;
	size_t outputIdx = 0;

	for (size_t i = 0; i < blocks; i++) {
		// Read 51-byte encrypted block
		std::array<uint8_t, BlockSize> block;
		std::memcpy(block.data(), &encrypted[1 + i * BlockSize], BlockSize);

		// Decrypt block
		std::array<uint8_t, OutputBytesPerBlock> blockOutput;
		DecryptBlock(
			std::span<uint8_t, OutputBytesPerBlock>(blockOutput),
			std::span<const uint8_t, BlockSize>(block),
			accumulator
		);

		// Copy to output
		std::memcpy(&result.Data[outputIdx], blockOutput.data(), OutputBytesPerBlock);
		outputIdx += OutputBytesPerBlock;
	}

	result.Checksum = accumulator;
	// Valid data has final accumulator of 0
	result.Valid = (accumulator == 0);

	return result;
}

// ============================================================================
// Modular Exponentiation (for Encryption)
// ============================================================================

void ModularExponentiate(
	std::span<uint8_t, BlockSize> result,
	std::span<const uint8_t, BlockSize> base,
	std::span<const uint8_t, BlockSize> exponent,
	std::span<const uint8_t, BlockSize> modulus
) {
	// Initialize result to 1 (identity for multiplication)
	std::memset(result.data(), 0, BlockSize);
	result[BlockSize - 1] = 1;

	// Find the first set bit (skip leading zeros for efficiency)
	bool foundFirstBit = false;

	// Square-and-multiply: scan exponent bits from MSB to LSB
	// Process bytes from 0 (MSB) to BlockSize-1 (LSB)
	for (size_t byteIdx = 0; byteIdx < BlockSize; byteIdx++) {
		uint8_t expByte = exponent[byteIdx];

		// Process bits from MSB to LSB within the byte
		for (int bit = 7; bit >= 0; bit--) {
			if (foundFirstBit) {
				// Square: result = result² mod modulus
				std::array<uint8_t, BlockSize> squared;
				MontgomeryMultiply(
					std::span<uint8_t, BlockSize>(squared),
					std::span<const uint8_t, BlockSize>(result.data(), BlockSize),
					std::span<const uint8_t, BlockSize>(result.data(), BlockSize),
					modulus
				);
				std::memcpy(result.data(), squared.data(), BlockSize);
			}

			bool bitSet = (expByte & (1 << bit)) != 0;
			if (bitSet) {
				foundFirstBit = true;
				// Multiply: result = result × base mod modulus
				std::array<uint8_t, BlockSize> temp;
				MontgomeryMultiply(
					std::span<uint8_t, BlockSize>(temp),
					std::span<const uint8_t, BlockSize>(result.data(), BlockSize),
					base,
					modulus
				);
				std::memcpy(result.data(), temp.data(), BlockSize);
			}
		}
	}
}

// ============================================================================
// Block Encryption
// ============================================================================

void EncryptBlock(
	std::span<uint8_t, BlockSize> output,
	std::span<const uint8_t, OutputBytesPerBlock> block,
	uint8_t accumulator  // Input only (from previous block's last byte)
) {
	// Build 51-byte encoded block before RSA
	//
	// For decryption to work with forward cumulative sum:
	//   encodedBlock[1] = p0 - accumulator (first byte)
	//   encodedBlock[k+1] = pk - p(k-1) for k=1..49 (differences)
	//
	// Then decryption: output[k] = cumsum(buf[1..k+1]) = pk

	std::array<uint8_t, BlockSize> encodedBlock;
	encodedBlock[0] = 0x15; // Padding byte (matches original Lynx encryption)

	// First difference: p0 - accumulator
	encodedBlock[1] = static_cast<uint8_t>(block[0] - accumulator);

	// Remaining differences: pk - p(k-1) for k=1..49
	for (size_t k = 1; k < OutputBytesPerBlock; k++) {
		encodedBlock[k + 1] = static_cast<uint8_t>(block[k] - block[k - 1]);
	}

	// RSA encryption: result = encodedBlock^d mod PublicModulus
	std::array<uint8_t, BlockSize> rsaResult;
	ModularExponentiate(
		std::span<uint8_t, BlockSize>(rsaResult),
		std::span<const uint8_t, BlockSize>(encodedBlock),
		std::span<const uint8_t, BlockSize>(PrivateExponent),
		std::span<const uint8_t, BlockSize>(PublicModulus)
	);

	// Reverse bytes: output[i] = rsaResult[50-i]
	// This matches how decryption reverses when loading
	for (size_t i = 0; i < BlockSize; i++) {
		output[i] = rsaResult[BlockSize - 1 - i];
	}
}

// ============================================================================
// High-Level Encryption API
// ============================================================================

EncryptResult Encrypt(std::span<const uint8_t> plaintext) {
	EncryptResult result;

	if (plaintext.empty() || plaintext.size() > 15 * OutputBytesPerBlock) {
		// Max 15 blocks × 50 bytes = 750 bytes
		return result;
	}

	// Calculate block count (round up)
	size_t blocks = (plaintext.size() + OutputBytesPerBlock - 1) / OutputBytesPerBlock;
	result.BlockCount = blocks;

	// Allocate output: 1 byte block count + blocks × 51 bytes
	result.Data.resize(1 + blocks * BlockSize);

	// Write block count byte (256 - N)
	result.Data[0] = static_cast<uint8_t>(256 - blocks);

	// Pad plaintext to full blocks
	std::vector<uint8_t> paddedPlaintext(blocks * OutputBytesPerBlock, 0);
	std::memcpy(paddedPlaintext.data(), plaintext.data(), plaintext.size());

	// Process blocks in FORWARD order
	// For each block, accumulator is the last byte of the PREVIOUS plaintext block
	// (or 0 for the first block)
	uint8_t accumulator = 0;

	for (size_t blockIdx = 0; blockIdx < blocks; blockIdx++) {
		// Get this block's plaintext
		std::array<uint8_t, OutputBytesPerBlock> blockInput;
		std::memcpy(blockInput.data(),
			&paddedPlaintext[blockIdx * OutputBytesPerBlock],
			OutputBytesPerBlock);

		// Encrypt block
		std::array<uint8_t, BlockSize> encryptedBlock;
		EncryptBlock(
			std::span<uint8_t, BlockSize>(encryptedBlock),
			std::span<const uint8_t, OutputBytesPerBlock>(blockInput),
			accumulator
		);

		// Copy to output
		std::memcpy(&result.Data[1 + blockIdx * BlockSize],
			encryptedBlock.data(), BlockSize);

		// Update accumulator to last byte of this block for next block
		accumulator = blockInput[OutputBytesPerBlock - 1];
	}

	result.Valid = true;
	return result;
}

} // namespace LynxCrypto
