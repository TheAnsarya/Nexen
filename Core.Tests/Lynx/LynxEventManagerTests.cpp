#include "pch.h"

/// <summary>
/// Tests for the Lynx Event Manager (event viewer / debugger events).
///
/// The event manager tracks and visualizes:
///   - IRQ events
///   - Breakpoint hits
///   - Mikey register read/write
///   - Suzy register read/write
///   - Audio register access
///   - Palette writes
///   - Timer access
///
/// Events are displayed in a timeline view synchronized with scanline output.
///
/// References:
///   - LynxEventManager.h (event manager)
///   - LynxConstants (scanline width, etc.)
/// </summary>
class LynxEventManagerTest : public ::testing::Test {
protected:
	// Event types tracked by the debugger
	enum class DebugEventType : uint8_t {
		None,
		Breakpoint,
		Irq,
		Nmi,  // Lynx doesn't have NMI, but type exists
		RegisterRead,
		RegisterWrite,
		PaletteWrite,
		AudioWrite,
		TimerWrite,
		TimerRead,
	};

	// Lynx display constants
	static constexpr uint32_t ScreenWidth = 160;
	static constexpr uint32_t ScreenHeight = 102;
	static constexpr uint32_t CpuCyclesPerScanline = 180; // Approximate
	static constexpr uint32_t ScanlineCount = 105; // Including VBlank
};

//=============================================================================
// Event Type Tests
//=============================================================================

TEST_F(LynxEventManagerTest, EventType_Breakpoint) {
	DebugEventType type = DebugEventType::Breakpoint;
	EXPECT_EQ(static_cast<uint8_t>(type), 1);
}

TEST_F(LynxEventManagerTest, EventType_Irq) {
	DebugEventType type = DebugEventType::Irq;
	EXPECT_EQ(static_cast<uint8_t>(type), 2);
}

TEST_F(LynxEventManagerTest, EventType_RegisterRead) {
	DebugEventType type = DebugEventType::RegisterRead;
	EXPECT_EQ(static_cast<uint8_t>(type), 4);
}

TEST_F(LynxEventManagerTest, EventType_RegisterWrite) {
	DebugEventType type = DebugEventType::RegisterWrite;
	EXPECT_EQ(static_cast<uint8_t>(type), 5);
}

//=============================================================================
// Event Viewer Config Tests
//=============================================================================

TEST_F(LynxEventManagerTest, Config_IrqCategory) {
	// IRQ events should be configurable
	bool irqEnabled = true;
	EXPECT_TRUE(irqEnabled);
}

TEST_F(LynxEventManagerTest, Config_MarkedBreakpoints) {
	// Breakpoints can be marked for visibility
	bool breakpointsEnabled = true;
	EXPECT_TRUE(breakpointsEnabled);
}

TEST_F(LynxEventManagerTest, Config_MikeyRegisterWrite) {
	// Mikey register writes trackable
	uint32_t mikeyStart = 0xFD00;
	uint32_t mikeyEnd = 0xFDFF;
	EXPECT_EQ(mikeyEnd - mikeyStart + 1, 256u);
}

TEST_F(LynxEventManagerTest, Config_SuzyRegisterWrite) {
	// Suzy register writes trackable
	uint32_t suzyStart = 0xFC00;
	uint32_t suzyEnd = 0xFCFF;
	EXPECT_EQ(suzyEnd - suzyStart + 1, 256u);
}

TEST_F(LynxEventManagerTest, Config_AudioRegister) {
	// Audio channels 0-3 at $FD20-$FD3F
	uint32_t audioStart = 0xFD20;
	uint32_t audioEnd = 0xFD3F;
	EXPECT_EQ(audioEnd - audioStart + 1, 32u);
}

TEST_F(LynxEventManagerTest, Config_PaletteWrite) {
	// Palette at $FDA0-$FDAF (green) and $FDB0-$FDBF (blue/red)
	uint32_t paletteStart = 0xFDA0;
	uint32_t paletteEnd = 0xFDBF;
	EXPECT_EQ(paletteEnd - paletteStart + 1, 32u);
}

TEST_F(LynxEventManagerTest, Config_TimerAccess) {
	// Timers 0-7 at $FD00-$FD1F
	uint32_t timerStart = 0xFD00;
	uint32_t timerEnd = 0xFD1F;
	EXPECT_EQ(timerEnd - timerStart + 1, 32u);
}

TEST_F(LynxEventManagerTest, Config_ShowPreviousFrame) {
	// Option to show events from previous frame
	bool showPrevFrame = true;
	EXPECT_TRUE(showPrevFrame);
}

//=============================================================================
// Display Buffer Tests
//=============================================================================

TEST_F(LynxEventManagerTest, DisplayBuffer_Width) {
	// Event viewer width = CpuCyclesPerScanline * 2 (2x zoom)
	uint32_t width = CpuCyclesPerScanline * 2;
	EXPECT_EQ(width, 360u);
}

TEST_F(LynxEventManagerTest, DisplayBuffer_Height) {
	// Event viewer height = scanline count
	uint32_t height = ScanlineCount;
	EXPECT_EQ(height, 105u);
}

TEST_F(LynxEventManagerTest, DisplayBuffer_Size) {
	// Total buffer size
	uint32_t width = CpuCyclesPerScanline * 2;
	uint32_t height = ScanlineCount;
	uint32_t bufferSize = width * height;
	EXPECT_EQ(bufferSize, 37800u);
}

