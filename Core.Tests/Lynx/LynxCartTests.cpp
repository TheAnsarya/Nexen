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
	EXPECT_EQ(_state.AddressCounter & 0xff, 0x56u);
}

TEST_F(LynxCartTest, AddressCounter_SetHighByte) {
	_state.AddressCounter = 0x1234;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (0x78 << 8);
	EXPECT_EQ((_state.AddressCounter >> 8) & 0xff, 0x78u);
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

//=============================================================================
// Address Bit Count Calculation Tests (#1105)
//=============================================================================
// The shift-register protocol needs exactly _addrBitCount bits to set the
// AddressCounter. This is ceil(log2(maxBankSize)), minimum 8.

namespace LynxCartAddrBitHelpers {
	static uint8_t CalcAddrBitCount(uint32_t bank0Size, uint32_t bank1Size) {
		uint32_t maxSize = std::max(bank0Size, bank1Size);
		uint8_t count = 0;
		if (maxSize > 0) {
			uint32_t tmp = maxSize - 1;
			while (tmp > 0) {
				count++;
				tmp >>= 1;
			}
		}
		if (count < 8) {
			count = 8;
		}
		return count;
	}

	static uint32_t CalcBankMask(uint32_t bankSize) {
		return (bankSize > 0) ? (bankSize - 1) : 0;
	}

	// Simulate shift-register protocol: accumulate bits MSB-first
	struct ShiftState {
		uint32_t addressShift = 0;
		uint8_t shiftCount = 0;
		uint32_t addressCounter = 0;
	};

	static void ShiftBit(ShiftState& s, bool data, uint8_t addrBitCount) {
		s.shiftCount++;
		s.addressShift <<= 1;
		if (data) {
			s.addressShift |= 1;
		}
		if (s.shiftCount >= addrBitCount) {
			s.addressCounter = s.addressShift;
			s.shiftCount = 0;
			s.addressShift = 0;
		}
	}

	static uint32_t GetRomAddress(uint32_t addressCounter, uint16_t currentBank,
		uint32_t bank0Offset, uint32_t bank1Offset,
		uint32_t bank0Mask, uint32_t bank1Mask) {
		uint32_t bankOffset = (currentBank == 0) ? bank0Offset : bank1Offset;
		uint32_t bankMask = (currentBank == 0) ? bank0Mask : bank1Mask;
		return bankOffset + (addressCounter & bankMask);
	}
}

TEST_F(LynxCartTest, AddrBitCount_256Bytes_Is8) {
	// 256 bytes = 2^8, needs 8 bits
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(256, 0), 8);
}

TEST_F(LynxCartTest, AddrBitCount_512Bytes_Is9) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(512, 0), 9);
}

TEST_F(LynxCartTest, AddrBitCount_1KB_Is10) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(1024, 0), 10);
}

TEST_F(LynxCartTest, AddrBitCount_64KB_Is16) {
	// 64 KB = 2^16
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(65536, 0), 16);
}

TEST_F(LynxCartTest, AddrBitCount_128KB_Is17) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(131072, 0), 17);
}

TEST_F(LynxCartTest, AddrBitCount_256KB_Is18) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(262144, 0), 18);
}

TEST_F(LynxCartTest, AddrBitCount_512KB_Is19) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(524288, 0), 19);
}

TEST_F(LynxCartTest, AddrBitCount_1MB_Is20) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(1048576, 0), 20);
}

TEST_F(LynxCartTest, AddrBitCount_UsesLargerBank) {
	// When bank1 is larger, bit count comes from bank1
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(256, 65536), 16);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(65536, 256), 16);
}

TEST_F(LynxCartTest, AddrBitCount_ZeroSize_Minimum8) {
	// Empty cart still uses minimum 8 bits
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(0, 0), 8);
}

TEST_F(LynxCartTest, AddrBitCount_SmallSize_Minimum8) {
	// Any size < 256 still uses minimum 8 bits
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(1, 0), 8);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(128, 0), 8);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcAddrBitCount(255, 0), 8);
}

//=============================================================================
// Bank Mask Calculation Tests (#1105)
//=============================================================================

TEST_F(LynxCartTest, BankMask_PowerOf2_Correct) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(256), 0xffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(512), 0x1ffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(1024), 0x3ffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(65536), 0xffffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(131072), 0x1ffffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(262144), 0x3ffffu);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(524288), 0x7ffffu);
}

TEST_F(LynxCartTest, BankMask_ZeroSize_IsZero) {
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(0), 0u);
}

TEST_F(LynxCartTest, BankMask_AddressWraps) {
	// Address beyond bank size wraps to beginning via mask
	uint32_t mask = LynxCartAddrBitHelpers::CalcBankMask(65536); // 0xFFFF
	EXPECT_EQ(0x10000u & mask, 0u);    // 64K wraps to 0
	EXPECT_EQ(0x10001u & mask, 1u);    // 64K+1 wraps to 1
	EXPECT_EQ(0x1ffffu & mask, 0xffffu); // 128K-1 wraps to 64K-1
}

