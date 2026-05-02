#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Shared/NotificationManager.h"
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
	Reset(true);
}


void GenesisVdp::Reset(bool hardReset) {
	memset(_state.Registers, 0, sizeof(_state.Registers));
	bool palMode = (_state.StatusRegister & VdpStatus::PalMode) != 0;
	_state.Registers[0] = 0x04; // Mode register 1
	_state.Registers[1] = 0x04; // Mode register 2 (display off)
	_state.Registers[12] = 0x81; // Mode register 4 (H40 startup default)
	_state.Registers[15] = 0x02; // Auto increment default
	_state.Registers[10] = 0xFF; // HBlank counter
	_state.HIntCounter = _state.Registers[10];
	_state.HCounter = 0;
	_state.VCounter = 0;
	_state.AddressRegister = 0;
	_state.CodeRegister = 0;
	_state.WritePending = false;
	_state.StatusRegister = 0x3400 | VdpStatus::FifoEmpty;
	if (palMode) {
		_state.StatusRegister |= VdpStatus::PalMode;
	}
	_state.StatusRegister &= ~VdpStatus::OddFrame;
	_state.DmaActive = false;
	_state.DmaMode = 0;
	_state.DataPortBuffer = 0;
	_autoIncrement = _state.Registers[15];
	_accessMode = 0;
	_addressReg = 0;
	_displayEnabledLast = false;
	_displayDisabledLogged = false;
	_lastFrameLog = 0;
	_pendingControlWrite = false;
	_firstControlWord = 0;
	_dmaInitialized = false;
	_dmaLatchedMode = 0;
	_dmaRemainingWords = 0;
	_dmaSourceAddress = 0;
	_dmaCopySourceAddress = 0;
	_dmaFillByte = 0;
	_dmaFillDataPending = false;
	_dmaStartupDelayCyclesRemaining = 0;
	_dmaBusCycleRemainder = 0;

	_screenWidth = IsH40Mode() ? 320 : 256;
	_screenHeight = 224;
	_totalLines = 262; // NTSC

	_scanline = 0;
	_hCounter = 0;
	_currentLineCycleTarget = 488;
	_lineCycleRemainder = 0;
	_lastRunCycle = 0;
	if (hardReset) {
		_state.FrameCount = 0;
		memset(_outputBuffers, 0, sizeof(_outputBuffers));
		_currentBuffer = 0;
		MessageManager::Log("[Genesis][VDP] Hard reset complete (frame counter cleared)");
	}
}

void GenesisVdp::SetRegion(bool pal) {
	_totalLines = pal ? 313 : 262;
	_screenHeight = pal ? 240 : 224;
	if (pal) {
		_state.StatusRegister |= VdpStatus::PalMode;
	} else {
		_state.StatusRegister &= ~VdpStatus::PalMode;
	}
}

// Run VDP forward to the target master clock cycle.
// One scanline is 3420 MCLK and 68k runs at MCLK/7, so the line length is
// 3420/7 = 488 + 4/7 68k cycles. Keep the fractional remainder to avoid
// fixed-phase aliasing in VBlank polling loops.
void GenesisVdp::Run(uint64_t targetCycle) {
	// Each M68k cycle = 1 VDP timing step in this simplified model.

	while (_lastRunCycle < targetCycle) {
		_lastRunCycle++;
		_hCounter++;
		_state.HCounter = _hCounter;
		_state.VCounter = _scanline;

		if (_state.DmaActive) {
			ProcessDma();
		}

		if (_hCounter >= _currentLineCycleTarget) {
			_hCounter = 0;
			_state.HCounter = 0;
			ProcessScanline();
			_scanline++;
			_state.VCounter = _scanline;

			// Enter VBlank at the start of the first VBlank line.
			if (_scanline == _screenHeight) {
				_state.StatusRegister |= VdpStatus::VBlankFlag;
				if (IsVBlankInterruptEnabled()) {
					_state.StatusRegister |= VdpStatus::VIntPending;
					if (_cpu) {
						_cpu->SetInterrupt(6); // Level 6 - VBlank
					}
				}
			}

			_currentLineCycleTarget = 488;
			_lineCycleRemainder += 4;
			if (_lineCycleRemainder >= 7) {
				_lineCycleRemainder -= 7;
				_currentLineCycleTarget = 489;
			}

			if (_scanline >= _totalLines) {
				// End of frame
				_scanline = 0;
				_state.VCounter = 0;
				_state.StatusRegister &= ~VdpStatus::VBlankFlag;
				_state.StatusRegister ^= VdpStatus::OddFrame;
				_currentBuffer ^= 1;
				_state.FrameCount++;
				_emu->GetNotificationManager()->SendNotification(ConsoleNotificationType::PpuFrameDone);

				if ((_state.FrameCount - _lastFrameLog) >= 120) {
					_lastFrameLog = _state.FrameCount;
					MessageManager::Log(std::format("[Genesis][VDP] Frame={} display={} size={}x{} totalLines={} masterCycle={}",
						_state.FrameCount,
						IsDisplayEnabled() ? "on" : "off",
						_screenWidth,
						_screenHeight,
						_totalLines,
						_lastRunCycle));
				}
			}
		}
	}
}

