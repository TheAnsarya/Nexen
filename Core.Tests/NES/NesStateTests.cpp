#include "pch.h"
#include "NES/NesTypes.h"

// ═══════════════════════════════════════════════════════════════════════════════
// PSFlags — 6502 Processor Status flags
// ═══════════════════════════════════════════════════════════════════════════════

TEST(PsFlagsTest, FlagValues)
{
	EXPECT_EQ(PSFlags::Carry, 0x01);
	EXPECT_EQ(PSFlags::Zero, 0x02);
	EXPECT_EQ(PSFlags::Interrupt, 0x04);
	EXPECT_EQ(PSFlags::Decimal, 0x08);
	EXPECT_EQ(PSFlags::Break, 0x10);
	EXPECT_EQ(PSFlags::Reserved, 0x20);
	EXPECT_EQ(PSFlags::Overflow, 0x40);
	EXPECT_EQ(PSFlags::Negative, 0x80);
}

TEST(PsFlagsTest, FlagsArePowersOfTwo)
{
	uint8_t flags[] = {
		PSFlags::Carry, PSFlags::Zero, PSFlags::Interrupt, PSFlags::Decimal,
		PSFlags::Break, PSFlags::Reserved, PSFlags::Overflow, PSFlags::Negative
	};
	for(int i = 0; i < 8; i++) {
		EXPECT_EQ(flags[i] & (flags[i] - 1), 0) << "Flag " << i << " not power of two";
	}
}

TEST(PsFlagsTest, AllFlagsCombined)
{
	uint8_t all = PSFlags::Carry | PSFlags::Zero | PSFlags::Interrupt | PSFlags::Decimal |
		PSFlags::Break | PSFlags::Reserved | PSFlags::Overflow | PSFlags::Negative;
	EXPECT_EQ(all, 0xff);
}

TEST(PsFlagsTest, CommonCombinations)
{
	// NV-BDIZC typical initial state ($34 = IRQ disabled, break, reserved)
	uint8_t initial = PSFlags::Interrupt | PSFlags::Break | PSFlags::Reserved;
	EXPECT_EQ(initial, 0x34);
}

// ═══════════════════════════════════════════════════════════════════════════════
// NesAddrMode — 6502 Addressing modes
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesAddrModeTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(NesAddrMode::None), 0);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Acc), 1);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Imp), 2);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Imm), 3);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Rel), 4);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Zero), 5);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Abs), 6);
	EXPECT_EQ(static_cast<int>(NesAddrMode::ZeroX), 7);
	EXPECT_EQ(static_cast<int>(NesAddrMode::ZeroY), 8);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Ind), 9);
	EXPECT_EQ(static_cast<int>(NesAddrMode::IndX), 10);
	EXPECT_EQ(static_cast<int>(NesAddrMode::IndY), 11);
	EXPECT_EQ(static_cast<int>(NesAddrMode::IndYW), 12);
	EXPECT_EQ(static_cast<int>(NesAddrMode::AbsX), 13);
	EXPECT_EQ(static_cast<int>(NesAddrMode::AbsXW), 14);
	EXPECT_EQ(static_cast<int>(NesAddrMode::AbsY), 15);
	EXPECT_EQ(static_cast<int>(NesAddrMode::AbsYW), 16);
	EXPECT_EQ(static_cast<int>(NesAddrMode::Other), 17);
}

// ═══════════════════════════════════════════════════════════════════════════════
// IRQSource — NES interrupt sources
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesIrqSourceTest, FlagValues)
{
	EXPECT_EQ(static_cast<int>(IRQSource::External), 1);
	EXPECT_EQ(static_cast<int>(IRQSource::FrameCounter), 2);
	EXPECT_EQ(static_cast<int>(IRQSource::DMC), 4);
	EXPECT_EQ(static_cast<int>(IRQSource::FdsDisk), 8);
	EXPECT_EQ(static_cast<int>(IRQSource::Epsm), 16);
}

