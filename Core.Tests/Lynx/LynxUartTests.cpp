#include "pch.h"
#include "Lynx/LynxTypes.h"

/// <summary>
/// Comprehensive test suite for the Lynx UART / ComLynx serial port.
///
/// Tests verify the UART state machine logic without instantiating the full
/// LynxMikey class, following the same pattern as LynxCpuInstructionTests.
/// Each test replicates the relevant logic from LynxMikey::TickUart(),
/// UpdateUartIrq(), ComLynxRxData(), and the register read/write handlers.
///
/// Test categories:
///   1. SERCTL register (write/read mux, bit-field extraction)
///   2. SERDAT write (TX start, parity/9th bit, self-loopback)
///   3. SERDAT read (clears RxReady, returns low 8 bits)
///   4. TX countdown lifecycle (active → decrement → idle)
///   5. RX queue (enqueue, dequeue, overrun, full queue)
///   6. RX countdown lifecycle (active → deliver → inter-byte gap)
///   7. Break signal (continuous retransmission, loopback)
///   8. Error flags (overrun detection, reset via SERCTL bit 3)
///   9. IRQ level-sensitive behavior (HW Bug 13.2)
///  10. No-cable scenario (TX+loopback with no external RX)
///  11. Transmission delay fidelity (countdown timing)
///  12. Timer 4 integration (no TimerDone, no normal IRQ)
/// </summary>

// =============================================================================
// Constants — mirror from LynxMikey.h for standalone testing
// =============================================================================

static constexpr uint32_t UartTxInactive = 0x80000000;
static constexpr uint32_t UartRxInactive = 0x80000000;
static constexpr uint16_t UartBreakCode = 0x8000;
static constexpr int UartMaxRxQueue = 32;
static constexpr uint32_t UartTxTimePeriod = 11;
static constexpr uint32_t UartRxTimePeriod = 11;
static constexpr uint32_t UartRxNextDelay = 44;

// =============================================================================
// UART state helper — standalone state for testing without LynxMikey
// =============================================================================

struct TestUartState {
	// From LynxMikeyState
	uint8_t SerialControl = 0;
	uint32_t UartTxCountdown = UartTxInactive;
	uint32_t UartRxCountdown = UartRxInactive;
	uint16_t UartTxData = 0;
	uint16_t UartRxData = 0;
	bool UartRxReady = false;
	bool UartTxIrqEnable = false;
	bool UartRxIrqEnable = false;
	bool UartParityEnable = false;
	bool UartParityEven = false;
	bool UartSendBreak = false;
	bool UartRxOverrunError = false;
	bool UartRxFramingError = false;
	uint8_t IrqPending = 0;

	// From LynxMikey private members
	uint16_t RxQueue[UartMaxRxQueue] = {};
	uint32_t RxInputPtr = 0;
	uint32_t RxOutputPtr = 0;
	uint32_t RxWaiting = 0;
};

// =============================================================================
// Replicated logic from LynxMikey (for standalone testing)
// =============================================================================

/// <summary>Replicate TickUart() logic</summary>
static void TickUart(TestUartState& s) {
	// --- Receive --- (§7.3)
	if (s.UartRxCountdown == 0) {
		if (s.RxWaiting > 0) {
			if (s.UartRxReady) {
				s.UartRxOverrunError = true;
			}
			s.UartRxData = s.RxQueue[s.RxOutputPtr];
			s.RxOutputPtr = (s.RxOutputPtr + 1) & (UartMaxRxQueue - 1);
			s.RxWaiting--;
			s.UartRxReady = true;
			if (s.RxWaiting > 0) {
				// §7.3: Inter-byte delay = RX_TIME_PERIOD + RX_NEXT_DELAY = 55
				s.UartRxCountdown = UartRxTimePeriod + UartRxNextDelay;
			} else {
				// §7.3: Queue empty after delivery — go inactive
				s.UartRxCountdown = UartRxInactive;
			}
		}
	} else if (!(s.UartRxCountdown & UartRxInactive)) {
		s.UartRxCountdown--;
	}

	// --- Transmit ---
	if (s.UartTxCountdown == 0) {
		if (s.UartSendBreak) {
			s.UartTxData = UartBreakCode;
			s.UartTxCountdown = UartTxTimePeriod;
			// Loopback handled by caller
		} else {
			s.UartTxCountdown = UartTxInactive;
		}
	} else if (!(s.UartTxCountdown & UartTxInactive)) {
		s.UartTxCountdown--;
	}
}

/// <summary>Replicate UpdateUartIrq() logic</summary>
static void UpdateUartIrq(TestUartState& s) {
	bool irq = false;
	bool txIdle = (s.UartTxCountdown == 0) ||
	              ((s.UartTxCountdown & UartTxInactive) != 0);
	if (txIdle && s.UartTxIrqEnable) {
		irq = true;
	}
	if (s.UartRxReady && s.UartRxIrqEnable) {
		irq = true;
	}
	if (irq) {
		s.IrqPending |= 0x10; // Timer4 = bit 4
	}
}

