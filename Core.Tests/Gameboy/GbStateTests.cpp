#include "pch.h"
#include "Gameboy/GbTypes.h"
#include "Gameboy/GbConstants.h"
#include "Gameboy/GameboyHeader.h"

//=============================================================================
// GB CPU Flags Tests
//=============================================================================

TEST(GbCpuFlagsTest, IndividualBits) {
	EXPECT_EQ(GbCpuFlags::Zero, 0x80);
	EXPECT_EQ(GbCpuFlags::AddSub, 0x40);
	EXPECT_EQ(GbCpuFlags::HalfCarry, 0x20);
	EXPECT_EQ(GbCpuFlags::Carry, 0x10);
}

TEST(GbCpuFlagsTest, UpperNibbleOnly) {
	uint8_t all = GbCpuFlags::Zero | GbCpuFlags::AddSub | GbCpuFlags::HalfCarry | GbCpuFlags::Carry;
	EXPECT_EQ(all, 0xf0);
	EXPECT_EQ(all & 0x0f, 0) << "Lower nibble must be zero";
}

TEST(GbCpuFlagsTest, NoBitOverlap) {
	uint8_t flags[] = { GbCpuFlags::Zero, GbCpuFlags::AddSub, GbCpuFlags::HalfCarry, GbCpuFlags::Carry };
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			EXPECT_EQ(flags[i] & flags[j], 0) << "Flags " << i << " and " << j << " overlap";
		}
	}
}

//=============================================================================
// GB IRQ Source Tests
//=============================================================================

TEST(GbIrqSourceTest, IndividualBits) {
	EXPECT_EQ(GbIrqSource::VerticalBlank, 0x01);
	EXPECT_EQ(GbIrqSource::LcdStat, 0x02);
	EXPECT_EQ(GbIrqSource::Timer, 0x04);
	EXPECT_EQ(GbIrqSource::Serial, 0x08);
	EXPECT_EQ(GbIrqSource::Joypad, 0x10);
}

TEST(GbIrqSourceTest, AllSourcesCombined) {
	uint8_t all = GbIrqSource::VerticalBlank | GbIrqSource::LcdStat |
		GbIrqSource::Timer | GbIrqSource::Serial | GbIrqSource::Joypad;
	EXPECT_EQ(all, 0x1f);
}

//=============================================================================
// GB PPU Status Flags Tests
//=============================================================================

TEST(GbPpuStatusFlagsTest, IndividualBits) {
	EXPECT_EQ(GbPpuStatusFlags::CoincidenceIrq, 0x40);
	EXPECT_EQ(GbPpuStatusFlags::OamIrq, 0x20);
	EXPECT_EQ(GbPpuStatusFlags::VBlankIrq, 0x10);
	EXPECT_EQ(GbPpuStatusFlags::HBlankIrq, 0x08);
}

TEST(GbPpuStatusFlagsTest, NoBitOverlap) {
	uint8_t flags[] = { GbPpuStatusFlags::CoincidenceIrq, GbPpuStatusFlags::OamIrq,
		GbPpuStatusFlags::VBlankIrq, GbPpuStatusFlags::HBlankIrq };
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			EXPECT_EQ(flags[i] & flags[j], 0) << "Flags " << i << " and " << j << " overlap";
		}
	}
}

//=============================================================================
// GB CPU State Tests
//=============================================================================

TEST(GbCpuStateTest, DefaultInitialization) {
	GbCpuState state{};
	EXPECT_EQ(state.CycleCount, 0u);
	EXPECT_EQ(state.PC, 0u);
	EXPECT_EQ(state.SP, 0u);
	EXPECT_EQ(state.HaltCounter, 0u);
	EXPECT_EQ(state.A, 0u);
	EXPECT_EQ(state.Flags, 0u);
	EXPECT_EQ(state.B, 0u);
	EXPECT_EQ(state.C, 0u);
	EXPECT_EQ(state.D, 0u);
	EXPECT_EQ(state.E, 0u);
	EXPECT_EQ(state.H, 0u);
	EXPECT_EQ(state.L, 0u);
	EXPECT_FALSE(state.EiPending);
	EXPECT_FALSE(state.IME);
	EXPECT_FALSE(state.HaltBug);
	EXPECT_FALSE(state.Stopped);
}

