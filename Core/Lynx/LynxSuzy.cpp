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

	_state = {};
	std::fill_n(_lineBuffer, LynxConstants::ScreenWidth, uint8_t{0});

	_state.Joystick = 0xff; // All buttons released (active-low)
	_state.Switches = 0xff;

	// Hardware defaults (matching Handy's Reset)
	_state.HSizeOff = 0x007f;
	_state.VSizeOff = 0x007f;

	// Handy initializes math registers to 0xFFFFFFFF due to
	// stun runner math initialization bug (see Handy whatsnew v0.7)
	_state.MathABCD = 0xffffffff;
	_state.MathEFGH = 0xffffffff;
	_state.MathJKLM = 0xffffffff;
	_state.MathNP = 0xffff;
	_state.MathAB_sign = 1;
	_state.MathCD_sign = 1;
	_state.MathEFGH_sign = 1;
}

__forceinline uint8_t LynxSuzy::ReadRam(uint16_t addr) {
	if (_spriteProcessingActive) _spriteBusCycles++;
	return _console->GetWorkRam()[addr & 0xFFFF];
}

__forceinline uint16_t LynxSuzy::ReadRam16(uint16_t addr) {
	uint8_t lo = ReadRam(addr);
	uint8_t hi = ReadRam(addr + 1);
	return static_cast<uint16_t>(hi << 8) | lo;
}

void LynxSuzy::DoMultiply() {
	// AB × CD → EFGH (matching Handy DoMathMultiply)
	// Sign conversion has ALREADY been applied at register write time.
	// The values in ABCD are positive magnitude; signs tracked separately.
	_state.MathInProgress = true;
	_state.MathOverflow = false;
	_state.LastCarry = false;

	// Basic multiply is ALWAYS unsigned
	uint16_t ab = static_cast<uint16_t>((_state.MathABCD >> 16) & 0xffff);
	uint16_t cd = static_cast<uint16_t>(_state.MathABCD & 0xffff);
	uint32_t result = static_cast<uint32_t>(ab) * static_cast<uint32_t>(cd);
	_state.MathEFGH = result;

	if (_state.MathSign) {
		// Add the sign bits, only >0 is +ve result (matching Handy)
		_state.MathEFGH_sign = _state.MathAB_sign + _state.MathCD_sign;
		if (!_state.MathEFGH_sign) {
			_state.MathEFGH ^= 0xffffffff;
			_state.MathEFGH++;
		}
	}

	// Accumulate: JKLM += EFGH
	if (_state.MathAccumulate) {
		uint32_t tmp = _state.MathJKLM + _state.MathEFGH;
		// Overflow if result wrapped around
		if (tmp < _state.MathJKLM) {
			_state.MathOverflow = true;
			_state.LastCarry = true;
		}
		_state.MathJKLM = tmp;
	}

	_state.MathInProgress = false;
}

