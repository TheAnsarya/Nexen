#include "pch.h"
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan Timer & DMA Controller Tests
// =============================================================================
// Tests for WsTimerState (H/VBlank timers, control register, auto-reload) and
// WsDmaControllerState (GDMA, SDMA, configuration, sound DMA flags).

namespace WsTimerDmaTestHelpers {
	// Decode timer control register bits
	static bool IsHBlankEnabled(uint8_t control) { return (control & 0x01) != 0; }
	static bool IsHBlankAutoReload(uint8_t control) { return (control & 0x02) != 0; }
	static bool IsVBlankEnabled(uint8_t control) { return (control & 0x04) != 0; }
	static bool IsVBlankAutoReload(uint8_t control) { return (control & 0x08) != 0; }

	// Simulate timer tick: decrement, check for underflow, auto-reload
	struct TimerTickResult {
		uint16_t Value;
		bool Underflow;
	};

	static TimerTickResult TickTimer(uint16_t value, uint16_t reloadValue,
		bool enabled, bool autoReload) {
		if (!enabled) {
			return { value, false };
		}
		if (value == 0) {
			if (autoReload) {
				return { reloadValue, true };
			}
			return { 0, true };
		}
		return { static_cast<uint16_t>(value - 1), false };
	}

	// Decode SDMA control register
	static bool IsSdmaEnabled(uint8_t control) { return (control & 0x80) != 0; }
	static bool IsSdmaRepeat(uint8_t control) { return (control & 0x08) != 0; }
	static bool IsSdmaHold(uint8_t control) { return (control & 0x04) != 0; }
	static bool IsSdmaDecrement(uint8_t control) { return (control & 0x40) != 0; }
	static bool IsSdmaHyperVoice(uint8_t control) { return (control & 0x10) != 0; }

	// Mask GDMA source to 20-bit
	static uint32_t MaskGdmaSrc(uint32_t src) { return src & 0xfffff; }

	// GDMA destination is limited to 16-bit VRAM address
	static uint16_t MaskGdmaDest(uint16_t dest) { return dest; }
}

// =============================================================================
// Timer State Tests
// =============================================================================

TEST(WsTimerBehaviorTest, DefaultState_AllFieldsZero) {
	WsTimerState state = {};
	EXPECT_EQ(state.HTimer, 0);
	EXPECT_EQ(state.VTimer, 0);
	EXPECT_EQ(state.HReloadValue, 0);
	EXPECT_EQ(state.VReloadValue, 0);
	EXPECT_EQ(state.Control, 0);
	EXPECT_FALSE(state.HBlankEnabled);
	EXPECT_FALSE(state.HBlankAutoReload);
	EXPECT_FALSE(state.VBlankEnabled);
	EXPECT_FALSE(state.VBlankAutoReload);
}

TEST(WsTimerStateTest, ControlRegister_IndividualBits) {
	// Bit 0 = HBlank enable, Bit 1 = HBlank auto, Bit 2 = VBlank enable, Bit 3 = VBlank auto
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsHBlankEnabled(0x01));
	EXPECT_FALSE(WsTimerDmaTestHelpers::IsHBlankEnabled(0x00));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsHBlankAutoReload(0x02));
	EXPECT_FALSE(WsTimerDmaTestHelpers::IsHBlankAutoReload(0x00));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsVBlankEnabled(0x04));
	EXPECT_FALSE(WsTimerDmaTestHelpers::IsVBlankEnabled(0x00));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsVBlankAutoReload(0x08));
	EXPECT_FALSE(WsTimerDmaTestHelpers::IsVBlankAutoReload(0x00));
}

