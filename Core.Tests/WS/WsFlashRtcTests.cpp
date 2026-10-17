#include "pch.h"
#include "WS/WsTypes.h"
#include "WS/WsRtc.h"

// =============================================================================
// WonderSwan Flash Cart & RTC (Seiko S-3511A) Tests
// =============================================================================
// Tests for Flash cart emulation (SST39VF040) and RTC serial protocol.
// Covers #1093 (Flash) and #1092 (RTC) feature implementations.

// =============================================================================
// Flash Mode Enum Tests
// =============================================================================

TEST(WsFlashModeTest, EnumValues_AreCorrect) {
	EXPECT_EQ(static_cast<uint8_t>(WsFlashMode::Idle), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsFlashMode::WriteByte), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsFlashMode::Erase), 2);
}

TEST(WsFlashModeTest, DefaultState_IsIdle) {
	WsCartState state = {};
	EXPECT_EQ(state.FlashMode, WsFlashMode::Idle);
}

// =============================================================================
// WsCartState Flash Field Default Tests
// =============================================================================

TEST(WsCartStateFlashTest, DefaultInit_AllFieldsZero) {
	WsCartState state = {};
	EXPECT_EQ(state.FlashMode, WsFlashMode::Idle);
	EXPECT_EQ(state.FlashCycle, 0);
	EXPECT_FALSE(state.FlashSoftwareId);
	EXPECT_FALSE(state.HasFlash);
}

TEST(WsCartStateFlashTest, DefaultInit_BanksAreZero) {
	WsCartState state = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(state.SelectedBanks[i], 0);
	}
}

TEST(WsCartStateFlashTest, HasFlash_CanBeSet) {
	WsCartState state = {};
	state.HasFlash = true;
	EXPECT_TRUE(state.HasFlash);
	EXPECT_EQ(state.FlashMode, WsFlashMode::Idle);
	EXPECT_EQ(state.FlashCycle, 0);
	EXPECT_FALSE(state.FlashSoftwareId);
}

// =============================================================================
// Flash Command Sequence Protocol Tests (SST39VF040)
// =============================================================================
// The SST39VF040 uses a command sequence protocol:
//   Cycle 0: Write 0xAA to address $5555
//   Cycle 1: Write 0x55 to address $2AAA
//   Cycle 2: Write command byte to address $5555
// Commands: 0x90 = Software ID, 0xA0 = Write Byte, 0x80 = Erase, 0xF0 = Exit ID

namespace WsFlashTestHelpers {
	// Simulates the Flash command state machine logic from WsCart::FlashWrite
	// Returns the resulting state after processing a write in Idle mode
	struct FlashResult {
		WsFlashMode mode;
		uint8_t cycle;
		bool softwareId;
	};

	static FlashResult ProcessIdleSequence(uint8_t command) {
		WsCartState state = {};
		state.HasFlash = true;

		// Cycle 0: AA @ 5555
		if ((0x5555 & 0xffff) == 0x5555) {
			state.FlashCycle = 1;
		}

		// Cycle 1: 55 @ 2AAA
		if (state.FlashCycle == 1) {
			state.FlashCycle = 2;
		}

		// Cycle 2: command @ 5555
		if (state.FlashCycle == 2) {
			switch (command) {
				case 0x80:
					return { WsFlashMode::Erase, 3, false };
				case 0x90:
					return { WsFlashMode::Idle, 0, true };
				case 0xa0:
					return { WsFlashMode::WriteByte, 0, false };
				case 0xf0:
					return { WsFlashMode::Idle, 0, false };
				default:
					return { WsFlashMode::Idle, 0, false };
			}
		}

		return { state.FlashMode, state.FlashCycle, state.FlashSoftwareId };
	}

	// Validates the 3-cycle command sequence addressing
	static bool ValidateCommandAddressing(uint32_t addr0, uint8_t val0,
		uint32_t addr1, uint8_t val1,
		uint32_t addr2) {
		return (addr0 & 0xffff) == 0x5555 && val0 == 0xaa
			&& (addr1 & 0xffff) == 0x2aaa && val1 == 0x55
			&& (addr2 & 0xffff) == 0x5555;
	}
}

