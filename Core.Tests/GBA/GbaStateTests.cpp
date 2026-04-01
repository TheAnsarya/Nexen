#include "pch.h"
#include "GBA/GbaTypes.h"
#include "Shared/SettingTypes.h"
#include "GBA/Cart/GbaEeprom.h"

// ═══════════════════════════════════════════════════════════════════════════════
// GBA Constants
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaConstantsTest, ScreenDimensions)
{
	EXPECT_EQ(GbaConstants::ScreenWidth, 240u);
	EXPECT_EQ(GbaConstants::ScreenHeight, 160u);
}

TEST(GbaConstantsTest, PixelCount)
{
	EXPECT_EQ(GbaConstants::PixelCount, 240u * 160u);
	EXPECT_EQ(GbaConstants::PixelCount, 38400u);
}

TEST(GbaConstantsTest, ScanlineCount)
{
	EXPECT_EQ(GbaConstants::ScanlineCount, 228u);
}

TEST(GbaConstantsTest, VBlankScanlines)
{
	// 228 total - 160 visible = 68 vblank scanlines
	uint32_t vblank = GbaConstants::ScanlineCount - GbaConstants::ScreenHeight;
	EXPECT_EQ(vblank, 68u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCpuMode — ARM7TDMI operating modes
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCpuModeTest, UserMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::User), 0x10);
}

TEST(GbaCpuModeTest, FiqMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::Fiq), 0x11);
}

TEST(GbaCpuModeTest, IrqMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::Irq), 0x12);
}

TEST(GbaCpuModeTest, SupervisorMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::Supervisor), 0x13);
}

TEST(GbaCpuModeTest, AbortMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::Abort), 0x17);
}

TEST(GbaCpuModeTest, UndefinedMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::Undefined), 0x1b);
}

TEST(GbaCpuModeTest, SystemMode)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaCpuMode::System), 0x1f);
}

TEST(GbaCpuModeTest, AllModesDistinct)
{
	uint8_t modes[] = {
		static_cast<uint8_t>(GbaCpuMode::User),
		static_cast<uint8_t>(GbaCpuMode::Fiq),
		static_cast<uint8_t>(GbaCpuMode::Irq),
		static_cast<uint8_t>(GbaCpuMode::Supervisor),
		static_cast<uint8_t>(GbaCpuMode::Abort),
		static_cast<uint8_t>(GbaCpuMode::Undefined),
		static_cast<uint8_t>(GbaCpuMode::System),
	};
	for(int i = 0; i < 7; i++) {
		for(int j = i + 1; j < 7; j++) {
			EXPECT_NE(modes[i], modes[j]) << "Modes " << i << " and " << j << " are not distinct";
		}
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCpuVector — ARM exception vector addresses
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCpuVectorTest, Undefined)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Undefined), 0x04u);
}

TEST(GbaCpuVectorTest, SoftwareIrq)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::SoftwareIrq), 0x08u);
}

TEST(GbaCpuVectorTest, AbortPrefetch)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::AbortPrefetch), 0x0cu);
}

TEST(GbaCpuVectorTest, AbortData)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::AbortData), 0x10u);
}

TEST(GbaCpuVectorTest, Irq)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Irq), 0x18u);
}

TEST(GbaCpuVectorTest, Fiq)
{
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Fiq), 0x1cu);
}

TEST(GbaCpuVectorTest, VectorSpacing)
{
	// Vectors are word-aligned (4-byte spacing) in ARM BIOS area
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Undefined) % 4, 0u);
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::SoftwareIrq) % 4, 0u);
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::AbortPrefetch) % 4, 0u);
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::AbortData) % 4, 0u);
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Irq) % 4, 0u);
	EXPECT_EQ(static_cast<uint32_t>(GbaCpuVector::Fiq) % 4, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaAccessMode — Memory bus access flags
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaAccessModeTest, FlagValues)
{
	EXPECT_EQ(GbaAccessMode::Sequential, 0x01);
	EXPECT_EQ(GbaAccessMode::Word, 0x02);
	EXPECT_EQ(GbaAccessMode::HalfWord, 0x04);
	EXPECT_EQ(GbaAccessMode::Byte, 0x08);
	EXPECT_EQ(GbaAccessMode::Signed, 0x10);
	EXPECT_EQ(GbaAccessMode::NoRotate, 0x20);
	EXPECT_EQ(GbaAccessMode::Prefetch, 0x40);
	EXPECT_EQ(GbaAccessMode::Dma, 0x80);
}

TEST(GbaAccessModeTest, FlagsArePowersOfTwo)
{
	uint8_t flags[] = {
		GbaAccessMode::Sequential, GbaAccessMode::Word, GbaAccessMode::HalfWord,
		GbaAccessMode::Byte, GbaAccessMode::Signed, GbaAccessMode::NoRotate,
		GbaAccessMode::Prefetch, GbaAccessMode::Dma
	};
	for(int i = 0; i < 8; i++) {
		EXPECT_EQ(flags[i] & (flags[i] - 1), 0) << "Flag at index " << i << " is not a power of two";
	}
}

TEST(GbaAccessModeTest, FlagCombinations)
{
	GbaAccessModeVal seqWord = GbaAccessMode::Sequential | GbaAccessMode::Word;
	EXPECT_EQ(seqWord, 0x03);
	EXPECT_TRUE(seqWord & GbaAccessMode::Sequential);
	EXPECT_TRUE(seqWord & GbaAccessMode::Word);
	EXPECT_FALSE(seqWord & GbaAccessMode::HalfWord);
}

