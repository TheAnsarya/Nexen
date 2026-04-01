#include "pch.h"
#include "PCE/PceTypes.h"
#include "PCE/PceConstants.h"

//=============================================================================
// PCE IRQ Source Tests
//=============================================================================

TEST(PceIrqSourceTest, IndividualBits) {
	EXPECT_EQ((int)PceIrqSource::Irq2, 1);
	EXPECT_EQ((int)PceIrqSource::Irq1, 2);
	EXPECT_EQ((int)PceIrqSource::TimerIrq, 4);
}

TEST(PceIrqSourceTest, NoDuplicateBits) {
	int all = (int)PceIrqSource::Irq2 | (int)PceIrqSource::Irq1 | (int)PceIrqSource::TimerIrq;
	EXPECT_EQ(all, 0x07);
}

//=============================================================================
// PCE CPU Flags Tests
//=============================================================================

TEST(PceCpuFlagsTest, IndividualBits) {
	EXPECT_EQ(PceCpuFlags::Carry, 0x01);
	EXPECT_EQ(PceCpuFlags::Zero, 0x02);
	EXPECT_EQ(PceCpuFlags::Interrupt, 0x04);
	EXPECT_EQ(PceCpuFlags::Decimal, 0x08);
	EXPECT_EQ(PceCpuFlags::Break, 0x10);
	EXPECT_EQ(PceCpuFlags::Memory, 0x20);
	EXPECT_EQ(PceCpuFlags::Overflow, 0x40);
	EXPECT_EQ(PceCpuFlags::Negative, 0x80);
}

TEST(PceCpuFlagsTest, AllFlagsCombined) {
	uint8_t all = PceCpuFlags::Carry | PceCpuFlags::Zero | PceCpuFlags::Interrupt |
		PceCpuFlags::Decimal | PceCpuFlags::Break | PceCpuFlags::Memory |
		PceCpuFlags::Overflow | PceCpuFlags::Negative;
	EXPECT_EQ(all, 0xff);
}

TEST(PceCpuFlagsTest, NoBitOverlap) {
	uint8_t flags[] = {
		PceCpuFlags::Carry, PceCpuFlags::Zero, PceCpuFlags::Interrupt,
		PceCpuFlags::Decimal, PceCpuFlags::Break, PceCpuFlags::Memory,
		PceCpuFlags::Overflow, PceCpuFlags::Negative
	};
	for (int i = 0; i < 8; i++) {
		for (int j = i + 1; j < 8; j++) {
			EXPECT_EQ(flags[i] & flags[j], 0) << "Flags " << i << " and " << j << " overlap";
		}
	}
}

//=============================================================================
// PCE Address Mode Tests
//=============================================================================

TEST(PceAddrModeTest, EnumValues) {
	// Verify first and last values exist and are distinct
	EXPECT_NE((int)PceAddrMode::None, (int)PceAddrMode::Acc);
	EXPECT_NE((int)PceAddrMode::None, (int)PceAddrMode::AbsXInd);
	EXPECT_NE((int)PceAddrMode::Imm, (int)PceAddrMode::Rel);
}

//=============================================================================
// PCE CPU State Tests
//=============================================================================

TEST(PceCpuStateTest, DefaultInitialization) {
	PceCpuState state{};
	EXPECT_EQ(state.CycleCount, 0u);
	EXPECT_EQ(state.PC, 0u);
	EXPECT_EQ(state.SP, 0u);
	EXPECT_EQ(state.A, 0u);
	EXPECT_EQ(state.X, 0u);
	EXPECT_EQ(state.Y, 0u);
	EXPECT_EQ(state.PS, 0u);
}

TEST(PceCpuStateTest, RegisterAssignment) {
	PceCpuState state{};
	state.PC = 0xe000;
	state.SP = 0xff;
	state.A = 0x42;
	state.X = 0x10;
	state.Y = 0x20;
	state.PS = PceCpuFlags::Carry | PceCpuFlags::Zero;
	EXPECT_EQ(state.PC, 0xe000);
	EXPECT_EQ(state.SP, 0xff);
	EXPECT_EQ(state.A, 0x42);
	EXPECT_EQ(state.X, 0x10);
	EXPECT_EQ(state.Y, 0x20);
	EXPECT_EQ(state.PS, 0x03);
}

//=============================================================================
// PCE VDC HV Latches Tests
//=============================================================================

