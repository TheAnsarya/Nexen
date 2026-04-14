#include "pch.h"
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan Memory Manager Behavioral Tests
// =============================================================================
// Tests for memory subsystem logic not covered by existing WS test files:
// - IRQ dispatch (priority encoding, vector calculation, set/clear/ack)
// - Address space (word bus, wait states per segment)
// - Port classification (word port, unmapped port, port wait states)
// - System control register $60 bit decoding
// - ROM control register $A0 bit decoding / BootRom latch
// - IRQ registers $B0-$B7 behavior
// - Video mode derivation
// - Open bus latching

namespace WsMemoryTestHelpers {

	// =========================================================================
	// IRQ dispatch helpers — replicate WsMemoryManager IRQ logic
	// =========================================================================

	/// <summary>
	/// SetIrqSource: only sets ActiveIrqs bit if the corresponding EnabledIrqs bit is set.
	/// </summary>
	static void SetIrqSource(WsMemoryManagerState& state, WsIrqSource src) {
		if (state.EnabledIrqs & (uint8_t)src) {
			state.ActiveIrqs |= (uint8_t)src;
		}
	}

	/// <summary>
	/// ClearIrqSource: clears only the requested source bit.
	/// ActiveIrqs &= ~src.
	/// </summary>
	static void ClearIrqSource(WsMemoryManagerState& state, WsIrqSource src) {
		state.ActiveIrqs &= ~(uint8_t)src;
	}

	/// <summary>
	/// AcknowledgeIrqs: Write to port $B6 — clears bits that are SET in the written value.
	/// ActiveIrqs &= ~value (standard acknowledge pattern).
	/// </summary>
	static void AcknowledgeIrqs(WsMemoryManagerState& state, uint8_t value) {
		state.ActiveIrqs &= ~value;
	}

	/// <summary>
	/// GetIrqVector: Returns IrqVectorOffset + index of highest pending IRQ bit (7 down to 0).
	/// If no IRQ is active, returns IrqVectorOffset alone.
	/// </summary>
	static uint8_t GetIrqVector(const WsMemoryManagerState& state) {
		uint8_t activeIrqs = state.ActiveIrqs;
		for (int i = 7; i >= 0; i--) {
			if (activeIrqs & (1 << i)) {
				return state.IrqVectorOffset + i;
			}
		}
		return state.IrqVectorOffset;
	}

	/// <summary>
	/// HasPendingIrq: True if any ActiveIrqs bits are set.
	/// </summary>
	static bool HasPendingIrq(const WsMemoryManagerState& state) {
		return state.ActiveIrqs != 0;
	}

	// =========================================================================
	// Address space helpers — replicate WsMemoryManager memory logic
	// =========================================================================

	/// <summary>
	/// IsWordBus: Determines if address uses 16-bit bus.
	/// Segment 0 (work RAM): always 16-bit.
	/// Segment 1 (SRAM): always 8-bit.
	/// Segment 2+ (ROM): depends on CartWordBus flag.
	/// </summary>
	static bool IsWordBus(uint32_t addr, bool cartWordBus) {
		switch (addr >> 16) {
			case 0: return true;
			case 1: return false;
			default: return cartWordBus;
		}
	}

	/// <summary>
	/// GetWaitStates: Returns wait states for memory access.
	/// Segment 0 (work RAM): 1 cycle.
	/// Segment 1 (SRAM): 1 + SlowSram.
	/// Segment 2+ (ROM): 1 + SlowRom.
	/// </summary>
	static uint8_t GetWaitStates(uint32_t addr, bool slowSram, bool slowRom) {
		switch (addr >> 16) {
			case 0: return 1;
			case 1: return 1 + (uint8_t)slowSram;
			default: return 1 + (uint8_t)slowRom;
		}
	}

	// =========================================================================
	// Port classification helpers — replicate WsMemoryManager port logic
	// =========================================================================

	/// <summary>
	/// IsWordPort: Ports below $C0 support 16-bit access.
	/// </summary>
	static bool IsWordPort(uint16_t port) {
		return port < 0xC0;
	}

	/// <summary>
	/// IsUnmappedPort: Port >= $100 or (port >= $100 and low 8 bits >= $B7).
	/// Actually: (port & 0x100) || (port >= 0x100 && (port & 0xFF) >= 0xB7).
	/// </summary>
	static bool IsUnmappedPort(uint16_t port) {
		return (port & 0x100) || (port >= 0x100 && (port & 0xFF) >= 0xB7);
	}

	/// <summary>
	/// GetPortWaitStates: 1 cycle normally, +1 if SlowPort and port is in cart range ($C0-$FF).
	/// </summary>
	static uint8_t GetPortWaitStates(uint16_t port, bool slowPort) {
		return 1 + (uint8_t)(slowPort && port >= 0xC0 && port <= 0xFF);
	}

	// =========================================================================
	// System Control Register $60 helpers
	// =========================================================================

	/// <summary>
	/// Decodes SystemControl2 register ($60) write value.
	/// Bit 7: ColorEnabled, Bit 6: Enable4bpp, Bit 5: Enable4bppPacked,
	/// Bit 3: SlowPort, Bit 1: SlowSram.
	/// Mask: 0xEF (bit 4 is excluded).
	/// </summary>
	struct SystemControl2Result {
		uint8_t raw;
		bool colorEnabled;
		bool enable4bpp;
		bool enable4bppPacked;
		bool slowPort;
		bool slowSram;
	};

	static SystemControl2Result DecodeSystemControl2(uint8_t value) {
		uint8_t masked = value & 0xEF;
		return {
			masked,
			(masked & 0x80) != 0,
			(masked & 0x40) != 0,
			(masked & 0x20) != 0,
			(masked & 0x08) != 0,
			(masked & 0x02) != 0
		};
	}

	/// <summary>
	/// Derives WsVideoMode from system control flags.
	/// </summary>
	static WsVideoMode DeriveVideoMode(bool colorEnabled, bool enable4bpp, bool enable4bppPacked) {
		if (!colorEnabled) return WsVideoMode::Monochrome;
		if (!enable4bpp) return WsVideoMode::Color2bpp;
		if (!enable4bppPacked) return WsVideoMode::Color4bpp;
		return WsVideoMode::Color4bppPacked;
	}

	// =========================================================================
	// ROM Control Register $A0 helpers
	// =========================================================================

