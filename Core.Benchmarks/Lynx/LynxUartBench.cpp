#include "pch.h"
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx UART / ComLynx Benchmarks
// =============================================================================
// TickUart() is called on every Timer 4 underflow. At 9600 baud (default),
// Timer 4 runs at ~62,500 Hz → ~62,500 TickUart calls/sec.
//
// Hot path: when both TX and RX are inactive (common case during gameplay
// with no serial activity), TickUart should be as cheap as two bit-tests.
//
// Cold paths: ComLynxRxData (queue enqueue), UpdateUartIrq (IRQ re-assert),
// SERCTL/SERDAT read/write (register access).

// Replicated constants from LynxMikey.h
static constexpr uint32_t UartTxInactive = 0x80000000;
static constexpr uint32_t UartRxInactive = 0x80000000;
static constexpr uint16_t UartBreakCode = 0x8000;
static constexpr int UartMaxRxQueue = 32;
static constexpr uint32_t UartTxTimePeriod = 11;
static constexpr uint32_t UartRxTimePeriod = 11;
static constexpr uint32_t UartRxNextDelay = 44;

// Standalone UART state for benchmarking
struct BenchUartState {
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

	uint16_t RxQueue[UartMaxRxQueue] = {};
	uint32_t RxInputPtr = 0;
	uint32_t RxOutputPtr = 0;
	uint32_t RxWaiting = 0;
};

// Replicated TickUart logic
static void BenchTickUart(BenchUartState& s) {
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
				s.UartRxCountdown = UartRxTimePeriod + UartRxNextDelay;
			} else {
				s.UartRxCountdown = UartRxInactive;
			}
		}
	} else if (!(s.UartRxCountdown & UartRxInactive)) {
		s.UartRxCountdown--;
	}

	if (s.UartTxCountdown == 0) {
		if (s.UartSendBreak) {
			s.UartTxData = UartBreakCode;
			s.UartTxCountdown = UartTxTimePeriod;
		} else {
			s.UartTxCountdown = UartTxInactive;
		}
	} else if (!(s.UartTxCountdown & UartTxInactive)) {
		s.UartTxCountdown--;
	}
}

// Replicated UpdateUartIrq logic
static void BenchUpdateUartIrq(BenchUartState& s) {
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
		s.IrqPending |= 0x10;
	}
}

// Replicated ComLynxRxData logic
static void BenchComLynxRxData(BenchUartState& s, uint16_t data) {
	if (s.RxWaiting < (uint32_t)UartMaxRxQueue) {
		if (s.RxWaiting == 0) {
			s.UartRxCountdown = UartRxTimePeriod;
		}
		s.RxQueue[s.RxInputPtr] = data;
		s.RxInputPtr = (s.RxInputPtr + 1) & (UartMaxRxQueue - 1);
		s.RxWaiting++;
	}
}

