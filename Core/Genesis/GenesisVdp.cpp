#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

GenesisVdp::GenesisVdp() {
}

GenesisVdp::~GenesisVdp() {
}

void GenesisVdp::Init(Emulator* emu, GenesisConsole* console, GenesisM68k* cpu, GenesisMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_cpu = cpu;
	_memoryManager = memoryManager;

	memset(_vram, 0, VramSize);
	memset(_cram, 0, sizeof(_cram));
	memset(_vsram, 0, sizeof(_vsram));
	memset(&_state, 0, sizeof(_state));
	memset(_outputBuffers, 0, sizeof(_outputBuffers));

	_state.Registers[0] = 0x04; // Mode register 1
	_state.Registers[1] = 0x04; // Mode register 2 (display off)
	_state.Registers[10] = 0xFF; // HBlank counter
	_autoIncrement = 2;

	_screenWidth = 320;
	_screenHeight = 224;
	_totalLines = 262; // NTSC

	_scanline = 0;
	_hCounter = 0;
	_lastRunCycle = 0;
	_currentBuffer = 0;
}

void GenesisVdp::SetRegion(bool pal) {
	_totalLines = pal ? 313 : 262;
	_screenHeight = pal ? 240 : 224;
}

// Run VDP forward to the target master clock cycle
// Genesis M68k runs at 7.67 MHz, VDP pixel clock is the same
// Each scanline is 3420 master clocks (MCLK), which is 342 pixels * 10
// but we simplify: ~488 68k cycles per line (NTSC), 262/313 lines per frame
void GenesisVdp::Run(uint64_t targetCycle) {
	// Each M68k cycle = 1 VDP step (simplified)
	// Real hardware has MCLK but we use 68k cycles as base
	static constexpr uint32_t CyclesPerLine = 488;

	while (_lastRunCycle < targetCycle) {
		_lastRunCycle++;
		_hCounter++;

		if (_hCounter >= CyclesPerLine) {
			_hCounter = 0;
			ProcessScanline();
			_scanline++;

			if (_scanline >= _totalLines) {
				// End of frame
				_scanline = 0;
				_currentBuffer ^= 1;
				_state.FrameCount++;
			}
		}
	}
}

void GenesisVdp::ProcessScanline() {
	if (_scanline < _screenHeight) {
		// Active display — render this scanline
		if (IsDisplayEnabled()) {
			RenderScanline();
		} else {
			// Display off — fill with backdrop color
			uint16_t* buf = _outputBuffers[_currentBuffer];
			uint16_t bgcolor = CramToRgb555(_cram[GetBackgroundPaletteIndex() * 16 + GetBackgroundColorIndex()]);
			for (uint16_t x = 0; x < _screenWidth; x++) {
				buf[_scanline * MaxScreenWidth + x] = bgcolor;
			}
		}
	}

	if (_scanline == _screenHeight) {
		// VBlank start
		_state.StatusRegister |= VdpStatus::VBlankFlag;
		if (IsVBlankInterruptEnabled()) {
			_state.StatusRegister |= VdpStatus::VIntPending;
			_cpu->SetInterrupt(6); // Level 6 — VBlank
		}
	}

	// HBlank interrupt
	if (_scanline < _screenHeight && IsHBlankInterruptEnabled()) {
		// HBlank counter
		if (_state.Registers[10] == 0) {
			_state.Registers[10] = _state.Registers[10]; // Reload
			_state.StatusRegister |= VdpStatus::HIntPending;
			_cpu->SetInterrupt(4); // Level 4 — HBlank
		} else {
			_state.Registers[10]--;
		}
	}

	// DMA processing
	if (_state.DmaActive) {
		ProcessDma();
	}
}

