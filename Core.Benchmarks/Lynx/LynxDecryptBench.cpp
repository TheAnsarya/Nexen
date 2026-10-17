#include "pch.h"
#include <array>
#include <cstring>
#include <vector>

// =============================================================================
// Lynx RSA Decryption Benchmarks
// =============================================================================
// Benchmarks for the Lynx bootloader RSA decryption system.
// The Lynx uses Montgomery multiplication for RSA: B³ mod N
//
// Key performance factors:
//   - Montgomery multiplication: O(n²) where n = 408 bits
//   - Two multiplications per block: B² = B×B, then B³ = B×B²
//   - 50 accumulator operations per block
//   - Typical loader: 8 blocks = 16 Montgomery multiplications
//
// References:
//   - LynxDecrypt.h/.cpp (implementation)
//   - ~docs/plans/lynx-bootloader-encryption-research.md

// Lynx RSA constants (for standalone benchmarks)
namespace LynxCryptoBench {

constexpr size_t BlockSize = 51;
constexpr size_t OutputBytesPerBlock = 50;

const uint8_t PublicModulus[51] = {
	0x35, 0xB5, 0xA3, 0x94, 0x28, 0x06, 0xD8, 0xA2,
	0x26, 0x95, 0xD7, 0x71, 0xB2, 0x3C, 0xFD, 0x56,
	0x1C, 0x4A, 0x19, 0xB6, 0xA3, 0xB0, 0x26, 0x00,
	0x36, 0x5A, 0x30, 0x6E, 0x3C, 0x4D, 0x63, 0x38,
	0x1B, 0xD4, 0x1C, 0x13, 0x64, 0x89, 0x36, 0x4C,
	0xF2, 0xBA, 0x2A, 0x58, 0xF4, 0xFE, 0xE1, 0xFD,
	0xAC, 0x7E, 0x79
};

// Big integer helpers (same as LynxDecrypt.cpp)
void DoubleValue(uint8_t* value, size_t length) {
	int carry = 0;
	for (int i = static_cast<int>(length) - 1; i >= 0; i--) {
		int tmp = 2 * value[i] + carry;
		value[i] = static_cast<uint8_t>(tmp & 0xFF);
		carry = tmp >> 8;
	}
}

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
	return borrow == 0;
}

void AddValue(uint8_t* result, const uint8_t* addend, size_t length) {
	int carry = 0;
	for (int i = static_cast<int>(length) - 1; i >= 0; i--) {
		int tmp = result[i] + addend[i] + carry;
		carry = (tmp >= 256) ? 1 : 0;
		result[i] = static_cast<uint8_t>(tmp & 0xFF);
	}
}

void MontgomeryMultiply(uint8_t* result, const uint8_t* M, const uint8_t* N, const uint8_t* mod) {
	std::memset(result, 0, BlockSize);

	for (size_t i = 0; i < BlockSize; i++) {
		uint8_t nByte = N[i];
		for (int j = 0; j < 8; j++) {
			DoubleValue(result, BlockSize);
			bool bit = (nByte & 0x80) != 0;
			nByte <<= 1;

			if (bit) {
				AddValue(result, M, BlockSize);
				bool noBorrow = SubtractValue(result, mod, BlockSize);
				if (noBorrow) {
					SubtractValue(result, mod, BlockSize);
				}
			} else {
				std::array<uint8_t, BlockSize> backup;
				std::memcpy(backup.data(), result, BlockSize);
				bool noBorrow = SubtractValue(result, mod, BlockSize);
				if (!noBorrow) {
					std::memcpy(result, backup.data(), BlockSize);
				}
			}
		}
	}
}

} // namespace LynxCryptoBench

// -----------------------------------------------------------------------------
// Big Integer Arithmetic Benchmarks
// -----------------------------------------------------------------------------

