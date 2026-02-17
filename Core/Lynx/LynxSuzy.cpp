#include "pch.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxCart.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

void LynxSuzy::Init(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager, LynxCart* cart) {
	_emu = emu;
	_console = console;
	_memoryManager = memoryManager;
	_cart = cart;

	memset(&_state, 0, sizeof(_state));
	memset(_lineBuffer, 0, sizeof(_lineBuffer));

	_state.Joystick = 0xff; // All buttons released (active-low)
	_state.Switches = 0xff;
}

__forceinline uint8_t LynxSuzy::ReadRam(uint16_t addr) {
	if (_spriteProcessingActive) _spriteBusCycles++;
	return _console->GetWorkRam()[addr & 0xFFFF];
}

__forceinline uint16_t LynxSuzy::ReadRam16(uint16_t addr) {
	uint8_t lo = ReadRam(addr);
	uint8_t hi = ReadRam(addr + 1);
	return (uint16_t)(hi << 8) | lo;
}

void LynxSuzy::DoMultiply() {
	// 16x16 → 32-bit multiply
	// MATHC:MATHD × MATHE:MATHF → MATHG:MATHH:MATHJ:MATHK
	_state.MathInProgress = true;

	// HW Bug 13.10: The math overflow flag is OVERWRITTEN by each new operation,
	// not OR'd. A previous overflow is lost if the CPU doesn't read SPRSYS before
	// the next math operation completes. We clear it at the start of each operation.
	_state.MathOverflow = false;
	_state.LastCarry = false;

	if (_state.MathSign) {
		// Signed multiply
		// HW Bug 13.8: The hardware sign-detection logic has two errors:
		//   1. $8000 is treated as POSITIVE (the sign bit is checked, but the
		//      negate-and-complement produces $8000 again, so it stays positive)
		//   2. $0000 with the sign flag is treated as NEGATIVE (negate of 0 =
		//      $10000 which truncates to $0000, but sign is still set)
		// We emulate this by converting to sign-magnitude the way the HW does:
		//   - Check bit 15 of each operand for sign
		//   - If set, negate (two's complement) the value
		//   - Multiply as unsigned
		//   - If signs differ, negate the result
		// The bug manifests because negating $8000 = $8000 (positive $8000)
		// and negating $0000 = $0000 but still flagged as negative.
		uint16_t a = (uint16_t)_state.MathC;
		uint16_t b = (uint16_t)_state.MathE;
		bool aNeg = (a & 0x8000) != 0;
		bool bNeg = (b & 0x8000) != 0;

		// Hardware performs two's complement if sign bit set
		// Bug: ~$8000 + 1 = $7FFF + 1 = $8000 (unchanged, treated as positive magnitude)
		// Bug: ~$0000 + 1 = $FFFF + 1 = $0000 (truncated, but sign still flagged)
		if (aNeg) a = (uint16_t)(~a + 1);
		if (bNeg) b = (uint16_t)(~b + 1);

		uint32_t result = (uint32_t)a * (uint32_t)b;

		// If signs differ, negate the result
		bool resultNeg = aNeg ^ bNeg;
		if (resultNeg) {
			result = ~result + 1;
		}

		if (_state.MathAccumulate) {
			uint32_t accum = ((uint32_t)_state.MathG << 16) | _state.MathH;
			uint64_t sum64 = (uint64_t)result + (uint64_t)accum;
			if (sum64 > 0xFFFFFFFF) {
				_state.MathOverflow = true;
				_state.LastCarry = true;
			}
			result = (uint32_t)sum64;
		}

		_state.MathG = (uint16_t)((result >> 16) & 0xffff);
		_state.MathH = (uint16_t)(result & 0xffff);
	} else {
		// Unsigned multiply
		uint32_t a = (uint16_t)_state.MathC;
		uint32_t b = (uint16_t)_state.MathE;
		uint32_t result = a * b;

		if (_state.MathAccumulate) {
			uint32_t accum = ((uint32_t)_state.MathG << 16) | _state.MathH;
			uint64_t sum64 = (uint64_t)result + (uint64_t)accum;
			if (sum64 > 0xFFFFFFFF) {
				_state.MathOverflow = true;
				_state.LastCarry = true;
			}
			result = (uint32_t)sum64;
		}

		_state.MathG = (uint16_t)((result >> 16) & 0xffff);
		_state.MathH = (uint16_t)(result & 0xffff);
	}

	_state.MathInProgress = false;
}

