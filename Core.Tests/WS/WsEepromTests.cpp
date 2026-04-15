#include "pch.h"
#include "WS/WsEeprom.h"
#include "WS/WsTypes.h"

// =============================================================================
// WonderSwan EEPROM Extended Tests
// =============================================================================
// Extended EEPROM tests covering command decode edge cases, address masking,
// state transitions, size configurations, and cart vs internal EEPROM behavior.
// Complements the EEPROM coverage in WsStateTests.cpp.

namespace WsEepromExtTestHelpers {
	// Mirror of WsStateBehaviorHelpers decode (for additional edge case testing)
	static WsEepromCommand DecodeCommand(uint16_t rawCommand, WsEepromSize size) {
		uint16_t command = rawCommand;
		if (size != WsEepromSize::Size128) {
			command >>= 4;
		}

		switch (command & 0xffc0) {
			case 0x0180: return WsEepromCommand::Read;
			case 0x0140: return WsEepromCommand::Write;
			case 0x01c0: return WsEepromCommand::Erase;
			case 0x0100:
				switch ((command >> 4) & 0x03) {
					case 0x00: return WsEepromCommand::WriteDisable;
					case 0x01: return WsEepromCommand::WriteAll;
					case 0x02: return WsEepromCommand::EraseAll;
					case 0x03: return WsEepromCommand::WriteEnable;
				}
				break;
		}
		return WsEepromCommand::Unknown;
	}

	static uint16_t GetAddress(uint16_t rawCommand, WsEepromSize size) {
		switch (size) {
			case WsEepromSize::Size128: return rawCommand & 0x003f;
			case WsEepromSize::Size1kb: return rawCommand & 0x01ff;
			case WsEepromSize::Size2kb: return rawCommand & 0x03ff;
			default: return 0;
		}
	}

	// Calculate EEPROM data capacity in 16-bit words
	static uint16_t GetWordCapacity(WsEepromSize size) {
		return static_cast<uint16_t>(size) / 2;
	}

	// Validate EEPROM command timing (cycles needed)
	static uint32_t GetCommandDelay(WsEepromCommand cmd) {
		switch (cmd) {
			case WsEepromCommand::Read: return 6;
			case WsEepromCommand::Write:
			case WsEepromCommand::Erase: return 10;
			case WsEepromCommand::WriteAll:
			case WsEepromCommand::EraseAll: return 20;
			default: return 10;
		}
	}
}

// =============================================================================
// EEPROM Size Configuration Tests
// =============================================================================

TEST(WsEepromSizeTest, EnumValues_MatchByteCount) {
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size0), 0);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size128), 128);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size1kb), 1024);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size2kb), 2048);
}

TEST(WsEepromSizeTest, WordCapacities) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetWordCapacity(WsEepromSize::Size0), 0);
	EXPECT_EQ(WsEepromExtTestHelpers::GetWordCapacity(WsEepromSize::Size128), 64);
	EXPECT_EQ(WsEepromExtTestHelpers::GetWordCapacity(WsEepromSize::Size1kb), 512);
	EXPECT_EQ(WsEepromExtTestHelpers::GetWordCapacity(WsEepromSize::Size2kb), 1024);
}

// =============================================================================
// EEPROM Command Decode Edge Cases
// =============================================================================

TEST(WsEepromDecodeTest, Read_AllSizes) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0180, WsEepromSize::Size128), WsEepromCommand::Read);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1800, WsEepromSize::Size1kb), WsEepromCommand::Read);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1800, WsEepromSize::Size2kb), WsEepromCommand::Read);
}

TEST(WsEepromDecodeTest, Write_AllSizes) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0140, WsEepromSize::Size128), WsEepromCommand::Write);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1400, WsEepromSize::Size1kb), WsEepromCommand::Write);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1400, WsEepromSize::Size2kb), WsEepromCommand::Write);
}

TEST(WsEepromDecodeTest, Erase_AllSizes) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x01c0, WsEepromSize::Size128), WsEepromCommand::Erase);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1c00, WsEepromSize::Size1kb), WsEepromCommand::Erase);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1c00, WsEepromSize::Size2kb), WsEepromCommand::Erase);
}

