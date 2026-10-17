#include "pch.h"
#include "Lynx/AtariLynxFormat.h"
#include "Utilities/CRC32.h"

/// <summary>
/// Tests for the AtariLynxFormat reader/writer (.atari-lynx container format).
///
/// Covers:
///   - Format detection (IsAtariLynxFormat, IsLnxFormat)
///   - Header parsing and validation
///   - Save/Load roundtrip (with and without metadata)
///   - LNX → atari-lynx conversion
///   - Raw → atari-lynx conversion
///   - atari-lynx → LNX back-conversion
///   - Metadata field preservation
///   - Error handling (bad magic, truncated files, bad versions)
///
/// See docs/ATARI-LYNX-FORMAT.md for the format specification.
/// </summary>
class AtariLynxFormatTest : public ::testing::Test {
protected:
	/// <summary>Create a fake ROM of the specified size filled with a pattern</summary>
	std::vector<uint8_t> CreateFakeRom(uint32_t size, uint8_t seed = 0x42) {
		std::vector<uint8_t> rom(size);
		for (uint32_t i = 0; i < size; i++) {
			rom[i] = static_cast<uint8_t>((seed + i) & 0xff);
		}
		return rom;
	}

	/// <summary>Create a minimal valid LNX file (64-byte header + ROM data)</summary>
	std::vector<uint8_t> CreateLnxFile(const std::vector<uint8_t>& rom,
		const char* name = "TestGame",
		const char* manufacturer = "TestMfg",
		uint16_t bank0Pages = 0x100,
		uint16_t bank1Pages = 0,
		uint8_t rotation = 0,
		uint8_t eeprom = 0) {
		std::vector<uint8_t> file(64, 0);
		// Magic
		file[0] = 'L'; file[1] = 'Y'; file[2] = 'N'; file[3] = 'X';
		// Bank sizes
		file[4] = bank0Pages & 0xff;
		file[5] = (bank0Pages >> 8) & 0xff;
		file[6] = bank1Pages & 0xff;
		file[7] = (bank1Pages >> 8) & 0xff;
		// Version
		file[8] = 1; file[9] = 0;
		// Name
		size_t nameLen = std::min(strlen(name), size_t{32});
		memcpy(&file[10], name, nameLen);
		// Manufacturer
		size_t mfgLen = std::min(strlen(manufacturer), size_t{16});
		memcpy(&file[42], manufacturer, mfgLen);
		// Rotation
		file[58] = rotation;
		// EEPROM
		file[60] = eeprom;
		// Append ROM
		file.insert(file.end(), rom.begin(), rom.end());
		return file;
	}

	/// <summary>Create sample metadata for testing</summary>
	AtariLynxMetadata CreateSampleMetadata() {
		AtariLynxMetadata meta;
		meta.CartName = "California Games";
		meta.Manufacturer = "Epyx";
		meta.Rotation = 0;
		meta.Bank0PageSize = 0x100;
		meta.Bank1PageSize = 0;
		meta.EepromType = 0;
		meta.EepromFlags = 0;
		meta.HardwareModel = 2;
		meta.YearOfRelease = 1989;
		meta.PlayerCount = 4;
		meta.Handedness = 0;
		meta.ComLynxSupport = 1;
		meta.CountryCode = 840;
		meta.Language = "en";
		return meta;
	}
};

//=============================================================================
// Format Detection
//=============================================================================