TEST(PceVdcHvLatchesTest, DefaultInitialization) {
	PceVdcHvLatches latches{};
	EXPECT_EQ(latches.BgScrollX, 0u);
	EXPECT_EQ(latches.BgScrollY, 0u);
	EXPECT_EQ(latches.ColumnCount, 0u);
	EXPECT_EQ(latches.RowCount, 0u);
	EXPECT_EQ(latches.SpriteAccessMode, 0u);
	EXPECT_EQ(latches.VramAccessMode, 0u);
	EXPECT_FALSE(latches.CgMode);
	EXPECT_EQ(latches.HorizDisplayStart, 0u);
	EXPECT_EQ(latches.HorizSyncWidth, 0u);
	EXPECT_EQ(latches.HorizDisplayWidth, 0u);
	EXPECT_EQ(latches.HorizDisplayEnd, 0u);
	EXPECT_EQ(latches.VertDisplayStart, 0u);
	EXPECT_EQ(latches.VertSyncWidth, 0u);
	EXPECT_EQ(latches.VertDisplayWidth, 0u);
	EXPECT_EQ(latches.VertEndPosVcr, 0u);
}

//=============================================================================
// PCE VDC State Tests
//=============================================================================

TEST(PceVdcStateTest, DefaultInitialization) {
	PceVdcState state{};
	EXPECT_EQ(state.FrameCount, 0u);
	EXPECT_EQ(state.HClock, 0u);
	EXPECT_EQ(state.Scanline, 0u);
	EXPECT_EQ(state.RcrCounter, 0u);
	EXPECT_EQ(state.CurrentReg, 0u);
	EXPECT_EQ(state.MemAddrWrite, 0u);
	EXPECT_EQ(state.MemAddrRead, 0u);
	EXPECT_EQ(state.ReadBuffer, 0u);
	EXPECT_EQ(state.VramData, 0u);
	EXPECT_FALSE(state.EnableCollisionIrq);
	EXPECT_FALSE(state.EnableOverflowIrq);
	EXPECT_FALSE(state.EnableScanlineIrq);
	EXPECT_FALSE(state.EnableVerticalBlankIrq);
	EXPECT_FALSE(state.SpritesEnabled);
	EXPECT_FALSE(state.BackgroundEnabled);
	EXPECT_EQ(state.VramAddrIncrement, 0u);
	EXPECT_EQ(state.RasterCompareRegister, 0u);
}

TEST(PceVdcStateTest, IrqEnableFlags) {
	PceVdcState state{};
	state.EnableCollisionIrq = true;
	state.EnableOverflowIrq = true;
	state.EnableScanlineIrq = true;
	state.EnableVerticalBlankIrq = true;
	EXPECT_TRUE(state.EnableCollisionIrq);
	EXPECT_TRUE(state.EnableOverflowIrq);
	EXPECT_TRUE(state.EnableScanlineIrq);
	EXPECT_TRUE(state.EnableVerticalBlankIrq);
}

TEST(PceVdcStateTest, DmaFields) {
	PceVdcState state{};
	state.BlockSrc = 0x1000;
	state.BlockDst = 0x2000;
	state.BlockLen = 0x0100;
	state.SatbBlockSrc = 0x7f00;
	state.RepeatSatbTransfer = true;
	EXPECT_EQ(state.BlockSrc, 0x1000);
	EXPECT_EQ(state.BlockDst, 0x2000);
	EXPECT_EQ(state.BlockLen, 0x0100);
	EXPECT_EQ(state.SatbBlockSrc, 0x7f00);
	EXPECT_TRUE(state.RepeatSatbTransfer);
}

TEST(PceVdcStateTest, StatusFlags) {
	PceVdcState state{};
	state.VerticalBlank = true;
	state.VramTransferDone = true;
	state.SatbTransferDone = true;
	state.ScanlineDetected = true;
	state.SpriteOverflow = true;
	state.Sprite0Hit = true;
	state.BurstModeEnabled = true;
	EXPECT_TRUE(state.VerticalBlank);
	EXPECT_TRUE(state.VramTransferDone);
	EXPECT_TRUE(state.SatbTransferDone);
	EXPECT_TRUE(state.ScanlineDetected);
	EXPECT_TRUE(state.SpriteOverflow);
	EXPECT_TRUE(state.Sprite0Hit);
	EXPECT_TRUE(state.BurstModeEnabled);
}

//=============================================================================
// PCE VCE State Tests
//=============================================================================

TEST(PceVceStateTest, DefaultInitialization) {
	PceVceState state{};
	EXPECT_EQ(state.ScanlineCount, 0u);
	EXPECT_EQ(state.PalAddr, 0u);
	EXPECT_EQ(state.ClockDivider, 0u);
	EXPECT_FALSE(state.Grayscale);
}

