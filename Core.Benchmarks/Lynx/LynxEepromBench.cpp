#include "pch.h"
#include <array>
#include <cstring>

// =============================================================================
// Lynx EEPROM Benchmarks
// =============================================================================
// The Lynx supports Microwire EEPROM (93C46/56/66/76/86) for save data.
// These benchmarks test EEPROM protocol handling, state machine transitions,
// and read/write operations.
//
// EEPROM Communication:
//   - Serial Microwire protocol
//   - Clock signal (AUDIN), Data In (EEPDAT), Chip Select logic
//   - Commands: READ, WRITE, ERASE, ERAL, WRAL, EWEN, EWDS
//
// References:
//   - 93C46 Datasheet (Microchip)
//   - LynxEeprom.cpp (implementation)

// -----------------------------------------------------------------------------
// State Machine Operations
// -----------------------------------------------------------------------------

/// <summary>EEPROM state machine states.</summary>
enum class EepromState : uint8_t {
	Idle,
	ReceivingOpcode,
	ReceivingAddress,
	ReceivingData,
	WritingData,
	ReadingData,
};

// Benchmark state machine transition check
static void BM_LynxEeprom_StateTransition(benchmark::State& state) {
	EepromState currentState = EepromState::Idle;
	bool chipSelect = true;
	bool clockRising = true;

	for (auto _ : state) {
		// Typical state transition logic
		if (!chipSelect) {
			currentState = EepromState::Idle;
		} else if (clockRising) {
			if (currentState == EepromState::Idle) {
				currentState = EepromState::ReceivingOpcode;
			}
		}
		benchmark::DoNotOptimize(currentState);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_StateTransition);

// Benchmark serial bit reception
static void BM_LynxEeprom_ReceiveBit(benchmark::State& state) {
	uint16_t shiftRegister = 0;
	uint8_t bitCount = 0;
	bool dataIn = true;

	for (auto _ : state) {
		// Shift in new bit (MSB first)
		shiftRegister = (shiftRegister << 1) | (dataIn ? 1 : 0);
		bitCount++;
		benchmark::DoNotOptimize(shiftRegister);
		benchmark::DoNotOptimize(bitCount);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_ReceiveBit);

// -----------------------------------------------------------------------------
// Command Parsing
// -----------------------------------------------------------------------------

// Benchmark opcode decoding
static void BM_LynxEeprom_DecodeOpcode(benchmark::State& state) {
	uint8_t opcodes[] = { 0x00, 0x01, 0x02, 0x03 }; // EWDS, WRAL/ERAL, ERASE/WRITE, READ
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t opcode = opcodes[idx % 4];
		uint8_t command;

		switch (opcode >> 1) {
			case 0: command = 0; break; // Extended command
			case 1: command = 1; break; // WRITE
			case 2: command = 2; break; // READ
			case 3: command = 3; break; // ERASE
			default: command = 0xFF; break;
		}

		benchmark::DoNotOptimize(command);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_DecodeOpcode);

// Benchmark address extraction from shift register
static void BM_LynxEeprom_ExtractAddress(benchmark::State& state) {
	uint16_t shiftRegister = 0x1A5; // Example address bits

	for (auto _ : state) {
		// 93C46: 7-bit address
		uint8_t addr_7bit = shiftRegister & 0x7F;
		// 93C66: 9-bit address
		uint16_t addr_9bit = shiftRegister & 0x1FF;

		benchmark::DoNotOptimize(addr_7bit);
		benchmark::DoNotOptimize(addr_9bit);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_ExtractAddress);

// -----------------------------------------------------------------------------
// Memory Operations
// -----------------------------------------------------------------------------

// Benchmark EEPROM word read
static void BM_LynxEeprom_ReadWord(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	// Initialize with test pattern
	for (size_t i = 0; i < eepromData.size(); i++) {
		eepromData[i] = static_cast<uint16_t>(i * 0x101);
	}

	size_t addr = 0;
	for (auto _ : state) {
		uint16_t value = eepromData[addr % eepromData.size()];
		benchmark::DoNotOptimize(value);
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_ReadWord);

// Benchmark EEPROM word write
static void BM_LynxEeprom_WriteWord(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	size_t addr = 0;
	uint16_t writeValue = 0xABCD;

	for (auto _ : state) {
		eepromData[addr % eepromData.size()] = writeValue;
		benchmark::ClobberMemory();
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_WriteWord);

// Benchmark EEPROM erase (set to all 1s)
static void BM_LynxEeprom_EraseWord(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	size_t addr = 0;

	for (auto _ : state) {
		eepromData[addr % eepromData.size()] = 0xFFFF;
		benchmark::ClobberMemory();
		addr++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_EraseWord);

// Benchmark ERAL (erase all)
static void BM_LynxEeprom_EraseAll(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};

	for (auto _ : state) {
		std::memset(eepromData.data(), 0xFF, eepromData.size() * sizeof(uint16_t));
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_EraseAll);

// Benchmark WRAL (write all)
static void BM_LynxEeprom_WriteAll(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	uint16_t pattern = 0x5A5A;

	for (auto _ : state) {
		std::fill(eepromData.begin(), eepromData.end(), pattern);
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_WriteAll);

// -----------------------------------------------------------------------------
// Serial Output
// -----------------------------------------------------------------------------

// Benchmark serial bit output (MSB first)
static void BM_LynxEeprom_OutputBit(benchmark::State& state) {
	uint16_t dataWord = 0xABCD;
	uint8_t bitIndex = 15; // Start at MSB

	for (auto _ : state) {
		bool outBit = (dataWord >> bitIndex) & 1;
		bitIndex = (bitIndex == 0) ? 15 : bitIndex - 1;
		benchmark::DoNotOptimize(outBit);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_OutputBit);

// Benchmark full word serial transmission (16 bits)
static void BM_LynxEeprom_TransmitWord(benchmark::State& state) {
	uint16_t dataWord = 0xABCD;

	for (auto _ : state) {
		for (int i = 15; i >= 0; i--) {
			bool outBit = (dataWord >> i) & 1;
			benchmark::DoNotOptimize(outBit);
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_TransmitWord);

// -----------------------------------------------------------------------------
// Write Protection
// -----------------------------------------------------------------------------

// Benchmark write enable/disable check
static void BM_LynxEeprom_WriteProtectionCheck(benchmark::State& state) {
	bool writeEnabled = false;
	uint8_t command = 2; // Example: WRITE

	for (auto _ : state) {
		bool canWrite = writeEnabled && (command == 1 || command == 2);
		benchmark::DoNotOptimize(canWrite);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_WriteProtectionCheck);

// -----------------------------------------------------------------------------
// EEPROM Type Detection
// -----------------------------------------------------------------------------

// Benchmark EEPROM size lookup by type
static void BM_LynxEeprom_SizeLookup(benchmark::State& state) {
	uint8_t types[] = { 0, 1, 2, 3, 4, 5 }; // None, 93C46-86
	uint16_t sizes[] = { 0, 128, 256, 512, 1024, 2048 }; // Words
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t eepromType = types[idx % 6];
		uint16_t size = sizes[eepromType];
		benchmark::DoNotOptimize(size);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_SizeLookup);

// Benchmark address bit count by EEPROM type
static void BM_LynxEeprom_AddressBitCount(benchmark::State& state) {
	uint8_t types[] = { 1, 2, 3, 4, 5 }; // 93C46-86
	uint8_t addrBits[] = { 7, 8, 9, 10, 11 }; // Address bits per type
	size_t idx = 0;

	for (auto _ : state) {
		uint8_t eepromType = types[idx % 5];
		uint8_t bits = addrBits[eepromType - 1];
		benchmark::DoNotOptimize(bits);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_AddressBitCount);

// -----------------------------------------------------------------------------
// Integration Scenarios
// -----------------------------------------------------------------------------

// Benchmark typical read cycle (opcode + address + data out)
static void BM_LynxEeprom_FullReadCycle(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	for (size_t i = 0; i < eepromData.size(); i++) {
		eepromData[i] = static_cast<uint16_t>(i * 0x101);
	}

	for (auto _ : state) {
		// Simulate: receive 3-bit opcode (READ=10b) + 7-bit address
		uint16_t shiftReg = 0x2A5; // READ + address 0x25
		uint8_t address = shiftReg & 0x7F;

		// Read data word
		uint16_t dataOut = eepromData[address];

		// Serial output 16 bits
		for (int i = 15; i >= 0; i--) {
			bool bit = (dataOut >> i) & 1;
			benchmark::DoNotOptimize(bit);
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_FullReadCycle);

// Benchmark typical write cycle (opcode + address + data in)
static void BM_LynxEeprom_FullWriteCycle(benchmark::State& state) {
	std::array<uint16_t, 128> eepromData{};
	bool writeEnabled = true;

	for (auto _ : state) {
		// Simulate: WRITE opcode + address + 16-bit data
		if (writeEnabled) {
			uint8_t address = 0x25;
			uint16_t dataIn = 0xBEEF;
			eepromData[address] = dataIn;
			benchmark::ClobberMemory();
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxEeprom_FullWriteCycle);

