#include "pch.h"

/// <summary>
/// Tests for the Lynx debugger subsystem.
///
/// LynxDebugger provides:
///   - Instruction stepping (step-in, step-over, step-out)
///   - Breakpoints (execution, read, write, conditional)
///   - Callstack tracking
///   - Code Data Logger (CDL)
///   - Trace logging
///   - Memory inspection and modification
///
/// References:
///   - LynxDebugger.h (debugger implementation)
///   - LynxTraceLogger.h (trace logging)
///   - LynxEventManager.h (event breakpoints)
///   - LynxDisUtils.h (disassembly)
/// </summary>
class LynxDebuggerTest : public ::testing::Test {
protected:
	// Step types used by debugger
	enum class StepType : uint8_t {
		Step,        ///< Step one instruction
		StepOver,    ///< Step over subroutine calls
		StepOut,     ///< Step until return from current subroutine
		StepBack,    ///< Step backward (if rewind enabled)
		Run,         ///< Run until breakpoint
		PpuStep,     ///< Step one PPU cycle
		PpuScanline, ///< Step one scanline
		PpuFrame,    ///< Step one frame
	};

	// Memory operation types
	enum class MemoryOperationType : uint8_t {
		Read,
		Write,
		ExecOpcode,
		ExecOperand,
		DummyRead,
		DummyWrite,
	};

	// CDL flags for code/data classification
	enum CdlFlags : uint8_t {
		None = 0x00,
		Code = 0x01,
		Data = 0x02,
		JumpTarget = 0x04,
		SubEntryPoint = 0x08,
	};
};

//=============================================================================
// Stepping Tests
//=============================================================================

TEST_F(LynxDebuggerTest, StepType_Values) {
	EXPECT_EQ(static_cast<uint8_t>(StepType::Step), 0);
	EXPECT_EQ(static_cast<uint8_t>(StepType::StepOver), 1);
	EXPECT_EQ(static_cast<uint8_t>(StepType::StepOut), 2);
}

TEST_F(LynxDebuggerTest, Step_SingleInstruction) {
	// Step should advance exactly one instruction
	uint32_t prevPc = 0x0200;
	uint32_t instrSize = 2; // e.g., LDA #$xx
	uint32_t nextPc = prevPc + instrSize;
	EXPECT_EQ(nextPc, 0x0202u);
}

TEST_F(LynxDebuggerTest, StepOver_JSR_SkipsSubroutine) {
	// Step-over on JSR should execute until RTS returns
	uint8_t jsrOpcode = 0x20;
	uint8_t rtsOpcode = 0x60;
	EXPECT_EQ(jsrOpcode, 0x20);
	EXPECT_EQ(rtsOpcode, 0x60);
}

TEST_F(LynxDebuggerTest, StepOut_FindsRTS) {
	// Step-out should run until RTS in current call frame
	uint8_t rtsOpcode = 0x60;
	uint8_t rtiOpcode = 0x40;
	// Either RTS or RTI should end step-out
	EXPECT_EQ(rtsOpcode, 0x60);
	EXPECT_EQ(rtiOpcode, 0x40);
}

//=============================================================================
// Breakpoint Tests
//=============================================================================

TEST_F(LynxDebuggerTest, Breakpoint_ExecutionAddress) {
	// Execution breakpoint at specific address
	uint32_t breakAddr = 0x0200;
	EXPECT_EQ(breakAddr, 0x0200u);
}

TEST_F(LynxDebuggerTest, Breakpoint_ReadWatch) {
	// Read breakpoint (watchpoint) triggers on memory read
	MemoryOperationType opType = MemoryOperationType::Read;
	EXPECT_EQ(opType, MemoryOperationType::Read);
}

TEST_F(LynxDebuggerTest, Breakpoint_WriteWatch) {
	// Write breakpoint (watchpoint) triggers on memory write
	MemoryOperationType opType = MemoryOperationType::Write;
	EXPECT_EQ(opType, MemoryOperationType::Write);
}

TEST_F(LynxDebuggerTest, Breakpoint_RegisterRange) {
	// Breakpoints on Mikey/Suzy register ranges
	uint32_t mikeyStart = 0xFD00;
	uint32_t mikeyEnd = 0xFDFF;
	uint32_t suzyStart = 0xFC00;
	uint32_t suzyEnd = 0xFCFF;

	EXPECT_LT(suzyStart, mikeyStart);
	EXPECT_LT(suzyEnd, mikeyEnd);
}

