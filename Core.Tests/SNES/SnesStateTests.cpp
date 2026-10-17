#include "pch.h"
#include "SNES/SnesState.h"
#include "SNES/CartTypes.h"

// ============================================================================
// SnesCpuStopState enum class
// ============================================================================

TEST(SnesCpuStopStateTest, Running) {
	EXPECT_EQ(static_cast<uint8_t>(SnesCpuStopState::Running), 0);
}

TEST(SnesCpuStopStateTest, Stopped) {
	EXPECT_EQ(static_cast<uint8_t>(SnesCpuStopState::Stopped), 1);
}

TEST(SnesCpuStopStateTest, WaitingForIrq) {
	EXPECT_EQ(static_cast<uint8_t>(SnesCpuStopState::WaitingForIrq), 2);
}

// ============================================================================
// ProcFlags namespace enum (65C816 processor status)
// ============================================================================

TEST(ProcFlagsTest, IndividualBits) {
	EXPECT_EQ(ProcFlags::Carry, 0x01);
	EXPECT_EQ(ProcFlags::Zero, 0x02);
	EXPECT_EQ(ProcFlags::IrqDisable, 0x04);
	EXPECT_EQ(ProcFlags::Decimal, 0x08);
	EXPECT_EQ(ProcFlags::IndexMode8, 0x10);
	EXPECT_EQ(ProcFlags::MemoryMode8, 0x20);
	EXPECT_EQ(ProcFlags::Overflow, 0x40);
	EXPECT_EQ(ProcFlags::Negative, 0x80);
}

TEST(ProcFlagsTest, AllFlagsCombined) {
	uint8_t all = ProcFlags::Carry | ProcFlags::Zero | ProcFlags::IrqDisable |
	              ProcFlags::Decimal | ProcFlags::IndexMode8 | ProcFlags::MemoryMode8 |
	              ProcFlags::Overflow | ProcFlags::Negative;
	EXPECT_EQ(all, 0xff);
}

TEST(ProcFlagsTest, EmulationModeFlags) {
	uint8_t emul = ProcFlags::IndexMode8 | ProcFlags::MemoryMode8;
	EXPECT_EQ(emul, 0x30);
}

// ============================================================================
// SnesIrqSource enum class
// ============================================================================

TEST(SnesIrqSourceTest, Values) {
	EXPECT_EQ(static_cast<int>(SnesIrqSource::None), 0);
	EXPECT_EQ(static_cast<int>(SnesIrqSource::Ppu), 1);
	EXPECT_EQ(static_cast<int>(SnesIrqSource::Coprocessor), 2);
}

// ============================================================================
// SnesCpuState struct
// ============================================================================

TEST(SnesCpuStateTest, RegisterLayout) {
	SnesCpuState state = {};
	state.A = 0xabcd;
	state.X = 0x1234;
	state.Y = 0x5678;
	state.SP = 0x01ff;
	state.D = 0x2100;
	state.PC = 0x8000;
	state.K = 0x80;
	state.DBR = 0x7e;
	EXPECT_EQ(state.A, 0xabcd);
	EXPECT_EQ(state.X, 0x1234);
	EXPECT_EQ(state.Y, 0x5678);
	EXPECT_EQ(state.SP, 0x01ff);
	EXPECT_EQ(state.D, 0x2100);
	EXPECT_EQ(state.PC, 0x8000);
	EXPECT_EQ(state.K, 0x80);
	EXPECT_EQ(state.DBR, 0x7e);
}

TEST(SnesCpuStateTest, ProcessorStatusFlags) {
	SnesCpuState state = {};
	state.PS = ProcFlags::Carry | ProcFlags::Negative;
	EXPECT_EQ(state.PS, 0x81);
	EXPECT_TRUE(state.PS & ProcFlags::Carry);
	EXPECT_TRUE(state.PS & ProcFlags::Negative);
	EXPECT_FALSE(state.PS & ProcFlags::Zero);
}