TEST(NesIrqSourceTest, FlagsArePowersOfTwo)
{
	int sources[] = {
		static_cast<int>(IRQSource::External),
		static_cast<int>(IRQSource::FrameCounter),
		static_cast<int>(IRQSource::DMC),
		static_cast<int>(IRQSource::FdsDisk),
		static_cast<int>(IRQSource::Epsm)
	};
	for(int i = 0; i < 5; i++) {
		EXPECT_EQ(sources[i] & (sources[i] - 1), 0) << "IRQ source " << i << " not power of two";
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// MemoryOperation
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesMemoryOperationTest, Values)
{
	EXPECT_EQ(static_cast<int>(MemoryOperation::Read), 1);
	EXPECT_EQ(static_cast<int>(MemoryOperation::Write), 2);
	EXPECT_EQ(static_cast<int>(MemoryOperation::Any), 3);
}

TEST(NesMemoryOperationTest, AnyIsReadOrWrite)
{
	int any = static_cast<int>(MemoryOperation::Read) | static_cast<int>(MemoryOperation::Write);
	EXPECT_EQ(any, static_cast<int>(MemoryOperation::Any));
}

// ═══════════════════════════════════════════════════════════════════════════════
// PrgMemoryType / ChrMemoryType
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesPrgMemoryTypeTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(PrgMemoryType::PrgRom), 0);
	EXPECT_EQ(static_cast<int>(PrgMemoryType::SaveRam), 1);
	EXPECT_EQ(static_cast<int>(PrgMemoryType::WorkRam), 2);
	EXPECT_EQ(static_cast<int>(PrgMemoryType::MapperRam), 3);
}

TEST(NesChrMemoryTypeTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(ChrMemoryType::Default), 0);
	EXPECT_EQ(static_cast<int>(ChrMemoryType::ChrRom), 1);
	EXPECT_EQ(static_cast<int>(ChrMemoryType::ChrRam), 2);
	EXPECT_EQ(static_cast<int>(ChrMemoryType::NametableRam), 3);
	EXPECT_EQ(static_cast<int>(ChrMemoryType::MapperRam), 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MemoryAccessType
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesMemoryAccessTypeTest, Values)
{
	EXPECT_EQ(static_cast<int>(MemoryAccessType::Unspecified), -1);
	EXPECT_EQ(static_cast<int>(MemoryAccessType::NoAccess), 0);
	EXPECT_EQ(static_cast<int>(MemoryAccessType::Read), 1);
	EXPECT_EQ(static_cast<int>(MemoryAccessType::Write), 2);
	EXPECT_EQ(static_cast<int>(MemoryAccessType::ReadWrite), 3);
}

TEST(NesMemoryAccessTypeTest, ReadWriteIsCombination)
{
	int rw = MemoryAccessType::Read | MemoryAccessType::Write;
	EXPECT_EQ(rw, MemoryAccessType::ReadWrite);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MirroringType
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesMirroringTypeTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(MirroringType::Horizontal), 0);
	EXPECT_EQ(static_cast<int>(MirroringType::Vertical), 1);
	EXPECT_EQ(static_cast<int>(MirroringType::ScreenAOnly), 2);
	EXPECT_EQ(static_cast<int>(MirroringType::ScreenBOnly), 3);
	EXPECT_EQ(static_cast<int>(MirroringType::FourScreens), 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// MapperStateValueType
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesMapperStateValueTypeTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(MapperStateValueType::None), 0);
	EXPECT_EQ(static_cast<int>(MapperStateValueType::String), 1);
	EXPECT_EQ(static_cast<int>(MapperStateValueType::Bool), 2);
	EXPECT_EQ(static_cast<int>(MapperStateValueType::Number8), 3);
	EXPECT_EQ(static_cast<int>(MapperStateValueType::Number16), 4);
	EXPECT_EQ(static_cast<int>(MapperStateValueType::Number32), 5);
}

// ═══════════════════════════════════════════════════════════════════════════════
// NesCpuState — 6502 CPU
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesCpuStateTest, DefaultState)
{
	NesCpuState cpu = {};
	EXPECT_EQ(cpu.CycleCount, 0u);
	EXPECT_EQ(cpu.PC, 0u);
	EXPECT_EQ(cpu.SP, 0u);
	EXPECT_EQ(cpu.A, 0u);
	EXPECT_EQ(cpu.X, 0u);
	EXPECT_EQ(cpu.Y, 0u);
	EXPECT_EQ(cpu.PS, 0u);
	EXPECT_EQ(cpu.IrqFlag, 0u);
	EXPECT_FALSE(cpu.NmiFlag);
}

TEST(NesCpuStateTest, RegisterMaxValues)
{
	NesCpuState cpu = {};
	cpu.PC = 0xffff;
	cpu.SP = 0xff;
	cpu.A = 0xff;
	cpu.X = 0xff;
	cpu.Y = 0xff;
	cpu.PS = 0xff;
	EXPECT_EQ(cpu.PC, 0xffffu);
	EXPECT_EQ(cpu.A, 0xffu);
}

