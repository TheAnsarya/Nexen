#include "pch.h"
#include "Lynx/LynxSuzy.h"
#include "Lynx/LynxConsole.h"
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

	// Extract BPP from SPRCTL0 bits 7:6 (0=1bpp, 1=2bpp, 2=3bpp, 3=4bpp)
	int bpp = ((sprCtl0 >> 6) & 0x03) + 1;

	// Extract sprite type from SPRCTL0 bits 2:0
	// (background, normal, boundary, shadow, etc.)
	LynxSpriteType spriteType = static_cast<LynxSpriteType>(sprCtl0 & 0x07);

	// Extract draw direction from SPRCTL0 bits 5:4
	// Bit 5: H-flip (0=left-to-right, 1=right-to-left)
	// Bit 4: V-flip (0=top-to-bottom, 1=bottom-to-top)
	bool hFlip = (sprCtl0 & 0x20) != 0;
	bool vFlip = (sprCtl0 & 0x10) != 0;

	// Read sprite data pointer
	uint16_t dataAddr = ReadRam16(scbAddr + 4);

	// Read position (signed 16-bit)
	int16_t hpos = (int16_t)ReadRam16(scbAddr + 6);
	int16_t vpos = (int16_t)ReadRam16(scbAddr + 8);

	// Read size (8.8 fixed-point) for stretch support
	uint16_t hsizeRaw = ReadRam16(scbAddr + 10);
	uint16_t vsizeRaw = ReadRam16(scbAddr + 12);

	// Stretch and tilt (8.8 fixed-point) — only used for scaled sprites
	// uint16_t stretch = ReadRam16(scbAddr + 14);
	// uint16_t tilt = ReadRam16(scbAddr + 16);

	// Check for skip flag (SPRCTL1 bit 2)
	if (sprCtl1 & 0x04) {
		return;
	}

	// Collision number from collision depository index
	uint8_t collNum = ReadRam(scbAddr + 14) & 0x0f;

	// Process each scanline of sprite data
	uint16_t currentDataAddr = dataAddr;
	int hDir = hFlip ? -1 : 1;
	int vDir = vFlip ? -1 : 1;

	for (int line = 0; line < 256; line++) {
		// Read the line header — contains the byte count for this line
		uint8_t lineHeader = ReadRam(currentDataAddr);
		currentDataAddr++;

		if (lineHeader == 0) {
			break; // End of sprite data (0-length line terminates)
		}

		int lineY = vpos + (vFlip ? -line : line);

		// lineHeader is the total byte count for this line including the header
		// Data bytes = lineHeader - 1
		int dataBytes = lineHeader - 1;
		uint16_t lineEnd = currentDataAddr + dataBytes;

		if (lineY < 0 || lineY >= (int)LynxConstants::ScreenHeight) {
			// Off-screen — skip data but advance pointer
			currentDataAddr = lineEnd;
			continue;
		}

		// Decode pixels from the packed data for this line
		// Pixels are packed MSB-first within each byte
		int pixelX = 0;
		int bitBuffer = 0;
		int bitsRemaining = 0;

		while (currentDataAddr < lineEnd) {
			uint8_t dataByte = ReadRam(currentDataAddr);
			currentDataAddr++;

			// Add 8 bits to our buffer
			bitBuffer = (bitBuffer << 8) | dataByte;
			bitsRemaining += 8;

			// Extract as many pixels as we can from the bit buffer
			while (bitsRemaining >= bpp) {
				bitsRemaining -= bpp;
				uint8_t penIndex = (bitBuffer >> bitsRemaining) & ((1 << bpp) - 1);

				int drawX = hpos + (hFlip ? -pixelX : pixelX);

				// Write the pixel to the frame buffer in work RAM
				WriteSpritePixel(drawX, lineY, penIndex, collNum);

				pixelX++;
			}
		}

		// Ensure we consumed exactly the right number of bytes
		currentDataAddr = lineEnd;
	}
}

__forceinline void LynxSuzy::WriteRam(uint16_t addr, uint8_t value) const {
	_console->GetWorkRam()[addr & 0xFFFF] = value;
}

void LynxSuzy::WriteSpritePixel(int x, int y, uint8_t penIndex, uint8_t collNum) {
	// Bounds check
	if (x < 0 || x >= (int)LynxConstants::ScreenWidth ||
		y < 0 || y >= (int)LynxConstants::ScreenHeight) {
		return;
	}

	// Transparent pixel (index 0) is never drawn
	if (penIndex == 0) {
		return;
	}

	// Write pixel as 4bpp nibble into work RAM frame buffer
	// The frame buffer is at DisplayAddress in RAM, organized as
	// 80 bytes per scanline (160 pixels at 4bpp, 2 pixels per byte)
	// High nibble = even pixel, low nibble = odd pixel
	uint16_t dispAddr = _console->GetMikey()->GetState().DisplayAddress;
	uint16_t byteAddr = dispAddr + y * LynxConstants::BytesPerScanline + (x >> 1);
	uint8_t byte = ReadRam(byteAddr);

	if (x & 1) {
		// Odd pixel → low nibble
		byte = (byte & 0xF0) | (penIndex & 0x0F);
	} else {
		// Even pixel → high nibble
		byte = (byte & 0x0F) | ((penIndex & 0x0F) << 4);
	}
	WriteRam(byteAddr, byte);

	// Collision detection
	if (collNum > 0 && collNum < LynxConstants::CollisionBufferSize) {
		uint8_t existing = _state.CollisionBuffer[collNum];
		if (penIndex < LynxConstants::CollisionBufferSize) {
			uint8_t depository = _state.CollisionBuffer[penIndex];
			if (depository > existing) {
				_state.CollisionBuffer[collNum] = depository;
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

	// Collision
	SVArray(_state.CollisionBuffer, LynxConstants::CollisionBufferSize);

	// Input
	SV(_state.Joystick);
	SV(_state.Switches);
}