TEST(GbaAccessModeTest, DmaFlagComposition)
{
	GbaAccessModeVal dmaWord = GbaAccessMode::Dma | GbaAccessMode::Word;
	EXPECT_TRUE(dmaWord & GbaAccessMode::Dma);
	EXPECT_TRUE(dmaWord & GbaAccessMode::Word);
	EXPECT_FALSE(dmaWord & GbaAccessMode::Sequential);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaBgStereoMode
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaBgStereoModeTest, EnumValues)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaBgStereoMode::Disabled), 0);
	EXPECT_EQ(static_cast<uint8_t>(GbaBgStereoMode::EvenColumns), 1);
	EXPECT_EQ(static_cast<uint8_t>(GbaBgStereoMode::OddColumns), 2);
	EXPECT_EQ(static_cast<uint8_t>(GbaBgStereoMode::Both), 3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaPpuBlendEffect
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaPpuBlendEffectTest, EnumValues)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuBlendEffect::None), 0);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuBlendEffect::AlphaBlend), 1);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuBlendEffect::IncreaseBrightness), 2);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuBlendEffect::DecreaseBrightness), 3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaPpuObjMode
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaPpuObjModeTest, EnumValues)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuObjMode::Normal), 0);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuObjMode::Blending), 1);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuObjMode::Window), 2);
	EXPECT_EQ(static_cast<uint8_t>(GbaPpuObjMode::Stereoscopic), 3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaPpuMemAccess
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaPpuMemAccessTest, FlagValues)
{
	EXPECT_EQ(GbaPpuMemAccess::None, 0);
	EXPECT_EQ(GbaPpuMemAccess::Palette, 1);
	EXPECT_EQ(GbaPpuMemAccess::Vram, 2);
	EXPECT_EQ(GbaPpuMemAccess::Oam, 4);
	EXPECT_EQ(GbaPpuMemAccess::VramObj, 8);
}

TEST(GbaPpuMemAccessTest, FlagCombinations)
{
	uint8_t vramAndOam = GbaPpuMemAccess::Vram | GbaPpuMemAccess::Oam;
	EXPECT_EQ(vramAndOam, 6);
	EXPECT_TRUE(vramAndOam & GbaPpuMemAccess::Vram);
	EXPECT_TRUE(vramAndOam & GbaPpuMemAccess::Oam);
	EXPECT_FALSE(vramAndOam & GbaPpuMemAccess::Palette);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaDmaTrigger
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaDmaTriggerTest, EnumValues)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaTrigger::Immediate), 0);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaTrigger::VBlank), 1);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaTrigger::HBlank), 2);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaTrigger::Special), 3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaDmaAddrMode
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaDmaAddrModeTest, EnumValues)
{
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaAddrMode::Increment), 0);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaAddrMode::Decrement), 1);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaAddrMode::Fixed), 2);
	EXPECT_EQ(static_cast<uint8_t>(GbaDmaAddrMode::IncrementReload), 3);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaIrqSource — Interrupt flags
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaIrqSourceTest, None)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::None), 0);
}

TEST(GbaIrqSourceTest, LcdSources)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::LcdVblank), 1 << 0);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::LcdHblank), 1 << 1);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::LcdScanlineMatch), 1 << 2);
}

TEST(GbaIrqSourceTest, TimerSources)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Timer0), 1 << 3);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Timer1), 1 << 4);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Timer2), 1 << 5);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Timer3), 1 << 6);
}

TEST(GbaIrqSourceTest, SerialSource)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Serial), 1 << 7);
}

TEST(GbaIrqSourceTest, DmaSources)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::DmaChannel0), 1 << 8);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::DmaChannel1), 1 << 9);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::DmaChannel2), 1 << 10);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::DmaChannel3), 1 << 11);
}

TEST(GbaIrqSourceTest, KeypadAndExternal)
{
	EXPECT_EQ(static_cast<int>(GbaIrqSource::Keypad), 1 << 12);
	EXPECT_EQ(static_cast<int>(GbaIrqSource::External), 1 << 13);
}

TEST(GbaIrqSourceTest, AllSourcesFitIn14Bits)
{
	int allSources =
		static_cast<int>(GbaIrqSource::LcdVblank) |
		static_cast<int>(GbaIrqSource::LcdHblank) |
		static_cast<int>(GbaIrqSource::LcdScanlineMatch) |
		static_cast<int>(GbaIrqSource::Timer0) |
		static_cast<int>(GbaIrqSource::Timer1) |
		static_cast<int>(GbaIrqSource::Timer2) |
		static_cast<int>(GbaIrqSource::Timer3) |
		static_cast<int>(GbaIrqSource::Serial) |
		static_cast<int>(GbaIrqSource::DmaChannel0) |
		static_cast<int>(GbaIrqSource::DmaChannel1) |
		static_cast<int>(GbaIrqSource::DmaChannel2) |
		static_cast<int>(GbaIrqSource::DmaChannel3) |
		static_cast<int>(GbaIrqSource::Keypad) |
		static_cast<int>(GbaIrqSource::External);
	EXPECT_EQ(allSources, 0x3fff);
	EXPECT_TRUE(allSources <= 0xffff);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaThumbOpCategory — Thumb instruction classification
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaThumbOpCategoryTest, CategoryCount)
{
	// 20 categories including InvalidOp
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::InvalidOp), 19);
}

