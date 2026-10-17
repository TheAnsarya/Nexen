#include "pch.h"

/// <summary>
/// Tests for NES, SNES, and Game Boy memory mapping logic.
/// These tests verify address calculations, mirroring, and bank switching
/// without requiring a full emulator environment.
/// </summary>

//=============================================================================
// NES Memory Map Tests
//=============================================================================
// NES Memory Layout:
// $0000-$07FF: Internal RAM (2KB)
// $0800-$1FFF: Mirrors of $0000-$07FF (3 times)
// $2000-$2007: PPU Registers
// $2008-$3FFF: Mirrors of $2000-$2007 (every 8 bytes)
// $4000-$4017: APU and I/O Registers
// $4018-$401F: APU and I/O (normally disabled)
// $4020-$FFFF: Cartridge space (PRG ROM, PRG RAM, mappers)

class NesMemoryMapTest : public ::testing::Test {
protected:
	// Mirror address in internal RAM range
	static uint16_t MirrorInternalRam(uint16_t addr) {
		if (addr < 0x2000) {
			return addr & 0x07FF;  // Mask to 2KB
		}
		return addr;
	}

	// Mirror address in PPU register range
	static uint16_t MirrorPpuRegisters(uint16_t addr) {
		if (addr >= 0x2000 && addr < 0x4000) {
			return 0x2000 + (addr & 0x0007);  // Map to $2000-$2007
		}
		return addr;
	}

	// Check if address is in cartridge space
	static bool IsCartridgeSpace(uint16_t addr) {
		return addr >= 0x4020;
	}

	// Calculate PRG ROM offset for simple mappers (NROM-like)
	// $8000-$BFFF: First 16KB bank
	// $C000-$FFFF: Last 16KB bank (or mirror)
	static uint32_t GetPrgRomOffset(uint16_t addr, [[maybe_unused]] uint32_t prgSize, bool is16K) {
		if (addr < 0x8000) return 0xFFFFFFFF;  // Not PRG ROM

		uint16_t offset = addr - 0x8000;
		if (is16K) {
			// 16KB ROM is mirrored at $C000-$FFFF
			return offset & 0x3FFF;
		} else {
			// 32KB ROM
			return offset;
		}
	}
};

TEST_F(NesMemoryMapTest, InternalRam_BaseAddress_NoChange) {
	EXPECT_EQ(MirrorInternalRam(0x0000), 0x0000);
	EXPECT_EQ(MirrorInternalRam(0x0100), 0x0100);
	EXPECT_EQ(MirrorInternalRam(0x07FF), 0x07FF);
}

TEST_F(NesMemoryMapTest, InternalRam_FirstMirror) {
	EXPECT_EQ(MirrorInternalRam(0x0800), 0x0000);
	EXPECT_EQ(MirrorInternalRam(0x0900), 0x0100);
	EXPECT_EQ(MirrorInternalRam(0x0FFF), 0x07FF);
}

TEST_F(NesMemoryMapTest, InternalRam_SecondMirror) {
	EXPECT_EQ(MirrorInternalRam(0x1000), 0x0000);
	EXPECT_EQ(MirrorInternalRam(0x1100), 0x0100);
	EXPECT_EQ(MirrorInternalRam(0x17FF), 0x07FF);
}

TEST_F(NesMemoryMapTest, InternalRam_ThirdMirror) {
	EXPECT_EQ(MirrorInternalRam(0x1800), 0x0000);
	EXPECT_EQ(MirrorInternalRam(0x1900), 0x0100);
	EXPECT_EQ(MirrorInternalRam(0x1FFF), 0x07FF);
}

TEST_F(NesMemoryMapTest, PpuRegisters_BaseAddress_NoChange) {
	EXPECT_EQ(MirrorPpuRegisters(0x2000), 0x2000);  // PPUCTRL
	EXPECT_EQ(MirrorPpuRegisters(0x2001), 0x2001);  // PPUMASK
	EXPECT_EQ(MirrorPpuRegisters(0x2002), 0x2002);  // PPUSTATUS
	EXPECT_EQ(MirrorPpuRegisters(0x2007), 0x2007);  // PPUDATA
}