void LynxSuzy::DoDivide() {
	// 32÷16 → 16 quotient, 16 remainder
	// MATHG:MATHH ÷ MATHE → MATHC (quotient), MATHG (remainder)
	_state.MathInProgress = true;

	if (_state.MathE == 0) {
		// Division by zero
		_state.MathC = 0;
		_state.MathD = 0;
		_state.MathG = 0;
		_state.MathH = 0;
	} else if (_state.MathSign) {
		// HW Bug 13.9: Signed division remainder errors.
		// The hardware performs sign-magnitude division similar to multiply:
		// 1. Extract signs, negate to positive magnitude
		// 2. Divide as unsigned
		// 3. Negate quotient if signs differ
		// 4. The remainder follows the dividend's sign, BUT has the same
		//    $8000/$0000 bugs as multiply (Bug 13.8).
		uint32_t dividend = ((uint32_t)_state.MathG << 16) | (uint16_t)_state.MathH;
		uint16_t divisor = _state.MathE;
		bool dividendNeg = (dividend & 0x80000000) != 0;
		bool divisorNeg = (divisor & 0x8000) != 0;

		// Two's complement like the hardware does (same $8000 bug as multiply)
		if (dividendNeg) dividend = ~dividend + 1;
		if (divisorNeg) divisor = (uint16_t)(~divisor + 1);

		uint32_t quotient = dividend / (uint32_t)divisor;
		uint32_t remainder = dividend % (uint32_t)divisor;

		// Negate quotient if signs differ
		if (dividendNeg ^ divisorNeg) {
			quotient = ~quotient + 1;
		}

		// HW Bug 13.9: Remainder should follow dividend sign but the hardware
		// doesn't always negate it correctly — remainder is always positive
		// magnitude from the unsigned division. We match the hardware behavior:
		// remainder is NOT negated, regardless of dividend sign.

		_state.MathC = (int16_t)(quotient & 0xffff);
		_state.MathD = (int16_t)((quotient >> 16) & 0xffff);
		_state.MathG = (uint16_t)(remainder & 0xffff);
		_state.MathH = 0;
	} else {
		uint32_t dividend = ((uint32_t)_state.MathG << 16) | (uint16_t)_state.MathH;
		uint16_t divisor = _state.MathE;
		uint32_t quotient = dividend / divisor;
		uint32_t remainder = dividend % divisor;

		_state.MathC = (int16_t)(quotient & 0xffff);
		_state.MathD = (int16_t)((quotient >> 16) & 0xffff);
		_state.MathG = (uint16_t)(remainder & 0xffff);
		_state.MathH = 0;
	}

	_state.MathInProgress = false;
}

void LynxSuzy::ProcessSpriteChain() {
	if (!_state.SpriteEnabled) {
		_state.SpriteBusy = false;
		return;
	}

	_state.SpriteBusy = true;
	_spriteBusCycles = 0;
	_spriteProcessingActive = true;
	uint16_t scbAddr = _state.SCBAddress;

	// Walk the sprite linked list
	int spriteCount = 0;
	// HW Bug 13.12: The hardware only checks the UPPER BYTE of the SCB NEXT
	// address for zero to terminate the sprite chain. If the upper byte is
	// zero but the lower byte is non-zero (e.g., $0080), the chain still
	// terminates. Conversely, $0100 would NOT terminate (upper byte = $01).
	while ((scbAddr >> 8) != 0 && spriteCount < 256) { // Safety limit
		ProcessSprite(scbAddr);
		// Read next SCB pointer from SCB offset 3-4 (after CTL0, CTL1, COLL)
		scbAddr = ReadRam16(scbAddr + 3);
		spriteCount++;
	}

	_spriteProcessingActive = false;
	_state.SpriteBusy = false;

	// Apply bus contention -- stall CPU for cycles consumed by sprite processing.
	// On real hardware, the CPU is halted while Suzy owns the bus.
	// Each byte-wide bus access costs ~1 CPU cycle (5 master clocks / 4).
	if (_spriteBusCycles > 0) {
		_console->GetCpu()->AddCycles(_spriteBusCycles);
	}
}