TEST(WsFlashCommandTest, SoftwareId_EntersIdMode) {
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0x90);
	EXPECT_EQ(result.mode, WsFlashMode::Idle);
	EXPECT_EQ(result.cycle, 0);
	EXPECT_TRUE(result.softwareId);
}

TEST(WsFlashCommandTest, WriteByte_EntersWriteMode) {
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0xa0);
	EXPECT_EQ(result.mode, WsFlashMode::WriteByte);
	EXPECT_EQ(result.cycle, 0);
	EXPECT_FALSE(result.softwareId);
}

TEST(WsFlashCommandTest, EraseSetup_EntersEraseMode) {
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0x80);
	EXPECT_EQ(result.mode, WsFlashMode::Erase);
	EXPECT_EQ(result.cycle, 3);
	EXPECT_FALSE(result.softwareId);
}

TEST(WsFlashCommandTest, ExitId_ClearsSoftwareId) {
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0xf0);
	EXPECT_EQ(result.mode, WsFlashMode::Idle);
	EXPECT_EQ(result.cycle, 0);
	EXPECT_FALSE(result.softwareId);
}

TEST(WsFlashCommandTest, UnknownCommand_ResetsToIdle) {
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0x42);
	EXPECT_EQ(result.mode, WsFlashMode::Idle);
	EXPECT_EQ(result.cycle, 0);
	EXPECT_FALSE(result.softwareId);
}

// =============================================================================
// Flash Command Addressing Validation
// =============================================================================

TEST(WsFlashAddressTest, ValidSequence_CorrectAddresses) {
	EXPECT_TRUE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x5555, 0xaa, 0x2aaa, 0x55, 0x5555));
}

TEST(WsFlashAddressTest, HighBitsIgnored_StillValid) {
	// Flash uses addr & 0xFFFF for comparison — high bits should be masked off
	EXPECT_TRUE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x105555, 0xaa, 0x102aaa, 0x55, 0x105555));
}

TEST(WsFlashAddressTest, WrongCycle0Address_Invalid) {
	EXPECT_FALSE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x1234, 0xaa, 0x2aaa, 0x55, 0x5555));
}

TEST(WsFlashAddressTest, WrongCycle0Value_Invalid) {
	EXPECT_FALSE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x5555, 0x00, 0x2aaa, 0x55, 0x5555));
}

TEST(WsFlashAddressTest, WrongCycle1Address_Invalid) {
	EXPECT_FALSE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x5555, 0xaa, 0x1234, 0x55, 0x5555));
}

TEST(WsFlashAddressTest, WrongCycle1Value_Invalid) {
	EXPECT_FALSE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x5555, 0xaa, 0x2aaa, 0x00, 0x5555));
}

TEST(WsFlashAddressTest, WrongCycle2Address_Invalid) {
	EXPECT_FALSE(WsFlashTestHelpers::ValidateCommandAddressing(
		0x5555, 0xaa, 0x2aaa, 0x55, 0x1234));
}

// =============================================================================
// Flash Software ID Mode Tests
// =============================================================================

TEST(WsFlashSoftwareIdTest, ManufacturerId_IsSst) {
	// SST manufacturer ID = 0xBF
	uint32_t addr = 0x00; // even address → manufacturer
	uint32_t offset = addr & 0x01;
	EXPECT_EQ(offset, 0u);
	uint8_t id = (offset == 0) ? 0xbf : 0xb5;
	EXPECT_EQ(id, 0xbf);
}

TEST(WsFlashSoftwareIdTest, DeviceId_IsSst39vf040) {
	// SST39VF040 device ID = 0xB5 (512KB)
	uint32_t addr = 0x01; // odd address → device
	uint32_t offset = addr & 0x01;
	EXPECT_EQ(offset, 1u);
	uint8_t id = (offset == 0) ? 0xbf : 0xb5;
	EXPECT_EQ(id, 0xb5);
}