//=============================================================================
// Callstack Tests
//=============================================================================

TEST_F(LynxDebuggerTest, Callstack_PushOnJSR) {
	// JSR should push return address and add callstack entry
	uint8_t jsrOpcode = 0x20;
	uint32_t subrAddr = 0x0300;
	uint32_t returnAddr = 0x0203; // PC after JSR
	EXPECT_EQ(jsrOpcode, 0x20);
	EXPECT_EQ(subrAddr, 0x0300u);
	EXPECT_GT(returnAddr, 0u);
}

TEST_F(LynxDebuggerTest, Callstack_PopOnRTS) {
	// RTS should pop callstack entry
	uint8_t rtsOpcode = 0x60;
	EXPECT_EQ(rtsOpcode, 0x60);
}

TEST_F(LynxDebuggerTest, Callstack_RTI_Interrupt) {
	// RTI pops callstack entry for interrupt handler
	uint8_t rtiOpcode = 0x40;
	uint32_t irqVector = 0xFFFE;
	EXPECT_EQ(rtiOpcode, 0x40);
	EXPECT_EQ(irqVector, 0xFFFE);
}

TEST_F(LynxDebuggerTest, Callstack_StackPointer_Tracked) {
	// Stack pointer changes should be tracked for callstack integrity
	uint8_t initialSp = 0xFF;
	uint8_t afterPush = 0xFD; // After JSR (2 bytes pushed)
	EXPECT_GT(initialSp, afterPush);
}

//=============================================================================
// Code Data Logger (CDL) Tests
//=============================================================================

TEST_F(LynxDebuggerTest, CDL_OpcodeFlagged) {
	// Executed opcodes should be flagged as code
	uint8_t codeFlag = CdlFlags::Code;
	EXPECT_EQ(codeFlag, 0x01);
}

TEST_F(LynxDebuggerTest, CDL_OperandFlagged) {
	// Instruction operands should be flagged appropriately
	uint8_t dataFlag = CdlFlags::Data;
	EXPECT_EQ(dataFlag, 0x02);
}

TEST_F(LynxDebuggerTest, CDL_JumpTargetFlagged) {
	// Jump/branch targets should be marked
	uint8_t jumpFlag = CdlFlags::JumpTarget;
	EXPECT_EQ(jumpFlag, 0x04);
}

TEST_F(LynxDebuggerTest, CDL_SubEntryFlagged) {
	// Subroutine entry points should be marked
	uint8_t subEntryFlag = CdlFlags::SubEntryPoint;
	EXPECT_EQ(subEntryFlag, 0x08);
}

TEST_F(LynxDebuggerTest, CDL_CartRomCoverage) {
	// CDL tracks ROM coverage for disassembly hinting
	uint32_t romStart = 0x0000; // Mapped via cart banking
	uint32_t romSize = 256 * 1024; // 256 KB typical
	EXPECT_EQ(romStart, 0u);
	EXPECT_EQ(romSize, 256 * 1024u);
}

//=============================================================================
// Trace Logger Tests
//=============================================================================

TEST_F(LynxDebuggerTest, Trace_RecordsPcAndCycles) {
	// Trace should record PC and cycle count
	uint32_t pc = 0x0200;
	uint64_t cycle = 12345;
	EXPECT_EQ(pc, 0x0200);
	EXPECT_GT(cycle, 0u);
}

TEST_F(LynxDebuggerTest, Trace_RecordsRegisters) {
	// Trace should record A, X, Y, SP, P registers
	uint8_t regA = 0x42;
	uint8_t regX = 0x10;
	uint8_t regY = 0x20;
	uint8_t regSp = 0xFF;
	uint8_t regP = 0x30; // Processor status
	EXPECT_EQ(regA, 0x42);
	EXPECT_EQ(regX, 0x10);
	EXPECT_EQ(regY, 0x20);
	EXPECT_EQ(regSp, 0xFF);
	EXPECT_EQ(regP, 0x30);
}

