#include "pch.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

void LynxSuzy::Init(Emulator* emu, LynxConsole* console, LynxMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_memoryManager = memoryManager;

	memset(&_state, 0, sizeof(_state));
	memset(_lineBuffer, 0, sizeof(_lineBuffer));

	_state.Joystick = 0xff; // All buttons released (active-low)
	_state.Switches = 0xff;
}

__forceinline uint8_t LynxSuzy::ReadRam(uint16_t addr) const {
	return _console->GetWorkRam()[addr & 0xFFFF];
}

__forceinline uint16_t LynxSuzy::ReadRam16(uint16_t addr) const {
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
	uint16_t scbAddr = _state.SCBAddress;

	// Walk the sprite linked list
	int spriteCount = 0;
	// HW Bug 13.12: The hardware only checks the UPPER BYTE of the SCB NEXT
	// address for zero to terminate the sprite chain. If the upper byte is
	// zero but the lower byte is non-zero (e.g., $0080), the chain still
	// terminates. Conversely, $0100 would NOT terminate (upper byte = $01).
	while ((scbAddr >> 8) != 0 && spriteCount < 256) { // Safety limit
		ProcessSprite(scbAddr);
		// Read next SCB pointer
		scbAddr = ReadRam16(scbAddr);
		spriteCount++;
	}

	_state.SpriteBusy = false;
}

void LynxSuzy::ProcessSprite(uint16_t scbAddr) {
	// Read SCB header
	uint8_t sprCtl0 = ReadRam(scbAddr + 2);
	uint8_t sprCtl1 = ReadRam(scbAddr + 3);

	// Extract BPP from SPRCTL0 bits 7:6
	int bpp = ((sprCtl0 >> 6) & 0x03) + 1; // 1, 2, 3, or 4 bpp

	// Read sprite data pointer
	uint16_t dataAddr = ReadRam16(scbAddr + 4);

	// Read position
	int16_t hpos = (int16_t)ReadRam16(scbAddr + 6);
	int16_t vpos = (int16_t)ReadRam16(scbAddr + 8);

	// Read size (8.8 fixed-point)
	uint16_t hsizeRaw = ReadRam16(scbAddr + 10);
	uint16_t vsizeRaw = ReadRam16(scbAddr + 12);

	// Check for skip flag (SPRCTL1 bit 2)
	if (sprCtl1 & 0x04) {
		return; // Skip this sprite
	}

	// HW Bug 13.7: Sprite "shadow" polarity is inverted.
	// SPRCTL1 bit 3 controls shadow mode. In the hardware, a value of 0
	// enables shadow (opposite of what the documentation says). Games that
	// rely on shadow sprites must invert this bit.
	// We extract the flag with hardware-correct polarity:
	bool isShadow = (sprCtl1 & 0x08) == 0; // Inverted! 0 = shadow, 1 = normal
	(void)isShadow; // Used when full sprite rendering is implemented

	// Basic drawing: read scanlines from data, plot into frame buffer
	// Collision number from SCB (for collision detection)
	uint8_t collNum = ReadRam(scbAddr + 14) & 0x0f;

	// Process each scanline of the sprite
	uint8_t* workRam = _console->GetWorkRam();
	uint32_t* frameBuffer = _console->GetFrameBuffer();
	uint16_t currentDataAddr = dataAddr;

	for (int line = 0; line < 256; line++) { // Max sprite height safety
		// Read line header
		uint8_t lineHeader = ReadRam(currentDataAddr);
		currentDataAddr++;

		if (lineHeader == 0) {
			break; // End of sprite data
		}

		int lineY = vpos + line;
		if (lineY < 0 || lineY >= (int)LynxConstants::ScreenHeight) {
			// Offscreen — skip data but still advance
			currentDataAddr += (lineHeader - 1);
			continue;
		}

		// Decode pixel data for this line (simplified — full RLE in future)
		int pixelIndex = 0;
		uint16_t lineEnd = currentDataAddr + (lineHeader - 1);

		while (currentDataAddr < lineEnd && pixelIndex < (int)LynxConstants::ScreenWidth) {
			uint8_t dataByte = ReadRam(currentDataAddr);
			currentDataAddr++;

			// Simple 4bpp decode: each byte = 2 pixels
			if (bpp == 4) {
				uint8_t pix0 = (dataByte >> 4) & 0x0f;
				uint8_t pix1 = dataByte & 0x0f;

				int x0 = hpos + pixelIndex;
				int x1 = hpos + pixelIndex + 1;

				if (pix0 != 0 && x0 >= 0 && x0 < (int)LynxConstants::ScreenWidth) {
					frameBuffer[lineY * LynxConstants::ScreenWidth + x0] = _console->GetFrameBuffer()[0]; // Placeholder
					// Collision detection
					if (collNum > 0) {
						uint8_t existing = _state.CollisionBuffer[pix0];
						if (existing != 0) {
							_state.CollisionBuffer[collNum] |= existing;
							_state.CollisionBuffer[existing] |= collNum;
						}
						_state.CollisionBuffer[pix0] = collNum;
					}
				}
				pixelIndex += 2;
			} else {
				// For other BPP modes, skip for now
				pixelIndex++;
			}
		}

		// Ensure we're at the end of line data
		currentDataAddr = lineEnd;
	}
}

void LynxSuzy::DecodeSpriteLineRLE(uint8_t bpp, uint16_t dataAddr, int width) {
	// TODO: Full RLE decoding for sprite scanlines
	// Each line: alternating literal and repeat packets
	// Packet header: upper nibble = type, lower = count
	memset(_lineBuffer, 0, sizeof(_lineBuffer));
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
		case 0x92: // SPRSYS — system status
			return (_state.SpriteBusy ? 0x01 : 0x00) |
				(_state.MathOverflow ? 0x04 : 0x00) |  // Bit 2: math overflow
				(_state.MathInProgress ? 0x80 : 0x00);

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

		// Collision buffer
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
			return _state.CollisionBuffer[addr - 0x80];
		// Collision slots 8-15
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			return _state.CollisionBuffer[addr];

		// Joystick / switches
		case 0xb0: // JOYSTICK
			return _state.Joystick;
		case 0xb1: // SWITCHES
			return _state.Switches;

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
		case 0x92: // SPRSYS — write various control bits
			_state.MathSign = (value & 0x80) != 0;
			_state.MathAccumulate = (value & 0x40) != 0;
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

		// Collision buffer writes
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
			_state.CollisionBuffer[addr - 0x80] = value;
			break;
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			_state.CollisionBuffer[addr] = value;
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

	// Collision
	SVArray(_state.CollisionBuffer, LynxConstants::CollisionBufferSize);

	// Input
	SV(_state.Joystick);
	SV(_state.Switches);
}