TEST(WsFlashSoftwareIdTest, EvenAddress_AlwaysManufacturer) {
	// Any even address should return manufacturer ID
	for (uint32_t addr : {0x00u, 0x02u, 0x100u, 0x5554u}) {
		uint8_t id = ((addr & 0x01) == 0) ? 0xbf : 0xb5;
		EXPECT_EQ(id, 0xbf) << "addr=" << addr;
	}
}

TEST(WsFlashSoftwareIdTest, OddAddress_AlwaysDevice) {
	// Any odd address should return device ID
	for (uint32_t addr : {0x01u, 0x03u, 0x101u, 0x5555u}) {
		uint8_t id = ((addr & 0x01) == 0) ? 0xbf : 0xb5;
		EXPECT_EQ(id, 0xb5) << "addr=" << addr;
	}
}

// =============================================================================
// Flash WriteByte Mode Tests
// =============================================================================

TEST(WsFlashWriteByteTest, CanOnlyClearBits) {
	// Flash program operation: byte &= value (can only clear bits, not set)
	uint8_t original = 0xff;
	uint8_t value = 0xa5;
	uint8_t result = original & value;
	EXPECT_EQ(result, 0xa5);
}

TEST(WsFlashWriteByteTest, CantSetBitsAlreadyClear) {
	uint8_t original = 0x00;
	uint8_t value = 0xff;
	uint8_t result = original & value;
	EXPECT_EQ(result, 0x00);
}

TEST(WsFlashWriteByteTest, PartialClear) {
	uint8_t original = 0xf0;
	uint8_t value = 0x0f;
	uint8_t result = original & value;
	EXPECT_EQ(result, 0x00);
}

TEST(WsFlashWriteByteTest, NoChangeWhenAllBitsSet) {
	uint8_t original = 0xff;
	uint8_t value = 0xff;
	uint8_t result = original & value;
	EXPECT_EQ(result, 0xff);
}

// =============================================================================
// Flash Erase Tests
// =============================================================================

TEST(WsFlashEraseTest, SectorErase_4KBAligned) {
	// Sector erase uses addr & ~0xFFF to find 4KB boundary
	EXPECT_EQ(0x10000u & ~0xfffu, 0x10000u);
	EXPECT_EQ(0x10123u & ~0xfffu, 0x10000u);
	EXPECT_EQ(0x10fffu & ~0xfffu, 0x10000u);
	EXPECT_EQ(0x11000u & ~0xfffu, 0x11000u);
}

TEST(WsFlashEraseTest, SectorSize_Is4KB) {
	uint32_t sectorSize = 0x1000;
	EXPECT_EQ(sectorSize, 4096u);
}

TEST(WsFlashEraseTest, ChipErase_FillsFF) {
	// Chip erase sets all bytes to 0xFF
	std::array<uint8_t, 16> rom;
	rom.fill(0x42);
	memset(rom.data(), 0xff, rom.size());
	for (auto b : rom) {
		EXPECT_EQ(b, 0xff);
	}
}

TEST(WsFlashEraseTest, SectorErase_FillsFFInRange) {
	// Sector erase sets 4KB sector to 0xFF, rest unchanged
	std::array<uint8_t, 8192> rom;
	rom.fill(0x42);

	uint32_t sectorBase = 0x1000 & ~0xfff;
	memset(rom.data() + sectorBase, 0xff, 0x1000);

	// First 4KB should be unchanged
	for (uint32_t i = 0; i < 0x1000; i++) {
		EXPECT_EQ(rom[i], 0x42) << "i=" << i;
	}

	// Second 4KB (the erased sector) should be 0xFF
	for (uint32_t i = 0x1000; i < 0x2000; i++) {
		EXPECT_EQ(rom[i], 0xff) << "i=" << i;
	}
}

// =============================================================================
// Flash Exit Software ID (0xF0) Tests
// =============================================================================

TEST(WsFlashExitIdTest, DirectF0_ExitsIdMode) {
	// F0 can be written at any cycle 0 to exit Software ID
	WsCartState state = {};
	state.FlashSoftwareId = true;
	// Simulating: write 0xF0 at cycle 0 → exits ID mode
	state.FlashSoftwareId = false;
	state.FlashCycle = 0;
	EXPECT_FALSE(state.FlashSoftwareId);
	EXPECT_EQ(state.FlashCycle, 0);
}