static void BM_LynxDecrypt_DoubleValue(benchmark::State& state) {
	std::array<uint8_t, 51> value{};
	value[50] = 0x01;

	for (auto _ : state) {
		LynxCryptoBench::DoubleValue(value.data(), 51);
		benchmark::DoNotOptimize(value[0]);
		// Reset to prevent overflow
		if (value[0] > 0x80) {
			value[0] = 0;
			value[50] = 0x01;
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_DoubleValue);

static void BM_LynxDecrypt_AddValue(benchmark::State& state) {
	std::array<uint8_t, 51> result{};
	std::array<uint8_t, 51> addend{};
	addend[50] = 0x01;

	for (auto _ : state) {
		LynxCryptoBench::AddValue(result.data(), addend.data(), 51);
		benchmark::DoNotOptimize(result[0]);
		if (result[0] > 0x80) {
			std::memset(result.data(), 0, 51);
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_AddValue);

static void BM_LynxDecrypt_SubtractValue(benchmark::State& state) {
	std::array<uint8_t, 51> result{};
	result[0] = 0xFF;
	result[50] = 0xFF;
	std::array<uint8_t, 51> subtrahend{};
	subtrahend[50] = 0x01;

	for (auto _ : state) {
		LynxCryptoBench::SubtractValue(result.data(), subtrahend.data(), 51);
		benchmark::DoNotOptimize(result[50]);
		if (result[50] < 0x10) {
			result[50] = 0xFF;
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_SubtractValue);

// -----------------------------------------------------------------------------
// Montgomery Multiplication Benchmarks
// -----------------------------------------------------------------------------

static void BM_LynxDecrypt_Montgomery_SmallValues(benchmark::State& state) {
	std::array<uint8_t, 51> M{};
	std::array<uint8_t, 51> N{};
	std::array<uint8_t, 51> result{};

	M[50] = 0x05;  // Small value
	N[50] = 0x03;

	for (auto _ : state) {
		LynxCryptoBench::MontgomeryMultiply(
			result.data(), M.data(), N.data(), LynxCryptoBench::PublicModulus
		);
		benchmark::DoNotOptimize(result[0]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_Montgomery_SmallValues);

static void BM_LynxDecrypt_Montgomery_LargeValues(benchmark::State& state) {
	std::array<uint8_t, 51> M{};
	std::array<uint8_t, 51> N{};
	std::array<uint8_t, 51> result{};

	// Fill with large values (close to modulus)
	for (size_t i = 0; i < 51; i++) {
		M[i] = 0x30 + static_cast<uint8_t>(i);
		N[i] = 0x20 + static_cast<uint8_t>(i * 2);
	}

	for (auto _ : state) {
		LynxCryptoBench::MontgomeryMultiply(
			result.data(), M.data(), N.data(), LynxCryptoBench::PublicModulus
		);
		benchmark::DoNotOptimize(result[0]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_Montgomery_LargeValues);

static void BM_LynxDecrypt_Montgomery_Squaring(benchmark::State& state) {
	std::array<uint8_t, 51> M{};
	std::array<uint8_t, 51> result{};

	for (size_t i = 0; i < 51; i++) {
		M[i] = static_cast<uint8_t>(i * 5);
	}

	for (auto _ : state) {
		// B² = B × B (squaring, as used in RSA)
		LynxCryptoBench::MontgomeryMultiply(
			result.data(), M.data(), M.data(), LynxCryptoBench::PublicModulus
		);
		benchmark::DoNotOptimize(result[0]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_Montgomery_Squaring);

// -----------------------------------------------------------------------------
// Block Decryption Benchmarks
// -----------------------------------------------------------------------------

static void BM_LynxDecrypt_SingleBlock(benchmark::State& state) {
	std::array<uint8_t, 51> block{};
	std::array<uint8_t, 51> squared{};
	std::array<uint8_t, 51> cubed{};

	for (size_t i = 0; i < 51; i++) {
		block[i] = static_cast<uint8_t>(i * 3);
	}

	for (auto _ : state) {
		// B² = B × B
		LynxCryptoBench::MontgomeryMultiply(
			squared.data(), block.data(), block.data(), LynxCryptoBench::PublicModulus
		);
		// B³ = B × B²
		LynxCryptoBench::MontgomeryMultiply(
			cubed.data(), block.data(), squared.data(), LynxCryptoBench::PublicModulus
		);

		benchmark::DoNotOptimize(cubed[0]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_SingleBlock);

static void BM_LynxDecrypt_SingleBlock_WithAccumulator(benchmark::State& state) {
	std::array<uint8_t, 51> block{};
	std::array<uint8_t, 51> squared{};
	std::array<uint8_t, 51> cubed{};
	std::array<uint8_t, 50> output{};

	for (size_t i = 0; i < 51; i++) {
		block[i] = static_cast<uint8_t>(i * 7);
	}

	for (auto _ : state) {
		// RSA: B³ mod N
		LynxCryptoBench::MontgomeryMultiply(
			squared.data(), block.data(), block.data(), LynxCryptoBench::PublicModulus
		);
		LynxCryptoBench::MontgomeryMultiply(
			cubed.data(), block.data(), squared.data(), LynxCryptoBench::PublicModulus
		);

		// Accumulator obfuscation
		uint8_t accumulator = 0;
		for (size_t j = 0; j < 50; j++) {
			accumulator = static_cast<uint8_t>(accumulator + cubed[j + 1]);
			output[j] = accumulator;
		}

		benchmark::DoNotOptimize(output[49]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_SingleBlock_WithAccumulator);

// -----------------------------------------------------------------------------
// Full Loader Decryption Benchmarks
// -----------------------------------------------------------------------------

static void BM_LynxDecrypt_8Blocks_FullLoader(benchmark::State& state) {
	// Simulate typical 8-block loader decryption
	std::vector<uint8_t> encrypted(409);
	encrypted[0] = 0xF8; // 8 blocks
	for (size_t i = 1; i < 409; i++) {
		encrypted[i] = static_cast<uint8_t>(i * 13);
	}

	for (auto _ : state) {
		std::vector<uint8_t> output(400);
		uint8_t accumulator = 0;

		for (int blockIdx = 0; blockIdx < 8; blockIdx++) {
			std::array<uint8_t, 51> block{};
			std::memcpy(block.data(), &encrypted[1 + blockIdx * 51], 51);

			std::array<uint8_t, 51> squared{};
			std::array<uint8_t, 51> cubed{};

			LynxCryptoBench::MontgomeryMultiply(
				squared.data(), block.data(), block.data(), LynxCryptoBench::PublicModulus
			);
			LynxCryptoBench::MontgomeryMultiply(
				cubed.data(), block.data(), squared.data(), LynxCryptoBench::PublicModulus
			);

			for (size_t j = 0; j < 50; j++) {
				accumulator = static_cast<uint8_t>(accumulator + cubed[j + 1]);
				output[blockIdx * 50 + j] = accumulator;
			}
		}

		benchmark::DoNotOptimize(output[399]);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_8Blocks_FullLoader);

// -----------------------------------------------------------------------------
// Validation Benchmarks
// -----------------------------------------------------------------------------

static void BM_LynxDecrypt_Validate(benchmark::State& state) {
	std::vector<uint8_t> encrypted(409);
	encrypted[0] = 0xF8; // 8 blocks

	for (auto _ : state) {
		size_t blocks = 256 - encrypted[0];
		bool valid = (blocks > 0 && blocks <= 15);
		valid = valid && (encrypted.size() >= 1 + blocks * 51);
		benchmark::DoNotOptimize(valid);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_Validate);

static void BM_LynxDecrypt_GetBlockCount(benchmark::State& state) {
	std::vector<uint8_t> encrypted = { 0xF8 };

	for (auto _ : state) {
		size_t blocks = 256 - encrypted[0];
		benchmark::DoNotOptimize(blocks);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_GetBlockCount);

static void BM_LynxDecrypt_GetDecryptedSize(benchmark::State& state) {
	std::vector<uint8_t> encrypted = { 0xF8 };

	for (auto _ : state) {
		size_t blocks = 256 - encrypted[0];
		size_t decryptedSize = blocks * 50;
		benchmark::DoNotOptimize(decryptedSize);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxDecrypt_GetDecryptedSize);