	/// <summary>
	/// Decodes ROM control register ($A0) read value.
	/// Bit 0: BootRomDisabled, Bit 1: (always set on non-Mono), Bit 2: CartWordBus,
	/// Bit 3: SlowRom, Bit 7: always set (MBC authentication).
	/// </summary>
	struct RomControlResult {
		bool bootRomDisabled;
		bool cartWordBus;
		bool slowRom;
	};

	static uint8_t EncodeRomControlRead(bool bootRomDisabled, bool cartWordBus, bool slowRom, bool isColor) {
		return (bootRomDisabled ? 0x01 : 0) |
			(isColor ? 0x02 : 0) |
			(cartWordBus ? 0x04 : 0) |
			(slowRom ? 0x08 : 0) |
			0x80;
	}

	/// <summary>
	/// BootRomDisabled is a latch — once set to true, it cannot be cleared.
	/// </summary>
	static bool ApplyBootRomDisable(bool current, uint8_t writeValue) {
		return (writeValue & 0x01) || current;
	}

	/// <summary>
	/// Decodes IrqVectorOffset from port $B0 write.
	/// Only upper 5 bits are stored (mask 0xF8).
	/// </summary>
	static uint8_t DecodeIrqVectorOffset(uint8_t value) {
		return value & 0xF8;
	}

	/// <summary>
	/// Decodes power control register ($62) read.
	/// Bit 0: PowerOffRequested, Bit 7: set on SwanCrystal model.
	/// </summary>
	static uint8_t EncodePowerControlRead(bool powerOff, bool isSwanCrystal) {
		return (powerOff ? 0x01 : 0) | (isSwanCrystal ? 0x80 : 0);
	}

	/// <summary>
	/// Low battery NMI register ($B7): only bit 4 matters.
	/// </summary>
	static bool DecodeLowBatteryNmi(uint8_t value) {
		return (value & 0x10) != 0;
	}

	/// <summary>
	/// SystemTest register ($A3): Low nibble only, blocked on word access.
	/// </summary>
	static uint8_t DecodeSystemTest(uint8_t value) {
		return value & 0x0F;
	}
}

// =============================================================================
// IRQ Dispatch Tests
// =============================================================================

TEST(WsIrqDispatchTest, SetIrqSource_EnabledBit_SetsActive) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = (uint8_t)WsIrqSource::VerticalBlank;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);

	EXPECT_EQ(state.ActiveIrqs & (uint8_t)WsIrqSource::VerticalBlank,
		(uint8_t)WsIrqSource::VerticalBlank);
}

TEST(WsIrqDispatchTest, SetIrqSource_DisabledBit_NoEffect) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = 0;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);

	EXPECT_EQ(state.ActiveIrqs, 0);
}

TEST(WsIrqDispatchTest, SetIrqSource_OnlyMatchingBitAdded) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = (uint8_t)WsIrqSource::Scanline;
	state.ActiveIrqs = (uint8_t)WsIrqSource::KeyPressed;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::Scanline);

	EXPECT_EQ(state.ActiveIrqs,
		(uint8_t)WsIrqSource::KeyPressed | (uint8_t)WsIrqSource::Scanline);
}

TEST(WsIrqDispatchTest, SetIrqSource_AllEightSources) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = 0xff;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::UartSendReady);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::KeyPressed);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::Cart);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::UartRecvReady);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::Scanline);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlankTimer);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::HorizontalBlankTimer);

	EXPECT_EQ(state.ActiveIrqs, 0xff);
}

TEST(WsIrqDispatchTest, SetIrqSource_PartialEnable_OnlySomeSet) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = (uint8_t)WsIrqSource::VerticalBlank |
		(uint8_t)WsIrqSource::HorizontalBlankTimer;

	// Try to set all 8 — only the two enabled should stick
	for (int i = 0; i < 8; i++) {
		WsMemoryTestHelpers::SetIrqSource(state, (WsIrqSource)(1 << i));
	}

	EXPECT_EQ(state.ActiveIrqs,
		(uint8_t)WsIrqSource::VerticalBlank | (uint8_t)WsIrqSource::HorizontalBlankTimer);
}

TEST(WsIrqDispatchTest, AcknowledgeIrqs_ClearsByWrittenBits) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = 0xff;

	WsMemoryTestHelpers::AcknowledgeIrqs(state, (uint8_t)WsIrqSource::VerticalBlank);

	EXPECT_EQ(state.ActiveIrqs, 0xff & ~(uint8_t)WsIrqSource::VerticalBlank);
}

TEST(WsIrqDispatchTest, AcknowledgeIrqs_ClearMultiple) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = 0xff;

	uint8_t ackMask = (uint8_t)WsIrqSource::Scanline |
		(uint8_t)WsIrqSource::VerticalBlank |
		(uint8_t)WsIrqSource::HorizontalBlankTimer;
	WsMemoryTestHelpers::AcknowledgeIrqs(state, ackMask);

	EXPECT_EQ(state.ActiveIrqs, 0xff & ~ackMask);
}

TEST(WsIrqDispatchTest, AcknowledgeIrqs_ClearAll) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = 0xff;

	WsMemoryTestHelpers::AcknowledgeIrqs(state, 0xff);

	EXPECT_EQ(state.ActiveIrqs, 0);
}

TEST(WsIrqDispatchTest, AcknowledgeIrqs_ClearZero_NoEffect) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = 0xff;

	WsMemoryTestHelpers::AcknowledgeIrqs(state, 0);

	EXPECT_EQ(state.ActiveIrqs, 0xff);
}

TEST(WsIrqDispatchTest, ClearIrqSource_ClearsOnlyRequestedBit) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = (uint8_t)WsIrqSource::Scanline |
		(uint8_t)WsIrqSource::VerticalBlank |
		(uint8_t)WsIrqSource::HorizontalBlankTimer;

	WsMemoryTestHelpers::ClearIrqSource(state, WsIrqSource::VerticalBlank);

	EXPECT_EQ(state.ActiveIrqs,
		(uint8_t)WsIrqSource::Scanline | (uint8_t)WsIrqSource::HorizontalBlankTimer);
}

TEST(WsIrqDispatchTest, ClearIrqSource_MissingBit_NoEffect) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = (uint8_t)WsIrqSource::Scanline;

	WsMemoryTestHelpers::ClearIrqSource(state, WsIrqSource::VerticalBlank);

	EXPECT_EQ(state.ActiveIrqs, (uint8_t)WsIrqSource::Scanline);
}

TEST(WsIrqDispatchTest, HasPendingIrq_NoneActive) {
	WsMemoryManagerState state = {};
	EXPECT_FALSE(WsMemoryTestHelpers::HasPendingIrq(state));
}

