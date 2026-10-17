#include "pch.h"
#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <cstring>
#include <sstream>
#include "Utilities/Serializer.h"

// =============================================================================
// Serializer Unit Tests
// =============================================================================
// Tests for Nexen's key-value serialization system used for save states and rewind.

// -----------------------------------------------------------------------------
// Test Fixture
// -----------------------------------------------------------------------------

class SerializerTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Fresh state for each test
	}

	void TearDown() override {
	}
};

// =============================================================================
// Mock Serializable Classes
// =============================================================================

/// <summary>Simple mock state for testing basic serialization</summary>
class MockCpuState : public ISerializable {
public:
	uint32_t pc = 0;
	uint16_t sp = 0;
	uint8_t a = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t status = 0;
	bool irqPending = false;
	int32_t cycles = 0;

	void Serialize(Serializer& s) override {
		SV(pc);
		SV(sp);
		SV(a);
		SV(x);
		SV(y);
		SV(status);
		SV(irqPending);
		SV(cycles);
	}

	bool operator==(const MockCpuState& other) const {
		return pc == other.pc && sp == other.sp && a == other.a &&
		       x == other.x && y == other.y && status == other.status &&
		       irqPending == other.irqPending && cycles == other.cycles;
	}
};

/// <summary>Mock state with arrays for testing array serialization</summary>
class MockPpuState : public ISerializable {
public:
	std::array<uint8_t, 2048> vram = {};   // 2KB
	std::array<uint8_t, 256> oam = {};     // 256 bytes
	std::array<uint8_t, 32> palette = {};  // 32 bytes
	uint16_t scanline = 0;
	uint16_t cycle = 0;

	void Serialize(Serializer& s) override {
		SVArray(vram.data(), static_cast<uint32_t>(vram.size()));
		SVArray(oam.data(), static_cast<uint32_t>(oam.size()));
		SVArray(palette.data(), static_cast<uint32_t>(palette.size()));
		SV(scanline);
		SV(cycle);
	}

	bool operator==(const MockPpuState& other) const {
		return vram == other.vram && oam == other.oam && palette == other.palette &&
		       scanline == other.scanline && cycle == other.cycle;
	}
};

/// <summary>Mock state with nested objects</summary>
class MockConsoleState : public ISerializable {
public:
	MockCpuState cpu;
	MockPpuState ppu;
	std::array<uint8_t, 2048> ram = {};  // 2KB RAM
	uint32_t frameCount = 0;

	void Serialize(Serializer& s) override {
		SV(cpu);
		SV(ppu);
		SVArray(ram.data(), static_cast<uint32_t>(ram.size()));
		SV(frameCount);
	}

	bool operator==(const MockConsoleState& other) const {
		return cpu == other.cpu && ppu == other.ppu && ram == other.ram &&
		       frameCount == other.frameCount;
	}
};

// =============================================================================
// Binary Format Tests
// =============================================================================

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadPrimitives) {
	// Test primitive types round-trip
	uint8_t u8 = 0xab;
	uint16_t u16 = 0x1234;
	uint32_t u32 = 0xdeadbeef;
	int8_t i8 = -42;
	int16_t i16 = -1000;
	int32_t i32 = -123456;
	bool b = true;

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(u8, "u8");
	saver.Stream(u16, "u16");
	saver.Stream(u32, "u32");
	saver.Stream(i8, "i8");
	saver.Stream(i16, "i16");
	saver.Stream(i32, "i32");
	saver.Stream(b, "b");

	// Get serialized data via stringstream
	std::stringstream ss;
	saver.SaveTo(ss);
	ASSERT_GT(ss.str().size(), 0u);

	// Load into new variables
	uint8_t u8_loaded = 0;
	uint16_t u16_loaded = 0;
	uint32_t u32_loaded = 0;
	int8_t i8_loaded = 0;
	int16_t i16_loaded = 0;
	int32_t i32_loaded = 0;
	bool b_loaded = false;

	ss.seekg(0);  // Reset to beginning
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(u8_loaded, "u8");
	loader.Stream(u16_loaded, "u16");
	loader.Stream(u32_loaded, "u32");
	loader.Stream(i8_loaded, "i8");
	loader.Stream(i16_loaded, "i16");
	loader.Stream(i32_loaded, "i32");
	loader.Stream(b_loaded, "b");

	EXPECT_EQ(u8, u8_loaded);
	EXPECT_EQ(u16, u16_loaded);
	EXPECT_EQ(u32, u32_loaded);
	EXPECT_EQ(i8, i8_loaded);
	EXPECT_EQ(i16, i16_loaded);
	EXPECT_EQ(i32, i32_loaded);
	EXPECT_EQ(b, b_loaded);
}

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadArray) {
	std::array<uint8_t, 16> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	saver.StreamArray(original.data(), 16, "data");

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load
	std::array<uint8_t, 16> loaded = {};
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.StreamArray(loaded.data(), 16, "data");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadLargeArray) {
	// Test with larger array (64KB)
	std::vector<uint8_t> original(65536);
	for (size_t i = 0; i < original.size(); i++) {
		original[i] = static_cast<uint8_t>(i & 0xff);
	}

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	saver.StreamArray(original.data(), static_cast<uint32_t>(original.size()), "bigData");

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load
	std::vector<uint8_t> loaded(65536);
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.StreamArray(loaded.data(), static_cast<uint32_t>(loaded.size()), "bigData");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadMockCpu) {
	MockCpuState original;
	original.pc = 0xc000;
	original.sp = 0x01fd;
	original.a = 0x42;
	original.x = 0x10;
	original.y = 0x20;
	original.status = 0x24;
	original.irqPending = true;
	original.cycles = 12345678;

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load
	MockCpuState loaded;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loaded.Serialize(loader);

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadMockPpu) {
	MockPpuState original;
	// Fill VRAM with pattern
	for (size_t i = 0; i < original.vram.size(); i++) {
		original.vram[i] = static_cast<uint8_t>(i & 0xff);
	}
	// Fill OAM
	for (size_t i = 0; i < original.oam.size(); i++) {
		original.oam[i] = static_cast<uint8_t>(i * 2);
	}
	// Fill palette
	for (size_t i = 0; i < original.palette.size(); i++) {
		original.palette[i] = static_cast<uint8_t>(i * 8);
	}
	original.scanline = 240;
	original.cycle = 280;

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load
	MockPpuState loaded;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loaded.Serialize(loader);

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, BinaryFormat_SaveAndLoadNestedState) {
	MockConsoleState original;
	original.cpu.pc = 0x8000;
	original.cpu.a = 0x55;
	original.ppu.scanline = 100;
	original.frameCount = 999999;

	// Fill RAM
	for (size_t i = 0; i < original.ram.size(); i++) {
		original.ram[i] = static_cast<uint8_t>(i ^ 0xa5);
	}

	// Save
	Serializer saver(1, true, SerializeFormat::Binary);
	original.Serialize(saver);

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load
	MockConsoleState loaded;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loaded.Serialize(loader);

	EXPECT_EQ(original, loaded);
}