void GenesisVdp::RenderScanline() {
	uint16_t lineBuffer[MaxScreenWidth] = {};

	// Background color (backdrop)
	uint16_t bgcolor = CramToRgb555(_cram[GetBackgroundPaletteIndex() * 16 + GetBackgroundColorIndex()]);
	for (int x = 0; x < _screenWidth; x++) {
		lineBuffer[x] = bgcolor;
	}

	// Render Plane B (low priority background)
	RenderBackground(lineBuffer, 1);

	// Render Plane A (high priority background)
	RenderBackground(lineBuffer, 0);

	// Render sprites
	RenderSprites(lineBuffer);

	// Copy to output buffer
	uint16_t* buf = _outputBuffers[_currentBuffer];
	memcpy(&buf[_scanline * MaxScreenWidth], lineBuffer, _screenWidth * sizeof(uint16_t));
}

void GenesisVdp::RenderBackground(uint16_t* lineBuffer, uint8_t planeIndex) {
	uint16_t planeBase = planeIndex == 0 ? GetPlaneABase() : GetPlaneBBase();
	uint16_t planeW = GetPlaneWidth();
	uint16_t planeH = GetPlaneHeight();

	// Get scroll values
	uint16_t hscrollBase = GetHScrollBase();
	uint8_t hscrollMode = _state.Registers[11] & 0x03;
	uint8_t vscrollMode = (_state.Registers[11] >> 2) & 0x01;

	// Get horizontal scroll for this line
	int16_t hscroll = 0;
	uint32_t hscrollAddr = hscrollBase + (planeIndex == 0 ? 0 : 2);
	if (hscrollMode == 0) {
		// Whole screen
		hscroll = (int16_t)((_vram[hscrollAddr] << 8) | _vram[hscrollAddr + 1]);
	} else if (hscrollMode == 2) {
		// Per-cell (8 lines)
		uint32_t cellAddr = hscrollAddr + (_scanline / 8) * 32;
		if (cellAddr + 1 < VramSize) {
			hscroll = (int16_t)((_vram[cellAddr] << 8) | _vram[cellAddr + 1]);
		}
	} else if (hscrollMode == 3) {
		// Per-line
		uint32_t lineAddr = hscrollAddr + _scanline * 4;
		if (lineAddr + 1 < VramSize) {
			hscroll = (int16_t)((_vram[lineAddr] << 8) | _vram[lineAddr + 1]);
		}
	}
	hscroll &= 0x3FF;

	for (uint16_t x = 0; x < _screenWidth; x++) {
		// Get vertical scroll
		uint16_t vscroll = 0;
		if (vscrollMode == 0) {
			// Whole screen
			vscroll = _vsram[planeIndex];
		} else {
			// Per-2-cell column
			uint8_t col = x / 16;
			if (col * 2 + planeIndex < 40) {
				vscroll = _vsram[col * 2 + planeIndex];
			}
		}
		vscroll &= 0x3FF;

		// Calculate pixel position in plane
		uint16_t px = (x - hscroll) & ((planeW * 8) - 1);
		uint16_t py = (_scanline + vscroll) & ((planeH * 8) - 1);

		// Get tile from nametable
		uint16_t tileCol = px / 8;
		uint16_t tileRow = py / 8;
		uint32_t ntAddr = planeBase + (tileRow * planeW + tileCol) * 2;
		if (ntAddr + 1 >= VramSize) continue;

		uint16_t ntEntry = ((uint16_t)_vram[ntAddr] << 8) | _vram[ntAddr + 1];

		// Decode nametable entry
		uint16_t tileIndex = ntEntry & 0x7FF;
		bool hFlip = (ntEntry >> 11) & 1;
		bool vFlip = (ntEntry >> 12) & 1;
		uint8_t palette = (ntEntry >> 13) & 3;
		bool priority = (ntEntry >> 15) & 1;

		// Get pixel within tile
		uint8_t tileX = px & 7;
		uint8_t tileY = py & 7;
		if (hFlip) tileX = 7 - tileX;
		if (vFlip) tileY = 7 - tileY;

		// Read 4bpp tile data (each tile is 32 bytes = 8 rows * 4 bytes)
		uint32_t tileAddr = tileIndex * 32 + tileY * 4;
		if (tileAddr + 3 >= VramSize) continue;

		// Each row: 4 bytes = 8 pixels (4bpp, big-endian nibbles)
		uint8_t nibbleOffset = tileX;
		uint8_t byteIdx = nibbleOffset / 2;
		uint8_t pixelData = _vram[tileAddr + byteIdx];
		uint8_t colorIdx;
		if (nibbleOffset & 1) {
			colorIdx = pixelData & 0x0F;
		} else {
			colorIdx = (pixelData >> 4) & 0x0F;
		}

		if (colorIdx != 0) { // Color 0 is transparent
			uint16_t cramIdx = palette * 16 + colorIdx;
			lineBuffer[x] = CramToRgb555(_cram[cramIdx]);
		}
	}
}