void LynxSuzy::ProcessSprite(uint16_t scbAddr) {
	// SCB Layout (matching Handy/hardware):
	// Offset 0:    SPRCTL0 — sprite type, BPP, H/V flip
	// Offset 1:    SPRCTL1 — skip, reload, sizing, literal
	// Offset 2:    SPRCOLL — collision number and flags
	// Offset 3-4:  SCBNEXT — link to next SCB (read in ProcessSpriteChain)
	// Offset 5-6:  SPRDLINE — sprite data pointer (always loaded)
	// Offset 7-8:  HPOSSTRT — horizontal position (always loaded)
	// Offset 9-10: VPOSSTRT — vertical position (always loaded)
	// Offset 11+:  Variable-length optional fields:
	//   If ReloadDepth >= 1: SPRHSIZ(2), SPRVSIZ(2)
	//   If ReloadDepth >= 2: STRETCH(2)
	//   If ReloadDepth >= 3: TILT(2)
	//   If ReloadPalette:    PALETTE(8)

	uint8_t sprCtl0 = ReadRam(scbAddr);       // Offset 0: SPRCTL0
	uint8_t sprCtl1 = ReadRam(scbAddr + 1);   // Offset 1: SPRCTL1
	uint8_t sprColl = ReadRam(scbAddr + 2);   // Offset 2: SPRCOLL

	// Check for skip flag (SPRCTL1 bit 2: "skip this sprite")
	if (sprCtl1 & 0x04) {
		return;
	}

	// SPRCTL0 decoding:
	// Bits 7:6 = BPP (0=1bpp, 1=2bpp, 2=3bpp, 3=4bpp)
	// Bit 5 = H-flip (0 = left-to-right, 1 = right-to-left)
	// Bit 4 = V-flip (0 = top-to-bottom, 1 = bottom-to-top)
	// Bits 2:0 = Sprite type (0-7)
	int bpp = ((sprCtl0 >> 6) & 0x03) + 1;
	LynxSpriteType spriteType = static_cast<LynxSpriteType>(sprCtl0 & 0x07);
	bool hFlip = (sprCtl0 & 0x20) != 0;
	bool vFlip = (sprCtl0 & 0x10) != 0;

	// SPRCTL1 decoding (Handy/hardware bit layout):
	// Bit 0: StartLeft — start drawing quadrant (left side)
	// Bit 1: StartUp — start drawing quadrant (upper side)
	// Bit 2: SkipSprite — handled above
	// Bit 3: ReloadPalette — 0 = reload from SCB, 1 = reuse previous
	// Bits 5:4: ReloadDepth — 0=none, 1=size, 2=size+stretch, 3=size+stretch+tilt
	// Bit 6: Sizing enable
	// Bit 7: Literal mode — 1 = raw pixel data (no RLE packets)
	bool reloadPalette = (sprCtl1 & 0x08) == 0;   // Bit 3: 0 means reload
	int reloadDepth    = (sprCtl1 >> 4) & 0x03;    // Bits 5:4
	bool literalMode   = (sprCtl1 & 0x80) != 0;    // Bit 7

	// SPRCOLL decoding:
	// Bits 3:0 = Collision number (0-15)
	// Bit 5 = Don't collide flag
	uint8_t collNum = sprColl & 0x0f;

	// Read always-present fields from SCB (after fixed 5-byte header):
	// Data pointer at offset 5-6
	uint16_t dataAddr = ReadRam16(scbAddr + 5);
	// Position at offset 7-8, 9-10 (always loaded)
	_persistHpos = (int16_t)ReadRam16(scbAddr + 7);
	_persistVpos = (int16_t)ReadRam16(scbAddr + 9);

	// Variable-length fields start at offset 11.
	// Which fields are present depends on ReloadDepth and ReloadPalette.
	// The walking pointer advances only for fields actually present in the SCB.
	int scbOffset = 11;

	// Load size if ReloadDepth >= 1
	if (reloadDepth >= 1) {
		_persistHsize = ReadRam16(scbAddr + scbOffset);
		_persistVsize = ReadRam16(scbAddr + scbOffset + 2);
		scbOffset += 4;
	}

	// Load stretch if ReloadDepth >= 2
	if (reloadDepth >= 2) {
		_persistStretch = (int16_t)ReadRam16(scbAddr + scbOffset);
		scbOffset += 2;
	}

	// Load tilt if ReloadDepth >= 3
	if (reloadDepth >= 3) {
		_persistTilt = (int16_t)ReadRam16(scbAddr + scbOffset);
		scbOffset += 2;
	}

	// Load palette remap (8 bytes = 16 nibble entries for pen index remap)
	// Only present in SCB when ReloadPalette is true (SPRCTL1 bit 3 = 0)
	if (reloadPalette) {
		for (int i = 0; i < 8; i++) {
			uint8_t byte = ReadRam(scbAddr + scbOffset + i);
			_penIndex[i * 2] = byte >> 4;
			_penIndex[i * 2 + 1] = byte & 0x0f;
		}
	}

	int16_t hpos = _persistHpos;
	int16_t vpos = _persistVpos;

	// 8.8 fixed-point scaling: integer part = pixels per sprite texel
	// For unscaled sprites, hsize/vsize = 0x0100 (1.0)
	// The fractional part (lower 8 bits) allows sub-pixel positioning
	// and enables hardware to draw sprites at arbitrary scales.
	// Example: 0x0180 = 1.5x scale, 0x0040 = 0.25x scale
	int16_t stretch = _persistStretch; // Per-scanline horizontal size delta (8.8 fixed-point)
	int16_t tilt = _persistTilt;       // Per-scanline horizontal offset delta (8.8 fixed-point)
	// Stretch creates trapezoid/triangle shapes: each line is wider/narrower
	// Tilt creates parallelogram/italic shapes: each line is offset horizontally

	// Process each scanline of sprite data
	uint16_t currentDataAddr = dataAddr;
	int vDir = vFlip ? -1 : 1;
	int hDir = hFlip ? -1 : 1;

	// For stretch/tilt: accumulate across lines
	// hsize changes by stretch each line, hpos changes by tilt each line
	uint16_t currentHsize = _persistHsize;
	int32_t currentHposAccum = (int32_t)hpos << 8; // 8.8 fixed-point accumulator
	int32_t tiltAccum = 0;

	for (int line = 0; line < 256; line++) {
		// Read the line header — contains the byte count for this line
		uint8_t lineHeader = ReadRam(currentDataAddr);
		currentDataAddr++;

		if (lineHeader == 0) {
			break; // End of sprite data (0-length line terminates)
		}

		int lineY = vpos + (vDir * line);

		// lineHeader is the total byte count for this line including the header
		int dataBytes = lineHeader - 1;
		uint16_t lineEnd = currentDataAddr + dataBytes;

		if (lineY < 0 || lineY >= (int)LynxConstants::ScreenHeight || dataBytes <= 0) {
			// Off-screen or empty — skip data but advance pointer
			currentDataAddr = lineEnd;
			// Still apply stretch and tilt for off-screen lines
			currentHsize = (uint16_t)((int32_t)currentHsize + stretch);
			tiltAccum += tilt;
			continue;
		}

		// Decode pixel data for this line
		uint8_t pixelBuf[512]; // Max decoded pixels for one line
		int pixelCount = DecodeSpriteLinePixels(currentDataAddr, lineEnd, bpp, literalMode, pixelBuf, 512);
		currentDataAddr = lineEnd;

		// Calculate effective hpos for this line (with tilt)
		int32_t lineHpos = (int32_t)hpos + (tiltAccum >> 8);

		// Calculate horizontal scale factor: pixels per texel (8.8 fixed-point)
		int32_t hscale = currentHsize; // 8.8 fixed-point

		// Render scaled pixels
		if (hscale == 0x0100) {
			// Unscaled: 1:1 mapping
			for (int px = 0; px < pixelCount; px++) {
				uint8_t penRaw = pixelBuf[px];
				if (penRaw == 0) continue; // Transparent

				uint8_t penMapped = _penIndex[penRaw & 0x0f];
				int drawX = (int)lineHpos + (hDir * px);

				WriteSpritePixel(drawX, lineY, penMapped, collNum, spriteType);
			}
		} else {
			// Scaled: walk source pixels, each covering hscale/256 output pixels
			int32_t xAccum = 0; // 8.8 accumulator for sub-pixel position
			for (int px = 0; px < pixelCount; px++) {
				uint8_t penRaw = pixelBuf[px];
				uint8_t penMapped = _penIndex[penRaw & 0x0f];

				// How many output pixels this source pixel covers
				int32_t nextAccum = xAccum + hscale;
				int startOut = xAccum >> 8;
				int endOut = nextAccum >> 8;

				if (penRaw != 0) {
					for (int outPx = startOut; outPx < endOut; outPx++) {
						int drawX = (int)lineHpos + (hDir * outPx);
						WriteSpritePixel(drawX, lineY, penMapped, collNum, spriteType);
					}
				}

				xAccum = nextAccum;
			}
		}

		// Apply stretch and tilt for next line
		currentHsize = (uint16_t)((int32_t)currentHsize + stretch);
		tiltAccum += tilt;
	}
}