void GenesisVdp::ProcessScanline() {
	if (_scanline < _screenHeight) {
		// Active display — render this scanline
		if (IsDisplayEnabled()) {
			if (_displayDisabledLogged) {
				MessageManager::Log(std::format("[Genesis][VDP] Display enabled at frame {} scanline {}", _state.FrameCount, _scanline));
				_displayDisabledLogged = false;
			}
			RenderScanline();
		} else {
			if (_scanline == 0 && !_displayDisabledLogged) {
				MessageManager::Log(std::format("[Genesis][VDP] Display disabled at frame {} (R1=${:02x})", _state.FrameCount, _state.Registers[1]));
				_displayDisabledLogged = true;
			}
			// Display off — fill with backdrop color
			uint16_t* buf = _outputBuffers[_currentBuffer];
			uint16_t bgcolor = CramToRgb555(_cram[GetBackgroundPaletteIndex() * 16 + GetBackgroundColorIndex()]);
			for (uint16_t x = 0; x < _screenWidth; x++) {
				buf[_scanline * MaxScreenWidth + x] = bgcolor;
			}
		}
	}

	// HBlank interrupt
	if (_scanline < _screenHeight && IsHBlankInterruptEnabled()) {
		// HBlank counter (reload from R10 after underflow)
		if (_state.HIntCounter == 0) {
			_state.HIntCounter = _state.Registers[10];
			if (_cpu) {
				_cpu->SetInterrupt(4); // Level 4 - HBlank
			}
		} else {
			_state.HIntCounter--;
		}
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
	uint8_t accessMode = _accessMode & 0x0F;

	if (accessMode == 0) { // VRAM read
		uint32_t addr = _addressReg & 0xFFFE;
		if (addr + 1 < VramSize) {
			result = ((uint16_t)_vram[addr] << 8) | _vram[addr + 1];
		}
	} else if (accessMode == 4) { // VSRAM read
		uint8_t idx = (_addressReg / 2) & 0x3F;
		if (idx < 40) result = _vsram[idx];
	} else if (accessMode == 8) { // CRAM read
		uint8_t idx = (_addressReg / 2) & 0x3F;
		result = _cram[idx];
	}

	_addressReg += _autoIncrement;
	_state.DataPortBuffer = result;
	return result;
}

uint16_t GenesisVdp::ReadControlPort() {
	_pendingControlWrite = false;
	if (_scanline >= _screenHeight && _scanline < _totalLines) {
		_state.StatusRegister |= VdpStatus::VBlankFlag;
	} else {
		_state.StatusRegister &= ~VdpStatus::VBlankFlag;
	}

	static uint64_t controlReadCount = 0;
	controlReadCount++;
	if (controlReadCount <= 128 || (controlReadCount % 4096) == 0) {
		uint8_t statusLo = (uint8_t)(_state.StatusRegister & 0xff);
		bool vblankBit = (statusLo & (uint8_t)VdpStatus::VBlankFlag) != 0;
		MessageManager::Log(std::format("[Genesis][VDP] CtrlRead #{} status=${:04x} lo=${:02x} vb={} display={} scanline={} hcounter={} r1=${:02x}",
			controlReadCount,
			_state.StatusRegister,
			statusLo,
			vblankBit ? 1 : 0,
			IsDisplayEnabled() ? 1 : 0,
			_scanline,
			_hCounter,
			_state.Registers[1]));
	}
	return _state.StatusRegister;
}

void GenesisVdp::AcknowledgeInterrupt(uint8_t level) {
	if (level == 6) {
		_state.StatusRegister &= ~VdpStatus::VIntPending;
	}
}

GenesisVdpState GenesisVdp::GetState() const {
	GenesisVdpState state = _state;
	state.AddressRegister = _addressReg;
	state.CodeRegister = _accessMode;
	state.WritePending = _pendingControlWrite;
	state.HCounter = _hCounter;
	state.VCounter = _scanline;
	memcpy(state.Cram, _cram, sizeof(_cram));
	memcpy(state.Vsram, _vsram, sizeof(_vsram));
	return state;
}

uint16_t GenesisVdp::ReadHVCounter() {
	uint8_t v = (uint8_t)_scanline;
	uint8_t h = (uint8_t)(_hCounter >> 1);
	return ((uint16_t)v << 8) | h;
}

void GenesisVdp::WriteDataPort(uint16_t value) {
	_pendingControlWrite = false;
	_state.DataPortBuffer = value;

	// DMA fill takes its fill byte from the first data-port write after the DMA command.
	if (_state.DmaActive) {
		uint8_t dmaMode = _dmaInitialized ? _dmaLatchedMode : (uint8_t)((_state.Registers[23] >> 6) & 3);
		if (dmaMode == 2 && _dmaFillDataPending) {
			_dmaFillByte = (uint8_t)((value >> 8) & 0xFF);
			_dmaFillDataPending = false;

			// The first data-port write both provides the fill byte and writes one byte to the destination.
			uint32_t fillAddr = _addressReg & 0xFFFF;
			if (fillAddr < VramSize) {
				_vram[fillAddr] = _dmaFillByte;
			}
			_addressReg += _autoIncrement;

			MessageManager::Log(std::format("[Genesis][VDP] Latched DMA fill byte ${:02x} at addr ${:04x} (frame={})",
				_dmaFillByte,
				_addressReg,
				_state.FrameCount));
			return;
		}
	}

	uint8_t accessMode = _accessMode & 0x0F;

	static uint64_t dataWriteCount = 0;
	static bool loggedUnsupportedMode = false;
	static bool loggedMode0WriteFallback = false;
	static bool loggedFirstNonZeroVram = false;
	static bool loggedFirstNonZeroCram = false;
	static bool loggedFirstNonZeroVsram = false;
	dataWriteCount++;

	if (accessMode == 0 || accessMode == 1) { // VRAM write (mode 0 fallback used by some boot paths)
		if (accessMode == 0 && !loggedMode0WriteFallback) {
			loggedMode0WriteFallback = true;
			MessageManager::Log(std::format("[Genesis][VDP] Treating mode 0 data-port writes as VRAM writes for compatibility (addr=${:04x}, frame={})",
				_addressReg,
				_state.FrameCount));
		}

		uint32_t addr = _addressReg & 0xFFFE;
		if (addr + 1 < VramSize) {
			uint8_t hi = (uint8_t)(value >> 8);
			uint8_t lo = (uint8_t)(value & 0xFF);
			_vram[addr] = hi;
			_vram[addr + 1] = lo;

			if (!loggedFirstNonZeroVram && (hi != 0 || lo != 0)) {
				loggedFirstNonZeroVram = true;
				MessageManager::Log(std::format("[Genesis][VDP] First non-zero VRAM write at ${:04x}: ${:02x} ${:02x} (word=${:04x}, dataWrite#{}, frame={})",
					addr,
					hi,
					lo,
					value,
					dataWriteCount,
					_state.FrameCount));
			}
		}
	} else if (accessMode == 3) { // CRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		uint16_t cramValue = value & 0x0EEE;
		_cram[idx] = cramValue; // Mask to valid Genesis color bits

		if (!loggedFirstNonZeroCram && cramValue != 0) {
			loggedFirstNonZeroCram = true;
			MessageManager::Log(std::format("[Genesis][VDP] First non-zero CRAM write at idx {}: ${:04x} (raw=${:04x}, dataWrite#{}, frame={})",
				idx,
				cramValue,
				value,
				dataWriteCount,
				_state.FrameCount));
		}
	} else if (accessMode == 5) { // VSRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		if (idx < 40) {
			uint16_t vsramValue = value & 0x07FF;
			_vsram[idx] = vsramValue;

			if (!loggedFirstNonZeroVsram && vsramValue != 0) {
				loggedFirstNonZeroVsram = true;
				MessageManager::Log(std::format("[Genesis][VDP] First non-zero VSRAM write at idx {}: ${:04x} (raw=${:04x}, dataWrite#{}, frame={})",
					idx,
					vsramValue,
					value,
					dataWriteCount,
					_state.FrameCount));
			}
		}
	} else if (!loggedUnsupportedMode) {
		loggedUnsupportedMode = true;
		MessageManager::Log(std::format("[Genesis][VDP] Data write used unsupported mode {} at addr ${:04x} value=${:04x} (dataWrite#{}, frame={})",
			accessMode,
			_addressReg,
			value,
			dataWriteCount,
			_state.FrameCount));
	}

	_addressReg += _autoIncrement;
}

