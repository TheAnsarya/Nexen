#include "pch.h"
#include "Shared/LightweightCdlRecorder.h"
#include "Debugger/DebugTypes.h"
#include <cstring>
#include <filesystem>
#include <fstream>

// =============================================================================
// LightweightCdlRecorder Unit Tests
// =============================================================================
// Tests for the CDL data management layer (statistics, flags, file I/O).
// RecordInstruction/RecordRead are NOT tested here — they require a live IConsole.

class LightweightCdlRecorderTest : public ::testing::Test {
protected:
	static constexpr uint32_t TestRomSize = 1024;
	static constexpr uint32_t TestCrc32 = 0xdeadbeef;
	static constexpr MemoryType TestMemType = MemoryType::NesPrgRom;
	static constexpr CpuType TestCpuType = CpuType::Nes;

	std::unique_ptr<LightweightCdlRecorder> MakeRecorder(uint32_t size = TestRomSize, uint32_t crc = TestCrc32) {
		// nullptr console — only data-management methods are safe to call
		return std::make_unique<LightweightCdlRecorder>(nullptr, TestMemType, size, TestCpuType, crc);
	}

	/// <summary>Get a temp file path for CDL file I/O tests</summary>
	std::string GetTempCdlPath() {
		auto tmp = std::filesystem::temp_directory_path() / "nexen_cdl_test.cdl";
		return tmp.string();
	}

	void TearDown() override {
		// Clean up temp files
		std::string tmpPath = GetTempCdlPath();
		if (std::filesystem::exists(tmpPath)) {
			std::filesystem::remove(tmpPath);
		}
	}
};

// =============================================================================
// Construction & Reset
// =============================================================================

TEST_F(LightweightCdlRecorderTest, Constructor_InitializesSize) {
	auto rec = MakeRecorder(2048);
	EXPECT_EQ(rec->GetSize(), 2048u);
}

TEST_F(LightweightCdlRecorderTest, Constructor_InitializesMemoryType) {
	auto rec = MakeRecorder();
	EXPECT_EQ(rec->GetMemoryType(), TestMemType);
}

TEST_F(LightweightCdlRecorderTest, Constructor_AllFlagsZero) {
	auto rec = MakeRecorder();
	for (uint32_t i = 0; i < TestRomSize; i++) {
		EXPECT_EQ(rec->GetFlags(i), CdlFlags::None);
	}
}

TEST_F(LightweightCdlRecorderTest, Reset_ClearsAllFlags) {
	auto rec = MakeRecorder();
	// Set some flags manually
	uint8_t data[TestRomSize] = {};
	memset(data, CdlFlags::Code | CdlFlags::Data, TestRomSize);
	rec->SetCdlData(data, TestRomSize);

	rec->Reset();

	for (uint32_t i = 0; i < TestRomSize; i++) {
		EXPECT_EQ(rec->GetFlags(i), CdlFlags::None);
	}
}

TEST_F(LightweightCdlRecorderTest, Constructor_RawDataNotNull) {
	auto rec = MakeRecorder();
	EXPECT_NE(rec->GetRawData(), nullptr);
}

// =============================================================================
// GetFlags / SetCdlData
// =============================================================================

TEST_F(LightweightCdlRecorderTest, GetFlags_ValidAddress) {
	auto rec = MakeRecorder();
	uint8_t* raw = rec->GetRawData();
	raw[0] = CdlFlags::Code;
	raw[100] = CdlFlags::Data;
	raw[TestRomSize - 1] = CdlFlags::JumpTarget;

	EXPECT_EQ(rec->GetFlags(0), CdlFlags::Code);
	EXPECT_EQ(rec->GetFlags(100), CdlFlags::Data);
	EXPECT_EQ(rec->GetFlags(TestRomSize - 1), CdlFlags::JumpTarget);
}

TEST_F(LightweightCdlRecorderTest, GetFlags_OutOfBounds_ReturnsZero) {
	auto rec = MakeRecorder();
	EXPECT_EQ(rec->GetFlags(TestRomSize), 0);
	EXPECT_EQ(rec->GetFlags(TestRomSize + 1000), 0);
	EXPECT_EQ(rec->GetFlags(UINT32_MAX), 0);
}

TEST_F(LightweightCdlRecorderTest, SetCdlData_OverwritesExisting) {
	auto rec = MakeRecorder(8);
	uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	rec->SetCdlData(data, 8);

	for (int i = 0; i < 8; i++) {
		EXPECT_EQ(rec->GetFlags(i), data[i]);
	}
}