TEST_F(AtariLynxFormatTest, IsAtariLynxFormat_ValidMagic) {
	std::vector<uint8_t> data(64, 0);
	memcpy(data.data(), AtariLynxFormatConstants::Magic, 8);
	EXPECT_TRUE(AtariLynxFormat::IsAtariLynxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsAtariLynxFormat_InvalidMagic) {
	std::vector<uint8_t> data(64, 0);
	data[0] = 'N'; data[1] = 'O'; data[2] = 'P'; data[3] = 'E';
	EXPECT_FALSE(AtariLynxFormat::IsAtariLynxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsAtariLynxFormat_TooSmall) {
	std::vector<uint8_t> data(16, 0);
	memcpy(data.data(), AtariLynxFormatConstants::Magic, 8);
	EXPECT_FALSE(AtariLynxFormat::IsAtariLynxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsAtariLynxFormat_LnxMagicDoesNotMatch) {
	std::vector<uint8_t> data(64, 0);
	data[0] = 'L'; data[1] = 'Y'; data[2] = 'N'; data[3] = 'X';
	// "LYNX" is NOT "LYNXROM\0" — should not match
	EXPECT_FALSE(AtariLynxFormat::IsAtariLynxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsLnxFormat_ValidMagic) {
	std::vector<uint8_t> data(64, 0);
	data[0] = 'L'; data[1] = 'Y'; data[2] = 'N'; data[3] = 'X';
	EXPECT_TRUE(AtariLynxFormat::IsLnxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsLnxFormat_InvalidMagic) {
	std::vector<uint8_t> data(64, 0);
	EXPECT_FALSE(AtariLynxFormat::IsLnxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsLnxFormat_TooSmall) {
	std::vector<uint8_t> data(32, 0);
	data[0] = 'L'; data[1] = 'Y'; data[2] = 'N'; data[3] = 'X';
	EXPECT_FALSE(AtariLynxFormat::IsLnxFormat(data.data(), data.size()));
}

TEST_F(AtariLynxFormatTest, IsLnxFormat_AtariLynxMagicDoesNotMatch) {
	std::vector<uint8_t> data(64, 0);
	memcpy(data.data(), AtariLynxFormatConstants::Magic, 8);
	// "LYNXROM\0" — byte [3] is 'X' so this WOULD match LNX magic
	// This is expected — IsLnxFormat just checks bytes 0-3 = "LYNX"
	// The caller should check IsAtariLynxFormat first
	EXPECT_TRUE(AtariLynxFormat::IsLnxFormat(data.data(), data.size()));
}

//=============================================================================
// Header Constants
//=============================================================================

TEST_F(AtariLynxFormatTest, Header_Size_32Bytes) {
	EXPECT_EQ(sizeof(AtariLynxHeader), 32u);
}

TEST_F(AtariLynxFormatTest, Magic_Is_LYNXROM_Null) {
	EXPECT_EQ(AtariLynxFormatConstants::Magic[0], 'L');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[1], 'Y');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[2], 'N');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[3], 'X');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[4], 'R');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[5], 'O');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[6], 'M');
	EXPECT_EQ(AtariLynxFormatConstants::Magic[7], '\0');
}

TEST_F(AtariLynxFormatTest, Version_Is_0_1_0) {
	EXPECT_EQ(AtariLynxFormatConstants::VersionMajor, 0);
	EXPECT_EQ(AtariLynxFormatConstants::VersionMinor, 1);
	EXPECT_EQ(AtariLynxFormatConstants::VersionPatch, 0);
}

//=============================================================================
// Save + Load Roundtrip (with metadata)
//=============================================================================

TEST_F(AtariLynxFormatTest, Roundtrip_WithMetadata_PreservesRom) {
	auto rom = CreateFakeRom(65536);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	ASSERT_EQ(loaded.RomData.size(), rom.size());
	EXPECT_EQ(loaded.RomData, rom);
}

TEST_F(AtariLynxFormatTest, Roundtrip_WithMetadata_PreservesCartName) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	ASSERT_TRUE(loaded.HasMetadata);
	EXPECT_EQ(loaded.Metadata.CartName, "California Games");
}

TEST_F(AtariLynxFormatTest, Roundtrip_WithMetadata_PreservesManufacturer) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_EQ(loaded.Metadata.Manufacturer, "Epyx");
}