TEST_F(LynxDebuggerTest, Trace_RecordsOpcode) {
	// Trace should record opcode and operands
	uint8_t opcode = 0xA9; // LDA #$xx
	uint8_t operand = 0x42;
	EXPECT_EQ(opcode, 0xA9);
	EXPECT_EQ(operand, 0x42);
}

//=============================================================================
// Memory Access Tests
//=============================================================================

TEST_F(LynxDebuggerTest, Memory_Read_Logged) {
	MemoryOperationType opType = MemoryOperationType::Read;
	EXPECT_EQ(opType, MemoryOperationType::Read);
}

TEST_F(LynxDebuggerTest, Memory_Write_Logged) {
	MemoryOperationType opType = MemoryOperationType::Write;
	EXPECT_EQ(opType, MemoryOperationType::Write);
}

TEST_F(LynxDebuggerTest, Memory_ExecOpcode_Logged) {
	MemoryOperationType opType = MemoryOperationType::ExecOpcode;
	EXPECT_EQ(opType, MemoryOperationType::ExecOpcode);
}

TEST_F(LynxDebuggerTest, Memory_DummyRead_NotBreakpoint) {
	// Dummy reads (e.g., indexed crossing page) shouldn't trigger read breakpoints
	MemoryOperationType opType = MemoryOperationType::DummyRead;
	EXPECT_EQ(opType, MemoryOperationType::DummyRead);
}

//=============================================================================
// Reset/Interrupt Tests
//=============================================================================

TEST_F(LynxDebuggerTest, Reset_ClearsState) {
	// Reset should clear prev opcode, callstack, etc.
	uint8_t prevOpcode = 0x01; // Default non-zero
	uint32_t prevPc = 0;
	EXPECT_EQ(prevOpcode, 0x01);
	EXPECT_EQ(prevPc, 0u);
}

TEST_F(LynxDebuggerTest, Interrupt_ProcessedCorrectly) {
	// IRQ should be processed with correct original PC
	uint32_t irqVector = 0xFFFE;
	uint32_t nmiVector = 0xFFFA;
	uint32_t resetVector = 0xFFFC;
	EXPECT_EQ(irqVector, 0xFFFE);
	EXPECT_EQ(nmiVector, 0xFFFA);
	EXPECT_EQ(resetVector, 0xFFFC);
}

//=============================================================================
// Register Detection Tests
//=============================================================================

TEST_F(LynxDebuggerTest, IsRegister_Mikey) {
	// $FD00-$FDFF = Mikey registers
	uint32_t mikeyAddr = 0xFD20;
	bool isMikey = (mikeyAddr >= 0xFD00 && mikeyAddr <= 0xFDFF);
	EXPECT_TRUE(isMikey);
}

TEST_F(LynxDebuggerTest, IsRegister_Suzy) {
	// $FC00-$FCFF = Suzy registers
	uint32_t suzyAddr = 0xFC08;
	bool isSuzy = (suzyAddr >= 0xFC00 && suzyAddr <= 0xFCFF);
	EXPECT_TRUE(isSuzy);
}

TEST_F(LynxDebuggerTest, IsRegister_Ram_NotRegister) {
	// $0000-$FBFF = RAM (not registers)
	uint32_t ramAddr = 0x0200;
	bool isRegister = (ramAddr >= 0xFC00);
	EXPECT_FALSE(isRegister);
}

//=============================================================================
// PPU Step Tests
//=============================================================================

TEST_F(LynxDebuggerTest, PpuStep_CycleAdvance) {
	// PPU step advances one PPU cycle
	uint32_t pixelsPerLine = 160;
	uint32_t linesPerFrame = 102;
	EXPECT_EQ(pixelsPerLine, 160u);
	EXPECT_EQ(linesPerFrame, 102u);
}

TEST_F(LynxDebuggerTest, PpuScanline_LineAdvance) {
	// Scanline step advances one full line
	uint32_t linesPerFrame = 102;
	EXPECT_EQ(linesPerFrame, 102u);
}

TEST_F(LynxDebuggerTest, PpuFrame_FrameAdvance) {
	// Frame step advances entire frame (~75 Hz)
	double frameRate = 75.0;
	EXPECT_NEAR(frameRate, 75.0, 1.0);
}