TEST(SnesCpuStateTest, EmulationMode) {
	SnesCpuState state = {};
	state.EmulationMode = true;
	EXPECT_TRUE(state.EmulationMode);
	state.EmulationMode = false;
	EXPECT_FALSE(state.EmulationMode);
}

TEST(SnesCpuStateTest, StopState) {
	SnesCpuState state = {};
	state.StopState = SnesCpuStopState::WaitingForIrq;
	EXPECT_EQ(state.StopState, SnesCpuStopState::WaitingForIrq);
}

TEST(SnesCpuStateTest, InternalState) {
	SnesCpuState state = {};
	state.IrqSource = 0x03;
	state.NeedNmi = true;
	state.IrqLock = true;
	EXPECT_EQ(state.IrqSource, 0x03);
	EXPECT_TRUE(state.NeedNmi);
	EXPECT_TRUE(state.IrqLock);
}

// ============================================================================
// CoprocessorType enum class
// ============================================================================

TEST(CoprocessorTypeTest, Values) {
	EXPECT_EQ(static_cast<int>(CoprocessorType::None), 0);
	EXPECT_EQ(static_cast<int>(CoprocessorType::DSP1), 1);
	EXPECT_EQ(static_cast<int>(CoprocessorType::DSP1B), 2);
	EXPECT_EQ(static_cast<int>(CoprocessorType::DSP2), 3);
	EXPECT_EQ(static_cast<int>(CoprocessorType::DSP3), 4);
	EXPECT_EQ(static_cast<int>(CoprocessorType::DSP4), 5);
	EXPECT_EQ(static_cast<int>(CoprocessorType::GSU), 6);
	EXPECT_EQ(static_cast<int>(CoprocessorType::OBC1), 7);
	EXPECT_EQ(static_cast<int>(CoprocessorType::SA1), 8);
	EXPECT_EQ(static_cast<int>(CoprocessorType::SDD1), 9);
	EXPECT_EQ(static_cast<int>(CoprocessorType::RTC), 10);
	EXPECT_EQ(static_cast<int>(CoprocessorType::Satellaview), 11);
	EXPECT_EQ(static_cast<int>(CoprocessorType::SPC7110), 12);
	EXPECT_EQ(static_cast<int>(CoprocessorType::ST010), 13);
	EXPECT_EQ(static_cast<int>(CoprocessorType::ST011), 14);
	EXPECT_EQ(static_cast<int>(CoprocessorType::ST018), 15);
	EXPECT_EQ(static_cast<int>(CoprocessorType::CX4), 16);
	EXPECT_EQ(static_cast<int>(CoprocessorType::SGB), 17);
}

// ============================================================================
// CartFlags namespace enum
// ============================================================================

TEST(CartFlagsTest, IndividualBits) {
	EXPECT_EQ(CartFlags::None, 0);
	EXPECT_EQ(CartFlags::LoRom, 1);
	EXPECT_EQ(CartFlags::HiRom, 2);
	EXPECT_EQ(CartFlags::FastRom, 4);
	EXPECT_EQ(CartFlags::ExLoRom, 8);
	EXPECT_EQ(CartFlags::ExHiRom, 16);
	EXPECT_EQ(CartFlags::CopierHeader, 32);
}

TEST(CartFlagsTest, Combinations) {
	int hifast = CartFlags::HiRom | CartFlags::FastRom;
	EXPECT_EQ(hifast, 6);
	int exhi_fast = CartFlags::ExHiRom | CartFlags::FastRom;
	EXPECT_EQ(exhi_fast, 20);
}

// ============================================================================
// WindowMaskLogic enum class
// ============================================================================

TEST(WindowMaskLogicTest, Values) {
	EXPECT_EQ(static_cast<int>(WindowMaskLogic::Or), 0);
	EXPECT_EQ(static_cast<int>(WindowMaskLogic::And), 1);
	EXPECT_EQ(static_cast<int>(WindowMaskLogic::Xor), 2);
	EXPECT_EQ(static_cast<int>(WindowMaskLogic::Xnor), 3);
}