TEST(GbCpuStateTest, RegisterPairAssignment) {
	GbCpuState state{};
	// Set register pairs for BC, DE, HL
	state.B = 0x01; state.C = 0x23;
	state.D = 0x45; state.E = 0x67;
	state.H = 0x89; state.L = 0xab;
	EXPECT_EQ(state.B, 0x01);
	EXPECT_EQ(state.C, 0x23);
	EXPECT_EQ(state.D, 0x45);
	EXPECT_EQ(state.E, 0x67);
	EXPECT_EQ(state.H, 0x89);
	EXPECT_EQ(state.L, 0xab);
}

TEST(GbCpuStateTest, FlagManipulation) {
	GbCpuState state{};
	state.Flags = GbCpuFlags::Zero | GbCpuFlags::Carry;
	EXPECT_EQ(state.Flags & GbCpuFlags::Zero, GbCpuFlags::Zero);
	EXPECT_EQ(state.Flags & GbCpuFlags::Carry, GbCpuFlags::Carry);
	EXPECT_EQ(state.Flags & GbCpuFlags::AddSub, 0);
	EXPECT_EQ(state.Flags & GbCpuFlags::HalfCarry, 0);
}

//=============================================================================
// GB PPU Mode Tests
//=============================================================================

TEST(GbPpuModeTest, EnumValues) {
	EXPECT_NE((int)PpuMode::HBlank, (int)PpuMode::VBlank);
	EXPECT_NE((int)PpuMode::VBlank, (int)PpuMode::OamEvaluation);
	EXPECT_NE((int)PpuMode::OamEvaluation, (int)PpuMode::Drawing);
	EXPECT_NE((int)PpuMode::Drawing, (int)PpuMode::NoIrq);
}

//=============================================================================
// GB PPU State Tests
//=============================================================================

TEST(GbPpuStateTest, DefaultInitialization) {
	GbPpuState state{};
	EXPECT_EQ(state.Scanline, 0u);
	EXPECT_EQ(state.Cycle, 0u);
	EXPECT_EQ(state.FrameCount, 0u);
	EXPECT_EQ(state.Ly, 0u);
	EXPECT_EQ(state.LyCompare, 0u);
	EXPECT_FALSE(state.LyCoincidenceFlag);
	EXPECT_FALSE(state.StatIrqFlag);
	EXPECT_EQ(state.BgPalette, 0u);
	EXPECT_EQ(state.ObjPalette0, 0u);
	EXPECT_EQ(state.ObjPalette1, 0u);
	EXPECT_EQ(state.ScrollX, 0u);
	EXPECT_EQ(state.ScrollY, 0u);
	EXPECT_EQ(state.WindowX, 0u);
	EXPECT_EQ(state.WindowY, 0u);
	EXPECT_EQ(state.Control, 0u);
	EXPECT_EQ(state.Status, 0u);
}

TEST(GbPpuStateTest, LcdcBitFields) {
	GbPpuState state{};
	state.LcdEnabled = true;
	state.WindowTilemapSelect = true;
	state.WindowEnabled = true;
	state.BgTileSelect = true;
	state.BgTilemapSelect = true;
	state.LargeSprites = true;
	state.SpritesEnabled = true;
	state.BgEnabled = true;
	EXPECT_TRUE(state.LcdEnabled);
	EXPECT_TRUE(state.WindowTilemapSelect);
	EXPECT_TRUE(state.WindowEnabled);
	EXPECT_TRUE(state.BgTileSelect);
	EXPECT_TRUE(state.BgTilemapSelect);
	EXPECT_TRUE(state.LargeSprites);
	EXPECT_TRUE(state.SpritesEnabled);
	EXPECT_TRUE(state.BgEnabled);
}

TEST(GbPpuStateTest, CgbPaletteFields) {
	GbPpuState state{};
	EXPECT_FALSE(state.CgbEnabled);
	EXPECT_EQ(state.CgbVramBank, 0u);
	EXPECT_EQ(state.CgbBgPalPosition, 0u);
	EXPECT_FALSE(state.CgbBgPalAutoInc);
	EXPECT_EQ(state.CgbObjPalPosition, 0u);
	EXPECT_FALSE(state.CgbObjPalAutoInc);
}

TEST(GbPpuStateTest, CgbPaletteArrays) {
	GbPpuState state{};
	// 8 palettes x 4 colors = 32 entries
	for (int i = 0; i < 32; i++) {
		EXPECT_EQ(state.CgbBgPalettes[i], 0u) << "BG palette[" << i << "] not zero";
		EXPECT_EQ(state.CgbObjPalettes[i], 0u) << "OBJ palette[" << i << "] not zero";
	}
	// Write a color to first BG palette
	state.CgbBgPalettes[0] = 0x7fff; // White in RGB555
	EXPECT_EQ(state.CgbBgPalettes[0], 0x7fff);
}