TEST(PceVceStateTest, ClockDividerModes) {
	PceVceState state{};
	// Mode 1: 5.37 MHz (divider = 4)
	state.ClockDivider = 4;
	EXPECT_EQ(state.ClockDivider, 4);
	// Mode 2: 7.16 MHz (divider = 3)
	state.ClockDivider = 3;
	EXPECT_EQ(state.ClockDivider, 3);
	// Mode 3: 10.74 MHz (divider = 2)
	state.ClockDivider = 2;
	EXPECT_EQ(state.ClockDivider, 2);
}

//=============================================================================
// PCE VPC Tests
//=============================================================================

TEST(PceVpcPriorityModeTest, EnumValues) {
	EXPECT_EQ((int)PceVpcPriorityMode::Default, 0);
	EXPECT_EQ((int)PceVpcPriorityMode::Vdc2SpritesAboveVdc1Bg, 1);
	EXPECT_EQ((int)PceVpcPriorityMode::Vdc1SpritesBelowVdc2Bg, 2);
}

TEST(PceVpcPixelWindowTest, EnumValues) {
	EXPECT_NE((int)PceVpcPixelWindow::NoWindow, (int)PceVpcPixelWindow::Window1);
	EXPECT_NE((int)PceVpcPixelWindow::Window2, (int)PceVpcPixelWindow::Both);
}

TEST(PceVpcStateTest, DefaultInitialization) {
	PceVpcState state{};
	EXPECT_EQ(state.Priority1, 0u);
	EXPECT_EQ(state.Priority2, 0u);
	EXPECT_EQ(state.Window1, 0u);
	EXPECT_EQ(state.Window2, 0u);
	EXPECT_FALSE(state.StToVdc2Mode);
	EXPECT_FALSE(state.HasIrqVdc1);
	EXPECT_FALSE(state.HasIrqVdc2);
}

TEST(PceVpcStateTest, WindowConfigArray) {
	PceVpcState state{};
	state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].PriorityMode = PceVpcPriorityMode::Default;
	state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].Vdc1Enabled = true;
	state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].Vdc2Enabled = false;
	state.WindowCfg[(int)PceVpcPixelWindow::Both].PriorityMode = PceVpcPriorityMode::Vdc2SpritesAboveVdc1Bg;
	EXPECT_EQ(state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].PriorityMode, PceVpcPriorityMode::Default);
	EXPECT_TRUE(state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].Vdc1Enabled);
	EXPECT_FALSE(state.WindowCfg[(int)PceVpcPixelWindow::NoWindow].Vdc2Enabled);
	EXPECT_EQ(state.WindowCfg[(int)PceVpcPixelWindow::Both].PriorityMode, PceVpcPriorityMode::Vdc2SpritesAboveVdc1Bg);
}

//=============================================================================
// PCE Video State Tests
//=============================================================================

TEST(PceVideoStateTest, DefaultInitialization) {
	PceVideoState state{};
	EXPECT_EQ(state.Vdc.FrameCount, 0u);
	EXPECT_EQ(state.Vdc2.FrameCount, 0u);
	EXPECT_EQ(state.Vce.ScanlineCount, 0u);
	EXPECT_EQ(state.Vpc.Priority1, 0u);
}

//=============================================================================
// PCE Memory Manager State Tests
//=============================================================================

TEST(PceMemoryManagerStateTest, DefaultInitialization) {
	PceMemoryManagerState state{};
	EXPECT_EQ(state.CycleCount, 0u);
	EXPECT_EQ(state.ActiveIrqs, 0u);
	EXPECT_EQ(state.DisabledIrqs, 0u);
	EXPECT_FALSE(state.FastCpuSpeed);
	EXPECT_EQ(state.MprReadBuffer, 0u);
	EXPECT_EQ(state.IoBuffer, 0u);
	for (int i = 0; i < 8; i++) {
		EXPECT_EQ(state.Mpr[i], 0) << "MPR[" << i << "] not zero";
	}
}

TEST(PceMemoryManagerStateTest, MprBanking) {
	PceMemoryManagerState state{};
	// Typical bank mappings
	state.Mpr[0] = 0xff; // I/O page
	state.Mpr[1] = 0xf8; // RAM
	state.Mpr[7] = 0x00; // Base ROM
	EXPECT_EQ(state.Mpr[0], 0xff);
	EXPECT_EQ(state.Mpr[1], 0xf8);
	EXPECT_EQ(state.Mpr[7], 0x00);
}