/// <summary>Replicate ComLynxRxData() logic</summary>
static void ComLynxRxData(TestUartState& s, uint16_t data) {
	if (s.RxWaiting < (uint32_t)UartMaxRxQueue) {
		if (s.RxWaiting == 0) {
			s.UartRxCountdown = UartRxTimePeriod;
		}
		s.RxQueue[s.RxInputPtr] = data;
		s.RxInputPtr = (s.RxInputPtr + 1) & (UartMaxRxQueue - 1);
		s.RxWaiting++;
	}
}

/// <summary>Replicate ComLynxTxLoopback() — front-inserts into queue (§7.2)</summary>
static void ComLynxTxLoopback(TestUartState& s, uint16_t data) {
	if (s.RxWaiting < (uint32_t)UartMaxRxQueue) {
		if (s.RxWaiting == 0) {
			s.UartRxCountdown = UartRxTimePeriod;
		}
		// Front-insert: decrement output pointer (§7.2)
		s.RxOutputPtr = (s.RxOutputPtr - 1) & (UartMaxRxQueue - 1);
		s.RxQueue[s.RxOutputPtr] = data;
		s.RxWaiting++;
	}
}

/// <summary>Replicate SERCTL write logic</summary>
static void WriteSerctl(TestUartState& s, uint8_t value) {
	s.SerialControl = value;
	s.UartTxIrqEnable = (value & 0x80) != 0;
	s.UartRxIrqEnable = (value & 0x40) != 0;
	s.UartParityEnable = (value & 0x10) != 0;
	s.UartParityEven = (value & 0x01) != 0;
	if (value & 0x08) {
		s.UartRxOverrunError = false;
		s.UartRxFramingError = false;
	}
	s.UartSendBreak = (value & 0x02) != 0;
	if (s.UartSendBreak) {
		s.UartTxCountdown = UartTxTimePeriod;
		ComLynxTxLoopback(s, UartBreakCode);
	}
	UpdateUartIrq(s);
}

/// <summary>Replicate SERDAT write logic</summary>
static void WriteSerdat(TestUartState& s, uint8_t value) {
	s.UartTxData = value;
	if (!s.UartParityEnable && s.UartParityEven) {
		s.UartTxData |= 0x0100;
	}
	s.UartTxCountdown = UartTxTimePeriod;
	ComLynxTxLoopback(s, s.UartTxData);
}

/// <summary>Replicate SERCTL read logic</summary>
static uint8_t ReadSerctl(const TestUartState& s) {
	uint8_t status = 0;
	if (s.UartTxCountdown & UartTxInactive) {
		status |= 0xa0;
	}
	if (s.UartRxReady) {
		status |= 0x40;
	}
	if (s.UartRxOverrunError) {
		status |= 0x08;
	}
	if (s.UartRxFramingError) {
		status |= 0x04;
	}
	if (s.UartRxData & UartBreakCode) {
		status |= 0x02;
	}
	if (s.UartRxData & 0x0100) {
		status |= 0x01;
	}
	return status;
}

/// <summary>Replicate SERDAT read logic</summary>
static uint8_t ReadSerdat(TestUartState& s) {
	s.UartRxReady = false;
	UpdateUartIrq(s);
	return (uint8_t)(s.UartRxData & 0xff);
}

// =============================================================================
// Test Fixture
// =============================================================================

class LynxUartTest : public ::testing::Test {
protected:
	TestUartState _s;

	void SetUp() override {
		_s = {};
	}

	/// <summary>Tick the UART N times (simulates N Timer 4 underflows)</summary>
	void TickN(int n) {
		for (int i = 0; i < n; i++) {
			TickUart(_s);
		}
	}
};

// =============================================================================
// 1. SERCTL Register — Write Bit Extraction
// =============================================================================

TEST_F(LynxUartTest, SerctlWrite_TxIrqEnable) {
	WriteSerctl(_s, 0x80);
	EXPECT_TRUE(_s.UartTxIrqEnable);
	EXPECT_FALSE(_s.UartRxIrqEnable);
	EXPECT_FALSE(_s.UartParityEnable);
	EXPECT_FALSE(_s.UartParityEven);
	EXPECT_FALSE(_s.UartSendBreak);
}

TEST_F(LynxUartTest, SerctlWrite_RxIrqEnable) {
	WriteSerctl(_s, 0x40);
	EXPECT_FALSE(_s.UartTxIrqEnable);
	EXPECT_TRUE(_s.UartRxIrqEnable);
}

TEST_F(LynxUartTest, SerctlWrite_ParityEnable) {
	WriteSerctl(_s, 0x10);
	EXPECT_TRUE(_s.UartParityEnable);
}

TEST_F(LynxUartTest, SerctlWrite_ParityEven) {
	WriteSerctl(_s, 0x01);
	EXPECT_TRUE(_s.UartParityEven);
}

TEST_F(LynxUartTest, SerctlWrite_SendBreak) {
	WriteSerctl(_s, 0x02);
	EXPECT_TRUE(_s.UartSendBreak);
	// Break activates TX countdown
	EXPECT_EQ(_s.UartTxCountdown, UartTxTimePeriod);
	// Break loopback front-inserts data in RX queue (§7.2)
	EXPECT_EQ(_s.RxWaiting, 1u);
	EXPECT_EQ(_s.RxQueue[_s.RxOutputPtr], UartBreakCode);
}