/// <summary>
/// Decode one line of sprite pixel data.
///
/// The Lynx sprite engine supports two data formats (controlled by SPRCTL1 bit 7):
///
/// **Literal mode** (bit 7 = 1): All pixel data is raw linear bpp-wide values.
/// Each pixel is simply the next `bpp` bits from the data stream. No packet
/// structure, no run-length encoding.
///
/// **Packed mode** (bit 7 = 0): Uses a packetized format with RLE compression.
/// Each packet starts with a 1-bit flag:
///   - 1 = literal packet: 4-bit count, then count+1 literal pixel values (each bpp bits)
///   - 0 = packed (repeat) packet: 4-bit count, then one bpp-wide pixel repeated count+1 times
///     If count = 0 in a packed packet, it signals end-of-line.
///
/// In both modes, the line offset byte (already consumed by caller) gives the
/// total byte length of this line's data, limiting how many bits can be read.
/// </summary>
int LynxSuzy::DecodeSpriteLinePixels(uint16_t& dataAddr, uint16_t lineEnd, int bpp, bool literalMode, uint8_t* pixelBuf, int maxPixels) {
	int pixelCount = 0;
	uint8_t bppMask = (1 << bpp) - 1;

	// Bit-level reading state
	uint32_t shiftReg = 0;
	int shiftRegCount = 0;
	int totalBitsLeft = (int)(lineEnd - dataAddr) * 8;

	// Lambda to read N bits from the data stream
	auto getBits = [&](int bits) -> uint8_t {
		if (totalBitsLeft <= bits) {
			return 0; // No more data (matches Handy's <= check for demo006 fix)
		}

		// Refill shift register if needed
		while (shiftRegCount < bits && dataAddr < lineEnd) {
			shiftReg = (shiftReg << 8) | ReadRam(dataAddr);
			dataAddr++;
			shiftRegCount += 8;
		}

		if (shiftRegCount < bits) return 0;

		shiftRegCount -= bits;
		totalBitsLeft -= bits;
		return (uint8_t)((shiftReg >> shiftRegCount) & ((1 << bits) - 1));
	};

	if (literalMode) {
		// Literal mode: all pixels are raw bpp-wide values, no packet structure.
		// Total pixel count is (total data bits) / bpp.
		int totalPixels = totalBitsLeft / bpp;
		for (int i = 0; i < totalPixels && pixelCount < maxPixels; i++) {
			uint8_t pixel = getBits(bpp);
			pixelBuf[pixelCount++] = pixel;
			// In literal mode, a zero pixel as the very last pixel signals end of data
			// (matching Handy's line_abs_literal handling)
			if (pixelCount == totalPixels && pixel == 0) {
				pixelCount--; // Don't include trailing zero
				break;
			}
		}
	} else {
		// Packed mode: packetized data with literal and repeat packets
		while (totalBitsLeft > 0 && pixelCount < maxPixels) {
			// Read 1-bit literal flag
			uint8_t isLiteral = getBits(1);
			if (totalBitsLeft <= 0) break;

			// Read 4-bit count
			uint8_t count = getBits(4);

			if (!isLiteral && count == 0) {
				// Packed packet with count=0 = end of line
				break;
			}

			count++; // Actual count is stored count + 1

			if (isLiteral) {
				// Literal packet: read 'count' individual pixel values
				for (int i = 0; i < count && pixelCount < maxPixels; i++) {
					pixelBuf[pixelCount++] = getBits(bpp);
				}
			} else {
				// Packed (repeat) packet: read one pixel value, repeat 'count' times
				uint8_t pixel = getBits(bpp);
				for (int i = 0; i < count && pixelCount < maxPixels; i++) {
					pixelBuf[pixelCount++] = pixel;
				}
			}
		}
	}

	// Ensure dataAddr advances to lineEnd even if we stopped early
	dataAddr = lineEnd;
	return pixelCount;
}