//=============================================================================
// GB Pixel Type Tests
//=============================================================================

TEST(GbPixelTypeTest, EnumValues) {
	EXPECT_EQ((uint8_t)GbPixelType::Background, 0);
	EXPECT_EQ((uint8_t)GbPixelType::Object, 1);
}

//=============================================================================
// GB FIFO Tests
//=============================================================================

TEST(GbFifoEntryTest, DefaultInitialization) {
	GbFifoEntry entry{};
	EXPECT_EQ(entry.Color, 0u);
	EXPECT_EQ(entry.Attributes, 0u);
	EXPECT_EQ(entry.Index, 0u);
}

TEST(GbPpuFifoTest, DefaultInitialization) {
	GbPpuFifo fifo{};
	EXPECT_EQ(fifo.Position, 0u);
	EXPECT_EQ(fifo.Size, 0u);
}

TEST(GbPpuFifoTest, ResetClearsState) {
	GbPpuFifo fifo{};
	fifo.Position = 3;
	fifo.Size = 5;
	fifo.Reset();
	EXPECT_EQ(fifo.Position, 0u);
	EXPECT_EQ(fifo.Size, 0u);
}

//=============================================================================
// GB PPU Fetcher Tests
//=============================================================================

TEST(GbPpuFetcherTest, DefaultInitialization) {
	GbPpuFetcher fetcher{};
	EXPECT_EQ(fetcher.Addr, 0u);
	EXPECT_EQ(fetcher.Attributes, 0u);
	EXPECT_EQ(fetcher.Step, 0u);
	EXPECT_EQ(fetcher.LowByte, 0u);
	EXPECT_EQ(fetcher.HighByte, 0u);
}

//=============================================================================
// GB DMA Controller State Tests
//=============================================================================

TEST(GbDmaControllerStateTest, DefaultInitialization) {
	GbDmaControllerState state{};
	// OAM DMA
	EXPECT_EQ(state.OamDmaSource, 0u);
	EXPECT_EQ(state.DmaStartDelay, 0u);
	EXPECT_EQ(state.InternalDest, 0u);
	EXPECT_EQ(state.DmaCounter, 0u);
	EXPECT_EQ(state.DmaReadBuffer, 0u);
	EXPECT_FALSE(state.OamDmaRunning);
	// CGB HDMA
	EXPECT_EQ(state.CgbDmaSource, 0u);
	EXPECT_EQ(state.CgbDmaDest, 0u);
	EXPECT_EQ(state.CgbDmaLength, 0u);
	EXPECT_FALSE(state.CgbHdmaRunning);
}

//=============================================================================
// GB Timer State Tests
//=============================================================================

TEST(GbTimerStateTest, DefaultInitialization) {
	GbTimerState state{};
	EXPECT_EQ(state.Divider, 0u);
	EXPECT_FALSE(state.NeedReload);
	EXPECT_FALSE(state.Reloaded);
	EXPECT_EQ(state.Counter, 0u);
	EXPECT_EQ(state.Modulo, 0u);
	EXPECT_EQ(state.Control, 0u);
	EXPECT_FALSE(state.TimerEnabled);
	EXPECT_EQ(state.TimerDivider, 0u);
}

//=============================================================================
// GB APU State Tests
//=============================================================================

TEST(GbApuStateTest, DefaultInitialization) {
	GbApuState state{};
	EXPECT_FALSE(state.ApuEnabled);
	EXPECT_EQ(state.LeftVolume, 0u);
	EXPECT_EQ(state.RightVolume, 0u);
	EXPECT_EQ(state.FrameSequenceStep, 0u);
	EXPECT_FALSE(state.ExtAudioLeftEnabled);
	EXPECT_FALSE(state.ExtAudioRightEnabled);
}

TEST(GbApuStateTest, ChannelOutputEnable) {
	GbApuState state{};
	state.EnableLeftSq1 = 1;
	state.EnableRightSq2 = 1;
	state.EnableLeftWave = 1;
	state.EnableRightNoise = 1;
	EXPECT_EQ(state.EnableLeftSq1, 1);
	EXPECT_EQ(state.EnableRightSq2, 1);
	EXPECT_EQ(state.EnableLeftWave, 1);
	EXPECT_EQ(state.EnableRightNoise, 1);
	EXPECT_EQ(state.EnableRightSq1, 0);
	EXPECT_EQ(state.EnableLeftSq2, 0);
}