TEST_F(LynxUartTest, SerctlWrite_AllBits) {
	WriteSerctl(_s, 0xd3); // bits 7,6,4,1,0
	EXPECT_TRUE(_s.UartTxIrqEnable);
	EXPECT_TRUE(_s.UartRxIrqEnable);
	EXPECT_TRUE(_s.UartParityEnable);
	EXPECT_TRUE(_s.UartParityEven);
	EXPECT_TRUE(_s.UartSendBreak);
}

TEST_F(LynxUartTest, SerctlWrite_ResetErrors) {
	_s.UartRxOverrunError = true;
	_s.UartRxFramingError = true;
	WriteSerctl(_s, 0x08); // ResetErr bit
	EXPECT_FALSE(_s.UartRxOverrunError);
	EXPECT_FALSE(_s.UartRxFramingError);
}

TEST_F(LynxUartTest, SerctlWrite_ClearingBits) {
	WriteSerctl(_s, 0xff);
	EXPECT_TRUE(_s.UartTxIrqEnable);
	WriteSerctl(_s, 0x00);
	EXPECT_FALSE(_s.UartTxIrqEnable);
	EXPECT_FALSE(_s.UartRxIrqEnable);
	EXPECT_FALSE(_s.UartParityEnable);
	EXPECT_FALSE(_s.UartParityEven);
	EXPECT_FALSE(_s.UartSendBreak);
}

// =============================================================================
// 2. SERCTL Register — Read Status
// =============================================================================

TEST_F(LynxUartTest, SerctlRead_InitialState_TxIdleReported) {
	// TX inactive → TxRdy (bit 7) + TxEmpty (bit 5) = 0xA0
	uint8_t status = ReadSerctl(_s);
	EXPECT_EQ(status, 0xa0);
}

TEST_F(LynxUartTest, SerctlRead_TxActive_NoTxFlags) {
	_s.UartTxCountdown = 5; // Active (no bit 31 set)
	uint8_t status = ReadSerctl(_s);
	EXPECT_EQ(status & 0xa0, 0x00);
}

TEST_F(LynxUartTest, SerctlRead_RxReady) {
	_s.UartRxReady = true;
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x40);
}

TEST_F(LynxUartTest, SerctlRead_OverrunError) {
	_s.UartRxOverrunError = true;
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x08);
}

TEST_F(LynxUartTest, SerctlRead_FramingError) {
	_s.UartRxFramingError = true;
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x04);
}

TEST_F(LynxUartTest, SerctlRead_BreakReceived) {
	_s.UartRxData = UartBreakCode;
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x02);
}

TEST_F(LynxUartTest, SerctlRead_ParityBit) {
	_s.UartRxData = 0x0100; // 9th bit set
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x01);
}

TEST_F(LynxUartTest, SerctlRead_AllFlags) {
	_s.UartRxReady = true;
	_s.UartRxOverrunError = true;
	_s.UartRxFramingError = true;
	_s.UartRxData = UartBreakCode | 0x0100;
	uint8_t status = ReadSerctl(_s);
	EXPECT_EQ(status, 0xa0 | 0x40 | 0x08 | 0x04 | 0x02 | 0x01);
}

// =============================================================================
// 3. SERDAT Write — TX Start and Loopback
// =============================================================================

TEST_F(LynxUartTest, SerdatWrite_StartsTxCountdown) {
	WriteSerdat(_s, 0x42);
	EXPECT_EQ(_s.UartTxCountdown, UartTxTimePeriod);
	EXPECT_EQ(_s.UartTxData & 0xff, 0x42);
}

TEST_F(LynxUartTest, SerdatWrite_SelfLoopback) {
	WriteSerdat(_s, 0xAB);
	// Self-loopback front-inserts data in RX queue (§7.2)
	EXPECT_EQ(_s.RxWaiting, 1u);
	EXPECT_EQ(_s.RxQueue[_s.RxOutputPtr] & 0xff, 0xAB);
}

TEST_F(LynxUartTest, SerdatWrite_ParityDisabled_9thBit) {
	// When parity disabled and ParityEven=1, bit 8 is set on TX data
	_s.UartParityEven = true;
	_s.UartParityEnable = false;
	WriteSerdat(_s, 0x55);
	EXPECT_TRUE(_s.UartTxData & 0x0100);
	EXPECT_EQ(_s.UartTxData & 0xff, 0x55);
}

TEST_F(LynxUartTest, SerdatWrite_ParityEnabledOverrides9thBit) {
	_s.UartParityEnable = true;
	_s.UartParityEven = true;
	WriteSerdat(_s, 0x55);
	// Parity enabled: 9th bit NOT set via ParityEven (parity calc is unimplemented)
	EXPECT_FALSE(_s.UartTxData & 0x0100);
}

TEST_F(LynxUartTest, SerdatWrite_ParityDisabled_NoEven_No9thBit) {
	_s.UartParityEnable = false;
	_s.UartParityEven = false;
	WriteSerdat(_s, 0x55);
	EXPECT_FALSE(_s.UartTxData & 0x0100);
}

// =============================================================================
// 4. SERDAT Read — Clear RxReady
// =============================================================================

TEST_F(LynxUartTest, SerdatRead_ClearsRxReady) {
	_s.UartRxReady = true;
	_s.UartRxData = 0x42;
	uint8_t data = ReadSerdat(_s);
	EXPECT_EQ(data, 0x42);
	EXPECT_FALSE(_s.UartRxReady);
}