TEST(GbaThumbOpCategoryTest, SequentialValues)
{
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::MoveShiftedRegister), 0);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::AddSubtract), 1);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::MoveCmpAddSub), 2);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::AluOperation), 3);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::HiRegBranchExch), 4);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::PcRelLoad), 5);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LoadStoreRegOffset), 6);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LoadStoreSignExtended), 7);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LoadStoreImmOffset), 8);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LoadStoreHalfWord), 9);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::SpRelLoadStore), 10);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LoadAddress), 11);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::AddOffsetToSp), 12);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::PushPopReg), 13);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::MultipleLoadStore), 14);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::ConditionalBranch), 15);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::SoftwareInterrupt), 16);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::UnconditionalBranch), 17);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::LongBranchLink), 18);
	EXPECT_EQ(static_cast<int>(GbaThumbOpCategory::InvalidOp), 19);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaEepromMode (from GbaEeprom.h)
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaEepromModeTest, EnumValues)
{
	EXPECT_EQ(static_cast<int>(GbaEepromMode::Idle), 0);
	EXPECT_EQ(static_cast<int>(GbaEepromMode::Command), 1);
	EXPECT_EQ(static_cast<int>(GbaEepromMode::ReadCommand), 2);
	EXPECT_EQ(static_cast<int>(GbaEepromMode::ReadDataReady), 3);
	EXPECT_EQ(static_cast<int>(GbaEepromMode::WriteCommand), 4);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaSaveType (from SettingTypes.h)
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaSaveTypeTest, EnumValues)
{
	EXPECT_EQ(static_cast<int>(GbaSaveType::AutoDetect), 0);
	EXPECT_EQ(static_cast<int>(GbaSaveType::None), 1);
	EXPECT_EQ(static_cast<int>(GbaSaveType::Sram), 2);
	EXPECT_EQ(static_cast<int>(GbaSaveType::EepromUnknown), 3);
	EXPECT_EQ(static_cast<int>(GbaSaveType::Eeprom512), 4);
	EXPECT_EQ(static_cast<int>(GbaSaveType::Eeprom8192), 5);
	EXPECT_EQ(static_cast<int>(GbaSaveType::Flash64), 6);
	EXPECT_EQ(static_cast<int>(GbaSaveType::Flash128), 7);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaRtcType (from SettingTypes.h)
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaRtcTypeTest, EnumValues)
{
	EXPECT_EQ(static_cast<int>(GbaRtcType::AutoDetect), 0);
	EXPECT_EQ(static_cast<int>(GbaRtcType::Enabled), 1);
	EXPECT_EQ(static_cast<int>(GbaRtcType::Disabled), 2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCartridgeType (from SettingTypes.h)
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCartridgeTypeTest, EnumValues)
{
	EXPECT_EQ(static_cast<int>(GbaCartridgeType::Default), 0);
	EXPECT_EQ(static_cast<int>(GbaCartridgeType::TiltSensor), 1);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCpuFlags — CPSR/SPSR flags structure
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCpuFlagsTest, DefaultState)
{
	GbaCpuFlags flags = {};
	EXPECT_FALSE(flags.Thumb);
	EXPECT_FALSE(flags.FiqDisable);
	EXPECT_FALSE(flags.IrqDisable);
	EXPECT_FALSE(flags.Overflow);
	EXPECT_FALSE(flags.Carry);
	EXPECT_FALSE(flags.Zero);
	EXPECT_FALSE(flags.Negative);
}

TEST(GbaCpuFlagsTest, SetAllFlags)
{
	GbaCpuFlags flags = {};
	flags.Thumb = true;
	flags.FiqDisable = true;
	flags.IrqDisable = true;
	flags.Overflow = true;
	flags.Carry = true;
	flags.Zero = true;
	flags.Negative = true;
	EXPECT_TRUE(flags.Thumb);
	EXPECT_TRUE(flags.FiqDisable);
	EXPECT_TRUE(flags.IrqDisable);
	EXPECT_TRUE(flags.Overflow);
	EXPECT_TRUE(flags.Carry);
	EXPECT_TRUE(flags.Zero);
	EXPECT_TRUE(flags.Negative);
}

TEST(GbaCpuFlagsTest, ModeAssignment)
{
	GbaCpuFlags flags = {};
	flags.Mode = GbaCpuMode::Irq;
	EXPECT_EQ(flags.Mode, GbaCpuMode::Irq);
	flags.Mode = GbaCpuMode::User;
	EXPECT_EQ(flags.Mode, GbaCpuMode::User);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCpuState — ARM7TDMI CPU state
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCpuStateTest, DefaultRegisters)
{
	GbaCpuState cpu = {};
	for(int i = 0; i < 16; i++) {
		EXPECT_EQ(cpu.R[i], 0u) << "R" << i << " should be zero-initialized";
	}
}

TEST(GbaCpuStateTest, RegisterMaxValues)
{
	GbaCpuState cpu = {};
	for(int i = 0; i < 16; i++) {
		cpu.R[i] = 0xffffffff;
		EXPECT_EQ(cpu.R[i], 0xffffffffu);
	}
}

TEST(GbaCpuStateTest, CycleCountMaxValue)
{
	GbaCpuState cpu = {};
	cpu.CycleCount = UINT64_MAX;
	EXPECT_EQ(cpu.CycleCount, UINT64_MAX);
}

TEST(GbaCpuStateTest, DefaultStopAndFreezeState)
{
	GbaCpuState cpu = {};
	EXPECT_FALSE(cpu.Stopped);
	EXPECT_FALSE(cpu.Frozen);
}

TEST(GbaCpuStateTest, BankedRegistersIndependence)
{
	GbaCpuState cpu = {};
	// FIQ has 7 banked registers (R8-R14)
	for(int i = 0; i < 7; i++) {
		cpu.FiqRegs[i] = 0xf1000000 + i;
	}
	// IRQ has 2 banked registers (R13-R14)
	cpu.IrqRegs[0] = 0xdeadbeef;
	cpu.IrqRegs[1] = 0xcafebabe;
	// Supervisor has 2 banked registers
	cpu.SupervisorRegs[0] = 0x11111111;
	cpu.SupervisorRegs[1] = 0x22222222;
	// Abort has 2 banked registers
	cpu.AbortRegs[0] = 0x33333333;
	cpu.AbortRegs[1] = 0x44444444;
	// Undefined has 2 banked registers
	cpu.UndefinedRegs[0] = 0x55555555;
	cpu.UndefinedRegs[1] = 0x66666666;

	EXPECT_EQ(cpu.IrqRegs[0], 0xdeadbeefu);
	EXPECT_EQ(cpu.IrqRegs[1], 0xcafebabeu);
	EXPECT_EQ(cpu.SupervisorRegs[0], 0x11111111u);
	EXPECT_EQ(cpu.AbortRegs[0], 0x33333333u);
	EXPECT_EQ(cpu.UndefinedRegs[0], 0x55555555u);
}

TEST(GbaCpuStateTest, SpsrPerExceptionMode)
{
	GbaCpuState cpu = {};
	cpu.FiqSpsr.Mode = GbaCpuMode::User;
	cpu.FiqSpsr.Thumb = true;
	cpu.IrqSpsr.Mode = GbaCpuMode::System;
	cpu.IrqSpsr.Negative = true;
	cpu.SupervisorSpsr.Mode = GbaCpuMode::User;
	cpu.SupervisorSpsr.Carry = true;
	cpu.AbortSpsr.Mode = GbaCpuMode::Irq;
	cpu.UndefinedSpsr.Mode = GbaCpuMode::Supervisor;

	EXPECT_EQ(cpu.FiqSpsr.Mode, GbaCpuMode::User);
	EXPECT_TRUE(cpu.FiqSpsr.Thumb);
	EXPECT_EQ(cpu.IrqSpsr.Mode, GbaCpuMode::System);
	EXPECT_TRUE(cpu.IrqSpsr.Negative);
	EXPECT_TRUE(cpu.SupervisorSpsr.Carry);
	EXPECT_EQ(cpu.AbortSpsr.Mode, GbaCpuMode::Irq);
	EXPECT_EQ(cpu.UndefinedSpsr.Mode, GbaCpuMode::Supervisor);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaCpuPipeline — 3-stage ARM pipeline
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaCpuPipelineTest, DefaultState)
{
	GbaCpuPipeline pipe = {};
	EXPECT_EQ(pipe.Fetch.Address, 0u);
	EXPECT_EQ(pipe.Fetch.OpCode, 0u);
	EXPECT_EQ(pipe.Decode.Address, 0u);
	EXPECT_EQ(pipe.Decode.OpCode, 0u);
	EXPECT_EQ(pipe.Execute.Address, 0u);
	EXPECT_EQ(pipe.Execute.OpCode, 0u);
	EXPECT_FALSE(pipe.ReloadRequested);
}

TEST(GbaCpuPipelineTest, InstructionDataAssignment)
{
	GbaCpuPipeline pipe = {};
	pipe.Fetch.Address = 0x08000000;
	pipe.Fetch.OpCode = 0xe3a00042;
	pipe.Decode.Address = 0x08000004;
	pipe.Decode.OpCode = 0xe1a0f00e;
	EXPECT_EQ(pipe.Fetch.Address, 0x08000000u);
	EXPECT_EQ(pipe.Fetch.OpCode, 0xe3a00042u);
	EXPECT_EQ(pipe.Decode.Address, 0x08000004u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaPpuState — PPU (graphics) state
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaPpuStateTest, DefaultScanlineAndCycle)
{
	GbaPpuState ppu = {};
	EXPECT_EQ(ppu.FrameCount, 0u);
	EXPECT_EQ(ppu.Cycle, 0u);
	EXPECT_EQ(ppu.Scanline, 0u);
}

TEST(GbaPpuStateTest, BgModeRange)
{
	GbaPpuState ppu = {};
	// BG modes are 0-5 on GBA
	for(uint8_t mode = 0; mode <= 5; mode++) {
		ppu.BgMode = mode;
		EXPECT_EQ(ppu.BgMode, mode);
	}
}

TEST(GbaPpuStateTest, BgLayerCount)
{
	GbaPpuState ppu = {};
	// GBA has 4 BG layers (BG0-BG3)
	for(int i = 0; i < 4; i++) {
		ppu.BgLayers[i].Enabled = true;
		ppu.BgLayers[i].Priority = static_cast<uint8_t>(i);
	}
	for(int i = 0; i < 4; i++) {
		EXPECT_TRUE(ppu.BgLayers[i].Enabled);
		EXPECT_EQ(ppu.BgLayers[i].Priority, static_cast<uint8_t>(i));
	}
}

TEST(GbaPpuStateTest, BlendEffectAssignment)
{
	GbaPpuState ppu = {};
	ppu.BlendEffect = GbaPpuBlendEffect::AlphaBlend;
	EXPECT_EQ(ppu.BlendEffect, GbaPpuBlendEffect::AlphaBlend);
	ppu.BlendEffect = GbaPpuBlendEffect::IncreaseBrightness;
	EXPECT_EQ(ppu.BlendEffect, GbaPpuBlendEffect::IncreaseBrightness);
}

TEST(GbaPpuStateTest, BlendTargetLayers)
{
	GbaPpuState ppu = {};
	// 6 blend targets: BG0, BG1, BG2, BG3, OBJ, Backdrop
	for(int i = 0; i < 6; i++) {
		ppu.BlendMain[i] = (i % 2 == 0);
		ppu.BlendSub[i] = (i % 2 == 1);
	}
	for(int i = 0; i < 6; i++) {
		EXPECT_EQ(ppu.BlendMain[i], (i % 2 == 0));
		EXPECT_EQ(ppu.BlendSub[i], (i % 2 == 1));
	}
}

TEST(GbaPpuStateTest, BlendCoefficients)
{
	GbaPpuState ppu = {};
	ppu.BlendMainCoefficient = 16;
	ppu.BlendSubCoefficient = 0;
	ppu.Brightness = 16;
	EXPECT_EQ(ppu.BlendMainCoefficient, 16);
	EXPECT_EQ(ppu.BlendSubCoefficient, 0);
	EXPECT_EQ(ppu.Brightness, 16);
}

TEST(GbaPpuStateTest, WindowConfig)
{
	GbaPpuState ppu = {};
	ppu.Window[0].LeftX = 0;
	ppu.Window[0].RightX = 240;
	ppu.Window[0].TopY = 0;
	ppu.Window[0].BottomY = 160;
	ppu.Window[1].LeftX = 64;
	ppu.Window[1].RightX = 192;
	EXPECT_EQ(ppu.Window[0].RightX, 240);
	EXPECT_EQ(ppu.Window[1].LeftX, 64);
}

TEST(GbaPpuStateTest, MosaicSettings)
{
	GbaPpuState ppu = {};
	ppu.BgMosaicSizeX = 3;
	ppu.BgMosaicSizeY = 7;
	ppu.ObjMosaicSizeX = 1;
	ppu.ObjMosaicSizeY = 5;
	EXPECT_EQ(ppu.BgMosaicSizeX, 3);
	EXPECT_EQ(ppu.BgMosaicSizeY, 7);
	EXPECT_EQ(ppu.ObjMosaicSizeX, 1);
	EXPECT_EQ(ppu.ObjMosaicSizeY, 5);
}

TEST(GbaPpuStateTest, TransformMatrix)
{
	GbaPpuState ppu = {};
	// Two transform configs for BG2 and BG3
	ppu.Transform[0].Matrix[0] = 0x0100; // PA = 1.0 in fixed-point
	ppu.Transform[0].Matrix[1] = 0;      // PB = 0
	ppu.Transform[0].Matrix[2] = 0;      // PC = 0
	ppu.Transform[0].Matrix[3] = 0x0100; // PD = 1.0 (identity)
	EXPECT_EQ(ppu.Transform[0].Matrix[0], 0x0100);
	EXPECT_EQ(ppu.Transform[0].Matrix[3], 0x0100);
}

TEST(GbaPpuStateTest, ForcedBlankDefault)
{
	GbaPpuState ppu = {};
	EXPECT_FALSE(ppu.ForcedBlank);
	EXPECT_FALSE(ppu.DisplayFrameSelect);
	EXPECT_FALSE(ppu.ObjVramMappingOneDimension);
}

TEST(GbaPpuStateTest, WindowEnableFlags)
{
	GbaPpuState ppu = {};
	ppu.Window0Enabled = true;
	ppu.Window1Enabled = true;
	ppu.ObjWindowEnabled = true;
	EXPECT_TRUE(ppu.Window0Enabled);
	EXPECT_TRUE(ppu.Window1Enabled);
	EXPECT_TRUE(ppu.ObjWindowEnabled);
}

TEST(GbaPpuStateTest, BgLayerStereoMode)
{
	GbaPpuState ppu = {};
	ppu.BgLayers[0].StereoMode = GbaBgStereoMode::EvenColumns;
	ppu.BgLayers[1].StereoMode = GbaBgStereoMode::OddColumns;
	EXPECT_EQ(ppu.BgLayers[0].StereoMode, GbaBgStereoMode::EvenColumns);
	EXPECT_EQ(ppu.BgLayers[1].StereoMode, GbaBgStereoMode::OddColumns);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaTimerState — Hardware timer
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaTimerStateTest, DefaultState)
{
	GbaTimerState timer = {};
	EXPECT_EQ(timer.Timer, 0u);
	EXPECT_EQ(timer.ReloadValue, 0u);
	EXPECT_FALSE(timer.Enabled);
	EXPECT_FALSE(timer.IrqEnabled);
	EXPECT_FALSE(timer.Mode);
}

TEST(GbaTimerStateTest, FieldAssignment)
{
	GbaTimerState timer = {};
	timer.Timer = 0xffff;
	timer.ReloadValue = 0x8000;
	timer.PrescaleMask = 1023;
	timer.Enabled = true;
	timer.IrqEnabled = true;
	timer.Mode = true;
	EXPECT_EQ(timer.Timer, 0xffffu);
	EXPECT_EQ(timer.ReloadValue, 0x8000u);
	EXPECT_EQ(timer.PrescaleMask, 1023u);
	EXPECT_TRUE(timer.Enabled);
	EXPECT_TRUE(timer.IrqEnabled);
	EXPECT_TRUE(timer.Mode);
}

TEST(GbaTimerStateTest, FourTimersIndependent)
{
	GbaTimersState timers = {};
	for(int i = 0; i < 4; i++) {
		timers.Timer[i].Timer = static_cast<uint16_t>(i * 100);
		timers.Timer[i].Enabled = (i % 2 == 0);
	}
	for(int i = 0; i < 4; i++) {
		EXPECT_EQ(timers.Timer[i].Timer, static_cast<uint16_t>(i * 100));
		EXPECT_EQ(timers.Timer[i].Enabled, (i % 2 == 0));
	}
}

TEST(GbaTimerStateTest, PrescaleMaskValues)
{
	// Valid prescaler masks: 0, 63, 255, 1023 (for dividers 1, 64, 256, 1024)
	GbaTimerState timer = {};
	uint16_t masks[] = {0, 63, 255, 1023};
	for(auto mask : masks) {
		timer.PrescaleMask = mask;
		EXPECT_EQ(timer.PrescaleMask, mask);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaDmaChannel — DMA transfer channel
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaDmaChannelTest, DefaultState)
{
	GbaDmaChannel ch = {};
	EXPECT_EQ(ch.Source, 0u);
	EXPECT_EQ(ch.Destination, 0u);
	EXPECT_EQ(ch.Length, 0u);
	EXPECT_FALSE(ch.Enabled);
	EXPECT_FALSE(ch.Active);
	EXPECT_FALSE(ch.Pending);
}

TEST(GbaDmaChannelTest, AddressModes)
{
	GbaDmaChannel ch = {};
	ch.DestMode = GbaDmaAddrMode::IncrementReload;
	ch.SrcMode = GbaDmaAddrMode::Fixed;
	EXPECT_EQ(ch.DestMode, GbaDmaAddrMode::IncrementReload);
	EXPECT_EQ(ch.SrcMode, GbaDmaAddrMode::Fixed);
}

TEST(GbaDmaChannelTest, TriggerModes)
{
	GbaDmaChannel ch = {};
	ch.Trigger = GbaDmaTrigger::VBlank;
	EXPECT_EQ(ch.Trigger, GbaDmaTrigger::VBlank);
	ch.Trigger = GbaDmaTrigger::HBlank;
	EXPECT_EQ(ch.Trigger, GbaDmaTrigger::HBlank);
	ch.Trigger = GbaDmaTrigger::Special;
	EXPECT_EQ(ch.Trigger, GbaDmaTrigger::Special);
}

TEST(GbaDmaChannelTest, LatchedValues)
{
	GbaDmaChannel ch = {};
	ch.Source = 0x08000000;
	ch.Destination = 0x06000000;
	ch.Length = 0x4000;
	ch.SrcLatch = 0x08001000;
	ch.DestLatch = 0x06001000;
	ch.LenLatch = 0x3000;
	EXPECT_EQ(ch.SrcLatch, 0x08001000u);
	EXPECT_EQ(ch.DestLatch, 0x06001000u);
	EXPECT_EQ(ch.LenLatch, 0x3000u);
}

TEST(GbaDmaChannelTest, TransferWidth)
{
	GbaDmaChannel ch = {};
	ch.WordTransfer = false;
	EXPECT_FALSE(ch.WordTransfer); // 16-bit transfer
	ch.WordTransfer = true;
	EXPECT_TRUE(ch.WordTransfer);  // 32-bit transfer
}

TEST(GbaDmaControllerStateTest, FourChannelsIndependent)
{
	GbaDmaControllerState dma = {};
	for(int i = 0; i < 4; i++) {
		dma.Ch[i].Source = 0x08000000 + i * 0x10000;
		dma.Ch[i].Enabled = (i == 1 || i == 3);
	}
	for(int i = 0; i < 4; i++) {
		EXPECT_EQ(dma.Ch[i].Source, 0x08000000u + i * 0x10000);
		EXPECT_EQ(dma.Ch[i].Enabled, (i == 1 || i == 3));
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaMemoryManagerState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaMemoryManagerStateTest, DefaultInterruptState)
{
	GbaMemoryManagerState mem = {};
	EXPECT_EQ(mem.IE, 0u);
	EXPECT_EQ(mem.IF, 0u);
	EXPECT_EQ(mem.IME, 0u);
}

TEST(GbaMemoryManagerStateTest, WaitStatesDefaults)
{
	GbaMemoryManagerState mem = {};
	// Default wait states from GBA hardware
	EXPECT_EQ(mem.PrgWaitStates0[0], 5);  // Bank 0 non-sequential
	EXPECT_EQ(mem.PrgWaitStates0[1], 3);  // Bank 0 sequential
	EXPECT_EQ(mem.PrgWaitStates1[0], 5);  // Bank 1 non-sequential
	EXPECT_EQ(mem.PrgWaitStates1[1], 5);  // Bank 1 sequential
	EXPECT_EQ(mem.PrgWaitStates2[0], 5);  // Bank 2 non-sequential
	EXPECT_EQ(mem.PrgWaitStates2[1], 9);  // Bank 2 sequential
	EXPECT_EQ(mem.SramWaitStates, 5);     // SRAM wait states
}

TEST(GbaMemoryManagerStateTest, HaltAndStopFlags)
{
	GbaMemoryManagerState mem = {};
	EXPECT_FALSE(mem.BusLocked);
	EXPECT_FALSE(mem.StopMode);
	EXPECT_FALSE(mem.PostBootFlag);
	EXPECT_FALSE(mem.PrefetchEnabled);
}

TEST(GbaMemoryManagerStateTest, OpenBusArrays)
{
	GbaMemoryManagerState mem = {};
	for(int i = 0; i < 4; i++) {
		mem.BootRomOpenBus[i] = static_cast<uint8_t>(0xa0 + i);
		mem.InternalOpenBus[i] = static_cast<uint8_t>(0xb0 + i);
		mem.IwramOpenBus[i] = static_cast<uint8_t>(0xc0 + i);
	}
	EXPECT_EQ(mem.BootRomOpenBus[0], 0xa0);
	EXPECT_EQ(mem.InternalOpenBus[3], 0xb3);
	EXPECT_EQ(mem.IwramOpenBus[2], 0xc2);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaRomPrefetchState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaRomPrefetchStateTest, DefaultState)
{
	GbaRomPrefetchState pf = {};
	EXPECT_EQ(pf.ReadAddr, 0u);
	EXPECT_EQ(pf.PrefetchAddr, 0u);
	EXPECT_FALSE(pf.Started);
	EXPECT_FALSE(pf.Sequential);
	EXPECT_FALSE(pf.WasFilled);
	EXPECT_FALSE(pf.HitBoundary);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaApuState — Audio processing unit
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaApuStateTest, DefaultState)
{
	GbaApuState apu = {};
	EXPECT_FALSE(apu.ApuEnabled);
	EXPECT_EQ(apu.FrameSequenceStep, 0u);
	EXPECT_EQ(apu.Bias, 0u);
	EXPECT_EQ(apu.SamplingRate, 0u);
}

TEST(GbaApuStateTest, DmaSoundConfig)
{
	GbaApuState apu = {};
	apu.EnableLeftA = true;
	apu.EnableRightA = true;
	apu.EnableLeftB = false;
	apu.EnableRightB = true;
	apu.TimerA = 0;
	apu.TimerB = 1;
	apu.VolumeA = 1;
	apu.VolumeB = 0;
	EXPECT_TRUE(apu.EnableLeftA);
	EXPECT_TRUE(apu.EnableRightA);
	EXPECT_FALSE(apu.EnableLeftB);
	EXPECT_TRUE(apu.EnableRightB);
	EXPECT_EQ(apu.TimerA, 0);
	EXPECT_EQ(apu.TimerB, 1);
	EXPECT_EQ(apu.VolumeA, 1);  // 100%
	EXPECT_EQ(apu.VolumeB, 0);  // 50%
}

TEST(GbaApuStateTest, GbChannelPanning)
{
	GbaApuState apu = {};
	apu.EnableLeftSq1 = 1;
	apu.EnableRightSq1 = 0;
	apu.EnableLeftSq2 = 0;
	apu.EnableRightSq2 = 1;
	apu.EnableLeftWave = 1;
	apu.EnableRightWave = 1;
	apu.EnableLeftNoise = 0;
	apu.EnableRightNoise = 0;
	EXPECT_EQ(apu.EnableLeftSq1, 1);
	EXPECT_EQ(apu.EnableRightSq1, 0);
	EXPECT_EQ(apu.EnableLeftWave, 1);
	EXPECT_EQ(apu.EnableRightWave, 1);
}

TEST(GbaApuStateTest, MasterVolume)
{
	GbaApuState apu = {};
	apu.LeftVolume = 7;
	apu.RightVolume = 7;
	EXPECT_EQ(apu.LeftVolume, 7);
	EXPECT_EQ(apu.RightVolume, 7);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaSquareState — Square wave channel
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaSquareStateTest, DefaultState)
{
	GbaSquareState sq = {};
	EXPECT_FALSE(sq.Enabled);
	EXPECT_EQ(sq.Frequency, 0u);
	EXPECT_EQ(sq.Volume, 0u);
	EXPECT_EQ(sq.Duty, 0u);
	EXPECT_EQ(sq.Output, 0u);
}

TEST(GbaSquareStateTest, EnvelopeConfig)
{
	GbaSquareState sq = {};
	sq.EnvVolume = 15;
	sq.EnvRaiseVolume = true;
	sq.EnvPeriod = 7;
	sq.Volume = 15;
	EXPECT_EQ(sq.EnvVolume, 15);
	EXPECT_TRUE(sq.EnvRaiseVolume);
	EXPECT_EQ(sq.EnvPeriod, 7);
	EXPECT_EQ(sq.Volume, 15);
}

TEST(GbaSquareStateTest, SweepConfig)
{
	GbaSquareState sq = {};
	sq.SweepEnabled = true;
	sq.SweepPeriod = 7;
	sq.SweepNegate = true;
	sq.SweepShift = 7;
	EXPECT_TRUE(sq.SweepEnabled);
	EXPECT_EQ(sq.SweepPeriod, 7u);
	EXPECT_TRUE(sq.SweepNegate);
	EXPECT_EQ(sq.SweepShift, 7);
}

TEST(GbaSquareStateTest, DutyCycleRange)
{
	GbaSquareState sq = {};
	// Duty cycles: 0=12.5%, 1=25%, 2=50%, 3=75%
	for(uint8_t d = 0; d < 4; d++) {
		sq.Duty = d;
		EXPECT_EQ(sq.Duty, d);
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaNoiseState — Noise channel (LFSR)
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaNoiseStateTest, DefaultState)
{
	GbaNoiseState noise = {};
	EXPECT_FALSE(noise.Enabled);
	EXPECT_EQ(noise.ShiftRegister, 0u);
	EXPECT_EQ(noise.Output, 0u);
}

TEST(GbaNoiseStateTest, LfsrModes)
{
	GbaNoiseState noise = {};
	noise.ShortWidthMode = false; // 15-bit LFSR
	EXPECT_FALSE(noise.ShortWidthMode);
	noise.ShortWidthMode = true;  // 7-bit LFSR
	EXPECT_TRUE(noise.ShortWidthMode);
}

TEST(GbaNoiseStateTest, ShiftRegisterMaxValue)
{
	GbaNoiseState noise = {};
	noise.ShiftRegister = 0x7fff; // 15-bit max
	EXPECT_EQ(noise.ShiftRegister, 0x7fffu);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaWaveState — Wave channel
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaWaveStateTest, DefaultState)
{
	GbaWaveState wave = {};
	EXPECT_FALSE(wave.DacEnabled);
	EXPECT_FALSE(wave.Enabled);
	EXPECT_EQ(wave.Volume, 0u);
	EXPECT_EQ(wave.Frequency, 0u);
}

TEST(GbaWaveStateTest, GbaSpecificFeatures)
{
	GbaWaveState wave = {};
	wave.DoubleLength = true;   // GBA-specific: two 32-byte banks
	wave.SelectedBank = 1;
	wave.OverrideVolume = true; // GBA-specific: force 75% volume
	EXPECT_TRUE(wave.DoubleLength);
	EXPECT_EQ(wave.SelectedBank, 1);
	EXPECT_TRUE(wave.OverrideVolume);
}

TEST(GbaWaveStateTest, WaveRamSize)
{
	GbaWaveState wave = {};
	// Wave RAM: 32 bytes storing 64 4-bit samples
	for(int i = 0; i < 0x20; i++) {
		wave.Ram[i] = static_cast<uint8_t>(i);
	}
	EXPECT_EQ(wave.Ram[0], 0);
	EXPECT_EQ(wave.Ram[0x1f], 0x1f);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaSerialState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaSerialStateTest, DefaultState)
{
	GbaSerialState serial = {};
	EXPECT_FALSE(serial.Active);
	EXPECT_FALSE(serial.IrqEnabled);
	EXPECT_FALSE(serial.TransferWord);
	EXPECT_EQ(serial.Control, 0u);
}

TEST(GbaSerialStateTest, DataRegisters)
{
	GbaSerialState serial = {};
	for(int i = 0; i < 4; i++) {
		serial.Data[i] = static_cast<uint16_t>(0x1000 + i);
	}
	EXPECT_EQ(serial.Data[0], 0x1000u);
	EXPECT_EQ(serial.Data[3], 0x1003u);
}

TEST(GbaSerialStateTest, JoybusRegisters)
{
	GbaSerialState serial = {};
	serial.JoyReceive = 0xdeadbeef;
	serial.JoySend = 0xcafebabe;
	serial.JoyStatus = 0xff;
	EXPECT_EQ(serial.JoyReceive, 0xdeadbeefu);
	EXPECT_EQ(serial.JoySend, 0xcafebabeu);
	EXPECT_EQ(serial.JoyStatus, 0xff);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaControlManagerState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaControlManagerStateTest, DefaultState)
{
	GbaControlManagerState ctrl = {};
	EXPECT_EQ(ctrl.KeyControl, 0u);
	EXPECT_EQ(ctrl.ActiveKeys, 0u);
}

TEST(GbaControlManagerStateTest, KeyBits)
{
	GbaControlManagerState ctrl = {};
	// GBA has 10 buttons: A, B, Select, Start, Right, Left, Up, Down, R, L
	ctrl.ActiveKeys = 0x03ff;
	EXPECT_EQ(ctrl.ActiveKeys, 0x03ffu);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaGpioState / GbaCartState
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaGpioStateTest, DefaultState)
{
	GbaGpioState gpio = {};
	EXPECT_EQ(gpio.Data, 0u);
	EXPECT_EQ(gpio.WritablePins, 0u);
	EXPECT_FALSE(gpio.ReadWrite);
}

TEST(GbaCartStateTest, DefaultState)
{
	GbaCartState cart = {};
	EXPECT_FALSE(cart.HasGpio);
	EXPECT_EQ(cart.Gpio.Data, 0u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaState — Complete system aggregate
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaStateTest, AllSubsystemsAccessible)
{
	GbaState state = {};
	state.Cpu.CycleCount = 1000;
	state.Cpu.R[0] = 0x42;
	state.Ppu.FrameCount = 60;
	state.Ppu.Scanline = 100;
	state.Apu.Common.ApuEnabled = true;
	state.Timer.Timer[0].Timer = 0x8000;
	state.Dma.Ch[0].Source = 0x08000000;
	state.MemoryManager.IE = 0x3fff;
	state.Cart.HasGpio = true;

	EXPECT_EQ(state.Cpu.CycleCount, 1000u);
	EXPECT_EQ(state.Cpu.R[0], 0x42u);
	EXPECT_EQ(state.Ppu.FrameCount, 60u);
	EXPECT_EQ(state.Ppu.Scanline, 100u);
	EXPECT_TRUE(state.Apu.Common.ApuEnabled);
	EXPECT_EQ(state.Timer.Timer[0].Timer, 0x8000u);
	EXPECT_EQ(state.Dma.Ch[0].Source, 0x08000000u);
	EXPECT_EQ(state.MemoryManager.IE, 0x3fffu);
	EXPECT_TRUE(state.Cart.HasGpio);
}

TEST(GbaStateTest, CpuPipelineAccessible)
{
	GbaState state = {};
	state.Cpu.Pipeline.Fetch.Address = 0x08000000;
	state.Cpu.Pipeline.Decode.OpCode = 0xe3a00000;
	state.Cpu.Pipeline.ReloadRequested = true;
	EXPECT_EQ(state.Cpu.Pipeline.Fetch.Address, 0x08000000u);
	EXPECT_EQ(state.Cpu.Pipeline.Decode.OpCode, 0xe3a00000u);
	EXPECT_TRUE(state.Cpu.Pipeline.ReloadRequested);
}

TEST(GbaStateTest, ApuDebugStateAllChannels)
{
	GbaState state = {};
	state.Apu.Square1.Enabled = true;
	state.Apu.Square1.Frequency = 1024;
	state.Apu.Square2.Enabled = false;
	state.Apu.Wave.DacEnabled = true;
	state.Apu.Wave.DoubleLength = true;
	state.Apu.Noise.Enabled = true;
	state.Apu.Noise.ShortWidthMode = true;

	EXPECT_TRUE(state.Apu.Square1.Enabled);
	EXPECT_EQ(state.Apu.Square1.Frequency, 1024u);
	EXPECT_FALSE(state.Apu.Square2.Enabled);
	EXPECT_TRUE(state.Apu.Wave.DacEnabled);
	EXPECT_TRUE(state.Apu.Wave.DoubleLength);
	EXPECT_TRUE(state.Apu.Noise.Enabled);
	EXPECT_TRUE(state.Apu.Noise.ShortWidthMode);
}

TEST(GbaStateTest, PrefetchStateAccessible)
{
	GbaState state = {};
	state.Prefetch.ReadAddr = 0x08000100;
	state.Prefetch.PrefetchAddr = 0x08000104;
	state.Prefetch.Started = true;
	state.Prefetch.Sequential = true;
	EXPECT_EQ(state.Prefetch.ReadAddr, 0x08000100u);
	EXPECT_EQ(state.Prefetch.PrefetchAddr, 0x08000104u);
	EXPECT_TRUE(state.Prefetch.Started);
	EXPECT_TRUE(state.Prefetch.Sequential);
}

TEST(GbaStateTest, ControlManagerAccessible)
{
	GbaState state = {};
	state.ControlManager.ActiveKeys = 0x0001; // A button
	state.ControlManager.KeyControl = 0x4001; // IRQ on A button
	EXPECT_EQ(state.ControlManager.ActiveKeys, 0x0001u);
	EXPECT_EQ(state.ControlManager.KeyControl, 0x4001u);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaBgConfig — Background layer configuration
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaBgConfigTest, DefaultState)
{
	GbaBgConfig bg = {};
	EXPECT_FALSE(bg.Enabled);
	EXPECT_FALSE(bg.Mosaic);
	EXPECT_FALSE(bg.WrapAround);
	EXPECT_FALSE(bg.Bpp8Mode);
	EXPECT_EQ(bg.Priority, 0u);
	EXPECT_EQ(bg.ScreenSize, 0u);
}

TEST(GbaBgConfigTest, ScreenSizeVariants)
{
	GbaBgConfig bg = {};
	// Screen size: 0=256x256, 1=512x256, 2=256x512, 3=512x512
	for(uint8_t s = 0; s < 4; s++) {
		bg.ScreenSize = s;
		EXPECT_EQ(bg.ScreenSize, s);
	}
}

TEST(GbaBgConfigTest, ScrollRange)
{
	GbaBgConfig bg = {};
	bg.ScrollX = 511;
	bg.ScrollY = 511;
	EXPECT_EQ(bg.ScrollX, 511u);
	EXPECT_EQ(bg.ScrollY, 511u);
}

TEST(GbaBgConfigTest, PriorityRange)
{
	GbaBgConfig bg = {};
	for(uint8_t p = 0; p < 4; p++) {
		bg.Priority = p;
		EXPECT_EQ(bg.Priority, p);
	}
}

TEST(GbaBgConfigTest, EnableDisableTimers)
{
	GbaBgConfig bg = {};
	bg.EnableTimer = 3;
	bg.DisableTimer = 5;
	EXPECT_EQ(bg.EnableTimer, 3);
	EXPECT_EQ(bg.DisableTimer, 5);
}

// ═══════════════════════════════════════════════════════════════════════════════
// GbaTransformConfig — Affine BG transform
// ═══════════════════════════════════════════════════════════════════════════════

TEST(GbaTransformConfigTest, DefaultState)
{
	GbaTransformConfig xform = {};
	EXPECT_EQ(xform.OriginX, 0u);
	EXPECT_EQ(xform.OriginY, 0u);
	for(int i = 0; i < 4; i++) {
		EXPECT_EQ(xform.Matrix[i], 0);
	}
	EXPECT_FALSE(xform.PendingUpdateX);
	EXPECT_FALSE(xform.PendingUpdateY);
}

TEST(GbaTransformConfigTest, IdentityMatrix)
{
	GbaTransformConfig xform = {};
	// Identity in 8.8 fixed-point: PA=PD=256, PB=PC=0
	xform.Matrix[0] = 256;  // PA
	xform.Matrix[1] = 0;    // PB
	xform.Matrix[2] = 0;    // PC
	xform.Matrix[3] = 256;  // PD
	EXPECT_EQ(xform.Matrix[0], 256);
	EXPECT_EQ(xform.Matrix[3], 256);
}
