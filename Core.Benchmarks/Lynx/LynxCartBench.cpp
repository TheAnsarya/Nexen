#include "pch.h"
#include <array>
#include <cstring>

// =============================================================================
// Lynx Cartridge Benchmarks
// =============================================================================
// The Lynx cartridge uses bank-switching to access ROMs larger than the
// 64 KB address space. These benchmarks test header parsing, bank switching,
// and ROM access patterns.
//
// LNX Header Format (64 bytes):
//   - Magic: "LYNX" (4 bytes)
//   - Bank0 size (2 bytes LE, 256-byte pages)
//   - Bank1 size (2 bytes LE, 256-byte pages)
//   - Version (2 bytes)
//   - Cart name (32 bytes)
//   - Manufacturer (16 bytes)
//   - Rotation (1 byte: 0=none, 1=left, 2=right)
//   - Reserved (5 bytes)
//
// References:
//   - Handy Lynx Emulator source
//   - LynxCart.cpp (implementation)

// -----------------------------------------------------------------------------
// Header Parsing
// -----------------------------------------------------------------------------

// Benchmark magic number validation
static void BM_LynxCart_ValidateMagic(benchmark::State& state) {
	const char validMagic[4] = { 'L', 'Y', 'N', 'X' };
	std::array<char, 4> headerMagic = { 'L', 'Y', 'N', 'X' };

	for (auto _ : state) {
		bool isValid = (headerMagic[0] == validMagic[0] &&
		                headerMagic[1] == validMagic[1] &&
		                headerMagic[2] == validMagic[2] &&
		                headerMagic[3] == validMagic[3]);
		benchmark::DoNotOptimize(isValid);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ValidateMagic);

// Benchmark magic validation using memcmp
static void BM_LynxCart_ValidateMagic_Memcmp(benchmark::State& state) {
	const char validMagic[4] = { 'L', 'Y', 'N', 'X' };
	std::array<char, 4> headerMagic = { 'L', 'Y', 'N', 'X' };

	for (auto _ : state) {
		bool isValid = (std::memcmp(headerMagic.data(), validMagic, 4) == 0);
		benchmark::DoNotOptimize(isValid);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ValidateMagic_Memcmp);

// Benchmark bank size extraction from header
static void BM_LynxCart_ExtractBankSize(benchmark::State& state) {
	// Simulated header bytes at offset 4-7
	uint8_t header[] = { 0x00, 0x02, 0x00, 0x00 }; // 512 pages = 128 KB

	for (auto _ : state) {
		// Little-endian 16-bit values
		uint16_t bank0Pages = header[0] | (header[1] << 8);
		uint16_t bank1Pages = header[2] | (header[3] << 8);
		uint32_t bank0Size = bank0Pages * 256;
		uint32_t bank1Size = bank1Pages * 256;
		benchmark::DoNotOptimize(bank0Size);
		benchmark::DoNotOptimize(bank1Size);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ExtractBankSize);

// Benchmark rotation byte parsing
static void BM_LynxCart_ParseRotation(benchmark::State& state) {
	uint8_t rotationBytes[] = { 0, 1, 2, 0, 1, 2 };
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t rotByte = rotationBytes[idx % 6];
		uint8_t rotation;
		switch (rotByte) {
			case 0: rotation = 0; break; // None
			case 1: rotation = 1; break; // Left
			case 2: rotation = 2; break; // Right
			default: rotation = 0; break;
		}
		benchmark::DoNotOptimize(rotation);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ParseRotation);

// -----------------------------------------------------------------------------
// Bank Switching
// -----------------------------------------------------------------------------

// Benchmark bank selection register write
static void BM_LynxCart_BankSwitch(benchmark::State& state) {
	uint8_t currentBank = 0;
	uint8_t bankValues[] = { 0, 1, 2, 3, 0, 2 };
	size_t idx = 0;

	for (auto _ : state) {
		currentBank = bankValues[idx % 6];
		benchmark::DoNotOptimize(currentBank);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_BankSwitch);

// Benchmark ROM address calculation with banking
static void BM_LynxCart_AddressCalculation(benchmark::State& state) {
	uint8_t bank = 2;
	uint32_t bankSize = 256 * 1024; // 256 KB per bank
	uint16_t offset = 0x1234;

	for (auto _ : state) {
		uint32_t romAddress = (bank * bankSize) + offset;
		benchmark::DoNotOptimize(romAddress);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_AddressCalculation);

// Benchmark address calculation with shift (power-of-2 bank size)
static void BM_LynxCart_AddressCalculation_Shift(benchmark::State& state) {
	uint8_t bank = 2;
	uint8_t bankShift = 18; // 256 KB = 2^18
	uint16_t offset = 0x1234;

	for (auto _ : state) {
		uint32_t romAddress = (bank << bankShift) | offset;
		benchmark::DoNotOptimize(romAddress);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_AddressCalculation_Shift);

// -----------------------------------------------------------------------------
// ROM Access
// -----------------------------------------------------------------------------

// Benchmark ROM byte read
static void BM_LynxCart_ReadByte(benchmark::State& state) {
	std::array<uint8_t, 256 * 1024> romData{};
	// Fill with test pattern
	for (size_t i = 0; i < romData.size(); i++) {
		romData[i] = static_cast<uint8_t>(i & 0xFF);
	}

	size_t addr = 0;
	for (auto _ : state) {
		uint8_t value = romData[addr % romData.size()];
		benchmark::DoNotOptimize(value);
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ReadByte);

// Benchmark ROM word read (16-bit)
static void BM_LynxCart_ReadWord(benchmark::State& state) {
	std::array<uint8_t, 256 * 1024> romData{};
	for (size_t i = 0; i < romData.size(); i++) {
		romData[i] = static_cast<uint8_t>(i & 0xFF);
	}

	size_t addr = 0;
	for (auto _ : state) {
		uint16_t value = romData[addr % (romData.size() - 1)] |
		                 (romData[(addr + 1) % romData.size()] << 8);
		benchmark::DoNotOptimize(value);
		addr += 2;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ReadWord);

// Benchmark sequential ROM read (simulating code fetch)
static void BM_LynxCart_SequentialRead(benchmark::State& state) {
	std::array<uint8_t, 64 * 1024> romData{};
	for (size_t i = 0; i < romData.size(); i++) {
		romData[i] = static_cast<uint8_t>(i & 0xFF);
	}

	for (auto _ : state) {
		uint32_t sum = 0;
		for (size_t i = 0; i < 256; i++) {
			sum += romData[i];
		}
		benchmark::DoNotOptimize(sum);
	}
	state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(BM_LynxCart_SequentialRead);

// -----------------------------------------------------------------------------
// Shift Register Counter
// -----------------------------------------------------------------------------

// Benchmark shift register counter update (Lynx cart addressing)
static void BM_LynxCart_ShiftCounter(benchmark::State& state) {
	uint32_t shiftCounter = 0;
	uint32_t counterMask = 0x000FFFFF; // 20-bit counter

	for (auto _ : state) {
		shiftCounter = (shiftCounter + 1) & counterMask;
		benchmark::DoNotOptimize(shiftCounter);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_ShiftCounter);

// Benchmark AUDIN/CART address strobe handling
static void BM_LynxCart_AddressStrobe(benchmark::State& state) {
	uint32_t address = 0;
	bool strobeActive = false;

	for (auto _ : state) {
		// AUDIN toggles cart address strobe
		strobeActive = !strobeActive;
		if (strobeActive) {
			address++;
		}
		benchmark::DoNotOptimize(address);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_AddressStrobe);

// -----------------------------------------------------------------------------
// CRC32 Calculation
// -----------------------------------------------------------------------------

// Benchmark CRC32 update (for ROM identification)
static void BM_LynxCart_Crc32Update(benchmark::State& state) {
	// Pre-computed CRC32 table (first 8 entries for benchmark)
	static constexpr uint32_t crcTable[8] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
		0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3
	};

	uint32_t crc = 0xFFFFFFFF;
	uint8_t dataByte = 0xAB;

	for (auto _ : state) {
		uint8_t index = (crc ^ dataByte) & 0x07;
		crc = crcTable[index] ^ (crc >> 8);
		benchmark::DoNotOptimize(crc);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_Crc32Update);

// -----------------------------------------------------------------------------
// ROM Size Detection
// -----------------------------------------------------------------------------

// Benchmark ROM size calculation from header
static void BM_LynxCart_CalculateTotalSize(benchmark::State& state) {
	// Various header configurations
	struct HeaderConfig {
		uint16_t bank0Pages;
		uint16_t bank1Pages;
	};
	HeaderConfig configs[] = {
		{ 256, 0 },   // 64 KB
		{ 512, 0 },   // 128 KB
		{ 1024, 0 },  // 256 KB
		{ 2048, 0 },  // 512 KB
		{ 512, 512 }, // 256 KB split
	};
	size_t idx = 0;

	for (auto _ : state) {
		auto& cfg = configs[idx % 5];
		uint32_t totalSize = (cfg.bank0Pages + cfg.bank1Pages) * 256;
		benchmark::DoNotOptimize(totalSize);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_CalculateTotalSize);

// Benchmark power-of-2 check for banking optimization
static void BM_LynxCart_IsPowerOf2(benchmark::State& state) {
	uint32_t sizes[] = { 65536, 131072, 262144, 524288, 100000 };
	size_t idx = 0;

	for (auto _ : state) {
		uint32_t size = sizes[idx % 5];
		bool isPow2 = (size & (size - 1)) == 0;
		benchmark::DoNotOptimize(isPow2);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_IsPowerOf2);

// -----------------------------------------------------------------------------
// Headerless ROM Handling
// -----------------------------------------------------------------------------

// Benchmark headerless detection (no LYNX magic)
static void BM_LynxCart_DetectHeaderless(benchmark::State& state) {
	std::array<char, 4> headers[] = {
		{ 'L', 'Y', 'N', 'X' },  // Valid
		{ 0x00, 0x00, 0x00, 0x00 }, // Headerless
		{ 'B', 'S', '9', '3' },  // Not Lynx
	};
	size_t idx = 0;

	for (auto _ : state) {
		auto& hdr = headers[idx % 3];
		bool hasHeader = (hdr[0] == 'L' && hdr[1] == 'Y' &&
		                  hdr[2] == 'N' && hdr[3] == 'X');
		benchmark::DoNotOptimize(hasHeader);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxCart_DetectHeaderless);