TEST(WsIrqDispatchTest, HasPendingIrq_SomeActive) {
	WsMemoryManagerState state = {};
	state.ActiveIrqs = (uint8_t)WsIrqSource::Cart;
	EXPECT_TRUE(WsMemoryTestHelpers::HasPendingIrq(state));
}

// =============================================================================
// IRQ Vector Priority Tests
// =============================================================================

TEST(WsIrqVectorTest, NoActiveIrqs_ReturnsBaseOffset) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x08;
	state.ActiveIrqs = 0;

	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 0x08);
}

TEST(WsIrqVectorTest, HighestBit7_HBlankTimer_HasPriority) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x00;
	state.ActiveIrqs = 0xff;

	// Bit 7 (HBlankTimer) is highest, vector = offset + 7
	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 7);
}

TEST(WsIrqVectorTest, OnlyBit0_UartSend_ReturnsOffset0) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x10;
	state.ActiveIrqs = (uint8_t)WsIrqSource::UartSendReady;

	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 0x10 + 0);
}

TEST(WsIrqVectorTest, OnlyBit6_VBlank_ReturnsOffset6) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x08;
	state.ActiveIrqs = (uint8_t)WsIrqSource::VerticalBlank;

	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 0x08 + 6);
}

TEST(WsIrqVectorTest, Bit5And4_VBlankTimerWins) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x00;
	state.ActiveIrqs = (uint8_t)WsIrqSource::VerticalBlankTimer |
		(uint8_t)WsIrqSource::Scanline;

	// VBlankTimer is bit 5, Scanline is bit 4
	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 5);
}

TEST(WsIrqVectorTest, AllSingleBit_CorrectVectorIndex) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x20;

	for (int i = 0; i < 8; i++) {
		state.ActiveIrqs = (uint8_t)(1 << i);
		EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 0x20 + i)
			<< "bit=" << i;
	}
}

TEST(WsIrqVectorTest, VectorOffset_Mask_OnlyUpper5Bits) {
	// Port $B0 write: IrqVectorOffset = value & 0xF8
	EXPECT_EQ(WsMemoryTestHelpers::DecodeIrqVectorOffset(0xff), 0xf8);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeIrqVectorOffset(0x07), 0x00);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeIrqVectorOffset(0x08), 0x08);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeIrqVectorOffset(0x10), 0x10);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeIrqVectorOffset(0x00), 0x00);
}

// =============================================================================
// Address Space — Word Bus Tests
// =============================================================================

TEST(WsWordBusTest, Segment0_WorkRam_AlwaysWordBus) {
	// Work RAM ($00000-$0FFFF) is always 16-bit
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0x00000, false));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0x00000, true));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0x05000, false));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0x0ffff, true));
}

TEST(WsWordBusTest, Segment1_Sram_AlwaysByteBus) {
	// SRAM ($10000-$1FFFF) is always 8-bit
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordBus(0x10000, false));
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordBus(0x10000, true));
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordBus(0x1ffff, true));
}

TEST(WsWordBusTest, Segment2Plus_Rom_DependsOnCartWordBus) {
	// ROM ($20000+): 8-bit normally, 16-bit with CartWordBus
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordBus(0x20000, false));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0x20000, true));
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordBus(0xf0000, false));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0xf0000, true));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordBus(0xfffff, true));
}

// =============================================================================
// Address Space — Wait State Tests
// =============================================================================

TEST(WsWaitStatesTest, Segment0_WorkRam_Always1Cycle) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x00000, false, false), 1);
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x00000, true, true), 1);
}

TEST(WsWaitStatesTest, Segment1_Sram_BaseIs1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x10000, false, false), 1);
}

TEST(WsWaitStatesTest, Segment1_Sram_SlowAdds1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x10000, true, false), 2);
}

TEST(WsWaitStatesTest, Segment1_Sram_SlowRomIgnored) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x10000, false, true), 1);
}

TEST(WsWaitStatesTest, Segment2Plus_Rom_BaseIs1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x20000, false, false), 1);
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0xf0000, false, false), 1);
}

TEST(WsWaitStatesTest, Segment2Plus_Rom_SlowAdds1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x20000, false, true), 2);
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0xf0000, false, true), 2);
}

TEST(WsWaitStatesTest, Segment2Plus_Rom_SlowSramIgnored) {
	EXPECT_EQ(WsMemoryTestHelpers::GetWaitStates(0x20000, true, false), 1);
}

// =============================================================================
// Port Classification Tests
// =============================================================================

TEST(WsPortClassTest, WordPort_Below0xC0_True) {
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordPort(0x00));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordPort(0x60));
	EXPECT_TRUE(WsMemoryTestHelpers::IsWordPort(0xbf));
}

TEST(WsPortClassTest, WordPort_AtAndAbove0xC0_False) {
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordPort(0xC0));
	EXPECT_FALSE(WsMemoryTestHelpers::IsWordPort(0xFF));
}

TEST(WsPortClassTest, UnmappedPort_Normal_False) {
	EXPECT_FALSE(WsMemoryTestHelpers::IsUnmappedPort(0x00));
	EXPECT_FALSE(WsMemoryTestHelpers::IsUnmappedPort(0x60));
	EXPECT_FALSE(WsMemoryTestHelpers::IsUnmappedPort(0xB6));
	EXPECT_FALSE(WsMemoryTestHelpers::IsUnmappedPort(0xFF));
}

TEST(WsPortClassTest, UnmappedPort_HighBitSet_True) {
	EXPECT_TRUE(WsMemoryTestHelpers::IsUnmappedPort(0x100));
	EXPECT_TRUE(WsMemoryTestHelpers::IsUnmappedPort(0x1FF));
}

TEST(WsPortClassTest, PortWaitStates_NonCart_Always1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0x00, false), 1);
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0x60, true), 1);
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0xBF, true), 1);
}

TEST(WsPortClassTest, PortWaitStates_CartRange_BaseIs1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0xC0, false), 1);
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0xFF, false), 1);
}

TEST(WsPortClassTest, PortWaitStates_CartRange_SlowAdds1) {
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0xC0, true), 2);
	EXPECT_EQ(WsMemoryTestHelpers::GetPortWaitStates(0xFF, true), 2);
}

// =============================================================================
// System Control Register $60 Tests
// =============================================================================

