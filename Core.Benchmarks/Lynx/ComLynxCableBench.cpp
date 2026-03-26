#include "pch.h"
#include "Lynx/LynxTypes.h"

// =============================================================================
// ComLynx Cable Benchmarks
// =============================================================================
// Measures the overhead of the ComLynxCable broadcast mechanism and the
// impact of the null-cable check in ComLynxTxLoopback().
//
// The cable check (if (_comLynxCable) [[unlikely]]) is on the TX path,
// called once per byte transmitted. During normal ComLynx games this is
// ~9600 baud = 960 bytes/sec, so it's a cold path. But during gameplay
// without serial activity, the check should be essentially free (branch
// predicted not-taken).

// Replicated constants from LynxMikey.h
static constexpr uint32_t UartTxInactive = 0x80000000;
static constexpr uint32_t UartRxInactive = 0x80000000;
static constexpr uint16_t UartBreakCode = 0x8000;
static constexpr int UartMaxRxQueue = 32;
static constexpr uint32_t UartTxTimePeriod = 11;
static constexpr uint32_t UartRxTimePeriod = 11;
static constexpr uint32_t UartRxNextDelay = 44;

// Standalone UART state for benchmarking
struct CableBenchUnit {
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

// Replicated ComLynxRxData (back-insert)
static void BenchComLynxRxData(CableBenchUnit& s, uint16_t data) {
	if (s.RxWaiting < (uint32_t)UartMaxRxQueue) {
		if (s.RxWaiting == 0) {
			s.UartRxCountdown = UartRxTimePeriod;
		}
		s.RxQueue[s.RxInputPtr] = data;
		s.RxInputPtr = (s.RxInputPtr + 1) & (UartMaxRxQueue - 1);
		s.RxWaiting++;
	}
}

// Replicated ComLynxTxLoopback (front-insert / self-loopback)
static void BenchComLynxTxLoopback(CableBenchUnit& s, uint16_t data) {
	if (s.RxWaiting < (uint32_t)UartMaxRxQueue) {
		if (s.RxWaiting == 0) {
			s.UartRxCountdown = UartRxTimePeriod;
		}
		s.RxOutputPtr = (s.RxOutputPtr - 1) & (UartMaxRxQueue - 1);
		s.RxQueue[s.RxOutputPtr] = data;
		s.RxWaiting++;
	}
}

// Replicated cable broadcast (iterates connected units)
static void BenchBroadcast(CableBenchUnit* sender, CableBenchUnit* units, int unitCount, uint16_t data) {
	for (int i = 0; i < unitCount; i++) {
		if (&units[i] != sender) {
			BenchComLynxRxData(units[i], data);
		}
	}
}

// Replicated TickUart logic
static void BenchTickUart(CableBenchUnit& s) {
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

// =============================================================================
// BASELINE: Null cable check cost (no cable attached)
// =============================================================================

// Simulates the [[unlikely]] branch in ComLynxTxLoopback when no cable is set.
// This is the common single-player path — must be near-zero overhead.
static void BM_ComLynxCable_NullCheck_NoCable(benchmark::State& state) {
	CableBenchUnit sender = {};
	CableBenchUnit* cable = nullptr;

	for (auto _ : state) {
		// Self-loopback always happens
		BenchComLynxTxLoopback(sender, 0x42);

		// The null check (this is what we're measuring)
		if (cable) [[unlikely]] {
			// Would broadcast here — but cable is null
		}

		// Reset for next iteration
		sender.RxWaiting = 0;
		sender.RxOutputPtr = 0;
		sender.RxInputPtr = 0;
		benchmark::DoNotOptimize(sender);
		benchmark::DoNotOptimize(cable);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_NullCheck_NoCable);

// =============================================================================
// CABLE: Broadcast to 1 other unit (2-player)
// =============================================================================

static void BM_ComLynxCable_Broadcast_2Units(benchmark::State& state) {
	CableBenchUnit units[2] = {};

	for (auto _ : state) {
		// Sender transmits + self-loopback
		BenchComLynxTxLoopback(units[0], 0x42);

		// Broadcast to other unit
		BenchBroadcast(&units[0], units, 2, 0x42);

		// Reset
		for (auto& u : units) {
			u.RxWaiting = 0;
			u.RxOutputPtr = 0;
			u.RxInputPtr = 0;
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_Broadcast_2Units);

// =============================================================================
// CABLE: Broadcast to 3 other units (4-player)
// =============================================================================

static void BM_ComLynxCable_Broadcast_4Units(benchmark::State& state) {
	CableBenchUnit units[4] = {};

	for (auto _ : state) {
		BenchComLynxTxLoopback(units[0], 0x42);
		BenchBroadcast(&units[0], units, 4, 0x42);

		for (auto& u : units) {
			u.RxWaiting = 0;
			u.RxOutputPtr = 0;
			u.RxInputPtr = 0;
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_Broadcast_4Units);

// =============================================================================
// CABLE: Broadcast to 7 other units (8-player — stress test)
// =============================================================================

static void BM_ComLynxCable_Broadcast_8Units(benchmark::State& state) {
	CableBenchUnit units[8] = {};

	for (auto _ : state) {
		BenchComLynxTxLoopback(units[0], 0x42);
		BenchBroadcast(&units[0], units, 8, 0x42);

		for (auto& u : units) {
			u.RxWaiting = 0;
			u.RxOutputPtr = 0;
			u.RxInputPtr = 0;
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_Broadcast_8Units);

// =============================================================================
// FULL TX PATH: Loopback + Broadcast + TickUart (2-player)
// =============================================================================

// Full TX→RX path for 2-player: TX on unit 0, verify both receive
// Includes TickUart calls to process through the RX pipeline.
static void BM_ComLynxCable_FullTxPath_2Units(benchmark::State& state) {
	CableBenchUnit units[2] = {};

	for (auto _ : state) {
		// TX + loopback + broadcast
		BenchComLynxTxLoopback(units[0], 0x42);
		BenchBroadcast(&units[0], units, 2, 0x42);

		// Tick both through to RX delivery (11 ticks countdown + deliver)
		for (int t = 0; t < 12; t++) {
			BenchTickUart(units[0]);
			BenchTickUart(units[1]);
		}

		// Reset
		for (auto& u : units) {
			u = {};
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_FullTxPath_2Units);

// =============================================================================
// FULL TX PATH: Loopback + Broadcast + TickUart (4-player)
// =============================================================================

static void BM_ComLynxCable_FullTxPath_4Units(benchmark::State& state) {
	CableBenchUnit units[4] = {};

	for (auto _ : state) {
		BenchComLynxTxLoopback(units[0], 0x42);
		BenchBroadcast(&units[0], units, 4, 0x42);

		for (int t = 0; t < 12; t++) {
			for (auto& u : units) {
				BenchTickUart(u);
			}
		}

		for (auto& u : units) {
			u = {};
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_FullTxPath_4Units);

// =============================================================================
// COMPARISON: TX without cable vs TX with cable (overhead delta)
// =============================================================================

// TX with self-loopback only (no cable) — single-player baseline
static void BM_ComLynxCable_TxLoopbackOnly(benchmark::State& state) {
	CableBenchUnit sender = {};

	for (auto _ : state) {
		BenchComLynxTxLoopback(sender, 0x42);

		sender.RxWaiting = 0;
		sender.RxOutputPtr = 0;
		sender.RxInputPtr = 0;
		benchmark::DoNotOptimize(sender);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_TxLoopbackOnly);

// TX with loopback + broadcast to 1 other (2-player delta)
static void BM_ComLynxCable_TxLoopbackPlusBroadcast(benchmark::State& state) {
	CableBenchUnit units[2] = {};

	for (auto _ : state) {
		BenchComLynxTxLoopback(units[0], 0x42);
		BenchBroadcast(&units[0], units, 2, 0x42);

		for (auto& u : units) {
			u.RxWaiting = 0;
			u.RxOutputPtr = 0;
			u.RxInputPtr = 0;
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_TxLoopbackPlusBroadcast);

// =============================================================================
// BURST: Multiple bytes transmitted in sequence (protocol packet)
// =============================================================================

// Typical ComLynx protocol sends multi-byte packets.
// Measure cost of broadcasting a 16-byte packet across 2 units.
static void BM_ComLynxCable_BurstPacket_16Bytes_2Units(benchmark::State& state) {
	CableBenchUnit units[2] = {};

	for (auto _ : state) {
		for (int b = 0; b < 16; b++) {
			BenchComLynxTxLoopback(units[0], (uint16_t)b);
			BenchBroadcast(&units[0], units, 2, (uint16_t)b);
		}

		for (auto& u : units) {
			u = {};
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations() * 16);
}
BENCHMARK(BM_ComLynxCable_BurstPacket_16Bytes_2Units);

// Same but 4 units
static void BM_ComLynxCable_BurstPacket_16Bytes_4Units(benchmark::State& state) {
	CableBenchUnit units[4] = {};

	for (auto _ : state) {
		for (int b = 0; b < 16; b++) {
			BenchComLynxTxLoopback(units[0], (uint16_t)b);
			BenchBroadcast(&units[0], units, 4, (uint16_t)b);
		}

		for (auto& u : units) {
			u = {};
		}
		benchmark::DoNotOptimize(units);
	}
	state.SetItemsProcessed(state.iterations() * 16);
}
BENCHMARK(BM_ComLynxCable_BurstPacket_16Bytes_4Units);

// =============================================================================
// IDLE: TickUart with cable attached but no TX activity
// =============================================================================

// When cable is attached but no one is transmitting, TickUart overhead
// should be identical to no-cable (the cable pointer is never dereferenced).
static void BM_ComLynxCable_TickIdle_WithCable(benchmark::State& state) {
	CableBenchUnit sender = {};
	int cablePlaceholder = 1; // Non-null "cable" pointer analog

	for (auto _ : state) {
		BenchTickUart(sender);
		benchmark::DoNotOptimize(sender);
		benchmark::DoNotOptimize(cablePlaceholder);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ComLynxCable_TickIdle_WithCable);