TEST_F(NesMemoryMapTest, PpuRegisters_FirstMirror) {
	EXPECT_EQ(MirrorPpuRegisters(0x2008), 0x2000);
	EXPECT_EQ(MirrorPpuRegisters(0x2009), 0x2001);
	EXPECT_EQ(MirrorPpuRegisters(0x200F), 0x2007);
}

TEST_F(NesMemoryMapTest, PpuRegisters_HighMirror) {
	EXPECT_EQ(MirrorPpuRegisters(0x3FF8), 0x2000);
	EXPECT_EQ(MirrorPpuRegisters(0x3FF9), 0x2001);
	EXPECT_EQ(MirrorPpuRegisters(0x3FFF), 0x2007);
}

TEST_F(NesMemoryMapTest, CartridgeSpace_ValidRange) {
	EXPECT_FALSE(IsCartridgeSpace(0x0000));
	EXPECT_FALSE(IsCartridgeSpace(0x4000));
	EXPECT_FALSE(IsCartridgeSpace(0x401F));
	EXPECT_TRUE(IsCartridgeSpace(0x4020));
	EXPECT_TRUE(IsCartridgeSpace(0x8000));
	EXPECT_TRUE(IsCartridgeSpace(0xFFFF));
}

TEST_F(NesMemoryMapTest, PrgRom_16KB_Mirrored) {
	// 16KB ROM is mirrored
	EXPECT_EQ(GetPrgRomOffset(0x8000, 0x4000, true), 0x0000);
	EXPECT_EQ(GetPrgRomOffset(0x9000, 0x4000, true), 0x1000);
	EXPECT_EQ(GetPrgRomOffset(0xBFFF, 0x4000, true), 0x3FFF);
	EXPECT_EQ(GetPrgRomOffset(0xC000, 0x4000, true), 0x0000);  // Mirror
	EXPECT_EQ(GetPrgRomOffset(0xFFFF, 0x4000, true), 0x3FFF);  // Mirror
}

TEST_F(NesMemoryMapTest, PrgRom_32KB_NoMirror) {
	// 32KB ROM is not mirrored
	EXPECT_EQ(GetPrgRomOffset(0x8000, 0x8000, false), 0x0000);
	EXPECT_EQ(GetPrgRomOffset(0xC000, 0x8000, false), 0x4000);
	EXPECT_EQ(GetPrgRomOffset(0xFFFF, 0x8000, false), 0x7FFF);
}

TEST_F(NesMemoryMapTest, PrgRom_InvalidRange) {
	EXPECT_EQ(GetPrgRomOffset(0x0000, 0x8000, false), 0xFFFFFFFF);
	EXPECT_EQ(GetPrgRomOffset(0x7FFF, 0x8000, false), 0xFFFFFFFF);
}

//=============================================================================
// NES Bank Switching Tests (Generic mapper logic)
//=============================================================================

class NesMapperTest : public ::testing::Test {
protected:
	// MMC1-style bank switching (16KB or 32KB modes)
	struct Mmc1State {
		uint8_t control = 0x0C;  // Default: 16KB mode, fixed last bank
		uint8_t chrBank0 = 0;
		uint8_t chrBank1 = 0;
		uint8_t prgBank = 0;
	};

	// Get PRG bank number for address
	static uint8_t GetMmc1PrgBank(const Mmc1State& state, uint16_t addr, uint8_t prgBankCount) {
		uint8_t prgMode = (state.control >> 2) & 0x03;
		uint8_t bank = state.prgBank & 0x0F;

		if (addr < 0x8000) return 0xFF;  // Invalid

		switch (prgMode) {
			case 0:
			case 1:
				// 32KB mode
				return (addr < 0xC000) ? (bank & 0xFE) : (bank | 0x01);
			case 2:
				// Fixed first bank
				return (addr < 0xC000) ? 0 : bank;
			case 3:
				// Fixed last bank
				return (addr < 0xC000) ? bank : (prgBankCount - 1);
			default:
				return 0xFF;
		}
	}