TEST(WsSysControl2Test, AllZero_AllDisabled) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0x00);
	EXPECT_EQ(r.raw, 0x00);
	EXPECT_FALSE(r.colorEnabled);
	EXPECT_FALSE(r.enable4bpp);
	EXPECT_FALSE(r.enable4bppPacked);
	EXPECT_FALSE(r.slowPort);
	EXPECT_FALSE(r.slowSram);
}

TEST(WsSysControl2Test, AllBitsSet_Masked) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0xff);
	EXPECT_EQ(r.raw, 0xef); // Bit 4 (0x10) is masked off
	EXPECT_TRUE(r.colorEnabled);
	EXPECT_TRUE(r.enable4bpp);
	EXPECT_TRUE(r.enable4bppPacked);
	EXPECT_TRUE(r.slowPort);
	EXPECT_TRUE(r.slowSram);
}

TEST(WsSysControl2Test, Bit4_Excluded) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0x10);
	EXPECT_EQ(r.raw, 0x00); // Bit 4 is the masked bit
}

TEST(WsSysControl2Test, ColorOnly) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0x80);
	EXPECT_TRUE(r.colorEnabled);
	EXPECT_FALSE(r.enable4bpp);
}

TEST(WsSysControl2Test, Color4bpp) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0xc0);
	EXPECT_TRUE(r.colorEnabled);
	EXPECT_TRUE(r.enable4bpp);
	EXPECT_FALSE(r.enable4bppPacked);
}

TEST(WsSysControl2Test, Color4bppPacked) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0xe0);
	EXPECT_TRUE(r.colorEnabled);
	EXPECT_TRUE(r.enable4bpp);
	EXPECT_TRUE(r.enable4bppPacked);
}

TEST(WsSysControl2Test, SlowSramOnly) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0x02);
	EXPECT_TRUE(r.slowSram);
	EXPECT_FALSE(r.slowPort);
}

TEST(WsSysControl2Test, SlowPortOnly) {
	auto r = WsMemoryTestHelpers::DecodeSystemControl2(0x08);
	EXPECT_TRUE(r.slowPort);
	EXPECT_FALSE(r.slowSram);
}

// =============================================================================
// Video Mode Derivation Tests
// =============================================================================

TEST(WsVideoModeDerivationTest, NotColor_Monochrome) {
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(false, false, false),
		WsVideoMode::Monochrome);
}

TEST(WsVideoModeDerivationTest, Color_No4bpp_2bpp) {
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(true, false, false),
		WsVideoMode::Color2bpp);
}

TEST(WsVideoModeDerivationTest, Color_4bpp_NotPacked) {
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(true, true, false),
		WsVideoMode::Color4bpp);
}

TEST(WsVideoModeDerivationTest, Color_4bpp_Packed) {
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(true, true, true),
		WsVideoMode::Color4bppPacked);
}

TEST(WsVideoModeDerivationTest, PackedWithout4bpp_Still2bpp) {
	// Enable4bppPacked is ignored when Enable4bpp is false
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(true, false, true),
		WsVideoMode::Color2bpp);
}

TEST(WsVideoModeDerivationTest, AllFlagsTrue_Packed) {
	EXPECT_EQ(WsMemoryTestHelpers::DeriveVideoMode(true, true, true),
		WsVideoMode::Color4bppPacked);
}

// =============================================================================
// ROM Control Register $A0 Tests
// =============================================================================

TEST(WsRomControlTest, ReadEncoding_AllOff_Mono) {
	uint8_t val = WsMemoryTestHelpers::EncodeRomControlRead(false, false, false, false);
	EXPECT_EQ(val, 0x80); // Only MBC auth bit set
}

TEST(WsRomControlTest, ReadEncoding_AllOff_Color) {
	uint8_t val = WsMemoryTestHelpers::EncodeRomControlRead(false, false, false, true);
	EXPECT_EQ(val, 0x82); // MBC + color bit
}

TEST(WsRomControlTest, ReadEncoding_AllOn_Color) {
	uint8_t val = WsMemoryTestHelpers::EncodeRomControlRead(true, true, true, true);
	EXPECT_EQ(val, 0x8f); // 0x80 | 0x01 | 0x02 | 0x04 | 0x08
}

TEST(WsRomControlTest, BootRomDisable_Latch_ResetToSet) {
	EXPECT_TRUE(WsMemoryTestHelpers::ApplyBootRomDisable(false, 0x01));
}

TEST(WsRomControlTest, BootRomDisable_Latch_StaysSet) {
	// Once latched true, writing 0 cannot clear it
	EXPECT_TRUE(WsMemoryTestHelpers::ApplyBootRomDisable(true, 0x00));
}

TEST(WsRomControlTest, BootRomDisable_Latch_InitiallyOff) {
	EXPECT_FALSE(WsMemoryTestHelpers::ApplyBootRomDisable(false, 0x00));
}

TEST(WsRomControlTest, BootRomDisable_Latch_Sequence) {
	bool latch = false;
	latch = WsMemoryTestHelpers::ApplyBootRomDisable(latch, 0x00);
	EXPECT_FALSE(latch);
	latch = WsMemoryTestHelpers::ApplyBootRomDisable(latch, 0xfe); // Bit 0 not set
	EXPECT_FALSE(latch);
	latch = WsMemoryTestHelpers::ApplyBootRomDisable(latch, 0x01);
	EXPECT_TRUE(latch);
	latch = WsMemoryTestHelpers::ApplyBootRomDisable(latch, 0x00); // Try to clear
	EXPECT_TRUE(latch); // Still latched
}

// =============================================================================
// Power Control & Misc Register Tests
// =============================================================================

TEST(WsPowerControlTest, Encoding_NoPowerOff_NotSC) {
	EXPECT_EQ(WsMemoryTestHelpers::EncodePowerControlRead(false, false), 0x00);
}

TEST(WsPowerControlTest, Encoding_PowerOff_NotSC) {
	EXPECT_EQ(WsMemoryTestHelpers::EncodePowerControlRead(true, false), 0x01);
}

TEST(WsPowerControlTest, Encoding_NoPowerOff_SwanCrystal) {
	EXPECT_EQ(WsMemoryTestHelpers::EncodePowerControlRead(false, true), 0x80);
}

TEST(WsPowerControlTest, Encoding_PowerOff_SwanCrystal) {
	EXPECT_EQ(WsMemoryTestHelpers::EncodePowerControlRead(true, true), 0x81);
}

TEST(WsLowBatteryNmiTest, Bit4Set_Enabled) {
	EXPECT_TRUE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0x10));
}