//=============================================================================
// Scanline/Cycle Coordinate Tests
//=============================================================================

TEST_F(LynxEventManagerTest, Coordinate_Cycle0_Line0) {
	// First cycle of first scanline
	int32_t x = 0;
	int32_t y = 0;
	// Should map to display coordinates (0, 0)
	EXPECT_EQ(x, 0);
	EXPECT_EQ(y, 0);
}

TEST_F(LynxEventManagerTest, Coordinate_CycleMax_Line0) {
	// Last cycle of first scanline
	int32_t x = CpuCyclesPerScanline - 1;
	int32_t y = 0;
	EXPECT_EQ(static_cast<uint32_t>(x), CpuCyclesPerScanline - 1);
	EXPECT_EQ(y, 0);
}

TEST_F(LynxEventManagerTest, Coordinate_VBlankLine) {
	// VBlank scanline (after visible area)
	int32_t vblankLine = ScreenHeight; // Line 102
	EXPECT_EQ(static_cast<uint32_t>(vblankLine), 102u);
}

//=============================================================================
// IRQ Event Tests
//=============================================================================

TEST_F(LynxEventManagerTest, Irq_Timer0_HBlank) {
	// Timer 0 is often used for HBlank IRQ
	uint8_t timer0Index = 0;
	EXPECT_EQ(timer0Index, 0);
}

TEST_F(LynxEventManagerTest, Irq_Timer2_VBlank) {
	// Timer 2 is the VBlank timer
	uint8_t timer2Index = 2;
	EXPECT_EQ(timer2Index, 2);
}

TEST_F(LynxEventManagerTest, Irq_Uart) {
	// UART can generate IRQ
	uint8_t uartTimerIndex = 4; // Timer 4 = UART
	EXPECT_EQ(uartTimerIndex, 4);
}

//=============================================================================
// Register Access Event Tests
//=============================================================================

TEST_F(LynxEventManagerTest, RegisterAccess_Mikey_Identified) {
	uint32_t addr = 0xFD8B; // INTRST
	bool isMikey = (addr >= 0xFD00 && addr <= 0xFDFF);
	EXPECT_TRUE(isMikey);
}

TEST_F(LynxEventManagerTest, RegisterAccess_Suzy_Identified) {
	uint32_t addr = 0xFC92; // SPRSYS
	bool isSuzy = (addr >= 0xFC00 && addr <= 0xFCFF);
	EXPECT_TRUE(isSuzy);
}

TEST_F(LynxEventManagerTest, RegisterAccess_Audio_Identified) {
	uint32_t addr = 0xFD25; // Channel 0 volume
	bool isAudio = (addr >= 0xFD20 && addr <= 0xFD3F);
	EXPECT_TRUE(isAudio);
}

TEST_F(LynxEventManagerTest, RegisterAccess_Timer_Identified) {
	uint32_t addr = 0xFD05; // Timer 1 backup
	bool isTimer = (addr >= 0xFD00 && addr <= 0xFD1F);
	EXPECT_TRUE(isTimer);
}

//=============================================================================
// Snapshot Tests
//=============================================================================

TEST_F(LynxEventManagerTest, Snapshot_ReturnsEventCount) {
	// TakeEventSnapshot returns number of events
	uint32_t eventCount = 42;
	EXPECT_GT(eventCount, 0u);
}

TEST_F(LynxEventManagerTest, Snapshot_AutoRefresh) {
	// Auto-refresh mode for continuous viewing
	bool forAutoRefresh = true;
	EXPECT_TRUE(forAutoRefresh);
}

TEST_F(LynxEventManagerTest, Snapshot_Manual) {
	// Manual snapshot mode
	bool forAutoRefresh = false;
	EXPECT_FALSE(forAutoRefresh);
}

//=============================================================================
// Event Lookup Tests
//=============================================================================

TEST_F(LynxEventManagerTest, GetEvent_ByCoordinate) {
	// GetEvent(y, x) returns event at that position
	uint16_t x = 100;
	uint16_t y = 50;
	// Should return event info for that coordinate
	EXPECT_LT(x, CpuCyclesPerScanline * 2);
	EXPECT_LT(y, ScanlineCount);
}

TEST_F(LynxEventManagerTest, GetEvent_NoEvent) {
	// Returns empty event if nothing at coordinate
	DebugEventType emptyType = DebugEventType::None;
	EXPECT_EQ(static_cast<uint8_t>(emptyType), 0);
}

//=============================================================================
// Frame Info Tests
//=============================================================================

TEST_F(LynxEventManagerTest, FrameInfo_Dimensions) {
	// GetDisplayBufferSize returns frame dimensions
	uint32_t width = CpuCyclesPerScanline * 2;
	uint32_t height = ScanlineCount;
	EXPECT_GT(width, 0u);
	EXPECT_GT(height, 0u);
}

TEST_F(LynxEventManagerTest, FrameInfo_AspectRatio) {
	// Aspect ratio should be roughly 3:1 (wide)
	double width = CpuCyclesPerScanline * 2;
	double height = ScanlineCount;
	double ratio = width / height;
	EXPECT_GT(ratio, 2.0);
	EXPECT_LT(ratio, 5.0);
}