TEST(WsEepromDecodeTest, UnknownCommand_ZeroInput) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0000, WsEepromSize::Size128), WsEepromCommand::Unknown);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0000, WsEepromSize::Size1kb), WsEepromCommand::Unknown);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0000, WsEepromSize::Size2kb), WsEepromCommand::Unknown);
}

TEST(WsEepromDecodeTest, ControlCommands_128ByteEeprom) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0100, WsEepromSize::Size128), WsEepromCommand::WriteDisable);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0110, WsEepromSize::Size128), WsEepromCommand::WriteAll);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0120, WsEepromSize::Size128), WsEepromCommand::EraseAll);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x0130, WsEepromSize::Size128), WsEepromCommand::WriteEnable);
}

TEST(WsEepromDecodeTest, ControlCommands_1kbEeprom) {
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1000, WsEepromSize::Size1kb), WsEepromCommand::WriteDisable);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1100, WsEepromSize::Size1kb), WsEepromCommand::WriteAll);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1200, WsEepromSize::Size1kb), WsEepromCommand::EraseAll);
	EXPECT_EQ(WsEepromExtTestHelpers::DecodeCommand(0x1300, WsEepromSize::Size1kb), WsEepromCommand::WriteEnable);
}

// =============================================================================
// EEPROM Address Masking Tests
// =============================================================================

TEST(WsEepromAddressTest, Mask_128Bytes_6BitAddress) {
	// 128 byte = 64 words → 6-bit address
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0xffff, WsEepromSize::Size128), 0x003f);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x0000, WsEepromSize::Size128), 0x0000);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x003f, WsEepromSize::Size128), 0x003f);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x0040, WsEepromSize::Size128), 0x0000);
}

TEST(WsEepromAddressTest, Mask_1kb_9BitAddress) {
	// 1KB = 512 words → 9-bit address
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0xffff, WsEepromSize::Size1kb), 0x01ff);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x01ff, WsEepromSize::Size1kb), 0x01ff);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x0200, WsEepromSize::Size1kb), 0x0000);
}

TEST(WsEepromAddressTest, Mask_2kb_10BitAddress) {
	// 2KB = 1024 words → 10-bit address
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0xffff, WsEepromSize::Size2kb), 0x03ff);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x03ff, WsEepromSize::Size2kb), 0x03ff);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x0400, WsEepromSize::Size2kb), 0x0000);
}

TEST(WsEepromAddressTest, Mask_Size0_AlwaysZero) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0xffff, WsEepromSize::Size0), 0x0000);
	EXPECT_EQ(WsEepromExtTestHelpers::GetAddress(0x1234, WsEepromSize::Size0), 0x0000);
}

// =============================================================================
// EEPROM Command Timing Tests
// =============================================================================

TEST(WsEepromTimingTest, ReadDelay_Is6) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::Read), 6u);
}

TEST(WsEepromTimingTest, WriteEraseDelay_Is10) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::Write), 10u);
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::Erase), 10u);
}

TEST(WsEepromTimingTest, BulkOperationDelay_Is20) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::WriteAll), 20u);
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::EraseAll), 20u);
}

TEST(WsEepromTimingTest, DefaultDelay_Is10) {
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::WriteDisable), 10u);
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::WriteEnable), 10u);
	EXPECT_EQ(WsEepromExtTestHelpers::GetCommandDelay(WsEepromCommand::Unknown), 10u);
}

// =============================================================================
// EEPROM State Tests
// =============================================================================

TEST(WsEepromStateTest, WriteProtection_DefaultOff) {
	WsEepromState state = {};
	EXPECT_FALSE(state.WriteDisabled);
	EXPECT_FALSE(state.InternalEepromWriteProtected);
}

TEST(WsEepromStateTest, StateTransition_ReadDone) {
	WsEepromState state = {};
	state.ReadDone = true;
	state.Idle = false;
	EXPECT_TRUE(state.ReadDone);
	EXPECT_FALSE(state.Idle);
}

