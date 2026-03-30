#include "pch.h"
#include "WS/WsEeprom.h"
#include "WS/WsTypes.h"

namespace WsStateBehaviorHelpers {
	static WsEepromCommand DecodeEepromCommand(uint16_t rawCommand, WsEepromSize size) {
		uint16_t command = rawCommand;
		if (size != WsEepromSize::Size128) {
			command >>= 4;
		}

		switch (command & 0xffc0) {
			case 0x0180:
				return WsEepromCommand::Read;
			case 0x0140:
				return WsEepromCommand::Write;
			case 0x01c0:
				return WsEepromCommand::Erase;
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

	static uint16_t GetEepromCommandAddress(uint16_t rawCommand, WsEepromSize size) {
		switch (size) {
			case WsEepromSize::Size128:
				return rawCommand & 0x003f;
			case WsEepromSize::Size1kb:
				return rawCommand & 0x01ff;
			case WsEepromSize::Size2kb:
				return rawCommand & 0x03ff;
			default:
				return 0;
		}
	}

	static uint32_t GetEepromCommandDelay(WsEepromCommand cmd) {
		switch (cmd) {
			case WsEepromCommand::Read:
				return 6;
			case WsEepromCommand::Write:
			case WsEepromCommand::Erase:
				return 10;
			case WsEepromCommand::WriteAll:
			case WsEepromCommand::EraseAll:
				return 20;
			default:
				return 10;
		}
	}

	static uint8_t ReadSystemTestPortBehavior(uint8_t openBusValue, uint8_t systemTestValue) {
		(void)systemTestValue;
		// Port 0xA3 is write-only and reads as open bus.
		return openBusValue;
	}

	static bool IsWordBusBehavior(uint32_t addr, bool cartWordBus) {
		switch (addr >> 16) {
			case 0:
				return true;
			case 1:
				return false;
			default:
				return cartWordBus;
		}
	}
}

// =============================================================================
// WonderSwan Core State Tests
// =============================================================================
// Focused state and helper coverage for non-PPU WS types used across CPU/memory/
// input/timer/audio/serial/eeprom paths.

TEST(WsStateMemoryManagerTest, DefaultState_ZeroInitialized) {
	WsMemoryManagerState state = {};
	EXPECT_EQ(state.ActiveIrqs, 0);
	EXPECT_EQ(state.EnabledIrqs, 0);
	EXPECT_EQ(state.IrqVectorOffset, 0);
	EXPECT_EQ(state.OpenBus, 0);
	EXPECT_EQ(state.SystemControl2, 0);
	EXPECT_EQ(state.SystemTest, 0);
	EXPECT_FALSE(state.ColorEnabled);
	EXPECT_FALSE(state.Enable4bpp);
	EXPECT_FALSE(state.Enable4bppPacked);
	EXPECT_FALSE(state.BootRomDisabled);
	EXPECT_FALSE(state.CartWordBus);
	EXPECT_FALSE(state.SlowRom);
	EXPECT_FALSE(state.SlowSram);
	EXPECT_FALSE(state.SlowPort);
	EXPECT_FALSE(state.EnableLowBatteryNmi);
	EXPECT_FALSE(state.PowerOffRequested);
}

TEST(WsStateControlManagerTest, DefaultState_ZeroInitialized) {
	WsControlManagerState state = {};
	EXPECT_EQ(state.InputSelect, 0);
}

TEST(WsStateTimerTest, DefaultState_ZeroInitialized) {
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

TEST(WsStateSerialTest, DefaultState_ZeroInitialized) {
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

TEST(WsStateEepromTest, DefaultState_ZeroInitialized) {
	WsEepromState state = {};
	EXPECT_EQ(state.CmdStartClock, 0u);
	EXPECT_EQ(state.Size, WsEepromSize::Size0);
	EXPECT_EQ(state.ReadBuffer, 0);
	EXPECT_EQ(state.WriteBuffer, 0);
	EXPECT_EQ(state.Command, 0);
	EXPECT_EQ(state.Control, 0);
	EXPECT_FALSE(state.WriteDisabled);
	EXPECT_FALSE(state.ReadDone);
	EXPECT_FALSE(state.Idle);
	EXPECT_FALSE(state.InternalEepromWriteProtected);
}

TEST(WsStateEepromSizeTest, EnumValues) {
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size0), 0x0000);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size128), 0x0080);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size1kb), 0x0400);
	EXPECT_EQ(static_cast<uint16_t>(WsEepromSize::Size2kb), 0x0800);
}

