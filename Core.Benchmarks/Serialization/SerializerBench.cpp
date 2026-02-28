#include "pch.h"
#include <array>
#include <vector>
#include <cstring>
#include <sstream>
#include "Utilities/Serializer.h"

// =============================================================================
// Serialization/Deserialization Benchmarks
// =============================================================================
// Nexen uses a key-value serialization system for save states and rewind buffer.
// Performance is critical as save states are created frequently during rewind.

// -----------------------------------------------------------------------------
// Helper Types for Benchmarking
// -----------------------------------------------------------------------------

/// <summary>Mock serializable object for benchmarking</summary>
class MockSerializableState : public ISerializable {
public:
	uint32_t registers[16] = {};
	uint16_t programCounter = 0;
	uint8_t statusFlags = 0;
	bool interruptEnable = false;
	int32_t cycleCount = 0;

	void Serialize(Serializer& s) override {
		SVArray(registers, 16);
		SV(programCounter);
		SV(statusFlags);
		SV(interruptEnable);
		SV(cycleCount);
	}
};

/// <summary>Larger mock state simulating PPU state</summary>
class MockPpuState : public ISerializable {
public:
	std::array<uint8_t, 0x800> vram = {};  // 2KB VRAM
	std::array<uint8_t, 256> oam = {};     // 256 bytes OAM
	std::array<uint8_t, 32> palette = {};   // 32 bytes palette
	uint16_t scanline = 0;
	uint16_t cycle = 0;
	uint8_t control = 0;
	uint8_t mask = 0;
	uint8_t status = 0;
	bool frameOdd = false;
	bool nmiOccurred = false;
	bool spriteOverflow = false;

	void Serialize(Serializer& s) override {
		SVArray(vram.data(), (uint32_t)vram.size());
		SVArray(oam.data(), (uint32_t)oam.size());
		SVArray(palette.data(), (uint32_t)palette.size());
		SV(scanline);
		SV(cycle);
		SV(control);
		SV(mask);
		SV(status);
		SV(frameOdd);
		SV(nmiOccurred);
		SV(spriteOverflow);
	}
};

/// <summary>Large mock state simulating full console RAM</summary>
class MockConsoleState : public ISerializable {
public:
	std::array<uint8_t, 0x10000> ram = {};  // 64KB RAM
	MockSerializableState cpu;
	MockPpuState ppu;

	void Serialize(Serializer& s) override {
		SVArray(ram.data(), (uint32_t)ram.size());
		SV(cpu);
		SV(ppu);
	}
};

// -----------------------------------------------------------------------------
// Binary Format Serialization (Save States)
// -----------------------------------------------------------------------------

// Benchmark serializing small CPU state
static void BM_Serializer_SaveSmallState(benchmark::State& state) {
	MockSerializableState cpuState;
	for (int i = 0; i < 16; i++) cpuState.registers[i] = static_cast<uint32_t>(i * 0x1000);
	cpuState.programCounter = 0x8000;
	cpuState.statusFlags = 0x24;
	cpuState.interruptEnable = true;
	cpuState.cycleCount = 12345;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		cpuState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockSerializableState));
}
BENCHMARK(BM_Serializer_SaveSmallState);

// Benchmark serializing PPU state (medium size with arrays)
static void BM_Serializer_SavePpuState(benchmark::State& state) {
	MockPpuState ppuState;
	// Initialize with some data
	for (size_t i = 0; i < ppuState.vram.size(); i++) {
		ppuState.vram[i] = static_cast<uint8_t>(i);
	}
	for (size_t i = 0; i < ppuState.oam.size(); i++) {
		ppuState.oam[i] = static_cast<uint8_t>(i * 2);
	}

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		ppuState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockPpuState));
}
BENCHMARK(BM_Serializer_SavePpuState);

// Benchmark serializing large console state (rewind buffer scenario)
static void BM_Serializer_SaveLargeState(benchmark::State& state) {
	MockConsoleState consoleState;
	// Initialize RAM with pattern
	for (size_t i = 0; i < consoleState.ram.size(); i++) {
		consoleState.ram[i] = static_cast<uint8_t>(i ^ (i >> 8));
	}

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		consoleState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockConsoleState));
}
BENCHMARK(BM_Serializer_SaveLargeState);

// -----------------------------------------------------------------------------
// Text Format Serialization (Debugging)
// -----------------------------------------------------------------------------