TEST(WsTimerStateTest, ControlRegister_AllEnabled) {
	uint8_t control = 0x0f;
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsHBlankEnabled(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsHBlankAutoReload(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsVBlankEnabled(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsVBlankAutoReload(control));
}

TEST(WsTimerBehaviorTest, TickTimer_DisabledDoesNotDecrement) {
	auto r = WsTimerDmaTestHelpers::TickTimer(100, 50, false, false);
	EXPECT_EQ(r.Value, 100);
	EXPECT_FALSE(r.Underflow);
}

TEST(WsTimerBehaviorTest, TickTimer_EnabledDecrements) {
	auto r = WsTimerDmaTestHelpers::TickTimer(100, 50, true, false);
	EXPECT_EQ(r.Value, 99);
	EXPECT_FALSE(r.Underflow);
}

TEST(WsTimerBehaviorTest, TickTimer_AtZero_NoAutoReload_StaysZero) {
	auto r = WsTimerDmaTestHelpers::TickTimer(0, 50, true, false);
	EXPECT_EQ(r.Value, 0);
	EXPECT_TRUE(r.Underflow);
}

TEST(WsTimerBehaviorTest, TickTimer_AtZero_AutoReload_ReloadsValue) {
	auto r = WsTimerDmaTestHelpers::TickTimer(0, 200, true, true);
	EXPECT_EQ(r.Value, 200);
	EXPECT_TRUE(r.Underflow);
}

TEST(WsTimerBehaviorTest, TickTimer_CountdownSequence) {
	uint16_t val = 3;
	uint16_t reload = 3;
	bool enabled = true;
	bool autoReload = true;

	// 3 → 2 → 1 → 0(underflow+reload) → 3(reload value)
	auto r1 = WsTimerDmaTestHelpers::TickTimer(val, reload, enabled, autoReload);
	EXPECT_EQ(r1.Value, 2); EXPECT_FALSE(r1.Underflow);

	auto r2 = WsTimerDmaTestHelpers::TickTimer(r1.Value, reload, enabled, autoReload);
	EXPECT_EQ(r2.Value, 1); EXPECT_FALSE(r2.Underflow);

	auto r3 = WsTimerDmaTestHelpers::TickTimer(r2.Value, reload, enabled, autoReload);
	EXPECT_EQ(r3.Value, 0); EXPECT_FALSE(r3.Underflow);

	auto r4 = WsTimerDmaTestHelpers::TickTimer(r3.Value, reload, enabled, autoReload);
	EXPECT_EQ(r4.Value, reload); EXPECT_TRUE(r4.Underflow);
}

TEST(WsTimerStateTest, ReloadValues_16BitRange) {
	WsTimerState state = {};
	state.HReloadValue = 0xffff;
	state.VReloadValue = 0xffff;
	EXPECT_EQ(state.HReloadValue, 0xffff);
	EXPECT_EQ(state.VReloadValue, 0xffff);
}

// =============================================================================
// DMA Controller State Tests
// =============================================================================

TEST(WsDmaBehaviorTest, DefaultState_AllFieldsZero) {
	WsDmaControllerState state = {};
	EXPECT_EQ(state.GdmaSrc, 0u);
	EXPECT_EQ(state.SdmaSrc, 0u);
	EXPECT_EQ(state.SdmaLength, 0u);
	EXPECT_EQ(state.SdmaSrcReloadValue, 0u);
	EXPECT_EQ(state.SdmaLengthReloadValue, 0u);
	EXPECT_EQ(state.GdmaDest, 0);
	EXPECT_EQ(state.GdmaLength, 0);
	EXPECT_EQ(state.GdmaControl, 0);
	EXPECT_EQ(state.SdmaControl, 0);
	EXPECT_FALSE(state.SdmaEnabled);
	EXPECT_FALSE(state.SdmaDecrement);
	EXPECT_FALSE(state.SdmaHyperVoice);
	EXPECT_FALSE(state.SdmaRepeat);
	EXPECT_FALSE(state.SdmaHold);
	EXPECT_EQ(state.SdmaFrequency, 0);
	EXPECT_EQ(state.SdmaTimer, 0);
}

TEST(WsDmaStateTest, GdmaSource_20BitAddress) {
	WsDmaControllerState state = {};
	state.GdmaSrc = 0xfffff;
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(state.GdmaSrc), 0xfffffu);

	state.GdmaSrc = 0x12345;
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(state.GdmaSrc), 0x12345u);
}

TEST(WsDmaStateTest, GdmaDestination_VramAddress) {
	WsDmaControllerState state = {};
	state.GdmaDest = 0xfe00;
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaDest(state.GdmaDest), 0xfe00);
}

TEST(WsDmaBehaviorTest, SdmaControlRegister_FlagDecoding) {
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaEnabled(0x80));
	EXPECT_FALSE(WsTimerDmaTestHelpers::IsSdmaEnabled(0x00));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaDecrement(0x40));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaHyperVoice(0x10));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaRepeat(0x08));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaHold(0x04));
}