//=============================================================================
// PCE Timer State Tests
//=============================================================================

TEST(PceTimerStateTest, DefaultInitialization) {
	PceTimerState state{};
	EXPECT_EQ(state.ReloadValue, 0u);
	EXPECT_EQ(state.Counter, 0u);
	EXPECT_EQ(state.Scaler, 0u);
	EXPECT_FALSE(state.Enabled);
}

TEST(PceTimerStateTest, ReloadValue7Bit) {
	PceTimerState state{};
	state.ReloadValue = 0x7f; // Max 7-bit value
	EXPECT_EQ(state.ReloadValue, 0x7f);
}

//=============================================================================
// PCE PSG State Tests
//=============================================================================

TEST(PcePsgStateTest, DefaultInitialization) {
	PcePsgState state{};
	EXPECT_EQ(state.ChannelSelect, 0u);
	EXPECT_EQ(state.LeftVolume, 0u);
	EXPECT_EQ(state.RightVolume, 0u);
	EXPECT_EQ(state.LfoFrequency, 0u);
	EXPECT_EQ(state.LfoControl, 0u);
}

TEST(PcePsgChannelStateTest, DefaultInitialization) {
	PcePsgChannelState state{};
	EXPECT_EQ(state.Frequency, 0u);
	EXPECT_EQ(state.Amplitude, 0u);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.LeftVolume, 0u);
	EXPECT_EQ(state.RightVolume, 0u);
	EXPECT_FALSE(state.DdaEnabled);
	EXPECT_EQ(state.DdaOutputValue, 0u);
	EXPECT_EQ(state.WaveAddr, 0u);
	EXPECT_EQ(state.Timer, 0u);
	EXPECT_EQ(state.CurrentOutput, 0);
	EXPECT_FALSE(state.NoiseEnabled);
	EXPECT_EQ(state.NoiseOutput, 0);
	EXPECT_EQ(state.NoiseFrequency, 0u);
}

TEST(PcePsgChannelStateTest, WaveDataArray) {
	PcePsgChannelState state{};
	for (int i = 0; i < 0x20; i++) {
		EXPECT_EQ(state.WaveData[i], 0) << "WaveData[" << i << "] not zero";
	}
	// Write pattern
	state.WaveData[0] = 0x1f;
	state.WaveData[0x1f] = 0x00;
	EXPECT_EQ(state.WaveData[0], 0x1f);
	EXPECT_EQ(state.WaveData[0x1f], 0x00);
}

//=============================================================================
// PCE Arcade Card Tests
//=============================================================================

TEST(PceArcadePortOffsetTriggerTest, EnumValues) {
	EXPECT_EQ((int)PceArcadePortOffsetTrigger::None, 0);
	EXPECT_EQ((int)PceArcadePortOffsetTrigger::AddOnLowWrite, 1);
	EXPECT_EQ((int)PceArcadePortOffsetTrigger::AddOnHighWrite, 2);
	EXPECT_EQ((int)PceArcadePortOffsetTrigger::AddOnReg0AWrite, 3);
}

TEST(PceArcadeCardPortConfigTest, DefaultInitialization) {
	PceArcadeCardPortConfig config{};
	EXPECT_EQ(config.BaseAddress, 0u);
	EXPECT_EQ(config.Offset, 0u);
	EXPECT_EQ(config.IncValue, 0u);
	EXPECT_EQ(config.Control, 0u);
	EXPECT_FALSE(config.AutoIncrement);
	EXPECT_FALSE(config.AddOffset);
	EXPECT_FALSE(config.SignedIncrement);
	EXPECT_FALSE(config.SignedOffset);
	EXPECT_FALSE(config.AddIncrementToBase);
}

TEST(PceArcadeCardStateTest, DefaultInitialization) {
	PceArcadeCardState state{};
	EXPECT_EQ(state.ValueReg, 0u);
	EXPECT_EQ(state.ShiftReg, 0u);
	EXPECT_EQ(state.RotateReg, 0u);
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(state.Port[i].BaseAddress, 0u) << "Port[" << i << "] base not zero";
	}
}

//=============================================================================
// PCE CD-ROM State Tests
//=============================================================================

TEST(PceCdRomIrqSourceTest, IndividualBits) {
	EXPECT_EQ((int)PceCdRomIrqSource::Adpcm, 0x04);
	EXPECT_EQ((int)PceCdRomIrqSource::Stop, 0x08);
	EXPECT_EQ((int)PceCdRomIrqSource::SubCode, 0x10);
	EXPECT_EQ((int)PceCdRomIrqSource::StatusMsgIn, 0x20);
	EXPECT_EQ((int)PceCdRomIrqSource::DataIn, 0x40);
}