// Benchmark text format serialization (debugging/inspection)
static void BM_Serializer_SaveTextFormat(benchmark::State& state) {
	MockSerializableState cpuState;
	for (int i = 0; i < 16; i++) cpuState.registers[i] = static_cast<uint32_t>(i * 0x1000);
	cpuState.programCounter = 0x8000;
	cpuState.statusFlags = 0x24;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Text);
		cpuState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockSerializableState));
}
BENCHMARK(BM_Serializer_SaveTextFormat);

// -----------------------------------------------------------------------------
// Map Format Serialization (Lua API)
// -----------------------------------------------------------------------------

// Benchmark map format serialization (Lua scripting API)
static void BM_Serializer_SaveMapFormat(benchmark::State& state) {
	MockSerializableState cpuState;
	for (int i = 0; i < 16; i++) cpuState.registers[i] = static_cast<uint32_t>(i * 0x1000);
	cpuState.programCounter = 0x8000;
	cpuState.statusFlags = 0x24;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Map);
		cpuState.Serialize(s);
		benchmark::DoNotOptimize(s.GetMapValues());
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockSerializableState));
}
BENCHMARK(BM_Serializer_SaveMapFormat);

// -----------------------------------------------------------------------------
// Primitive Type Streaming
// -----------------------------------------------------------------------------

// Benchmark streaming individual uint8_t values
static void BM_Serializer_StreamUint8(benchmark::State& state) {
	uint8_t value = 0x42;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		for (int i = 0; i < 100; i++) {
			s.Stream(value, "value", i);
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_Serializer_StreamUint8);

// Benchmark streaming individual uint16_t values
static void BM_Serializer_StreamUint16(benchmark::State& state) {
	uint16_t value = 0x1234;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		for (int i = 0; i < 100; i++) {
			s.Stream(value, "value", i);
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_Serializer_StreamUint16);

// Benchmark streaming individual uint32_t values
static void BM_Serializer_StreamUint32(benchmark::State& state) {
	uint32_t value = 0x12345678;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		for (int i = 0; i < 100; i++) {
			s.Stream(value, "value", i);
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_Serializer_StreamUint32);

// Benchmark streaming boolean values
static void BM_Serializer_StreamBool(benchmark::State& state) {
	bool value = true;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		for (int i = 0; i < 100; i++) {
			s.Stream(value, "value", i);
			value = !value;
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations() * 100);
}
BENCHMARK(BM_Serializer_StreamBool);

// -----------------------------------------------------------------------------
// Array Streaming
// -----------------------------------------------------------------------------

// Benchmark streaming small arrays (registers)
static void BM_Serializer_StreamSmallArray(benchmark::State& state) {
	uint32_t registers[16];
	for (int i = 0; i < 16; i++) registers[i] = static_cast<uint32_t>(i * 0x1000);

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		s.StreamArray(registers, 16, "registers");
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(registers));
}
BENCHMARK(BM_Serializer_StreamSmallArray);

// Benchmark streaming medium arrays (VRAM-like)
static void BM_Serializer_StreamMediumArray(benchmark::State& state) {
	std::array<uint8_t, 0x800> vram;
	for (size_t i = 0; i < vram.size(); i++) vram[i] = static_cast<uint8_t>(i);

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		s.StreamArray(vram.data(), static_cast<uint32_t>(vram.size()), "vram");
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * vram.size());
}
BENCHMARK(BM_Serializer_StreamMediumArray);

// Benchmark streaming large arrays (RAM-like)
static void BM_Serializer_StreamLargeArray(benchmark::State& state) {
	std::vector<uint8_t> ram(0x10000);  // 64KB
	for (size_t i = 0; i < ram.size(); i++) ram[i] = static_cast<uint8_t>(i ^ (i >> 8));

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		s.StreamArray(ram.data(), static_cast<uint32_t>(ram.size()), "ram");
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * ram.size());
}
BENCHMARK(BM_Serializer_StreamLargeArray);

// -----------------------------------------------------------------------------
// Key Management
// -----------------------------------------------------------------------------

// Benchmark key prefix push/pop (nested serialization)
static void BM_Serializer_KeyPrefixManagement(benchmark::State& state) {
	MockSerializableState innerState;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		s.AddKeyPrefix("outer");
		s.AddKeyPrefix("inner");
		innerState.Serialize(s);
		s.RemoveKeyPrefix("inner");
		s.RemoveKeyPrefix("outer");
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Serializer_KeyPrefixManagement);