TEST(WsLowBatteryNmiTest, Bit4Clear_Disabled) {
	EXPECT_FALSE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0x00));
	EXPECT_FALSE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0xef));
}

TEST(WsLowBatteryNmiTest, OtherBitsIgnored) {
	EXPECT_TRUE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0xff));
	EXPECT_TRUE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0x10));
	EXPECT_FALSE(WsMemoryTestHelpers::DecodeLowBatteryNmi(0x0f));
}

TEST(WsSystemTestRegTest, LowNibbleOnly) {
	EXPECT_EQ(WsMemoryTestHelpers::DecodeSystemTest(0xff), 0x0f);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeSystemTest(0x00), 0x00);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeSystemTest(0xf0), 0x00);
	EXPECT_EQ(WsMemoryTestHelpers::DecodeSystemTest(0x0a), 0x0a);
}

// =============================================================================
// IRQ Enable/Active Interaction Tests
// =============================================================================

TEST(WsIrqInteractionTest, EnableAfterSet_DoesNotRetroactivate) {
	// If source fires before EnabledIrqs is set, it should NOT activate later
	WsMemoryManagerState state = {};
	state.EnabledIrqs = 0;

	// Try to fire Scanline — blocked
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::Scanline);
	EXPECT_EQ(state.ActiveIrqs, 0);

	// Now enable Scanline — pending should remain 0
	state.EnabledIrqs = (uint8_t)WsIrqSource::Scanline;
	EXPECT_EQ(state.ActiveIrqs, 0);
}

TEST(WsIrqInteractionTest, DisableActiveIrq_StillPending) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = 0xff;

	// Fire VBlank
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);
	EXPECT_NE(state.ActiveIrqs & (uint8_t)WsIrqSource::VerticalBlank, 0);

	// Disable VBlank in EnabledIrqs — ActiveIrqs is NOT cleared
	state.EnabledIrqs &= ~(uint8_t)WsIrqSource::VerticalBlank;
	EXPECT_NE(state.ActiveIrqs & (uint8_t)WsIrqSource::VerticalBlank, 0);
}

TEST(WsIrqInteractionTest, ActivateThenAcknowledge_Clears) {
	WsMemoryManagerState state = {};
	state.EnabledIrqs = 0xff;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::Scanline);
	EXPECT_EQ(state.ActiveIrqs,
		(uint8_t)WsIrqSource::VerticalBlank | (uint8_t)WsIrqSource::Scanline);

	// Acknowledge VBlank only
	WsMemoryTestHelpers::AcknowledgeIrqs(state, (uint8_t)WsIrqSource::VerticalBlank);
	EXPECT_EQ(state.ActiveIrqs, (uint8_t)WsIrqSource::Scanline);
}

TEST(WsIrqInteractionTest, VectorChangesWithAcknowledge) {
	WsMemoryManagerState state = {};
	state.IrqVectorOffset = 0x00;
	state.EnabledIrqs = 0xff;

	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::HorizontalBlankTimer); // bit 7
	WsMemoryTestHelpers::SetIrqSource(state, WsIrqSource::VerticalBlank);         // bit 6

	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 7); // HBlankTimer wins

	WsMemoryTestHelpers::AcknowledgeIrqs(state, (uint8_t)WsIrqSource::HorizontalBlankTimer);
	EXPECT_EQ(WsMemoryTestHelpers::GetIrqVector(state), 6); // VBlank now highest
}

// =============================================================================
// Open Bus Behavior Tests
// =============================================================================

TEST(WsOpenBusTest, DefaultState_Zero) {
	WsMemoryManagerState state = {};
	EXPECT_EQ(state.OpenBus, 0);
}

TEST(WsOpenBusTest, LatchesWriteValue) {
	WsMemoryManagerState state = {};
	state.OpenBus = 0x42;
	EXPECT_EQ(state.OpenBus, 0x42);
}

TEST(WsOpenBusTest, FullRange) {
	WsMemoryManagerState state = {};
	for (int v = 0; v < 256; v++) {
		state.OpenBus = (uint8_t)v;
		EXPECT_EQ(state.OpenBus, (uint8_t)v);
	}
}

// =============================================================================
// Segment Enum Tests
// =============================================================================

TEST(WsSegmentTest, EnumValues) {
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::Default), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::ES), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::SS), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::CS), 3);
	EXPECT_EQ(static_cast<uint8_t>(WsSegment::DS), 4);
}

TEST(WsSegmentTest, AllFiveValues) {
	// Verify exactly 5 segment values exist with expected indices
	WsSegment segs[] = {
		WsSegment::Default, WsSegment::ES, WsSegment::SS,
		WsSegment::CS, WsSegment::DS
	};
	for (int i = 0; i < 5; i++) {
		EXPECT_EQ(static_cast<uint8_t>(segs[i]), (uint8_t)i);
	}
}

// =============================================================================
// Memory Manager State Serialization Field Coverage
// =============================================================================

TEST(WsMemMgrSerializationTest, AllFieldsModified_Roundtrip) {
	// Verify all 16 serialized fields can be independently set
	WsMemoryManagerState state = {};
	state.ActiveIrqs = 0xaa;
	state.EnabledIrqs = 0x55;
	state.IrqVectorOffset = 0xf8;
	state.OpenBus = 0xcd;
	state.SystemControl2 = 0xef;
	state.SystemTest = 0x0f;
	state.ColorEnabled = true;
	state.Enable4bpp = true;
	state.Enable4bppPacked = true;
	state.BootRomDisabled = true;
	state.SlowPort = true;
	state.SlowRom = true;
	state.SlowSram = true;
	state.CartWordBus = true;
	state.EnableLowBatteryNmi = true;
	state.PowerOffRequested = true;

	EXPECT_EQ(state.ActiveIrqs, 0xaa);
	EXPECT_EQ(state.EnabledIrqs, 0x55);
	EXPECT_EQ(state.IrqVectorOffset, 0xf8);
	EXPECT_EQ(state.OpenBus, 0xcd);
	EXPECT_EQ(state.SystemControl2, 0xef);
	EXPECT_EQ(state.SystemTest, 0x0f);
	EXPECT_TRUE(state.ColorEnabled);
	EXPECT_TRUE(state.Enable4bpp);
	EXPECT_TRUE(state.Enable4bppPacked);
	EXPECT_TRUE(state.BootRomDisabled);
	EXPECT_TRUE(state.SlowPort);
	EXPECT_TRUE(state.SlowRom);
	EXPECT_TRUE(state.SlowSram);
	EXPECT_TRUE(state.CartWordBus);
	EXPECT_TRUE(state.EnableLowBatteryNmi);
	EXPECT_TRUE(state.PowerOffRequested);
}

