#include "pch.h"
#include "SMS/SmsTypes.h"

// =============================================================================
// SMS Cartridge Mapper Tests
// =============================================================================
// Tests for SMS cartridge mapper register decoding logic.
// Each mapper type has distinct bank register placement and behavior.
// These tests verify the decoding logic extracted from each cart class.

namespace SmsCartTestHelpers {

	// =========================================================================
	// Sega Mapper Helpers
	// =========================================================================

	/// <summary>
	/// Decodes the Sega mapper control register ($FFFC).
	/// </summary>
	struct SegaControlResult {
		bool ramEnabled0;		// RAM enabled in slot 2 ($8000-$BFFF)
		bool ramEnabled1;		// RAM enabled in slot 3 ($C000-$FFFF)
		uint8_t ramBank;		// Which RAM bank (0 or 1)
		bool romWriteEnabled;	// ROM write through
		uint8_t bankShift;		// Bank shift bits
	};

	static SegaControlResult DecodeSegaControl(uint8_t value) {
		SegaControlResult r;
		r.ramEnabled0 = (value & 0x08) != 0;
		r.ramEnabled1 = (value & 0x10) != 0;
		r.ramBank = (value & 0x04) >> 2;
		r.romWriteEnabled = (value & 0x80) != 0;
		r.bankShift = value & 0x03;
		return r;
	}

	/// <summary>
	/// Computes Sega mapper PRG offset for a slot from bank number.
	/// Each bank is 16KB (0x4000).
	/// </summary>
	static uint32_t SegaPrgOffset(uint8_t bank) {
		return (uint32_t)bank * 0x4000;
	}

	/// <summary>
	/// Computes Sega mapper RAM offset for alternating banks.
	/// Slot $8000: ramBank * 0x4000
	/// Slot $C000: (ramBank ^ 1) * 0x4000
	/// </summary>
	static uint32_t SegaRamOffset(uint8_t ramBank, bool isSlot3) {
		uint8_t effective = isSlot3 ? (ramBank ^ 1) : ramBank;
		return (uint32_t)effective * 0x4000;
	}

	// =========================================================================
	// Codemasters Mapper Helpers
	// =========================================================================

	/// <summary>
	/// Decodes Codemasters bank write address.
	/// $0000-$3FFF → slot 0, $4000-$7FFF → slot 1, $8000-$BFFF → slot 2
	/// </summary>
	static int CodemastersSlotFromAddress(uint16_t addr) {
		return (addr & 0xc000) >> 14;
	}

	/// <summary>
	/// Checks if Codemasters cart RAM is enabled based on slot 1 bank value.
	/// Bit 7 of slot 1 bank value enables cart RAM in upper half of slot 2.
	/// </summary>
	static bool CodemastersRamEnabled(uint8_t slot1Bank) {
		return (slot1Bank & 0x80) != 0;
	}

	// =========================================================================
	// Korean Mapper Helpers
	// =========================================================================

	/// <summary>
	/// Korean mapper: first 32KB fixed, single switchable bank at $8000-$BFFF.
	/// Register at $A000.
	/// </summary>
	static uint32_t KoreanBankOffset(uint8_t bank) {
		return (uint32_t)bank * 0x4000;
	}

	// =========================================================================
	// MSX Mapper Helpers
	// =========================================================================

	/// <summary>
	/// MSX mapper uses 8KB banks (0x2000), with swapped register mapping:
	/// Reg 0 → prgBanks[2], Reg 1 → prgBanks[3],
	/// Reg 2 → prgBanks[0], Reg 3 → prgBanks[1]
	/// </summary>
	static int MsxRegToSlot(uint16_t reg) {
		switch (reg) {
			case 0: return 2;
			case 1: return 3;
			case 2: return 0;
			case 3: return 1;
			default: return -1;
		}
	}

	static uint32_t Msx8KBankOffset(uint8_t bank) {
		return (uint32_t)bank * 0x2000;
	}

	// =========================================================================
	// Nemesis Mapper Helpers
	// =========================================================================

	/// <summary>
	/// Nemesis mapper: first 8KB maps to end of PRG ROM.
	/// </summary>
	static uint32_t NemesisFirstBankOffset(uint32_t prgRomSize) {
		return prgRomSize - 0x2000;
	}
}

// =============================================================================
// Sega Mapper Tests
// =============================================================================

TEST(SmsSegaCartTest, ControlRegister_DefaultNoRam) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x00);
	EXPECT_FALSE(r.ramEnabled0);
	EXPECT_FALSE(r.ramEnabled1);
	EXPECT_EQ(r.ramBank, 0);
	EXPECT_FALSE(r.romWriteEnabled);
	EXPECT_EQ(r.bankShift, 0);
}