void GenesisVdp::WriteControlPort(uint16_t value) {
	static uint64_t controlWriteCount = 0;
	controlWriteCount++;

	if (_pendingControlWrite) {
		// Second word of control write
		_pendingControlWrite = false;
		uint16_t full = _firstControlWord;

		// 6-bit command code: CD1:CD0 from first word bits 15:14, CD5:CD2 from second word bits 7:4.
		_accessMode = (uint8_t)((full >> 14) | ((value >> 2) & 0x3C));
		_addressReg = (full & 0x3FFF) | ((value & 0x03) << 14);

		if (controlWriteCount <= 128 || (controlWriteCount % 4096) == 0) {
			MessageManager::Log(std::format("[Genesis][VDP] CtrlWrite2 #{} first=${:04x} second=${:04x} mode={} addr=${:04x} autoInc=${:02x} r1=${:02x} frame={}",
				controlWriteCount,
				full,
				value,
				_accessMode,
				_addressReg,
				_autoIncrement,
				_state.Registers[1],
				_state.FrameCount));
		}

		// Check for DMA trigger (bit 5 of command code)
		if ((_accessMode & 0x20) && (_state.Registers[1] & 0x10)) {
			// DMA enabled
			_state.DmaActive = true;
			_dmaInitialized = false;
			_state.StatusRegister |= VdpStatus::DmaBusy;
		}
		return;
	}

	if ((value & 0xE000) == 0x8000) {
		// Register write: 100R RRRR DDDD DDDD
		uint8_t reg = (value >> 8) & 0x1F;
		uint8_t data = value & 0xFF;
		bool displayEnabledBefore = IsDisplayEnabled();
		if (reg < 24) {
			_state.Registers[reg] = data;
		}

		// Auto-increment from register 15
		if (reg == 15) _autoIncrement = data;
		if (reg == VdpReg::HIntCounter) _state.HIntCounter = data;

		// Update screen mode
		if (reg == 12) {
			_screenWidth = IsH40Mode() ? 320 : 256;
		}

		if (reg == 1) {
			bool displayEnabledAfter = IsDisplayEnabled();
			if (displayEnabledAfter != displayEnabledBefore) {
				MessageManager::Log(std::format("[Genesis][VDP] Display {} via R1 write (${:#04x})", displayEnabledAfter ? "enabled" : "disabled", data));
			}
		}

		if (controlWriteCount <= 128 || (controlWriteCount % 4096) == 0) {
			MessageManager::Log(std::format("[Genesis][VDP] CtrlRegWrite #{} reg={} data=${:02x} r1=${:02x} frame={}",
				controlWriteCount,
				reg,
				data,
				_state.Registers[1],
				_state.FrameCount));
		}
		return;
	}

	// First word of two-word control access
	_pendingControlWrite = true;
	_firstControlWord = value;

	// Update access mode from first word (partially)
	_accessMode = (value >> 14) & 0x03;
	_addressReg = value & 0x3FFF;

	if (controlWriteCount <= 128 || (controlWriteCount % 4096) == 0) {
		MessageManager::Log(std::format("[Genesis][VDP] CtrlWrite1 #{} word=${:04x} modeLow={} addrLow=${:04x} frame={}",
			controlWriteCount,
			value,
			_accessMode,
			_addressReg,
			_state.FrameCount));
	}
}