TEST_F(LightweightCdlRecorderTest, SetCdlData_TruncatesIfTooLarge) {
	auto rec = MakeRecorder(4);
	uint8_t data[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	// Should only copy 4 bytes (the recorder's size)
	rec->SetCdlData(data, 4);
	EXPECT_EQ(rec->GetFlags(0), 0x01);
	EXPECT_EQ(rec->GetFlags(3), 0x04);
}

// =============================================================================
// GetCdlData
// =============================================================================

TEST_F(LightweightCdlRecorderTest, GetCdlData_FullRange) {
	auto rec = MakeRecorder(8);
	uint8_t setData[8] = {0x01, 0x03, 0x07, 0x0f, 0x00, 0x01, 0x02, 0x04};
	rec->SetCdlData(setData, 8);

	uint8_t getData[8] = {};
	rec->GetCdlData(0, 8, getData);
	EXPECT_EQ(memcmp(setData, getData, 8), 0);
}

TEST_F(LightweightCdlRecorderTest, GetCdlData_PartialRange) {
	auto rec = MakeRecorder(8);
	uint8_t setData[8] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
	rec->SetCdlData(setData, 8);

	uint8_t getData[4] = {};
	rec->GetCdlData(2, 4, getData);
	EXPECT_EQ(getData[0], 0x30);
	EXPECT_EQ(getData[1], 0x40);
	EXPECT_EQ(getData[2], 0x50);
	EXPECT_EQ(getData[3], 0x60);
}

TEST_F(LightweightCdlRecorderTest, GetCdlData_SetCdlData_Roundtrip) {
	auto rec = MakeRecorder(256);
	uint8_t original[256];
	for (int i = 0; i < 256; i++) {
		original[i] = static_cast<uint8_t>(i & 0x0f); // CDL flag patterns
	}

	rec->SetCdlData(original, 256);

	uint8_t readback[256] = {};
	rec->GetCdlData(0, 256, readback);
	EXPECT_EQ(memcmp(original, readback, 256), 0);
}

// =============================================================================
// GetStatistics
// =============================================================================

TEST_F(LightweightCdlRecorderTest, Statistics_EmptyRom) {
	auto rec = MakeRecorder(100);
	auto stats = rec->GetStatistics();

	EXPECT_EQ(stats.CodeBytes, 0u);
	EXPECT_EQ(stats.DataBytes, 0u);
	EXPECT_EQ(stats.TotalBytes, 100u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_AllCode) {
	auto rec = MakeRecorder(10);
	uint8_t data[10];
	memset(data, CdlFlags::Code, 10);
	rec->SetCdlData(data, 10);

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 10u);
	EXPECT_EQ(stats.DataBytes, 0u);
	EXPECT_EQ(stats.TotalBytes, 10u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_AllData) {
	auto rec = MakeRecorder(10);
	uint8_t data[10];
	memset(data, CdlFlags::Data, 10);
	rec->SetCdlData(data, 10);

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 0u);
	EXPECT_EQ(stats.DataBytes, 10u);
	EXPECT_EQ(stats.TotalBytes, 10u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_MixedCodeAndData) {
	auto rec = MakeRecorder(10);
	uint8_t data[10] = {};
	data[0] = CdlFlags::Code;                    // Code only
	data[1] = CdlFlags::Code;                    // Code only
	data[2] = CdlFlags::Data;                    // Data only
	data[3] = CdlFlags::Data;                    // Data only
	data[4] = CdlFlags::Data;                    // Data only
	data[5] = CdlFlags::Code | CdlFlags::Data;   // Both
	data[6] = CdlFlags::Code | CdlFlags::Data;   // Both
	// 7-9 remain CdlFlags::None

	rec->SetCdlData(data, 10);
	auto stats = rec->GetStatistics();

	// CodeBytes: 0,1 (Code), 5,6 (Both) = 4
	EXPECT_EQ(stats.CodeBytes, 4u);
	// DataBytes: 2,3,4 (Data only) — bytes marked Both are NOT counted as data
	EXPECT_EQ(stats.DataBytes, 3u);
	EXPECT_EQ(stats.TotalBytes, 10u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_BothFlagsDeduplication) {
	// Verify that bytes with both Code + Data flags are counted in Code but not Data
	auto rec = MakeRecorder(4);
	uint8_t data[4] = {
		CdlFlags::Code | CdlFlags::Data,
		CdlFlags::Code | CdlFlags::Data,
		CdlFlags::Code | CdlFlags::Data,
		CdlFlags::Code | CdlFlags::Data,
	};
	rec->SetCdlData(data, 4);

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 4u);
	EXPECT_EQ(stats.DataBytes, 0u);
	EXPECT_EQ(stats.TotalBytes, 4u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_JumpTargetNotCounted) {
	// JumpTarget alone does not count as Code or Data
	auto rec = MakeRecorder(4);
	uint8_t data[4] = {
		CdlFlags::JumpTarget,
		CdlFlags::SubEntryPoint,
		CdlFlags::JumpTarget | CdlFlags::SubEntryPoint,
		CdlFlags::None,
	};
	rec->SetCdlData(data, 4);

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 0u);
	EXPECT_EQ(stats.DataBytes, 0u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_LargeRom) {
	const uint32_t size = 65536; // 64KB
	auto rec = MakeRecorder(size);
	uint8_t* raw = rec->GetRawData();

	// First 32KB: code, next 16KB: data, next 8KB: both, rest: empty
	memset(raw, CdlFlags::Code, 32768);
	memset(raw + 32768, CdlFlags::Data, 16384);
	memset(raw + 49152, CdlFlags::Code | CdlFlags::Data, 8192);

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 32768u + 8192u);
	EXPECT_EQ(stats.DataBytes, 16384u); // Both bytes NOT counted as data
	EXPECT_EQ(stats.TotalBytes, size);
}

// =============================================================================
// CDL File I/O — Save & Load Roundtrip
// =============================================================================

TEST_F(LightweightCdlRecorderTest, SaveLoad_Roundtrip) {
	auto rec = MakeRecorder(64, 0x12345678);
	uint8_t testData[64];
	for (int i = 0; i < 64; i++) {
		testData[i] = static_cast<uint8_t>(i % 5); // 0,1,2,3,4,0,1,...
	}
	rec->SetCdlData(testData, 64);

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	// Load into a fresh recorder with same CRC32
	auto rec2 = MakeRecorder(64, 0x12345678);
	ASSERT_TRUE(rec2->LoadCdlFile(path));

	uint8_t readback[64] = {};
	rec2->GetCdlData(0, 64, readback);
	EXPECT_EQ(memcmp(testData, readback, 64), 0);
}

TEST_F(LightweightCdlRecorderTest, SaveLoad_FileHeader) {
	auto rec = MakeRecorder(16, 0xaabbccdd);
	uint8_t data[16];
	memset(data, CdlFlags::Code, 16);
	rec->SetCdlData(data, 16);

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	// Read raw file to verify header
	std::ifstream file(path, std::ios::binary);
	ASSERT_TRUE(file.is_open());

	// Header: "CDLv2" (5 bytes) + CRC32 LE (4 bytes) = 9 bytes
	char magic[5] = {};
	file.read(magic, 5);
	EXPECT_EQ(std::string(magic, 5), "CDLv2");

	uint8_t crcBytes[4] = {};
	file.read(reinterpret_cast<char*>(crcBytes), 4);
	uint32_t storedCrc = crcBytes[0] | (crcBytes[1] << 8) | (crcBytes[2] << 16) | (crcBytes[3] << 24);
	EXPECT_EQ(storedCrc, 0xaabbccddu);

	// Read CDL data payload
	uint8_t payload[16] = {};
	file.read(reinterpret_cast<char*>(payload), 16);
	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(payload[i], CdlFlags::Code);
	}
}

TEST_F(LightweightCdlRecorderTest, SaveLoad_CrcMismatch_DoesNotLoad) {
	auto rec = MakeRecorder(16, 0x11111111);
	uint8_t data[16];
	memset(data, CdlFlags::Data, 16);
	rec->SetCdlData(data, 16);

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	// Load with different CRC — should load file but data should remain zero (CRC mismatch)
	auto rec2 = MakeRecorder(16, 0x22222222);
	(void)rec2->LoadCdlFile(path);

	// CRC mismatch means data is not copied — all flags should be None (from Reset)
	for (uint32_t i = 0; i < 16; i++) {
		EXPECT_EQ(rec2->GetFlags(i), CdlFlags::None);
	}
}

TEST_F(LightweightCdlRecorderTest, LoadCdlFile_NonexistentFile_ReturnsFalse) {
	auto rec = MakeRecorder();
	EXPECT_FALSE(rec->LoadCdlFile("C:/nonexistent/path/fake.cdl"));
}

TEST_F(LightweightCdlRecorderTest, SaveLoad_StatisticsPreserved) {
	auto rec = MakeRecorder(100, 0x55555555);
	uint8_t* raw = rec->GetRawData();
	memset(raw, CdlFlags::Code, 30);
	memset(raw + 30, CdlFlags::Data, 20);

	auto statsBefore = rec->GetStatistics();

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	auto rec2 = MakeRecorder(100, 0x55555555);
	ASSERT_TRUE(rec2->LoadCdlFile(path));

	auto statsAfter = rec2->GetStatistics();
	EXPECT_EQ(statsAfter.CodeBytes, statsBefore.CodeBytes);
	EXPECT_EQ(statsAfter.DataBytes, statsBefore.DataBytes);
	EXPECT_EQ(statsAfter.TotalBytes, statsBefore.TotalBytes);
}

// =============================================================================
// CDL Flags Combinations
// =============================================================================

TEST_F(LightweightCdlRecorderTest, Flags_AllCombinations) {
	// Verify all 16 possible flag combinations (4 bits: Code, Data, JumpTarget, SubEntryPoint)
	auto rec = MakeRecorder(16);
	uint8_t data[16];
	for (int i = 0; i < 16; i++) {
		data[i] = static_cast<uint8_t>(i);
	}
	rec->SetCdlData(data, 16);

	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(rec->GetFlags(i), static_cast<uint8_t>(i));
	}
}