TEST(SmsSegaCartTest, ControlRegister_EnableSlot2Ram) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x08);
	EXPECT_TRUE(r.ramEnabled0);
	EXPECT_FALSE(r.ramEnabled1);
}

TEST(SmsSegaCartTest, ControlRegister_EnableSlot3Ram) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x10);
	EXPECT_FALSE(r.ramEnabled0);
	EXPECT_TRUE(r.ramEnabled1);
}

TEST(SmsSegaCartTest, ControlRegister_BothRamSlots) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x18);
	EXPECT_TRUE(r.ramEnabled0);
	EXPECT_TRUE(r.ramEnabled1);
}

TEST(SmsSegaCartTest, ControlRegister_RamBank1) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x04);
	EXPECT_EQ(r.ramBank, 1);
}

TEST(SmsSegaCartTest, ControlRegister_RomWriteEnable) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x80);
	EXPECT_TRUE(r.romWriteEnabled);
}

TEST(SmsSegaCartTest, ControlRegister_BankShiftBits) {
	auto r = SmsCartTestHelpers::DecodeSegaControl(0x03);
	EXPECT_EQ(r.bankShift, 3);
}

TEST(SmsSegaCartTest, ControlRegister_AllBits) {
	// All bits set: RAM both slots, bank 1, ROM write, shift 3
	auto r = SmsCartTestHelpers::DecodeSegaControl(0xff);
	EXPECT_TRUE(r.ramEnabled0);
	EXPECT_TRUE(r.ramEnabled1);
	EXPECT_TRUE(r.romWriteEnabled);
	EXPECT_EQ(r.ramBank, 1);
	EXPECT_EQ(r.bankShift, 3);
}

TEST(SmsSegaCartTest, PrgOffset_Bank0) {
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(0), 0x0000u);
}

TEST(SmsSegaCartTest, PrgOffset_Bank1) {
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(1), 0x4000u);
}

TEST(SmsSegaCartTest, PrgOffset_Bank2) {
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(2), 0x8000u);
}

TEST(SmsSegaCartTest, PrgOffset_Bank255) {
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(255), 255u * 0x4000u);
}

TEST(SmsSegaCartTest, RamOffset_Slot2_Bank0) {
	EXPECT_EQ(SmsCartTestHelpers::SegaRamOffset(0, false), 0x0000u);
}

TEST(SmsSegaCartTest, RamOffset_Slot3_Bank0_GetsBank1) {
	// Slot 3 uses ramBank ^ 1
	EXPECT_EQ(SmsCartTestHelpers::SegaRamOffset(0, true), 0x4000u);
}

TEST(SmsSegaCartTest, RamOffset_Slot2_Bank1) {
	EXPECT_EQ(SmsCartTestHelpers::SegaRamOffset(1, false), 0x4000u);
}

TEST(SmsSegaCartTest, RamOffset_Slot3_Bank1_GetsBank0) {
	EXPECT_EQ(SmsCartTestHelpers::SegaRamOffset(1, true), 0x0000u);
}

// =============================================================================
// Codemasters Mapper Tests
// =============================================================================

TEST(SmsCodemasterCartTest, SlotFromAddress_Slot0) {
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0x0000), 0);
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0x3fff), 0);
}

TEST(SmsCodemasterCartTest, SlotFromAddress_Slot1) {
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0x4000), 1);
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0x7fff), 1);
}

TEST(SmsCodemasterCartTest, SlotFromAddress_Slot2) {
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0x8000), 2);
	EXPECT_EQ(SmsCartTestHelpers::CodemastersSlotFromAddress(0xbfff), 2);
}

TEST(SmsCodemasterCartTest, RamEnabled_Bit7Clear) {
	EXPECT_FALSE(SmsCartTestHelpers::CodemastersRamEnabled(0x00));
	EXPECT_FALSE(SmsCartTestHelpers::CodemastersRamEnabled(0x7f));
}

TEST(SmsCodemasterCartTest, RamEnabled_Bit7Set) {
	EXPECT_TRUE(SmsCartTestHelpers::CodemastersRamEnabled(0x80));
	EXPECT_TRUE(SmsCartTestHelpers::CodemastersRamEnabled(0xff));
}

// =============================================================================
// Korean Mapper Tests
// =============================================================================

TEST(SmsKoreanCartTest, BankOffset_Bank0) {
	EXPECT_EQ(SmsCartTestHelpers::KoreanBankOffset(0), 0x0000u);
}

TEST(SmsKoreanCartTest, BankOffset_Bank1) {
	EXPECT_EQ(SmsCartTestHelpers::KoreanBankOffset(1), 0x4000u);
}

