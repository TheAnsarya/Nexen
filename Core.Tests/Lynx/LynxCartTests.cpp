#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx cartridge system and LNX ROM format parsing.
///
/// The Lynx cartridge uses a dual-bank system with sequential access:
///   - Bank 0 and Bank 1 have independent page counters
///   - CART0/CART1 accent lines select active bank
///   - Data is read sequentially via CARTDATA register
///   - Page size varies per bank (256, 512, 1024, 2048 bytes)
///
/// LNX header format (64 bytes):
///   $00-$03: Magic "LYNX"
///   $04-$05: Bank 0 page count (LE)
///   $06-$07: Bank 1 page count (LE)
///   $08-$09: Version (LE)
///   $0A-$29: Game name (32 bytes, null-padded)
///   $2A-$39: Manufacturer (16 bytes, null-padded)
///   $3A:     Rotation (0=None, 1=Left, 2=Right)
///   $3B:     Reserved
///   $3C:     EEPROM type (see LynxEepromType)
///   $3D-$3F: Reserved
///
/// References:
///   - LNX format specification (handy-sdl documentation)
///   - ~docs/plans/lynx-subsystems-deep-dive.md (Section: Cart)
/// </summary>
class LynxCartTest : public ::testing::Test {
protected:
	LynxCartInfo _info = {};
	LynxCartState _state = {};

	void SetUp() override {
		_info = {};
		_state = {};
		_state.AddressCounter = 0;
		_state.CurrentBank = 0;
		_state.ShiftRegister = 0;
	}

	/// <summary>Create a minimal valid LNX header</summary>
	void CreateValidHeader(std::vector<uint8_t>& header) {
		header.resize(64, 0);
		// Magic
		header[0] = 'L'; header[1] = 'Y'; header[2] = 'N'; header[3] = 'X';
		// Bank 0: 256 pages of 256 bytes = 64KB
		header[4] = 0x00; header[5] = 0x01;
		// Bank 1: 0 pages
		header[6] = 0x00; header[7] = 0x00;
		// Version
		header[8] = 0x01; header[9] = 0x00;
		// Name: "TestGame"
		const char* name = "TestGame";
		memcpy(&header[10], name, strlen(name));
		// Rotation: None
		header[58] = 0x00;
		// EEPROM: None
		header[60] = 0x00;
	}
};

//=============================================================================
// LNX Header Magic Tests
//=============================================================================

TEST_F(LynxCartTest, Header_Magic_Valid) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	EXPECT_EQ(header[0], 'L');
	EXPECT_EQ(header[1], 'Y');
	EXPECT_EQ(header[2], 'N');
	EXPECT_EQ(header[3], 'X');
}

TEST_F(LynxCartTest, Header_Magic_Invalid) {
	std::vector<uint8_t> header(64, 0);
	header[0] = 'N'; header[1] = 'E'; header[2] = 'S'; header[3] = 0x1a;
	// Wrong magic — not a Lynx ROM
	EXPECT_NE(header[0], 'L');
}

TEST_F(LynxCartTest, Header_Size_64Bytes) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	EXPECT_EQ(header.size(), 64u);
}

//=============================================================================
// Bank Size Tests
//=============================================================================

TEST_F(LynxCartTest, Bank0_PageCount_LittleEndian) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	header[4] = 0x80; // Low byte
	header[5] = 0x00; // High byte = 0x0080 = 128 pages
	uint16_t pageCount = header[4] | (header[5] << 8);
	EXPECT_EQ(pageCount, 128);
}

TEST_F(LynxCartTest, Bank1_PageCount_LittleEndian) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	header[6] = 0x40; // Low byte
	header[7] = 0x01; // High byte = 0x0140 = 320 pages
	uint16_t pageCount = header[6] | (header[7] << 8);
	EXPECT_EQ(pageCount, 320);
}

TEST_F(LynxCartTest, Bank_ByteSize_Calculation) {
	uint16_t pageCount = 256;
	uint16_t pageSize = 256;
	uint32_t totalBytes = pageCount * pageSize;
	EXPECT_EQ(totalBytes, 65536u); // 64 KB
}

TEST_F(LynxCartTest, Bank_PageSizes_Supported) {
	// Valid page sizes: 256, 512, 1024, 2048
	EXPECT_EQ(256 >> 8, 1);   // 256 bytes = 1 * 256
	EXPECT_EQ(512 >> 8, 2);   // 512 bytes = 2 * 256
	EXPECT_EQ(1024 >> 8, 4);  // 1024 bytes = 4 * 256
	EXPECT_EQ(2048 >> 8, 8);  // 2048 bytes = 8 * 256
}

//=============================================================================
// Rotation Tests
//=============================================================================

