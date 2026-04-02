#include "pch.h"
#include "ChannelF/ChannelFTypes.h"
#include "ChannelF/ChannelFBiosDatabase.h"
#include "ChannelF/ChannelFMemoryManager.h"

// =============================================================================
// Channel F Memory Map + Port I/O Tests
// =============================================================================
// Tests for Channel F memory map decoding, port I/O register behavior,
// and PSU (Program Storage Unit) interface constants.
// Addresses #974 (F8 PSU interface), #976 (cart type detection).

namespace ChfMemoryHelpers {

	/// <summary>
	/// Channel F memory map region classification.
	/// $0000-$03FF: BIOS ROM 1 (1KB)
	/// $0400-$07FF: BIOS ROM 2 (1KB)
	/// $0800-$FFFF: Cartridge ROM (up to ~62KB)
	/// </summary>
	enum class MemRegion {
		BiosRom1,
		BiosRom2,
		CartRom,
		Invalid
	};

	static MemRegion ClassifyAddress(uint16_t addr) {
		if (addr < 0x0400) return MemRegion::BiosRom1;
		if (addr < 0x0800) return MemRegion::BiosRom2;
		return MemRegion::CartRom;
	}

	/// <summary>
	/// Computes the offset within a region for a given address.
	/// </summary>
	static uint16_t RegionOffset(uint16_t addr) {
		if (addr < 0x0400) return addr;
		if (addr < 0x0800) return (uint16_t)(addr - 0x0400);
		return (uint16_t)(addr - 0x0800);
	}

	/// <summary>
	/// Channel F I/O port classification.
	/// Port 0: Console switches / video color+sound control
	/// Port 1: Video row/column latch, controller input
	/// Port 4: Right controller
	/// Port 5: Left controller, sound output
	/// </summary>
	enum class PortFunction {
		Console,		// Port 0
		VideoLatch,		// Port 1
		Unused,			// Ports 2-3
		RightCtrl,		// Port 4
		LeftCtrl,		// Port 5
		Unknown			// Port 6+
	};

	static PortFunction ClassifyPort(uint8_t port) {
		switch (port) {
			case 0: return PortFunction::Console;
			case 1: return PortFunction::VideoLatch;
			case 2:
			case 3: return PortFunction::Unused;
			case 4: return PortFunction::RightCtrl;
			case 5: return PortFunction::LeftCtrl;
			default: return PortFunction::Unknown;
		}
	}

	/// <summary>
	/// Decodes Port 0 write bits for video/sound control.
	/// Bits 0-1: Color select (4 colors)
	/// Bits 6-7: Sound control
	/// </summary>
	static uint8_t Port0Color(uint8_t value) {
		return value & 0x03;
	}

	static uint8_t Port0Sound(uint8_t value) {
		return (value >> 6) & 0x03;
	}

	/// <summary>
	/// F8 CPU ISAR register masking — 6-bit field.
	/// </summary>
	static uint8_t MaskIsar(uint8_t value) {
		return value & 0x3f;
	}

	/// <summary>
	/// F8 CPU W register (status) flag extraction.
	/// Bit 0: Overflow, Bit 1: Zero, Bit 2: Carry, Bit 3: Sign
	/// </summary>
	static bool WFlagSign(uint8_t w) { return (w & 0x08) != 0; }
	static bool WFlagCarry(uint8_t w) { return (w & 0x04) != 0; }
	static bool WFlagZero(uint8_t w) { return (w & 0x02) != 0; }
	static bool WFlagOverflow(uint8_t w) { return (w & 0x01) != 0; }

	/// <summary>
	/// F8 scratchpad register named aliases.
	/// </summary>
	static constexpr uint8_t RegJ  = 9;
	static constexpr uint8_t RegHU = 10;
	static constexpr uint8_t RegHL = 11;
	static constexpr uint8_t RegKU = 12;
	static constexpr uint8_t RegKL = 13;
	static constexpr uint8_t RegQU = 14;
	static constexpr uint8_t RegQL = 15;
}

// =============================================================================
// Memory Map Classification
// =============================================================================

TEST(ChfMemoryMapTest, BiosRom1_Range) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x0000), ChfMemoryHelpers::MemRegion::BiosRom1);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x0100), ChfMemoryHelpers::MemRegion::BiosRom1);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x03ff), ChfMemoryHelpers::MemRegion::BiosRom1);
}

TEST(ChfMemoryMapTest, BiosRom2_Range) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x0400), ChfMemoryHelpers::MemRegion::BiosRom2);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x07ff), ChfMemoryHelpers::MemRegion::BiosRom2);
}