TEST(WsStateEepromBehaviorTest, DecodeCommandAcrossSizes) {
	EXPECT_EQ(WsStateBehaviorHelpers::DecodeEepromCommand(0x0180, WsEepromSize::Size128), WsEepromCommand::Read);
	EXPECT_EQ(WsStateBehaviorHelpers::DecodeEepromCommand(0x1400, WsEepromSize::Size1kb), WsEepromCommand::Write);
	EXPECT_EQ(WsStateBehaviorHelpers::DecodeEepromCommand(0x0000, WsEepromSize::Size2kb), WsEepromCommand::Unknown);
}

TEST(WsStateEepromBehaviorTest, CommandAddressMaskMatchesSize) {
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandAddress(0xffff, WsEepromSize::Size128), 0x003f);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandAddress(0xffff, WsEepromSize::Size1kb), 0x01ff);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandAddress(0xffff, WsEepromSize::Size2kb), 0x03ff);
}

TEST(WsStateEepromBehaviorTest, CommandDelayBuckets) {
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandDelay(WsEepromCommand::Read), 6u);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandDelay(WsEepromCommand::Write), 10u);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandDelay(WsEepromCommand::Erase), 10u);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandDelay(WsEepromCommand::WriteAll), 20u);
	EXPECT_EQ(WsStateBehaviorHelpers::GetEepromCommandDelay(WsEepromCommand::EraseAll), 20u);
}

TEST(WsStateMemoryBehaviorTest, SystemTestReadUsesOpenBus) {
	EXPECT_EQ(WsStateBehaviorHelpers::ReadSystemTestPortBehavior(0x5a, 0x0f), 0x5a);
	EXPECT_EQ(WsStateBehaviorHelpers::ReadSystemTestPortBehavior(0x00, 0x07), 0x00);
}

TEST(WsStateMemoryBehaviorTest, WordBusSelectionByAddressRegion) {
	EXPECT_TRUE(WsStateBehaviorHelpers::IsWordBusBehavior(0x00000, false));
	EXPECT_FALSE(WsStateBehaviorHelpers::IsWordBusBehavior(0x10000, true));
	EXPECT_FALSE(WsStateBehaviorHelpers::IsWordBusBehavior(0x20000, false));
	EXPECT_TRUE(WsStateBehaviorHelpers::IsWordBusBehavior(0x20000, true));
}

TEST(WsStateCpuFlagsTest, GetSetRoundTripPreservesDefinedFlags) {
	WsCpuFlags flags = {};
	flags.Carry = true;
	flags.Parity = true;
	flags.AuxCarry = false;
	flags.Zero = true;
	flags.Sign = false;
	flags.Trap = true;
	flags.Irq = true;
	flags.Direction = false;
	flags.Overflow = true;
	flags.Mode = true;

	uint16_t packed = flags.Get();
	EXPECT_EQ((packed & 0x01), 0x01);
	EXPECT_EQ((packed & 0x04), 0x04);
	EXPECT_EQ((packed & 0x40), 0x40);
	EXPECT_EQ((packed & 0x100), 0x100);
	EXPECT_EQ((packed & 0x200), 0x200);
	EXPECT_EQ((packed & 0x800), 0x800);
	EXPECT_EQ((packed & 0x8000), 0x8000);

	WsCpuFlags roundTrip = {};
	roundTrip.Set(packed);
	EXPECT_TRUE(roundTrip.Carry);
	EXPECT_TRUE(roundTrip.Parity);
	EXPECT_FALSE(roundTrip.AuxCarry);
	EXPECT_TRUE(roundTrip.Zero);
	EXPECT_FALSE(roundTrip.Sign);
	EXPECT_TRUE(roundTrip.Trap);
	EXPECT_TRUE(roundTrip.Irq);
	EXPECT_FALSE(roundTrip.Direction);
	EXPECT_TRUE(roundTrip.Overflow);
	EXPECT_TRUE(roundTrip.Mode);
}