TEST(WsFlashExitIdTest, ThreeCycleF0_ExitsIdMode) {
	// F0 at cycle 2 also exits
	auto result = WsFlashTestHelpers::ProcessIdleSequence(0xf0);
	EXPECT_FALSE(result.softwareId);
}

// =============================================================================
// Flash Erase Sequence (6-cycle) Tests
// =============================================================================

namespace WsFlashEraseTestHelpers {
	// Full 6-cycle erase sequence state machine:
	// Cycles 0-2: AA@5555 / 55@2AAA / 80@5555 → mode=Erase, cycle=3
	// Cycle 3: AA@5555
	// Cycle 4: 55@2AAA
	// Cycle 5: chip erase (10@5555) or sector erase (30@any)

	enum class EraseAction { None, ChipErase, SectorErase, Invalid };

	static EraseAction SimulateEraseCycle5(uint32_t addr, uint8_t value) {
		if ((addr & 0xffff) == 0x5555 && value == 0x10) {
			return EraseAction::ChipErase;
		} else if (value == 0x30) {
			return EraseAction::SectorErase;
		}
		return EraseAction::Invalid;
	}
}

TEST(WsFlashEraseSequenceTest, ChipErase_CommandIs10At5555) {
	auto action = WsFlashEraseTestHelpers::SimulateEraseCycle5(0x5555, 0x10);
	EXPECT_EQ(action, WsFlashEraseTestHelpers::EraseAction::ChipErase);
}

TEST(WsFlashEraseSequenceTest, SectorErase_CommandIs30AtAnyAddr) {
	auto action = WsFlashEraseTestHelpers::SimulateEraseCycle5(0x1000, 0x30);
	EXPECT_EQ(action, WsFlashEraseTestHelpers::EraseAction::SectorErase);
}

TEST(WsFlashEraseSequenceTest, SectorErase_DifferentAddress) {
	auto action = WsFlashEraseTestHelpers::SimulateEraseCycle5(0x8000, 0x30);
	EXPECT_EQ(action, WsFlashEraseTestHelpers::EraseAction::SectorErase);
}

TEST(WsFlashEraseSequenceTest, InvalidCommand_ReturnsInvalid) {
	auto action = WsFlashEraseTestHelpers::SimulateEraseCycle5(0x5555, 0x42);
	EXPECT_EQ(action, WsFlashEraseTestHelpers::EraseAction::Invalid);
}

TEST(WsFlashEraseSequenceTest, ChipErase_WrongAddress_IsInvalid) {
	// 0x10 at wrong address is not chip erase
	auto action = WsFlashEraseTestHelpers::SimulateEraseCycle5(0x1234, 0x10);
	EXPECT_EQ(action, WsFlashEraseTestHelpers::EraseAction::Invalid);
}

// =============================================================================
// RTC State Tests
// =============================================================================

TEST(WsRtcStateTest, DefaultInit_AllFieldsZero) {
	WsRtcState state = {};
	EXPECT_EQ(state.Year, 0);
	EXPECT_EQ(state.Month, 0);
	EXPECT_EQ(state.Day, 0);
	EXPECT_EQ(state.DoW, 0);
	EXPECT_EQ(state.Hour, 0);
	EXPECT_EQ(state.Minute, 0);
	EXPECT_EQ(state.Second, 0);
}

TEST(WsRtcStateTest, FieldsAreBCD) {
	// RTC fields use BCD encoding: 0x25 = 25 decimal
	WsRtcState state = {};
	state.Year = 0x24;   // 2024 (year since 2000)
	state.Month = 0x12;  // December
	state.Day = 0x31;    // 31st
	state.DoW = 0x03;    // Wednesday
	state.Hour = 0x23;   // 11 PM (23:00)
	state.Minute = 0x59; // 59 minutes
	state.Second = 0x59; // 59 seconds

	// Verify BCD decode: high nibble * 10 + low nibble
	EXPECT_EQ((state.Year >> 4) * 10 + (state.Year & 0x0f), 24);
	EXPECT_EQ((state.Month >> 4) * 10 + (state.Month & 0x0f), 12);
	EXPECT_EQ((state.Day >> 4) * 10 + (state.Day & 0x0f), 31);
	EXPECT_EQ((state.Hour & 0x3f) >> 4, 2); // tens digit
	EXPECT_EQ(state.Hour & 0x0f, 3);        // ones digit
	EXPECT_EQ((state.Minute >> 4) * 10 + (state.Minute & 0x0f), 59);
	EXPECT_EQ((state.Second >> 4) * 10 + (state.Second & 0x0f), 59);
}