TEST(ChfMemoryMapTest, CartRom_Range) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x0800), ChfMemoryHelpers::MemRegion::CartRom);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0x1000), ChfMemoryHelpers::MemRegion::CartRom);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyAddress(0xffff), ChfMemoryHelpers::MemRegion::CartRom);
}

TEST(ChfMemoryMapTest, RegionOffset_BiosRom1) {
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x0000), 0x0000);
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x0100), 0x0100);
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x03ff), 0x03ff);
}

TEST(ChfMemoryMapTest, RegionOffset_BiosRom2) {
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x0400), 0x0000);
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x07ff), 0x03ff);
}

TEST(ChfMemoryMapTest, RegionOffset_CartRom) {
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x0800), 0x0000);
	EXPECT_EQ(ChfMemoryHelpers::RegionOffset(0x1000), 0x0800);
}

// =============================================================================
// I/O Port Classification
// =============================================================================

TEST(ChfPortTest, Port0_Console) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(0), ChfMemoryHelpers::PortFunction::Console);
}

TEST(ChfPortTest, Port1_VideoLatch) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(1), ChfMemoryHelpers::PortFunction::VideoLatch);
}

TEST(ChfPortTest, Port23_Unused) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(2), ChfMemoryHelpers::PortFunction::Unused);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(3), ChfMemoryHelpers::PortFunction::Unused);
}

TEST(ChfPortTest, Port4_RightCtrl) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(4), ChfMemoryHelpers::PortFunction::RightCtrl);
}

TEST(ChfPortTest, Port5_LeftCtrl) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(5), ChfMemoryHelpers::PortFunction::LeftCtrl);
}

TEST(ChfPortTest, Port6Plus_Unknown) {
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(6), ChfMemoryHelpers::PortFunction::Unknown);
	EXPECT_EQ(ChfMemoryHelpers::ClassifyPort(255), ChfMemoryHelpers::PortFunction::Unknown);
}

// =============================================================================
// Port 0 Decode
// =============================================================================

TEST(ChfPort0Test, Color_Bits01) {
	EXPECT_EQ(ChfMemoryHelpers::Port0Color(0x00), 0);
	EXPECT_EQ(ChfMemoryHelpers::Port0Color(0x01), 1);
	EXPECT_EQ(ChfMemoryHelpers::Port0Color(0x02), 2);
	EXPECT_EQ(ChfMemoryHelpers::Port0Color(0x03), 3);
	EXPECT_EQ(ChfMemoryHelpers::Port0Color(0xff), 3);	// Masked
}

TEST(ChfPort0Test, Sound_Bits67) {
	EXPECT_EQ(ChfMemoryHelpers::Port0Sound(0x00), 0);
	EXPECT_EQ(ChfMemoryHelpers::Port0Sound(0x40), 1);
	EXPECT_EQ(ChfMemoryHelpers::Port0Sound(0x80), 2);
	EXPECT_EQ(ChfMemoryHelpers::Port0Sound(0xc0), 3);
}

// =============================================================================
// F8 CPU Status Register (W) Flags
// =============================================================================

TEST(ChfWFlagsTest, AllClear) {
	EXPECT_FALSE(ChfMemoryHelpers::WFlagSign(0x00));
	EXPECT_FALSE(ChfMemoryHelpers::WFlagCarry(0x00));
	EXPECT_FALSE(ChfMemoryHelpers::WFlagZero(0x00));
	EXPECT_FALSE(ChfMemoryHelpers::WFlagOverflow(0x00));
}

TEST(ChfWFlagsTest, SignFlag) {
	EXPECT_TRUE(ChfMemoryHelpers::WFlagSign(0x08));
	EXPECT_FALSE(ChfMemoryHelpers::WFlagSign(0x07));
}

TEST(ChfWFlagsTest, CarryFlag) {
	EXPECT_TRUE(ChfMemoryHelpers::WFlagCarry(0x04));
}

TEST(ChfWFlagsTest, ZeroFlag) {
	EXPECT_TRUE(ChfMemoryHelpers::WFlagZero(0x02));
}

TEST(ChfWFlagsTest, OverflowFlag) {
	EXPECT_TRUE(ChfMemoryHelpers::WFlagOverflow(0x01));
}

TEST(ChfWFlagsTest, AllSet) {
	EXPECT_TRUE(ChfMemoryHelpers::WFlagSign(0x0f));
	EXPECT_TRUE(ChfMemoryHelpers::WFlagCarry(0x0f));
	EXPECT_TRUE(ChfMemoryHelpers::WFlagZero(0x0f));
	EXPECT_TRUE(ChfMemoryHelpers::WFlagOverflow(0x0f));
}