	// UNROM-style bank switching (16KB switchable + 16KB fixed)
	static uint8_t GetUnromPrgBank(uint8_t bankSelect, uint16_t addr, uint8_t prgBankCount) {
		if (addr < 0x8000) return 0xFF;
		if (addr < 0xC000) return bankSelect & 0x07;  // Switchable
		return prgBankCount - 1;  // Fixed last bank
	}
};

TEST_F(NesMapperTest, Mmc1_Mode3_FixedLastBank) {
	Mmc1State state;
	state.control = 0x0C;  // Mode 3
	state.prgBank = 2;

	EXPECT_EQ(GetMmc1PrgBank(state, 0x8000, 16), 2);   // Switchable bank
	EXPECT_EQ(GetMmc1PrgBank(state, 0xC000, 16), 15);  // Fixed last bank
}

TEST_F(NesMapperTest, Mmc1_Mode2_FixedFirstBank) {
	Mmc1State state;
	state.control = 0x08;  // Mode 2
	state.prgBank = 5;

	EXPECT_EQ(GetMmc1PrgBank(state, 0x8000, 16), 0);   // Fixed first bank
	EXPECT_EQ(GetMmc1PrgBank(state, 0xC000, 16), 5);   // Switchable bank
}

TEST_F(NesMapperTest, Mmc1_Mode0_32KBSwitching) {
	Mmc1State state;
	state.control = 0x00;  // Mode 0 (32KB)
	state.prgBank = 6;  // Even bank

	EXPECT_EQ(GetMmc1PrgBank(state, 0x8000, 16), 6);   // Lower 16KB
	EXPECT_EQ(GetMmc1PrgBank(state, 0xC000, 16), 7);   // Upper 16KB
}

TEST_F(NesMapperTest, Unrom_BankSwitching) {
	EXPECT_EQ(GetUnromPrgBank(0, 0x8000, 8), 0);
	EXPECT_EQ(GetUnromPrgBank(3, 0x8000, 8), 3);
	EXPECT_EQ(GetUnromPrgBank(7, 0x8000, 8), 7);
	EXPECT_EQ(GetUnromPrgBank(0, 0xC000, 8), 7);   // Fixed last
	EXPECT_EQ(GetUnromPrgBank(3, 0xC000, 8), 7);   // Fixed last
}

//=============================================================================
// SNES Memory Map Tests
//=============================================================================
// SNES has 24-bit addressing (bank:address)
// LoROM: $00-$7D:$8000-$FFFF (32KB banks, skip $7E-$7F)
// HiROM: $C0-$FF:$0000-$FFFF (64KB banks)
// WRAM: $7E-$7F:$0000-$FFFF (128KB total)

class SnesMemoryMapTest : public ::testing::Test {
protected:
	// Calculate LoROM offset from 24-bit address
	static uint32_t GetLoRomOffset(uint8_t bank, uint16_t addr) {
		// LoROM: Data only in upper half ($8000-$FFFF) of banks $00-$7D
		if (addr < 0x8000) return 0xFFFFFFFF;  // Not ROM space
		if (bank >= 0x7E && bank <= 0x7F) return 0xFFFFFFFF;  // WRAM banks

		// Mirror banks $80-$FF to $00-$7F
		uint8_t effectiveBank = bank & 0x7F;

		// Each bank contributes 32KB
		return ((uint32_t)effectiveBank * 0x8000) + (addr - 0x8000);
	}

	// Calculate HiROM offset from 24-bit address
	static uint32_t GetHiRomOffset(uint8_t bank, uint16_t addr) {
		// HiROM: Banks $C0-$FF contain 64KB each
		if (bank < 0xC0) {
			// Mirror: banks $00-$3F also map to HiROM at $8000-$FFFF
			if (addr >= 0x8000) {
				return ((uint32_t)(bank & 0x3F) * 0x10000) + addr;
			}
			return 0xFFFFFFFF;  // Not ROM space
		}

		// Direct HiROM access
		return ((uint32_t)(bank - 0xC0) * 0x10000) + addr;
	}