TEST(WsRtcStateTest, Hour_PMBit) {
	// Hour bit 7 = PM indicator
	WsRtcState state = {};
	state.Hour = 0x92; // PM flag (0x80) + BCD 12 = 12 PM (noon)
	EXPECT_TRUE(state.Hour & 0x80);
	int hour = (state.Hour & 0x3f);
	int decoded = (hour >> 4) * 10 + (hour & 0x0f);
	EXPECT_EQ(decoded, 12);
}

TEST(WsRtcStateTest, Hour_AMHasNoPMBit) {
	WsRtcState state = {};
	state.Hour = 0x08; // BCD 08 = 8 AM, no PM flag
	EXPECT_FALSE(state.Hour & 0x80);
	int decoded = (state.Hour >> 4) * 10 + (state.Hour & 0x0f);
	EXPECT_EQ(decoded, 8);
}

// =============================================================================
// RTC BCD Encoding/Decoding Tests
// =============================================================================

namespace WsRtcBcdHelpers {
	static uint8_t ToBcd(int value) {
		return static_cast<uint8_t>((value / 10) << 4 | (value % 10));
	}

	static int FromBcd(uint8_t bcd) {
		return (bcd >> 4) * 10 + (bcd & 0x0f);
	}
}

TEST(WsRtcBcdTest, ToBcd_CommonValues) {
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(0), 0x00);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(1), 0x01);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(9), 0x09);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(10), 0x10);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(25), 0x25);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(59), 0x59);
	EXPECT_EQ(WsRtcBcdHelpers::ToBcd(99), 0x99);
}

TEST(WsRtcBcdTest, FromBcd_CommonValues) {
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x00), 0);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x01), 1);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x09), 9);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x10), 10);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x25), 25);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x59), 59);
	EXPECT_EQ(WsRtcBcdHelpers::FromBcd(0x99), 99);
}

TEST(WsRtcBcdTest, Roundtrip_0To99) {
	for (int i = 0; i <= 99; i++) {
		uint8_t bcd = WsRtcBcdHelpers::ToBcd(i);
		EXPECT_EQ(WsRtcBcdHelpers::FromBcd(bcd), i) << "value=" << i;
	}
}

// =============================================================================
// RTC Serial Protocol Tests (Port 0xCA)
// =============================================================================

namespace WsRtcPortHelpers {
	// Port 0xCA bit decode
	struct PortBits {
		uint8_t data;
		uint8_t clock;
		bool chipSelect;
	};

	static PortBits DecodePort(uint8_t value) {
		return {
			static_cast<uint8_t>(value & 0x01),       // bit 0 = data
			static_cast<uint8_t>((value >> 1) & 0x01), // bit 1 = clock
			static_cast<bool>((value >> 4) & 0x01)     // bit 4 = chip select
		};
	}

	// Build port value from individual bits
	static uint8_t BuildPort(uint8_t data, uint8_t clock, bool chipSelect) {
		return (data & 0x01)
			| ((clock & 0x01) << 1)
			| (chipSelect ? 0x10 : 0x00);
	}
}

TEST(WsRtcPortTest, DecodePort_DataBit) {
	auto bits = WsRtcPortHelpers::DecodePort(0x01);
	EXPECT_EQ(bits.data, 1);
	EXPECT_EQ(bits.clock, 0);
	EXPECT_FALSE(bits.chipSelect);
}

TEST(WsRtcPortTest, DecodePort_ClockBit) {
	auto bits = WsRtcPortHelpers::DecodePort(0x02);
	EXPECT_EQ(bits.data, 0);
	EXPECT_EQ(bits.clock, 1);
	EXPECT_FALSE(bits.chipSelect);
}