uint8_t GenesisVdp::GetDmaWordPeriodCycles() const {
	bool blanking = !IsDisplayEnabled() || _scanline >= _screenHeight;
	if (blanking) {
		// Approximation informed by Mesen2-Expanded: blanking DMA is faster.
		return IsH40Mode() ? 5 : 6;
	}

	// Active display DMA uses explicit external-slot gating.
	return 0;
}

namespace {
	static constexpr uint16_t H40ExternalSlotCycles[14] = {
		41, 43, 46, 49, 82, 85, 88,
		90, 93, 461, 463, 465, 468, 472
	};

	static constexpr uint16_t H32ExternalSlotCycles[14] = {
		42, 45, 48, 51, 74, 77, 80,
		82, 85, 454, 457, 460, 462, 468
	};

	static constexpr uint16_t MaxSlotCycle = 472;

	void BuildSlotLookup(const uint16_t* slotCycles, bool (&lookup)[MaxSlotCycle + 1]) {
		for (uint16_t i = 0; i <= MaxSlotCycle; i++) {
			lookup[i] = false;
		}

		for (uint8_t i = 0; i < 14; i++) {
			lookup[slotCycles[i]] = true;
		}
	}
}

bool GenesisVdp::IsActiveDisplayExternalDmaSlot() const {
	uint16_t cycle = _hCounter;
	if (cycle > MaxSlotCycle) {
		return false;
	}

	static bool h40Lookup[MaxSlotCycle + 1] = {};
	static bool h32Lookup[MaxSlotCycle + 1] = {};
	static bool lookupInitialized = false;
	if (!lookupInitialized) {
		BuildSlotLookup(H40ExternalSlotCycles, h40Lookup);
		BuildSlotLookup(H32ExternalSlotCycles, h32Lookup);
		lookupInitialized = true;
	}

	return IsH40Mode() ? h40Lookup[cycle] : h32Lookup[cycle];
}