TEST(NesCpuStateTest, StackPointerRange)
{
	NesCpuState cpu = {};
	// Stack is $0100-$01FF, SP indexes into this page
	cpu.SP = 0xfd; // Typical initial value
	EXPECT_EQ(cpu.SP, 0xfd);
	cpu.SP = 0x00; // Stack full
	EXPECT_EQ(cpu.SP, 0x00);
}

TEST(NesCpuStateTest, IrqFlagCombinations)
{
	NesCpuState cpu = {};
	cpu.IrqFlag = static_cast<uint8_t>(IRQSource::External) | static_cast<uint8_t>(IRQSource::DMC);
	EXPECT_EQ(cpu.IrqFlag, 5);
}

// ═══════════════════════════════════════════════════════════════════════════════
// PPUStatusFlags / PpuControlFlags / PpuMaskFlags
// ═══════════════════════════════════════════════════════════════════════════════

TEST(PpuStatusFlagsTest, DefaultState)
{
	PPUStatusFlags status = {};
	EXPECT_FALSE(status.SpriteOverflow);
	EXPECT_FALSE(status.Sprite0Hit);
	EXPECT_FALSE(status.VerticalBlank);
}

TEST(PpuControlFlagsTest, DefaultState)
{
	PpuControlFlags ctrl = {};
	EXPECT_EQ(ctrl.BackgroundPatternAddr, 0u);
	EXPECT_EQ(ctrl.SpritePatternAddr, 0u);
	EXPECT_FALSE(ctrl.VerticalWrite);
	EXPECT_FALSE(ctrl.LargeSprites);
	EXPECT_FALSE(ctrl.SecondaryPpu);
	EXPECT_FALSE(ctrl.NmiOnVerticalBlank);
}

TEST(PpuControlFlagsTest, PatternTableAddresses)
{
	PpuControlFlags ctrl = {};
	ctrl.BackgroundPatternAddr = 0x0000;
	ctrl.SpritePatternAddr = 0x1000;
	EXPECT_EQ(ctrl.BackgroundPatternAddr, 0x0000u);
	EXPECT_EQ(ctrl.SpritePatternAddr, 0x1000u);
}

TEST(PpuMaskFlagsTest, DefaultState)
{
	PpuMaskFlags mask = {};
	EXPECT_FALSE(mask.Grayscale);
	EXPECT_FALSE(mask.BackgroundMask);
	EXPECT_FALSE(mask.SpriteMask);
	EXPECT_FALSE(mask.BackgroundEnabled);
	EXPECT_FALSE(mask.SpritesEnabled);
	EXPECT_FALSE(mask.IntensifyRed);
	EXPECT_FALSE(mask.IntensifyGreen);
	EXPECT_FALSE(mask.IntensifyBlue);
}

TEST(PpuMaskFlagsTest, AllEnabled)
{
	PpuMaskFlags mask = {};
	mask.BackgroundEnabled = true;
	mask.SpritesEnabled = true;
	mask.BackgroundMask = true;
	mask.SpriteMask = true;
	EXPECT_TRUE(mask.BackgroundEnabled);
	EXPECT_TRUE(mask.SpritesEnabled);
}

// ═══════════════════════════════════════════════════════════════════════════════
// NesPpuState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesPpuStateTest, DefaultState)
{
	NesPpuState ppu = {};
	EXPECT_EQ(ppu.Scanline, 0);
	EXPECT_EQ(ppu.Cycle, 0u);
	EXPECT_EQ(ppu.FrameCount, 0u);
	EXPECT_EQ(ppu.VideoRamAddr, 0u);
	EXPECT_EQ(ppu.TmpVideoRamAddr, 0u);
	EXPECT_EQ(ppu.ScrollX, 0u);
	EXPECT_FALSE(ppu.WriteToggle);
	EXPECT_EQ(ppu.SpriteRamAddr, 0u);
}

TEST(NesPpuStateTest, NtscScanlineCount)
{
	NesPpuState ppu = {};
	ppu.ScanlineCount = 262; // NTSC
	ppu.NmiScanline = 241;
	EXPECT_EQ(ppu.ScanlineCount, 262u);
	EXPECT_EQ(ppu.NmiScanline, 241u);
}