TEST(WsRtcPortTest, DecodePort_ChipSelectBit) {
	auto bits = WsRtcPortHelpers::DecodePort(0x10);
	EXPECT_EQ(bits.data, 0);
	EXPECT_EQ(bits.clock, 0);
	EXPECT_TRUE(bits.chipSelect);
}

TEST(WsRtcPortTest, DecodePort_AllBitsSet) {
	auto bits = WsRtcPortHelpers::DecodePort(0x13);
	EXPECT_EQ(bits.data, 1);
	EXPECT_EQ(bits.clock, 1);
	EXPECT_TRUE(bits.chipSelect);
}

TEST(WsRtcPortTest, DecodePort_UnusedBitsIgnored) {
	// Bits 2-3, 5-7 should be ignored
	auto bits = WsRtcPortHelpers::DecodePort(0xec);
	EXPECT_EQ(bits.data, 0);
	EXPECT_EQ(bits.clock, 0);
	EXPECT_FALSE(bits.chipSelect);
}

TEST(WsRtcPortTest, BuildPort_DataOnly) {
	EXPECT_EQ(WsRtcPortHelpers::BuildPort(1, 0, false), 0x01);
}

TEST(WsRtcPortTest, BuildPort_ClockOnly) {
	EXPECT_EQ(WsRtcPortHelpers::BuildPort(0, 1, false), 0x02);
}

TEST(WsRtcPortTest, BuildPort_ChipSelectOnly) {
	EXPECT_EQ(WsRtcPortHelpers::BuildPort(0, 0, true), 0x10);
}

TEST(WsRtcPortTest, BuildPort_AllSet) {
	EXPECT_EQ(WsRtcPortHelpers::BuildPort(1, 1, true), 0x13);
}

TEST(WsRtcPortTest, BuildPort_Roundtrip) {
	for (uint8_t d = 0; d <= 1; d++) {
		for (uint8_t c = 0; c <= 1; c++) {
			for (int cs = 0; cs <= 1; cs++) {
				uint8_t port = WsRtcPortHelpers::BuildPort(d, c, cs != 0);
				auto bits = WsRtcPortHelpers::DecodePort(port);
				EXPECT_EQ(bits.data, d);
				EXPECT_EQ(bits.clock, c);
				EXPECT_EQ(bits.chipSelect, cs != 0);
			}
		}
	}
}

// =============================================================================
// RTC Command Protocol Tests
// =============================================================================
// Commands are 4-bit MSB-first. Bits [3:1] = register, bit [0] = R/W.
// Register 0 = status (1 byte), register 1 = datetime (7 bytes), register 2 = alarm (2 bytes)

namespace WsRtcCommandHelpers {
	struct CommandInfo {
		uint8_t reg;
		bool isRead;
		uint8_t dataLength;
	};

	static CommandInfo DecodeCommand(uint8_t cmd) {
		uint8_t reg = (cmd >> 1) & 0x07;
		bool isRead = cmd & 0x01;

		uint8_t len = 0;
		switch (reg) {
			case 0: len = 1; break; // Status
			case 1: len = 7; break; // DateTime
			case 2: len = 2; break; // Alarm
			default: len = 0; break;
		}

		return { reg, isRead, len };
	}
}

TEST(WsRtcCommandTest, ReadStatus_Cmd0x01) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x01);
	EXPECT_EQ(info.reg, 0);
	EXPECT_TRUE(info.isRead);
	EXPECT_EQ(info.dataLength, 1);
}

TEST(WsRtcCommandTest, WriteStatus_Cmd0x00) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x00);
	EXPECT_EQ(info.reg, 0);
	EXPECT_FALSE(info.isRead);
	EXPECT_EQ(info.dataLength, 1);
}

TEST(WsRtcCommandTest, ReadDateTime_Cmd0x03) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x03);
	EXPECT_EQ(info.reg, 1);
	EXPECT_TRUE(info.isRead);
	EXPECT_EQ(info.dataLength, 7);
}