// ============================================================================
// ColorWindowMode enum class
// ============================================================================

TEST(ColorWindowModeTest, Values) {
	EXPECT_EQ(static_cast<int>(ColorWindowMode::Never), 0);
	EXPECT_EQ(static_cast<int>(ColorWindowMode::OutsideWindow), 1);
	EXPECT_EQ(static_cast<int>(ColorWindowMode::InsideWindow), 2);
	EXPECT_EQ(static_cast<int>(ColorWindowMode::Always), 3);
}

// ============================================================================
// PixelFlags enum
// ============================================================================

TEST(PixelFlagsTest, AllowColorMath) {
	EXPECT_EQ(PixelFlags::AllowColorMath, 0x80);
}

// ============================================================================
// SnesPpuState struct
// ============================================================================

TEST(SnesPpuStateTest, DefaultValues) {
	SnesPpuState state = {};
	EXPECT_EQ(state.BgMode, 0);
	EXPECT_FALSE(state.ForcedBlank);
	EXPECT_EQ(state.MainScreenLayers, 0);
	EXPECT_EQ(state.SubScreenLayers, 0);
	EXPECT_EQ(state.ScreenBrightness, 0);
	EXPECT_EQ(state.ColorMathEnabled, 0);
	EXPECT_EQ(state.MosaicSize, 0);
	EXPECT_EQ(state.MosaicEnabled, 0);
	EXPECT_FALSE(state.HiResMode);
	EXPECT_FALSE(state.ScreenInterlace);
}

TEST(SnesPpuStateTest, BgModeRange) {
	SnesPpuState state = {};
	for (uint8_t mode = 0; mode <= 7; mode++) {
		state.BgMode = mode;
		EXPECT_EQ(state.BgMode, mode);
	}
}

TEST(SnesPpuStateTest, LayerConfig) {
	LayerConfig layer = {};
	EXPECT_EQ(layer.TilemapAddress, 0);
	EXPECT_EQ(layer.ChrAddress, 0);
	EXPECT_EQ(layer.HScroll, 0);
	EXPECT_EQ(layer.VScroll, 0);
	EXPECT_FALSE(layer.DoubleWidth);
	EXPECT_FALSE(layer.DoubleHeight);
	EXPECT_FALSE(layer.LargeTiles);
}

TEST(SnesPpuStateTest, Mode7Config) {
	Mode7Config m7 = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(m7.Matrix[i], 0);
	}
	EXPECT_EQ(m7.HScroll, 0);
	EXPECT_EQ(m7.VScroll, 0);
	EXPECT_EQ(m7.CenterX, 0);
	EXPECT_EQ(m7.CenterY, 0);
	EXPECT_EQ(m7.ValueLatch, 0);
	EXPECT_FALSE(m7.LargeMap);
	EXPECT_FALSE(m7.FillWithTile0);
	EXPECT_FALSE(m7.HorizontalMirroring);
	EXPECT_FALSE(m7.VerticalMirroring);
}

TEST(SnesPpuStateTest, WindowConfig) {
	WindowConfig win = {};
	for (int i = 0; i < 6; i++) {
		EXPECT_FALSE(win.ActiveLayers[i]);
		EXPECT_FALSE(win.InvertedLayers[i]);
	}
}

TEST(SnesPpuStateTest, ColorMathFields) {
	SnesPpuState state = {};
	EXPECT_FALSE(state.ColorMathAddSubscreen);
	EXPECT_FALSE(state.ColorMathSubtractMode);
	EXPECT_FALSE(state.ColorMathHalveResult);
	EXPECT_EQ(state.FixedColor, 0);
}

// ============================================================================
// SpriteInfo struct
// ============================================================================