TEST(ScsiPhaseTest, EnumValues) {
	EXPECT_NE((int)ScsiPhase::BusFree, (int)ScsiPhase::Command);
	EXPECT_NE((int)ScsiPhase::Command, (int)ScsiPhase::DataIn);
	EXPECT_NE((int)ScsiPhase::DataIn, (int)ScsiPhase::Status);
	EXPECT_NE((int)ScsiPhase::Status, (int)ScsiPhase::MessageIn);
	EXPECT_NE((int)ScsiPhase::MessageIn, (int)ScsiPhase::Busy);
}

TEST(CdPlayEndBehaviorTest, EnumValues) {
	EXPECT_NE((int)CdPlayEndBehavior::Stop, (int)CdPlayEndBehavior::Loop);
	EXPECT_NE((int)CdPlayEndBehavior::Loop, (int)CdPlayEndBehavior::Irq);
}

TEST(CdAudioStatusTest, EnumValues) {
	EXPECT_EQ((uint8_t)CdAudioStatus::Playing, 0);
	EXPECT_EQ((uint8_t)CdAudioStatus::Inactive, 1);
	EXPECT_EQ((uint8_t)CdAudioStatus::Paused, 2);
	EXPECT_EQ((uint8_t)CdAudioStatus::Stopped, 3);
}

TEST(PceCdRomStateTest, DefaultInitialization) {
	PceCdRomState state{};
	EXPECT_EQ(state.AudioSampleLatch, 0u);
	EXPECT_EQ(state.ActiveIrqs, 0u);
	EXPECT_EQ(state.EnabledIrqs, 0u);
	EXPECT_FALSE(state.ReadRightChannel);
	EXPECT_FALSE(state.BramLocked);
	EXPECT_EQ(state.ResetRegValue, 0u);
}

TEST(PceAdpcmStateTest, DefaultInitialization) {
	PceAdpcmState state{};
	EXPECT_EQ(state.ReadAddress, 0u);
	EXPECT_EQ(state.WriteAddress, 0u);
	EXPECT_EQ(state.AddressPort, 0u);
	EXPECT_EQ(state.DmaControl, 0u);
	EXPECT_EQ(state.Control, 0u);
	EXPECT_EQ(state.PlaybackRate, 0u);
	EXPECT_EQ(state.AdpcmLength, 0u);
	EXPECT_FALSE(state.EndReached);
	EXPECT_FALSE(state.HalfReached);
	EXPECT_FALSE(state.Playing);
}

TEST(PceCdAudioPlayerStateTest, DefaultInitialization) {
	PceCdAudioPlayerState state{};
	EXPECT_EQ(state.StartSector, 0u);
	EXPECT_EQ(state.EndSector, 0u);
	EXPECT_EQ(state.CurrentSector, 0u);
	EXPECT_EQ(state.CurrentSample, 0u);
}

TEST(PceScsiBusStateTest, DefaultInitialization) {
	PceScsiBusState state{};
	EXPECT_EQ(state.DataPort, 0u);
	EXPECT_EQ(state.ReadDataPort, 0u);
	EXPECT_EQ(state.Sector, 0u);
	EXPECT_EQ(state.SectorsToRead, 0u);
	EXPECT_FALSE(state.MessageDone);
}

//=============================================================================
// PCE Audio Fader Tests
//=============================================================================

TEST(PceAudioFaderTargetTest, EnumValues) {
	EXPECT_NE((int)PceAudioFaderTarget::Adpcm, (int)PceAudioFaderTarget::CdAudio);
}

TEST(PceAudioFaderStateTest, DefaultInitialization) {
	PceAudioFaderState state{};
	EXPECT_EQ(state.StartClock, 0u);
	EXPECT_FALSE(state.FastFade);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.RegValue, 0u);
}

//=============================================================================
// PCE Constants Tests
//=============================================================================

TEST(PceConstantsTest, MasterClock) {
	EXPECT_EQ(PceConstants::MasterClockRate, 21477270u);
}

TEST(PceConstantsTest, ScanlineConstants) {
	EXPECT_EQ(PceConstants::ClockPerScanline, 1365u);
	EXPECT_EQ(PceConstants::ScanlineCount, 263u);
}

TEST(PceConstantsTest, ScreenDimensions) {
	EXPECT_EQ(PceConstants::MaxScreenWidth, 682u);
	EXPECT_EQ(PceConstants::ScreenHeight, 242u);
}