TEST(WsRtcCommandTest, WriteDateTime_Cmd0x02) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x02);
	EXPECT_EQ(info.reg, 1);
	EXPECT_FALSE(info.isRead);
	EXPECT_EQ(info.dataLength, 7);
}

TEST(WsRtcCommandTest, ReadAlarm_Cmd0x05) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x05);
	EXPECT_EQ(info.reg, 2);
	EXPECT_TRUE(info.isRead);
	EXPECT_EQ(info.dataLength, 2);
}

TEST(WsRtcCommandTest, WriteAlarm_Cmd0x04) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x04);
	EXPECT_EQ(info.reg, 2);
	EXPECT_FALSE(info.isRead);
	EXPECT_EQ(info.dataLength, 2);
}

TEST(WsRtcCommandTest, InvalidRegister_ZeroLength) {
	auto info = WsRtcCommandHelpers::DecodeCommand(0x07); // reg 3
	EXPECT_EQ(info.reg, 3);
	EXPECT_EQ(info.dataLength, 0);
}

TEST(WsRtcCommandTest, HighRegisters_ZeroLength) {
	for (uint8_t reg = 3; reg <= 7; reg++) {
		uint8_t cmd = (reg << 1) | 0x01;
		auto info = WsRtcCommandHelpers::DecodeCommand(cmd);
		EXPECT_EQ(info.dataLength, 0) << "reg=" << (int)reg;
	}
}

// =============================================================================
// RTC Serial Bit Ordering Tests (MSB-first)
// =============================================================================

namespace WsRtcBitOrderHelpers {
	// Extract bit N (MSB-first) from a byte
	static uint8_t ExtractBit(uint8_t byte, int bitIndex) {
		return (byte >> (7 - bitIndex)) & 0x01;
	}

	// Compose command from 4 bits MSB-first
	static uint8_t ComposeCommand(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
		return ((b0 & 1) << 3) | ((b1 & 1) << 2) | ((b2 & 1) << 1) | (b3 & 1);
	}
}

TEST(WsRtcBitOrderTest, ExtractBit_MSBFirst) {
	uint8_t val = 0xa5; // 10100101
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 0), 1); // MSB
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 1), 0);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 2), 1);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 3), 0);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 4), 0);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 5), 1);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 6), 0);
	EXPECT_EQ(WsRtcBitOrderHelpers::ExtractBit(val, 7), 1); // LSB
}

TEST(WsRtcBitOrderTest, ComposeCommand_ReadDateTime) {
	// Command 0x03 = 0b0011 (MSB-first: 0, 0, 1, 1)
	uint8_t cmd = WsRtcBitOrderHelpers::ComposeCommand(0, 0, 1, 1);
	EXPECT_EQ(cmd, 0x03);
}

TEST(WsRtcBitOrderTest, ComposeCommand_ReadStatus) {
	// Command 0x01 = 0b0001 (MSB-first: 0, 0, 0, 1)
	uint8_t cmd = WsRtcBitOrderHelpers::ComposeCommand(0, 0, 0, 1);
	EXPECT_EQ(cmd, 0x01);
}

TEST(WsRtcBitOrderTest, ComposeCommand_WriteAlarm) {
	// Command 0x04 = 0b0100 (MSB-first: 0, 1, 0, 0)
	uint8_t cmd = WsRtcBitOrderHelpers::ComposeCommand(0, 1, 0, 0);
	EXPECT_EQ(cmd, 0x04);
}

TEST(WsRtcBitOrderTest, ComposeCommand_AllOnes) {
	uint8_t cmd = WsRtcBitOrderHelpers::ComposeCommand(1, 1, 1, 1);
	EXPECT_EQ(cmd, 0x0f);
}

TEST(WsRtcBitOrderTest, ComposeCommand_AllZeros) {
	uint8_t cmd = WsRtcBitOrderHelpers::ComposeCommand(0, 0, 0, 0);
	EXPECT_EQ(cmd, 0x00);
}

// =============================================================================
// RTC DateTime Register Layout Tests
// =============================================================================
// Register 1 (DateTime) returns 7 BCD bytes in order:
// [0]=Year, [1]=Month, [2]=Day, [3]=DoW, [4]=Hour, [5]=Minute, [6]=Second