TEST_F(AtariLynxFormatTest, Roundtrip_WithMetadata_PreservesAllFields) {
	auto rom = CreateFakeRom(2048);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	ASSERT_TRUE(loaded.HasMetadata);
	EXPECT_EQ(loaded.Metadata.CartName, meta.CartName);
	EXPECT_EQ(loaded.Metadata.Manufacturer, meta.Manufacturer);
	EXPECT_EQ(loaded.Metadata.Rotation, meta.Rotation);
	EXPECT_EQ(loaded.Metadata.Bank0PageSize, meta.Bank0PageSize);
	EXPECT_EQ(loaded.Metadata.Bank1PageSize, meta.Bank1PageSize);
	EXPECT_EQ(loaded.Metadata.EepromType, meta.EepromType);
	EXPECT_EQ(loaded.Metadata.EepromFlags, meta.EepromFlags);
	EXPECT_EQ(loaded.Metadata.HardwareModel, meta.HardwareModel);
	EXPECT_EQ(loaded.Metadata.YearOfRelease, meta.YearOfRelease);
	EXPECT_EQ(loaded.Metadata.PlayerCount, meta.PlayerCount);
	EXPECT_EQ(loaded.Metadata.Handedness, meta.Handedness);
	EXPECT_EQ(loaded.Metadata.ComLynxSupport, meta.ComLynxSupport);
	EXPECT_EQ(loaded.Metadata.CountryCode, meta.CountryCode);
	EXPECT_EQ(loaded.Metadata.Language, meta.Language);
}

TEST_F(AtariLynxFormatTest, Roundtrip_WithMetadata_CRC32Matches) {
	auto rom = CreateFakeRom(4096);
	auto meta = CreateSampleMetadata();

	uint32_t expectedCrc = CRC32::GetCRC(rom.data(), static_cast<std::streamoff>(rom.size()));
	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_EQ(loaded.RomCrc32, expectedCrc);
}

//=============================================================================
// Save + Load Roundtrip (header only, no metadata)
//=============================================================================

TEST_F(AtariLynxFormatTest, Roundtrip_HeaderOnly_PreservesRom) {
	auto rom = CreateFakeRom(32768);

	auto saved = AtariLynxFormat::SaveHeaderOnly(rom.data(), static_cast<uint32_t>(rom.size()));
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	ASSERT_EQ(loaded.RomData.size(), rom.size());
	EXPECT_EQ(loaded.RomData, rom);
}

TEST_F(AtariLynxFormatTest, Roundtrip_HeaderOnly_NoMetadata) {
	auto rom = CreateFakeRom(1024);

	auto saved = AtariLynxFormat::SaveHeaderOnly(rom.data(), static_cast<uint32_t>(rom.size()));
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_FALSE(loaded.HasMetadata);
}

TEST_F(AtariLynxFormatTest, Roundtrip_HeaderOnly_CorrectFileSize) {
	auto rom = CreateFakeRom(8192);

	auto saved = AtariLynxFormat::SaveHeaderOnly(rom.data(), static_cast<uint32_t>(rom.size()));
	EXPECT_EQ(saved.size(), 32u + 8192u); // 32-byte header + ROM
}