// =============================================================================
// Versioning Tests
// =============================================================================

TEST_F(SerializerTest, Version_GetVersion) {
	Serializer s(5, true, SerializeFormat::Binary);
	EXPECT_EQ(5u, s.GetVersion());
}

TEST_F(SerializerTest, Version_Roundtrip) {
	uint32_t value1 = 100;
	uint32_t value2 = 200;

	// Save with version 2
	Serializer saver(2, true, SerializeFormat::Binary);
	saver.Stream(value1, "v1");
	saver.Stream(value2, "v2");

	std::stringstream ss;
	saver.SaveTo(ss);

	// Load and check version
	ss.seekg(0);
	Serializer loader(2, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	EXPECT_EQ(2u, loader.GetVersion());
}

// =============================================================================
// Key/Prefix Tests
// =============================================================================

TEST_F(SerializerTest, Prefix_PushAndPop) {
	Serializer s(1, true, SerializeFormat::Binary);

	uint8_t val1 = 1;
	uint8_t val2 = 2;

	s.Stream(val1, "root");

	s.PushNamePrefix("cpu");
	s.Stream(val2, "a");
	s.PopNamePrefix();

	// Should be back at root level
	uint8_t val3 = 3;
	s.Stream(val3, "after");

	// Verify no crash (successful push/pop)
	std::stringstream ss;
	s.SaveTo(ss);
	EXPECT_GT(ss.str().size(), 0u);
}

TEST_F(SerializerTest, Prefix_NestedPrefixes) {
	Serializer s(1, true, SerializeFormat::Binary);

	uint8_t val = 42;

	s.PushNamePrefix("level1");
	s.PushNamePrefix("level2");
	s.PushNamePrefix("level3");
	s.Stream(val, "value");
	s.PopNamePrefix();
	s.PopNamePrefix();
	s.PopNamePrefix();

	std::stringstream ss;
	s.SaveTo(ss);
	EXPECT_GT(ss.str().size(), 0u);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(SerializerTest, EdgeCase_MaxUint32) {
	uint32_t original = UINT32_MAX;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "max");

	std::stringstream ss;
	saver.SaveTo(ss);

	uint32_t loaded = 0;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "max");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, EdgeCase_MinInt32) {
	int32_t original = INT32_MIN;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "min");

	std::stringstream ss;
	saver.SaveTo(ss);

	int32_t loaded = 0;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "min");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, EdgeCase_AllBoolValues) {
	bool trueVal = true;
	bool falseVal = false;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(trueVal, "t");
	saver.Stream(falseVal, "f");

	std::stringstream ss;
	saver.SaveTo(ss);

	bool loadedTrue = false;
	bool loadedFalse = true;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loadedTrue, "t");
	loader.Stream(loadedFalse, "f");

	EXPECT_TRUE(loadedTrue);
	EXPECT_FALSE(loadedFalse);
}

// =============================================================================
// Data Integrity Tests
// =============================================================================