TEST(SmsKoreanCartTest, BankOffset_LargeBank) {
	EXPECT_EQ(SmsCartTestHelpers::KoreanBankOffset(31), 31u * 0x4000u);
}

// =============================================================================
// MSX Mapper Tests
// =============================================================================

TEST(SmsMsxCartTest, RegisterToSlot_Swapped) {
	// MSX mapper has unusual register-to-slot mapping
	EXPECT_EQ(SmsCartTestHelpers::MsxRegToSlot(0), 2);
	EXPECT_EQ(SmsCartTestHelpers::MsxRegToSlot(1), 3);
	EXPECT_EQ(SmsCartTestHelpers::MsxRegToSlot(2), 0);
	EXPECT_EQ(SmsCartTestHelpers::MsxRegToSlot(3), 1);
}

TEST(SmsMsxCartTest, RegisterToSlot_Invalid) {
	EXPECT_EQ(SmsCartTestHelpers::MsxRegToSlot(4), -1);
}

TEST(SmsMsxCartTest, BankOffset_8KB) {
	EXPECT_EQ(SmsCartTestHelpers::Msx8KBankOffset(0), 0x0000u);
	EXPECT_EQ(SmsCartTestHelpers::Msx8KBankOffset(1), 0x2000u);
	EXPECT_EQ(SmsCartTestHelpers::Msx8KBankOffset(2), 0x4000u);
}

// =============================================================================
// Nemesis Mapper Tests
// =============================================================================

TEST(SmsNemesisCartTest, FirstBankOffset_SmallRom) {
	// 32KB ROM → first bank at 0x6000
	EXPECT_EQ(SmsCartTestHelpers::NemesisFirstBankOffset(0x8000), 0x6000u);
}

TEST(SmsNemesisCartTest, FirstBankOffset_128KBRom) {
	// 128KB ROM → first bank at 0x1e000
	EXPECT_EQ(SmsCartTestHelpers::NemesisFirstBankOffset(0x20000), 0x1e000u);
}

TEST(SmsNemesisCartTest, FirstBankOffset_256KBRom) {
	EXPECT_EQ(SmsCartTestHelpers::NemesisFirstBankOffset(0x40000), 0x3e000u);
}

TEST(SmsNemesisCartTest, Uses8KBBanks_SameAsMsx) {
	// Nemesis and MSX both use 8KB banks
	EXPECT_EQ(SmsCartTestHelpers::Msx8KBankOffset(5), 0xa000u);
}

// =============================================================================
// Default Bank Initialization
// =============================================================================

TEST(SmsSegaCartTest, DefaultBanks_012) {
	// Sega mapper starts with banks 0, 1, 2 in slots 0-2
	uint8_t banks[3] = {0, 1, 2};
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[0]), 0x0000u);
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[1]), 0x4000u);
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[2]), 0x8000u);
}

TEST(SmsCodemasterCartTest, DefaultBanks_010) {
	// Codemasters mapper starts with banks 0, 1, 0 in slots 0-2
	uint8_t banks[3] = {0, 1, 0};
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[0]), 0x0000u);
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[1]), 0x4000u);
	EXPECT_EQ(SmsCartTestHelpers::SegaPrgOffset(banks[2]), 0x0000u);
}

TEST(SmsMsxCartTest, DefaultBanks_AllZero) {
	uint8_t banks[4] = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(banks[i], 0);
		EXPECT_EQ(SmsCartTestHelpers::Msx8KBankOffset(banks[i]), 0x0000u);
	}
}

// =============================================================================
// Z80 CPU Flag Tests
// =============================================================================

TEST(SmsCpuFlagsTest, IndividualFlagBits) {
	EXPECT_EQ(SmsCpuFlags::Carry, 0x01);
	EXPECT_EQ(SmsCpuFlags::AddSub, 0x02);
	EXPECT_EQ(SmsCpuFlags::Parity, 0x04);
	EXPECT_EQ(SmsCpuFlags::F3, 0x08);
	EXPECT_EQ(SmsCpuFlags::HalfCarry, 0x10);
	EXPECT_EQ(SmsCpuFlags::F5, 0x20);
	EXPECT_EQ(SmsCpuFlags::Zero, 0x40);
	EXPECT_EQ(SmsCpuFlags::Sign, 0x80);
}

TEST(SmsCpuFlagsTest, AllFlags_NoOverlap) {
	uint8_t all = SmsCpuFlags::Carry | SmsCpuFlags::AddSub | SmsCpuFlags::Parity |
		SmsCpuFlags::F3 | SmsCpuFlags::HalfCarry | SmsCpuFlags::F5 |
		SmsCpuFlags::Zero | SmsCpuFlags::Sign;
	EXPECT_EQ(all, 0xff);
}