TEST(SnesSpriteInfoTest, Fields) {
	SpriteInfo spr = {};
	spr.X = -128;
	spr.Y = 100;
	spr.Index = 64;
	spr.Width = 32;
	spr.Height = 32;
	spr.HorizontalMirror = true;
	spr.Priority = 3;
	spr.Palette = 7;
	EXPECT_EQ(spr.X, -128);
	EXPECT_EQ(spr.Y, 100);
	EXPECT_EQ(spr.Index, 64);
	EXPECT_EQ(spr.Width, 32);
	EXPECT_EQ(spr.Height, 32);
	EXPECT_TRUE(spr.HorizontalMirror);
	EXPECT_EQ(spr.Priority, 3);
	EXPECT_EQ(spr.Palette, 7);
}

// ============================================================================
// SpcFlags namespace enum
// ============================================================================

TEST(SpcFlagsTest, IndividualBits) {
	EXPECT_EQ(SpcFlags::Carry, 0x01);
	EXPECT_EQ(SpcFlags::Zero, 0x02);
	EXPECT_EQ(SpcFlags::IrqEnable, 0x04);
	EXPECT_EQ(SpcFlags::HalfCarry, 0x08);
	EXPECT_EQ(SpcFlags::Break, 0x10);
	EXPECT_EQ(SpcFlags::DirectPage, 0x20);
	EXPECT_EQ(SpcFlags::Overflow, 0x40);
	EXPECT_EQ(SpcFlags::Negative, 0x80);
}

TEST(SpcFlagsTest, AllFlagsCombined) {
	uint8_t all = SpcFlags::Carry | SpcFlags::Zero | SpcFlags::IrqEnable |
	              SpcFlags::HalfCarry | SpcFlags::Break | SpcFlags::DirectPage |
	              SpcFlags::Overflow | SpcFlags::Negative;
	EXPECT_EQ(all, 0xff);
}

// ============================================================================
// SpcControlFlags namespace enum
// ============================================================================

TEST(SpcControlFlagsTest, IndividualBits) {
	EXPECT_EQ(SpcControlFlags::Timer0, 0x01);
	EXPECT_EQ(SpcControlFlags::Timer1, 0x02);
	EXPECT_EQ(SpcControlFlags::Timer2, 0x04);
	EXPECT_EQ(SpcControlFlags::ClearPortsA, 0x10);
	EXPECT_EQ(SpcControlFlags::ClearPortsB, 0x20);
	EXPECT_EQ(SpcControlFlags::EnableRom, 0x80);
}

TEST(SpcControlFlagsTest, AllTimers) {
	uint8_t timers = SpcControlFlags::Timer0 | SpcControlFlags::Timer1 | SpcControlFlags::Timer2;
	EXPECT_EQ(timers, 0x07);
}

// ============================================================================
// SpcTestFlags namespace enum
// ============================================================================

TEST(SpcTestFlagsTest, IndividualBits) {
	EXPECT_EQ(SpcTestFlags::TimersDisabled, 0x01);
	EXPECT_EQ(SpcTestFlags::WriteEnabled, 0x02);
	EXPECT_EQ(SpcTestFlags::Unknown, 0x04);
	EXPECT_EQ(SpcTestFlags::TimersEnabled, 0x08);
	EXPECT_EQ(SpcTestFlags::ExternalSpeed, 0x30);
	EXPECT_EQ(SpcTestFlags::InternalSpeed, 0xc0);
}

// ============================================================================
// SpcOpStep enum class
// ============================================================================

TEST(SpcOpStepTest, Values) {
	EXPECT_EQ(static_cast<uint8_t>(SpcOpStep::ReadOpCode), 0);
	EXPECT_EQ(static_cast<uint8_t>(SpcOpStep::Addressing), 1);
	EXPECT_EQ(static_cast<uint8_t>(SpcOpStep::AfterAddressing), 2);
	EXPECT_EQ(static_cast<uint8_t>(SpcOpStep::Operation), 3);
}