//=============================================================================
// GB Square State Tests
//=============================================================================

TEST(GbSquareStateTest, DefaultInitialization) {
	GbSquareState state{};
	EXPECT_EQ(state.Frequency, 0u);
	EXPECT_EQ(state.Timer, 0u);
	EXPECT_EQ(state.Volume, 0u);
	EXPECT_EQ(state.EnvVolume, 0u);
	EXPECT_FALSE(state.EnvRaiseVolume);
	EXPECT_EQ(state.EnvPeriod, 0u);
	EXPECT_EQ(state.EnvTimer, 0u);
	EXPECT_FALSE(state.EnvStopped);
	EXPECT_EQ(state.Duty, 0u);
	EXPECT_EQ(state.DutyPos, 0u);
	EXPECT_EQ(state.Length, 0u);
	EXPECT_FALSE(state.LengthEnabled);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.Output, 0u);
}

TEST(GbSquareStateTest, SweepFields) {
	GbSquareState state{};
	EXPECT_EQ(state.SweepTimer, 0u);
	EXPECT_EQ(state.SweepFreq, 0u);
	EXPECT_EQ(state.SweepPeriod, 0u);
	EXPECT_FALSE(state.SweepNegate);
	EXPECT_EQ(state.SweepShift, 0u);
	EXPECT_FALSE(state.SweepEnabled);
	EXPECT_FALSE(state.SweepNegateCalcDone);
}

//=============================================================================
// GB Noise State Tests
//=============================================================================

TEST(GbNoiseStateTest, DefaultInitialization) {
	GbNoiseState state{};
	EXPECT_EQ(state.Volume, 0u);
	EXPECT_EQ(state.EnvVolume, 0u);
	EXPECT_FALSE(state.EnvRaiseVolume);
	EXPECT_EQ(state.EnvPeriod, 0u);
	EXPECT_EQ(state.EnvTimer, 0u);
	EXPECT_FALSE(state.EnvStopped);
	EXPECT_EQ(state.Length, 0u);
	EXPECT_FALSE(state.LengthEnabled);
	EXPECT_EQ(state.ShiftRegister, 0u);
	EXPECT_EQ(state.PeriodShift, 0u);
	EXPECT_EQ(state.Divisor, 0u);
	EXPECT_FALSE(state.ShortWidthMode);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.Timer, 0u);
	EXPECT_EQ(state.Output, 0u);
}

//=============================================================================
// GB Wave State Tests
//=============================================================================

TEST(GbWaveStateTest, DefaultInitialization) {
	GbWaveState state{};
	EXPECT_FALSE(state.DacEnabled);
	EXPECT_EQ(state.SampleBuffer, 0u);
	EXPECT_EQ(state.Position, 0u);
	EXPECT_EQ(state.Volume, 0u);
	EXPECT_EQ(state.Frequency, 0u);
	EXPECT_EQ(state.Length, 0u);
	EXPECT_FALSE(state.LengthEnabled);
	EXPECT_FALSE(state.Enabled);
	EXPECT_EQ(state.Timer, 0u);
	EXPECT_EQ(state.Output, 0u);
}

TEST(GbWaveStateTest, WaveRamArray) {
	GbWaveState state{};
	for (int i = 0; i < 16; i++) {
		EXPECT_EQ(state.Ram[i], 0) << "Wave RAM[" << i << "] not zero";
	}
	// Write a pattern: typical triangle wave
	state.Ram[0] = 0x01;
	state.Ram[7] = 0xff;
	state.Ram[15] = 0x80;
	EXPECT_EQ(state.Ram[0], 0x01);
	EXPECT_EQ(state.Ram[7], 0xff);
	EXPECT_EQ(state.Ram[15], 0x80);
}

//=============================================================================
// GB APU Debug State Tests
//=============================================================================

TEST(GbApuDebugStateTest, DefaultInitialization) {
	GbApuDebugState state{};
	EXPECT_FALSE(state.Common.ApuEnabled);
	EXPECT_FALSE(state.Square1.Enabled);
	EXPECT_FALSE(state.Square2.Enabled);
	EXPECT_FALSE(state.Wave.Enabled);
	EXPECT_FALSE(state.Noise.Enabled);
}