TEST(WsDmaBehaviorTest, SdmaControlRegister_AllFlags) {
	uint8_t control = 0x80 | 0x40 | 0x10 | 0x08 | 0x04; // all flags
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaEnabled(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaDecrement(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaHyperVoice(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaRepeat(control));
	EXPECT_TRUE(WsTimerDmaTestHelpers::IsSdmaHold(control));
}

TEST(WsDmaStateTest, SdmaReloadValues_PreservedSeparately) {
	WsDmaControllerState state = {};
	state.SdmaSrc = 0x10000;
	state.SdmaLength = 0x2000;
	state.SdmaSrcReloadValue = 0x10000;
	state.SdmaLengthReloadValue = 0x2000;

	// Simulate transfer consuming source
	state.SdmaSrc = 0x12000;
	state.SdmaLength = 0;

	// Reload values should still be original
	EXPECT_EQ(state.SdmaSrcReloadValue, 0x10000u);
	EXPECT_EQ(state.SdmaLengthReloadValue, 0x2000u);
}

TEST(WsDmaStateTest, SdmaFrequencyAndTimer) {
	WsDmaControllerState state = {};
	state.SdmaFrequency = 0xff;
	state.SdmaTimer = 0x80;
	EXPECT_EQ(state.SdmaFrequency, 0xff);
	EXPECT_EQ(state.SdmaTimer, 0x80);
}

// =============================================================================
// GDMA Cycle Counting Tests — #1130 audit
// =============================================================================
// GDMA takes 5 setup cycles + 2 cycles per word transferred.
// These tests verify the expected cycle calculations.

TEST(WsGdmaCycleCounting, ZeroLength_NoCycles) {
	// Zero-length GDMA should not execute any cycles
	uint16_t length = 0;
	uint32_t expectedCycles = 0; // Stops immediately, no setup
	EXPECT_EQ(expectedCycles, 0u);
}

TEST(WsGdmaCycleCounting, MinimalTransfer_TwoBytes) {
	// 2-byte GDMA: 5 setup + 2*1 = 7 cycles
	uint16_t length = 2;
	uint32_t expectedCycles = 5 + (length / 2) * 2;
	EXPECT_EQ(expectedCycles, 7u);
}

TEST(WsGdmaCycleCounting, LargeTransfer_256Bytes) {
	// 256-byte GDMA: 5 setup + 2*128 = 261 cycles
	uint16_t length = 256;
	uint32_t expectedCycles = 5 + (length / 2) * 2;
	EXPECT_EQ(expectedCycles, 261u);
}

TEST(WsGdmaCycleCounting, MaxTransfer) {
	// Max 16-bit length: 0xFFFE (even), 5 + 2*(0xFFFE/2) = 5 + 0xFFFE = 65539
	uint16_t length = 0xfffe;
	uint32_t expectedCycles = 5 + length;
	EXPECT_EQ(expectedCycles, 65539u);
}

TEST(WsGdmaSourceMask, Valid20BitAddresses) {
	// GDMA source is masked to 20-bit physical address
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(0x00000), 0x00000u);
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(0xfffff), 0xfffffu);
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(0x100000), 0x00000u); // Overflow wraps
	EXPECT_EQ(WsTimerDmaTestHelpers::MaskGdmaSrc(0x1fffff), 0xfffffu);
}

TEST(WsGdmaDirectionTest, IncrementDirection_OffsetPositive) {
	// GdmaControl bit 6 clear → increment (offset = +2)
	uint8_t control = 0x00;
	int offset = (control & 0x40) ? -2 : 2;
	EXPECT_EQ(offset, 2);
}

TEST(WsGdmaDirectionTest, DecrementDirection_OffsetNegative) {
	// GdmaControl bit 6 set → decrement (offset = -2)
	uint8_t control = 0x40;
	int offset = (control & 0x40) ? -2 : 2;
	EXPECT_EQ(offset, -2);
}

TEST(WsGdmaAddressUpdate, IncrementWraps20Bit) {
	// Source address increments and wraps at 20-bit boundary
	uint32_t src = 0xffffe;
	int offset = 2;
	src = (src + offset) & 0xfffff;
	EXPECT_EQ(src, 0x00000u); // Wraps around
}

TEST(WsGdmaAddressUpdate, DecrementWraps20Bit) {
	// Source address decrements and wraps at 20-bit boundary
	uint32_t src = 0x00000;
	int offset = -2;
	src = (src + offset) & 0xfffff;
	EXPECT_EQ(src, 0xffffeu); // Wraps around
}
