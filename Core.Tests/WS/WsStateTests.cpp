#include "pch.h"
#include "WS/WsTypes.h"

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
