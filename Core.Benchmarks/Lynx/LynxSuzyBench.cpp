#include "pch.h"
#include <array>
#include <cstring>
#include <random>
#include "Lynx/LynxTypes.h"

// =============================================================================
// Lynx Suzy Benchmarks (Sprite Engine, Math, Collision)
// =============================================================================
// Suzy handles the most compute-intensive operations on the Lynx:
// - 16×16→32 hardware multiply/divide (sprite scaling)
// - Sprite chain traversal and pixel rendering
// - Collision detection buffer management
// On real hardware, Suzy halts the CPU during sprite processing.

// -----------------------------------------------------------------------------
// Hardware Math: Multiply
// -----------------------------------------------------------------------------

// Benchmark unsigned 16×16→32 multiply
static void BM_LynxSuzy_Multiply_Unsigned(benchmark::State& state) {
	uint16_t a = 0;
	uint16_t b = 0x1234;

	for (auto _ : state) {
		uint32_t result = static_cast<uint32_t>(a) * static_cast<uint32_t>(b);
		benchmark::DoNotOptimize(result);
		a++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Multiply_Unsigned);

// Benchmark signed 16×16→32 multiply (sign-magnitude, Bug 13.8)
static void BM_LynxSuzy_Multiply_SignMagnitude(benchmark::State& state) {
	uint16_t rawA = 0;
	uint16_t rawB = 0x1234;

	for (auto _ : state) {
		// Bug 13.8: sign-magnitude — $8000 = positive, $0000 = negative
		bool negA = (rawA & 0x8000) == 0;
		bool negB = (rawB & 0x8000) == 0;
		uint16_t magA = negA ? (~rawA + 1) : rawA;
		uint16_t magB = negB ? (~rawB + 1) : rawB;

		uint32_t result = static_cast<uint32_t>(magA) * static_cast<uint32_t>(magB);

		bool negResult = negA ^ negB;
		if (negResult) {
			result = ~result + 1;
		}
		benchmark::DoNotOptimize(result);
		rawA++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Multiply_SignMagnitude);

// Benchmark multiply with accumulate (sprite scaling accumulator)
static void BM_LynxSuzy_Multiply_Accumulate(benchmark::State& state) {
	uint16_t a = 100;
	uint16_t b = 50;
	uint32_t accumulator = 0;

	for (auto _ : state) {
		uint32_t result = static_cast<uint32_t>(a) * static_cast<uint32_t>(b);
		accumulator += result;
		bool overflow = (accumulator < result); // carry detection
		benchmark::DoNotOptimize(accumulator);
		benchmark::DoNotOptimize(overflow);
		a++;
		b = static_cast<uint16_t>(b ^ (a >> 8));
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Multiply_Accumulate);

// -----------------------------------------------------------------------------
// Hardware Math: Divide
// -----------------------------------------------------------------------------

// Benchmark unsigned 32÷16 divide
static void BM_LynxSuzy_Divide_Unsigned(benchmark::State& state) {
	uint32_t dividend = 0x12345678;
	uint16_t divisor = 0x0100;

	for (auto _ : state) {
		uint16_t quotient = 0;
		uint16_t remainder = 0;
		if (divisor != 0) {
			quotient = static_cast<uint16_t>(dividend / divisor);
			remainder = static_cast<uint16_t>(dividend % divisor);
		}
		benchmark::DoNotOptimize(quotient);
		benchmark::DoNotOptimize(remainder);
		dividend += 0x1111;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Divide_Unsigned);

// Benchmark signed divide with Bug 13.9 (remainder always positive)
static void BM_LynxSuzy_Divide_SignedBug13_9(benchmark::State& state) {
	int32_t dividend = -12345678;
	int16_t divisor = 100;

	for (auto _ : state) {
		int16_t quotient = 0;
		uint16_t remainder = 0; // Bug 13.9: always positive
		if (divisor != 0) {
			quotient = static_cast<int16_t>(dividend / divisor);
			int16_t rawRemainder = static_cast<int16_t>(dividend % divisor);
			remainder = static_cast<uint16_t>(rawRemainder < 0 ? -rawRemainder : rawRemainder);
		}
		benchmark::DoNotOptimize(quotient);
		benchmark::DoNotOptimize(remainder);
		dividend += 13579;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Divide_SignedBug13_9);

// Benchmark divide-by-zero handling (edge case encountered in sprite scaling)
static void BM_LynxSuzy_Divide_ByZero(benchmark::State& state) {
	uint32_t dividend = 0xFFFFFFFF;
	uint16_t divisor = 0;

	for (auto _ : state) {
		uint16_t quotient = 0;
		uint16_t remainder = 0;
		if (divisor != 0) {
			quotient = static_cast<uint16_t>(dividend / divisor);
			remainder = static_cast<uint16_t>(dividend % divisor);
		} else {
			// Divide by zero: result is undefined on Lynx, typically 0
			quotient = 0;
			remainder = 0;
		}
		benchmark::DoNotOptimize(quotient);
		benchmark::DoNotOptimize(remainder);
		dividend ^= 0xDEADBEEF;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Divide_ByZero);

// -----------------------------------------------------------------------------
// Collision Buffer
// -----------------------------------------------------------------------------

// Benchmark collision buffer mutual update (per sprite pixel)
static void BM_LynxSuzy_CollisionUpdate(benchmark::State& state) {
	uint8_t collisionBuffer[LynxConstants::CollisionBufferSize] = {};
	uint8_t spriteNum = 0;

	for (auto _ : state) {
		uint8_t x = spriteNum & 0x0F;
		uint8_t existing = collisionBuffer[x];
		// Rule: write larger of sprite number and existing value
		if (spriteNum > existing) {
			collisionBuffer[x] = spriteNum;
		}
		// Collision depository for sprite gets existing value
		uint8_t depositoryValue = existing;
		benchmark::DoNotOptimize(depositoryValue);
		benchmark::DoNotOptimize(collisionBuffer);
		spriteNum++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_CollisionUpdate);

// Benchmark collision buffer clear (start of frame)
static void BM_LynxSuzy_CollisionClear(benchmark::State& state) {
	uint8_t collisionBuffer[LynxConstants::CollisionBufferSize] = {};

	for (auto _ : state) {
		std::memset(collisionBuffer, 0, sizeof(collisionBuffer));
		benchmark::DoNotOptimize(collisionBuffer);
	}
	state.SetBytesProcessed(state.iterations() * LynxConstants::CollisionBufferSize);
}
BENCHMARK(BM_LynxSuzy_CollisionClear);

// Benchmark collision check pattern (branchless max)
static void BM_LynxSuzy_CollisionUpdate_Branchless(benchmark::State& state) {
	uint8_t collisionBuffer[LynxConstants::CollisionBufferSize] = {};
	uint8_t spriteNum = 0;

	for (auto _ : state) {
		uint8_t x = spriteNum & 0x0F;
		uint8_t existing = collisionBuffer[x];
		// Branchless max
		collisionBuffer[x] = spriteNum > existing ? spriteNum : existing;
		uint8_t depositoryValue = existing;
		benchmark::DoNotOptimize(depositoryValue);
		benchmark::DoNotOptimize(collisionBuffer);
		spriteNum++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_CollisionUpdate_Branchless);

// -----------------------------------------------------------------------------
// Sprite Pixel Rendering
// -----------------------------------------------------------------------------

// Benchmark 4bpp pixel extraction from sprite data
static void BM_LynxSuzy_PixelExtract_4bpp(benchmark::State& state) {
	uint8_t spriteData[32] = {};
	for (int i = 0; i < 32; i++) spriteData[i] = static_cast<uint8_t>(i * 7 + 0x42);

	for (auto _ : state) {
		for (int i = 0; i < 32; i++) {
			uint8_t byte = spriteData[i];
			uint8_t pix0 = (byte >> 4) & 0x0F;
			uint8_t pix1 = byte & 0x0F;
			benchmark::DoNotOptimize(pix0);
			benchmark::DoNotOptimize(pix1);
		}
	}
	state.SetItemsProcessed(state.iterations() * 64); // 64 pixels from 32 bytes
}
BENCHMARK(BM_LynxSuzy_PixelExtract_4bpp);

// Benchmark 1bpp pixel extraction (background sprites)
static void BM_LynxSuzy_PixelExtract_1bpp(benchmark::State& state) {
	uint8_t spriteData[16] = {};
	for (int i = 0; i < 16; i++) spriteData[i] = static_cast<uint8_t>(i * 17 + 0xAA);

	for (auto _ : state) {
		for (int i = 0; i < 16; i++) {
			uint8_t byte = spriteData[i];
			for (int bit = 7; bit >= 0; bit--) {
				uint8_t pixel = (byte >> bit) & 1;
				benchmark::DoNotOptimize(pixel);
			}
		}
	}
	state.SetItemsProcessed(state.iterations() * 128); // 128 pixels from 16 bytes
}
BENCHMARK(BM_LynxSuzy_PixelExtract_1bpp);

// Benchmark sprite pixel write to frame buffer with palette lookup
static void BM_LynxSuzy_SpritePixelWrite(benchmark::State& state) {
	std::array<uint32_t, LynxConstants::PixelCount> frameBuffer{};
	uint32_t palette[16] = {};
	for (int i = 0; i < 16; i++) {
		palette[i] = 0xFF000000 | (i * 17 << 16) | (i * 17 << 8) | (i * 17);
	}

	uint16_t x = 0;
	uint16_t y = 0;
	uint8_t pixelIndex = 0;

	for (auto _ : state) {
		// Bounds check + palette lookup + write (per sprite pixel)
		if (x < LynxConstants::ScreenWidth && y < LynxConstants::ScreenHeight) {
			uint32_t offset = y * LynxConstants::ScreenWidth + x;
			frameBuffer[offset] = palette[pixelIndex & 0x0F];
		}
		benchmark::DoNotOptimize(frameBuffer);
		x = (x + 1) % LynxConstants::ScreenWidth;
		if (x == 0) y = (y + 1) % LynxConstants::ScreenHeight;
		pixelIndex++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_SpritePixelWrite);

// Benchmark sprite pixel write (unchecked, hot inner loop)
static void BM_LynxSuzy_SpritePixelWrite_Unchecked(benchmark::State& state) {
	std::array<uint32_t, LynxConstants::PixelCount> frameBuffer{};
	uint32_t palette[16] = {};
	for (int i = 0; i < 16; i++) {
		palette[i] = 0xFF000000 | (i * 17 << 16) | (i * 17 << 8) | (i * 17);
	}

	uint32_t offset = 0;
	uint8_t pixelIndex = 0;

	for (auto _ : state) {
		frameBuffer[offset] = palette[pixelIndex & 0x0F];
		benchmark::DoNotOptimize(frameBuffer[offset]);
		offset = (offset + 1) % LynxConstants::PixelCount;
		pixelIndex++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_SpritePixelWrite_Unchecked);

// -----------------------------------------------------------------------------
// SCB (Sprite Control Block) Parsing
// -----------------------------------------------------------------------------

// Benchmark SCB header parsing (8 bytes per sprite)
static void BM_LynxSuzy_SCBParse(benchmark::State& state) {
	// Simulate SCB data in RAM
	struct SCBHeader {
		uint16_t next;    // Pointer to next SCB
		uint16_t data;    // Pointer to sprite pixel data
		int16_t xpos;     // X position
		int16_t ypos;     // Y position
	};

	std::array<uint8_t, 64 * 1024> ram{};
	// Place 32 SCBs in a chain
	for (int i = 0; i < 32; i++) {
		uint16_t addr = static_cast<uint16_t>(0x2000 + i * 16);
		uint16_t nextAddr = (i < 31) ? static_cast<uint16_t>(addr + 16) : 0;
		ram[addr + 0] = nextAddr & 0xFF;
		ram[addr + 1] = nextAddr >> 8;
		ram[addr + 2] = 0x00;
		ram[addr + 3] = 0x30; // sprite data at $3000
		ram[addr + 4] = static_cast<uint8_t>(i * 5);
		ram[addr + 5] = 0x00; // xpos
		ram[addr + 6] = static_cast<uint8_t>(i * 3);
		ram[addr + 7] = 0x00; // ypos
	}

	for (auto _ : state) {
		uint16_t scbAddr = 0x2000;
		int spriteCount = 0;
		while (scbAddr != 0) {
			// Read SCB header (little-endian)
			uint16_t nextSCB = ram[scbAddr] | (ram[scbAddr + 1] << 8);
			uint16_t dataPtr = ram[scbAddr + 2] | (ram[scbAddr + 3] << 8);
			int16_t xpos = static_cast<int16_t>(ram[scbAddr + 4] | (ram[scbAddr + 5] << 8));
			int16_t ypos = static_cast<int16_t>(ram[scbAddr + 6] | (ram[scbAddr + 7] << 8));
			benchmark::DoNotOptimize(dataPtr);
			benchmark::DoNotOptimize(xpos);
			benchmark::DoNotOptimize(ypos);
			spriteCount++;

			// Bug 13.12: chain terminates when next has upper byte = 0
			if ((nextSCB & 0xFF00) == 0) break;
			scbAddr = nextSCB;
		}
		benchmark::DoNotOptimize(spriteCount);
	}
	state.SetItemsProcessed(state.iterations() * 32);
}
BENCHMARK(BM_LynxSuzy_SCBParse);

// Benchmark Bug 13.12 termination check patterns
static void BM_LynxSuzy_Bug13_12_TerminationCheck(benchmark::State& state) {
	uint16_t nextAddr = 0;
	uint8_t hi = 0;

	for (auto _ : state) {
		// Bug 13.12: only upper byte checked for zero
		bool terminate = (nextAddr & 0xFF00) == 0;
		benchmark::DoNotOptimize(terminate);
		hi++;
		nextAddr = (hi << 8) | 0x42;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_Bug13_12_TerminationCheck);

// Benchmark correct termination check (full address == 0)
static void BM_LynxSuzy_CorrectTerminationCheck(benchmark::State& state) {
	uint16_t nextAddr = 0;
	uint8_t hi = 0;

	for (auto _ : state) {
		bool terminate = (nextAddr == 0);
		benchmark::DoNotOptimize(terminate);
		hi++;
		nextAddr = (hi << 8) | 0x42;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_CorrectTerminationCheck);

// -----------------------------------------------------------------------------
// Joystick/Input
// -----------------------------------------------------------------------------

// Benchmark joystick register read pattern
static void BM_LynxSuzy_JoystickRead(benchmark::State& state) {
	uint8_t joystick = 0;
	uint8_t switches = 0;

	for (auto _ : state) {
		// Typical game reads joystick and checks individual buttons
		bool up = joystick & 0x80;
		bool down = joystick & 0x40;
		bool left = joystick & 0x20;
		bool right = joystick & 0x10;
		bool optA = joystick & 0x08;
		bool optB = joystick & 0x04;
		bool btnA = joystick & 0x02;
		bool btnB = joystick & 0x01;
		bool pause = switches & 0x01;
		benchmark::DoNotOptimize(up);
		benchmark::DoNotOptimize(down);
		benchmark::DoNotOptimize(left);
		benchmark::DoNotOptimize(right);
		benchmark::DoNotOptimize(optA);
		benchmark::DoNotOptimize(optB);
		benchmark::DoNotOptimize(btnA);
		benchmark::DoNotOptimize(btnB);
		benchmark::DoNotOptimize(pause);
		joystick++;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_JoystickRead);

//=============================================================================
// Audit Fix Benchmarks
//=============================================================================

/// [12.23/#405] PeekRegister math byte extraction — must be zero side-effect.
static void BM_LynxSuzy_PeekMathRegisters(benchmark::State& state) {
	uint32_t mathJKLM = 0xDEADBEEF;
	uint32_t mathEFGH = 0xCAFEBABE;
	uint32_t mathABCD = 0x12345678;
	uint16_t mathNP = 0x9ABC;
	for (auto _ : state) {
		// Extract all byte-level views (what PeekRegister does)
		uint8_t h = (uint8_t)(mathEFGH & 0xFF);
		uint8_t g = (uint8_t)((mathEFGH >> 8) & 0xFF);
		uint8_t f = (uint8_t)((mathEFGH >> 16) & 0xFF);
		uint8_t e = (uint8_t)((mathEFGH >> 24) & 0xFF);
		uint8_t m = (uint8_t)(mathJKLM & 0xFF);
		uint8_t l = (uint8_t)((mathJKLM >> 8) & 0xFF);
		uint8_t k = (uint8_t)((mathJKLM >> 16) & 0xFF);
		uint8_t j = (uint8_t)((mathJKLM >> 24) & 0xFF);
		uint8_t d = (uint8_t)(mathABCD & 0xFF);
		uint8_t c = (uint8_t)((mathABCD >> 8) & 0xFF);
		uint8_t b = (uint8_t)((mathABCD >> 16) & 0xFF);
		uint8_t a = (uint8_t)((mathABCD >> 24) & 0xFF);
		uint8_t p = (uint8_t)(mathNP & 0xFF);
		uint8_t n = (uint8_t)((mathNP >> 8) & 0xFF);
		benchmark::DoNotOptimize(h); benchmark::DoNotOptimize(g);
		benchmark::DoNotOptimize(f); benchmark::DoNotOptimize(e);
		benchmark::DoNotOptimize(m); benchmark::DoNotOptimize(l);
		benchmark::DoNotOptimize(k); benchmark::DoNotOptimize(j);
		benchmark::DoNotOptimize(d); benchmark::DoNotOptimize(c);
		benchmark::DoNotOptimize(b); benchmark::DoNotOptimize(a);
		benchmark::DoNotOptimize(p); benchmark::DoNotOptimize(n);
		mathJKLM++; mathEFGH++; // Vary inputs
	}
	state.SetItemsProcessed(state.iterations() * 14); // 14 byte extractions
}
BENCHMARK(BM_LynxSuzy_PeekMathRegisters);

/// [12.23/#405] SPRSYS register read — combined flag packing in PeekRegister.
static void BM_LynxSuzy_SprsysRead(benchmark::State& state) {
	bool mathInProgress = false;
	bool mathOverflow = false;
	bool lastCarry = false;
	bool unsafeAccess = false;
	bool vstretch = true;
	bool leftHand = false;
	bool stopOnCurrent = false;
	bool spriteBusy = true;
	for (auto _ : state) {
		uint8_t sprsys = 0;
		if (mathInProgress) sprsys |= 0x80;
		if (mathOverflow)   sprsys |= 0x40;
		if (lastCarry)      sprsys |= 0x20;
		if (vstretch)       sprsys |= 0x10;
		if (leftHand)       sprsys |= 0x08;
		if (unsafeAccess)   sprsys |= 0x04;
		if (stopOnCurrent)  sprsys |= 0x02;
		if (spriteBusy)     sprsys |= 0x01;
		benchmark::DoNotOptimize(sprsys);
		mathInProgress = !mathInProgress;
		mathOverflow = !mathOverflow;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_SprsysRead);

// =============================================================================
// Sprite Decode Benchmarks
// =============================================================================

// Benchmark per-sprite SCB field decode + pixel line decode throughput.
// Simulates reading an SCB header (control, collision, next pointer, position,
// size) and decoding one packed 4-BPP sprite line into a pixel buffer.
// This isolates the per-sprite data processing overhead.
static void BM_LynxSuzy_SpriteLineDecode(benchmark::State& state) {
	// Simulated SCB header in work RAM (5 bytes of control + position fields)
	uint8_t sprctl0 = 0xC4; // 4bpp, normal type
	uint8_t sprcoll = 0x03; // collision number 3
	int16_t hpos = 80;
	uint16_t hsize = 0x0200; // 2x scale (8.8 fixed)

	// Simulated packed 4-BPP sprite data (16 pixels per line)
	// Format: literal mode, each nibble = 1 pixel index
	static constexpr uint8_t spriteData[8] = {
		0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
	};
	uint8_t pixelBuf[160]; // output scanline

	for (auto _ : state) {
		// Phase 1: SCB field decode
		int bpp = ((sprctl0 >> 6) & 0x03) + 1; // 1-4 bpp from bits 7:6
		bool hflip = (sprctl0 & 0x20) != 0;
		bool vflip = (sprctl0 & 0x10) != 0;
		uint8_t spriteType = sprctl0 & 0x07;
		uint8_t collNum = sprcoll & 0x0f;
		bool dontCollide = (sprcoll & 0x20) != 0;

		benchmark::DoNotOptimize(bpp);
		benchmark::DoNotOptimize(hflip);
		benchmark::DoNotOptimize(vflip);
		benchmark::DoNotOptimize(spriteType);
		benchmark::DoNotOptimize(collNum);
		benchmark::DoNotOptimize(dontCollide);

		// Phase 2: Decode one 4-BPP packed sprite scanline (16 pixels)
		int pixelCount = 0;
		if (bpp == 4) {
			// 4-BPP literal: each byte = 2 pixels (high nibble, low nibble)
			for (int i = 0; i < 8 && pixelCount < 16; i++) {
				pixelBuf[pixelCount++] = (spriteData[i] >> 4) & 0x0f;
				pixelBuf[pixelCount++] = spriteData[i] & 0x0f;
			}
		} else {
			// Simplified BPP decode for other depths
			uint8_t mask = (1 << bpp) - 1;
			int bitsRemaining = 0;
			uint16_t bitBuffer = 0;
			for (int i = 0; i < 8 && pixelCount < 16; i++) {
				bitBuffer = (bitBuffer << 8) | spriteData[i];
				bitsRemaining += 8;
				while (bitsRemaining >= bpp && pixelCount < 16) {
					bitsRemaining -= bpp;
					pixelBuf[pixelCount++] = (bitBuffer >> bitsRemaining) & mask;
				}
			}
		}

		// Phase 3: Apply scaling per pixel (hsize 8.8 fixed-point accumulation)
		uint16_t hAccum = 0;
		int screenPixels = 0;
		for (int p = 0; p < pixelCount; p++) {
			hAccum += hsize;
			int wholePixels = hAccum >> 8;
			hAccum &= 0xff;
			screenPixels += wholePixels;
		}

		benchmark::DoNotOptimize(pixelBuf);
		benchmark::DoNotOptimize(screenPixels);

		// Vary inputs to prevent dead-code elimination
		sprctl0 ^= 0x01;
		hpos += 1;
	}
	state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LynxSuzy_SpriteLineDecode);
