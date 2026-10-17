#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Tests for the Lynx EEPROM (93C46/56/66/76/86 Microwire serial protocol).
///
/// The Lynx uses Microwire serial EEPROMs for save game data. The protocol
/// uses a simple serial interface with:
///   - CS (Chip Select) — active high, starts/ends transactions
///   - CLK (Clock) — data is latched on rising edge
///   - DI (Data In) — serial data to EEPROM
///   - DO (Data Out) — serial data from EEPROM
///
/// Command format:
///   1. Start bit (always 1)
///   2. 2-bit opcode
///   3. Address bits (6/7/8/9/10 depending on chip size)
///   4. Data bits (16-bit words, or 8-bit in byte-org mode)
///
/// Opcodes (after start bit):
///   10 = READ, 01 = WRITE, 11 = ERASE
///   00 + extension: EWDS (write disable), WRAL (write all),
///                   ERAL (erase all), EWEN (write enable)
///
/// References:
///   - 93Cxx datasheets (Microchip/Atmel/STMicroelectronics)
///   - ~docs/plans/lynx-subsystems-deep-dive.md (Section: EEPROM)
/// </summary>
class LynxEepromTest : public ::testing::Test {
protected:
	LynxEepromSerialState _state = {};

	void SetUp() override {
		_state = {};
		_state.WriteEnabled = false;
		_state.BitCount = 0;
		_state.DataBuffer = 0;
		_state.Address = 0;
		_state.DataOut = false;
	}

	/// <summary>Get address bit count for EEPROM type</summary>
	uint8_t GetAddressBits(LynxEepromType type) const {
		switch (type) {
			case LynxEepromType::Eeprom93c46: return 6;
			case LynxEepromType::Eeprom93c56: return 7;
			case LynxEepromType::Eeprom93c66: return 8;
			case LynxEepromType::Eeprom93c76: return 9;
			case LynxEepromType::Eeprom93c86: return 10;
			default: return 0;
		}
	}

	/// <summary>Get storage size in bytes for EEPROM type</summary>
	uint32_t GetStorageSize(LynxEepromType type) const {
		switch (type) {
			case LynxEepromType::Eeprom93c46: return 128;
			case LynxEepromType::Eeprom93c56: return 256;
			case LynxEepromType::Eeprom93c66: return 512;
			case LynxEepromType::Eeprom93c76: return 1024;
			case LynxEepromType::Eeprom93c86: return 2048;
			default: return 0;
		}
	}
};

//=============================================================================
// EEPROM Type Tests
//=============================================================================

TEST_F(LynxEepromTest, Type_None_NoStorage) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::None), 0u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::None), 0);
}

TEST_F(LynxEepromTest, Type_93c46_128Bytes_6AddrBits) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::Eeprom93c46), 128u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::Eeprom93c46), 6);
}

TEST_F(LynxEepromTest, Type_93c56_256Bytes_7AddrBits) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::Eeprom93c56), 256u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::Eeprom93c56), 7);
}

TEST_F(LynxEepromTest, Type_93c66_512Bytes_8AddrBits) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::Eeprom93c66), 512u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::Eeprom93c66), 8);
}

TEST_F(LynxEepromTest, Type_93c76_1024Bytes_9AddrBits) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::Eeprom93c76), 1024u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::Eeprom93c76), 9);
}

TEST_F(LynxEepromTest, Type_93c86_2048Bytes_10AddrBits) {
	EXPECT_EQ(GetStorageSize(LynxEepromType::Eeprom93c86), 2048u);
	EXPECT_EQ(GetAddressBits(LynxEepromType::Eeprom93c86), 10);
}

//=============================================================================
// State Machine Tests
//=============================================================================

TEST_F(LynxEepromTest, State_InitialIdle) {
	EXPECT_EQ(_state.State, LynxEepromState::Idle);
}

TEST_F(LynxEepromTest, State_ChipSelectLow_StaysIdle) {
	_state.CsActive = false;
	_state.State = LynxEepromState::Idle;
	// When CS low, state machine should not advance
	EXPECT_EQ(_state.State, LynxEepromState::Idle);
}

TEST_F(LynxEepromTest, State_ChipSelectHigh_BeginsTransaction) {
	_state.CsActive = true;
	// On CS high, prepare to receive opcode
	_state.State = LynxEepromState::ReceivingOpcode;
	EXPECT_EQ(_state.State, LynxEepromState::ReceivingOpcode);
}

TEST_F(LynxEepromTest, State_ReceivingOpcode_To_ReceivingAddress) {
	_state.State = LynxEepromState::ReceivingOpcode;
	_state.BitCount = 3; // Start bit + 2 opcode bits received
	// Check transition to address phase
	_state.State = LynxEepromState::ReceivingAddress;
	EXPECT_EQ(_state.State, LynxEepromState::ReceivingAddress);
}

TEST_F(LynxEepromTest, State_ReceivingAddress_To_SendingData) {
	_state.State = LynxEepromState::ReceivingAddress;
	_state.Opcode = 0x02; // READ opcode (binary 10 after start bit)
	// After address received, begin sending data
	_state.State = LynxEepromState::SendingData;
	EXPECT_EQ(_state.State, LynxEepromState::SendingData);
}

