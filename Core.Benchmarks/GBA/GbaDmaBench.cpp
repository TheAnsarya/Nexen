#include "pch.h"
#include <array>
#include <cstring>
#include "GBA/GbaTypes.h"

// =============================================================================
// GBA DMA Block Transfer Throughput Benchmarks
// =============================================================================
// The GBA (Game Boy Advance) uses DMA (Direct Memory Access) extensively.
// Games use DMA for:
//   - Copying sprite tiles to OAM/VRAM (DMA0, DMA1)
//   - Streaming audio to sound FIFOs (DMA1, DMA2 — most latency-critical)
//   - Bulk VRAM/IWRAM copies (DMA3 — up to 16KB per operation)
//   - Bitmap mode framebuffer updates (DMA3 — up to 38KB per frame)
//
// GbaDmaController runs 1 to 16,384 transfers per DMA activation.
// Each transfer is either 16-bit (halfword) or 32-bit (word).
//
// Key hot paths measured here:
//   1. Halfword (16-bit) DMA copy at various sizes (16B, 256B, 1KB, 16KB)
//   2. Word (32-bit) DMA copy at the same sizes
//   3. Comparison vs raw memcpy (the theoretical floor)
//   4. DMA channel priority arbitration (4-channel scan)
//   5. Channel enable / active state check
//   6. DMA repeat trigger logic (audio FIFO fill: 4 words, triggered per sample)
// =============================================================================

// ---------------------------------------------------------------------------
// 1. Halfword DMA Copy — Various Sizes
// ---------------------------------------------------------------------------
// GBA DMA in halfword mode copies 16-bit units. The GBA bus is 16-bit wide
// to most memory regions. Each transfer increments src and dst by 2 bytes.

// Helper: simulate a halfword DMA copy (matches GbaDmaController::RunHalfword)
static void DmaHalfwordCopy(uint16_t* dst, const uint16_t* src, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		dst[i] = src[i];
	}
}

// Helper: simulate a word (32-bit) DMA copy
static void DmaWordCopy(uint32_t* dst, const uint32_t* src, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		dst[i] = src[i];
	}
}

// --- 16-byte DMA copies ---

// Benchmark: Halfword DMA copy — 16 bytes (8 halfword units)
// Typical for small sprite attribute updates
static void BM_GbaDma_Halfword_16Bytes(benchmark::State& state) {
	alignas(32) uint16_t src[8] = {0xBEEF, 0xDEAD, 0xCAFE, 0xBABE, 0x1234, 0x5678, 0x9ABC, 0xDEF0};
	alignas(32) uint16_t dst[8] = {};

	for (auto _ : state) {
		DmaHalfwordCopy(dst, src, 8);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16);
}
BENCHMARK(BM_GbaDma_Halfword_16Bytes);

// Benchmark: Word DMA copy — 16 bytes (4 word units)
static void BM_GbaDma_Word_16Bytes(benchmark::State& state) {
	alignas(32) uint32_t src[4] = {0xBEEFDEAD, 0xCAFEBABE, 0x12345678, 0x9ABCDEF0};
	alignas(32) uint32_t dst[4] = {};

	for (auto _ : state) {
		DmaWordCopy(dst, src, 4);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16);
}
BENCHMARK(BM_GbaDma_Word_16Bytes);