void GenesisVdp::RenderSprites(uint16_t* lineBuffer) {
	uint16_t satBase = GetSpriteTableBase();
	uint16_t maxSprites = IsH40Mode() ? 80 : 64;
	uint16_t maxPerLine = IsH40Mode() ? 20 : 16;
	uint16_t spritesOnLine = 0;

	// Walk sprite link list (entry 0 is first)
	uint16_t spriteIdx = 0;
	for (uint16_t i = 0; i < maxSprites; i++) {
		uint32_t satAddr = satBase + spriteIdx * 8;
		if (satAddr + 7 >= VramSize) break;

		// Sprite attribute table entry (8 bytes each):
		// Word 0: Y position (10 bits)
		// Word 1: bits 0-1: width-1, bits 2-3: height-1, bits 0-6 (byte 3): link
		// Word 2: bits 0-10: tile index, bit 11: hflip, bit 12: vflip, bits 13-14: palette, bit 15: priority
		// Word 3: X position (10 bits)

		uint16_t yPos = (((uint16_t)_vram[satAddr] << 8) | _vram[satAddr + 1]) & 0x3FF;
		uint8_t sizeH = ((_vram[satAddr + 2] >> 0) & 3) + 1; // Width in cells (1-4)
		uint8_t sizeV = ((_vram[satAddr + 2] >> 2) & 3) + 1; // Height in cells (1-4)
		uint8_t link = _vram[satAddr + 3] & 0x7F;

		uint16_t attr = ((uint16_t)_vram[satAddr + 4] << 8) | _vram[satAddr + 5];
		uint16_t tileIndex = attr & 0x7FF;
		bool hFlip = (attr >> 11) & 1;
		bool vFlip = (attr >> 12) & 1;
		uint8_t palette = (attr >> 13) & 3;
		// bool priority = (attr >> 15) & 1;

		uint16_t xPos = (((uint16_t)_vram[satAddr + 6] << 8) | _vram[satAddr + 7]) & 0x3FF;

		// Sprites use screen coordinates offset by 128
		int16_t screenY = (int16_t)yPos - 128;
		int16_t screenX = (int16_t)xPos - 128;
		uint16_t spriteHeight = sizeV * 8;
		uint16_t spriteWidth = sizeH * 8;

		// Check if sprite is on this scanline
		if (_scanline >= screenY && _scanline < screenY + spriteHeight) {
			if (++spritesOnLine > maxPerLine) break;

			uint16_t py = _scanline - screenY;
			if (vFlip) py = spriteHeight - 1 - py;

			for (uint16_t sx = 0; sx < spriteWidth; sx++) {
				int16_t px = screenX + sx;
				if (px < 0 || px >= _screenWidth) continue;

				uint16_t tileX = hFlip ? (spriteWidth - 1 - sx) : sx;

				// Calculate tile within multi-cell sprite
				// Tiles are laid out column-major: column0(rows), column1(rows), ...
				uint16_t cellCol = tileX / 8;
				uint16_t cellRow = py / 8;
				uint16_t tileNum = tileIndex + cellCol * sizeV + cellRow;

				uint8_t pixX = tileX & 7;
				uint8_t pixY = py & 7;

				uint32_t tileAddr = tileNum * 32 + pixY * 4;
				if (tileAddr + 3 >= VramSize) continue;

				uint8_t byteIdx = pixX / 2;
				uint8_t pixelData = _vram[tileAddr + byteIdx];
				uint8_t colorIdx;
				if (pixX & 1) {
					colorIdx = pixelData & 0x0F;
				} else {
					colorIdx = (pixelData >> 4) & 0x0F;
				}

				if (colorIdx != 0) {
					uint16_t cramIdx = palette * 16 + colorIdx;
					lineBuffer[px] = CramToRgb555(_cram[cramIdx]);
				}
			}
		}

		// Follow link for next sprite
		if (link == 0) break;
		spriteIdx = link;
	}
}