void GenesisVdp::ProcessDma() {
	if (!_state.DmaActive) return;

	if (!_dmaInitialized) {
		_dmaLatchedMode = (uint8_t)((_state.Registers[23] >> 6) & 3);
		_dmaRemainingWords = ((uint16_t)_state.Registers[20] << 8) | _state.Registers[19];
		if (_dmaRemainingWords == 0) {
			_dmaRemainingWords = 0x10000;
		}

		_dmaSourceAddress = ((uint32_t)(_state.Registers[23] & 0x3F) << 17)
		                | ((uint32_t)_state.Registers[22] << 9)
		                | ((uint32_t)_state.Registers[21] << 1);
		_dmaCopySourceAddress = ((uint16_t)_state.Registers[22] << 8) | _state.Registers[21];

		if (_dmaLatchedMode <= 1) {
			_state.DmaMode = 0;
			// Startup latency approximation for 68K-bus DMA in the core-cycle domain.
			_dmaStartupDelayCyclesRemaining = IsH40Mode() ? 7 : 9;
			_dmaBusCycleRemainder = 0;
		} else if (_dmaLatchedMode == 2) {
			_state.DmaMode = 1;
			_dmaFillDataPending = true;
			_dmaFillByte = 0;
			_dmaStartupDelayCyclesRemaining = 0;
			_dmaBusCycleRemainder = 0;
		} else {
			_state.DmaMode = 2;
			_dmaFillDataPending = false;
			_dmaFillByte = 0;
			_dmaStartupDelayCyclesRemaining = 0;
			_dmaBusCycleRemainder = 0;
		}

		_dmaInitialized = true;
	}

	_state.StatusRegister |= VdpStatus::DmaBusy;

	uint32_t wordsThisStep = _dmaRemainingWords;
	if (_dmaLatchedMode == 0 || _dmaLatchedMode == 1) {
		if (_dmaStartupDelayCyclesRemaining > 0) {
			_dmaStartupDelayCyclesRemaining--;
			return;
		}

		bool activeDisplay = IsDisplayEnabled() && _scanline < _screenHeight;
		if (activeDisplay) {
			_dmaBusCycleRemainder = 0;
			if (!IsActiveDisplayExternalDmaSlot()) {
				return;
			}

			wordsThisStep = 1;
		} else {
			uint8_t periodCycles = GetDmaWordPeriodCycles();
			uint32_t budgetCycles = 1u + _dmaBusCycleRemainder;
			uint32_t transferableWords = budgetCycles / periodCycles;
			_dmaBusCycleRemainder = (uint8_t)(budgetCycles % periodCycles);
			if (transferableWords == 0) {
				return;
			}

			wordsThisStep = std::min(_dmaRemainingWords, transferableWords);
		}
	}

	if (wordsThisStep == 0) {
		return;
	}

	if (_dmaLatchedMode == 0 || _dmaLatchedMode == 1) {
		// 68K → VRAM/CRAM/VSRAM copy
		for (uint32_t i = 0; i < wordsThisStep; i++) {
			uint16_t word = _memoryManager->Read16(_dmaSourceAddress);
			WriteDataPort(word);
			_dmaSourceAddress += 2;
		}

		uint32_t srcWordAddress = (_dmaSourceAddress >> 1) & 0x3FFFFF;
		_state.Registers[21] = (uint8_t)(srcWordAddress & 0xFF);
		_state.Registers[22] = (uint8_t)((srcWordAddress >> 8) & 0xFF);
		_state.Registers[23] = (uint8_t)((_state.Registers[23] & 0xC0) | ((srcWordAddress >> 16) & 0x3F));
	} else if (_dmaLatchedMode == 2) {
		if (_dmaFillDataPending) {
			return;
		}

		// VRAM fill
		uint8_t fillByte = _dmaFillByte;
		uint32_t dmaDst = _addressReg;
		for (uint32_t i = 0; i < wordsThisStep; i++) {
			uint32_t addr = dmaDst & 0xFFFF;
			if (addr < VramSize) _vram[addr] = fillByte;
			dmaDst += _autoIncrement;
		}
		_addressReg = (uint16_t)dmaDst;
	} else if (_dmaLatchedMode == 3) {
		// VRAM copy
		uint32_t dmaDst = _addressReg;
		for (uint32_t i = 0; i < wordsThisStep; i++) {
			uint32_t src = (_dmaCopySourceAddress + i) & 0xFFFF;
			uint32_t dst = dmaDst & 0xFFFF;
			if (src < VramSize && dst < VramSize) {
				_vram[dst] = _vram[src];
			}
			dmaDst += _autoIncrement;
		}
		_dmaCopySourceAddress = (uint16_t)(_dmaCopySourceAddress + wordsThisStep);
		_addressReg = (uint16_t)dmaDst;
	}

	_dmaRemainingWords -= wordsThisStep;
	if (_dmaRemainingWords > 0) {
		_state.Registers[19] = (uint8_t)(_dmaRemainingWords & 0xFF);
		_state.Registers[20] = (uint8_t)((_dmaRemainingWords >> 8) & 0xFF);
		return;
	}

	_state.Registers[19] = 0;
	_state.Registers[20] = 0;

	_state.DmaActive = false;
	_dmaInitialized = false;
	_dmaFillDataPending = false;
	_dmaStartupDelayCyclesRemaining = 0;
	_dmaBusCycleRemainder = 0;
	_state.StatusRegister &= ~VdpStatus::DmaBusy;
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
	SV(_currentLineCycleTarget);
	SV(_lineCycleRemainder);
	SV(_lastRunCycle);
	SV(_currentBuffer);
	SV(_screenWidth);
	SV(_screenHeight);
	SV(_totalLines);
	SV(_pendingControlWrite);
	SV(_firstControlWord);
	SV(_accessMode);
	SV(_addressReg);
	SV(_autoIncrement);
	SV(_state.StatusRegister);
	SV(_state.HIntCounter);
	SV(_state.FrameCount);
	SV(_state.DmaActive);
	SV(_state.DmaMode);
	SV(_state.DataPortBuffer);
	SV(_dmaInitialized);
	SV(_dmaLatchedMode);
	SV(_dmaRemainingWords);
	SV(_dmaSourceAddress);
	SV(_dmaCopySourceAddress);
	SV(_dmaFillByte);
	SV(_dmaFillDataPending);
	SV(_dmaStartupDelayCyclesRemaining);
	SV(_dmaBusCycleRemainder);
	for (int i = 0; i < 24; i++) {
		SVI(_state.Registers[i]);
	}
}