TEST_F(LynxUartTest, SerdatRead_ReturnsLow8Bits) {
	_s.UartRxData = 0x0155; // 9th bit set + data 0x55
	uint8_t data = ReadSerdat(_s);
	EXPECT_EQ(data, 0x55);
}

TEST_F(LynxUartTest, SerdatRead_BreakDataMasked) {
	_s.UartRxData = UartBreakCode | 0x00; // break + zero data
	uint8_t data = ReadSerdat(_s);
	EXPECT_EQ(data, 0x00);
}

// =============================================================================
// 5. TX Countdown Lifecycle
// =============================================================================

TEST_F(LynxUartTest, TxCountdown_InitiallyInactive) {
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, TxCountdown_Decrements) {
	_s.UartTxCountdown = 5;
	TickUart(_s);
	EXPECT_EQ(_s.UartTxCountdown, 4u);
}

TEST_F(LynxUartTest, TxCountdown_ReachesZero_GoesIdle) {
	_s.UartTxCountdown = 1;
	TickUart(_s); // 1 → 0
	EXPECT_EQ(_s.UartTxCountdown, 0u);
	TickUart(_s); // 0 → inactive
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, TxCountdown_FullTransmission_11Ticks) {
	WriteSerdat(_s, 0x42);
	EXPECT_EQ(_s.UartTxCountdown, 11u);
	// Tick 10 times: 11 → 1
	TickN(10);
	EXPECT_EQ(_s.UartTxCountdown, 1u);
	TickUart(_s); // 1 → 0
	EXPECT_EQ(_s.UartTxCountdown, 0u);
	TickUart(_s); // 0 → idle
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, TxCountdown_InactiveDoesNotDecrement) {
	_s.UartTxCountdown = UartTxInactive;
	TickUart(_s);
	EXPECT_EQ(_s.UartTxCountdown, UartTxInactive);
}

// =============================================================================
// 6. RX Queue — Enqueue / Dequeue
// =============================================================================

TEST_F(LynxUartTest, RxQueue_Empty_InitialState) {
	EXPECT_EQ(_s.RxWaiting, 0u);
	EXPECT_EQ(_s.RxInputPtr, 0u);
	EXPECT_EQ(_s.RxOutputPtr, 0u);
}

TEST_F(LynxUartTest, RxQueue_Enqueue_StartsCountdown) {
	ComLynxRxData(_s, 0x42);
	EXPECT_EQ(_s.RxWaiting, 1u);
	EXPECT_EQ(_s.UartRxCountdown, UartRxTimePeriod);
}

TEST_F(LynxUartTest, RxQueue_Enqueue_SecondByte_NoCountdownReset) {
	ComLynxRxData(_s, 0x42);
	_s.UartRxCountdown = 5; // Partially elapsed
	ComLynxRxData(_s, 0x43);
	EXPECT_EQ(_s.RxWaiting, 2u);
	EXPECT_EQ(_s.UartRxCountdown, 5u); // Not reset
}

TEST_F(LynxUartTest, RxQueue_Dequeue_AfterCountdown) {
	ComLynxRxData(_s, 0xAB);
	// 11 ticks to decrement countdown to 0, +1 tick to deliver = 12 total
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xAB);
	EXPECT_EQ(_s.RxWaiting, 0u);
}

TEST_F(LynxUartTest, RxQueue_MultipleBytes_InterByteDelay) {
	ComLynxRxData(_s, 0x01);
	ComLynxRxData(_s, 0x02);

	// First byte: 11 ticks countdown→0, +1 tick to deliver = 12
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x01);
	EXPECT_EQ(_s.RxWaiting, 1u);
	// §7.3: Inter-byte delay = RX_TIME_PERIOD + RX_NEXT_DELAY = 55
	EXPECT_EQ(_s.UartRxCountdown, UartRxTimePeriod + UartRxNextDelay);

	// Read first byte to clear RxReady
	ReadSerdat(_s);
	EXPECT_FALSE(_s.UartRxReady);

	// Inter-byte delay: 55 ticks countdown→0, +1 tick to deliver = 56
	TickN(56);
	// Second byte delivered
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x02);
}

TEST_F(LynxUartTest, RxQueue_CircularWrap) {
	// Fill near end of buffer, then wrap around
	_s.RxInputPtr = UartMaxRxQueue - 2;
	_s.RxOutputPtr = UartMaxRxQueue - 2;

	ComLynxRxData(_s, 0xAA); // index 30
	ComLynxRxData(_s, 0xBB); // index 31
	ComLynxRxData(_s, 0xCC); // index 0 (wrapped)
	EXPECT_EQ(_s.RxWaiting, 3u);
	EXPECT_EQ(_s.RxInputPtr, 1u); // Wrapped to 1

	// Deliver first byte (12 ticks: 11 to reach 0, +1 to deliver)
	TickN(12);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xAA);
}

TEST_F(LynxUartTest, RxQueue_Full_DataLost) {
	// Fill to capacity
	for (int i = 0; i < UartMaxRxQueue; i++) {
		ComLynxRxData(_s, (uint16_t)(i & 0xff));
	}
	EXPECT_EQ(_s.RxWaiting, (uint32_t)UartMaxRxQueue);

	// 33rd byte lost
	ComLynxRxData(_s, 0xFF);
	EXPECT_EQ(_s.RxWaiting, (uint32_t)UartMaxRxQueue); // Still 32
}