// Convert Genesis CRAM color (0000BBB0GGG0RRR0) to RGB555
uint16_t GenesisVdp::CramToRgb555(uint16_t cramColor) {
	uint8_t r = (cramColor >> 1) & 7;
	uint8_t g = (cramColor >> 5) & 7;
	uint8_t b = (cramColor >> 9) & 7;
	// Scale 3-bit (0-7) to 5-bit (0-31)
	r = (r << 2) | (r >> 1);
	g = (g << 2) | (g >> 1);
	b = (b << 2) | (b >> 1);
	return (r << 10) | (g << 5) | b; // RGB555
}

// Port access
uint16_t GenesisVdp::ReadDataPort() {
	_pendingControlWrite = false;
	uint16_t result = 0;

	if (_accessMode == 0) { // VRAM read
		uint32_t addr = _addressReg & 0xFFFE;
		if (addr + 1 < VramSize) {
			result = ((uint16_t)_vram[addr] << 8) | _vram[addr + 1];
		}
	} else if (_accessMode == 4) { // VSRAM read
		uint8_t idx = (_addressReg / 2) & 0x3F;
		if (idx < 40) result = _vsram[idx];
	} else if (_accessMode == 8) { // CRAM read
		uint8_t idx = (_addressReg / 2) & 0x3F;
		result = _cram[idx];
	}

	_addressReg += _autoIncrement;
	return result;
}

uint16_t GenesisVdp::ReadControlPort() {
	_pendingControlWrite = false;
	uint16_t status = _state.StatusRegister;
	// Reading control port clears pending interrupts
	_state.StatusRegister &= ~(VdpStatus::VIntPending | VdpStatus::HIntPending);
	return status;
}

uint16_t GenesisVdp::ReadHVCounter() {
	uint8_t v = (uint8_t)_scanline;
	uint8_t h = (uint8_t)(_hCounter >> 1);
	return ((uint16_t)v << 8) | h;
}

void GenesisVdp::WriteDataPort(uint16_t value) {
	_pendingControlWrite = false;

	if (_accessMode == 1) { // VRAM write
		uint32_t addr = _addressReg & 0xFFFE;
		if (addr + 1 < VramSize) {
			_vram[addr] = (uint8_t)(value >> 8);
			_vram[addr + 1] = (uint8_t)(value & 0xFF);
		}
	} else if (_accessMode == 3) { // CRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		_cram[idx] = value & 0x0EEE; // Mask to valid Genesis color bits
	} else if (_accessMode == 5) { // VSRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		if (idx < 40) _vsram[idx] = value & 0x7FF;
	}

	_addressReg += _autoIncrement;
}

