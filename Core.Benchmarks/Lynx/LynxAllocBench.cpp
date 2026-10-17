#include "pch.h"
#include <array>
#include <cstring>
#include <memory>
#include <vector>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx Memory, Allocation, and State Size Benchmarks
// =============================================================================
// These benchmarks measure memory footprint, allocation costs, and data
// structure efficiency for the Lynx emulator subsystems. Critical for
// understanding the real cost of save states, frame buffer copies, and
// per-frame allocations.

// =============================================================================
// SECTION 1: State Struct Size Verification
// =============================================================================
// Emulator state structs are serialized for save states, rewind, and movie
// recording. Smaller structs = faster I/O and less memory pressure.

static void BM_LynxAlloc_LynxStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t totalSize = sizeof(LynxState);
		benchmark::DoNotOptimize(totalSize);
	}
	// Report the actual size as a custom counter for visibility
	state.counters["StateBytes"] = sizeof(LynxState);
}
BENCHMARK(BM_LynxAlloc_LynxStateSize);

static void BM_LynxAlloc_CpuStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxCpuState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxCpuState);
}
BENCHMARK(BM_LynxAlloc_CpuStateSize);

static void BM_LynxAlloc_MikeyStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxMikeyState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxMikeyState);
}
BENCHMARK(BM_LynxAlloc_MikeyStateSize);

static void BM_LynxAlloc_SuzyStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxSuzyState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxSuzyState);
}
BENCHMARK(BM_LynxAlloc_SuzyStateSize);

static void BM_LynxAlloc_ApuStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxApuState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxApuState);
}
BENCHMARK(BM_LynxAlloc_ApuStateSize);

static void BM_LynxAlloc_TimerStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxTimerState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxTimerState);
	state.counters["AllTimersBytes"] = sizeof(LynxTimerState) * LynxConstants::TimerCount;
}
BENCHMARK(BM_LynxAlloc_TimerStateSize);

static void BM_LynxAlloc_MemManagerStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxMemoryManagerState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxMemoryManagerState);
}
BENCHMARK(BM_LynxAlloc_MemManagerStateSize);

static void BM_LynxAlloc_CartStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxCartState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxCartState);
}
BENCHMARK(BM_LynxAlloc_CartStateSize);

static void BM_LynxAlloc_EepromStateSize(benchmark::State& state) {
	for (auto _ : state) {
		size_t sz = sizeof(LynxEepromSerialState);
		benchmark::DoNotOptimize(sz);
	}
	state.counters["StateBytes"] = sizeof(LynxEepromSerialState);
}
BENCHMARK(BM_LynxAlloc_EepromStateSize);

// =============================================================================
// SECTION 2: Frame Buffer Allocation & Copy
// =============================================================================
// The frame buffer is 160×102 pixels, 32-bit RGBA. Copied every frame.
// Cost of memcpy is the display pipeline's baseline overhead.

static void BM_LynxAlloc_FrameBufferCopy(benchmark::State& state) {
	constexpr size_t pixelCount = LynxConstants::PixelCount;
	std::array<uint32_t, pixelCount> src{};
	std::array<uint32_t, pixelCount> dst{};

	// Fill with pattern
	for (size_t i = 0; i < pixelCount; i++) {
		src[i] = static_cast<uint32_t>(i * 0xDEAD + 0xBEEF);
	}

	for (auto _ : state) {
		memcpy(dst.data(), src.data(), pixelCount * sizeof(uint32_t));
		benchmark::DoNotOptimize(dst[0]);
	}
	state.SetBytesProcessed(state.iterations() * pixelCount * sizeof(uint32_t));
	state.counters["FrameBytes"] = pixelCount * sizeof(uint32_t);
}
BENCHMARK(BM_LynxAlloc_FrameBufferCopy);

static void BM_LynxAlloc_FrameBufferFillZero(benchmark::State& state) {
	constexpr size_t pixelCount = LynxConstants::PixelCount;
	std::array<uint32_t, pixelCount> fb{};

	for (auto _ : state) {
		std::fill_n(fb.data(), pixelCount, 0u);
		benchmark::DoNotOptimize(fb[0]);
	}
	state.SetBytesProcessed(state.iterations() * pixelCount * sizeof(uint32_t));
}
BENCHMARK(BM_LynxAlloc_FrameBufferFillZero);