TEST(NesPpuStateTest, PalScanlineCount)
{
	NesPpuState ppu = {};
	ppu.ScanlineCount = 312; // PAL
	ppu.NmiScanline = 241;
	EXPECT_EQ(ppu.ScanlineCount, 312u);
}

TEST(NesPpuStateTest, VramAddrRange)
{
	NesPpuState ppu = {};
	ppu.VideoRamAddr = 0x3fff; // 14-bit address space
	EXPECT_EQ(ppu.VideoRamAddr, 0x3fffu);
}

TEST(NesPpuStateTest, ScrollXRange)
{
	NesPpuState ppu = {};
	ppu.ScrollX = 7; // Fine X: 0-7 (3 bits)
	EXPECT_EQ(ppu.ScrollX, 7u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// TileInfo / NesSpriteInfo
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesTileInfoTest, DefaultState)
{
	TileInfo tile = {};
	EXPECT_EQ(tile.TileAddr, 0u);
	EXPECT_EQ(tile.LowByte, 0u);
	EXPECT_EQ(tile.HighByte, 0u);
	EXPECT_EQ(tile.PaletteOffset, 0u);
}

TEST(NesTileInfoTest, PaletteOffsets)
{
	// BG palette offsets: 0, 4, 8, 12
	TileInfo tile = {};
	uint8_t offsets[] = {0, 4, 8, 12};
	for(auto off : offsets) {
		tile.PaletteOffset = off;
		EXPECT_EQ(tile.PaletteOffset, off);
	}
}

TEST(NesSpriteInfoTest, DefaultState)
{
	NesSpriteInfo sprite = {};
	EXPECT_FALSE(sprite.HorizontalMirror);
	EXPECT_FALSE(sprite.BackgroundPriority);
	EXPECT_EQ(sprite.SpriteX, 0u);
	EXPECT_EQ(sprite.LowByte, 0u);
	EXPECT_EQ(sprite.HighByte, 0u);
	EXPECT_EQ(sprite.PaletteOffset, 0u);
}

TEST(NesSpriteInfoTest, SpritePaletteOffsets)
{
	// Sprite palette offsets: 16, 20, 24, 28
	NesSpriteInfo sprite = {};
	uint8_t offsets[] = {16, 20, 24, 28};
	for(auto off : offsets) {
		sprite.PaletteOffset = off;
		EXPECT_EQ(sprite.PaletteOffset, off);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuLengthCounterState / ApuEnvelopeState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuLengthCounterTest, DefaultState)
{
	ApuLengthCounterState lc = {};
	EXPECT_FALSE(lc.Halt);
	EXPECT_EQ(lc.Counter, 0u);
	EXPECT_EQ(lc.ReloadValue, 0u);
}

TEST(NesApuEnvelopeTest, DefaultState)
{
	ApuEnvelopeState env = {};
	EXPECT_FALSE(env.StartFlag);
	EXPECT_FALSE(env.Loop);
	EXPECT_FALSE(env.ConstantVolume);
	EXPECT_EQ(env.Divider, 0u);
	EXPECT_EQ(env.Counter, 0u);
	EXPECT_EQ(env.Volume, 0u);
}

TEST(NesApuEnvelopeTest, VolumeRange)
{
	ApuEnvelopeState env = {};
	env.Volume = 15;
	EXPECT_EQ(env.Volume, 15);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuSquareState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuSquareTest, DefaultState)
{
	ApuSquareState sq = {};
	EXPECT_EQ(sq.Duty, 0u);
	EXPECT_EQ(sq.Period, 0u);
	EXPECT_FALSE(sq.Enabled);
	EXPECT_EQ(sq.OutputVolume, 0u);
}

TEST(NesApuSquareTest, DutyCycleRange)
{
	ApuSquareState sq = {};
	for(uint8_t d = 0; d < 4; d++) {
		sq.Duty = d;
		EXPECT_EQ(sq.Duty, d);
	}
}

TEST(NesApuSquareTest, SweepConfig)
{
	ApuSquareState sq = {};
	sq.SweepEnabled = true;
	sq.SweepNegate = true;
	sq.SweepPeriod = 7;
	sq.SweepShift = 7;
	EXPECT_TRUE(sq.SweepEnabled);
	EXPECT_TRUE(sq.SweepNegate);
	EXPECT_EQ(sq.SweepPeriod, 7);
	EXPECT_EQ(sq.SweepShift, 7);
}

TEST(NesApuSquareTest, PeriodRange)
{
	ApuSquareState sq = {};
	sq.Period = 0x7ff; // 11-bit max
	EXPECT_EQ(sq.Period, 0x7ff);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuTriangleState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuTriangleTest, DefaultState)
{
	ApuTriangleState tri = {};
	EXPECT_EQ(tri.Period, 0u);
	EXPECT_EQ(tri.SequencePosition, 0u);
	EXPECT_FALSE(tri.Enabled);
	EXPECT_EQ(tri.LinearCounter, 0u);
	EXPECT_FALSE(tri.LinearReloadFlag);
}

TEST(NesApuTriangleTest, SequencePositionRange)
{
	ApuTriangleState tri = {};
	// 32-step triangle waveform
	tri.SequencePosition = 31;
	EXPECT_EQ(tri.SequencePosition, 31);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuNoiseState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuNoiseTest, DefaultState)
{
	ApuNoiseState noise = {};
	EXPECT_EQ(noise.Period, 0u);
	EXPECT_EQ(noise.ShiftRegister, 0u);
	EXPECT_FALSE(noise.ModeFlag);
	EXPECT_FALSE(noise.Enabled);
}

TEST(NesApuNoiseTest, LfsrMaxValue)
{
	ApuNoiseState noise = {};
	noise.ShiftRegister = 0x7fff; // 15-bit
	EXPECT_EQ(noise.ShiftRegister, 0x7fffu);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuDmcState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuDmcTest, DefaultState)
{
	ApuDmcState dmc = {};
	EXPECT_EQ(dmc.SampleAddr, 0u);
	EXPECT_EQ(dmc.SampleLength, 0u);
	EXPECT_FALSE(dmc.Loop);
	EXPECT_FALSE(dmc.IrqEnabled);
	EXPECT_EQ(dmc.OutputVolume, 0u);
}

TEST(NesApuDmcTest, SampleAddressRange)
{
	ApuDmcState dmc = {};
	dmc.SampleAddr = 0xc000; // Minimum DMC address
	EXPECT_EQ(dmc.SampleAddr, 0xc000u);
	dmc.SampleAddr = 0xffff; // Maximum
	EXPECT_EQ(dmc.SampleAddr, 0xffffu);
}

TEST(NesApuDmcTest, OutputVolumeRange)
{
	ApuDmcState dmc = {};
	dmc.OutputVolume = 127; // 7-bit DAC
	EXPECT_EQ(dmc.OutputVolume, 127);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuFrameCounterState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuFrameCounterTest, DefaultState)
{
	ApuFrameCounterState fc = {};
	EXPECT_FALSE(fc.FiveStepMode);
	EXPECT_EQ(fc.SequencePosition, 0u);
	EXPECT_FALSE(fc.IrqEnabled);
}

TEST(NesApuFrameCounterTest, SequencePositionRange)
{
	ApuFrameCounterState fc = {};
	fc.SequencePosition = 4; // Max position in 5-step mode
	EXPECT_EQ(fc.SequencePosition, 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// ApuState — Complete APU aggregate
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesApuStateTest, AllChannelsAccessible)
{
	ApuState apu = {};
	apu.Square1.Enabled = true;
	apu.Square1.Duty = 2;
	apu.Square2.Enabled = false;
	apu.Triangle.Enabled = true;
	apu.Noise.Enabled = false;
	apu.Dmc.Loop = true;
	apu.FrameCounter.FiveStepMode = true;

	EXPECT_TRUE(apu.Square1.Enabled);
	EXPECT_EQ(apu.Square1.Duty, 2);
	EXPECT_FALSE(apu.Square2.Enabled);
	EXPECT_TRUE(apu.Triangle.Enabled);
	EXPECT_FALSE(apu.Noise.Enabled);
	EXPECT_TRUE(apu.Dmc.Loop);
	EXPECT_TRUE(apu.FrameCounter.FiveStepMode);
}

// ═══════════════════════════════════════════════════════════════════════════════
// CartridgeState — Memory mapping
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesCartridgeStateTest, DefaultSizes)
{
	CartridgeState cart = {};
	EXPECT_EQ(cart.PrgRomSize, 0u);
	EXPECT_EQ(cart.ChrRomSize, 0u);
	EXPECT_EQ(cart.ChrRamSize, 0u);
	EXPECT_FALSE(cart.HasBattery);
	EXPECT_EQ(cart.CustomEntryCount, 0u);
}

TEST(NesCartridgeStateTest, PrgMemoryMapping)
{
	CartridgeState cart = {};
	// 256 PRG pages of 256 bytes = 64K address space
	cart.PrgMemoryOffset[0] = 0;
	cart.PrgMemoryOffset[0xff] = 0x7f00;
	cart.PrgType[0] = PrgMemoryType::PrgRom;
	cart.PrgType[0x60] = PrgMemoryType::WorkRam;
	cart.PrgMemoryAccess[0x80] = MemoryAccessType::Read;
	EXPECT_EQ(cart.PrgType[0], PrgMemoryType::PrgRom);
	EXPECT_EQ(cart.PrgType[0x60], PrgMemoryType::WorkRam);
	EXPECT_EQ(cart.PrgMemoryAccess[0x80], MemoryAccessType::Read);
}

TEST(NesCartridgeStateTest, ChrMemoryMapping)
{
	CartridgeState cart = {};
	// 64 CHR pages of 256 bytes = 16K PPU address space
	cart.ChrType[0] = ChrMemoryType::ChrRom;
	cart.ChrType[0x20] = ChrMemoryType::NametableRam;
	EXPECT_EQ(cart.ChrType[0], ChrMemoryType::ChrRom);
	EXPECT_EQ(cart.ChrType[0x20], ChrMemoryType::NametableRam);
}

TEST(NesCartridgeStateTest, MirroringAssignment)
{
	CartridgeState cart = {};
	cart.Mirroring = MirroringType::Horizontal;
	EXPECT_EQ(cart.Mirroring, MirroringType::Horizontal);
	cart.Mirroring = MirroringType::Vertical;
	EXPECT_EQ(cart.Mirroring, MirroringType::Vertical);
	cart.Mirroring = MirroringType::FourScreens;
	EXPECT_EQ(cart.Mirroring, MirroringType::FourScreens);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GameSystem — NES system variants
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesGameSystemTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(GameSystem::NesNtsc), 0);
	EXPECT_EQ(static_cast<int>(GameSystem::NesPal), 1);
	EXPECT_EQ(static_cast<int>(GameSystem::Famicom), 2);
	EXPECT_EQ(static_cast<int>(GameSystem::Dendy), 3);
	EXPECT_EQ(static_cast<int>(GameSystem::VsSystem), 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// NesState — Complete system aggregate
// ═══════════════════════════════════════════════════════════════════════════════

TEST(NesStateTest, AllSubsystemsAccessible)
{
	NesState state = {};
	state.Cpu.PC = 0x8000;
	state.Cpu.A = 0x42;
	state.Cpu.CycleCount = 1000;
	state.Ppu.FrameCount = 60;
	state.Ppu.Scanline = 241;
	state.Apu.Square1.Enabled = true;
	state.Cartridge.PrgRomSize = 32768;
	state.ClockRate = 1789773; // NTSC master clock / 12

	EXPECT_EQ(state.Cpu.PC, 0x8000u);
	EXPECT_EQ(state.Cpu.A, 0x42);
	EXPECT_EQ(state.Ppu.FrameCount, 60u);
	EXPECT_EQ(state.Ppu.Scanline, 241);
	EXPECT_TRUE(state.Apu.Square1.Enabled);
	EXPECT_EQ(state.Cartridge.PrgRomSize, 32768u);
	EXPECT_EQ(state.ClockRate, 1789773u);
}

TEST(NesStateTest, ApuSubchannelTraversal)
{
	NesState state = {};
	state.Apu.Square1.Duty = 2;
	state.Apu.Square1.Envelope.Volume = 15;
	state.Apu.Square1.LengthCounter.Counter = 10;
	state.Apu.Square2.Period = 0x100;
	state.Apu.Triangle.LinearCounter = 0x40;
	state.Apu.Noise.ShiftRegister = 0x4000;
	state.Apu.Dmc.OutputVolume = 64;
	state.Apu.FrameCounter.FiveStepMode = true;

	EXPECT_EQ(state.Apu.Square1.Envelope.Volume, 15);
	EXPECT_EQ(state.Apu.Square1.LengthCounter.Counter, 10);
	EXPECT_EQ(state.Apu.Square2.Period, 0x100u);
	EXPECT_EQ(state.Apu.Triangle.LinearCounter, 0x40);
	EXPECT_EQ(state.Apu.Noise.ShiftRegister, 0x4000u);
	EXPECT_EQ(state.Apu.Dmc.OutputVolume, 64);
	EXPECT_TRUE(state.Apu.FrameCounter.FiveStepMode);
}