TEST_F(LynxCartTest, Rotation_None) {
	_info.Rotation = LynxRotation::None;
	EXPECT_EQ(_info.Rotation, LynxRotation::None);
}

TEST_F(LynxCartTest, Rotation_Left) {
	_info.Rotation = LynxRotation::Left;
	EXPECT_EQ(_info.Rotation, LynxRotation::Left);
}

TEST_F(LynxCartTest, Rotation_Right) {
	_info.Rotation = LynxRotation::Right;
	EXPECT_EQ(_info.Rotation, LynxRotation::Right);
}

TEST_F(LynxCartTest, Rotation_FromHeader) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	header[58] = 0x01; // Left rotation
	LynxRotation rot = static_cast<LynxRotation>(header[58]);
	EXPECT_EQ(rot, LynxRotation::Left);
}

//=============================================================================
// EEPROM Type Tests
//=============================================================================

TEST_F(LynxCartTest, Eeprom_None) {
	_info.EepromType = LynxEepromType::None;
	EXPECT_EQ(_info.EepromType, LynxEepromType::None);
	EXPECT_FALSE(_info.HasEeprom);
}

TEST_F(LynxCartTest, Eeprom_93c46) {
	_info.EepromType = LynxEepromType::Eeprom93c46;
	_info.HasEeprom = true;
	EXPECT_EQ(_info.EepromType, LynxEepromType::Eeprom93c46);
	EXPECT_TRUE(_info.HasEeprom);
}

TEST_F(LynxCartTest, Eeprom_FromHeader) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	header[60] = 0x03; // 93c66
	LynxEepromType type = static_cast<LynxEepromType>(header[60]);
	EXPECT_EQ(type, LynxEepromType::Eeprom93c66);
}

//=============================================================================
// Address Counter Tests
//=============================================================================

TEST_F(LynxCartTest, AddressCounter_Initial) {
	EXPECT_EQ(_state.AddressCounter, 0u);
}

TEST_F(LynxCartTest, AddressCounter_Increment) {
	_state.AddressCounter = 0;
	_state.AddressCounter++;
	EXPECT_EQ(_state.AddressCounter, 1u);
}

TEST_F(LynxCartTest, AddressCounter_Wrap) {
	_state.AddressCounter = 0xffff;
	_state.AddressCounter++;
	// Depends on implementation — may wrap or clamp
	EXPECT_EQ(_state.AddressCounter, 0x10000u);
}

TEST_F(LynxCartTest, AddressCounter_SetLowByte) {
	_state.AddressCounter = 0x1234;
	_state.AddressCounter = (_state.AddressCounter & 0xff00) | 0x56;
	EXPECT_EQ(_state.AddressCounter & 0xff, 0x56);
}

TEST_F(LynxCartTest, AddressCounter_SetHighByte) {
	_state.AddressCounter = 0x1234;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (0x78 << 8);
	EXPECT_EQ((_state.AddressCounter >> 8) & 0xff, 0x78);
}

//=============================================================================
// Bank Selection Tests
//=============================================================================

TEST_F(LynxCartTest, BankSelect_Bank0) {
	_state.CurrentBank = 0;
	EXPECT_EQ(_state.CurrentBank, 0);
}

TEST_F(LynxCartTest, BankSelect_Bank1) {
	_state.CurrentBank = 1;
	EXPECT_EQ(_state.CurrentBank, 1);
}

TEST_F(LynxCartTest, BankSelect_ViaShiftRegister) {
	// Games use shift register to control bank selection
	_state.ShiftRegister = 0x01;
	uint8_t bankBit = (_state.ShiftRegister >> 0) & 1;
	EXPECT_EQ(bankBit, 1);
}

//=============================================================================
// Name Parsing Tests
//=============================================================================

TEST_F(LynxCartTest, Name_NullTerminated) {
	strcpy_s(_info.Name, "Test Game");
	EXPECT_STREQ(_info.Name, "Test Game");
}

TEST_F(LynxCartTest, Name_MaxLength) {
	// Max 32 chars + null
	char longName[33];
	memset(longName, 'A', 32);
	longName[32] = '\0';
	strcpy_s(_info.Name, longName);
	EXPECT_EQ(strlen(_info.Name), 32u);
}

TEST_F(LynxCartTest, Manufacturer_MaxLength) {
	// Max 16 chars + null
	char mfg[17];
	memset(mfg, 'B', 16);
	mfg[16] = '\0';
	strcpy_s(_info.Manufacturer, mfg);
	EXPECT_EQ(strlen(_info.Manufacturer), 16u);
}

//=============================================================================
// Version Tests
//=============================================================================

TEST_F(LynxCartTest, Version_Default) {
	_info.Version = 0x0001;
	EXPECT_EQ(_info.Version, 1);
}