// Benchmark deeply nested serialization
static void BM_Serializer_DeepNesting(benchmark::State& state) {
	uint32_t value = 0x12345678;

	for (auto _ : state) {
		Serializer s(1, true, SerializeFormat::Binary);
		s.AddKeyPrefix("level1");
		s.AddKeyPrefix("level2");
		s.AddKeyPrefix("level3");
		s.AddKeyPrefix("level4");
		s.AddKeyPrefix("level5");
		s.Stream(value, "value");
		s.RemoveKeyPrefix("level5");
		s.RemoveKeyPrefix("level4");
		s.RemoveKeyPrefix("level3");
		s.RemoveKeyPrefix("level2");
		s.RemoveKeyPrefix("level1");
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Serializer_DeepNesting);

// -----------------------------------------------------------------------------
// Comparison Benchmarks
// -----------------------------------------------------------------------------

// =============================================================================
// FastBinary Format Serialization (Run-Ahead Hot Path)
// =============================================================================
// FastBinary eliminates all string key overhead: no key generation, no hash map
// lookups, no key prefix management. Data is read/written positionally.
// This is critical for run-ahead which saves+loads state every frame.

// Benchmark FastBinary save of small CPU state (compare with BM_Serializer_SaveSmallState)
static void BM_Serializer_SaveSmallState_FastBinary(benchmark::State& state) {
	MockSerializableState cpuState;
	for (int i = 0; i < 16; i++) cpuState.registers[i] = static_cast<uint32_t>(i * 0x1000);
	cpuState.programCounter = 0x8000;
	cpuState.statusFlags = 0x24;
	cpuState.interruptEnable = true;
	cpuState.cycleCount = 12345;

	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		cpuState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockSerializableState));
}
BENCHMARK(BM_Serializer_SaveSmallState_FastBinary);

// Benchmark FastBinary save of PPU state (compare with BM_Serializer_SavePpuState)
static void BM_Serializer_SavePpuState_FastBinary(benchmark::State& state) {
	MockPpuState ppuState;
	for (size_t i = 0; i < ppuState.vram.size(); i++) {
		ppuState.vram[i] = static_cast<uint8_t>(i);
	}
	for (size_t i = 0; i < ppuState.oam.size(); i++) {
		ppuState.oam[i] = static_cast<uint8_t>(i * 2);
	}

	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		ppuState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockPpuState));
}
BENCHMARK(BM_Serializer_SavePpuState_FastBinary);

// Benchmark FastBinary save of large console state (compare with BM_Serializer_SaveLargeState)
static void BM_Serializer_SaveLargeState_FastBinary(benchmark::State& state) {
	MockConsoleState consoleState;
	for (size_t i = 0; i < consoleState.ram.size(); i++) {
		consoleState.ram[i] = static_cast<uint8_t>(i ^ (i >> 8));
	}

	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		consoleState.Serialize(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockConsoleState));
}
BENCHMARK(BM_Serializer_SaveLargeState_FastBinary);

// Benchmark FastBinary round-trip (save + load) — the run-ahead pattern
static void BM_Serializer_RoundTrip_FastBinary(benchmark::State& state) {
	MockConsoleState consoleState;
	MockConsoleState loadTarget;
	for (size_t i = 0; i < consoleState.ram.size(); i++) {
		consoleState.ram[i] = static_cast<uint8_t>(i ^ (i >> 8));
	}

	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		// Save
		s.ResetForFastSave(1);
		consoleState.Serialize(s);

		// Load
		s.ResetForFastLoad();
		loadTarget.Serialize(s);
		benchmark::DoNotOptimize(loadTarget);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockConsoleState) * 2);
}
BENCHMARK(BM_Serializer_RoundTrip_FastBinary);

// Benchmark Binary round-trip (save + load) — the old run-ahead pattern
static void BM_Serializer_RoundTrip_Binary(benchmark::State& state) {
	MockConsoleState consoleState;
	MockConsoleState loadTarget;
	for (size_t i = 0; i < consoleState.ram.size(); i++) {
		consoleState.ram[i] = static_cast<uint8_t>(i ^ (i >> 8));
	}

	for (auto _ : state) {
		// Save (creates new serializer with string keys)
		Serializer save(1, true, SerializeFormat::Binary);
		consoleState.Serialize(save);

		// Write to stream then load back (emulates save state round-trip)
		std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
		save.SaveTo(ss, 0);  // No compression for fair comparison
		ss.seekg(0);

		Serializer load(1, false, SerializeFormat::Binary);
		load.LoadFrom(ss);
		loadTarget.Serialize(load);
		benchmark::DoNotOptimize(loadTarget);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockConsoleState) * 2);
}
BENCHMARK(BM_Serializer_RoundTrip_Binary);