TEST_F(LightweightCdlRecorderTest, Flags_OrSemantics) {
	// CDL accumulates flags via OR — verify manually
	auto rec = MakeRecorder(4);
	uint8_t* raw = rec->GetRawData();

	raw[0] |= CdlFlags::Code;
	EXPECT_EQ(raw[0], CdlFlags::Code);

	raw[0] |= CdlFlags::Data;
	EXPECT_EQ(raw[0], CdlFlags::Code | CdlFlags::Data);

	raw[0] |= CdlFlags::JumpTarget;
	EXPECT_EQ(raw[0], CdlFlags::Code | CdlFlags::Data | CdlFlags::JumpTarget);

	raw[0] |= CdlFlags::SubEntryPoint;
	EXPECT_EQ(raw[0], 0x0f);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(LightweightCdlRecorderTest, MinimalSize) {
	auto rec = MakeRecorder(1);
	EXPECT_EQ(rec->GetSize(), 1u);
	EXPECT_EQ(rec->GetFlags(0), CdlFlags::None);

	uint8_t* raw = rec->GetRawData();
	raw[0] = CdlFlags::Code;
	EXPECT_EQ(rec->GetFlags(0), CdlFlags::Code);
}

TEST_F(LightweightCdlRecorderTest, SaveLoad_MinimalSize) {
	auto rec = MakeRecorder(1, 0x42);
	uint8_t* raw = rec->GetRawData();
	raw[0] = CdlFlags::Data | CdlFlags::JumpTarget;

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	auto rec2 = MakeRecorder(1, 0x42);
	ASSERT_TRUE(rec2->LoadCdlFile(path));
	EXPECT_EQ(rec2->GetFlags(0), CdlFlags::Data | CdlFlags::JumpTarget);
}

TEST_F(LightweightCdlRecorderTest, SaveLoad_FileSize) {
	const uint32_t romSize = 128;
	auto rec = MakeRecorder(romSize, 0x99);
	uint8_t* raw = rec->GetRawData();
	memset(raw, 0xff, romSize);

	std::string path = GetTempCdlPath();
	ASSERT_TRUE(rec->SaveCdlFile(path));

	// File size should be header (9 bytes) + ROM size
	auto fileSize = std::filesystem::file_size(path);
	EXPECT_EQ(fileSize, 9u + romSize);
}

TEST_F(LightweightCdlRecorderTest, Statistics_SingleByte) {
	auto rec = MakeRecorder(1);
	uint8_t* raw = rec->GetRawData();
	raw[0] = CdlFlags::Code;

	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 1u);
	EXPECT_EQ(stats.DataBytes, 0u);
	EXPECT_EQ(stats.TotalBytes, 1u);
}

TEST_F(LightweightCdlRecorderTest, Statistics_AfterReset) {
	auto rec = MakeRecorder(100);
	uint8_t* raw = rec->GetRawData();
	memset(raw, CdlFlags::Code, 100);

	rec->Reset();
	auto stats = rec->GetStatistics();
	EXPECT_EQ(stats.CodeBytes, 0u);
	EXPECT_EQ(stats.DataBytes, 0u);
	EXPECT_EQ(stats.TotalBytes, 100u);
}