// Replicated SERCTL read
static uint8_t BenchReadSerctl(const BenchUartState& s) {
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

// =============================================================================
// HOT PATH: TickUart — Idle (most common case)
// =============================================================================

// The #1 hot path: both TX and RX inactive. This is the common
// state during single-player gameplay with no serial activity.
static void BM_LynxUart_TickIdle(benchmark::State& state) {
	BenchUartState s = {};
	// Both TX/RX are inactive (default state)

	for (auto _ : state) {
		BenchTickUart(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickIdle);

// =============================================================================
// HOT PATH: TickUart — TX Active (transmitting)
// =============================================================================

static void BM_LynxUart_TickTxActive(benchmark::State& state) {
	BenchUartState s = {};
	s.UartTxCountdown = UartTxTimePeriod;

	for (auto _ : state) {
		BenchTickUart(s);
		// Reset if completed to keep actively ticking
		if (s.UartTxCountdown & UartTxInactive) {
			s.UartTxCountdown = UartTxTimePeriod;
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickTxActive);

// =============================================================================
// HOT PATH: TickUart — RX Active (countdown decrementing)
// =============================================================================

static void BM_LynxUart_TickRxActive(benchmark::State& state) {
	BenchUartState s = {};
	s.UartRxCountdown = 100; // Mid-countdown

	for (auto _ : state) {
		BenchTickUart(s);
		// Reset if reached 0 and no data waiting
		if (s.UartRxCountdown == 0) {
			s.UartRxCountdown = 100;
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickRxActive);

// =============================================================================
// HOT PATH: TickUart — Both TX and RX Active
// =============================================================================

static void BM_LynxUart_TickBothActive(benchmark::State& state) {
	BenchUartState s = {};
	s.UartTxCountdown = UartTxTimePeriod;
	s.UartRxCountdown = 100;

	for (auto _ : state) {
		BenchTickUart(s);
		if (s.UartTxCountdown & UartTxInactive) {
			s.UartTxCountdown = UartTxTimePeriod;
		}
		if (s.UartRxCountdown == 0) {
			s.UartRxCountdown = 100;
		}
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickBothActive);

// =============================================================================
// HOT PATH: TickUart — RX Delivery (worst case per tick)
// =============================================================================

// Most expensive TickUart path: RX countdown hits 0 with data waiting.
// Involves queue dequeue, overrun check, inter-byte delay setup.
static void BM_LynxUart_TickRxDelivery(benchmark::State& state) {
	BenchUartState s = {};

	for (auto _ : state) {
		// Set up for delivery on this tick
		s.UartRxCountdown = 0;
		s.RxQueue[0] = 0x42;
		s.RxInputPtr = 1;
		s.RxOutputPtr = 0;
		s.RxWaiting = 1;
		s.UartRxReady = false;

		BenchTickUart(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickRxDelivery);

// =============================================================================
// HOT PATH: TickUart + UpdateUartIrq (real combined cost)
// =============================================================================

// In real code, TickUart() calls UpdateUartIrq() at the end.
// This measures the combined cost.
static void BM_LynxUart_TickPlusIrq_Idle(benchmark::State& state) {
	BenchUartState s = {};

	for (auto _ : state) {
		BenchTickUart(s);
		BenchUpdateUartIrq(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickPlusIrq_Idle);

static void BM_LynxUart_TickPlusIrq_TxIrqEnabled(benchmark::State& state) {
	BenchUartState s = {};
	s.UartTxIrqEnable = true;
	// TX is inactive → level-sensitive IRQ fires every tick

	for (auto _ : state) {
		BenchTickUart(s);
		BenchUpdateUartIrq(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_TickPlusIrq_TxIrqEnabled);

// =============================================================================
// COLD PATH: UpdateUartIrq alone
// =============================================================================

static void BM_LynxUart_UpdateIrq_NoIrq(benchmark::State& state) {
	BenchUartState s = {};
	// Both IRQ enables off, no conditions met

	for (auto _ : state) {
		BenchUpdateUartIrq(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_UpdateIrq_NoIrq);

static void BM_LynxUart_UpdateIrq_BothFiring(benchmark::State& state) {
	BenchUartState s = {};
	s.UartTxIrqEnable = true;
	s.UartRxIrqEnable = true;
	s.UartRxReady = true;
	// TX idle (inactive) → both conditions met

	for (auto _ : state) {
		BenchUpdateUartIrq(s);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_UpdateIrq_BothFiring);

// =============================================================================
// COLD PATH: ComLynxRxData (queue enqueue)
// =============================================================================

static void BM_LynxUart_RxEnqueue_EmptyQueue(benchmark::State& state) {
	BenchUartState s = {};

	for (auto _ : state) {
		s = {}; // Reset each time
		BenchComLynxRxData(s, 0x42);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_RxEnqueue_EmptyQueue);

static void BM_LynxUart_RxEnqueue_PartialQueue(benchmark::State& state) {
	BenchUartState s = {};
	// Pre-fill with some data
	for (int i = 0; i < 16; i++) {
		BenchComLynxRxData(s, (uint16_t)i);
	}
	uint32_t savedInputPtr = s.RxInputPtr;
	uint32_t savedWaiting = s.RxWaiting;

	for (auto _ : state) {
		// Reset to partial state
		s.RxInputPtr = savedInputPtr;
		s.RxWaiting = savedWaiting;
		BenchComLynxRxData(s, 0x42);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_RxEnqueue_PartialQueue);

static void BM_LynxUart_RxEnqueue_FullQueue(benchmark::State& state) {
	BenchUartState s = {};
	// Fill queue to capacity
	for (int i = 0; i < UartMaxRxQueue; i++) {
		BenchComLynxRxData(s, (uint16_t)i);
	}

	for (auto _ : state) {
		// Enqueue into full queue (silently lost)
		BenchComLynxRxData(s, 0xFF);
		benchmark::DoNotOptimize(s);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_RxEnqueue_FullQueue);

// =============================================================================
// COLD PATH: SERCTL Read (status register construction)
// =============================================================================

static void BM_LynxUart_ReadSerctl_Idle(benchmark::State& state) {
	BenchUartState s = {};

	for (auto _ : state) {
		uint8_t status = BenchReadSerctl(s);
		benchmark::DoNotOptimize(status);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_ReadSerctl_Idle);

static void BM_LynxUart_ReadSerctl_AllFlags(benchmark::State& state) {
	BenchUartState s = {};
	s.UartRxReady = true;
	s.UartRxOverrunError = true;
	s.UartRxFramingError = true;
	s.UartRxData = UartBreakCode | 0x0100;

	for (auto _ : state) {
		uint8_t status = BenchReadSerctl(s);
		benchmark::DoNotOptimize(status);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_ReadSerctl_AllFlags);

// =============================================================================
// COLD PATH: Queue Modulo (% vs & for power-of-2)
// =============================================================================

// The RX queue uses modulo 32 for circular indexing. Since 32 is a power
// of 2, the compiler can optimize % to & mask. Benchmark both to verify.
static void BM_LynxUart_QueueIndex_Modulo(benchmark::State& state) {
	uint32_t index = 0;

	for (auto _ : state) {
		index = (index + 1) % UartMaxRxQueue;
		benchmark::DoNotOptimize(index);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_QueueIndex_Modulo);

static void BM_LynxUart_QueueIndex_BitMask(benchmark::State& state) {
	uint32_t index = 0;
	static constexpr uint32_t mask = UartMaxRxQueue - 1;

	for (auto _ : state) {
		index = (index + 1) & mask;
		benchmark::DoNotOptimize(index);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_QueueIndex_BitMask);

// =============================================================================
// THROUGHPUT: Full TX→RX round trip
// =============================================================================

// Measures throughput of complete TX→loopback→countdown→delivery→read cycle
static void BM_LynxUart_FullRoundTrip(benchmark::State& state) {
	BenchUartState s = {};

	for (auto _ : state) {
		// TX: set data and start countdown
		s.UartTxData = 0x42;
		s.UartTxCountdown = UartTxTimePeriod;

		// Loopback → enqueue
		BenchComLynxRxData(s, s.UartTxData);

		// Tick through TX + RX (12 ticks for both to complete)
		for (int i = 0; i < 12; i++) {
			BenchTickUart(s);
		}

		// Read result
		uint8_t data = (uint8_t)(s.UartRxData & 0xff);
		s.UartRxReady = false;
		benchmark::DoNotOptimize(data);

		// Reset for next iteration
		s.UartTxCountdown = UartTxInactive;
		s.UartRxCountdown = UartRxInactive;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxUart_FullRoundTrip);