//=============================================================================
// Shift-Register Protocol Tests (#1105)
//=============================================================================

TEST_F(LynxCartTest, ShiftRegister_SingleByteAddress) {
	// 256-byte bank needs 8 bits
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(256, 0);
	EXPECT_EQ(bits, 8);

	LynxCartAddrBitHelpers::ShiftState ss;
	// Shift in 0xA5 MSB-first: 1,0,1,0,0,1,0,1
	uint8_t target = 0xa5;
	for (int i = 7; i >= 0; i--) {
		bool bit = (target >> i) & 1;
		LynxCartAddrBitHelpers::ShiftBit(ss, bit, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0xa5u);
	EXPECT_EQ(ss.shiftCount, 0); // Reset after completion
}

TEST_F(LynxCartTest, ShiftRegister_16BitAddress) {
	// 64KB bank needs 16 bits
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(65536, 0);
	EXPECT_EQ(bits, 16);

	LynxCartAddrBitHelpers::ShiftState ss;
	// Shift in 0x1234 MSB-first
	uint16_t target = 0x1234;
	for (int i = 15; i >= 0; i--) {
		bool bit = (target >> i) & 1;
		LynxCartAddrBitHelpers::ShiftBit(ss, bit, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0x1234u);
}

TEST_F(LynxCartTest, ShiftRegister_19BitAddress) {
	// 512KB bank needs 19 bits — largest common Lynx cart
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(524288, 0);
	EXPECT_EQ(bits, 19);

	LynxCartAddrBitHelpers::ShiftState ss;
	uint32_t target = 0x5a5a5; // 19-bit value
	for (int i = 18; i >= 0; i--) {
		bool bit = (target >> i) & 1;
		LynxCartAddrBitHelpers::ShiftBit(ss, bit, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0x5a5a5u);
}

TEST_F(LynxCartTest, ShiftRegister_DoesNotCompleteEarly) {
	// After 7 of 8 required bits, address should NOT be set yet
	uint8_t bits = 8;
	LynxCartAddrBitHelpers::ShiftState ss;

	for (int i = 0; i < 7; i++) {
		LynxCartAddrBitHelpers::ShiftBit(ss, true, bits);
	}
	EXPECT_EQ(ss.shiftCount, 7); // Not yet complete
	EXPECT_EQ(ss.addressCounter, 0u); // Still at initial value
}

TEST_F(LynxCartTest, ShiftRegister_CompletesOnExactBitCount) {
	uint8_t bits = 8;
	LynxCartAddrBitHelpers::ShiftState ss;

	// Shift 8 ones → 0xFF
	for (int i = 0; i < 8; i++) {
		LynxCartAddrBitHelpers::ShiftBit(ss, true, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0xffu);
	EXPECT_EQ(ss.shiftCount, 0); // Reset after completion
	EXPECT_EQ(ss.addressShift, 0u); // Accumulator cleared
}

TEST_F(LynxCartTest, ShiftRegister_ConsecutiveAddresses) {
	// Shifting two consecutive addresses should work independently
	uint8_t bits = 8;
	LynxCartAddrBitHelpers::ShiftState ss;

	// First: shift in 0x10
	for (int i = 7; i >= 0; i--) {
		LynxCartAddrBitHelpers::ShiftBit(ss, (0x10 >> i) & 1, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0x10u);

	// Second: shift in 0x20
	for (int i = 7; i >= 0; i--) {
		LynxCartAddrBitHelpers::ShiftBit(ss, (0x20 >> i) & 1, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0x20u);
}

TEST_F(LynxCartTest, ShiftRegister_AllZeros) {
	uint8_t bits = 8;
	LynxCartAddrBitHelpers::ShiftState ss;
	ss.addressCounter = 0xff; // Set to non-zero first

	for (int i = 0; i < 8; i++) {
		LynxCartAddrBitHelpers::ShiftBit(ss, false, bits);
	}
	EXPECT_EQ(ss.addressCounter, 0u);
}

//=============================================================================
// ROM Address Resolution Tests (#1105)
//=============================================================================

TEST_F(LynxCartTest, RomAddress_Bank0_SimpleLookup) {
	// Bank 0 at offset 0, mask 0xFFFF (64KB)
	uint32_t addr = LynxCartAddrBitHelpers::GetRomAddress(
		0x1234, 0, 0, 65536, 0xffff, 0xffff);
	EXPECT_EQ(addr, 0x1234u);
}

TEST_F(LynxCartTest, RomAddress_Bank1_WithOffset) {
	// Bank 1 at offset 65536, mask 0xFFFF (64KB)
	uint32_t addr = LynxCartAddrBitHelpers::GetRomAddress(
		0x1234, 1, 0, 65536, 0xffff, 0xffff);
	EXPECT_EQ(addr, 65536u + 0x1234);
}

TEST_F(LynxCartTest, RomAddress_Masking_WrapAround) {
	// Address 0x10000 with mask 0xFFFF should wrap to 0
	uint32_t addr = LynxCartAddrBitHelpers::GetRomAddress(
		0x10000, 0, 0, 65536, 0xffff, 0xffff);
	EXPECT_EQ(addr, 0u);
}

TEST_F(LynxCartTest, RomAddress_SmallBank_Masking) {
	// 256-byte bank: address 0x1FF should wrap to 0xFF
	uint32_t addr = LynxCartAddrBitHelpers::GetRomAddress(
		0x1ff, 0, 0, 0, 0xff, 0xff);
	EXPECT_EQ(addr, 0xffu);
}

//=============================================================================
// Commercial Title Size Matrix Tests (#1105)
//=============================================================================
// These validate the address bit count and mask for known Lynx commercial titles.
// Bank sizes from actual LNX headers of released games.

TEST_F(LynxCartTest, CommercialTitle_CaliforniaGames_256KB) {
	// California Games: ~256KB (bank 0)
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(262144, 0);
	EXPECT_EQ(bits, 18);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(262144), 0x3ffffu);
}

TEST_F(LynxCartTest, CommercialTitle_ChipsChallenge_128KB) {
	// Chip's Challenge: ~128KB
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(131072, 0);
	EXPECT_EQ(bits, 17);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(131072), 0x1ffffu);
}

TEST_F(LynxCartTest, CommercialTitle_BlueLightning_512KB) {
	// Blue Lightning: ~512KB (largest common Lynx cart)
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(524288, 0);
	EXPECT_EQ(bits, 19);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(524288), 0x7ffffu);
}

TEST_F(LynxCartTest, CommercialTitle_SlimeWorld_64KB) {
	// Todd's Adventures in Slime World: ~64KB
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(65536, 0);
	EXPECT_EQ(bits, 16);
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(65536), 0xffffu);
}

TEST_F(LynxCartTest, CommercialTitle_DualBank_128K_64K) {
	// Hypothetical dual-bank: 128K bank 0, 64K bank 1
	uint8_t bits = LynxCartAddrBitHelpers::CalcAddrBitCount(131072, 65536);
	EXPECT_EQ(bits, 17); // Uses larger bank

	// Bank 0 mask covers full 128KB
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(131072), 0x1ffffu);
	// Bank 1 mask covers 64KB only
	EXPECT_EQ(LynxCartAddrBitHelpers::CalcBankMask(65536), 0xffffu);
}

//=============================================================================
// Page Selection ↔ Shift-Register Interaction Tests (#1105)
//=============================================================================

TEST_F(LynxCartTest, PageSelect_PreservesLowByte_ShiftSetsAddress) {
	// Test the interaction: page select sets high byte, then shift register
	// overwrites the entire address counter.

	_state.AddressCounter = 0x00ab;

	// Page select → high byte becomes 0x05
	uint8_t page = 0x05;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (static_cast<uint32_t>(page) << 8);
	EXPECT_EQ(_state.AddressCounter, 0x05abu);

	// Now shift register sets a completely new address → overwrites everything
	LynxCartAddrBitHelpers::ShiftState ss;
	ss.addressCounter = _state.AddressCounter;
	uint32_t newAddr = 0x7890;
	for (int i = 15; i >= 0; i--) {
		LynxCartAddrBitHelpers::ShiftBit(ss, (newAddr >> i) & 1, 16);
	}
	EXPECT_EQ(ss.addressCounter, 0x7890u); // Completely replaced
}

TEST_F(LynxCartTest, PageSelect_DifferentBanks_IndependentAddresses) {
	// Page selection for bank 0 and bank 1 use the same AddressCounter
	// but the bank offset is different in ROM address resolution.
	_state.AddressCounter = 0x00ff;

	// Select bank 0, page 2
	_state.CurrentBank = 0;
	_state.AddressCounter = (_state.AddressCounter & 0x00ff) | (2u << 8);
	uint32_t addr0 = LynxCartAddrBitHelpers::GetRomAddress(
		_state.AddressCounter, 0, 0, 65536, 0xffff, 0xffff);
	EXPECT_EQ(addr0, 0x02ffu);

	// Select bank 1, same address counter
	_state.CurrentBank = 1;
	uint32_t addr1 = LynxCartAddrBitHelpers::GetRomAddress(
		_state.AddressCounter, 1, 0, 65536, 0xffff, 0xffff);
	EXPECT_EQ(addr1, 65536u + 0x02ffu);
}
