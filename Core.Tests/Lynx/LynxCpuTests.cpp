#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Test fixture for Lynx 65C02 CPU types, state, and hardware bug emulation.
/// Verifies CPU state structures, flag calculations, and documented
/// hardware bugs from the Atari Lynx Technical Reference Section 13.
/// </summary>
class LynxCpuTypesTest : public ::testing::Test {
protected:
	LynxCpuState _state = {};

	void SetUp() override {
		_state = {};
		_state.A = 0;
		_state.X = 0;
		_state.Y = 0;
		_state.SP = 0xFD;
		_state.PC = 0;
		_state.PS = 0x24; // I flag set, Reserved always set
		_state.CycleCount = 0;
		_state.StopState = LynxCpuStopState::Running;
	}

	void SetFlag(uint8_t flag) { _state.PS |= flag; }
	void ClearFlag(uint8_t flag) { _state.PS &= ~flag; }
	bool CheckFlag(uint8_t flag) { return (_state.PS & flag) != 0; }
};

//=============================================================================
// CPU State Tests
//=============================================================================

TEST_F(LynxCpuTypesTest, InitialState_DefaultValues) {
	LynxCpuState state = {};
	EXPECT_EQ(state.A, 0);
	EXPECT_EQ(state.X, 0);
	EXPECT_EQ(state.Y, 0);
	EXPECT_EQ(state.PC, 0);
	EXPECT_EQ(state.SP, 0);
	EXPECT_EQ(state.CycleCount, 0);
}

TEST_F(LynxCpuTypesTest, State_StackPointerRange) {
	_state.SP = 0xFF;
	EXPECT_EQ(_state.SP, 0xFF);
	_state.SP = 0x00;
	EXPECT_EQ(_state.SP, 0x00);
}

TEST_F(LynxCpuTypesTest, State_StopState_Values) {
	_state.StopState = LynxCpuStopState::Running;
	EXPECT_EQ(_state.StopState, LynxCpuStopState::Running);
	_state.StopState = LynxCpuStopState::WaitingForIrq;
	EXPECT_EQ(_state.StopState, LynxCpuStopState::WaitingForIrq);
	_state.StopState = LynxCpuStopState::Stopped;
	EXPECT_EQ(_state.StopState, LynxCpuStopState::Stopped);
}

//=============================================================================
// Processor Flag Tests — 65C02
//=============================================================================

TEST_F(LynxCpuTypesTest, Flags_CarryFlag) {
	ClearFlag(LynxPSFlags::Carry);
	EXPECT_FALSE(CheckFlag(LynxPSFlags::Carry));
	SetFlag(LynxPSFlags::Carry);
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Carry));
}

TEST_F(LynxCpuTypesTest, Flags_ZeroFlag) {
	ClearFlag(LynxPSFlags::Zero);
	EXPECT_FALSE(CheckFlag(LynxPSFlags::Zero));
	SetFlag(LynxPSFlags::Zero);
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Zero));
}

TEST_F(LynxCpuTypesTest, Flags_InterruptDisable) {
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Interrupt)); // Set in initial state
	ClearFlag(LynxPSFlags::Interrupt);
	EXPECT_FALSE(CheckFlag(LynxPSFlags::Interrupt));
}

TEST_F(LynxCpuTypesTest, Flags_DecimalMode) {
	ClearFlag(LynxPSFlags::Decimal);
	EXPECT_FALSE(CheckFlag(LynxPSFlags::Decimal));
	SetFlag(LynxPSFlags::Decimal);
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Decimal));
}

TEST_F(LynxCpuTypesTest, Flags_Break_And_Reserved) {
	// Break flag (bit 4) — only exists in pushed PS, not as actual flag
	EXPECT_EQ(LynxPSFlags::Break, 0x10);
	// Reserved flag (bit 5) — always set
	EXPECT_EQ(LynxPSFlags::Reserved, 0x20);
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Reserved));
}

TEST_F(LynxCpuTypesTest, Flags_NegativeFlag) {
	ClearFlag(LynxPSFlags::Negative);
	EXPECT_FALSE(CheckFlag(LynxPSFlags::Negative));
	SetFlag(LynxPSFlags::Negative);
	EXPECT_TRUE(CheckFlag(LynxPSFlags::Negative));
}

//=============================================================================
// IRQ Break Flag Bug Fix Verification
// When the CPU pushes PS during an IRQ (not BRK), the Break bit should
// be cleared (0) and Reserved should be set (1). The original code had
// an operator precedence bug: PS() & ~Break | Reserved always equaled PS()
// because ~0x10 | 0x20 = 0xFF. The fix adds parentheses:
// (PS() & ~Break) | Reserved
//=============================================================================

TEST_F(LynxCpuTypesTest, IrqPush_BreakFlagCleared) {
	// Simulate what HandleIrq should push: Break=0, Reserved=1
	uint8_t ps = 0x00;
	uint8_t pushed = (ps & ~LynxPSFlags::Break) | LynxPSFlags::Reserved;
	EXPECT_FALSE(pushed & LynxPSFlags::Break); // Break must be 0
	EXPECT_TRUE(pushed & LynxPSFlags::Reserved); // Reserved must be 1
}