	// Check if address is WRAM
	static bool IsWram(uint8_t bank, uint16_t addr) {
		// Banks $7E-$7F are always WRAM
		if (bank >= 0x7E && bank <= 0x7F) return true;

		// First 8KB of banks $00-$3F mirror WRAM
		if ((bank & 0xC0) == 0x00 && addr < 0x2000) return true;

		return false;
	}

	// Get WRAM offset
	static uint32_t GetWramOffset(uint8_t bank, uint16_t addr) {
		if (bank >= 0x7E && bank <= 0x7F) {
			return ((uint32_t)(bank - 0x7E) * 0x10000) + addr;
		}
		if ((bank & 0xC0) == 0x00 && addr < 0x2000) {
			return addr;  // First 8KB
		}
		return 0xFFFFFFFF;
	}
};

TEST_F(SnesMemoryMapTest, LoRom_Bank00_UpperHalf) {
	EXPECT_EQ(GetLoRomOffset(0x00, 0x8000), 0x0000);
	EXPECT_EQ(GetLoRomOffset(0x00, 0x9000), 0x1000);
	EXPECT_EQ(GetLoRomOffset(0x00, 0xFFFF), 0x7FFF);
}

TEST_F(SnesMemoryMapTest, LoRom_Bank01_Offset) {
	EXPECT_EQ(GetLoRomOffset(0x01, 0x8000), 0x8000);
	EXPECT_EQ(GetLoRomOffset(0x01, 0xFFFF), 0xFFFF);
}

TEST_F(SnesMemoryMapTest, LoRom_LowerHalf_Invalid) {
	EXPECT_EQ(GetLoRomOffset(0x00, 0x0000), 0xFFFFFFFF);
	EXPECT_EQ(GetLoRomOffset(0x00, 0x7FFF), 0xFFFFFFFF);
}

TEST_F(SnesMemoryMapTest, LoRom_WramBanks_Invalid) {
	EXPECT_EQ(GetLoRomOffset(0x7E, 0x8000), 0xFFFFFFFF);
	EXPECT_EQ(GetLoRomOffset(0x7F, 0xFFFF), 0xFFFFFFFF);
}

TEST_F(SnesMemoryMapTest, LoRom_MirrorBanks) {
	// Bank $80 mirrors $00
	EXPECT_EQ(GetLoRomOffset(0x80, 0x8000), 0x0000);
	// Bank $81 mirrors $01
	EXPECT_EQ(GetLoRomOffset(0x81, 0x8000), 0x8000);
}

TEST_F(SnesMemoryMapTest, HiRom_BankC0_Start) {
	EXPECT_EQ(GetHiRomOffset(0xC0, 0x0000), 0x00000);
	EXPECT_EQ(GetHiRomOffset(0xC0, 0xFFFF), 0x0FFFF);
}

TEST_F(SnesMemoryMapTest, HiRom_BankC1_Offset) {
	EXPECT_EQ(GetHiRomOffset(0xC1, 0x0000), 0x10000);
	EXPECT_EQ(GetHiRomOffset(0xC1, 0xFFFF), 0x1FFFF);
}

TEST_F(SnesMemoryMapTest, HiRom_MirrorBanks) {
	// Banks $00-$3F mirror HiROM at $8000-$FFFF
	EXPECT_EQ(GetHiRomOffset(0x00, 0x8000), 0x8000);
	EXPECT_EQ(GetHiRomOffset(0x01, 0x8000), 0x18000);
}

TEST_F(SnesMemoryMapTest, Wram_DirectBanks) {
	EXPECT_TRUE(IsWram(0x7E, 0x0000));
	EXPECT_TRUE(IsWram(0x7E, 0xFFFF));
	EXPECT_TRUE(IsWram(0x7F, 0x0000));
	EXPECT_TRUE(IsWram(0x7F, 0xFFFF));
}