__forceinline void LynxSuzy::WriteRam(uint16_t addr, uint8_t value) {
	if (_spriteProcessingActive) _spriteBusCycles++;
	_console->GetWorkRam()[addr & 0xFFFF] = value;
}

void LynxSuzy::WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum, LynxSpriteType spriteType) {
	// Bounds check
	if (x < 0 || x >= (int)LynxConstants::ScreenWidth ||
		y < 0 || y >= (int)LynxConstants::ScreenHeight) {
		return;
	}

	// Transparent pixel (index 0) is never drawn
	if (penIndex == 0) {
		return;
	}

	// Non-collidable sprites draw pixels but don't update collision
	bool doCollision = (spriteType != LynxSpriteType::NonCollidable);

	// Background sprites only draw where the pixel is currently 0 (transparent)
	// and don't participate in collision
	if (spriteType == LynxSpriteType::Background) {
		doCollision = false;
	}

	// Shadow sprites: XOR the pixel value with the existing value
	bool isShadow = (spriteType == LynxSpriteType::NormalShadow ||
					 spriteType == LynxSpriteType::BoundaryShadow ||
					 spriteType == LynxSpriteType::XorShadow ||
					 spriteType == LynxSpriteType::Shadow);

	// Write pixel as 4bpp nibble into work RAM frame buffer
	uint16_t dispAddr = _console->GetMikey()->GetState().DisplayAddress;
	uint16_t byteAddr = dispAddr + y * LynxConstants::BytesPerScanline + (x >> 1);
	uint8_t byte = ReadRam(byteAddr);

	uint8_t existingPixel;
	uint8_t writePixel = penIndex & 0x0f;

	if (x & 1) {
		existingPixel = byte & 0x0f;
	} else {
		existingPixel = (byte >> 4) & 0x0f;
	}

	// Background sprite: only draw if existing pixel is transparent
	if (spriteType == LynxSpriteType::Background && existingPixel != 0) {
		return;
	}

	// Shadow/XOR sprites: XOR with existing pixel
	if (isShadow) {
		writePixel = existingPixel ^ writePixel;
	}

	if (x & 1) {
		byte = (byte & 0xF0) | writePixel;
	} else {
		byte = (byte & 0x0F) | (writePixel << 4);
	}
	WriteRam(byteAddr, byte);

	// Collision detection — mutual update algorithm
	// The collision buffer has 16 slots (indices 0-15).
	// When sprite A (collNum) draws over sprite B (existingPixel):
	//   1. Buffer[A] = max(Buffer[A], B) — A records that B collided with it
	//   2. Buffer[B] = max(Buffer[B], A) — B records that A collided with it
	// This ensures both sprites' entries reflect the highest-numbered collider.
	// The game reads Buffer[spriteN] to see what collided with sprite N.
	if (doCollision && collNum > 0 && collNum < LynxConstants::CollisionBufferSize) {
		// Read existing collision value for this sprite's collision number
		uint8_t existing = _state.CollisionBuffer[collNum];
		// Use the pen index of the existing pixel as the collider ID
		// Higher-numbered sprites have priority; only update if existingPixel > current value
		if (existingPixel > 0 && existingPixel > existing) {
			_state.CollisionBuffer[collNum] = existingPixel;
			// Set sticky sprite-to-sprite collision flag (cleared only by explicit write)
			_state.SpriteToSpriteCollision = true;
		}

		// Also update the colliding sprite's entry (mutual)
		if (existingPixel > 0 && existingPixel < LynxConstants::CollisionBufferSize) {
			uint8_t otherExisting = _state.CollisionBuffer[existingPixel];
			if (collNum > otherExisting) {
				_state.CollisionBuffer[existingPixel] = (uint8_t)collNum;
				_state.SpriteToSpriteCollision = true;
			}
		}
	}
}