//=============================================================================
// LNX → atari-lynx Conversion
//=============================================================================

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsRom) {
	auto rom = CreateFakeRom(65536);
	auto lnx = CreateLnxFile(rom);

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	ASSERT_EQ(result.RomData.size(), rom.size());
	EXPECT_EQ(result.RomData, rom);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsCartName) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom, "SuperGame");

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.CartName, "SuperGame");
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsManufacturer) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom, "TestGame", "Atari");

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.Manufacturer, "Atari");
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsBankSizes) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom, "TestGame", "TestMfg", 0x80, 0x40);

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.Bank0PageSize, 0x80);
	EXPECT_EQ(result.Metadata.Bank1PageSize, 0x40);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsRotation) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom, "TestGame", "TestMfg", 0x100, 0, 2); // rotation=Right

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.Rotation, 2);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ExtractsEeprom) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom, "TestGame", "TestMfg", 0x100, 0, 0, 0x41); // eeprom=1, SD flag

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.EepromType, 1); // 93C46
	EXPECT_EQ(result.Metadata.EepromFlags, 1); // SD flag
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_HasMetadata) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom);

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_TRUE(result.HasMetadata);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_ComputesCrc32) {
	auto rom = CreateFakeRom(1024);
	uint32_t expectedCrc = CRC32::GetCRC(rom.data(), static_cast<std::streamoff>(rom.size()));
	auto lnx = CreateLnxFile(rom);

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.RomCrc32, expectedCrc);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_RejectsBadMagic) {
	auto rom = CreateFakeRom(1024);
	auto lnx = CreateLnxFile(rom);
	lnx[0] = 'N'; // Break magic

	auto result = AtariLynxFormat::ConvertFromLnx(lnx.data(), lnx.size());

	EXPECT_FALSE(result.Success);
}

TEST_F(AtariLynxFormatTest, ConvertFromLnx_RejectsTooSmall) {
	std::vector<uint8_t> data(32, 0);
	data[0] = 'L'; data[1] = 'Y'; data[2] = 'N'; data[3] = 'X';

	auto result = AtariLynxFormat::ConvertFromLnx(data.data(), data.size());

	EXPECT_FALSE(result.Success);
}

//=============================================================================
// Raw → atari-lynx Conversion
//=============================================================================

TEST_F(AtariLynxFormatTest, ConvertFromRaw_PreservesRom) {
	auto rom = CreateFakeRom(65536);

	auto result = AtariLynxFormat::ConvertFromRaw(rom.data(), rom.size());

	ASSERT_TRUE(result.Success);
	ASSERT_EQ(result.RomData.size(), rom.size());
	EXPECT_EQ(result.RomData, rom);
}

TEST_F(AtariLynxFormatTest, ConvertFromRaw_InfersBankSize) {
	auto rom = CreateFakeRom(65536); // 256 * 256 = 65536

	auto result = AtariLynxFormat::ConvertFromRaw(rom.data(), rom.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.Metadata.Bank0PageSize, 256);
	EXPECT_EQ(result.Metadata.Bank1PageSize, 0);
}

TEST_F(AtariLynxFormatTest, ConvertFromRaw_ComputesCrc32) {
	auto rom = CreateFakeRom(4096);
	uint32_t expectedCrc = CRC32::GetCRC(rom.data(), static_cast<std::streamoff>(rom.size()));

	auto result = AtariLynxFormat::ConvertFromRaw(rom.data(), rom.size());

	ASSERT_TRUE(result.Success);
	EXPECT_EQ(result.RomCrc32, expectedCrc);
}

TEST_F(AtariLynxFormatTest, ConvertFromRaw_RejectsEmpty) {
	auto result = AtariLynxFormat::ConvertFromRaw(nullptr, 0);
	EXPECT_FALSE(result.Success);
}

//=============================================================================
// atari-lynx → LNX Back-Conversion
//=============================================================================

TEST_F(AtariLynxFormatTest, ConvertToLnx_ProducesValidHeader) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();

	auto lnx = AtariLynxFormat::ConvertToLnx(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	ASSERT_GE(lnx.size(), 64u + rom.size());
	EXPECT_EQ(lnx[0], 'L');
	EXPECT_EQ(lnx[1], 'Y');
	EXPECT_EQ(lnx[2], 'N');
	EXPECT_EQ(lnx[3], 'X');
}

TEST_F(AtariLynxFormatTest, ConvertToLnx_PreservesRom) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();

	auto lnx = AtariLynxFormat::ConvertToLnx(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	std::vector<uint8_t> extractedRom(lnx.begin() + 64, lnx.end());
	EXPECT_EQ(extractedRom, rom);
}