TEST_F(SnesMemoryMapTest, Wram_MirrorRange) {
	EXPECT_TRUE(IsWram(0x00, 0x0000));
	EXPECT_TRUE(IsWram(0x00, 0x1FFF));
	EXPECT_FALSE(IsWram(0x00, 0x2000));
}

TEST_F(SnesMemoryMapTest, Wram_Offset) {
	EXPECT_EQ(GetWramOffset(0x7E, 0x0000), 0x00000);
	EXPECT_EQ(GetWramOffset(0x7E, 0xFFFF), 0x0FFFF);
	EXPECT_EQ(GetWramOffset(0x7F, 0x0000), 0x10000);
	EXPECT_EQ(GetWramOffset(0x7F, 0xFFFF), 0x1FFFF);
}

//=============================================================================
// Game Boy Memory Map Tests
//=============================================================================
// Game Boy Memory Layout:
// $0000-$00FF: Boot ROM (mapped out after boot)
// $0000-$3FFF: ROM Bank 0 (fixed)
// $4000-$7FFF: ROM Bank 1-N (switchable)
// $8000-$9FFF: VRAM
// $A000-$BFFF: External RAM (cartridge)
// $C000-$CFFF: WRAM Bank 0
// $D000-$DFFF: WRAM Bank 1-7 (GBC only, bank 1 on DMG)
// $E000-$FDFF: Echo of C000-DDFF
// $FE00-$FE9F: OAM
// $FEA0-$FEFF: Not usable
// $FF00-$FF7F: I/O Registers
// $FF80-$FFFE: HRAM
// $FFFF: IE Register

class GbMemoryMapTest : public ::testing::Test {
protected:
	// Get memory region for address
	enum class GbMemoryRegion {
		RomBank0,
		RomBankN,
		Vram,
		ExternalRam,
		WramBank0,
		WramBankN,
		Echo,
		Oam,
		Unusable,
		IoRegisters,
		Hram,
		IeRegister
	};

	static GbMemoryRegion GetMemoryRegion(uint16_t addr) {
		if (addr < 0x4000) return GbMemoryRegion::RomBank0;
		if (addr < 0x8000) return GbMemoryRegion::RomBankN;
		if (addr < 0xA000) return GbMemoryRegion::Vram;
		if (addr < 0xC000) return GbMemoryRegion::ExternalRam;
		if (addr < 0xD000) return GbMemoryRegion::WramBank0;
		if (addr < 0xE000) return GbMemoryRegion::WramBankN;
		if (addr < 0xFE00) return GbMemoryRegion::Echo;
		if (addr < 0xFEA0) return GbMemoryRegion::Oam;
		if (addr < 0xFF00) return GbMemoryRegion::Unusable;
		if (addr < 0xFF80) return GbMemoryRegion::IoRegisters;
		if (addr < 0xFFFF) return GbMemoryRegion::Hram;
		return GbMemoryRegion::IeRegister;
	}

	// Calculate echo RAM address (mirrors $C000-$DDFF)
	static uint16_t GetEchoRamSource(uint16_t addr) {
		if (addr >= 0xE000 && addr < 0xFE00) {
			return addr - 0x2000;
		}
		return addr;
	}

	// MBC1 ROM bank calculation
	static uint8_t GetMbc1RomBank(uint8_t bankLow, uint8_t bankHigh, bool advancedMode, uint16_t addr) {
		if (addr < 0x4000) {
			// Bank 0 area - can be switched in advanced mode
			if (advancedMode) {
				return (bankHigh << 5) & 0x60;
			}
			return 0;
		}
		if (addr < 0x8000) {
			// Switchable bank area
			uint8_t bank = ((bankHigh << 5) | bankLow) & 0x7F;
			// Banks 0x00, 0x20, 0x40, 0x60 are mapped to 0x01, 0x21, 0x41, 0x61
			if ((bank & 0x1F) == 0) {
				bank |= 0x01;
			}
			return bank;
		}
		return 0xFF;  // Invalid
	}