// Benchmark: memcpy baseline — 16 bytes
static void BM_GbaDma_Memcpy_16Bytes(benchmark::State& state) {
	alignas(32) uint8_t src[16] = {0xBE, 0xEF, 0xDE, 0xAD, 0xCA, 0xFE, 0xBA, 0xBE,
	                               0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
	alignas(32) uint8_t dst[16] = {};

	for (auto _ : state) {
		memcpy(dst, src, 16);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16);
}
BENCHMARK(BM_GbaDma_Memcpy_16Bytes);

// --- 256-byte DMA copies ---

// Benchmark: Halfword DMA copy — 256 bytes (128 halfword units)
// Typical for OAM update (128 sprite entries × 2 bytes each for attribute words)
static void BM_GbaDma_Halfword_256Bytes(benchmark::State& state) {
	alignas(64) uint16_t src[128];
	alignas(64) uint16_t dst[128];
	for (int i = 0; i < 128; i++) src[i] = static_cast<uint16_t>(i * 257);

	for (auto _ : state) {
		DmaHalfwordCopy(dst, src, 128);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(BM_GbaDma_Halfword_256Bytes);

// Benchmark: Word DMA copy — 256 bytes (64 word units)
static void BM_GbaDma_Word_256Bytes(benchmark::State& state) {
	alignas(64) uint32_t src[64];
	alignas(64) uint32_t dst[64];
	for (int i = 0; i < 64; i++) src[i] = static_cast<uint32_t>(i * 0x01010101);

	for (auto _ : state) {
		DmaWordCopy(dst, src, 64);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(BM_GbaDma_Word_256Bytes);

// Benchmark: memcpy baseline — 256 bytes
static void BM_GbaDma_Memcpy_256Bytes(benchmark::State& state) {
	alignas(64) uint8_t src[256];
	alignas(64) uint8_t dst[256];
	for (int i = 0; i < 256; i++) src[i] = static_cast<uint8_t>(i);

	for (auto _ : state) {
		memcpy(dst, src, 256);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 256);
}
BENCHMARK(BM_GbaDma_Memcpy_256Bytes);

// --- 1KB DMA copies ---

// Benchmark: Halfword DMA copy — 1KB (512 halfword units)
// Common for VRAM tile update or OBJ char data
static void BM_GbaDma_Halfword_1KB(benchmark::State& state) {
	alignas(64) uint16_t src[512];
	alignas(64) uint16_t dst[512];
	for (int i = 0; i < 512; i++) src[i] = static_cast<uint16_t>(i * 257 + 1);

	for (auto _ : state) {
		DmaHalfwordCopy(dst, src, 512);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_GbaDma_Halfword_1KB);

// Benchmark: Word DMA copy — 1KB (256 word units)
static void BM_GbaDma_Word_1KB(benchmark::State& state) {
	alignas(64) uint32_t src[256];
	alignas(64) uint32_t dst[256];
	for (int i = 0; i < 256; i++) src[i] = static_cast<uint32_t>(i * 0x01030507);

	for (auto _ : state) {
		DmaWordCopy(dst, src, 256);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_GbaDma_Word_1KB);

// Benchmark: memcpy baseline — 1KB
static void BM_GbaDma_Memcpy_1KB(benchmark::State& state) {
	alignas(64) uint8_t src[1024];
	alignas(64) uint8_t dst[1024];
	for (int i = 0; i < 1024; i++) src[i] = static_cast<uint8_t>(i * 37);

	for (auto _ : state) {
		memcpy(dst, src, 1024);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_GbaDma_Memcpy_1KB);

// --- 16KB DMA copies ---

// Benchmark: Halfword DMA copy — 16KB (8192 halfword units)
// DMA3 maximum: covers entire SRAM or large VRAM regions
static void BM_GbaDma_Halfword_16KB(benchmark::State& state) {
	alignas(64) uint16_t src[8192];
	alignas(64) uint16_t dst[8192];
	for (int i = 0; i < 8192; i++) src[i] = static_cast<uint16_t>(i * 257);

	for (auto _ : state) {
		DmaHalfwordCopy(dst, src, 8192);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GbaDma_Halfword_16KB);

// Benchmark: Word DMA copy — 16KB (4096 word units)
static void BM_GbaDma_Word_16KB(benchmark::State& state) {
	alignas(64) uint32_t src[4096];
	alignas(64) uint32_t dst[4096];
	for (int i = 0; i < 4096; i++) src[i] = static_cast<uint32_t>(i * 0x01030507);

	for (auto _ : state) {
		DmaWordCopy(dst, src, 4096);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GbaDma_Word_16KB);

// Benchmark: memcpy baseline — 16KB
static void BM_GbaDma_Memcpy_16KB(benchmark::State& state) {
	alignas(64) uint8_t src[16384];
	alignas(64) uint8_t dst[16384];
	for (int i = 0; i < 16384; i++) src[i] = static_cast<uint8_t>(i * 37);

	for (auto _ : state) {
		memcpy(dst, src, 16384);
		benchmark::DoNotOptimize(dst[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16384);
}
BENCHMARK(BM_GbaDma_Memcpy_16KB);

// ---------------------------------------------------------------------------
// 2. DMA Channel Priority Arbitration
// ---------------------------------------------------------------------------
// GbaDmaController scans all 4 channels in priority order (0 = highest)
// to find the next active/pending channel. This scan happens every time
// a DMA transfer completes or a new trigger fires.
// With 4 channels and few active at once, this is a fast linear scan.

// Benchmark: No channels active — scan all 4 channels, find none
static void BM_GbaDma_ChannelScan_NoneActive(benchmark::State& state) {
	GbaDmaControllerState dmaState = {};
	// All channels disabled
	for (int i = 0; i < 4; i++) {
		dmaState.Ch[i].Enabled = false;
		dmaState.Ch[i].Active  = false;
		dmaState.Ch[i].Pending = false;
	}

	for (auto _ : state) {
		int activeChannel = -1;
		for (int i = 0; i < 4; i++) {
			if (dmaState.Ch[i].Active && dmaState.Ch[i].Enabled) {
				activeChannel = i;
				break;
			}
		}
		benchmark::DoNotOptimize(activeChannel);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbaDma_ChannelScan_NoneActive);

// Benchmark: Channel 3 active (lowest priority) — must scan all 4 channels
static void BM_GbaDma_ChannelScan_Channel3Active(benchmark::State& state) {
	GbaDmaControllerState dmaState = {};
	for (int i = 0; i < 3; i++) {
		dmaState.Ch[i].Enabled = false;
		dmaState.Ch[i].Active  = false;
	}
	dmaState.Ch[3].Enabled = true;
	dmaState.Ch[3].Active  = true;
	dmaState.Ch[3].LenLatch = 4096;

	for (auto _ : state) {
		int activeChannel = -1;
		for (int i = 0; i < 4; i++) {
			if (dmaState.Ch[i].Active && dmaState.Ch[i].Enabled) {
				activeChannel = i;
				break;
			}
		}
		benchmark::DoNotOptimize(activeChannel);
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbaDma_ChannelScan_Channel3Active);

// Benchmark: Channel 0 active (highest priority) — early-exit after first check
static void BM_GbaDma_ChannelScan_Channel0Active(benchmark::State& state) {
	GbaDmaControllerState dmaState = {};
	dmaState.Ch[0].Enabled = true;
	dmaState.Ch[0].Active  = true;
	dmaState.Ch[0].LenLatch = 256;

	for (auto _ : state) {
		int activeChannel = -1;
		for (int i = 0; i < 4; i++) {
			if (dmaState.Ch[i].Active && dmaState.Ch[i].Enabled) {
				activeChannel = i;
				break;
			}
		}
		benchmark::DoNotOptimize(activeChannel);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaDma_ChannelScan_Channel0Active);

// ---------------------------------------------------------------------------
// 3. Audio FIFO DMA — Small Repeat Transfer
// ---------------------------------------------------------------------------
// DMA1/DMA2 in audio FIFO mode transfer exactly 4 words (16 bytes) per
// sound-sample request. This fires ~18,000 times per second (32kHz sample
// rate / 4 samples per FIFO refill). It's the most frequent DMA activation.

// Benchmark: 4-word (16-byte) audio FIFO transfer (fires ~18K/second)
static void BM_GbaDma_AudioFifo_4Words(benchmark::State& state) {
	alignas(32) uint32_t src[4] = {0x01020304, 0x05060708, 0x09101112, 0x13141516};
	alignas(32) uint32_t fifoA[8] = {};  // Simulate FIFO A write buffer
	int fifoWritePos = 0;

	for (auto _ : state) {
		// Transfer 4 words to FIFO (circular buffer write)
		for (int i = 0; i < 4; i++) {
			fifoA[fifoWritePos & 7] = src[i];
			fifoWritePos++;
		}
		benchmark::DoNotOptimize(fifoA[0]);
		benchmark::ClobberMemory();
	}
	state.SetBytesProcessed(state.iterations() * 16);
}
BENCHMARK(BM_GbaDma_AudioFifo_4Words);

// ---------------------------------------------------------------------------
// 4. DMA State Machine — Enable / Trigger / Disable Cycle
// ---------------------------------------------------------------------------
// Measures the overhead of the full DMA channel state machine:
// Enable → trigger → transfer → disable, repeated for a typical audio DMA.

// Benchmark: DMA channel enable/disable cycle (overhead per DMA activation)
static void BM_GbaDma_StateMachine_EnableDisableCycle(benchmark::State& state) {
	GbaDmaChannel ch = {};
	ch.Trigger       = GbaDmaTrigger::HBlank;
	ch.WordTransfer  = true;
	ch.LenLatch      = 4;       // 4 words for audio FIFO
	ch.DestLatch     = 0x040000B0; // FIFO_A address
	ch.SrcLatch      = 0x02000000; // IWRAM source
	ch.DestMode      = GbaDmaAddrMode::Fixed; // FIFO: fixed destination

	for (auto _ : state) {
		// Simulate: set active, check, clear active
		ch.Active  = true;
		ch.Enabled = true;
		bool shouldTransfer = ch.Active && ch.Enabled && ch.LenLatch > 0;
		benchmark::DoNotOptimize(shouldTransfer);
		ch.Active = false;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaDma_StateMachine_EnableDisableCycle);

// Benchmark: DMA channel trigger check (called per-scanline for HBlank DMA)
// GBA runs HBlank DMA check once per scanline (160 times per frame).
static void BM_GbaDma_TriggerCheck_HBlank(benchmark::State& state) {
	GbaDmaControllerState dmaState = {};
	// Channel 0: audio DMA (HBlank), Channel 3: VRAM copy (VBlank)
	dmaState.Ch[0].Enabled = true;
	dmaState.Ch[0].Trigger  = GbaDmaTrigger::HBlank;
	dmaState.Ch[0].LenLatch = 4;

	dmaState.Ch[3].Enabled = true;
	dmaState.Ch[3].Trigger  = GbaDmaTrigger::VBlank;
	dmaState.Ch[3].LenLatch = 8192;

	for (auto _ : state) {
		// Simulate HBlank trigger: activate all channels with HBlank trigger
		for (int i = 0; i < 4; i++) {
			if (dmaState.Ch[i].Enabled &&
			    dmaState.Ch[i].Trigger == GbaDmaTrigger::HBlank) {
				dmaState.Ch[i].Active = true;
			}
		}
		benchmark::DoNotOptimize(dmaState.Ch[0].Active);
		// Reset for next iteration
		dmaState.Ch[0].Active = false;
	}
	state.SetItemsProcessed(state.iterations() * 4);
}
BENCHMARK(BM_GbaDma_TriggerCheck_HBlank);

// ---------------------------------------------------------------------------
// 5. DMA Address Mode Increment Logic
// ---------------------------------------------------------------------------
// Each DMA transfer requires incrementing (or not) the source and destination
// addresses based on GbaDmaAddrMode. This is evaluated per-word/halfword.
// Modes: Increment, Decrement, Fixed, IncrementReload

// Benchmark: Increment address mode (most common)
static void BM_GbaDma_AddrMode_Increment(benchmark::State& state) {
	uint32_t src = 0x02000000;
	uint32_t dst = 0x06000000;
	constexpr uint32_t step = 4; // word transfers: 4 bytes

	for (auto _ : state) {
		// Simulate per-word address update in Increment mode
		src += step;
		dst += step;
		benchmark::DoNotOptimize(src);
		benchmark::DoNotOptimize(dst);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaDma_AddrMode_Increment);

// Benchmark: Fixed destination (audio FIFO — destination never increments)
static void BM_GbaDma_AddrMode_FixedDst(benchmark::State& state) {
	uint32_t src = 0x02000000;
	uint32_t dst = 0x040000B0; // FIFO_A: fixed
	constexpr uint32_t step = 4;

	for (auto _ : state) {
		src += step;
		// dst stays fixed
		benchmark::DoNotOptimize(src);
		benchmark::DoNotOptimize(dst);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaDma_AddrMode_FixedDst);

// Benchmark: Address mode dispatch (branch on GbaDmaAddrMode per transfer)
static void BM_GbaDma_AddrMode_Dispatch(benchmark::State& state) {
	uint32_t addr = 0x06000000;
	constexpr uint32_t step = 4;
	GbaDmaAddrMode modes[] = {
		GbaDmaAddrMode::Increment,
		GbaDmaAddrMode::Decrement,
		GbaDmaAddrMode::Fixed,
		GbaDmaAddrMode::IncrementReload
	};
	int modeIdx = 0;

	for (auto _ : state) {
		GbaDmaAddrMode mode = modes[modeIdx & 3];
		switch (mode) {
			case GbaDmaAddrMode::Increment:       addr += step; break;
			case GbaDmaAddrMode::Decrement:       addr -= step; break;
			case GbaDmaAddrMode::Fixed:           break;
			case GbaDmaAddrMode::IncrementReload: addr += step; break;
		}
		benchmark::DoNotOptimize(addr);
		modeIdx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GbaDma_AddrMode_Dispatch);