uint8_t LynxSuzy::ReadRegister(uint8_t addr) {
	switch (addr) {
		// Sprite engine registers
		case 0x88: // SPRCTL0
			return _state.SpriteControl0;
		case 0x89: // SPRCTL1
			return _state.SpriteControl1;
		case 0x8a: // SPRINIT
			return _state.SpriteInit;

		// Sprite engine status
		case 0x90: // SUZYBUSEN — sprite engine busy
			return _state.SpriteBusy ? 0x01 : 0x00;
		case 0x91: // SPRGO
			return _state.SpriteEnabled ? 0x01 : 0x00;
		case 0x92: // SPRSYS — system status (read)
			return (_state.MathInProgress ? 0x01 : 0x00) |     // Bit 0: math in progress
				(_state.SpriteBusy ? 0x02 : 0x00) |              // Bit 1: sprite busy
				(_state.LastCarry ? 0x04 : 0x00) |               // Bit 2: last carry
				(_state.UnsafeAccess ? 0x08 : 0x00) |            // Bit 3: unsafe access
				(_state.SpriteToSpriteCollision ? 0x10 : 0x00) | // Bit 4: sprite-to-sprite collision
				(_state.MathOverflow ? 0x20 : 0x00);             // Bit 5: math overflow

		// SCB address
		case 0x10: return (uint8_t)(_state.SCBAddress & 0xff);
		case 0x11: return (uint8_t)((_state.SCBAddress >> 8) & 0xff);

		// Math registers (read result)
		case 0x60: return (uint8_t)(_state.MathC & 0xff);
		case 0x61: return (uint8_t)((_state.MathC >> 8) & 0xff);
		case 0x62: return (uint8_t)(_state.MathD & 0xff);
		case 0x63: return (uint8_t)((_state.MathD >> 8) & 0xff);
		case 0x64: return (uint8_t)(_state.MathE & 0xff);
		case 0x65: return (uint8_t)((_state.MathE >> 8) & 0xff);
		case 0x66: return (uint8_t)(_state.MathF & 0xff);
		case 0x67: return (uint8_t)((_state.MathF >> 8) & 0xff);
		case 0x6c: return (uint8_t)(_state.MathG & 0xff);
		case 0x6d: return (uint8_t)((_state.MathG >> 8) & 0xff);
		case 0x6e: return (uint8_t)(_state.MathH & 0xff);
		case 0x6f: return (uint8_t)((_state.MathH >> 8) & 0xff);
		case 0x70: return (uint8_t)(_state.MathJ & 0xff);
		case 0x71: return (uint8_t)((_state.MathJ >> 8) & 0xff);
		case 0x72: return (uint8_t)(_state.MathK & 0xff);
		case 0x73: return (uint8_t)((_state.MathK >> 8) & 0xff);

		// Collision depository: 16 slots at Suzy offsets 0x00-0x0F
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x06: case 0x07:
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			return _state.CollisionBuffer[addr];

		// Joystick / switches
		case 0xb0: // JOYSTICK
			return _state.Joystick;
		case 0xb1: // SWITCHES
			return _state.Switches;

		// Cart access registers
		case 0xa0: // RCART0 — read from cart bank 0 (auto-increment)
			if (_cart) {
				_cart->SelectBank(0);
				return _cart->ReadData();
			}
			return 0xff;
		case 0xa2: // RCART1 — read from cart bank 1 (auto-increment)
			if (_cart) {
				_cart->SelectBank(1);
				return _cart->ReadData();
			}
			return 0xff;

		default:
			return 0xff;
	}
}