	// MBC3 ROM bank calculation
	static uint8_t GetMbc3RomBank(uint8_t bank, uint16_t addr) {
		if (addr < 0x4000) return 0;
		if (addr < 0x8000) {
			// Bank 0 is mapped to bank 1
			return (bank == 0) ? 1 : (bank & 0x7F);
		}
		return 0xFF;
	}

	// MBC5 ROM bank calculation (9-bit bank number)
	static uint16_t GetMbc5RomBank(uint8_t bankLow, uint8_t bankHigh, uint16_t addr) {
		if (addr < 0x4000) return 0;
		if (addr < 0x8000) {
			return ((uint16_t)(bankHigh & 0x01) << 8) | bankLow;
		}
		return 0xFFFF;
	}
};

TEST_F(GbMemoryMapTest, Region_RomBank0) {
	EXPECT_EQ(GetMemoryRegion(0x0000), GbMemoryRegion::RomBank0);
	EXPECT_EQ(GetMemoryRegion(0x00FF), GbMemoryRegion::RomBank0);
	EXPECT_EQ(GetMemoryRegion(0x3FFF), GbMemoryRegion::RomBank0);
}

TEST_F(GbMemoryMapTest, Region_RomBankN) {
	EXPECT_EQ(GetMemoryRegion(0x4000), GbMemoryRegion::RomBankN);
	EXPECT_EQ(GetMemoryRegion(0x7FFF), GbMemoryRegion::RomBankN);
}

TEST_F(GbMemoryMapTest, Region_Vram) {
	EXPECT_EQ(GetMemoryRegion(0x8000), GbMemoryRegion::Vram);
	EXPECT_EQ(GetMemoryRegion(0x9FFF), GbMemoryRegion::Vram);
}

TEST_F(GbMemoryMapTest, Region_ExternalRam) {
	EXPECT_EQ(GetMemoryRegion(0xA000), GbMemoryRegion::ExternalRam);
	EXPECT_EQ(GetMemoryRegion(0xBFFF), GbMemoryRegion::ExternalRam);
}

TEST_F(GbMemoryMapTest, Region_Wram) {
	EXPECT_EQ(GetMemoryRegion(0xC000), GbMemoryRegion::WramBank0);
	EXPECT_EQ(GetMemoryRegion(0xCFFF), GbMemoryRegion::WramBank0);
	EXPECT_EQ(GetMemoryRegion(0xD000), GbMemoryRegion::WramBankN);
	EXPECT_EQ(GetMemoryRegion(0xDFFF), GbMemoryRegion::WramBankN);
}

TEST_F(GbMemoryMapTest, Region_Echo) {
	EXPECT_EQ(GetMemoryRegion(0xE000), GbMemoryRegion::Echo);
	EXPECT_EQ(GetMemoryRegion(0xFDFF), GbMemoryRegion::Echo);
}

TEST_F(GbMemoryMapTest, Region_Oam) {
	EXPECT_EQ(GetMemoryRegion(0xFE00), GbMemoryRegion::Oam);
	EXPECT_EQ(GetMemoryRegion(0xFE9F), GbMemoryRegion::Oam);
}

TEST_F(GbMemoryMapTest, Region_HighAddresses) {
	EXPECT_EQ(GetMemoryRegion(0xFEA0), GbMemoryRegion::Unusable);
	EXPECT_EQ(GetMemoryRegion(0xFF00), GbMemoryRegion::IoRegisters);
	EXPECT_EQ(GetMemoryRegion(0xFF80), GbMemoryRegion::Hram);
	EXPECT_EQ(GetMemoryRegion(0xFFFF), GbMemoryRegion::IeRegister);
}

TEST_F(GbMemoryMapTest, EchoRam_MirrorsWram) {
	EXPECT_EQ(GetEchoRamSource(0xE000), 0xC000);
	EXPECT_EQ(GetEchoRamSource(0xE100), 0xC100);
	EXPECT_EQ(GetEchoRamSource(0xFDFF), 0xDDFF);
}