//=============================================================================
// GB Memory Manager State Tests
//=============================================================================

TEST(GbMemoryManagerStateTest, DefaultInitialization) {
	GbMemoryManagerState state{};
	EXPECT_EQ(state.ApuCycleCount, 0u);
	EXPECT_EQ(state.CgbWorkRamBank, 0u);
	EXPECT_FALSE(state.CgbSwitchSpeedRequest);
	EXPECT_FALSE(state.CgbHighSpeed);
	EXPECT_FALSE(state.DisableBootRom);
	EXPECT_EQ(state.IrqRequests, 0u);
	EXPECT_EQ(state.IrqEnabled, 0u);
	EXPECT_EQ(state.SerialData, 0u);
	EXPECT_EQ(state.SerialControl, 0u);
	EXPECT_EQ(state.SerialBitCount, 0u);
}

TEST(GbMemoryManagerStateTest, CgbRegisters) {
	GbMemoryManagerState state{};
	state.CgbRegRpInfrared = 0x3e;
	state.CgbRegFF72 = 0x42;
	state.CgbRegFF73 = 0x43;
	state.CgbRegFF74 = 0x44;
	state.CgbRegFF75 = 0x45;
	EXPECT_EQ(state.CgbRegRpInfrared, 0x3e);
	EXPECT_EQ(state.CgbRegFF72, 0x42);
	EXPECT_EQ(state.CgbRegFF73, 0x43);
	EXPECT_EQ(state.CgbRegFF74, 0x44);
	EXPECT_EQ(state.CgbRegFF75, 0x45);
}

//=============================================================================
// GB Controller Manager State Tests
//=============================================================================

TEST(GbControlManagerStateTest, DefaultInitialization) {
	GbControlManagerState state{};
	EXPECT_EQ(state.InputSelect, 0u);
}

//=============================================================================
// GB Type Tests
//=============================================================================

TEST(GbTypeTest, EnumValues) {
	EXPECT_EQ((int)GbType::Gb, 0);
	EXPECT_EQ((int)GbType::Cgb, 1);
}

//=============================================================================
// CGB Compat Tests
//=============================================================================

TEST(CgbCompatTest, EnumValues) {
	EXPECT_EQ((uint8_t)CgbCompat::Gameboy, 0x00);
	EXPECT_EQ((uint8_t)CgbCompat::GameboyColorSupport, 0x80);
	EXPECT_EQ((uint8_t)CgbCompat::GameboyColorExclusive, 0xc0);
}

//=============================================================================
// GB Register Access Tests
//=============================================================================

TEST(RegisterAccessTest, EnumValues) {
	EXPECT_EQ((int)RegisterAccess::None, 0);
	EXPECT_EQ((int)RegisterAccess::Read, 1);
	EXPECT_EQ((int)RegisterAccess::Write, 2);
	EXPECT_EQ((int)RegisterAccess::ReadWrite, 3);
}

TEST(RegisterAccessTest, ReadWriteIsCombination) {
	int rw = (int)RegisterAccess::Read | (int)RegisterAccess::Write;
	EXPECT_EQ(rw, (int)RegisterAccess::ReadWrite);
}

//=============================================================================
// GB Constants Tests
//=============================================================================

TEST(GbConstantsTest, ScreenDimensions) {
	EXPECT_EQ(GbConstants::ScreenWidth, 160u);
	EXPECT_EQ(GbConstants::ScreenHeight, 144u);
	EXPECT_EQ(GbConstants::PixelCount, 160u * 144u);
}

//=============================================================================
// GB Aggregate State Tests
//=============================================================================

TEST(GbStateTest, DefaultInitialization) {
	GbState state{};
	EXPECT_EQ(state.Cpu.CycleCount, 0u);
	EXPECT_EQ(state.Ppu.FrameCount, 0u);
	EXPECT_FALSE(state.Apu.Common.ApuEnabled);
	EXPECT_EQ(state.MemoryManager.ApuCycleCount, 0u);
	EXPECT_EQ(state.Timer.Divider, 0u);
	EXPECT_FALSE(state.Dma.OamDmaRunning);
	EXPECT_FALSE(state.HasBattery);
}

TEST(GbStateTest, TypeField) {
	GbState state{};
	state.Type = GbType::Gb;
	EXPECT_EQ(state.Type, GbType::Gb);
	state.Type = GbType::Cgb;
	EXPECT_EQ(state.Type, GbType::Cgb);
}