void LynxSuzy::DoDivide() {
	// EFGH ÷ NP → quotient in ABCD, remainder in JKLM (matching Handy)
	// Divide is ALWAYS unsigned arithmetic.
	_state.MathInProgress = true;
	_state.MathOverflow = false;
	_state.LastCarry = false;

	if (_state.MathNP) {
		_state.MathABCD = _state.MathEFGH / static_cast<uint32_t>(_state.MathNP);
		_state.MathJKLM = _state.MathEFGH % static_cast<uint32_t>(_state.MathNP);
	} else {
		// Division by zero
		_state.MathABCD = 0xffffffff;
		_state.MathJKLM = 0;
		_state.MathOverflow = true;
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
	// Offset 1:    SPRCTL1 — skip, reload, sizing, literal, quadrant
	// Offset 2:    SPRCOLL — collision number and flags
	// Offset 3-4:  SCBNEXT — link to next SCB (read in ProcessSpriteChain)
	// Offset 5-6:  SPRDLINE — sprite data pointer (always loaded)
	// Offset 7-8:  HPOSSTRT — horizontal position (always loaded)
	// Offset 9-10: VPOSSTRT — vertical position (always loaded)
	// Offset 11+:  Variable-length optional fields based on ReloadDepth

	uint8_t sprCtl0 = ReadRam(scbAddr);       // Offset 0: SPRCTL0
	uint8_t sprCtl1 = ReadRam(scbAddr + 1);   // Offset 1: SPRCTL1
	uint8_t sprColl = ReadRam(scbAddr + 2);   // Offset 2: SPRCOLL

	// SPRCTL1 bit 2: skip this sprite in the chain
	if (sprCtl1 & 0x04) {
		return;
	}

	// === SPRCTL0 decoding ===
	// Bits 7:6 = BPP: 00=1bpp, 01=2bpp, 10=3bpp, 11=4bpp
	// Bit 5 = H-flip (mirror horizontally)
	// Bit 4 = V-flip (mirror vertically)
	// Bits 2:0 = Sprite type (0-7)
	int bpp = ((sprCtl0 >> 6) & 0x03) + 1;
	LynxSpriteType spriteType = static_cast<LynxSpriteType>(sprCtl0 & 0x07);
	bool hFlip = (sprCtl0 & 0x20) != 0;
	bool vFlip = (sprCtl0 & 0x10) != 0;

	// === SPRCTL1 decoding (Handy/hardware bit layout) ===
	// Bit 0: StartLeft — quadrant start (left side)
	// Bit 1: StartUp — quadrant start (upper side)
	// Bit 3: ReloadPalette — 0 = reload from SCB, 1 = skip
	// Bits 5:4: ReloadDepth — 0=none, 1=size, 2=stretch, 3=tilt
	// Bit 7: Literal mode — raw pixel data
	bool startLeft      = (sprCtl1 & 0x01) != 0;
	bool startUp        = (sprCtl1 & 0x02) != 0;
	bool reloadPalette  = (sprCtl1 & 0x08) == 0; // Active low: 0=reload
	int  reloadDepth    = (sprCtl1 >> 4) & 0x03;
	bool literalMode    = (sprCtl1 & 0x80) != 0;

	// === SPRCOLL decoding ===
	uint8_t collNum      = sprColl & 0x0f;       // Collision number (0-15)
	bool    dontCollide  = (sprColl & 0x20) != 0; // Don't participate in collision

	// Determine which SCB fields to enable based on ReloadDepth
	bool enableStretch = false;
	bool enableTilt    = false;

	// Read always-present fields from SCB:
	uint16_t sprDataLine = ReadRam16(scbAddr + 5);   // Sprite data pointer
	_persistHpos = static_cast<int16_t>(ReadRam16(scbAddr + 7));  // Horizontal start position
	_persistVpos = static_cast<int16_t>(ReadRam16(scbAddr + 9));  // Vertical start position

	// Variable-length fields start at offset 11
	int scbOffset = 11;

	switch (reloadDepth) {
		case 3: // Size + stretch + tilt
			enableTilt = true;
			[[fallthrough]];
		case 2: // Size + stretch
			enableStretch = true;
			[[fallthrough]];
		case 1: // Size only
			_persistHsize = ReadRam16(scbAddr + scbOffset);
			_persistVsize = ReadRam16(scbAddr + scbOffset + 2);
			scbOffset += 4;
			if (reloadDepth >= 2) {
				_persistStretch = static_cast<int16_t>(ReadRam16(scbAddr + scbOffset));
				scbOffset += 2;
			}
			if (reloadDepth >= 3) {
				_persistTilt = static_cast<int16_t>(ReadRam16(scbAddr + scbOffset));
				scbOffset += 2;
			}
			break;
		default: // 0 = no optional fields
			break;
	}

	// Load palette remap if ReloadPalette is active
	if (reloadPalette) {
		for (int i = 0; i < 8; i++) {
			uint8_t byte = ReadRam(scbAddr + scbOffset + i);
			_penIndex[i * 2] = byte >> 4;
			_penIndex[i * 2 + 1] = byte & 0x0f;
		}
	}

	// === Quadrant rendering (matching Handy) ===
	// The Lynx renders each sprite in 4 quadrants: SE(0), NE(1), NW(2), SW(3)
	// starting from the quadrant specified by StartLeft/StartUp.
	//
	// Quadrant layout:    2 | 1
	//                    -------
	//                     3 | 0
	//
	// Each quadrant has independent hsign/vsign:
	//   Quadrant 0 (SE): hsign=+1, vsign=+1
	//   Quadrant 1 (NE): hsign=+1, vsign=-1
	//   Quadrant 2 (NW): hsign=-1, vsign=-1
	//   Quadrant 3 (SW): hsign=-1, vsign=+1
	//
	// H/V flip invert the signs.

	// Screen boundaries for clipping
	int screenHStart = static_cast<int>(_state.HOffset);
	int screenHEnd   = static_cast<int>(_state.HOffset) + static_cast<int>(LynxConstants::ScreenWidth);
	int screenVStart = static_cast<int>(_state.VOffset);
	int screenVEnd   = static_cast<int>(_state.VOffset) + static_cast<int>(LynxConstants::ScreenHeight);

	int worldHMid = screenHStart + static_cast<int>(LynxConstants::ScreenWidth) / 2;
	int worldVMid = screenVStart + static_cast<int>(LynxConstants::ScreenHeight) / 2;

	// Determine starting quadrant from SPRCTL1 bits 0-1
	int quadrant;
	if (startLeft) {
		quadrant = startUp ? 2 : 3;
	} else {
		quadrant = startUp ? 1 : 0;
	}

	// Superclipping: if sprite origin is off-screen, only render quadrants
	// that overlap the visible screen area
	int16_t sprH = _persistHpos;
	int16_t sprV = _persistVpos;
	bool superclip = (sprH < screenHStart || sprH >= screenHEnd ||
	                  sprV < screenVStart || sprV >= screenVEnd);

	// Track collision for this sprite
	bool everOnScreen = false;
	_spriteCollision = 0; // Reset max collision for this sprite

	// Current sprite data pointer (advances through quadrants)
	uint16_t currentDataAddr = sprDataLine;

	// Quad offset signs: persist across all 4 quadrants within a sprite.
	// Saves quad 0's sign; subsequent quads drawing in the opposite direction
	// get offset by 1 pixel to prevent the squashed look on multi-quad sprites.
	int vquadoff_sign = 0;
	int hquadoff_sign = 0;

	// Loop over 4 quadrants
	for (int loop = 0; loop < 4; loop++) {
		// Calculate direction signs for this quadrant
		int hsign = (quadrant == 0 || quadrant == 1) ? 1 : -1;
		int vsign = (quadrant == 0 || quadrant == 3) ? 1 : -1;

		// H/V flip inverts the signs
		if (vFlip) vsign = -vsign;
		if (hFlip) hsign = -hsign;

		// Determine whether to render this quadrant
		bool render = false;

		if (superclip) {
			// Superclipping: only render if the screen overlaps this quadrant
			// relative to the sprite origin. Must account for h/v flip.
			int modquad = quadrant;
			static constexpr int vquadflip[4] = { 1, 0, 3, 2 };
			static constexpr int hquadflip[4] = { 3, 2, 1, 0 };
			if (vFlip) modquad = vquadflip[modquad];
			if (hFlip) modquad = hquadflip[modquad];

			switch (modquad) {
				case 0: // SE: screen to the right and below
					render = ((sprH < screenHEnd || sprH >= worldHMid) &&
					          (sprV < screenVEnd || sprV >= worldVMid));
					break;
				case 1: // NE: screen to the right and above
					render = ((sprH < screenHEnd || sprH >= worldHMid) &&
					          (sprV >= screenVStart || sprV <= worldVMid));
					break;
				case 2: // NW: screen to the left and above
					render = ((sprH >= screenHStart || sprH <= worldHMid) &&
					          (sprV >= screenVStart || sprV <= worldVMid));
					break;
				case 3: // SW: screen to the left and below
					render = ((sprH >= screenHStart || sprH <= worldHMid) &&
					          (sprV < screenVEnd || sprV >= worldVMid));
					break;
			}
		} else {
			render = true; // Origin on-screen: render all quadrants
		}

		if (render) {
			// Initialize vertical offset from sprite origin to screen
			int voff = static_cast<int>(_persistVpos) - screenVStart;

			// Reset tilt accumulator for each quadrant
			int32_t tiltAccum = 0;

			// Initialize size accumulators
			uint16_t vsizAccum = (vsign == 1) ? _state.VSizeOff : 0;
			uint16_t hsizAccum;

			// Quad offset fix: save quad 0's vertical sign; offset subsequent quads
			// that draw in the opposite direction by 1 pixel (matches Handy behavior)
			if (loop == 0) vquadoff_sign = vsign;
			if (vsign != vquadoff_sign) voff += vsign;

			// Working copies for this quadrant
			uint16_t hsize = _persistHsize;
			uint16_t vsize = _persistVsize;
			int16_t  qStretch = _persistStretch;
			int16_t  qTilt    = _persistTilt;
			int16_t  qHpos    = _persistHpos;

			// Render scanlines for this quadrant
			for (;;) {
				// Vertical scaling: accumulate vsize
				vsizAccum += vsize;
				int pixelHeight = (vsizAccum >> 8);
				vsizAccum &= 0x00ff; // Keep fractional part

				// Read line offset byte from sprite data
				uint8_t lineOffset = ReadRam(currentDataAddr);
				currentDataAddr++;

				if (lineOffset == 1) {
					// End of this quadrant — advance to next
					break;
				}
				if (lineOffset == 0) {
					// End of sprite — halt all quadrants
					loop = 4; // Will exit the outer for loop
					break;
				}

				// lineOffset gives total bytes for this line's data (including the offset byte)
				int lineDataBytes = lineOffset - 1;
				uint16_t lineEnd = currentDataAddr + lineDataBytes;

				// Decode pixel data for this line
				uint8_t pixelBuf[512];
				int pixelCount = DecodeSpriteLinePixels(currentDataAddr, lineEnd, bpp, literalMode, pixelBuf, 512);
				currentDataAddr = lineEnd;

				// Render this source line for pixelHeight destination lines
				for (int vloop = 0; vloop < pixelHeight; vloop++) {
					// Early bailout if off-screen in the render direction
					if (vsign == 1 && voff >= static_cast<int>(LynxConstants::ScreenHeight)) break;
					if (vsign == -1 && voff < 0) break;

					if (voff >= 0 && voff < static_cast<int>(LynxConstants::ScreenHeight)) {
						// Calculate horizontal start with tilt offset
						int hoff = static_cast<int>(qHpos) + static_cast<int>(tiltAccum >> 8) - screenHStart;

						// Initialize horizontal size accumulator
						hsizAccum = _state.HSizeOff;

						// Quad offset fix for horizontal
						if (loop == 0) hquadoff_sign = hsign;
						if (hsign != hquadoff_sign) hoff += hsign;

						// Render decoded pixels with horizontal scaling
						bool onscreen = false;
						for (int px = 0; px < pixelCount; px++) {
							uint8_t pixel = pixelBuf[px];

							// Horizontal scaling: accumulate hsize
							hsizAccum += hsize;
							int pixelWidth = (hsizAccum >> 8);
							hsizAccum &= 0x00ff;

							// Map through pen index table
							uint8_t penMapped = _penIndex[pixel & 0x0f];

							for (int hloop = 0; hloop < pixelWidth; hloop++) {
								if (hoff >= 0 && hoff < static_cast<int>(LynxConstants::ScreenWidth)) {
									// Background types (0, 1) draw ALL pixels including pen 0.
									// All other types skip pen 0.
									if (pixel != 0 ||
										spriteType == LynxSpriteType::BackgroundShadow ||
										spriteType == LynxSpriteType::BackgroundNonCollide) {
										WriteSpritePixel(hoff, voff, penMapped, collNum, dontCollide, spriteType);
									}
									onscreen = true;
									everOnScreen = true;
								} else {
									if (onscreen) break; // Went off-screen, skip rest
								}
								hoff += hsign;
							}
						}
					}

					voff += vsign;

					// Apply stretch and tilt per destination line (matching Handy)
					if (enableStretch) {
						hsize = static_cast<uint16_t>(static_cast<int32_t>(hsize) + qStretch);
						// VStretch: also apply stretch to vsize per dest line
						if (_state.VStretch) {
							vsize = static_cast<uint16_t>(static_cast<int32_t>(vsize) + qStretch);
						}
					}
					if (enableTilt) {
						tiltAccum += qTilt;
					}
				}
			}
		} else {
			// Skip through data to next quadrant
			// We need to consume data without rendering
			for (;;) {
				uint8_t lineOffset = ReadRam(currentDataAddr);
				currentDataAddr++;

				if (lineOffset == 1) break; // End of quadrant
				if (lineOffset == 0) {
					loop = 4; // End of sprite
					break;
				}
				// Skip over line data
				currentDataAddr += (lineOffset - 1);
			}
		}

		// Advance to next quadrant (wrapping 0-3)
		quadrant = (quadrant + 1) & 0x03;
	}

	// Write collision depositary (per Handy: only for collidable types)
	// Writes the max collision number encountered during this sprite's rendering
	// to SCBAddr + COLLOFF in RAM. Only types that participate in collision write this.
	if (!dontCollide && !_state.NoCollide) {
		switch (spriteType) {
			case LynxSpriteType::XorShadow:
			case LynxSpriteType::Boundary:
			case LynxSpriteType::Normal:
			case LynxSpriteType::BoundaryShadow:
			case LynxSpriteType::Shadow: {
				uint16_t collDep = (scbAddr + _state.CollOffset) & 0xFFFF;
				WriteRam(collDep, _spriteCollision);
				break;
			}
			default:
				break;
		}
	}

	// EVERON tracking: set high bit of collision byte if sprite was never on-screen
	if (_state.EverOn) {
		uint16_t collDep = (scbAddr + _state.CollOffset) & 0xFFFF;
		uint8_t colDat = ReadRam(collDep);
		if (!everOnScreen) {
			colDat |= 0x80;
		} else {
			colDat &= 0x7f;
		}
		WriteRam(collDep, colDat);
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
	int totalBitsLeft = static_cast<int>(lineEnd - dataAddr) * 8;

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
		return static_cast<uint8_t>((shiftReg >> shiftRegCount) & ((1 << bits) - 1));
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

void LynxSuzy::WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum, bool dontCollide, LynxSpriteType spriteType) {
	// Bounds check
	if (x < 0 || x >= static_cast<int>(LynxConstants::ScreenWidth) ||
		y < 0 || y >= static_cast<int>(LynxConstants::ScreenHeight)) {
		return;
	}

	// Calculate video RAM address for this pixel (4bpp packed nibbles)
	uint16_t dispAddr = _state.VideoBase ? _state.VideoBase
	                    : _console->GetMikey()->GetState().DisplayAddress;
	uint16_t byteAddr = dispAddr + y * LynxConstants::BytesPerScanline + (x >> 1);
	uint8_t byte = ReadRam(byteAddr);

	uint8_t existingPixel;
	if (x & 1) {
		existingPixel = byte & 0x0f;
	} else {
		existingPixel = (byte >> 4) & 0x0f;
	}

	// Per-type pixel processing — matches Handy's ProcessPixel() switch
	// Each type defines: which pixels are drawn, whether XOR is applied,
	// and whether collision detection is performed.
	uint8_t writePixel = penIndex & 0x0f;
	bool doWrite = false;
	bool doCollision = false;

	switch (spriteType) {
		case LynxSpriteType::BackgroundShadow:
			// Type 0: Draw ALL pixels (including pen 0). No collision detect,
			// but does write collision buffer unconditionally.
			doWrite = true;
			// Collision buffer write (no read/compare) for pen != 0x0E
			if (!_state.NoCollide && !dontCollide && writePixel != 0x0e) {
				doCollision = true;
			}
			break;

		case LynxSpriteType::BackgroundNonCollide:
			// Type 1: Draw ALL pixels (including pen 0). No collision at all.
			doWrite = true;
			break;

		case LynxSpriteType::BoundaryShadow:
			// Type 2: Skip pen 0, 0x0E, 0x0F. Collision on pen != 0 && pen != 0x0E.
			if (writePixel != 0x00 && writePixel != 0x0e && writePixel != 0x0f) {
				doWrite = true;
			}
			if (writePixel != 0x00 && writePixel != 0x0e) {
				doCollision = !_state.NoCollide && !dontCollide;
			}
			break;

		case LynxSpriteType::Boundary:
			// Type 3: Skip pen 0, 0x0F. Collision on pen != 0 && pen != 0x0E.
			if (writePixel != 0x00 && writePixel != 0x0f) {
				doWrite = true;
			}
			if (writePixel != 0x00 && writePixel != 0x0e) {
				doCollision = !_state.NoCollide && !dontCollide && !dontCollide;
			}
			break;

		case LynxSpriteType::Normal:
			// Type 4: Skip pen 0. Collision on pen != 0 && pen != 0x0E.
			if (writePixel != 0x00) {
				doWrite = true;
			}
			if (writePixel != 0x00 && writePixel != 0x0e) {
				doCollision = !_state.NoCollide && !dontCollide && !dontCollide;
			}
			break;

		case LynxSpriteType::NonCollidable:
			// Type 5: Skip pen 0. No collision.
			if (writePixel != 0x00) {
				doWrite = true;
			}
			break;

		case LynxSpriteType::XorShadow:
			// Type 6: Skip pen 0. XOR with existing pixel. Collision on pen != 0 && pen != 0x0E.
			if (writePixel != 0x00) {
				writePixel = existingPixel ^ writePixel;
				doWrite = true;
			}
			if ((penIndex & 0x0f) != 0x00 && (penIndex & 0x0f) != 0x0e) {
				doCollision = !_state.NoCollide && !dontCollide;
			}
			break;

		case LynxSpriteType::Shadow:
			// Type 7: Skip pen 0. Normal write. Collision on pen != 0 && pen != 0x0E.
			if (writePixel != 0x00) {
				doWrite = true;
			}
			if (writePixel != 0x00 && writePixel != 0x0e) {
				doCollision = !_state.NoCollide && !dontCollide;
			}
			break;
	}

	// Write pixel to video RAM
	if (doWrite) {
		if (x & 1) {
			byte = (byte & 0xF0) | writePixel;
		} else {
			byte = (byte & 0x0F) | (writePixel << 4);
		}
		WriteRam(byteAddr, byte);
	}

	// Collision detection — RAM-based collision buffer at COLLBAS.
	// Per Handy: each pixel position has a nibble in the collision buffer (same
	// layout as video buffer). ReadCollision reads from COLLBAS + y*stride + x/2,
	// WriteCollision writes the sprite's collision number to the same position.
	// mCollision tracks the max collision number read during this sprite.
	if (doCollision && collNum > 0) {
		uint16_t collAddr = _state.CollisionBase + y * LynxConstants::BytesPerScanline + (x >> 1);
		uint8_t collByte = ReadRam(collAddr);
		uint8_t existingColl;
		if (x & 1) {
			existingColl = collByte & 0x0f;
		} else {
			existingColl = (collByte >> 4) & 0x0f;
		}

		// BackgroundShadow (type 0) only writes collision buffer, no read/compare.
		// All other collidable types read existing collision and track max.
		if (spriteType != LynxSpriteType::BackgroundShadow) {
			if (existingColl > 0 && existingColl > _spriteCollision) {
				_spriteCollision = existingColl;
				_state.SpriteToSpriteCollision = true;
			}
		}

		// Write this sprite's collision number to the collision buffer
		if (x & 1) {
			collByte = (collByte & 0xf0) | collNum;
		} else {
			collByte = (collByte & 0x0f) | (collNum << 4);
		}
		WriteRam(collAddr, collByte);
	}
}

uint8_t LynxSuzy::ReadRegister(uint8_t addr) {
	switch (addr) {
		// Sprite engine registers (FC80-FC83)
		case 0x80: // SPRCTL0
			return _state.SpriteControl0;
		case 0x81: // SPRCTL1
			return _state.SpriteControl1;
		case 0x82: // SPRCOLL
			return _state.SpriteInit; // TODO: separate SPRCOLL register
		case 0x83: // SPRINIT
			return _state.SpriteInit;

		// Suzy hardware revision (FC88)
		case 0x88: // SUZYHREV
			return 0x01; // Hardware revision = $01

		// Sprite engine status
		case 0x90: // SUZYBUSEN — sprite engine busy
			return _state.SpriteBusy ? 0x01 : 0x00;
		case 0x91: // SPRGO
			return _state.SpriteEnabled ? 0x01 : 0x00;
		case 0x92: // SPRSYS — system status (read)
			// Per Handy: bit0=SpriteWorking, bit1=StopOnCurrent, bit2=UnsafeAccess,
			// bit3=LeftHand, bit4=VStretch, bit5=LastCarry, bit6=MathOverflow,
			// bit7=MathInProgress
			return (_state.SpriteBusy ? 0x01 : 0x00) |              // Bit 0: sprite working
				(_state.StopOnCurrent ? 0x02 : 0x00) |                 // Bit 1: stop on current
				(_state.UnsafeAccess ? 0x04 : 0x00) |                 // Bit 2: unsafe access
				(_state.LeftHand ? 0x08 : 0x00) |                     // Bit 3: left-handed
				(_state.VStretch ? 0x10 : 0x00) |                     // Bit 4: VStretch
				(_state.LastCarry ? 0x20 : 0x00) |                    // Bit 5: last carry
				(_state.MathOverflow ? 0x40 : 0x00) |                 // Bit 6: math overflow
				(_state.MathInProgress ? 0x80 : 0x00);                // Bit 7: math in progress

		// SCB address
		case 0x10: return static_cast<uint8_t>(_state.SCBAddress & 0xff);
		case 0x11: return static_cast<uint8_t>((_state.SCBAddress >> 8) & 0xff);

		// Math registers — ABCD group (0x52-0x55): multiply operands
		case 0x52: return static_cast<uint8_t>(_state.MathABCD & 0xff);         // MATHD
		case 0x53: return static_cast<uint8_t>((_state.MathABCD >> 8) & 0xff);  // MATHC
		case 0x54: return static_cast<uint8_t>((_state.MathABCD >> 16) & 0xff); // MATHB
		case 0x55: return static_cast<uint8_t>((_state.MathABCD >> 24) & 0xff); // MATHA

		// Math registers — NP group (0x56-0x57): divide divisor
		case 0x56: return static_cast<uint8_t>(_state.MathNP & 0xff);        // MATHP
		case 0x57: return static_cast<uint8_t>((_state.MathNP >> 8) & 0xff); // MATHN

		// Math registers — EFGH group (0x60-0x63): result / dividend
		case 0x60: return static_cast<uint8_t>(_state.MathEFGH & 0xff);         // MATHH
		case 0x61: return static_cast<uint8_t>((_state.MathEFGH >> 8) & 0xff);  // MATHG
		case 0x62: return static_cast<uint8_t>((_state.MathEFGH >> 16) & 0xff); // MATHF
		case 0x63: return static_cast<uint8_t>((_state.MathEFGH >> 24) & 0xff); // MATHE

		// Math registers — JKLM group (0x6C-0x6F): accumulator / remainder
		case 0x6c: return static_cast<uint8_t>(_state.MathJKLM & 0xff);         // MATHM
		case 0x6d: return static_cast<uint8_t>((_state.MathJKLM >> 8) & 0xff);  // MATHL
		case 0x6e: return static_cast<uint8_t>((_state.MathJKLM >> 16) & 0xff); // MATHK
		case 0x6f: return static_cast<uint8_t>((_state.MathJKLM >> 24) & 0xff); // MATHJ

		// Sprite rendering register reads
		case 0x04: return static_cast<uint8_t>(_state.HOffset & 0xff);
		case 0x05: return static_cast<uint8_t>((_state.HOffset >> 8) & 0xff);
		case 0x06: return static_cast<uint8_t>(_state.VOffset & 0xff);
		case 0x07: return static_cast<uint8_t>((_state.VOffset >> 8) & 0xff);
		case 0x08: return static_cast<uint8_t>(_state.VideoBase & 0xff);
		case 0x09: return static_cast<uint8_t>((_state.VideoBase >> 8) & 0xff);
		case 0x0a: return static_cast<uint8_t>(_state.CollisionBase & 0xff);
		case 0x0b: return static_cast<uint8_t>((_state.CollisionBase >> 8) & 0xff);

		// Collision depository: slots 0-3, 12-15
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			return _state.CollisionBuffer[addr];

		// Joystick / switches
		case 0xb0: // JOYSTICK
			return _state.Joystick;
		case 0xb1: // SWITCHES
			return _state.Switches;

		// Cart access registers (FCB2-FCB3)
		case 0xb2: // RCART0 — read from cart bank 0 (auto-increment)
			if (_cart) {
				_cart->SelectBank(0);
				return _cart->ReadData();
			}
			return 0xff;
		case 0xb3: // RCART1 — read from cart bank 1 (auto-increment)
			if (_cart) {
				_cart->SelectBank(1);
				return _cart->ReadData();
			}
			return 0xff;

		default:
			return 0xff;
	}
}

uint8_t LynxSuzy::PeekRegister(uint8_t addr) const {
	switch (addr) {
		case 0xb2: // RCART0 — peek without cart address auto-increment
		case 0xb3: // RCART1
			return _cart ? _cart->PeekData() : 0xff;
		default:
			// All other registers are safe to read without side effects
			return const_cast<LynxSuzy*>(this)->ReadRegister(addr);
	}
}

void LynxSuzy::WriteRegister(uint8_t addr, uint8_t value) {
	switch (addr) {
		// Sprite engine registers (FC80-FC83)
		case 0x80: // SPRCTL0
			_state.SpriteControl0 = value;
			break;
		case 0x81: // SPRCTL1
			_state.SpriteControl1 = value;
			break;
		case 0x82: // SPRCOLL
			// TODO: separate SPRCOLL register
			break;
		case 0x83: // SPRINIT
			_state.SpriteInit = value;
			break;

		// Sprite go
		case 0x91: // SPRGO — write 1 starts sprite engine
			_state.SpriteEnabled = (value & 0x01) != 0;
			_state.EverOn = (value & 0x04) != 0;    // Bit 2: EVERON tracking enable
			if (_state.SpriteEnabled) {
				ProcessSpriteChain();
			}
			break;
		case 0x92: // SPRSYS — write control bits
			_state.MathSign = (value & 0x80) != 0;       // Bit 7: signed math
			_state.MathAccumulate = (value & 0x40) != 0;  // Bit 6: accumulate mode
			_state.NoCollide = (value & 0x20) != 0;       // Bit 5: no collide
			_state.VStretch = (value & 0x10) != 0;        // Bit 4: vertical stretch
			_state.LeftHand = (value & 0x08) != 0;        // Bit 3: left-handed
			if (value & 0x04) _state.UnsafeAccess = false;            // Bit 2: clear unsafe access
			_state.StopOnCurrent = (value & 0x02) != 0;              // Bit 1: stop on current (direct store)
			break;

		// Sprite rendering registers (FC04-FC2B)
		case 0x04: _state.HOffset = (_state.HOffset & static_cast<int16_t>(0xff00)) | value; break;
		case 0x05: _state.HOffset = static_cast<int16_t>((_state.HOffset & 0x00ff) | (static_cast<int16_t>(value) << 8)); break;
		case 0x06: _state.VOffset = (_state.VOffset & static_cast<int16_t>(0xff00)) | value; break;
		case 0x07: _state.VOffset = static_cast<int16_t>((_state.VOffset & 0x00ff) | (static_cast<int16_t>(value) << 8)); break;
		case 0x08: _state.VideoBase = (_state.VideoBase & 0xff00) | value; break;
		case 0x09: _state.VideoBase = (_state.VideoBase & 0x00ff) | (static_cast<uint16_t>(value) << 8); break;
		case 0x0a: _state.CollisionBase = (_state.CollisionBase & 0xff00) | value; break;
		case 0x0b: _state.CollisionBase = (_state.CollisionBase & 0x00ff) | (static_cast<uint16_t>(value) << 8); break;

		// SCB address (FC10-FC11)
		case 0x10:
			_state.SCBAddress = (_state.SCBAddress & 0xff00) | value;
			break;
		case 0x11:
			_state.SCBAddress = (_state.SCBAddress & 0x00ff) | (static_cast<uint16_t>(value) << 8);
			break;

		// Collision offset and size offset registers
		case 0x24: _state.CollOffset = (_state.CollOffset & 0xff00) | value; break;
		case 0x25: _state.CollOffset = (_state.CollOffset & 0x00ff) | (static_cast<uint16_t>(value) << 8); break;
		case 0x28: _state.HSizeOff = (_state.HSizeOff & 0xff00) | value; break;
		case 0x29: _state.HSizeOff = (_state.HSizeOff & 0x00ff) | (static_cast<uint16_t>(value) << 8); break;
		case 0x2a: _state.VSizeOff = (_state.VSizeOff & 0xff00) | value; break;
		case 0x2b: _state.VSizeOff = (_state.VSizeOff & 0x00ff) | (static_cast<uint16_t>(value) << 8); break;

		// Math registers — ABCD group (0x52-0x55): multiply operands
		// Matching Handy: cascading clears + sign conversion at write time
		case 0x52: // MATHD — set byte 0, clear C (matching Handy stun runner fix)
			_state.MathABCD = (_state.MathABCD & 0xffff0000) | value;
			// Writing D clears C (hardware quirk, required for stun runner)
			_state.MathABCD &= 0xffff00ff;
			break;
		case 0x53: { // MATHC — set byte 1, do sign conversion on CD if signed
			_state.MathABCD = (_state.MathABCD & 0xffff00ff) | (static_cast<uint32_t>(value) << 8);
			// Sign conversion at write time (matching Handy)
			if (_state.MathSign) {
				uint16_t cd = static_cast<uint16_t>(_state.MathABCD & 0xffff);
				// HW Bug 13.8: (value-1)&0x8000 check — $8000 is +ve, $0000 is -ve
				if (static_cast<uint16_t>(cd - 1) & 0x8000) {
					uint16_t conv = static_cast<uint16_t>(cd ^ 0xffff);
					conv++;
					_state.MathCD_sign = -1;
					_state.MathABCD = (_state.MathABCD & 0xffff0000) | conv;
				} else {
					_state.MathCD_sign = 1;
				}
			}
			break;
		}
		case 0x54: // MATHB — set byte 2, clear A
			_state.MathABCD = (_state.MathABCD & 0xff00ffff) | (static_cast<uint32_t>(value) << 16);
			_state.MathABCD &= 0x00ffffff; // Clear A
			break;
		case 0x55: { // MATHA — set byte 3, do sign conversion on AB, trigger multiply
			_state.MathABCD = (_state.MathABCD & 0x00ffffff) | (static_cast<uint32_t>(value) << 24);
			// Sign conversion at write time (matching Handy)
			if (_state.MathSign) {
				uint16_t ab = static_cast<uint16_t>((_state.MathABCD >> 16) & 0xffff);
				// HW Bug 13.8: same (value-1)&0x8000 check
				if (static_cast<uint16_t>(ab - 1) & 0x8000) {
					uint16_t conv = static_cast<uint16_t>(ab ^ 0xffff);
					conv++;
					_state.MathAB_sign = -1;
					_state.MathABCD = (_state.MathABCD & 0x0000ffff) | (static_cast<uint32_t>(conv) << 16);
				} else {
					_state.MathAB_sign = 1;
				}
			}
			DoMultiply(); // Writing MATHA triggers multiply
			break;
		}

		// Math registers — NP group (0x56-0x57): divide divisor
		case 0x56: // MATHP — set low byte, clear N
			_state.MathNP = value;
			break;
		case 0x57: // MATHN — set high byte
			_state.MathNP = (_state.MathNP & 0x00ff) | (static_cast<uint16_t>(value) << 8);
			break;

		// Math registers — EFGH group (0x60-0x63): result / divide dividend
		case 0x60: // MATHH — set byte 0, clear G
			_state.MathEFGH = (_state.MathEFGH & 0xffffff00) | value;
			_state.MathEFGH &= 0xffff00ff; // Clear G
			break;
		case 0x61: // MATHG — set byte 1
			_state.MathEFGH = (_state.MathEFGH & 0xffff00ff) | (static_cast<uint32_t>(value) << 8);
			break;
		case 0x62: // MATHF — set byte 2, clear E
			_state.MathEFGH = (_state.MathEFGH & 0xff00ffff) | (static_cast<uint32_t>(value) << 16);
			_state.MathEFGH &= 0x00ffffff; // Clear E
			break;
		case 0x63: // MATHE — set byte 3, trigger divide
			_state.MathEFGH = (_state.MathEFGH & 0x00ffffff) | (static_cast<uint32_t>(value) << 24);
			DoDivide(); // Writing MATHE triggers divide
			break;

		// Math registers — JKLM group (0x6C-0x6F): accumulator / remainder
		case 0x6c: // MATHM — set byte 0, clear L, clear overflow (matching Handy)
			_state.MathJKLM = (_state.MathJKLM & 0xffffff00) | value;
			_state.MathJKLM &= 0xffff00ff; // Clear L
			_state.MathOverflow = false;
			break;
		case 0x6d: // MATHL — set byte 1
			_state.MathJKLM = (_state.MathJKLM & 0xffff00ff) | (static_cast<uint32_t>(value) << 8);
			break;
		case 0x6e: // MATHK — set byte 2, clear J
			_state.MathJKLM = (_state.MathJKLM & 0xff00ffff) | (static_cast<uint32_t>(value) << 16);
			_state.MathJKLM &= 0x00ffffff; // Clear J
			break;
		case 0x6f: // MATHJ — set byte 3
			_state.MathJKLM = (_state.MathJKLM & 0x00ffffff) | (static_cast<uint32_t>(value) << 24);
			break;

		// Collision depository writes: slots 0-3, 12-15 via registers
		// Note: offsets 0x04-0x0B are sprite rendering registers (HOFF, VOFF, VIDBAS, COLLBAS)
		// On real hardware, collision data is stored in RAM at SCBAddr+COLLOFF, not
		// in the register space. This is a simplification for now.
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			_state.CollisionBuffer[addr] = value;
			break;

		// Cart access registers (FCB2-FCB3)
		// RCART0/RCART1 are read-only on hardware. Writes here are a no-op.
		// Handy also ignores writes to these addresses (no Poke case).
		case 0xb2:
		case 0xb3:
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

	// Math (grouped registers matching Handy hardware layout)
	SV(_state.MathABCD);
	SV(_state.MathEFGH);
	SV(_state.MathJKLM);
	SV(_state.MathNP);
	SV(_state.MathAB_sign);
	SV(_state.MathCD_sign);
	SV(_state.MathEFGH_sign);
	SV(_state.MathSign);
	SV(_state.MathAccumulate);
	SV(_state.MathInProgress);
	SV(_state.MathOverflow);
	SV(_state.LastCarry);
	SV(_state.UnsafeAccess);
	SV(_state.SpriteToSpriteCollision);
	SV(_state.StopOnCurrent);
	SV(_state.VStretch);
	SV(_state.LeftHand);

	// Collision
	SVArray(_state.CollisionBuffer, LynxConstants::CollisionBufferSize);
	SV(_spriteCollision);

	// Sprite rendering registers
	SV(_state.HOffset);
	SV(_state.VOffset);
	SV(_state.VideoBase);
	SV(_state.CollisionBase);
	SV(_state.CollOffset);
	SV(_state.HSizeOff);
	SV(_state.VSizeOff);
	SV(_state.EverOn);
	SV(_state.NoCollide);

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