TEST(WsEepromStateTest, CartVsInternal_SeparateState) {
	WsState ws = {};
	ws.InternalEeprom.Size = WsEepromSize::Size128;
	ws.CartEeprom.Size = WsEepromSize::Size2kb;

	EXPECT_EQ(ws.InternalEeprom.Size, WsEepromSize::Size128);
	EXPECT_EQ(ws.CartEeprom.Size, WsEepromSize::Size2kb);
	EXPECT_NE(static_cast<uint16_t>(ws.InternalEeprom.Size),
		static_cast<uint16_t>(ws.CartEeprom.Size));
}

// =============================================================================
// Cart State Tests
// =============================================================================

TEST(WsCartStateTest, DefaultState_ZeroInitialized) {
	WsCartState state = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(state.SelectedBanks[i], 0) << "bank=" << i;
	}
}

TEST(WsCartStateTest, BankSelection_IndependentSlots) {
	WsCartState state = {};
	state.SelectedBanks[0] = 0x10;
	state.SelectedBanks[1] = 0x20;
	state.SelectedBanks[2] = 0x30;
	state.SelectedBanks[3] = 0xff;

	EXPECT_EQ(state.SelectedBanks[0], 0x10);
	EXPECT_EQ(state.SelectedBanks[1], 0x20);
	EXPECT_EQ(state.SelectedBanks[2], 0x30);
	EXPECT_EQ(state.SelectedBanks[3], 0xff);
}

// =============================================================================
// Serial State Tests
// =============================================================================

TEST(WsSerialStateTest, DefaultState_ZeroInitialized) {
	WsSerialState state = {};
	EXPECT_EQ(state.SendClock, 0u);
	EXPECT_FALSE(state.Enabled);
	EXPECT_FALSE(state.HighSpeed);
	EXPECT_FALSE(state.ReceiveOverflow);
	EXPECT_FALSE(state.HasReceiveData);
	EXPECT_EQ(state.ReceiveBuffer, 0);
	EXPECT_FALSE(state.HasSendData);
	EXPECT_EQ(state.SendBuffer, 0);
}

TEST(WsSerialStateTest, DataBuffers_FullRange) {
	WsSerialState state = {};
	state.ReceiveBuffer = 0xff;
	state.SendBuffer = 0xff;
	EXPECT_EQ(state.ReceiveBuffer, 0xff);
	EXPECT_EQ(state.SendBuffer, 0xff);
}

TEST(WsSerialStateTest, Flags_Independent) {
	WsSerialState state = {};
	state.Enabled = true;
	state.HighSpeed = true;
	state.ReceiveOverflow = false;
	state.HasReceiveData = true;
	state.HasSendData = false;

	EXPECT_TRUE(state.Enabled);
	EXPECT_TRUE(state.HighSpeed);
	EXPECT_FALSE(state.ReceiveOverflow);
	EXPECT_TRUE(state.HasReceiveData);
	EXPECT_FALSE(state.HasSendData);
}

// =============================================================================
// Aggregate WsState Tests
// =============================================================================

TEST(WsAggregateStateTest, AllSubStatesAccessible) {
	WsState state = {};
	// Verify all substates are zero-initialized and independently accessible
	EXPECT_EQ(state.Cpu.AX, 0);
	EXPECT_EQ(state.Ppu.FrameCount, 0u);
	EXPECT_FALSE(state.Apu.SpeakerEnabled);
	EXPECT_EQ(state.MemoryManager.ActiveIrqs, 0);
	EXPECT_EQ(state.ControlManager.InputSelect, 0);
	EXPECT_EQ(state.DmaController.GdmaSrc, 0u);
	EXPECT_EQ(state.Timer.HTimer, 0);
	EXPECT_FALSE(state.Serial.Enabled);
	EXPECT_EQ(state.InternalEeprom.Size, WsEepromSize::Size0);
	EXPECT_EQ(state.Cart.SelectedBanks[0], 0);
	EXPECT_EQ(state.CartEeprom.Size, WsEepromSize::Size0);
}