TEST(WsStatePpuHelpersTest, BgLayerLatchCopiesLiveValues) {
	WsBgLayer layer = {};
	layer.Enabled = true;
	layer.ScrollX = 0x12;
	layer.ScrollY = 0x34;
	layer.MapAddress = 0x5678;

	layer.Latch();

	EXPECT_TRUE(layer.EnabledLatch);
	EXPECT_EQ(layer.ScrollXLatch, 0x12);
	EXPECT_EQ(layer.ScrollYLatch, 0x34);
	EXPECT_EQ(layer.MapAddressLatch, 0x5678);
}

TEST(WsStatePpuHelpersTest, WindowLatchAndInsideWindowUseLatchedBounds) {
	WsWindow window = {};
	window.Enabled = true;
	window.Left = 10;
	window.Right = 20;
	window.Top = 30;
	window.Bottom = 40;
	window.Latch();

	EXPECT_TRUE(window.EnabledLatch);
	EXPECT_TRUE(window.IsInsideWindow(10, 30));
	EXPECT_TRUE(window.IsInsideWindow(20, 40));
	EXPECT_FALSE(window.IsInsideWindow(9, 30));
	EXPECT_FALSE(window.IsInsideWindow(10, 41));
}

TEST(WsStateCpuHelpersTest, ParityTableMatchesEvenParityDefinition) {
	WsCpuParityTable table;
	EXPECT_TRUE(table.CheckParity(0x00)); // 0 set bits (even)
	EXPECT_FALSE(table.CheckParity(0x01)); // 1 set bit (odd)
	EXPECT_TRUE(table.CheckParity(0x03)); // 2 set bits (even)
	EXPECT_FALSE(table.CheckParity(0x07)); // 3 set bits (odd)
	EXPECT_TRUE(table.CheckParity(0xff)); // 8 set bits (even)
}

TEST(WsStateBaseApuTest, SetVolume_SplitsNibblesCorrectly) {
	BaseWsApuState state = {};
	state.SetVolume(0xab);
	EXPECT_EQ(state.LeftVolume, 0x0a);
	EXPECT_EQ(state.RightVolume, 0x0b);
}

TEST(WsStateBaseApuTest, GetVolume_MergesNibblesCorrectly) {
	BaseWsApuState state = {};
	state.LeftVolume = 0x0c;
	state.RightVolume = 0x05;
	EXPECT_EQ(state.GetVolume(), 0xc5);
}

TEST(WsStateApuTest, DefaultState_ZeroInitializedFlags) {
	WsApuState state = {};
	EXPECT_EQ(state.WaveTableAddress, 0);
	EXPECT_FALSE(state.SpeakerEnabled);
	EXPECT_EQ(state.SpeakerVolume, 0);
	EXPECT_EQ(state.InternalMasterVolume, 0);
	EXPECT_EQ(state.MasterVolume, 0);
	EXPECT_FALSE(state.HeadphoneEnabled);
	EXPECT_FALSE(state.HoldChannels);
	EXPECT_FALSE(state.ForceOutput2);
	EXPECT_FALSE(state.ForceOutput4);
	EXPECT_FALSE(state.ForceOutputCh2Voice);
	EXPECT_EQ(state.SoundTest, 0);
}

TEST(WsStateApuHyperVoiceTest, ChannelAndScalingEnums) {
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::Stereo), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoLeft), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoRight), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceChannelMode::MonoBoth), 3);

	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Unsigned), 0);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::UnsignedNegated), 1);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::Signed), 2);
	EXPECT_EQ(static_cast<uint8_t>(WsHyperVoiceScalingMode::None), 3);
}

TEST(WsStateAggregateTest, AggregateDefaultState_ZeroInitialized) {
	WsState state = {};
	EXPECT_EQ(state.Cpu.CS, 0);
	EXPECT_EQ(state.Ppu.Scanline, 0);
	EXPECT_EQ(state.MemoryManager.OpenBus, 0);
	EXPECT_EQ(state.ControlManager.InputSelect, 0);
	EXPECT_EQ(state.Timer.Control, 0);
	EXPECT_EQ(state.Serial.SendClock, 0u);
	EXPECT_EQ(state.InternalEeprom.Size, WsEepromSize::Size0);
	EXPECT_EQ(state.Cart.SelectedBanks[0], 0);
	EXPECT_EQ(state.Cart.SelectedBanks[1], 0);
	EXPECT_EQ(state.Cart.SelectedBanks[2], 0);
	EXPECT_EQ(state.Cart.SelectedBanks[3], 0);
}