TEST_F(AtariLynxFormatTest, ConvertToLnx_PreservesBankSizes) {
	auto rom = CreateFakeRom(1024);
	AtariLynxMetadata meta;
	meta.Bank0PageSize = 0x80;
	meta.Bank1PageSize = 0x40;

	auto lnx = AtariLynxFormat::ConvertToLnx(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	uint16_t bank0 = lnx[4] | (lnx[5] << 8);
	uint16_t bank1 = lnx[6] | (lnx[7] << 8);
	EXPECT_EQ(bank0, 0x80);
	EXPECT_EQ(bank1, 0x40);
}

TEST_F(AtariLynxFormatTest, ConvertToLnx_PreservesName) {
	auto rom = CreateFakeRom(1024);
	AtariLynxMetadata meta;
	meta.CartName = "TestCartridge";

	auto lnx = AtariLynxFormat::ConvertToLnx(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	std::string name(reinterpret_cast<const char*>(&lnx[10]), 32);
	name = name.c_str(); // Trim at null
	EXPECT_EQ(name, "TestCartridge");
}

TEST_F(AtariLynxFormatTest, ConvertToLnx_PreservesRotation) {
	auto rom = CreateFakeRom(1024);
	AtariLynxMetadata meta;
	meta.Rotation = 1; // Left

	auto lnx = AtariLynxFormat::ConvertToLnx(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	EXPECT_EQ(lnx[58], 1);
}

//=============================================================================
// LNX → atari-lynx → LNX Roundtrip
//=============================================================================

TEST_F(AtariLynxFormatTest, LnxRoundtrip_RomPreserved) {
	auto rom = CreateFakeRom(65536);
	auto originalLnx = CreateLnxFile(rom, "Roundtrip", "Test", 0x100, 0, 1, 0x02);

	// LNX → atari-lynx
	auto converted = AtariLynxFormat::ConvertFromLnx(originalLnx.data(), originalLnx.size());
	ASSERT_TRUE(converted.Success);

	// atari-lynx → LNX
	auto backLnx = AtariLynxFormat::ConvertToLnx(
		converted.RomData.data(),
		static_cast<uint32_t>(converted.RomData.size()),
		converted.Metadata);

	// Extract ROM from back-converted LNX
	std::vector<uint8_t> backRom(backLnx.begin() + 64, backLnx.end());
	EXPECT_EQ(backRom, rom);
}

TEST_F(AtariLynxFormatTest, LnxRoundtrip_MetadataPreserved) {
	auto rom = CreateFakeRom(1024);
	auto originalLnx = CreateLnxFile(rom, "MyGame", "MyCompany", 0x80, 0x20, 2, 0x03);

	auto converted = AtariLynxFormat::ConvertFromLnx(originalLnx.data(), originalLnx.size());
	ASSERT_TRUE(converted.Success);

	auto backLnx = AtariLynxFormat::ConvertToLnx(
		converted.RomData.data(),
		static_cast<uint32_t>(converted.RomData.size()),
		converted.Metadata);

	// Verify LNX header fields match
	uint16_t bank0 = backLnx[4] | (backLnx[5] << 8);
	uint16_t bank1 = backLnx[6] | (backLnx[7] << 8);
	EXPECT_EQ(bank0, 0x80);
	EXPECT_EQ(bank1, 0x20);
	EXPECT_EQ(backLnx[58], 2); // Rotation

	std::string name(reinterpret_cast<const char*>(&backLnx[10]), 32);
	name = name.c_str();
	EXPECT_EQ(name, "MyGame");

	std::string mfg(reinterpret_cast<const char*>(&backLnx[42]), 16);
	mfg = mfg.c_str();
	EXPECT_EQ(mfg, "MyCompany");
}

//=============================================================================
// Save → Load → Save Roundtrip (atari-lynx → atari-lynx)
//=============================================================================

TEST_F(AtariLynxFormatTest, FullRoundtrip_SaveLoadSave_Identical) {
	auto rom = CreateFakeRom(16384);
	auto meta = CreateSampleMetadata();

	auto saved1 = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved1.data(), saved1.size());
	ASSERT_TRUE(loaded.Success);

	auto saved2 = AtariLynxFormat::Save(
		loaded.RomData.data(),
		static_cast<uint32_t>(loaded.RomData.size()),
		loaded.Metadata);

	EXPECT_EQ(saved1, saved2);
}

//=============================================================================
// Error Handling
//=============================================================================

TEST_F(AtariLynxFormatTest, Load_BadMagic_Fails) {
	std::vector<uint8_t> data(64, 0);
	data[0] = 'B'; data[1] = 'A'; data[2] = 'D'; data[3] = '!';

	auto result = AtariLynxFormat::Load(data.data(), data.size());

	EXPECT_FALSE(result.Success);
	EXPECT_FALSE(result.Error.empty());
}

TEST_F(AtariLynxFormatTest, Load_TooSmall_Fails) {
	std::vector<uint8_t> data(16, 0);

	auto result = AtariLynxFormat::Load(data.data(), data.size());

	EXPECT_FALSE(result.Success);
}

TEST_F(AtariLynxFormatTest, Load_UnsupportedMajorVersion_Fails) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();
	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	// Corrupt the major version to 99
	saved[8] = 99;

	auto result = AtariLynxFormat::Load(saved.data(), saved.size());

	EXPECT_FALSE(result.Success);
	EXPECT_TRUE(result.Error.find("version") != std::string::npos ||
		result.Error.find("Unsupported") != std::string::npos);
}

TEST_F(AtariLynxFormatTest, Load_ZeroRomSize_Fails) {
	std::vector<uint8_t> data(64, 0);
	memcpy(data.data(), AtariLynxFormatConstants::Magic, 8);
	// RomSize at offset 12 = 0 (already zero)

	auto result = AtariLynxFormat::Load(data.data(), data.size());

	EXPECT_FALSE(result.Success);
}

TEST_F(AtariLynxFormatTest, Load_TruncatedFile_Fails) {
	auto rom = CreateFakeRom(1024);
	auto meta = CreateSampleMetadata();
	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);

	// Truncate to just the header
	saved.resize(32);

	auto result = AtariLynxFormat::Load(saved.data(), saved.size());

	EXPECT_FALSE(result.Success);
}