// ============================================================================
// SpcState struct
// ============================================================================

TEST(SpcStateTest, DefaultValues) {
	SpcState state = {};
	EXPECT_EQ(state.Cycle, 0u);
	EXPECT_EQ(state.PC, 0);
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.X, 0);
	EXPECT_EQ(state.Y, 0);
	EXPECT_EQ(state.SP, 0);
	EXPECT_EQ(state.PS, 0);
	EXPECT_FALSE(state.WriteEnabled);
	EXPECT_FALSE(state.RomEnabled);
	EXPECT_EQ(state.DspReg, 0);
}

TEST(SpcStateTest, RegisterAssignment) {
	SpcState state = {};
	state.PC = 0xffc0;
	state.A = 0xaa;
	state.X = 0xbb;
	state.Y = 0xcc;
	state.SP = 0xef;
	state.PS = SpcFlags::Carry | SpcFlags::Negative;
	EXPECT_EQ(state.PC, 0xffc0);
	EXPECT_EQ(state.A, 0xaa);
	EXPECT_EQ(state.X, 0xbb);
	EXPECT_EQ(state.Y, 0xcc);
	EXPECT_EQ(state.SP, 0xef);
	EXPECT_EQ(state.PS, 0x81);
}

TEST(SpcStateTest, OutputPorts) {
	SpcState state = {};
	for (int i = 0; i < 4; i++) {
		EXPECT_EQ(state.OutputReg[i], 0);
		state.OutputReg[i] = static_cast<uint8_t>(0x10 + i);
	}
	EXPECT_EQ(state.OutputReg[0], 0x10);
	EXPECT_EQ(state.OutputReg[3], 0x13);
}

// ============================================================================
// AluState struct
// ============================================================================

TEST(AluStateTest, DefaultValues) {
	AluState state = {};
	EXPECT_EQ(state.MultOperand1, 0);
	EXPECT_EQ(state.MultOperand2, 0);
	EXPECT_EQ(state.MultOrRemainderResult, 0);
	EXPECT_EQ(state.Dividend, 0);
	EXPECT_EQ(state.Divisor, 0);
	EXPECT_EQ(state.DivResult, 0);
}

TEST(AluStateTest, MultiplyValues) {
	AluState state = {};
	state.MultOperand1 = 0xff;
	state.MultOperand2 = 0xff;
	state.MultOrRemainderResult = 0xfe01;
	EXPECT_EQ(state.MultOperand1, 0xff);
	EXPECT_EQ(state.MultOperand2, 0xff);
	EXPECT_EQ(state.MultOrRemainderResult, 0xfe01);
}

TEST(AluStateTest, DivideValues) {
	AluState state = {};
	state.Dividend = 0xffff;
	state.Divisor = 0xff;
	state.DivResult = 0x0101;
	EXPECT_EQ(state.Dividend, 0xffff);
	EXPECT_EQ(state.Divisor, 0xff);
	EXPECT_EQ(state.DivResult, 0x0101);
}

// ============================================================================
// InternalRegisterState struct
// ============================================================================

TEST(InternalRegisterStateTest, DefaultValues) {
	InternalRegisterState state = {};
	EXPECT_FALSE(state.EnableAutoJoypadRead);
	EXPECT_FALSE(state.EnableFastRom);
	EXPECT_FALSE(state.EnableNmi);
	EXPECT_FALSE(state.EnableHorizontalIrq);
	EXPECT_FALSE(state.EnableVerticalIrq);
	EXPECT_EQ(state.HorizontalTimer, 0);
	EXPECT_EQ(state.VerticalTimer, 0);
}