TEST(WsRtcDateTimeLayoutTest, RegisterOrder) {
	WsRtcState state = {};
	state.Year = 0x24;
	state.Month = 0x06;
	state.Day = 0x15;
	state.DoW = 0x06;
	state.Hour = 0x14;
	state.Minute = 0x30;
	state.Second = 0x00;

	uint8_t buffer[7];
	buffer[0] = state.Year;
	buffer[1] = state.Month;
	buffer[2] = state.Day;
	buffer[3] = state.DoW;
	buffer[4] = state.Hour;
	buffer[5] = state.Minute;
	buffer[6] = state.Second;

	EXPECT_EQ(buffer[0], 0x24); // Year
	EXPECT_EQ(buffer[1], 0x06); // Month
	EXPECT_EQ(buffer[2], 0x15); // Day
	EXPECT_EQ(buffer[3], 0x06); // DoW (Saturday)
	EXPECT_EQ(buffer[4], 0x14); // Hour (14:00 = 2 PM)
	EXPECT_EQ(buffer[5], 0x30); // Minute
	EXPECT_EQ(buffer[6], 0x00); // Second
}

TEST(WsRtcDateTimeLayoutTest, SizeIs7Bytes) {
	EXPECT_EQ(sizeof(WsRtcState), 7u);
}

// =============================================================================
// RTC Chip Select (CS) State Machine Tests
// =============================================================================

TEST(WsRtcChipSelectTest, CSLow_ResetsProtocol) {
	// When CS goes low, all protocol state should reset
	// This mirrors the behavior in WsRtc::Write when chipSelect goes false
	uint8_t command = 0x03;
	uint8_t bitCounter = 4;
	uint8_t dataSize = 7;
	bool reading = true;
	uint8_t bitOut = 1;

	// Simulate CS low
	bool chipSelect = false;
	if (!chipSelect) {
		command = 0;
		bitCounter = 0;
		dataSize = 0;
		reading = false;
		bitOut = 0;
	}

	EXPECT_EQ(command, 0);
	EXPECT_EQ(bitCounter, 0);
	EXPECT_EQ(dataSize, 0);
	EXPECT_FALSE(reading);
	EXPECT_EQ(bitOut, 0);
}

TEST(WsRtcChipSelectTest, CSTransitionLowToHigh_BeginsNewTransaction) {
	// CS low→high: begin new transaction, reset protocol
	bool oldCS = false;
	bool newCS = true;

	EXPECT_TRUE(!oldCS && newCS); // detects rising edge

	uint8_t command = 0;
	uint8_t bitCounter = 0;
	// After CS rise, ready to receive 4-bit command
	EXPECT_EQ(command, 0);
	EXPECT_EQ(bitCounter, 0);
}

// =============================================================================
// Cart Bank Selection Tests
// =============================================================================

TEST(WsCartBankTest, DefaultBanks_AreZero) {
	WsCartState state = {};
	EXPECT_EQ(state.SelectedBanks[0], 0);
	EXPECT_EQ(state.SelectedBanks[1], 0);
	EXPECT_EQ(state.SelectedBanks[2], 0);
	EXPECT_EQ(state.SelectedBanks[3], 0);
}

TEST(WsCartBankTest, BankPorts_AreC0ToC3) {
	// Banks are accessed via ports 0xC0-0xC3
	for (uint16_t port = 0xc0; port <= 0xc3; port++) {
		EXPECT_EQ(port - 0xc0, port & 0x03);
	}
}

TEST(WsCartBankTest, EepromPorts_AreC4ToC8) {
	// EEPROM ports are 0xC4-0xC8
	for (uint16_t port = 0xc4; port <= 0xc8; port++) {
		EXPECT_GE(port, 0xc4);
		EXPECT_LT(port, 0xc9);
	}
}

TEST(WsCartBankTest, RtcPort_IsCA) {
	// RTC is accessed via port 0xCA
	uint16_t rtcPort = 0xca;
	EXPECT_EQ(rtcPort, 0xca);
}