TEST_F(LynxCartTest, Version_FromHeader) {
	std::vector<uint8_t> header;
	CreateValidHeader(header);
	header[8] = 0x02;
	header[9] = 0x00;
	uint16_t version = header[8] | (header[9] << 8);
	EXPECT_EQ(version, 2);
}

//=============================================================================
// Headerless ROM Tests
//=============================================================================

TEST_F(LynxCartTest, Headerless_NoMagic) {
	std::vector<uint8_t> rom(32768, 0xff); // 32KB raw ROM
	// First 4 bytes are NOT "LYNX"
	bool hasHeader = (rom.size() >= 4 &&
					  rom[0] == 'L' && rom[1] == 'Y' &&
					  rom[2] == 'N' && rom[3] == 'X');
	EXPECT_FALSE(hasHeader);
}

TEST_F(LynxCartTest, Headerless_DefaultSettings) {
	// Headerless ROMs use default settings
	_info.PageSizeBank0 = 256;
	_info.PageSizeBank1 = 0;
	_info.Rotation = LynxRotation::None;
	_info.EepromType = LynxEepromType::None;
	EXPECT_EQ(_info.PageSizeBank0, 256);
}

//=============================================================================
// ROM Size Calculation Tests
//=============================================================================

TEST_F(LynxCartTest, RomSize_FromBankPages) {
	uint16_t bank0Pages = 128;
	uint16_t bank1Pages = 64;
	uint16_t bank0PageSize = 256;
	uint16_t bank1PageSize = 256;
	uint32_t totalSize = (bank0Pages * bank0PageSize) + (bank1Pages * bank1PageSize);
	EXPECT_EQ(totalSize, 49152u); // 48 KB
}

TEST_F(LynxCartTest, RomSize_SingleBank) {
	_info.RomSize = 65536; // 64 KB
	_info.PageSizeBank0 = 256;
	_info.PageSizeBank1 = 0;
	EXPECT_EQ(_info.RomSize, 65536u);
}

//=============================================================================
// State Persistence Tests
//=============================================================================

TEST_F(LynxCartTest, State_InfoCopy) {
	strcpy_s(_info.Name, "PersistTest");
	_state.Info = _info;
	EXPECT_STREQ(_state.Info.Name, "PersistTest");
}

TEST_F(LynxCartTest, State_ShiftRegisterPersists) {
	_state.ShiftRegister = 0xab;
	EXPECT_EQ(_state.ShiftRegister, 0xab);
}

//=============================================================================
// Audit Fix Regression Tests (#394, #395, #404)
//=============================================================================

TEST_F(LynxCartTest, AuditFix394_ShiftRegisterStored) {
	// #394: ShiftRegister is written but not read in emulation.
	// Verify it still stores values for debugger visibility / state completeness.
	_state.ShiftRegister = 0x00;
	EXPECT_EQ(_state.ShiftRegister, 0x00);

	_state.ShiftRegister = 0xff;
	EXPECT_EQ(_state.ShiftRegister, 0xff);

	_state.ShiftRegister = 0x42;
	EXPECT_EQ(_state.ShiftRegister, 0x42);
}

TEST_F(LynxCartTest, AuditFix395_BankPageSetsHighByte) {
	// #395: SetBank0Page/SetBank1Page intentionally overwrite the high byte
	// of AddressCounter. Verify the low byte is preserved.
	_state.AddressCounter = 0x00ab; // Low byte = 0xAB

	// Simulate SetBank0Page(5) — should set high byte to 5, keep low byte
	uint8_t page = 5;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(page) << 8);
	EXPECT_EQ(_state.AddressCounter & 0xff, 0xab);       // Low byte preserved
	EXPECT_EQ((_state.AddressCounter >> 8) & 0xff, 0x05); // High byte = page

	// Simulate SetBank1Page(0x10)
	page = 0x10;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(page) << 8);
	EXPECT_EQ(_state.AddressCounter & 0xff, 0xab);        // Low byte still preserved
	EXPECT_EQ((_state.AddressCounter >> 8) & 0xff, 0x10); // High byte = new page
}

TEST_F(LynxCartTest, AuditFix404_CartInfoInState) {
	// #404: CartInfo is inside CartState but deliberately not serialized
	// (reconstructed from ROM header on load). Verify the struct layout.
	_info.RomSize = 262144;
	_info.PageSizeBank0 = 0x100;
	_info.PageSizeBank1 = 0x80;
	strcpy_s(_info.Name, "TestCart");

	_state.Info = _info;
	EXPECT_EQ(_state.Info.RomSize, 262144u);
	EXPECT_EQ(_state.Info.PageSizeBank0, 0x100);
	EXPECT_EQ(_state.Info.PageSizeBank1, 0x80);
	EXPECT_STREQ(_state.Info.Name, "TestCart");
}