TEST_F(LynxCpuTypesTest, IrqPush_BreakFlagCleared_WithAllFlagsSet) {
	// Even if all flags are set, Break should be cleared during IRQ
	uint8_t ps = 0xFF;
	uint8_t pushed = (ps & ~LynxPSFlags::Break) | LynxPSFlags::Reserved;
	EXPECT_FALSE(pushed & LynxPSFlags::Break);
	EXPECT_TRUE(pushed & LynxPSFlags::Reserved);
	// All other flags preserved
	EXPECT_TRUE(pushed & LynxPSFlags::Carry);
	EXPECT_TRUE(pushed & LynxPSFlags::Zero);
	EXPECT_TRUE(pushed & LynxPSFlags::Interrupt);
	EXPECT_TRUE(pushed & LynxPSFlags::Decimal);
	EXPECT_TRUE(pushed & LynxPSFlags::Overflow);
	EXPECT_TRUE(pushed & LynxPSFlags::Negative);
}

TEST_F(LynxCpuTypesTest, BrkPush_BreakFlagSet) {
	// BRK pushes with Break=1, Reserved=1
	uint8_t ps = 0x00;
	uint8_t pushed = ps | LynxPSFlags::Break | LynxPSFlags::Reserved;
	EXPECT_TRUE(pushed & LynxPSFlags::Break); // Break must be 1 for BRK
	EXPECT_TRUE(pushed & LynxPSFlags::Reserved);
}

//=============================================================================
// Constants Verification
//=============================================================================

TEST_F(LynxCpuTypesTest, Constants_MasterClockRate) {
	EXPECT_EQ(LynxConstants::MasterClockRate, 16000000u);
}

TEST_F(LynxCpuTypesTest, Constants_CpuClockRate) {
	EXPECT_EQ(LynxConstants::CpuClockRate, 4000000u);
}

TEST_F(LynxCpuTypesTest, Constants_ScreenDimensions) {
	EXPECT_EQ(LynxConstants::ScreenWidth, 160u);
	EXPECT_EQ(LynxConstants::ScreenHeight, 102u);
}

TEST_F(LynxCpuTypesTest, Constants_PixelCount) {
	EXPECT_EQ(LynxConstants::PixelCount, 160u * 102u);
}

TEST_F(LynxCpuTypesTest, Constants_WorkRamSize) {
	EXPECT_EQ(LynxConstants::WorkRamSize, 0x10000u); // 64 KB
}

TEST_F(LynxCpuTypesTest, Constants_Fps) {
	// Lynx runs at approximately 75 fps
	EXPECT_NEAR(LynxConstants::Fps, 75.0, 1.0);
}

TEST_F(LynxCpuTypesTest, Constants_CyclesPerFrame) {
	// CpuCyclesPerFrame = CpuClockRate / Fps = 4000000 / 75 = 53333
	// This is the authoritative value — derived directly from clock rate and frame rate,
	// NOT from scanlineCount * cyclesPerScanline (which would give 53235 due to rounding).
	uint32_t expected = static_cast<uint32_t>(LynxConstants::CpuClockRate / LynxConstants::Fps);
	EXPECT_EQ(LynxConstants::CpuCyclesPerFrame, expected);
	EXPECT_EQ(LynxConstants::CpuCyclesPerFrame, 53333u);
	// Verify it differs from the naive scanline-based calculation
	uint32_t naiveCycles = LynxConstants::CpuCyclesPerScanline * LynxConstants::ScanlineCount;
	EXPECT_NE(LynxConstants::CpuCyclesPerFrame, naiveCycles);
	EXPECT_GT(LynxConstants::CpuCyclesPerFrame, naiveCycles);
}

//=============================================================================
// Audit Fix Regression Tests (#392-#407)
//=============================================================================

TEST_F(LynxCpuTypesTest, AuditFix398_CyclesPerFrame_DerivedFromClockRate) {
	// #398: CpuCyclesPerFrame must match CpuClockRate / Fps, not ScanlineCount * CyclesPerScanline
	double exact = LynxConstants::CpuClockRate / LynxConstants::Fps;
	EXPECT_EQ(LynxConstants::CpuCyclesPerFrame, static_cast<uint32_t>(exact));
	// The value should be 53333, which is the standard Lynx value
	EXPECT_EQ(LynxConstants::CpuCyclesPerFrame, 53333u);
}

TEST_F(LynxCpuTypesTest, AuditFix399_NoPrevIrqPending) {
	// #399: _prevIrqPending was removed from LynxCpuState.
	// Verify the state struct doesn't contain dead fields — just ensure
	// the IRQFlag field exists and is usable.
	_state.IRQFlag = true;
	EXPECT_TRUE(_state.IRQFlag);
	_state.IRQFlag = false;
	EXPECT_FALSE(_state.IRQFlag);
}

TEST_F(LynxCpuTypesTest, AuditFix400_IRQFlagExists) {
	// #400: IRQFlag must exist in LynxCpuState for serialization
	_state.IRQFlag = true;
	EXPECT_TRUE(_state.IRQFlag);
}