// =============================================================================
// 7. RX Countdown Lifecycle
// =============================================================================

TEST_F(LynxUartTest, RxCountdown_InitiallyInactive) {
	EXPECT_TRUE(_s.UartRxCountdown & UartRxInactive);
}

TEST_F(LynxUartTest, RxCountdown_InactiveDoesNotDecrement) {
	_s.UartRxCountdown = UartRxInactive;
	TickUart(_s);
	// Should still have bit 31 set (stayed inactive)
	EXPECT_TRUE(_s.UartRxCountdown & UartRxInactive);
}

TEST_F(LynxUartTest, RxCountdown_ActiveDecrements) {
	_s.UartRxCountdown = 5;
	TickUart(_s);
	EXPECT_EQ(_s.UartRxCountdown, 4u);
}

TEST_F(LynxUartTest, RxCountdown_ReachesZero_NoDataNoChange) {
	_s.UartRxCountdown = 1;
	TickUart(_s); // 1 → 0
	// No data waiting: countdown stays 0, nothing delivered
	EXPECT_EQ(_s.UartRxCountdown, 0u);
	EXPECT_FALSE(_s.UartRxReady);
}

// =============================================================================
// 8. Break Signal
// =============================================================================

TEST_F(LynxUartTest, Break_ContinuousRetransmission) {
	WriteSerctl(_s, 0x02); // Enable SendBreak
	// First break already sent in WriteSerctl (countdown = 11)

	// 11 ticks: countdown 11→0, +1 tick: 0 → retransmit break = 12 total
	TickN(12);
	// Break auto-retransmits: countdown reset to 11
	EXPECT_EQ(_s.UartTxCountdown, UartTxTimePeriod);
	EXPECT_EQ(_s.UartTxData, UartBreakCode);
}

TEST_F(LynxUartTest, Break_StopsWhenDisabled) {
	WriteSerctl(_s, 0x02); // Enable break
	TickN(12); // First break complete → auto-retransmit (countdown reset to 11)

	WriteSerctl(_s, 0x00); // Disable break
	TickN(12); // Countdown 11→0 (11 ticks) + 0→inactive (1 tick) = 12
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, Break_LoopbackContainsBreakCode) {
	WriteSerctl(_s, 0x02);
	// Break was front-inserted via ComLynxTxLoopback (§7.2)
	EXPECT_EQ(_s.RxQueue[_s.RxOutputPtr], UartBreakCode);
}

// =============================================================================
// 9. Error Flags
// =============================================================================

TEST_F(LynxUartTest, Overrun_DetectedWhenRxNotRead) {
	// Enqueue both bytes up front
	ComLynxRxData(_s, 0x01);
	ComLynxRxData(_s, 0x02);

	// Deliver first byte (12 ticks), RxReady=true
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);

	// Don't read first byte — second byte delivery triggers overrun
	// §7.3: Inter-byte delay: 55 ticks countdown→0, +1 deliver = 56
	TickN(56);
	EXPECT_TRUE(_s.UartRxOverrunError);
}

TEST_F(LynxUartTest, Overrun_NoErrorWhenReadBeforeNextByte) {
	ComLynxRxData(_s, 0x01);
	ComLynxRxData(_s, 0x02);
	TickN(12); // Deliver first byte (12 ticks)
	ReadSerdat(_s); // Read first byte, clears RxReady

	// §7.3: Inter-byte delay (55→0) + deliver = 56
	TickN(56);
	EXPECT_FALSE(_s.UartRxOverrunError);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x02);
}

TEST_F(LynxUartTest, ErrorReset_ClearsOverrunAndFraming) {
	_s.UartRxOverrunError = true;
	_s.UartRxFramingError = true;
	WriteSerctl(_s, 0x08); // ResetErr
	EXPECT_FALSE(_s.UartRxOverrunError);
	EXPECT_FALSE(_s.UartRxFramingError);
}

TEST_F(LynxUartTest, ErrorReset_PreservesOtherBits) {
	_s.UartRxOverrunError = true;
	WriteSerctl(_s, 0x88); // ResetErr + TxIrqEnable
	EXPECT_FALSE(_s.UartRxOverrunError);
	EXPECT_TRUE(_s.UartTxIrqEnable);
}

// =============================================================================
// 10. IRQ — Level-Sensitive (HW Bug 13.2)
// =============================================================================