// =============================================================================
// ISAR Masking
// =============================================================================

TEST(ChfIsarTest, MaskTo6Bits) {
	EXPECT_EQ(ChfMemoryHelpers::MaskIsar(0x00), 0x00);
	EXPECT_EQ(ChfMemoryHelpers::MaskIsar(0x3f), 0x3f);
	EXPECT_EQ(ChfMemoryHelpers::MaskIsar(0x40), 0x00);
	EXPECT_EQ(ChfMemoryHelpers::MaskIsar(0xff), 0x3f);
	EXPECT_EQ(ChfMemoryHelpers::MaskIsar(0x80), 0x00);
}

// =============================================================================
// Named Scratchpad Register Constants
// =============================================================================

TEST(ChfScratchpadTest, NamedRegisterAliases) {
	EXPECT_EQ(ChfMemoryHelpers::RegJ, 9);
	EXPECT_EQ(ChfMemoryHelpers::RegHU, 10);
	EXPECT_EQ(ChfMemoryHelpers::RegHL, 11);
	EXPECT_EQ(ChfMemoryHelpers::RegKU, 12);
	EXPECT_EQ(ChfMemoryHelpers::RegKL, 13);
	EXPECT_EQ(ChfMemoryHelpers::RegQU, 14);
	EXPECT_EQ(ChfMemoryHelpers::RegQL, 15);
}

// =============================================================================
// Channel F Constants
// =============================================================================

TEST(ChfConstantsTest, ScreenDimensions) {
	EXPECT_EQ(ChannelFConstants::ScreenWidth, 128u);
	EXPECT_EQ(ChannelFConstants::ScreenHeight, 64u);
	EXPECT_EQ(ChannelFConstants::PixelCount, 128u * 64u);
}

TEST(ChfConstantsTest, BiosSize) {
	EXPECT_EQ(ChannelFConstants::BiosSize, 0x0800u);
}

TEST(ChfConstantsTest, MaxCartSize) {
	EXPECT_EQ(ChannelFConstants::MaxCartSize, 0x10000u);
}

TEST(ChfConstantsTest, VramSize) {
	EXPECT_EQ(ChannelFConstants::VramSize, 128u * 64u);
}

TEST(ChfConstantsTest, CpuClockHz) {
	EXPECT_EQ(ChannelFConstants::CpuClockHz, 1789773u);
}

TEST(ChfConstantsTest, NumColors) {
	EXPECT_EQ(ChannelFConstants::NumColors, 4);
}

TEST(ChfConstantsTest, ScratchpadSize) {
	EXPECT_EQ(ChannelFConstants::ScratchpadSize, 64u);
}

// =============================================================================
// Channel F State Defaults
// =============================================================================

TEST(ChfStateDefaultsTest, CpuState) {
	ChannelFCpuState state = {};
	EXPECT_EQ(state.CycleCount, 0u);
	EXPECT_EQ(state.PC0, 0);
	EXPECT_EQ(state.PC1, 0);
	EXPECT_EQ(state.DC0, 0);
	EXPECT_EQ(state.DC1, 0);
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.W, 0);
	EXPECT_EQ(state.ISAR, 0);
	EXPECT_FALSE(state.InterruptsEnabled);
}

TEST(ChfStateDefaultsTest, CpuScratchpad_AllZero) {
	ChannelFCpuState state = {};
	for (int i = 0; i < 64; i++) {
		EXPECT_EQ(state.Scratchpad[i], 0);
	}
}

TEST(ChfStateDefaultsTest, VideoState) {
	ChannelFVideoState state = {};
	EXPECT_EQ(state.Color, 0);
	EXPECT_EQ(state.X, 0);
	EXPECT_EQ(state.Y, 0);
}

TEST(ChfStateDefaultsTest, AudioState) {
	ChannelFAudioState state = {};
	EXPECT_EQ(state.ToneSelect, 0);
	EXPECT_FALSE(state.SoundEnabled);
}

TEST(ChfStateDefaultsTest, PortState) {
	ChannelFPortState state = {};
	EXPECT_EQ(state.Port0, 0);
	EXPECT_EQ(state.Port1, 0);
	EXPECT_EQ(state.Port4, 0xff);	// Default: no buttons pressed
	EXPECT_EQ(state.Port5, 0xff);	// Default: no buttons pressed
}