TEST(InternalRegisterStateTest, TimerValues) {
	InternalRegisterState state = {};
	state.HorizontalTimer = 339;
	state.VerticalTimer = 261;
	state.EnableHorizontalIrq = true;
	state.EnableVerticalIrq = true;
	EXPECT_EQ(state.HorizontalTimer, 339);
	EXPECT_EQ(state.VerticalTimer, 261);
	EXPECT_TRUE(state.EnableHorizontalIrq);
	EXPECT_TRUE(state.EnableVerticalIrq);
}

// ============================================================================
// DmaChannelConfig struct
// ============================================================================

TEST(DmaChannelConfigTest, Fields) {
	DmaChannelConfig ch = {};
	ch.SrcAddress = 0x8000;
	ch.SrcBank = 0x7e;
	ch.DestAddress = 0x18;
	ch.TransferMode = 1;
	ch.DmaActive = true;
	EXPECT_EQ(ch.SrcAddress, 0x8000);
	EXPECT_EQ(ch.SrcBank, 0x7e);
	EXPECT_EQ(ch.DestAddress, 0x18);
	EXPECT_EQ(ch.TransferMode, 1);
	EXPECT_TRUE(ch.DmaActive);
}

TEST(DmaChannelConfigTest, HdmaFields) {
	DmaChannelConfig ch = {};
	ch.HdmaTableAddress = 0xc000;
	ch.HdmaIndirectAddressing = true;
	ch.HdmaBank = 0x00;
	ch.HdmaLineCounterAndRepeat = 0x84;
	ch.DoTransfer = true;
	ch.HdmaFinished = false;
	EXPECT_EQ(ch.HdmaTableAddress, 0xc000);
	EXPECT_TRUE(ch.HdmaIndirectAddressing);
	EXPECT_EQ(ch.HdmaBank, 0x00);
	EXPECT_EQ(ch.HdmaLineCounterAndRepeat, 0x84);
	EXPECT_TRUE(ch.DoTransfer);
	EXPECT_FALSE(ch.HdmaFinished);
}

// ============================================================================
// SnesDmaControllerState struct
// ============================================================================

TEST(SnesDmaControllerStateTest, ChannelArray) {
	SnesDmaControllerState dma = {};
	EXPECT_EQ(sizeof(dma.Channel) / sizeof(DmaChannelConfig), 8u);
	dma.HdmaChannels = 0xff;
	EXPECT_EQ(dma.HdmaChannels, 0xff);
}

// ============================================================================
// SnesCartInformation struct
// ============================================================================

TEST(SnesCartInformationTest, CartNameSize) {
	SnesCartInformation cart = {};
	EXPECT_EQ(sizeof(cart.CartName), 21u);
}

TEST(SnesCartInformationTest, VectorTableSize) {
	SnesCartInformation cart = {};
	EXPECT_EQ(sizeof(cart.CpuVectors), 0x20u);
}

// ============================================================================
// SnesState aggregate
// ============================================================================

TEST(SnesStateTest, AggregateContainsAllSubsystems) {
	SnesState state = {};
	state.MasterClock = 100000;
	state.WramPosition = 0x7e0000;
	EXPECT_EQ(state.MasterClock, 100000u);
	EXPECT_EQ(state.WramPosition, 0x7e0000u);

	state.Cpu.A = 0x1234;
	EXPECT_EQ(state.Cpu.A, 0x1234);

	state.Ppu.BgMode = 7;
	EXPECT_EQ(state.Ppu.BgMode, 7);

	state.Spc.PC = 0xffc0;
	EXPECT_EQ(state.Spc.PC, 0xffc0);

	state.Alu.MultOperand1 = 0x42;
	EXPECT_EQ(state.Alu.MultOperand1, 0x42);

	state.InternalRegs.EnableNmi = true;
	EXPECT_TRUE(state.InternalRegs.EnableNmi);

	state.Dma.HdmaChannels = 0x0f;
	EXPECT_EQ(state.Dma.HdmaChannels, 0x0f);
}

TEST(SnesStateTest, SizeReasonable) {
	EXPECT_GT(sizeof(SnesState), 1000u);
}