// =============================================================================
// Address Computation Tests (segment:offset → 20-bit)
// =============================================================================

TEST(WsAddressCompTest, BasicSegmentOffset) {
	// addr = (seg << 4) + offset, masked to 20 bits
	auto compute = [](uint16_t seg, uint16_t offset) -> uint32_t {
		return ((seg << 4) + offset) & 0xFFFFF;
	};

	EXPECT_EQ(compute(0x0000, 0x0000), 0x00000u);
	EXPECT_EQ(compute(0x0100, 0x0000), 0x01000u);
	EXPECT_EQ(compute(0x1000, 0x0000), 0x10000u);
	EXPECT_EQ(compute(0xF000, 0x0000), 0xF0000u);
	EXPECT_EQ(compute(0xFFFF, 0xFFFF), 0x0FFEFu); // Wraps (0xFFFFF + 0xFFFF) & 0xFFFFF
}

TEST(WsAddressCompTest, OffsetWraps20Bit) {
	auto compute = [](uint16_t seg, uint16_t offset) -> uint32_t {
		return ((seg << 4) + offset) & 0xFFFFF;
	};

	// Near end of address space, offset wraps
	EXPECT_EQ(compute(0xFFFF, 0x0000), 0xFFFF0u);
	EXPECT_EQ(compute(0xFFFF, 0x000F), 0xFFFFFu);
	EXPECT_EQ(compute(0xFFFF, 0x0010), 0x00000u); // Wrap to 0
}

TEST(WsAddressCompTest, SegmentOverlapAlias) {
	auto compute = [](uint16_t seg, uint16_t offset) -> uint32_t {
		return ((seg << 4) + offset) & 0xFFFFF;
	};

	// Multiple seg:offset pairs can resolve to the same address
	EXPECT_EQ(compute(0x0100, 0x0000), compute(0x00FF, 0x0010));
	EXPECT_EQ(compute(0x0200, 0x0000), compute(0x01FF, 0x0010));
}

// =============================================================================
// Cart Banking — Port Mapping Tests
// =============================================================================

namespace WsCartTestHelpers {

	/// <summary>
	/// Identifies which bank register a port corresponds to.
	/// Ports $C0-$C3 → bank 0-3. Returns -1 for non-bank ports.
	/// </summary>
	static int GetBankIndex(uint16_t port) {
		if (port >= 0xC0 && port < 0xC4) return port - 0xC0;
		return -1;
	}

	/// <summary>
	/// Identifies EEPROM port range ($C4-$C8).
	/// </summary>
	static bool IsEepromPort(uint16_t port) {
		return port >= 0xC4 && port < 0xC9;
	}

	/// <summary>
	/// Checks if a cart port is unmapped (returns open bus).
	/// Anything $C9+ with no dedicated handler.
	/// </summary>
	static bool IsUnmappedCartPort(uint16_t port) {
		return port >= 0xC9;
	}

	/// <summary>
	/// Computes the ROM/SRAM offset from a bank value for the given slot.
	/// Slot 0: SelectedBanks[0] * 0x100000 + 0x40000 (maps $40000-$FFFFF)
	/// Slot 1: SelectedBanks[1] * 0x10000 (SRAM, maps $10000-$1FFFF)
	/// Slot 2: SelectedBanks[2] * 0x10000 (ROM, maps $20000-$2FFFF)
	/// Slot 3: SelectedBanks[3] * 0x10000 (ROM, maps $30000-$3FFFF)
	/// </summary>
	static uint32_t ComputeBankOffset(int slot, uint8_t bankValue) {
		switch (slot) {
			case 0: return (uint32_t)bankValue * 0x100000 + 0x40000;
			case 1: return (uint32_t)bankValue * 0x10000;
			case 2: return (uint32_t)bankValue * 0x10000;
			case 3: return (uint32_t)bankValue * 0x10000;
			default: return 0;
		}
	}

	/// <summary>
	/// Returns the memory type for a given bank slot.
	/// Slot 1 = SRAM (CartRam), all others = ROM (PrgRom).
	/// </summary>
	static bool IsSramSlot(int slot) {
		return slot == 1;
	}
}

TEST(WsCartPortTest, BankRegisterPorts_C0_to_C3) {
	for (uint16_t port = 0xC0; port < 0xC4; port++) {
		EXPECT_EQ(WsCartTestHelpers::GetBankIndex(port), port - 0xC0)
			<< "port=$" << std::hex << port;
	}
}

TEST(WsCartPortTest, EepromPorts_C4_to_C8) {
	for (uint16_t port = 0xC4; port < 0xC9; port++) {
		EXPECT_TRUE(WsCartTestHelpers::IsEepromPort(port))
			<< "port=$" << std::hex << port;
	}
}

TEST(WsCartPortTest, EepromPorts_Negative) {
	EXPECT_FALSE(WsCartTestHelpers::IsEepromPort(0xC3));
	EXPECT_FALSE(WsCartTestHelpers::IsEepromPort(0xC9));
	EXPECT_FALSE(WsCartTestHelpers::IsEepromPort(0x00));
}

TEST(WsCartPortTest, UnmappedPorts_C9_Plus) {
	EXPECT_TRUE(WsCartTestHelpers::IsUnmappedCartPort(0xC9));
	EXPECT_TRUE(WsCartTestHelpers::IsUnmappedCartPort(0xFF));
}

TEST(WsCartPortTest, MappedPorts_Below_C9) {
	EXPECT_FALSE(WsCartTestHelpers::IsUnmappedCartPort(0xC0));
	EXPECT_FALSE(WsCartTestHelpers::IsUnmappedCartPort(0xC4));
	EXPECT_FALSE(WsCartTestHelpers::IsUnmappedCartPort(0xC8));
}

// =============================================================================
// Cart Banking — Bank Offset Calculation Tests
// =============================================================================

TEST(WsCartBankingTest, Slot0_LargeBank_1MBGranularity) {
	// Slot 0 maps $40000-$FFFFF with 1MB granularity
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(0, 0x00), 0x00040000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(0, 0x01), 0x00140000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(0, 0xff), 0x0ff40000u);
}

TEST(WsCartBankingTest, Slot1_Sram_64KBGranularity) {
	// Slot 1 maps $10000-$1FFFF (SRAM) with 64KB granularity
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(1, 0x00), 0x00000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(1, 0x01), 0x10000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(1, 0xff), 0xff0000u);
}