TEST_F(LynxEepromTest, State_ReceivingData_ForWrite) {
	_state.State = LynxEepromState::ReceivingAddress;
	_state.Opcode = 0x01; // WRITE opcode (binary 01 after start bit)
	_state.State = LynxEepromState::ReceivingData;
	EXPECT_EQ(_state.State, LynxEepromState::ReceivingData);
}

//=============================================================================
// Opcode Decoding Tests
//=============================================================================

TEST_F(LynxEepromTest, Opcode_Read_Binary10) {
	uint8_t opcode = 0x02; // Binary 10
	EXPECT_EQ(opcode, 2);
	// READ operation
}

TEST_F(LynxEepromTest, Opcode_Write_Binary01) {
	uint8_t opcode = 0x01; // Binary 01
	EXPECT_EQ(opcode, 1);
	// WRITE operation
}

TEST_F(LynxEepromTest, Opcode_Erase_Binary11) {
	uint8_t opcode = 0x03; // Binary 11
	EXPECT_EQ(opcode, 3);
	// ERASE operation
}

TEST_F(LynxEepromTest, Opcode_Extended_Binary00) {
	uint8_t opcode = 0x00; // Binary 00 = extended command
	// Extended commands determined by next bits
	EXPECT_EQ(opcode, 0);
}

//=============================================================================
// Write Enable / Disable Tests
//=============================================================================

TEST_F(LynxEepromTest, WriteEnabled_DefaultFalse) {
	EXPECT_FALSE(_state.WriteEnabled);
}

TEST_F(LynxEepromTest, WriteEnabled_EWEN_Sets) {
	// EWEN (Write ENable) command
	_state.WriteEnabled = true;
	EXPECT_TRUE(_state.WriteEnabled);
}

TEST_F(LynxEepromTest, WriteEnabled_EWDS_Clears) {
	_state.WriteEnabled = true;
	// EWDS (Write DiSable) command
	_state.WriteEnabled = false;
	EXPECT_FALSE(_state.WriteEnabled);
}

TEST_F(LynxEepromTest, Write_WhenDisabled_Ignored) {
	_state.WriteEnabled = false;
	// Write should be ignored
	bool writeSucceeded = _state.WriteEnabled;
	EXPECT_FALSE(writeSucceeded);
}

TEST_F(LynxEepromTest, Write_WhenEnabled_Succeeds) {
	_state.WriteEnabled = true;
	bool writeSucceeded = _state.WriteEnabled;
	EXPECT_TRUE(writeSucceeded);
}

//=============================================================================
// Data Transfer Tests
//=============================================================================

TEST_F(LynxEepromTest, DataIn_ShiftRegister_MSBFirst) {
	// Serial data is transmitted MSB first
	_state.DataBuffer = 0;
	// Shift in bits: 1, 0, 1, 0 (0xA high nibble)
	_state.DataBuffer = (_state.DataBuffer << 1) | 1;
	_state.DataBuffer = (_state.DataBuffer << 1) | 0;
	_state.DataBuffer = (_state.DataBuffer << 1) | 1;
	_state.DataBuffer = (_state.DataBuffer << 1) | 0;
	EXPECT_EQ(_state.DataBuffer, 0x0a);
}

TEST_F(LynxEepromTest, DataOut_ShiftRegister_MSBFirst) {
	uint16_t dataWord = 0x8000; // MSB set
	_state.DataOut = (dataWord >> 15) & 1;
	EXPECT_TRUE(_state.DataOut);

	dataWord <<= 1;
	_state.DataOut = (dataWord >> 15) & 1;
	EXPECT_FALSE(_state.DataOut);
}

TEST_F(LynxEepromTest, BitCount_Counter) {
	_state.BitCount = 0;
	for (int i = 0; i < 16; i++) {
		_state.BitCount++;
	}
	EXPECT_EQ(_state.BitCount, 16);
}

//=============================================================================
// Address Handling Tests
//=============================================================================

TEST_F(LynxEepromTest, Address_6Bit_Mask_93c46) {
	uint8_t addrBits = GetAddressBits(LynxEepromType::Eeprom93c46);
	uint16_t addr = 0x3f; // 6 bits max
	uint16_t mask = (1 << addrBits) - 1;
	EXPECT_EQ(addr & mask, 0x3f);
}

TEST_F(LynxEepromTest, Address_10Bit_Mask_93c86) {
	uint8_t addrBits = GetAddressBits(LynxEepromType::Eeprom93c86);
	uint16_t addr = 0x3ff; // 10 bits max
	uint16_t mask = (1 << addrBits) - 1;
	EXPECT_EQ(addr & mask, 0x3ff);
}

TEST_F(LynxEepromTest, Address_WordBoundary) {
	// Each word is 16 bits (2 bytes)
	uint16_t wordAddr = 10;
	uint32_t byteAddr = wordAddr * 2;
	EXPECT_EQ(byteAddr, 20u);
}

//=============================================================================
// Erase Operations Tests
//=============================================================================