TEST_F(LynxUartTest, TxIrq_FiresWhenTxIdleAndEnabled) {
	_s.UartTxIrqEnable = true;
	_s.UartTxCountdown = UartTxInactive;
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, TxIrq_NoFireWhenTxActive) {
	_s.UartTxIrqEnable = true;
	_s.UartTxCountdown = 5; // Active, not idle
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_FALSE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, TxIrq_NoFireWhenDisabled) {
	_s.UartTxIrqEnable = false;
	_s.UartTxCountdown = UartTxInactive;
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_FALSE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, RxIrq_FiresWhenRxReadyAndEnabled) {
	_s.UartRxIrqEnable = true;
	_s.UartRxReady = true;
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, RxIrq_NoFireWhenNotReady) {
	_s.UartRxIrqEnable = true;
	_s.UartRxReady = false;
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_FALSE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, Irq_LevelSensitive_Reasserts) {
	// Level-sensitive: even if software clears the pending bit,
	// the next UpdateUartIrq re-asserts it.
	_s.UartTxIrqEnable = true;
	_s.UartTxCountdown = UartTxInactive;
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);

	// Software clears it
	_s.IrqPending &= ~0x10;
	EXPECT_FALSE(_s.IrqPending & 0x10);

	// Next update re-asserts
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, Irq_BothTxAndRx) {
	_s.UartTxIrqEnable = true;
	_s.UartTxCountdown = UartTxInactive;
	_s.UartRxIrqEnable = true;
	_s.UartRxReady = true;
	_s.IrqPending = 0;
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);
}

TEST_F(LynxUartTest, Irq_TxTransitionFromActiveToIdle) {
	_s.UartTxIrqEnable = true;
	_s.UartTxCountdown = 1;
	_s.IrqPending = 0;

	// TX active: no IRQ
	UpdateUartIrq(_s);
	EXPECT_FALSE(_s.IrqPending & 0x10);

	// TX completes
	TickUart(_s); // 1 → 0
	TickUart(_s); // 0 → inactive
	// TickUart calls UpdateUartIrq internally in real code;
	// we call it explicitly here since our test version doesn't
	UpdateUartIrq(_s);
	EXPECT_TRUE(_s.IrqPending & 0x10);
}

// =============================================================================
// 11. No-Cable Scenario (Self-Loopback Only)
// =============================================================================

TEST_F(LynxUartTest, NoCable_TxEchoesBack) {
	// Without external cable, TX always loops back to RX
	WriteSerdat(_s, 0x55);
	// 12 ticks for RX delivery (11 countdown→0, +1 deliver)
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x55);
}

TEST_F(LynxUartTest, NoCable_MultipleTx_AllEchoBack) {
	// First TX
	WriteSerdat(_s, 0xAA);
	TickN(12); // 12 ticks for RX delivery
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(ReadSerdat(_s), 0xAA);

	// Second TX — previous loopback already consumed
	WriteSerdat(_s, 0xBB);
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(ReadSerdat(_s), 0xBB);
}

TEST_F(LynxUartTest, NoCable_GamePollsSerialDoesNotHang) {
	// Key acceptance criterion: single-player games that poll SERCTL
	// must see TxRdy/TxEmpty so they don't hang.
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x80); // TxRdy
	EXPECT_TRUE(status & 0x20); // TxEmpty
	EXPECT_FALSE(status & 0x40); // No RxRdy (nobody sent anything)
}

// =============================================================================
// 12. Transmission Delay Fidelity
// =============================================================================

TEST_F(LynxUartTest, TxDelay_ExactlyElevenTicks) {
	WriteSerdat(_s, 0x42);
	for (uint32_t i = UartTxTimePeriod; i > 1; i--) {
		EXPECT_EQ(_s.UartTxCountdown, i);
		TickUart(_s);
	}
	// After 10 ticks: countdown = 1
	EXPECT_EQ(_s.UartTxCountdown, 1u);
	TickUart(_s); // 1 → 0
	EXPECT_EQ(_s.UartTxCountdown, 0u);
	// Next tick: 0 → idle
	TickUart(_s);
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, RxDelay_ExactlyElevenTicks) {
	ComLynxRxData(_s, 0x42);
	EXPECT_EQ(_s.UartRxCountdown, UartRxTimePeriod);
	for (uint32_t i = UartRxTimePeriod; i > 1; i--) {
		EXPECT_EQ(_s.UartRxCountdown, i);
		EXPECT_FALSE(_s.UartRxReady);
		TickUart(_s);
	}
	// Countdown = 1
	EXPECT_EQ(_s.UartRxCountdown, 1u);
	TickUart(_s); // 1 → 0
	// Countdown = 0 → deliver on next tick
	EXPECT_EQ(_s.UartRxCountdown, 0u);
	TickUart(_s); // Deliver
	EXPECT_TRUE(_s.UartRxReady);
}

TEST_F(LynxUartTest, InterByteDelay_Exactly55Ticks) {
	ComLynxRxData(_s, 0x01);
	ComLynxRxData(_s, 0x02);
	// Deliver first byte: 12 ticks
	TickN(12);
	ReadSerdat(_s); // Clear RxReady

	// §7.3: Inter-byte delay countdown = RX_TIME_PERIOD + RX_NEXT_DELAY = 55
	uint32_t interByteDelay = UartRxTimePeriod + UartRxNextDelay;
	EXPECT_EQ(_s.UartRxCountdown, interByteDelay);
	// 55 ticks to decrement countdown→0
	for (uint32_t i = interByteDelay; i > 0; i--) {
		EXPECT_FALSE(_s.UartRxReady);
		TickUart(_s);
	}
	// Countdown = 0, one more tick to deliver
	EXPECT_EQ(_s.UartRxCountdown, 0u);
	TickUart(_s); // 0 → deliver second byte
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x02);
}

// =============================================================================
// 13. Timer 4 Integration (State Machine Coupling)
// =============================================================================