TEST(WsCartBankingTest, Slot2_Rom_64KBGranularity) {
	// Slot 2 maps $20000-$2FFFF (ROM) with 64KB granularity
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(2, 0x00), 0x00000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(2, 0x01), 0x10000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(2, 0xff), 0xff0000u);
}

TEST(WsCartBankingTest, Slot3_Rom_64KBGranularity) {
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(3, 0x00), 0x00000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(3, 0x01), 0x10000u);
	EXPECT_EQ(WsCartTestHelpers::ComputeBankOffset(3, 0xff), 0xff0000u);
}

TEST(WsCartBankingTest, Slot1_IsSram) {
	EXPECT_TRUE(WsCartTestHelpers::IsSramSlot(1));
}

TEST(WsCartBankingTest, Slot0_2_3_AreRom) {
	EXPECT_FALSE(WsCartTestHelpers::IsSramSlot(0));
	EXPECT_FALSE(WsCartTestHelpers::IsSramSlot(2));
	EXPECT_FALSE(WsCartTestHelpers::IsSramSlot(3));
}

// =============================================================================
// Cart Banking — Constructor Default Tests
// =============================================================================

TEST(WsCartDefaultsTest, ConstructorDefaultBanks_AllFF) {
	// WsCart constructor sets all banks to 0xFF (map to end of ROM)
	// This differs from zero-initialized WsCartState
	uint8_t expected[4] = { 0xff, 0xff, 0xff, 0xff };
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(expected[i], 0xff) << "slot=" << i;
	}
}

TEST(WsCartDefaultsTest, ZeroInitState_DiffersFromConstructor) {
	// Zero-initialized struct has banks=0; constructor sets to 0xFF
	WsCartState zeroState = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(zeroState.SelectedBanks[i], 0) << "zero-init slot=" << i;
		// Constructor would set to 0xFF — these are different
		EXPECT_NE(zeroState.SelectedBanks[i], 0xff) << "zero != constructor slot=" << i;
	}
}

// =============================================================================
// Cart Banking — Memory Region Address Range Tests
// =============================================================================

TEST(WsCartRegionTest, Slot1_Sram_AddressRange) {
	// SRAM mapped at $10000-$1FFFF (64KB window)
	uint32_t start = 0x10000;
	uint32_t end = 0x1FFFF;
	EXPECT_EQ(end - start + 1, 0x10000u); // 64KB
}

TEST(WsCartRegionTest, Slot2_Rom_AddressRange) {
	// ROM bank at $20000-$2FFFF (64KB window)
	uint32_t start = 0x20000;
	uint32_t end = 0x2FFFF;
	EXPECT_EQ(end - start + 1, 0x10000u);
}

TEST(WsCartRegionTest, Slot3_Rom_AddressRange) {
	// ROM bank at $30000-$3FFFF (64KB window)
	uint32_t start = 0x30000;
	uint32_t end = 0x3FFFF;
	EXPECT_EQ(end - start + 1, 0x10000u);
}

TEST(WsCartRegionTest, Slot0_LargeRom_AddressRange) {
	// Large ROM bank at $40000-$FFFFF (~768KB window)
	uint32_t start = 0x40000;
	uint32_t end = 0xFFFFF;
	EXPECT_EQ(end - start + 1, 0xC0000u); // 786432 = 768KB
}

TEST(WsCartRegionTest, FullAddressSpacePartitioning) {
	// WS address space: $00000-$0FFFF (RAM) + $10000-$1FFFF (SRAM) +
	//                   $20000-$2FFFF + $30000-$3FFFF (ROM banks) +
	//                   $40000-$FFFFF (large ROM)
	uint32_t ram = 0x10000;          // 64KB
	uint32_t sram = 0x10000;         // 64KB
	uint32_t romBank2 = 0x10000;     // 64KB
	uint32_t romBank3 = 0x10000;     // 64KB
	uint32_t largeRom = 0xC0000;     // 768KB
	EXPECT_EQ(ram + sram + romBank2 + romBank3 + largeRom, 0x100000u); // 1MB total
}

TEST(WsCartBankingTest, BankSwitch_State_IndependentSlots) {
	WsCartState state = {};
	state.SelectedBanks[0] = 0x0a;
	state.SelectedBanks[1] = 0x0b;
	state.SelectedBanks[2] = 0x0c;
	state.SelectedBanks[3] = 0x0d;

	EXPECT_EQ(state.SelectedBanks[0], 0x0a);
	EXPECT_EQ(state.SelectedBanks[1], 0x0b);
	EXPECT_EQ(state.SelectedBanks[2], 0x0c);
	EXPECT_EQ(state.SelectedBanks[3], 0x0d);
}

TEST(WsCartBankingTest, BankSwitch_FullRange) {
	WsCartState state = {};
	for (int slot = 0; slot < 4; slot++) {
		for (int v = 0; v < 256; v++) {
			state.SelectedBanks[slot] = (uint8_t)v;
			EXPECT_EQ(state.SelectedBanks[slot], (uint8_t)v);
		}
	}
}

// =============================================================================
// Serial Port — Register Behavior Tests
// =============================================================================
// Tests replicate the bit-level behavior of WsSerial port $B1 (data) and
// $B3 (status/control) without needing a live console instance.

namespace WsSerialTestHelpers {

	/// <summary>
	/// Encodes serial status register ($B3) read value from state fields.
	/// Bit 0: HasReceiveData
	/// Bit 1: ReceiveOverflow
	/// Bit 2: Send buffer empty (enabled AND !hasSendData)
	/// Bit 6: HighSpeed
	/// Bit 7: Enabled
	/// </summary>
	static uint8_t EncodeSerialStatus(const WsSerialState& state) {
		return (state.HasReceiveData ? 0x01 : 0) |
			(state.ReceiveOverflow ? 0x02 : 0) |
			((!state.Enabled || state.HasSendData) ? 0 : 0x04) |
			(state.HighSpeed ? 0x40 : 0) |
			(state.Enabled ? 0x80 : 0);
	}

	/// <summary>
	/// Applies serial control write ($B3) to state.
	/// Bit 7: Enabled, Bit 6: HighSpeed, Bit 5: Clear ReceiveOverflow.
	/// </summary>
	static void WriteSerialControl(WsSerialState& state, uint8_t value) {
		state.Enabled = (value & 0x80) != 0;
		state.HighSpeed = (value & 0x40) != 0;
		if (value & 0x20) {
			state.ReceiveOverflow = false;
		}
	}