static void BM_LynxAlloc_FrameBufferFillColor(benchmark::State& state) {
	constexpr size_t pixelCount = LynxConstants::PixelCount;
	std::array<uint32_t, pixelCount> fb{};
	uint32_t color = 0xFF00FF00; // Green ARGB

	for (auto _ : state) {
		std::fill_n(fb.data(), pixelCount, color);
		benchmark::DoNotOptimize(fb[0]);
	}
	state.SetBytesProcessed(state.iterations() * pixelCount * sizeof(uint32_t));
}
BENCHMARK(BM_LynxAlloc_FrameBufferFillColor);

// =============================================================================
// SECTION 3: RAM Allocation Patterns
// =============================================================================
// The Lynx has 64KB of work RAM. These benchmarks measure the cost of
// allocating and initializing RAM arrays of various sizes.

static void BM_LynxAlloc_WorkRamStack(benchmark::State& state) {
	// Stack-allocated 64KB work RAM (typical emulator pattern)
	for (auto _ : state) {
		std::array<uint8_t, LynxConstants::WorkRamSize> ram{};
		ram[0] = 0x42;
		benchmark::DoNotOptimize(ram[0]);
	}
	state.counters["RamBytes"] = LynxConstants::WorkRamSize;
}
BENCHMARK(BM_LynxAlloc_WorkRamStack);

static void BM_LynxAlloc_WorkRamHeap(benchmark::State& state) {
	// Heap-allocated 64KB work RAM (dynamic pattern)
	for (auto _ : state) {
		auto ram = std::make_unique<uint8_t[]>(LynxConstants::WorkRamSize);
		ram[0] = 0x42;
		benchmark::DoNotOptimize(ram[0]);
	}
	state.counters["RamBytes"] = LynxConstants::WorkRamSize;
}
BENCHMARK(BM_LynxAlloc_WorkRamHeap);

static void BM_LynxAlloc_WorkRamVector(benchmark::State& state) {
	// Vector-allocated 64KB work RAM
	for (auto _ : state) {
		std::vector<uint8_t> ram(LynxConstants::WorkRamSize, 0);
		ram[0] = 0x42;
		benchmark::DoNotOptimize(ram[0]);
	}
	state.counters["RamBytes"] = LynxConstants::WorkRamSize;
}
BENCHMARK(BM_LynxAlloc_WorkRamVector);

// =============================================================================
// SECTION 4: Save State Serialization Cost
// =============================================================================
// Measures the cost of copying complete emulator state — critical for
// save states, rewind buffer, and movie recording.

static void BM_LynxAlloc_SaveStateCopy(benchmark::State& state) {
	LynxState srcState{};
	LynxState dstState{};

	// Initialize with non-zero data
	srcState.Cpu.PC = 0x1234;
	srcState.Cpu.A = 0x42;
	srcState.Mikey.IrqEnabled = 0xFF;
	srcState.Suzy.MathABCD = 0xBEEF;

	for (auto _ : state) {
		memcpy(&dstState, &srcState, sizeof(LynxState));
		benchmark::DoNotOptimize(dstState.Cpu.PC);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(LynxState));
	state.counters["StateBytes"] = sizeof(LynxState);
}
BENCHMARK(BM_LynxAlloc_SaveStateCopy);

static void BM_LynxAlloc_SaveStateCopyAssign(benchmark::State& state) {
	LynxState srcState{};
	LynxState dstState{};

	srcState.Cpu.PC = 0x1234;
	srcState.Mikey.IrqEnabled = 0xFF;

	for (auto _ : state) {
		dstState = srcState;  // Struct assignment
		benchmark::DoNotOptimize(dstState.Cpu.PC);
	}
	state.SetBytesProcessed(state.iterations() * sizeof(LynxState));
}
BENCHMARK(BM_LynxAlloc_SaveStateCopyAssign);

// =============================================================================
// SECTION 5: Palette Lookup Cost
// =============================================================================
// Every visible pixel requires a palette lookup. 16 entries × 2 bytes each.