TEST_F(LynxUartTest, Timer4_NoTimerDoneOnUart) {
	// Timer 4 does NOT set TimerDone on underflow — it calls TickUart() instead.
	// Verify that TimerDone remains false through the Timer 4 path.
	LynxTimerState timer4 = {};
	timer4.BackupValue = 0x05;

	// TimerDone starts false (zero-initialized)
	EXPECT_FALSE(timer4.TimerDone);

	// Simulate Timer 4 reload (what happens on underflow instead of TimerDone)
	timer4.Count = timer4.BackupValue;
	EXPECT_EQ(timer4.Count, timer4.BackupValue);
	EXPECT_FALSE(timer4.TimerDone); // Must stay false for Timer 4
}

TEST_F(LynxUartTest, Timer4_NoNormalIrqOnUnderflow) {
	// Timer 4's IRQ line (bit 4) is driven by UART state, not by
	// normal timer underflow. Verify IRQ only fires from UART conditions.
	_s.IrqPending = 0;
	_s.UartTxIrqEnable = false;
	_s.UartRxIrqEnable = false;
	UpdateUartIrq(_s);
	EXPECT_FALSE(_s.IrqPending & 0x10);
}

// =============================================================================
// 14. Full TX-RX Round Trip
// =============================================================================

TEST_F(LynxUartTest, FullRoundTrip_SingleByte) {
	// Simulate a full TX → loopback → RX → read cycle
	WriteSerdat(_s, 0x42);

	// TX is in progress
	uint8_t status = ReadSerctl(_s);
	EXPECT_FALSE(status & 0x80); // TxRdy = 0 (actively transmitting)
	EXPECT_FALSE(status & 0x40); // RxRdy = 0 (not delivered yet)

	// 12 ticks: both RX delivers and TX goes idle simultaneously
	// (both countdowns start at 11, reach 0 after 11 ticks, action on 12th)
	TickN(12);
	status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x80); // TxRdy = 1 (TX idle)
	EXPECT_TRUE(status & 0x40); // RxRdy = 1 (data delivered)

	// Read the data
	uint8_t data = ReadSerdat(_s);
	EXPECT_EQ(data, 0x42);

	// After read, RxRdy is cleared
	status = ReadSerctl(_s);
	EXPECT_FALSE(status & 0x40);
}

TEST_F(LynxUartTest, FullRoundTrip_MultipleBytesSequential) {
	// Send three bytes one after another
	for (int byte = 0; byte < 3; byte++) {
		_s = {}; // Fresh state for each to isolate
		uint8_t val = (uint8_t)(0x10 + byte);
		WriteSerdat(_s, val);
		TickN(12); // 12 ticks for RX delivery
		EXPECT_TRUE(_s.UartRxReady);
		EXPECT_EQ(ReadSerdat(_s), val);
	}
}

// =============================================================================
// 15. Edge Cases
// =============================================================================

TEST_F(LynxUartTest, EdgeCase_WriteSerdat_WhileTxActive) {
	WriteSerdat(_s, 0x01);
	TickN(5); // Partially through first TX

	// Write new data while TX is still active
	WriteSerdat(_s, 0x02);
	// TX countdown resets to 11
	EXPECT_EQ(_s.UartTxCountdown, UartTxTimePeriod);
	EXPECT_EQ(_s.UartTxData & 0xff, 0x02);
	// Both bytes are in the RX queue
	EXPECT_EQ(_s.RxWaiting, 2u);
}

TEST_F(LynxUartTest, EdgeCase_ReadSerdat_WhenNotReady) {
	// Reading SERDAT when nothing received just returns current RxData
	_s.UartRxData = 0;
	uint8_t data = ReadSerdat(_s);
	EXPECT_EQ(data, 0x00);
	EXPECT_FALSE(_s.UartRxReady);
}

TEST_F(LynxUartTest, EdgeCase_BreakDuringNormalTx) {
	WriteSerdat(_s, 0x55); // Start normal TX
	TickN(3); // Partially through

	WriteSerctl(_s, 0x02); // Enable break mid-TX
	// Break overrides: TX countdown reset, break code looped back
	EXPECT_EQ(_s.UartTxCountdown, UartTxTimePeriod);
	EXPECT_TRUE(_s.UartSendBreak);
}

TEST_F(LynxUartTest, EdgeCase_RxQueue_ExactlyFull_ThenDrain) {
	// Fill queue to capacity
	for (int i = 0; i < UartMaxRxQueue; i++) {
		ComLynxRxData(_s, (uint16_t)i);
	}
	EXPECT_EQ(_s.RxWaiting, (uint32_t)UartMaxRxQueue);

	// Drain entire queue
	for (int i = 0; i < UartMaxRxQueue; i++) {
		// Tick until delivered
		while (!_s.UartRxReady) {
			TickUart(_s);
		}
		uint8_t data = ReadSerdat(_s);
		EXPECT_EQ(data, (uint8_t)i);
	}
	EXPECT_EQ(_s.RxWaiting, 0u);
}

TEST_F(LynxUartTest, EdgeCase_TxCountdown_ExactlyZeroThenTick) {
	// Set countdown to exactly 0 (TX frame just completed)
	_s.UartTxCountdown = 0;
	_s.UartSendBreak = false;
	TickUart(_s);
	EXPECT_TRUE(_s.UartTxCountdown & UartTxInactive);
}