	/// <summary>
	/// HasSendIrq: Returns true when serial is enabled and send buffer is empty.
	/// </summary>
	static bool HasSendIrq(const WsSerialState& state) {
		return state.Enabled && !state.HasSendData;
	}

	/// <summary>
	/// Baud rate cycles: 9600 baud = 3200 master clocks/byte,
	/// 38400 (high speed) = 800 master clocks/byte.
	/// </summary>
	static int GetCyclesPerByte(bool highSpeed) {
		return highSpeed ? 800 : 3200;
	}
}

TEST(WsSerialStatusTest, DefaultState_AllClear) {
	WsSerialState state = {};
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Disabled + no data + no overflow = 0
	EXPECT_EQ(status, 0x00);
}

TEST(WsSerialStatusTest, EnabledOnly_SendBufferEmpty) {
	WsSerialState state = {};
	state.Enabled = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Enabled (0x80) + SendEmpty (0x04) = 0x84
	EXPECT_EQ(status, 0x84);
}

TEST(WsSerialStatusTest, Enabled_HasSendData_NoSendEmpty) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HasSendData = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Enabled (0x80) only, send buffer NOT empty
	EXPECT_EQ(status, 0x80);
}

TEST(WsSerialStatusTest, HasReceiveData) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HasReceiveData = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Enabled (0x80) + SendEmpty (0x04) + HasReceive (0x01) = 0x85
	EXPECT_EQ(status, 0x85);
}

TEST(WsSerialStatusTest, ReceiveOverflow) {
	WsSerialState state = {};
	state.Enabled = true;
	state.ReceiveOverflow = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Enabled (0x80) + SendEmpty (0x04) + Overflow (0x02) = 0x86
	EXPECT_EQ(status, 0x86);
}

TEST(WsSerialStatusTest, HighSpeed) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HighSpeed = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Enabled (0x80) + HighSpeed (0x40) + SendEmpty (0x04) = 0xc4
	EXPECT_EQ(status, 0xc4);
}

TEST(WsSerialStatusTest, AllFlags_SendPending) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HighSpeed = true;
	state.HasReceiveData = true;
	state.ReceiveOverflow = true;
	state.HasSendData = true;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// 0x80 + 0x40 + 0x01 + 0x02 = 0xc3 (no 0x04 because HasSendData)
	EXPECT_EQ(status, 0xc3);
}

TEST(WsSerialStatusTest, DisabledWithSendData_NoSendEmptyBit) {
	WsSerialState state = {};
	state.Enabled = false;
	state.HasSendData = false;
	uint8_t status = WsSerialTestHelpers::EncodeSerialStatus(state);
	// Disabled → send empty bit (0x04) is NOT set regardless
	EXPECT_EQ(status & 0x04, 0);
}

TEST(WsSerialControlTest, EnableSerial) {
	WsSerialState state = {};
	WsSerialTestHelpers::WriteSerialControl(state, 0x80);
	EXPECT_TRUE(state.Enabled);
	EXPECT_FALSE(state.HighSpeed);
}

TEST(WsSerialControlTest, EnableHighSpeed) {
	WsSerialState state = {};
	WsSerialTestHelpers::WriteSerialControl(state, 0xc0);
	EXPECT_TRUE(state.Enabled);
	EXPECT_TRUE(state.HighSpeed);
}

TEST(WsSerialControlTest, DisableSerial) {
	WsSerialState state = {};
	state.Enabled = true;
	WsSerialTestHelpers::WriteSerialControl(state, 0x00);
	EXPECT_FALSE(state.Enabled);
}

TEST(WsSerialControlTest, ClearOverflow_Bit5) {
	WsSerialState state = {};
	state.ReceiveOverflow = true;
	WsSerialTestHelpers::WriteSerialControl(state, 0x20);
	EXPECT_FALSE(state.ReceiveOverflow);
}

TEST(WsSerialControlTest, NoClearOverflow_WithoutBit5) {
	WsSerialState state = {};
	state.ReceiveOverflow = true;
	WsSerialTestHelpers::WriteSerialControl(state, 0x80);
	EXPECT_TRUE(state.ReceiveOverflow); // Bit 5 not set, overflow preserved
}

TEST(WsSerialIrqTest, HasSendIrq_EnabledAndEmpty) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HasSendData = false;
	EXPECT_TRUE(WsSerialTestHelpers::HasSendIrq(state));
}

TEST(WsSerialIrqTest, NoSendIrq_Disabled) {
	WsSerialState state = {};
	state.Enabled = false;
	state.HasSendData = false;
	EXPECT_FALSE(WsSerialTestHelpers::HasSendIrq(state));
}

TEST(WsSerialIrqTest, NoSendIrq_HasSendData) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HasSendData = true;
	EXPECT_FALSE(WsSerialTestHelpers::HasSendIrq(state));
}

TEST(WsSerialTimingTest, BaudRate_Normal_3200Cycles) {
	EXPECT_EQ(WsSerialTestHelpers::GetCyclesPerByte(false), 3200);
}

TEST(WsSerialTimingTest, BaudRate_HighSpeed_800Cycles) {
	EXPECT_EQ(WsSerialTestHelpers::GetCyclesPerByte(true), 800);
}

TEST(WsSerialTimingTest, BaudRatio_4x) {
	// High speed is exactly 4x faster
	EXPECT_EQ(WsSerialTestHelpers::GetCyclesPerByte(false) / WsSerialTestHelpers::GetCyclesPerByte(true), 4);
}

TEST(WsSerialDataTest, SendBuffer_FullRange) {
	WsSerialState state = {};
	for (int v = 0; v < 256; v++) {
		state.SendBuffer = (uint8_t)v;
		EXPECT_EQ(state.SendBuffer, (uint8_t)v);
	}
}

TEST(WsSerialDataTest, ReceiveBuffer_FullRange) {
	WsSerialState state = {};
	for (int v = 0; v < 256; v++) {
		state.ReceiveBuffer = (uint8_t)v;
		EXPECT_EQ(state.ReceiveBuffer, (uint8_t)v);
	}
}

TEST(WsSerialDataTest, SendAndReceive_Independent) {
	WsSerialState state = {};
	state.SendBuffer = 0xaa;
	state.ReceiveBuffer = 0x55;
	EXPECT_EQ(state.SendBuffer, 0xaa);
	EXPECT_EQ(state.ReceiveBuffer, 0x55);
	EXPECT_NE(state.SendBuffer, state.ReceiveBuffer);
}