// Benchmark FastBinary array streaming (compare with BM_Serializer_StreamLargeArray)
static void BM_Serializer_StreamLargeArray_FastBinary(benchmark::State& state) {
	std::vector<uint8_t> ram(0x10000);  // 64KB
	for (size_t i = 0; i < ram.size(); i++) ram[i] = static_cast<uint8_t>(i ^ (i >> 8));

	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		s.StreamArray(ram.data(), static_cast<uint32_t>(ram.size()), "ram");
		benchmark::DoNotOptimize(s);
	}
	state.SetBytesProcessed(state.iterations() * ram.size());
}
BENCHMARK(BM_Serializer_StreamLargeArray_FastBinary);

// Benchmark key prefix management cost (FastBinary skips this entirely)
static void BM_Serializer_KeyPrefixManagement_FastBinary(benchmark::State& state) {
	MockSerializableState innerState;
	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		s.AddKeyPrefix("outer");
		s.AddKeyPrefix("inner");
		innerState.Serialize(s);
		s.RemoveKeyPrefix("inner");
		s.RemoveKeyPrefix("outer");
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Serializer_KeyPrefixManagement_FastBinary);

// Benchmark deep nesting with FastBinary (prefix management is no-op)
static void BM_Serializer_DeepNesting_FastBinary(benchmark::State& state) {
	uint32_t value = 0x12345678;
	Serializer s;  // Persistent FastBinary serializer

	for (auto _ : state) {
		s.ResetForFastSave(1);
		s.AddKeyPrefix("level1");
		s.AddKeyPrefix("level2");
		s.AddKeyPrefix("level3");
		s.AddKeyPrefix("level4");
		s.AddKeyPrefix("level5");
		s.Stream(value, "value");
		s.RemoveKeyPrefix("level5");
		s.RemoveKeyPrefix("level4");
		s.RemoveKeyPrefix("level3");
		s.RemoveKeyPrefix("level2");
		s.RemoveKeyPrefix("level1");
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Serializer_DeepNesting_FastBinary);

// Benchmark persistent FastBinary (no allocation after warmup) vs fresh Binary (allocs every iter)
static void BM_Serializer_SaveLargeState_FreshBinary(benchmark::State& state) {
	MockConsoleState consoleState;
	for (size_t i = 0; i < consoleState.ram.size(); i++) {
		consoleState.ram[i] = static_cast<uint8_t>(i ^ (i >> 8));
	}

	for (auto _ : state) {
		// Fresh allocation every iteration (old run-ahead pattern)
		Serializer s(1, true, SerializeFormat::Binary);
		consoleState.Serialize(s);
		benchmark::DoNotOptimize(s);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * sizeof(MockConsoleState));
}
BENCHMARK(BM_Serializer_SaveLargeState_FreshBinary);

// =============================================================================
// Comparison Benchmarks
// =============================================================================

// Benchmark comparison: raw memcpy vs serialization for array
static void BM_Serializer_CompareRawMemcpy(benchmark::State& state) {
	std::vector<uint8_t> src(0x10000);  // 64KB
	std::vector<uint8_t> dst(0x10000);
	for (size_t i = 0; i < src.size(); i++) src[i] = static_cast<uint8_t>(i ^ (i >> 8));

	for (auto _ : state) {
		std::memcpy(dst.data(), src.data(), src.size());
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * src.size());
}
BENCHMARK(BM_Serializer_CompareRawMemcpy);

// Benchmark comparison: std::copy vs serialization for array
static void BM_Serializer_CompareStdCopy(benchmark::State& state) {
	std::vector<uint8_t> src(0x10000);  // 64KB
	std::vector<uint8_t> dst(0x10000);
	for (size_t i = 0; i < src.size(); i++) src[i] = static_cast<uint8_t>(i ^ (i >> 8));

	for (auto _ : state) {
		std::copy(src.begin(), src.end(), dst.begin());
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * src.size());
}
BENCHMARK(BM_Serializer_CompareStdCopy);

// -----------------------------------------------------------------------------
// Version Handling
// -----------------------------------------------------------------------------

// Benchmark version checking in serializer
static void BM_Serializer_VersionCheck(benchmark::State& state) {
	for (auto _ : state) {
		Serializer s(100, true, SerializeFormat::Binary);
		uint32_t version = s.GetVersion();
		bool isNewFormat = version >= 50;
		bool supportFeatureX = version >= 75;
		bool supportFeatureY = version >= 90;
		benchmark::DoNotOptimize(version);
		benchmark::DoNotOptimize(isNewFormat);
		benchmark::DoNotOptimize(supportFeatureX);
		benchmark::DoNotOptimize(supportFeatureY);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Serializer_VersionCheck);

