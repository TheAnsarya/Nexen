#include "pch.h"
#include <array>
#include <cstring>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx Mikey Benchmarks (Timers, Display, Palette, IRQ)
// =============================================================================
// Mikey's timer system is the hottest path in Lynx emulation. 8 timers are
// ticked every CPU cycle, and Timer 0/2 drive scanline/frame DMA.
// Display rendering converts 4bpp packed framebuffer to ARGB32 via palette.

// -----------------------------------------------------------------------------
// Timer Prescaler
// -----------------------------------------------------------------------------

// Benchmark timer prescaler period lookup (array vs switch)
static void BM_LynxMikey_PrescalerLookup_Array(benchmark::State& state) {
	// CPU-cycle periods: {4, 8, 16, 32, 64, 128, 256, 0}
	static constexpr uint16_t prescalerPeriods[8] = { 4, 8, 16, 32, 64, 128, 256, 0 };
	uint8_t source = 0;

	for (auto _ : state) {
		uint16_t period = prescalerPeriods[source & 0x07];
		benchmark::DoNotOptimize(period);
		source++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_PrescalerLookup_Array);

// Benchmark timer prescaler via shift computation (alternative implementation)
static void BM_LynxMikey_PrescalerLookup_Shift(benchmark::State& state) {
	uint8_t source = 0;

	for (auto _ : state) {
		// 4 << source gives: 4, 8, 16, 32, 64, 128, 256, 512
		// But source=7 should be 0 (disabled)
		uint16_t period = (source & 0x07) < 7 ? (4 << (source & 0x07)) : 0;
		benchmark::DoNotOptimize(period);
		source++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_PrescalerLookup_Shift);

// -----------------------------------------------------------------------------
// Timer Tick (Hot Path)
// -----------------------------------------------------------------------------

// Benchmark single timer tick (countdown + underflow check)
static void BM_LynxMikey_TickSingleTimer(benchmark::State& state) {
	LynxTimerState timer = {};
	timer.BackupValue = 100;
	timer.Count = 100;
	timer.ControlA = 0x80; // Enable bit set, source 0
	timer.TimerDone = false;
	uint8_t irqPending = 0;

	for (auto _ : state) {
		// Simulate timer countdown
		if (timer.ControlA & 0x80) { // Timer enabled
			if (!timer.TimerDone) {   // Bug 13.6: done blocks counting
				if (timer.Count == 0) {
					timer.Count = timer.BackupValue;
					timer.TimerDone = true;
					irqPending |= LynxIrqSource::Timer0;
				} else {
					timer.Count--;
				}
			}
		}
		benchmark::DoNotOptimize(timer.Count);
		benchmark::DoNotOptimize(irqPending);
		// Reset for next iteration to keep it interesting
		if (timer.TimerDone) {
			timer.TimerDone = false;
			timer.Count = timer.BackupValue;
		}
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_TickSingleTimer);

// Benchmark ticking all 8 timers (real per-cycle cost)
static void BM_LynxMikey_TickAll8Timers(benchmark::State& state) {
	LynxTimerState timers[8] = {};
	for (int i = 0; i < 8; i++) {
		timers[i].BackupValue = static_cast<uint8_t>(50 + i * 10);
		timers[i].Count = timers[i].BackupValue;
		timers[i].ControlA = 0x80; // Enabled, source 0
		timers[i].TimerDone = false;
	}
	uint8_t irqPending = 0;

	for (auto _ : state) {
		for (int i = 0; i < 8; i++) {
			if (timers[i].ControlA & 0x80) {
				if (!timers[i].TimerDone) {
					if (timers[i].Count == 0) {
						timers[i].Count = timers[i].BackupValue;
						timers[i].TimerDone = true;
						irqPending |= (1 << i);
					} else {
						timers[i].Count--;
					}
				}
			}
		}
		benchmark::DoNotOptimize(irqPending);
		benchmark::DoNotOptimize(timers);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_LynxMikey_TickAll8Timers);

// Benchmark timer cascade (linked timers — timer N counts timer N-1 underflows)
static void BM_LynxMikey_TimerCascade(benchmark::State& state) {
	// Timer 0 cascades into Timer 1 into Timer 2
	LynxTimerState timers[3] = {};
	for (int i = 0; i < 3; i++) {
		timers[i].BackupValue = 2;
		timers[i].Count = 2;
		timers[i].ControlA = 0x80;
		timers[i].Linked = (i > 0); // Timer 1,2 linked
		timers[i].TimerDone = false;
	}

	for (auto _ : state) {
		// Tick timer 0 and cascade
		if (timers[0].Count == 0) {
			timers[0].Count = timers[0].BackupValue;
			// Cascade to timer 1
			if (timers[1].Linked && !timers[1].TimerDone) {
				if (timers[1].Count == 0) {
					timers[1].Count = timers[1].BackupValue;
					// Cascade to timer 2
					if (timers[2].Linked && !timers[2].TimerDone) {
						if (timers[2].Count == 0) {
							timers[2].Count = timers[2].BackupValue;
						} else {
							timers[2].Count--;
						}
					}
				} else {
					timers[1].Count--;
				}
			}
		} else {
			timers[0].Count--;
		}
		benchmark::DoNotOptimize(timers);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_TimerCascade);

// -----------------------------------------------------------------------------
// IRQ Check (Per-Instruction)
// -----------------------------------------------------------------------------

// Benchmark IRQ line evaluation (called every instruction)
static void BM_LynxMikey_UpdateIrqLine(benchmark::State& state) {
	uint8_t irqPending = 0;
	uint8_t counter = 0;

	for (auto _ : state) {
		// Real implementation: no enable mask (removed per audit fix)
		bool irqAsserted = (irqPending != 0);
		benchmark::DoNotOptimize(irqAsserted);
		// Toggle some IRQ sources to vary workload
		if (counter & 0x01) irqPending |= LynxIrqSource::Timer0;
		if (counter & 0x02) irqPending &= ~LynxIrqSource::Timer0;
		if (counter & 0x04) irqPending |= LynxIrqSource::Timer2;
		counter++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_UpdateIrqLine);

// Compare: old broken IRQ check with enable mask (should show identical perf)
static void BM_LynxMikey_UpdateIrqLine_OldBroken(benchmark::State& state) {
	uint8_t irqPending = 0;
	uint8_t irqEnabled = 0; // Always 0 => never fires (the bug)
	uint8_t counter = 0;

	for (auto _ : state) {
		bool irqAsserted = (irqPending & irqEnabled) != 0;
		benchmark::DoNotOptimize(irqAsserted);
		if (counter & 0x01) irqPending |= LynxIrqSource::Timer0;
		if (counter & 0x02) irqPending &= ~LynxIrqSource::Timer0;
		if (counter & 0x04) irqPending |= LynxIrqSource::Timer2;
		counter++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_UpdateIrqLine_OldBroken);

// -----------------------------------------------------------------------------
// Palette Lookup
// -----------------------------------------------------------------------------

// Benchmark palette entry computation (Green/Blue/Red → ARGB32)
static void BM_LynxMikey_PaletteCompute(benchmark::State& state) {
	uint8_t paletteGreen[16] = {};
	uint8_t paletteBR[16] = {};
	uint32_t palette[16] = {};

	// Fill with test data
	for (int i = 0; i < 16; i++) {
		paletteGreen[i] = static_cast<uint8_t>(i);
		paletteBR[i] = static_cast<uint8_t>((i << 4) | i);
	}

	for (auto _ : state) {
		for (int i = 0; i < 16; i++) {
			uint8_t green4 = paletteGreen[i] & 0x0F;
			uint8_t blue4 = (paletteBR[i] >> 4) & 0x0F;
			uint8_t red4 = paletteBR[i] & 0x0F;
			// 4-bit to 8-bit expansion: val | (val << 4)
			uint8_t r = red4 | (red4 << 4);
			uint8_t g = green4 | (green4 << 4);
			uint8_t b = blue4 | (blue4 << 4);
			palette[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
		}
		benchmark::DoNotOptimize(palette);
	}
	state.SetItemsProcessed(state.iterations() * 16);
}
BENCHMARK(BM_LynxMikey_PaletteCompute);

// Benchmark palette lookup during pixel rendering
static void BM_LynxMikey_PaletteLookup(benchmark::State& state) {
	uint32_t palette[16] = {};
	for (int i = 0; i < 16; i++) {
		palette[i] = 0xFF000000 | (i << 20) | (i << 12) | (i << 4);
	}
	uint8_t pixelIndex = 0;

	for (auto _ : state) {
		uint32_t color = palette[pixelIndex & 0x0F];
		benchmark::DoNotOptimize(color);
		pixelIndex++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_PaletteLookup);

// -----------------------------------------------------------------------------
// Display Scanline Rendering (4bpp → ARGB32)
// -----------------------------------------------------------------------------

// Benchmark rendering a single scanline (80 bytes → 160 ARGB32 pixels)
static void BM_LynxMikey_RenderScanline(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::BytesPerScanline> scanlineData{};
	std::array<uint32_t, LynxConstants::ScreenWidth> outputPixels{};
	uint32_t palette[16] = {};

	// Fill palette with distinct colors
	for (int i = 0; i < 16; i++) {
		palette[i] = 0xFF000000 | (i * 17 << 16) | (i * 17 << 8) | (i * 17);
	}
	// Fill scanline with pattern data
	for (size_t i = 0; i < scanlineData.size(); i++) {
		scanlineData[i] = static_cast<uint8_t>(i * 3 + 0x42);
	}

	for (auto _ : state) {
		// 80 bytes → 160 pixels (4bpp packed: high nibble first)
		for (int i = 0; i < LynxConstants::BytesPerScanline; i++) {
			uint8_t byte = scanlineData[i];
			uint8_t pix0 = (byte >> 4) & 0x0F; // high nibble
			uint8_t pix1 = byte & 0x0F;         // low nibble
			outputPixels[i * 2] = palette[pix0];
			outputPixels[i * 2 + 1] = palette[pix1];
		}
		benchmark::DoNotOptimize(outputPixels);
	}
	state.SetBytesProcessed(state.iterations() * LynxConstants::BytesPerScanline);
	state.SetItemsProcessed(state.iterations() * LynxConstants::ScreenWidth);
}
BENCHMARK(BM_LynxMikey_RenderScanline);

// Benchmark rendering full frame (102 scanlines)
static void BM_LynxMikey_RenderFullFrame(benchmark::State& state) {
	std::array<uint8_t, LynxConstants::BytesPerScanline * LynxConstants::ScreenHeight> frameData{};
	std::array<uint32_t, LynxConstants::PixelCount> outputPixels{};
	uint32_t palette[16] = {};

	for (int i = 0; i < 16; i++) {
		palette[i] = 0xFF000000 | (i * 17 << 16) | (i * 17 << 8) | (i * 17);
	}
	for (size_t i = 0; i < frameData.size(); i++) {
		frameData[i] = static_cast<uint8_t>(i & 0xFF);
	}

	for (auto _ : state) {
		for (uint32_t scanline = 0; scanline < LynxConstants::ScreenHeight; scanline++) {
			const uint8_t* src = &frameData[scanline * LynxConstants::BytesPerScanline];
			uint32_t* dst = &outputPixels[scanline * LynxConstants::ScreenWidth];
			for (int i = 0; i < LynxConstants::BytesPerScanline; i++) {
				uint8_t byte = src[i];
				dst[i * 2] = palette[(byte >> 4) & 0x0F];
				dst[i * 2 + 1] = palette[byte & 0x0F];
			}
		}
		benchmark::DoNotOptimize(outputPixels);
	}
	state.SetBytesProcessed(state.iterations() * frameData.size());
	state.SetItemsProcessed(state.iterations() * LynxConstants::PixelCount);
}
BENCHMARK(BM_LynxMikey_RenderFullFrame);

//=============================================================================
// Audit Fix Benchmarks
//=============================================================================

/// [12.7] CTLA bit 6 self-clearing mask — must be zero-cost.
static void BM_LynxMikey_CtlaBit6Mask(benchmark::State& state) {
	uint8_t controlA = 0;
	uint8_t input = 0;
	for (auto _ : state) {
		controlA = input & ~0x40; // Mask off self-clearing bit 6
		benchmark::DoNotOptimize(controlA);
		input++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_CtlaBit6Mask);

/// [12.6] CTLB write — clear timer-done only.
static void BM_LynxMikey_CtlbWriteClearDone(benchmark::State& state) {
	uint8_t controlB = 0xFF;
	for (auto _ : state) {
		controlB &= ~0x08; // Clear timer-done bit only
		benchmark::DoNotOptimize(controlB);
		controlB = 0xFF; // Reset for next iteration
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_CtlbWriteClearDone);

/// [12.15/#397] IODAT read — bit extraction and direction masking.
static void BM_LynxMikey_IodatBitExtraction(benchmark::State& state) {
	uint8_t ioDir = 0x07;   // Bits 0-2 output
	uint8_t ioData = 0;
	uint8_t audin = 0;
	for (auto _ : state) {
		uint8_t result = (ioData & ioDir) | (audin & ~ioDir);
		result = (result & 0x07) | (audin << 3); // EEPROM + AUDIN
		benchmark::DoNotOptimize(result);
		ioData++;
		audin = (audin + 1) & 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_IodatBitExtraction);

/// [12.1] IrqEnabled tracking — per-timer IRQ enable bit set/clear.
static void BM_LynxMikey_IrqEnabledTracking(benchmark::State& state) {
	uint8_t irqEnabled = 0;
	uint8_t timerIdx = 0;
	for (auto _ : state) {
		uint8_t ctla = timerIdx; // pseudo-random pattern
		if (ctla & 0x80) {
			irqEnabled |= (1 << (timerIdx & 0x07));
		} else {
			irqEnabled &= ~(1 << (timerIdx & 0x07));
		}
		benchmark::DoNotOptimize(irqEnabled);
		timerIdx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxMikey_IrqEnabledTracking);

/// [12.25/#407] Display address computation with uint16_t wrapping.
static void BM_LynxMikey_DisplayAddressWrap(benchmark::State& state) {
	uint16_t displayAddr = 0xFDA0;
	uint16_t lineAddr = 0;
	for (auto _ : state) {
		for (uint32_t scanline = 0; scanline < LynxConstants::ScreenHeight; scanline++) {
			lineAddr = displayAddr + (uint16_t)(scanline * LynxConstants::BytesPerScanline);
			benchmark::DoNotOptimize(lineAddr);
		}
	}
	state.SetItemsProcessed(state.iterations() * LynxConstants::ScreenHeight);
}
BENCHMARK(BM_LynxMikey_DisplayAddressWrap);