void LynxSuzy::WriteRegister(uint8_t addr, uint8_t value) {
	switch (addr) {
		// Sprite engine
		case 0x88: // SPRCTL0
			_state.SpriteControl0 = value;
			break;
		case 0x89: // SPRCTL1
			_state.SpriteControl1 = value;
			break;
		case 0x8a: // SPRINIT
			_state.SpriteInit = value;
			break;

		// Sprite go
		case 0x91: // SPRGO — write 1 starts sprite engine
			_state.SpriteEnabled = (value & 0x01) != 0;
			if (_state.SpriteEnabled) {
				ProcessSpriteChain();
			}
			break;
		case 0x92: // SPRSYS — write control bits
			_state.MathSign = (value & 0x01) != 0;       // Bit 0: signed math
			_state.MathAccumulate = (value & 0x02) != 0;  // Bit 1: accumulate mode
			_state.VStretch = (value & 0x10) != 0;        // Bit 4: vertical stretch
			_state.LeftHand = (value & 0x20) != 0;        // Bit 5: left-handed controller
			if (value & 0x40) _state.UnsafeAccess = false;            // Bit 6: clear unsafe access
			if (value & 0x80) _state.SpriteToSpriteCollision = false; // Bit 7: clear collision flag
			break;

		// SCB address
		case 0x10:
			_state.SCBAddress = (_state.SCBAddress & 0xff00) | value;
			break;
		case 0x11:
			_state.SCBAddress = (_state.SCBAddress & 0x00ff) | ((uint16_t)value << 8);
			break;

		// Math registers (write operands)
		case 0x60: _state.MathC = (_state.MathC & (int16_t)0xff00) | value; break;
		case 0x61: _state.MathC = (_state.MathC & 0x00ff) | ((int16_t)value << 8); break;
		case 0x62: _state.MathD = (_state.MathD & (int16_t)0xff00) | value; break;
		case 0x63: _state.MathD = (_state.MathD & 0x00ff) | ((int16_t)value << 8); break;
		case 0x64: _state.MathE = (_state.MathE & 0xff00) | value; break;
		case 0x65: _state.MathE = (_state.MathE & 0x00ff) | ((uint16_t)value << 8); break;
		case 0x66: _state.MathF = (_state.MathF & 0xff00) | value; break;
		case 0x67:
			_state.MathF = (_state.MathF & 0x00ff) | ((uint16_t)value << 8);
			// Writing to MATHF high triggers multiply
			DoMultiply();
			break;

		// Writing to MATHK high triggers divide
		case 0x72: _state.MathK = (_state.MathK & 0xff00) | value; break;
		case 0x73:
			_state.MathK = (_state.MathK & 0x00ff) | ((uint16_t)value << 8);
			DoDivide();
			break;

		case 0x6c: _state.MathG = (_state.MathG & 0xff00) | value; break;
		case 0x6d: _state.MathG = (_state.MathG & 0x00ff) | ((uint16_t)value << 8); break;
		case 0x6e: _state.MathH = (_state.MathH & 0xff00) | value; break;
		case 0x6f: _state.MathH = (_state.MathH & 0x00ff) | ((uint16_t)value << 8); break;
		case 0x70: _state.MathJ = (_state.MathJ & 0xff00) | value; break;
		case 0x71: _state.MathJ = (_state.MathJ & 0x00ff) | ((uint16_t)value << 8); break;

		// Collision depository writes: 16 slots at Suzy offsets 0x00-0x0F
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x06: case 0x07:
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			_state.CollisionBuffer[addr] = value;
			break;

		// Cart address / bank registers
		case 0xa0: // RCART0 — write address low for bank 0
			if (_cart) _cart->SetAddressLow(value);
			break;
		case 0xa1: // RCART0 high — write address high for bank 0
			if (_cart) _cart->SetAddressHigh(value);
			break;
		case 0xa2: // RCART1 — write address low for bank 1
			if (_cart) _cart->SetAddressLow(value);
			break;
		case 0xa3: // RCART1 high — write address high for bank 1
			if (_cart) _cart->SetAddressHigh(value);
			break;
		case 0xb2: // CART0 — cart bank 0 strobe / shift register
			if (_cart) {
				_cart->SelectBank(0);
				_cart->WriteShiftRegister(value);
			}
			break;
		case 0xb3: // CART1 — cart bank 1 strobe / shift register
			if (_cart) {
				_cart->SelectBank(1);
				_cart->WriteShiftRegister(value);
			}
			break;

		default:
			break;
	}
}