static void BM_LynxAlloc_PaletteLookup(benchmark::State& state) {
	LynxMikeyState mikey{};
	// Fill palette with test colors
	for (int i = 0; i < 16; i++) {
		mikey.PaletteGreen[i] = static_cast<uint8_t>(i);
		mikey.PaletteBR[i] = static_cast<uint8_t>(i * 0x11);
	}

	uint8_t idx = 0;
	for (auto _ : state) {
		uint8_t green = mikey.PaletteGreen[idx & 0x0F];
		uint8_t br = mikey.PaletteBR[idx & 0x0F];
		uint32_t color = (green << 16) | (br << 8) | (br & 0x0F);
		benchmark::DoNotOptimize(color);
		idx++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxAlloc_PaletteLookup);

static void BM_LynxAlloc_PaletteLookupFullScanline(benchmark::State& state) {
	LynxMikeyState mikey{};
	for (int i = 0; i < 16; i++) {
		mikey.PaletteGreen[i] = static_cast<uint8_t>(i * 3);
		mikey.PaletteBR[i] = static_cast<uint8_t>(i * 0x11);
	}

	// Process one full scanline (160 pixels, packed 2 per byte = 80 bytes)
	std::array<uint8_t, LynxConstants::BytesPerScanline> scanline{};
	for (size_t i = 0; i < scanline.size(); i++) {
		scanline[i] = static_cast<uint8_t>(i * 17);
	}

	std::array<uint32_t, LynxConstants::ScreenWidth> output{};

	for (auto _ : state) {
		for (size_t i = 0; i < scanline.size(); i++) {
			uint8_t byte = scanline[i];
			uint8_t hiIdx = (byte >> 4) & 0x0F;
			uint8_t loIdx = byte & 0x0F;

			uint8_t g1 = mikey.PaletteGreen[hiIdx];
			uint8_t br1 = mikey.PaletteBR[hiIdx];
			output[i * 2] = 0xFF000000u | (g1 << 16) | ((br1 >> 4) << 8) | (br1 & 0x0F);

			uint8_t g2 = mikey.PaletteGreen[loIdx];
			uint8_t br2 = mikey.PaletteBR[loIdx];
			output[i * 2 + 1] = 0xFF000000u | (g2 << 16) | ((br2 >> 4) << 8) | (br2 & 0x0F);
		}
		benchmark::DoNotOptimize(output[0]);
	}
	state.SetItemsProcessed(state.iterations() * LynxConstants::ScreenWidth);
}
BENCHMARK(BM_LynxAlloc_PaletteLookupFullScanline);

// =============================================================================
// SECTION 6: Sprite Chain Memory Access Patterns
// =============================================================================
// Suzy processes sprites as a linked list of SCBs. Each SCB is at least
// 10 bytes. A typical frame may have 20-100 sprites.

static void BM_LynxAlloc_SpriteChainTraversal(benchmark::State& state) {
	// Simulate 64 sprites linked in sequence
	constexpr int spriteCount = 64;
	struct SimpleSCB {
		uint16_t nextAddr;
		uint8_t sprctl0;
		uint8_t sprctl1;
		int16_t hPos;
		int16_t vPos;
		uint16_t hSize;
		uint16_t vSize;
	};

	std::array<SimpleSCB, spriteCount> scbs{};
	for (int i = 0; i < spriteCount - 1; i++) {
		scbs[i].nextAddr = static_cast<uint16_t>((i + 1) * sizeof(SimpleSCB));
		scbs[i].hPos = static_cast<int16_t>(i * 3);
		scbs[i].vPos = static_cast<int16_t>(i * 2);
	}
	scbs[spriteCount - 1].nextAddr = 0; // End of chain

	for (auto _ : state) {
		int idx = 0;
		int count = 0;
		while (idx < spriteCount && scbs[idx].nextAddr != 0) {
			benchmark::DoNotOptimize(scbs[idx].hPos);
			benchmark::DoNotOptimize(scbs[idx].vPos);
			count++;
			idx++;
		}
		benchmark::DoNotOptimize(count);
	}
	state.SetItemsProcessed(state.iterations() * (spriteCount - 1));
}
BENCHMARK(BM_LynxAlloc_SpriteChainTraversal);

// =============================================================================
// SECTION 7: Collision Buffer Operations
// =============================================================================
// Collision detection uses a 160×102 collision buffer (one nibble per pixel)

static void BM_LynxAlloc_CollisionBufferClear(benchmark::State& state) {
	// Collision buffer: 160×102 / 2 = 8160 bytes (nibble-packed)
	constexpr size_t collisionBytes = LynxConstants::PixelCount / 2;
	std::vector<uint8_t> collisionBuf(collisionBytes);

	for (auto _ : state) {
		memset(collisionBuf.data(), 0, collisionBytes);
		benchmark::DoNotOptimize(collisionBuf[0]);
	}
	state.SetBytesProcessed(state.iterations() * collisionBytes);
	state.counters["CollisionBytes"] = collisionBytes;
}
BENCHMARK(BM_LynxAlloc_CollisionBufferClear);

static void BM_LynxAlloc_CollisionBufferWrite(benchmark::State& state) {
	constexpr size_t collisionBytes = LynxConstants::PixelCount / 2;
	std::vector<uint8_t> collisionBuf(collisionBytes, 0);

	uint16_t x = 0;
	uint8_t collNum = 1;

	for (auto _ : state) {
		// Write collision number to even/odd pixel position
		size_t byteIdx = x / 2;
		if (x & 1) {
			collisionBuf[byteIdx] = (collisionBuf[byteIdx] & 0xF0) | (collNum & 0x0F);
		} else {
			collisionBuf[byteIdx] = (collisionBuf[byteIdx] & 0x0F) | ((collNum & 0x0F) << 4);
		}
		benchmark::DoNotOptimize(collisionBuf[byteIdx]);
		x = (x + 1) % LynxConstants::PixelCount;
		collNum = ((collNum + 1) & 0x0F) | 1; // Keep non-zero
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxAlloc_CollisionBufferWrite);

// =============================================================================
// SECTION 8: Timer Tick Batching
// =============================================================================
// All 8 timers are ticked each CPU cycle. Batch ticking is critical.

static void BM_LynxAlloc_TimerTickAll8(benchmark::State& state) {
	LynxMikeyState mikey{};
	for (int i = 0; i < 8; i++) {
		mikey.Timers[i].BackupValue = 0xFF;
		mikey.Timers[i].Count = 0x80;
		mikey.Timers[i].TimerDone = false;
		mikey.Timers[i].Linked = false;
	}

	for (auto _ : state) {
		for (int i = 0; i < 8; i++) {
			auto& t = mikey.Timers[i];
			if (!t.TimerDone) {
				if (t.Count == 0) {
					t.Count = t.BackupValue;
					t.TimerDone = true;
				} else {
					t.Count--;
				}
			}
		}
		// Reset for next iteration
		for (int i = 0; i < 8; i++) {
			mikey.Timers[i].Count = 0x80;
			mikey.Timers[i].TimerDone = false;
		}
		benchmark::DoNotOptimize(mikey.Timers[0].Count);
	}
	state.SetItemsProcessed(state.iterations() * 8);
}
BENCHMARK(BM_LynxAlloc_TimerTickAll8);

// =============================================================================
// SECTION 9: Audio Sample Generation
// =============================================================================
// 4 LFSR audio channels generate samples at the CPU clock rate.
// Each sample requires LFSR clocking + volume + attenuation.

static void BM_LynxAlloc_AudioSample4Channels(benchmark::State& state) {
	LynxApuState apu{};
	for (int i = 0; i < 4; i++) {
		apu.Channels[i].ShiftRegister = 0x001;
		apu.Channels[i].Volume = 0x7F;
		apu.Channels[i].FeedbackEnable = 0x01;
		apu.Channels[i].Attenuation = 0xFF;
		apu.Channels[i].Enabled = true;
	}

	for (auto _ : state) {
		int16_t leftSample = 0;
		int16_t rightSample = 0;

		for (int ch = 0; ch < 4; ch++) {
			auto& c = apu.Channels[ch];
			if (!c.Enabled) continue;

			// Clock LFSR
			uint16_t feedback = 0;
			if (c.FeedbackEnable & 0x01) feedback ^= (c.ShiftRegister >> 0) & 1;
			c.ShiftRegister = ((c.ShiftRegister >> 1) | (feedback << 11)) & 0xFFF;

			// Generate output
			int8_t output = (c.ShiftRegister & 1) ? c.Volume : -c.Volume;

			// Apply stereo attenuation
			uint8_t leftAtten = (c.Attenuation >> 4) & 0x0F;
			uint8_t rightAtten = c.Attenuation & 0x0F;
			leftSample += (output * leftAtten) >> 4;
			rightSample += (output * rightAtten) >> 4;
		}
		benchmark::DoNotOptimize(leftSample);
		benchmark::DoNotOptimize(rightSample);
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxAlloc_AudioSample4Channels);

// =============================================================================
// SECTION 10: Input State Copy
// =============================================================================
// Input state is polled every frame — measure the overhead

static void BM_LynxAlloc_InputStateCopy(benchmark::State& state) {
	LynxControlManagerState src{};
	LynxControlManagerState dst{};
	src.Joystick = 0xFF;
	src.Switches = 0xFE;

	for (auto _ : state) {
		dst = src;
		benchmark::DoNotOptimize(dst.Joystick);
		benchmark::DoNotOptimize(dst.Switches);
	}
	state.SetItemsProcessed(state.iterations());
	state.counters["InputBytes"] = sizeof(LynxControlManagerState);
}
BENCHMARK(BM_LynxAlloc_InputStateCopy);