TEST_F(SerializerTest, Integrity_MultipleRoundtrips) {
	MockCpuState state;
	state.pc = 0xabcd;
	state.a = 0x12;

	// Do multiple save/load cycles
	for (int i = 0; i < 5; i++) {
		Serializer saver(1, true, SerializeFormat::Binary);
		state.Serialize(saver);

		std::stringstream ss;
		saver.SaveTo(ss);

		MockCpuState loaded;
		ss.seekg(0);
		Serializer loader(1, false, SerializeFormat::Binary);
		loader.LoadFrom(ss);
		loaded.Serialize(loader);

		EXPECT_EQ(state, loaded) << "Failed on round-trip " << i;
		state = loaded;  // Use loaded for next iteration
	}
}

TEST_F(SerializerTest, Integrity_DifferentDataSameKeys) {
	// Save two different states with same structure
	MockCpuState state1;
	state1.pc = 0x1111;
	state1.a = 0xaa;

	MockCpuState state2;
	state2.pc = 0x2222;
	state2.a = 0xbb;

	// Save state1
	Serializer saver1(1, true, SerializeFormat::Binary);
	state1.Serialize(saver1);
	std::stringstream ss1;
	saver1.SaveTo(ss1);

	// Save state2
	Serializer saver2(1, true, SerializeFormat::Binary);
	state2.Serialize(saver2);
	std::stringstream ss2;
	saver2.SaveTo(ss2);

	// Load and verify they're different
	MockCpuState loaded1, loaded2;

	ss1.seekg(0);
	Serializer loader1(1, false, SerializeFormat::Binary);
	loader1.LoadFrom(ss1);
	loaded1.Serialize(loader1);

	ss2.seekg(0);
	Serializer loader2(1, false, SerializeFormat::Binary);
	loader2.LoadFrom(ss2);
	loaded2.Serialize(loader2);

	EXPECT_EQ(state1, loaded1);
	EXPECT_EQ(state2, loaded2);
	EXPECT_NE(loaded1.pc, loaded2.pc);
}

// =============================================================================
// IsSaving Tests
// =============================================================================

TEST_F(SerializerTest, IsSaving_TrueWhenSaving) {
	Serializer s(1, true, SerializeFormat::Binary);
	EXPECT_TRUE(s.IsSaving());
}

TEST_F(SerializerTest, IsSaving_FalseWhenLoading) {
	Serializer s(1, false, SerializeFormat::Binary);
	EXPECT_FALSE(s.IsSaving());
}

// =============================================================================
// 64-bit Types
// =============================================================================

TEST_F(SerializerTest, Types_Uint64Roundtrip) {
	uint64_t original = 0x123456789abcdef0ULL;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "u64");

	std::stringstream ss;
	saver.SaveTo(ss);

	uint64_t loaded = 0;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "u64");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, Types_Int64Roundtrip) {
	int64_t original = -0x123456789abcdefLL;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "i64");

	std::stringstream ss;
	saver.SaveTo(ss);

	int64_t loaded = 0;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "i64");

	EXPECT_EQ(original, loaded);
}

// =============================================================================
// Floating Point Tests
// =============================================================================

TEST_F(SerializerTest, Types_FloatRoundtrip) {
	float original = 3.14159265f;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "f32");

	std::stringstream ss;
	saver.SaveTo(ss);

	float loaded = 0.0f;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "f32");

	EXPECT_FLOAT_EQ(original, loaded);
}

TEST_F(SerializerTest, Types_DoubleRoundtrip) {
	double original = 2.718281828459045;

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.Stream(original, "f64");

	std::stringstream ss;
	saver.SaveTo(ss);

	double loaded = 0.0;
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.Stream(loaded, "f64");

	EXPECT_DOUBLE_EQ(original, loaded);
}

// =============================================================================
// Array of Different Types
// =============================================================================

TEST_F(SerializerTest, Array_Uint16Array) {
	std::array<uint16_t, 8> original = {0x0001, 0x0102, 0x0203, 0x0304, 0x0405, 0x0506, 0x0607, 0x0708};

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.StreamArray(original.data(), 8, "u16arr");

	std::stringstream ss;
	saver.SaveTo(ss);

	std::array<uint16_t, 8> loaded = {};
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.StreamArray(loaded.data(), 8, "u16arr");

	EXPECT_EQ(original, loaded);
}

TEST_F(SerializerTest, Array_Uint32Array) {
	std::array<uint32_t, 4> original = {0xdeadbeef, 0xcafebabe, 0x12345678, 0x9abcdef0};

	Serializer saver(1, true, SerializeFormat::Binary);
	saver.StreamArray(original.data(), 4, "u32arr");

	std::stringstream ss;
	saver.SaveTo(ss);

	std::array<uint32_t, 4> loaded = {};
	ss.seekg(0);
	Serializer loader(1, false, SerializeFormat::Binary);
	loader.LoadFrom(ss);
	loader.StreamArray(loaded.data(), 4, "u32arr");

	EXPECT_EQ(original, loaded);
}