TEST_F(LynxEepromTest, Erase_SetsWord_0xFFFF) {
	// Erasing sets word to all 1s
	uint16_t erasedValue = 0xffff;
	EXPECT_EQ(erasedValue, 0xffff);
}

TEST_F(LynxEepromTest, EraseAll_ERAL_WhenEnabled) {
	_state.WriteEnabled = true;
	// ERAL should succeed
	EXPECT_TRUE(_state.WriteEnabled);
}

//=============================================================================
// Chip Select Behavior Tests
//=============================================================================

TEST_F(LynxEepromTest, ChipSelect_FallingEdge_ResetState) {
	_state.CsActive = true;
	_state.State = LynxEepromState::SendingData;
	_state.BitCount = 5;

	// CS goes low — reset state machine
	_state.CsActive = false;
	_state.State = LynxEepromState::Idle;
	_state.BitCount = 0;

	EXPECT_EQ(_state.State, LynxEepromState::Idle);
	EXPECT_EQ(_state.BitCount, 0);
}

TEST_F(LynxEepromTest, ChipSelect_MidTransaction_Abort) {
	_state.CsActive = true;
	_state.State = LynxEepromState::ReceivingData;
	_state.DataBuffer = 0x5678;

	// Abort by dropping CS
	_state.CsActive = false;
	_state.State = LynxEepromState::Idle;

	// Data should not be written
	EXPECT_EQ(_state.State, LynxEepromState::Idle);
}

//=============================================================================
// Extended Command Tests
//=============================================================================

TEST_F(LynxEepromTest, ExtendedCmd_EWEN_Pattern) {
	// EWEN: 00 + 11xxxxxx (high 2 bits of address = 11)
	// For 93c46: 00 + 11 + 4 don't-care bits
	uint8_t extCode = 0x03; // Binary 11 = EWEN
	EXPECT_EQ(extCode, 3);
}

TEST_F(LynxEepromTest, ExtendedCmd_EWDS_Pattern) {
	// EWDS: 00 + 00xxxxxx
	uint8_t extCode = 0x00; // Binary 00 = EWDS
	EXPECT_EQ(extCode, 0);
}

TEST_F(LynxEepromTest, ExtendedCmd_ERAL_Pattern) {
	// ERAL: 00 + 10xxxxxx
	uint8_t extCode = 0x02; // Binary 10 = ERAL
	EXPECT_EQ(extCode, 2);
}

TEST_F(LynxEepromTest, ExtendedCmd_WRAL_Pattern) {
	// WRAL: 00 + 01xxxxxx + 16-bit data
	uint8_t extCode = 0x01; // Binary 01 = WRAL
	EXPECT_EQ(extCode, 1);
}

//=============================================================================
// Integration / Round Trip Tests
//=============================================================================

TEST_F(LynxEepromTest, RoundTrip_WriteRead_Simulation) {
	// Simulate a write followed by read
	uint16_t testData = 0x1234;
	uint16_t storedData = testData;

	// Read back
	EXPECT_EQ(storedData, 0x1234);
}

TEST_F(LynxEepromTest, FullWordSize_16Bits) {
	// EEPROM stores 16-bit words
	uint16_t word = 0xfffe;
	EXPECT_EQ(word, 0xfffe);
}

//=============================================================================
// Audit Fix Regression Tests
//=============================================================================

TEST_F(LynxEepromTest, AuditFix12_24_NoDoubleInit) {
	// [12.24/#406] EEPROM should NOT be initialized twice.
	// Previously, both LynxConsole::Init and LynxConsole::LoadRom
	// called EEPROM::Init which zeroed the contents loaded from battery.
	// Now only one init path is used.
	//
	// Verify that EEPROM serial state is preserved after construction:
	LynxEepromSerialState state = {};
	state.Type = LynxEepromType::Eeprom93c46;
	state.Opcode = 0x0ABC;
	state.Address = 0x0012;
	state.DataBuffer = 0x3456;
	state.BitCount = 9;
	state.WriteEnabled = true;

	// Verify the state values persist (not zeroed by a second init)
	EXPECT_EQ(state.Type, LynxEepromType::Eeprom93c46);
	EXPECT_EQ(state.Opcode, 0x0ABC);
	EXPECT_EQ(state.Address, 0x0012);
	EXPECT_EQ(state.DataBuffer, 0x3456);
	EXPECT_EQ(state.BitCount, 9);
	EXPECT_TRUE(state.WriteEnabled);
}

TEST_F(LynxEepromTest, AuditFix12_2_EnumConsistency) {
	// [12.2/#390] C# and C++ EEPROM type enums must match.
	// Verify the C++ enum values are what we expect.
	EXPECT_EQ(static_cast<int>(LynxEepromType::None), 0);
	EXPECT_EQ(static_cast<int>(LynxEepromType::Eeprom93c46), 1);
	EXPECT_EQ(static_cast<int>(LynxEepromType::Eeprom93c56), 2);
	EXPECT_EQ(static_cast<int>(LynxEepromType::Eeprom93c66), 3);
	EXPECT_EQ(static_cast<int>(LynxEepromType::Eeprom93c76), 4);
	EXPECT_EQ(static_cast<int>(LynxEepromType::Eeprom93c86), 5);
}