void GenesisVdp::WriteControlPort(uint16_t value) {
	if (_pendingControlWrite) {
		// Second word of control write
		_pendingControlWrite = false;
		uint16_t full = _firstControlWord;

		// CD5-CD1 = access mode
		_accessMode = ((full >> 14) & 0x03) | (((value >> 4) & 0x03) << 2);
		_addressReg = (full & 0x3FFF) | ((value & 0x03) << 14);

		// Check for DMA trigger
		if ((value & 0x80) && (_state.Registers[1] & 0x10)) {
			// DMA enabled
			_state.DmaActive = true;
		}
		return;
	}

	if ((value & 0xC000) == 0x8000) {
		// Register write: 100R RRRR DDDD DDDD
		uint8_t reg = (value >> 8) & 0x1F;
		uint8_t data = value & 0xFF;
		if (reg < 24) {
			_state.Registers[reg] = data;
		}

		// Auto-increment from register 15
		if (reg == 15) _autoIncrement = data;

		// Update screen mode
		if (reg == 12) {
			_screenWidth = IsH40Mode() ? 320 : 256;
		}
		return;
	}

	// First word of two-word control access
	_pendingControlWrite = true;
	_firstControlWord = value;

	// Update access mode from first word (partially)
	_accessMode = (value >> 14) & 0x03;
	_addressReg = value & 0x3FFF;
}

void GenesisVdp::ProcessDma() {
	if (!_state.DmaActive) return;

	uint8_t dmaMode = (_state.Registers[23] >> 6) & 3;
	uint16_t dmaLength = ((uint16_t)_state.Registers[20] << 8) | _state.Registers[19];
	uint32_t dmaSrc = ((uint32_t)(_state.Registers[23] & 0x3F) << 17)
	                | ((uint32_t)_state.Registers[22] << 9)
	                | ((uint32_t)_state.Registers[21] << 1);

	if (dmaLength == 0) dmaLength = 0xFFFF;

	if (dmaMode == 0 || dmaMode == 1) {
		// 68K → VRAM/CRAM/VSRAM copy
		for (uint16_t i = 0; i < dmaLength; i++) {
			uint16_t word = _memoryManager->Read16(dmaSrc);
			WriteDataPort(word);
			dmaSrc += 2;
		}
	} else if (dmaMode == 2) {
		// VRAM fill
		uint8_t fillByte = _state.Registers[23] & 0xFF; // Simplified
		for (uint16_t i = 0; i < dmaLength; i++) {
			uint32_t addr = (_addressReg + i) & 0xFFFF;
			if (addr < VramSize) _vram[addr] = fillByte;
		}
	} else if (dmaMode == 3) {
		// VRAM copy
		uint16_t srcAddr = ((uint16_t)_state.Registers[22] << 8) | _state.Registers[21];
		for (uint16_t i = 0; i < dmaLength; i++) {
			uint32_t src = (srcAddr + i) & 0xFFFF;
			uint32_t dst = (_addressReg + i) & 0xFFFF;
			if (src < VramSize && dst < VramSize) {
				_vram[dst] = _vram[src];
			}
		}
	}

	_state.DmaActive = false;
}

uint16_t GenesisVdp::GetPlaneWidth() const {
	switch (_state.Registers[16] & 3) {
		case 0: return 32;
		case 1: return 64;
		case 3: return 128;
		default: return 32;
	}
}

uint16_t GenesisVdp::GetPlaneHeight() const {
	switch ((_state.Registers[16] >> 4) & 3) {
		case 0: return 32;
		case 1: return 64;
		case 3: return 128;
		default: return 32;
	}
}

void GenesisVdp::Serialize(Serializer& s) {
	SVArray(_vram, VramSize);
	SVArray(_cram, (uint32_t)64);
	SVArray(_vsram, (uint32_t)40);
	SV(_scanline);
	SV(_hCounter);
	SV(_lastRunCycle);
	SV(_currentBuffer);
	SV(_screenWidth);
	SV(_screenHeight);
	SV(_pendingControlWrite);
	SV(_firstControlWord);
	SV(_accessMode);
	SV(_addressReg);
	SV(_autoIncrement);
	SV(_state.StatusRegister);
	SV(_state.FrameCount);
	SV(_state.DmaActive);
	for (int i = 0; i < 24; i++) {
		SVI(_state.Registers[i]);
	}
}