TEST(ChfStateDefaultsTest, CompositeState) {
	ChannelFState state = {};
	EXPECT_EQ(state.Cpu.A, 0);
	EXPECT_EQ(state.Video.Color, 0);
	EXPECT_FALSE(state.Audio.SoundEnabled);
	EXPECT_EQ(state.Ports.Port4, 0xff);
}

// =============================================================================
// BIOS Variant Enum
// =============================================================================

TEST(ChfBiosVariantTest, EnumValues) {
	EXPECT_EQ(static_cast<int>(ChannelFBiosVariant::Unknown), 0);
	EXPECT_NE(static_cast<int>(ChannelFBiosVariant::SystemI), 0);
	EXPECT_NE(static_cast<int>(ChannelFBiosVariant::SystemII), 0);
	EXPECT_NE(static_cast<int>(ChannelFBiosVariant::SystemI),
		static_cast<int>(ChannelFBiosVariant::SystemII));
}

// =============================================================================
// Cartridge Board + RAM Mapping
// =============================================================================

TEST(ChfCartBoardTest, SmallCart_DetectsStandardRom) {
	ChannelFMemoryManager mm;
	vector<uint8_t> rom(0x1000, 0xea);
	mm.LoadCart(rom.data(), (uint32_t)rom.size());

	EXPECT_EQ(mm.GetCartBoardType(), ChannelFMemoryManager::CartBoardType::StandardRom);
	EXPECT_FALSE(mm.HasCartRam());
}

TEST(ChfCartBoardTest, LargeCart_DetectsBankedRom) {
	ChannelFMemoryManager mm;
	vector<uint8_t> rom(0x6000, 0xaa);
	mm.LoadCart(rom.data(), (uint32_t)rom.size());

	EXPECT_EQ(mm.GetCartBoardType(), ChannelFMemoryManager::CartBoardType::BankedRom);
	EXPECT_FALSE(mm.HasCartRam());
}

TEST(ChfCartBoardTest, SignatureCart_DetectsRamBoard) {
	ChannelFMemoryManager mm;
	vector<uint8_t> rom(0x1000, 0xff);
	const char* sig = "VIDEOCART-10";
	memcpy(rom.data() + 0x20, sig, strlen(sig));
	mm.LoadCart(rom.data(), (uint32_t)rom.size());

	EXPECT_EQ(mm.GetCartBoardType(), ChannelFMemoryManager::CartBoardType::RomWithRam);
	EXPECT_TRUE(mm.HasCartRam());
}

TEST(ChfCartBoardTest, BankSwitchPorts_SelectExpectedBankWindow) {
	ChannelFMemoryManager mm;
	vector<uint8_t> rom(0x6000, 0xff);
	rom[0x0000] = 0x11;
	rom[0x2000] = 0x22;
	rom[0x4000] = 0x33;
	mm.LoadCart(rom.data(), (uint32_t)rom.size());

	EXPECT_EQ(mm.GetCartBoardType(), ChannelFMemoryManager::CartBoardType::BankedRom);
	EXPECT_EQ(mm.ReadMemory(0x0800), 0x11);

	mm.WritePort(0x21, 0x00);
	EXPECT_EQ(mm.GetActiveCartBank(), 1);
	EXPECT_EQ(mm.ReadMemory(0x0800), 0x22);

	mm.WritePort(0x22, 0x00);
	EXPECT_EQ(mm.GetActiveCartBank(), 2);
	EXPECT_EQ(mm.ReadMemory(0x0800), 0x33);

	mm.WritePort(0x27, 0x00);
	EXPECT_EQ(mm.GetActiveCartBank(), 1);
	EXPECT_EQ(mm.ReadMemory(0x0800), 0x22);
}

TEST(ChfCartBoardTest, RamBoard_WritesAndReadsCartRamRange) {
	ChannelFMemoryManager mm;
	vector<uint8_t> rom(0x1000, 0xff);
	const char* sig = "MAZE";
	memcpy(rom.data() + 0x10, sig, strlen(sig));
	mm.LoadCart(rom.data(), (uint32_t)rom.size());

	ASSERT_TRUE(mm.HasCartRam());
	EXPECT_EQ(mm.GetCartRamSize(), ChannelFMemoryManager::CartRamSize);

	mm.WriteMemory(ChannelFMemoryManager::CartRamStartAddr, 0x5a);
	mm.WriteMemory(ChannelFMemoryManager::CartRamEndAddr, 0xa5);

	EXPECT_EQ(mm.ReadMemory(ChannelFMemoryManager::CartRamStartAddr), 0x5a);
	EXPECT_EQ(mm.ReadMemory(ChannelFMemoryManager::CartRamEndAddr), 0xa5);
}