//=============================================================================
// Edge Cases
//=============================================================================

TEST_F(AtariLynxFormatTest, EmptyMetadataStrings_Roundtrip) {
	auto rom = CreateFakeRom(256);
	AtariLynxMetadata meta; // All strings empty, all numbers zero

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	ASSERT_TRUE(loaded.HasMetadata);
	EXPECT_TRUE(loaded.Metadata.CartName.empty());
	EXPECT_TRUE(loaded.Metadata.Manufacturer.empty());
	EXPECT_TRUE(loaded.Metadata.Language.empty());
}

TEST_F(AtariLynxFormatTest, SmallRom_256Bytes) {
	auto rom = CreateFakeRom(256);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_EQ(loaded.RomData, rom);
}

TEST_F(AtariLynxFormatTest, LargeRom_512KB) {
	auto rom = CreateFakeRom(512 * 1024);
	auto meta = CreateSampleMetadata();

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_EQ(loaded.RomData.size(), rom.size());
	EXPECT_EQ(loaded.RomData, rom);
}

TEST_F(AtariLynxFormatTest, UnicodeCartName_Roundtrip) {
	auto rom = CreateFakeRom(256);
	AtariLynxMetadata meta;
	meta.CartName = "Jeu Français";

	auto saved = AtariLynxFormat::Save(rom.data(), static_cast<uint32_t>(rom.size()), meta);
	auto loaded = AtariLynxFormat::Load(saved.data(), saved.size());

	ASSERT_TRUE(loaded.Success);
	EXPECT_EQ(loaded.Metadata.CartName, "Jeu Français");
}