TEST_F(GbMemoryMapTest, EchoRam_NonEchoPassthrough) {
	EXPECT_EQ(GetEchoRamSource(0xC000), 0xC000);
	EXPECT_EQ(GetEchoRamSource(0xFE00), 0xFE00);
}

TEST_F(GbMemoryMapTest, Mbc1_Bank0Area_NormalMode) {
	EXPECT_EQ(GetMbc1RomBank(1, 0, false, 0x0000), 0);
	EXPECT_EQ(GetMbc1RomBank(5, 3, false, 0x2000), 0);
}

TEST_F(GbMemoryMapTest, Mbc1_Bank0Area_AdvancedMode) {
	EXPECT_EQ(GetMbc1RomBank(1, 1, true, 0x0000), 0x20);
	EXPECT_EQ(GetMbc1RomBank(1, 2, true, 0x0000), 0x40);
}

TEST_F(GbMemoryMapTest, Mbc1_SwitchableBank) {
	EXPECT_EQ(GetMbc1RomBank(1, 0, false, 0x4000), 1);
	EXPECT_EQ(GetMbc1RomBank(5, 0, false, 0x4000), 5);
	EXPECT_EQ(GetMbc1RomBank(15, 0, false, 0x4000), 15);
}

TEST_F(GbMemoryMapTest, Mbc1_Bank0MappedTo1) {
	// Bank 0 is special-cased to 1 in switchable area
	EXPECT_EQ(GetMbc1RomBank(0, 0, false, 0x4000), 1);
	EXPECT_EQ(GetMbc1RomBank(0x20, 0, false, 0x4000), 0x21);  // 0x20 -> 0x21
}

TEST_F(GbMemoryMapTest, Mbc1_HighBankBits) {
	EXPECT_EQ(GetMbc1RomBank(5, 1, false, 0x4000), 0x25);  // (1 << 5) | 5
	EXPECT_EQ(GetMbc1RomBank(5, 3, false, 0x4000), 0x65);  // (3 << 5) | 5
}

TEST_F(GbMemoryMapTest, Mbc3_Bank0MappedTo1) {
	EXPECT_EQ(GetMbc3RomBank(0, 0x4000), 1);
	EXPECT_EQ(GetMbc3RomBank(1, 0x4000), 1);
	EXPECT_EQ(GetMbc3RomBank(127, 0x4000), 127);
}

TEST_F(GbMemoryMapTest, Mbc5_9BitBankNumber) {
	EXPECT_EQ(GetMbc5RomBank(0x00, 0x00, 0x4000), 0x000);
	EXPECT_EQ(GetMbc5RomBank(0xFF, 0x00, 0x4000), 0x0FF);
	EXPECT_EQ(GetMbc5RomBank(0x00, 0x01, 0x4000), 0x100);
	EXPECT_EQ(GetMbc5RomBank(0xFF, 0x01, 0x4000), 0x1FF);
}

//=============================================================================
// Parameterized Tests for NES Memory Regions
//=============================================================================

struct NesRegionTestParam {
	uint16_t addr;
	uint16_t expected;
	const char* name;
};

class NesRamMirrorTest : public ::testing::TestWithParam<NesRegionTestParam> {};

TEST_P(NesRamMirrorTest, MirroredAddress) {
	auto param = GetParam();
	uint16_t actual = param.addr & 0x07FF;
	EXPECT_EQ(actual, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
	RamMirrors,
	NesRamMirrorTest,
	::testing::Values(
		NesRegionTestParam{0x0000, 0x0000, "Base"},
		NesRegionTestParam{0x0800, 0x0000, "Mirror1"},
		NesRegionTestParam{0x1000, 0x0000, "Mirror2"},
		NesRegionTestParam{0x1800, 0x0000, "Mirror3"},
		NesRegionTestParam{0x01FF, 0x01FF, "StackPage"},
		NesRegionTestParam{0x09FF, 0x01FF, "StackMirror1"},
		NesRegionTestParam{0x11FF, 0x01FF, "StackMirror2"},
		NesRegionTestParam{0x19FF, 0x01FF, "StackMirror3"}
	)
);