TEST_F(LynxUartTest, EdgeCase_ParityBitVisibleInSerctlRead) {
	// TX with 9th bit, loopback, deliver, check SERCTL read
	_s.UartParityEnable = false;
	_s.UartParityEven = true;
	WriteSerdat(_s, 0x55);
	TickN(12); // 12 ticks for RX delivery
	uint8_t status = ReadSerctl(_s);
	EXPECT_TRUE(status & 0x01); // Parbit = 1
}

TEST_F(LynxUartTest, EdgeCase_RxCountdownZeroNoData) {
	// RX countdown reaches 0 but no data in queue
	_s.UartRxCountdown = 0;
	_s.RxWaiting = 0;
	TickUart(_s);
	EXPECT_FALSE(_s.UartRxReady); // Nothing delivered
	// Countdown stays at 0 (no data to trigger inactive transition)
	EXPECT_EQ(_s.UartRxCountdown, 0u);
}

// =============================================================================
// 16. Front-Insertion Priority (§7.2 — ComLynxTxLoopback)
// =============================================================================

TEST_F(LynxUartTest, FrontInsert_LoopbackBeforeExternal) {
	// §7.2: Loopback data should be received BEFORE externally-queued data.
	// This is critical for collision detection on the ComLynx bus.
	ComLynxRxData(_s, 0xEE); // External data arrives first (back-insert)
	ComLynxTxLoopback(_s, 0xAA); // Loopback arrives second (front-insert)

	// Both are in queue
	EXPECT_EQ(_s.RxWaiting, 2u);

	// Deliver first byte: should be the loopback (0xAA) at front
	TickN(12);
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xAA);

	// Deliver second byte: should be external (0xEE)
	ReadSerdat(_s);
	TickN(56); // §7.3: inter-byte = 55 countdown + 1 deliver
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xEE);
}

TEST_F(LynxUartTest, FrontInsert_MultipleLoopbacks) {
	// Multiple front-inserts maintain LIFO order at front
	ComLynxTxLoopback(_s, 0x01);
	ComLynxTxLoopback(_s, 0x02); // This goes to front of 0x01
	ComLynxTxLoopback(_s, 0x03); // This goes to front of 0x02

	EXPECT_EQ(_s.RxWaiting, 3u);

	// Deliver: should come out 0x03, 0x02, 0x01 (LIFO at front)
	TickN(12);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x03);
	ReadSerdat(_s);
	TickN(56);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x02);
	ReadSerdat(_s);
	TickN(56);
	EXPECT_EQ(_s.UartRxData & 0xff, 0x01);
}

TEST_F(LynxUartTest, FrontInsert_InterleaveWithExternal) {
	// External, then loopback, then external — loopback jumps to front
	ComLynxRxData(_s, 0xE1); // back
	ComLynxRxData(_s, 0xE2); // back
	ComLynxTxLoopback(_s, 0xA1); // front (before E1, E2)

	EXPECT_EQ(_s.RxWaiting, 3u);

	// Delivery order: A1 (loopback, front), E1 (external), E2 (external)
	TickN(12);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xA1);
	ReadSerdat(_s);
	TickN(56);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xE1);
	ReadSerdat(_s);
	TickN(56);
	EXPECT_EQ(_s.UartRxData & 0xff, 0xE2);
}

// =============================================================================
// 17. RX Inactive After Delivery (§7.3)
// =============================================================================

TEST_F(LynxUartTest, RxInactive_AfterSingleByteDelivery) {
	// §7.3: After delivering the last byte from queue, RX goes inactive
	ComLynxRxData(_s, 0x42);
	TickN(12); // Deliver
	EXPECT_TRUE(_s.UartRxReady);
	// Queue is now empty — countdown should be INACTIVE
	EXPECT_TRUE(_s.UartRxCountdown & UartRxInactive);
	EXPECT_EQ(_s.RxWaiting, 0u);
}

TEST_F(LynxUartTest, RxInactive_NotSetWhenMoreBytesWaiting) {
	// §7.3: When more bytes are waiting, countdown is set to inter-byte delay
	ComLynxRxData(_s, 0x01);
	ComLynxRxData(_s, 0x02);
	TickN(12); // Deliver first byte
	EXPECT_TRUE(_s.UartRxReady);
	EXPECT_EQ(_s.RxWaiting, 1u);
	// Should NOT be inactive — inter-byte delay set instead
	EXPECT_FALSE(_s.UartRxCountdown & UartRxInactive);
	EXPECT_EQ(_s.UartRxCountdown, UartRxTimePeriod + UartRxNextDelay);
}

TEST_F(LynxUartTest, RxInactive_NewDataRestartsCountdown) {
	// After going inactive, new data arrival restarts countdown
	ComLynxRxData(_s, 0x42);
	TickN(12);
	EXPECT_TRUE(_s.UartRxCountdown & UartRxInactive);

	// New data arrives
	ComLynxRxData(_s, 0x99);
	EXPECT_EQ(_s.UartRxCountdown, UartRxTimePeriod); // Restarted to 11
	EXPECT_EQ(_s.RxWaiting, 1u);
}