void LynxSuzy::Serialize(Serializer& s) {
	// Sprite engine
	SV(_state.SCBAddress);
	SV(_state.SpriteControl0);
	SV(_state.SpriteControl1);
	SV(_state.SpriteInit);
	SV(_state.SpriteBusy);
	SV(_state.SpriteEnabled);

	// Math
	SV(_state.MathA);
	SV(_state.MathB);
	SV(_state.MathC);
	SV(_state.MathD);
	SV(_state.MathE);
	SV(_state.MathF);
	SV(_state.MathG);
	SV(_state.MathH);
	SV(_state.MathJ);
	SV(_state.MathK);
	SV(_state.MathM);
	SV(_state.MathN);
	SV(_state.MathSign);
	SV(_state.MathAccumulate);
	SV(_state.MathInProgress);
	SV(_state.MathOverflow);
	SV(_state.LastCarry);
	SV(_state.UnsafeAccess);
	SV(_state.SpriteToSpriteCollision);
	SV(_state.VStretch);
	SV(_state.LeftHand);

	// Collision
	SVArray(_state.CollisionBuffer, LynxConstants::CollisionBufferSize);

	// Input
	SV(_state.Joystick);
	SV(_state.Switches);

	// Pen index remap table
	SVArray(_penIndex, 16);

	// Persistent SCB fields (reused across sprites when reload flags clear)
	SV(_persistHpos);
	SV(_persistVpos);
	SV(_persistHsize);
	SV(_persistVsize);
	SV(_persistStretch);
	SV(_persistTilt);
}
