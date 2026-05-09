#include "pch.h"
#include "Genesis/GenesisVdp.h"
#include "Genesis/GenesisConsole.h"
#include "Genesis/GenesisM68k.h"
#include "Genesis/GenesisMemoryManager.h"
#include "Shared/Emulator.h"
#include "Shared/MessageManager.h"
#include "Shared/NotificationManager.h"
#include "Utilities/Serializer.h"

namespace {
	static constexpr uint32_t MclksPerLine = 3420u;
	static constexpr uint32_t H40LineChangeSlot = 165u;
	static constexpr uint32_t H32LineChangeSlot = 133u;

	static constexpr uint32_t H40HsyncCycles[17] = {
		19u, 20u, 20u, 20u, 18u, 20u, 20u, 20u, 18u,
		20u, 20u, 20u, 18u, 20u, 20u, 20u, 19u
	};

	static uint8_t HCounterTableH40[MclksPerLine] = {};
	static uint8_t HCounterTableH32[MclksPerLine] = {};
	static bool HCounterTablesBuilt = false;

	void EnsureHCounterTables() {
		if (HCounterTablesBuilt) {
			return;
		}

		HCounterTablesBuilt = true;

		{
			uint32_t mclk = 0;
			uint8_t hslot = (uint8_t)H40LineChangeSlot;
			while (mclk < MclksPerLine) {
				uint32_t cycles = (hslot >= 230u && hslot <= 246u)
					? H40HsyncCycles[hslot - 230u]
					: 16u;
				for (uint32_t c = 0; c < cycles && (mclk + c) < MclksPerLine; c++) {
					HCounterTableH40[mclk + c] = hslot;
				}
				mclk += cycles;
				if (hslot == 182u) {
					hslot = 229u;
				} else if (hslot == 255u) {
					hslot = 0u;
				} else {
					hslot++;
				}
			}
		}

		{
			uint32_t mclk = 0;
			uint8_t hslot = (uint8_t)H32LineChangeSlot;
			while (mclk < MclksPerLine) {
				for (uint32_t c = 0; c < 20u && (mclk + c) < MclksPerLine; c++) {
					HCounterTableH32[mclk + c] = hslot;
				}
				mclk += 20u;
				if (hslot == 147u) {
					hslot = 233u;
				} else if (hslot == 255u) {
					hslot = 0u;
				} else {
					hslot++;
				}
			}
		}
	}
}

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
	memset(_palette, 0, sizeof(_palette));
	memset(_shadowPalette, 0, sizeof(_shadowPalette));
	memset(_highlightPalette, 0, sizeof(_highlightPalette));
	memset(&_state, 0, sizeof(_state));
	memset(_outputBuffers, 0, sizeof(_outputBuffers));
	Reset(true);
}


void GenesisVdp::Reset(bool hardReset) {
	memset(_state.Registers, 0, sizeof(_state.Registers));
	bool palMode = (_state.StatusRegister & VdpStatus::PalMode) != 0;
	_state.Registers[0] = 0x00; // Mode register 1
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
	_pendingDataHighWrite = false;
	_dataHighWrite = 0;
	_pendingControlHighWrite = false;
	_controlHighWrite = 0;
	_firstControlWord = 0;
	_dmaInitialized = false;
	_dmaLatchedMode = 0;
	_dmaSourceReg23Latched = 0;
	_dmaRemainingWords = 0;
	_dmaSourceAddress = 0;
	_dmaCopySourceAddress = 0;
	_dmaFillByte = 0;
	_dmaFillDataPending = false;
	_dmaStartupDelayCyclesRemaining = 0;
	_dmaBusCycleRemainder = 0;
	_prevLineDotOverflow = false;
	_writeFifoRead = 0;
	_writeFifoWrite = 0;
	_writeFifoCount = 0;
	_hvCounterLatch = 0;

	_screenWidth = IsH40Mode() ? 320 : 256;
	_screenHeight = 224;
	_totalLines = 262; // NTSC
	_lineDisplayEnabled = IsDisplayEnabled();
	_lineH40Mode = IsH40Mode();
	_lineScreenWidth = _screenWidth;
	_vblankEnteredThisFrame = false;
	_vintPending = false;
	_vintNew = false;
	_hintPending = false;
	_hintNew = false;

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

	RefreshPaletteCache();
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

void GenesisVdp::UpdateFifoStatusBits() {
	if (_writeFifoCount == 0) {
		_state.StatusRegister |= VdpStatus::FifoEmpty;
		_state.StatusRegister &= ~VdpStatus::FifoFull;
	} else if (_writeFifoCount >= 4) {
		_state.StatusRegister &= ~VdpStatus::FifoEmpty;
		_state.StatusRegister |= VdpStatus::FifoFull;
	} else {
		_state.StatusRegister &= ~VdpStatus::FifoEmpty;
		_state.StatusRegister &= ~VdpStatus::FifoFull;
	}
}

uint8_t GenesisVdp::VCounterValue(uint32_t scanline) const {
	uint16_t vc = (uint16_t)scanline;

	if ((_state.StatusRegister & VdpStatus::PalMode) == 0) {
		if (scanline > 234u) {
			vc = (uint16_t)(0x1e5u + (scanline - 235u));
		}
	} else {
		bool v30 = (_state.Registers[1] & 0x08u) != 0;
		if (v30) {
			if (scanline > 266u) {
				vc = (uint16_t)(0x1d2u + (scanline - 267u));
			}
		} else {
			if (scanline > 258u) {
				vc = (uint16_t)(0x1cau + (scanline - 259u));
			}
		}
	}

	if (IsInterlaceMode()) {
		vc = (uint16_t)(vc << 1);
		if ((_state.StatusRegister & VdpStatus::OddFrame) != 0) {
			vc |= 1u;
		}
	}

	return (uint8_t)(vc & 0xffu);
}

uint8_t GenesisVdp::HCounterValue(uint32_t lineCycle, bool h40Mode) const {
	EnsureHCounterTables();
	if (_currentLineCycleTarget == 0) {
		return 0;
	}

	uint32_t clampedCycle = std::min(lineCycle, (uint32_t)(_currentLineCycleTarget - 1u));
	uint32_t mclkInLine = (clampedCycle * MclksPerLine) / _currentLineCycleTarget;
	if (mclkInLine >= MclksPerLine) {
		mclkInLine = MclksPerLine - 1u;
	}

	return h40Mode ? HCounterTableH40[mclkInLine] : HCounterTableH32[mclkInLine];
}

uint16_t GenesisVdp::GetVBlankFlagStartCycle() const {
	uint32_t slotOffsetMclk = (_lineH40Mode ? 32u : 40u);
	uint32_t cycle = (slotOffsetMclk * _currentLineCycleTarget + (MclksPerLine / 2u)) / MclksPerLine;
	if (cycle == 0u) {
		cycle = 1u;
	}

	if (cycle >= _currentLineCycleTarget) {
		cycle = _currentLineCycleTarget - 1u;
	}

	return (uint16_t)cycle;
}

uint16_t GenesisVdp::GetHBlankStartCycle() const {
	uint32_t hblankMclk = _lineH40Mode ? 2850u : 2860u;
	uint32_t cycle = (hblankMclk * _currentLineCycleTarget + (MclksPerLine / 2u)) / MclksPerLine;
	if (cycle == 0u) {
		cycle = 1u;
	}

	if (cycle >= _currentLineCycleTarget) {
		cycle = _currentLineCycleTarget - 1u;
	}

	return (uint16_t)cycle;
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
		_state.HCounter = HCounterValue(_hCounter, _lineH40Mode);
		_state.VCounter = VCounterValue(_scanline);

		uint16_t hblankStartCycle = GetHBlankStartCycle();
		if (_hCounter >= hblankStartCycle) {
			_state.StatusRegister |= VdpStatus::HBlanking;
		} else {
			_state.StatusRegister &= ~VdpStatus::HBlanking;
		}

		bool inVblankLines = _scanline >= _screenHeight && _scanline < _totalLines;
		bool vblankFlagActive = false;
		if (inVblankLines) {
			if (_scanline > _screenHeight) {
				vblankFlagActive = true;
			} else {
				vblankFlagActive = _hCounter >= GetVBlankFlagStartCycle();
			}
		}

		if (vblankFlagActive) {
			_state.StatusRegister |= VdpStatus::VBlankFlag;
		} else {
			_state.StatusRegister &= ~VdpStatus::VBlankFlag;
		}

		if (vblankFlagActive && !_vblankEnteredThisFrame) {
			_vblankEnteredThisFrame = true;
			static uint64_t vblankEnterCount = 0;
			vblankEnterCount++;
			if (vblankEnterCount <= 256 || (vblankEnterCount % 2048) == 0) {
				MessageManager::Log(std::format("[Genesis][VDP] VBlank enter #{} frame={} scanline={} hc={} status=${:04x} r1=${:02x} vintEn={}",
					vblankEnterCount,
					_state.FrameCount,
					_scanline,
					_hCounter,
					_state.StatusRegister,
					_state.Registers[1],
					IsVBlankInterruptEnabled() ? 1 : 0));
			}

			if (IsVBlankInterruptEnabled()) {
				_vintPending = true;
				_vintNew = true;
				_state.StatusRegister |= VdpStatus::VIntPending;
				if (_cpu) {
					_cpu->SetInterrupt(6);
				}
			} else {
				_vintPending = true;
				_vintNew = false;
				_state.StatusRegister |= VdpStatus::VIntPending;
			}
		}

		if (_scanline < _screenHeight && _hCounter == hblankStartCycle) {
			if (_state.HIntCounter == 0) {
				_state.HIntCounter = _state.Registers[10];
				_hintPending = true;
				if (IsHBlankInterruptEnabled() && _cpu) {
					_hintNew = true;
					_cpu->SetInterrupt(4);
				} else {
					_hintNew = false;
				}
			} else {
				_state.HIntCounter--;
			}
		}

		if (_state.DmaActive) {
			ProcessDma();
		}

		if (_writeFifoCount > 0) {
			bool activeDisplay = _lineDisplayEnabled && _scanline < _screenHeight;
			if (!activeDisplay || IsActiveDisplayExternalDmaSlot()) {
				DrainWriteFifoOne();
			}
		}

		if (_hCounter >= _currentLineCycleTarget) {
			_hCounter = 0;
			_state.HCounter = HCounterValue(0, _lineH40Mode);
			ProcessScanline();
			_scanline++;
			_state.VCounter = VCounterValue(_scanline);
			// Latch mode/display at scanline boundaries to avoid mid-line timing drift.
			_lineDisplayEnabled = IsDisplayEnabled();
			_lineH40Mode = IsH40Mode();
			_lineScreenWidth = _lineH40Mode ? 320 : 256;
			_screenWidth = _lineScreenWidth;

			// Keep startup timing checkpoints stable on 68K line boundaries.
			_currentLineCycleTarget = 488;

			if (_scanline >= _totalLines) {
				// End of frame
				_scanline = 0;
				_state.VCounter = VCounterValue(0);
				_vblankEnteredThisFrame = false;
				_vintPending = false;
				_vintNew = false;
				_hintPending = false;
				_hintNew = false;
				uint16_t statusBeforeExit = _state.StatusRegister;
				_state.StatusRegister &= ~VdpStatus::VBlankFlag;
				_state.StatusRegister &= ~VdpStatus::HBlanking;
				_state.StatusRegister ^= VdpStatus::OddFrame;
				static uint64_t vblankExitCount = 0;
				vblankExitCount++;
				if (vblankExitCount <= 256 || (vblankExitCount % 2048) == 0) {
					MessageManager::Log(std::format("[Genesis][VDP] VBlank exit #{} frame={} statusBefore=${:04x} statusAfter=${:04x} pendingAfter={}",
						vblankExitCount,
						_state.FrameCount,
						statusBeforeExit,
						_state.StatusRegister,
						(_state.StatusRegister & VdpStatus::VIntPending) ? 1 : 0));
				}
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
		RefreshPaletteCache();

		// Active display — render this scanline
		if (_lineDisplayEnabled) {
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
			uint8_t bgcolorIdx = (uint8_t)((GetBackgroundPaletteIndex() << 4) | GetBackgroundColorIndex());
			uint16_t bgcolor = _palette[bgcolorIdx & 0x3Fu];
			for (uint16_t x = 0; x < MaxScreenWidth; x++) {
				buf[_scanline * MaxScreenWidth + x] = bgcolor;
			}
		}
	}

}

void GenesisVdp::RenderScanline() {
	uint16_t lineBuffer[MaxScreenWidth] = {};
	uint8_t planeB[MaxScreenWidth] = {};
	uint8_t planeA[MaxScreenWidth] = {};
	uint8_t sprites[MaxScreenWidth] = {};

	RenderPlane(_scanline, false, planeB);
	RenderPlane(_scanline, true, planeA);
	RenderWindow(_scanline, planeA);
	RenderSprites(_scanline, sprites);
	Composite(lineBuffer, planeB, planeA, sprites, _lineScreenWidth);

	// Copy to output buffer
	uint16_t* buf = _outputBuffers[_currentBuffer];
	uint16_t* dstLine = &buf[_scanline * MaxScreenWidth];
	memcpy(dstLine, lineBuffer, _lineScreenWidth * sizeof(uint16_t));
	for (uint16_t x = _lineScreenWidth; x < MaxScreenWidth; x++) {
		dstLine[x] = 0;
	}
}

uint16_t GenesisVdp::GetHScrollForLine(uint16_t line, bool planeA) const {
	uint16_t hscrollBase = GetHScrollBase();
	uint16_t lineMask = 0;
	if ((_state.Registers[11] & 0x02u) != 0) {
		lineMask |= 0xF8u;
	}
	if ((_state.Registers[11] & 0x01u) != 0) {
		lineMask |= 0x07u;
	}

	uint32_t hscrollAddr = (hscrollBase + ((line & lineMask) * 4u) + (planeA ? 0u : 2u)) & 0xFFFFu;
	return (uint16_t)((_vram[hscrollAddr] << 8) | _vram[(hscrollAddr + 1u) & 0xFFFFu]);
}

uint16_t GenesisVdp::GetVScrollForPixel(uint16_t x, bool planeA) const {
	uint8_t vscrollMode = (_state.Registers[11] >> 2) & 0x01u;
	if (vscrollMode == 0) {
		uint8_t idx = planeA ? 0u : 1u;
		return _vsram[idx] & 0x03FFu;
	}

	uint8_t pair = (uint8_t)(((x >> 4) * 2u) + (planeA ? 0u : 1u));
	if (pair < 40) {
		return _vsram[pair] & 0x03FFu;
	}

	return planeA ? (_vsram[0] & 0x03FFu) : (_vsram[1] & 0x03FFu);
}

bool GenesisVdp::IsWindowPixel(uint16_t line, uint16_t x) const {
	uint8_t wx = _state.Registers[17];
	uint8_t wy = _state.Registers[18];
	bool windowRight = (wx & 0x80u) != 0;
	bool windowDown = (wy & 0x80u) != 0;
	uint16_t wxPix = (uint16_t)(wx & 0x1Fu) * 16u;
	uint16_t wyCell = (uint16_t)(wy & 0x1Fu);
	uint16_t lineCell = line >> 3;

	bool covY = windowDown ? (lineCell >= wyCell) : (lineCell < wyCell);
	bool covX = windowRight ? (x >= wxPix) : (x < wxPix);
	return covX || covY;
}

uint8_t GenesisVdp::FetchTilePixel(uint16_t tileBase, uint8_t row, uint8_t col) const {
	uint16_t byteAddr = (uint16_t)(tileBase + (uint16_t)row * 4u + (col >> 1));
	uint8_t b = _vram[byteAddr & 0xFFFFu];
	return (col & 1u) ? (b & 0x0Fu) : ((b >> 4) & 0x0Fu);
}

void GenesisVdp::RenderPlane(uint16_t line, bool planeA, uint8_t* dst) const {
	uint16_t planeBase = planeA ? GetPlaneABase() : GetPlaneBBase();
	uint16_t planeW = GetPlaneWidth();
	uint16_t planeH = GetPlaneHeight();
	uint16_t hscroll = GetHScrollForLine(line, planeA) & 0x03FFu;
	bool interlace2 = (_state.Registers[12] & 0x06u) == 0x06u;
	bool interlaceField = (_state.StatusRegister & VdpStatus::OddFrame) != 0;
	uint16_t tilePixH = interlace2 ? 16u : 8u;
	uint16_t tileRowLine = line / tilePixH;
	uint16_t pixRowLine = line % tilePixH;
	uint16_t interlacePixRow = interlace2 ? (uint16_t)(pixRowLine * 2u + (interlaceField ? 1u : 0u)) : pixRowLine;
	uint16_t width = _lineScreenWidth;
	uint32_t planePxW = (uint32_t)planeW * 8u;
	uint32_t planePxH = (uint32_t)planeH * tilePixH;

	for (uint16_t x = 0; x < width; x++) {
		uint16_t vscroll = GetVScrollForPixel(x, planeA);
		uint16_t px = (uint16_t)(x - hscroll) & (uint16_t)(planePxW - 1u);
		uint16_t py = (uint16_t)(((uint32_t)tileRowLine * tilePixH + interlacePixRow + vscroll) % planePxH);

		uint16_t tileCol = px >> 3;
		uint16_t tileRow = py / tilePixH;
		uint16_t tileX = px & 7u;
		uint16_t tileY = py % tilePixH;

		uint32_t ntAddr = (planeBase + (tileRow * planeW + tileCol) * 2u) & 0xFFFFu;
		uint16_t entry = ((uint16_t)_vram[ntAddr] << 8) | _vram[(ntAddr + 1u) & 0xFFFFu];

		bool priority = ((entry >> 15) & 1u) != 0;
		uint8_t palette = (uint8_t)((entry >> 13) & 0x03u);
		bool vFlip = ((entry >> 12) & 1u) != 0;
		bool hFlip = ((entry >> 11) & 1u) != 0;
		uint16_t tile = entry & 0x07FFu;

		uint8_t fetchX = hFlip ? (uint8_t)(7u - tileX) : (uint8_t)tileX;
		uint8_t fetchY = vFlip ? (uint8_t)(tilePixH - 1u - tileY) : (uint8_t)tileY;
		uint16_t tileBase = interlace2 ? (uint16_t)(tile * 64u) : (uint16_t)(tile * 32u);
		uint8_t color = FetchTilePixel(tileBase, fetchY, fetchX);

		dst[x] = (uint8_t)((priority ? 0x80u : 0x00u) | (palette << 4) | color);
	}
}

void GenesisVdp::RenderWindow(uint16_t line, uint8_t* dst) const {
	uint16_t nameBase = GetWindowBase();
	uint16_t cellW = IsH40Mode() ? 64u : 32u;
	bool interlace2 = (_state.Registers[12] & 0x06u) == 0x06u;
	bool interlaceField = (_state.StatusRegister & VdpStatus::OddFrame) != 0;
	uint16_t tilePixH = interlace2 ? 16u : 8u;
	uint16_t tileRow = line / tilePixH;
	uint16_t pixRow = line % tilePixH;
	uint16_t interlacePixRow = interlace2 ? (uint16_t)(pixRow * 2u + (interlaceField ? 1u : 0u)) : pixRow;

	for (uint16_t x = 0; x < _lineScreenWidth; x++) {
			if (!IsWindowPixel(line, x)) continue;

		uint16_t tileCol = x >> 3;
		uint16_t tileX = x & 7u;
		uint32_t ntAddr = (nameBase + (tileRow * cellW + tileCol) * 2u) & 0xFFFFu;
		uint16_t entry = ((uint16_t)_vram[ntAddr] << 8) | _vram[(ntAddr + 1u) & 0xFFFFu];

		bool priority = ((entry >> 15) & 1u) != 0;
		uint8_t palette = (uint8_t)((entry >> 13) & 0x03u);
		bool vFlip = ((entry >> 12) & 1u) != 0;
		bool hFlip = ((entry >> 11) & 1u) != 0;
		uint16_t tile = entry & 0x07FFu;

		uint8_t fetchX = hFlip ? (uint8_t)(7u - tileX) : (uint8_t)tileX;
		uint8_t fetchY = vFlip ? (uint8_t)(tilePixH - 1u - interlacePixRow) : (uint8_t)interlacePixRow;
		uint16_t tileBase = interlace2 ? (uint16_t)(tile * 64u) : (uint16_t)(tile * 32u);
		uint8_t color = FetchTilePixel(tileBase, fetchY, fetchX);

		dst[x] = (uint8_t)((priority ? 0x80u : 0x00u) | 0x40u | (palette << 4) | color);
	}
}

void GenesisVdp::RenderSprites(uint16_t line, uint8_t* dst) {
	struct LineSprite {
		uint16_t tile = 0;
		uint16_t rawX = 0;
		int16_t x = 0;
		uint8_t palette = 0;
		uint8_t vertCells = 1;
		uint8_t horizCells = 1;
		uint8_t cellRow = 0;
		uint8_t pixRow = 0;
		bool priority = false;
		bool hFlip = false;
		bool vFlip = false;
	};

	struct LineSpriteCell {
		uint16_t tile = 0;
		uint16_t rawX = 0;
		int16_t x = 0;
		uint8_t palette = 0;
		uint8_t vertCells = 1;
		uint8_t screenCellCol = 0;
		uint8_t patternCellOffsetX = 0;
		uint8_t patternCellOffsetY = 0;
		uint8_t pixRow = 0;
		bool priority = false;
		bool hFlip = false;
		bool vFlip = false;
	};

	uint16_t sprBase = GetSpriteTableBase();
	uint16_t maxSprites = IsH40Mode() ? 80u : 64u;
	uint16_t maxPerLine = IsH40Mode() ? 20u : 16u;
	uint16_t maxCells = IsH40Mode() ? 40u : 32u;
	bool interlace2 = (_state.Registers[12] & 0x06u) == 0x06u;
	bool interlaceField = (_state.StatusRegister & VdpStatus::OddFrame) != 0;
	uint16_t cellPixH = interlace2 ? 16u : 8u;
	uint16_t effectiveLine = interlace2 ? (uint16_t)(line * 2u + (interlaceField ? 1u : 0u)) : line;

	LineSprite spriteList[80] = {};
	LineSpriteCell cellList[40] = {};
	uint16_t spriteCount = 0;
	uint16_t cellCount = 0;
	bool lineDotOverflow = false;

	uint8_t idx = 0;
	for (uint16_t s = 0; s < maxSprites; s++) {
		uint16_t entryBase = (uint16_t)(sprBase + (uint16_t)idx * 8u);
		uint16_t w0 = ((uint16_t)_vram[(entryBase + 0u) & 0xFFFFu] << 8) | _vram[(entryBase + 1u) & 0xFFFFu];
		uint16_t w1 = ((uint16_t)_vram[(entryBase + 2u) & 0xFFFFu] << 8) | _vram[(entryBase + 3u) & 0xFFFFu];
		uint16_t w2 = ((uint16_t)_vram[(entryBase + 4u) & 0xFFFFu] << 8) | _vram[(entryBase + 5u) & 0xFFFFu];
		uint16_t w3 = ((uint16_t)_vram[(entryBase + 6u) & 0xFFFFu] << 8) | _vram[(entryBase + 7u) & 0xFFFFu];

		int16_t sprY = (int16_t)(w0 & 0x01FFu) - 128;
		uint8_t vertCells = (uint8_t)(((w1 >> 8) & 0x03u) + 1u);
		uint8_t horizCells = (uint8_t)(((w1 >> 10) & 0x03u) + 1u);
		uint8_t link = (uint8_t)(w1 & 0x7Fu);
		uint16_t sprH = (uint16_t)vertCells * cellPixH;

		if ((int16_t)effectiveLine >= sprY && (int16_t)effectiveLine < (int16_t)(sprY + sprH)) {
			if (spriteCount >= maxPerLine) {
				_state.StatusRegister |= 0x0040u;
				lineDotOverflow = true;
				break;
			}

			bool vFlip = (w2 & 0x1000u) != 0;
			uint16_t sprRow = (uint16_t)((int16_t)effectiveLine - sprY);
			uint8_t cellRow = (uint8_t)(sprRow / cellPixH);
			uint8_t pixRow = (uint8_t)(sprRow % cellPixH);

			LineSprite& sprite = spriteList[spriteCount++];
			sprite.tile = w2 & 0x07FFu;
			sprite.rawX = w3 & 0x01FFu;
			sprite.x = (int16_t)sprite.rawX - 128;
			sprite.palette = (uint8_t)((w2 >> 13) & 0x03u);
			sprite.vertCells = vertCells;
			sprite.horizCells = horizCells;
			sprite.cellRow = vFlip ? (uint8_t)(vertCells - 1u - cellRow) : cellRow;
			sprite.pixRow = pixRow;
			sprite.priority = (w2 & 0x8000u) != 0;
			sprite.hFlip = (w2 & 0x0800u) != 0;
			sprite.vFlip = vFlip;
		}

		if (link == 0 || link >= maxSprites) {
			break;
		}
		idx = link;
	}

	for (uint16_t i = 0; i < spriteCount; i++) {
		const LineSprite& sprite = spriteList[i];
		for (uint8_t screenCellCol = 0; screenCellCol < sprite.horizCells; screenCellCol++) {
			if (cellCount >= maxCells) {
				_state.StatusRegister |= 0x0040u;
				lineDotOverflow = true;
				break;
			}

			LineSpriteCell& cell = cellList[cellCount++];
			cell.tile = sprite.tile;
			cell.rawX = sprite.rawX;
			cell.x = sprite.x;
			cell.palette = sprite.palette;
			cell.vertCells = sprite.vertCells;
			cell.screenCellCol = screenCellCol;
			cell.patternCellOffsetX = sprite.hFlip ? (uint8_t)(sprite.horizCells - 1u - screenCellCol) : screenCellCol;
			cell.patternCellOffsetY = sprite.cellRow;
			cell.pixRow = sprite.pixRow;
			cell.priority = sprite.priority;
			cell.hFlip = sprite.hFlip;
			cell.vFlip = sprite.vFlip;
		}

		if (lineDotOverflow) {
			break;
		}
	}

	bool maskActive = false;
	bool nonMaskCellEncountered = false;
	for (uint16_t i = 0; i < cellCount; i++) {
		const LineSpriteCell& cell = cellList[i];
		if (cell.rawX == 0) {
			if (nonMaskCellEncountered || _prevLineDotOverflow) {
				maskActive = true;
			}
			continue;
		}

		nonMaskCellEncountered = true;
		if (maskActive) {
			continue;
		}

		uint16_t tileIdx = (uint16_t)(cell.tile + cell.patternCellOffsetX * cell.vertCells + cell.patternCellOffsetY);
		uint16_t tileBase = interlace2 ? (uint16_t)(tileIdx * 64u) : (uint16_t)(tileIdx * 32u);

		for (uint8_t px = 0; px < 8u; px++) {
			int16_t screenX = (int16_t)(cell.x + (int16_t)(cell.screenCellCol * 8u + px));
			if (screenX < 0 || screenX >= (int16_t)_lineScreenWidth) {
				continue;
			}

			uint8_t col = cell.hFlip ? (uint8_t)(7u - px) : px;
			uint8_t row = cell.vFlip ? (uint8_t)(cellPixH - 1u - cell.pixRow) : cell.pixRow;
			uint8_t pix = FetchTilePixel(tileBase, row, col);
			if (pix == 0) {
				continue;
			}

			if (dst[screenX] != 0) {
				_state.StatusRegister |= 0x0020u;
				continue;
			}

			dst[screenX] = (uint8_t)((cell.priority ? 0x80u : 0x00u) | (cell.palette << 4) | pix);
		}
	}

	_prevLineDotOverflow = lineDotOverflow;
}

namespace {
	static constexpr uint8_t kGenesisDacLevels[15] = {
		0, 27, 49, 71, 87, 103, 119, 130, 146, 157, 174, 190, 206, 228, 255
	};

	// RGB555 values are chosen so 5-bit expansion best matches the Genesis DAC levels.
	static constexpr uint8_t kGenesisDacLevelToRgb555[15] = {
		0, 3, 6, 9, 11, 13, 15, 16, 18, 19, 21, 23, 25, 28, 31
	};

	static uint16_t CramToRgb555WithShade(uint16_t cramColor, uint8_t shadeMode) {
		uint8_t r3 = (uint8_t)((cramColor >> 1) & 7u);
		uint8_t g3 = (uint8_t)((cramColor >> 5) & 7u);
		uint8_t b3 = (uint8_t)((cramColor >> 9) & 7u);

		uint8_t r5;
		uint8_t g5;
		uint8_t b5;
		if (shadeMode == 0) {
			r5 = kGenesisDacLevelToRgb555[r3];
			g5 = kGenesisDacLevelToRgb555[g3];
			b5 = kGenesisDacLevelToRgb555[b3];
		} else if (shadeMode == 2) {
			r5 = kGenesisDacLevelToRgb555[r3 + 7u];
			g5 = kGenesisDacLevelToRgb555[g3 + 7u];
			b5 = kGenesisDacLevelToRgb555[b3 + 7u];
		} else {
			r5 = kGenesisDacLevelToRgb555[r3 << 1];
			g5 = kGenesisDacLevelToRgb555[g3 << 1];
			b5 = kGenesisDacLevelToRgb555[b3 << 1];
		}
		return (uint16_t)((b5 << 10) | (g5 << 5) | r5);
	}
}

// Convert Genesis CRAM color (0000BBB0GGG0RRR0) to RGB555 where bits 4:0 are red.
uint16_t GenesisVdp::CramToRgb555(uint16_t cramColor) const {
	uint8_t r3 = (uint8_t)((cramColor >> 1) & 7u);
	uint8_t g3 = (uint8_t)((cramColor >> 5) & 7u);
	uint8_t b3 = (uint8_t)((cramColor >> 9) & 7u);

	// Normal mode uses DAC levels[channel3 << 1], mapped to closest RGB555 value.
	uint8_t r5 = kGenesisDacLevelToRgb555[r3 << 1];
	uint8_t g5 = kGenesisDacLevelToRgb555[g3 << 1];
	uint8_t b5 = kGenesisDacLevelToRgb555[b3 << 1];

	return (uint16_t)((b5 << 10) | (g5 << 5) | r5);
}

void GenesisVdp::UpdatePaletteEntry(uint8_t idx) {
	idx &= 0x3Fu;
	uint16_t cramColor = _cram[idx];
	_palette[idx] = CramToRgb555WithShade(cramColor, 1);
	_shadowPalette[idx] = CramToRgb555WithShade(cramColor, 0);
	_highlightPalette[idx] = CramToRgb555WithShade(cramColor, 2);
}

void GenesisVdp::RefreshPaletteCache() {
	for (uint8_t i = 0; i < 64u; i++) {
		UpdatePaletteEntry(i);
	}
}

void GenesisVdp::Composite(uint16_t* lineBuffer, const uint8_t* planeB, const uint8_t* planeA, const uint8_t* spr, uint16_t pixels) const {
	uint8_t bgIdx = _state.Registers[7] & 0x3Fu;
	bool shadowHighlight = (_state.Registers[12] & 0x08u) != 0;

	enum : uint8_t { Shade_Shadow = 0, Shade_Normal = 1, Shade_Highlight = 2 };
	enum : uint8_t {
		Winner_Backdrop = 0,
		Winner_HiWindow = 1,
		Winner_HiSprite = 2,
		Winner_HiPlaneA = 3,
		Winner_HiPlaneB = 4,
		Winner_LoWindow = 5,
		Winner_LoSprite = 6,
		Winner_LoPlaneA = 7,
		Winner_LoPlaneB = 8
	};

	uint16_t nonBackdropCount = 0;
	uint16_t shadowCount = 0;
	uint16_t highlightCount = 0;
	uint16_t operatorSpriteCount = 0;
	uint16_t winnerDigest = 0;

	for (uint16_t x = 0; x < pixels; x++) {
		uint8_t pB = planeB[x];
		uint8_t pA = planeA[x];
		uint8_t pS = spr[x];

		bool winSrc = (pA & 0x40u) != 0;
		bool winHi = winSrc && ((pA & 0x80u) != 0);
		bool winVis = winSrc && ((pA & 0x0Fu) != 0);
		bool sprHi = (pS & 0x80u) != 0;
		bool sprVis = (pS & 0x0Fu) != 0;
		bool pAHi = !winSrc && ((pA & 0x80u) != 0);
		bool pAVis = !winSrc && ((pA & 0x0Fu) != 0);
		bool pBHi = (pB & 0x80u) != 0;
		bool pBVis = (pB & 0x0Fu) != 0;

		uint8_t shade = Shade_Normal;
		bool sprIsOperator = false;
		if (shadowHighlight) {
			if ((winHi && winVis) || (pAHi && pAVis) || (pBHi && pBVis)) {
				shade = Shade_Normal;
			}

			if (sprVis) {
				uint8_t sprPal = (pS >> 4) & 0x03u;
				uint8_t sprColor = pS & 0x0Fu;
				if (!sprHi && sprPal == 3u) {
					if (sprColor == 14u) {
						shade = Shade_Shadow;
						sprIsOperator = true;
					} else if (sprColor == 15u) {
						shade = (shade == Shade_Shadow) ? Shade_Normal : Shade_Highlight;
						sprIsOperator = true;
					}
				} else if (sprHi || sprColor == 15u) {
					shade = Shade_Normal;
				}
			}
		}

		uint8_t cramIdx = bgIdx;
		bool backdrop = true;
		uint8_t winner = Winner_Backdrop;
		if (winHi && winVis) {
			cramIdx = (uint8_t)((((pA >> 4) & 3u) * 16u) + (pA & 0x0Fu));
			backdrop = false;
			winner = Winner_HiWindow;
		} else if (sprHi && sprVis && !sprIsOperator) {
			cramIdx = (uint8_t)((((pS >> 4) & 3u) * 16u) + (pS & 0x0Fu));
			backdrop = false;
			winner = Winner_HiSprite;
		} else if (pAHi && pAVis) {
			cramIdx = (uint8_t)((((pA >> 4) & 3u) * 16u) + (pA & 0x0Fu));
			backdrop = false;
			winner = Winner_HiPlaneA;
		} else if (pBHi && pBVis) {
			cramIdx = (uint8_t)((((pB >> 4) & 3u) * 16u) + (pB & 0x0Fu));
			backdrop = false;
			winner = Winner_HiPlaneB;
		} else if (!winHi && winVis) {
			cramIdx = (uint8_t)((((pA >> 4) & 3u) * 16u) + (pA & 0x0Fu));
			backdrop = false;
			winner = Winner_LoWindow;
		} else if (!sprHi && sprVis && !sprIsOperator) {
			cramIdx = (uint8_t)((((pS >> 4) & 3u) * 16u) + (pS & 0x0Fu));
			backdrop = false;
			winner = Winner_LoSprite;
		} else if (!pAHi && pAVis) {
			cramIdx = (uint8_t)((((pA >> 4) & 3u) * 16u) + (pA & 0x0Fu));
			backdrop = false;
			winner = Winner_LoPlaneA;
		} else if (!pBHi && pBVis) {
			cramIdx = (uint8_t)((((pB >> 4) & 3u) * 16u) + (pB & 0x0Fu));
			backdrop = false;
			winner = Winner_LoPlaneB;
		}

		cramIdx &= 0x3Fu;
		if (!backdrop) {
			nonBackdropCount++;
		}
		if (sprIsOperator) {
			operatorSpriteCount++;
		}
		if (shade == Shade_Shadow) {
			shadowCount++;
		} else if (shade == Shade_Highlight) {
			highlightCount++;
		}
		winnerDigest = (uint16_t)((winnerDigest * 131u) ^ (uint16_t)((winner << 2) | (shade & 0x03u)));

		if (shadowHighlight) {
			if (shade == Shade_Shadow) {
				lineBuffer[x] = _shadowPalette[cramIdx];
			} else if (shade == Shade_Highlight) {
				lineBuffer[x] = _highlightPalette[cramIdx];
			} else {
				lineBuffer[x] = _palette[cramIdx];
			}
		} else {
			lineBuffer[x] = _palette[cramIdx];
		}
	}

	if (_memoryManager != nullptr && _state.FrameCount <= 160u && (_scanline % 16u) == 0u) {
		uint16_t packedCounts = (uint16_t)((nonBackdropCount & 0x01FFu) | ((shadowCount & 0x003Fu) << 9));
		uint16_t packedAux = (uint16_t)(((_scanline & 0x00FFu) << 8) | (highlightCount & 0x00FFu));
		_memoryManager->TraceVdpStartupEvent("VDP_COMP", packedCounts, packedAux);
		uint16_t digestAux = (uint16_t)(((operatorSpriteCount & 0x00FFu) << 8) | (shadowHighlight ? 1u : 0u));
		_memoryManager->TraceVdpStartupEvent("VDP_SHAD", winnerDigest, digestAux);
	}
}

// Port access
uint16_t GenesisVdp::ReadDataPort() {
	_pendingControlWrite = false;
	uint16_t result = _state.DataPortBuffer;
	PrimeReadBuffer();
	return result;
}

void GenesisVdp::PrimeReadBuffer() {
	uint8_t accessMode = _accessMode & 0x0Fu;
	uint16_t next = _state.DataPortBuffer;

	if (accessMode == 0x00u) {
		uint32_t addr = _addressReg & 0xFFFFu;
		next = (uint16_t)(((uint16_t)_vram[addr] << 8) | _vram[(addr + 1u) & 0xFFFFu]);
		_addressReg = (uint16_t)(_addressReg + _autoIncrement);
	} else if (accessMode == 0x04u) {
		uint8_t idx = (uint8_t)((_addressReg >> 1) & 0x3Fu);
		if (idx < 40u) {
			next = _vsram[idx];
		} else {
			next = 0;
		}
		_addressReg = (uint16_t)(_addressReg + _autoIncrement);
	} else if (accessMode == 0x08u) {
		uint8_t idx = (uint8_t)((_addressReg >> 1) & 0x3Fu);
		next = _cram[idx];
		_addressReg = (uint16_t)(_addressReg + _autoIncrement);
	}

	_state.DataPortBuffer = next;
}

uint16_t GenesisVdp::ReadControlPort() {
	_pendingControlWrite = false;
	_pendingControlHighWrite = false;

	bool inVblankLines = _scanline >= _screenHeight && _scanline < _totalLines;
	bool vblankFlagActive = false;
	if (inVblankLines) {
		if (_scanline > _screenHeight) {
			vblankFlagActive = true;
		} else {
			vblankFlagActive = _hCounter >= GetVBlankFlagStartCycle();
		}
	}

	if (vblankFlagActive) {
		_state.StatusRegister |= VdpStatus::VBlankFlag;
	} else {
		_state.StatusRegister &= ~VdpStatus::VBlankFlag;
	}

	if (_hCounter >= GetHBlankStartCycle()) {
		_state.StatusRegister |= VdpStatus::HBlanking;
	} else {
		_state.StatusRegister &= ~VdpStatus::HBlanking;
	}
	if (_state.DmaActive) {
		_state.StatusRegister |= VdpStatus::DmaBusy;
	} else {
		_state.StatusRegister &= ~VdpStatus::DmaBusy;
	}
	UpdateFifoStatusBits();

	uint16_t status = _state.StatusRegister;

	static uint64_t controlReadCount = 0;
	controlReadCount++;
	if (controlReadCount <= 128 || (controlReadCount % 4096) == 0) {
		uint8_t statusLo = (uint8_t)(status & 0xff);
		bool vblankBit = (statusLo & (uint8_t)VdpStatus::VBlankFlag) != 0;
		MessageManager::Log(std::format("[Genesis][VDP] CtrlRead #{} status=${:04x} lo=${:02x} vb={} display={} scanline={} hcounter={} r1=${:02x}",
			controlReadCount,
			status,
			statusLo,
			vblankBit ? 1 : 0,
			IsDisplayEnabled() ? 1 : 0,
			_scanline,
			_hCounter,
			_state.Registers[1]));
	}

	// Hardware clears the latched VINT-pending bit when status is read.
	if (status & VdpStatus::VIntPending) {
		static uint64_t clearOnReadCount = 0;
		clearOnReadCount++;
		if (clearOnReadCount <= 256 || (clearOnReadCount % 2048) == 0) {
			MessageManager::Log(std::format("[Genesis][VDP] CtrlReadClearVInt #{} frame={} scanline={} hc={} statusBefore=${:04x}",
				clearOnReadCount,
				_state.FrameCount,
				_scanline,
				_hCounter,
				status));
		}
	}
	_vintPending = false;
	_vintNew = false;
	_state.StatusRegister &= ~VdpStatus::VIntPending;
	_state.StatusRegister &= ~VdpStatus::SprOverflow;
	_state.StatusRegister &= ~VdpStatus::SprCollision;
	return status;
}

void GenesisVdp::AcknowledgeInterrupt(uint8_t level) {
	if (level >= 6 && _vintPending && IsVBlankInterruptEnabled()) {
		_vintPending = false;
		_vintNew = false;
		_state.StatusRegister &= ~VdpStatus::VIntPending;
	} else if (level >= 4 && _hintPending && IsHBlankInterruptEnabled()) {
		_hintPending = false;
		_hintNew = false;
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
	if ((_state.Registers[0] & 0x02u) != 0) {
		return _hvCounterLatch;
	}

	uint8_t v = VCounterValue(_scanline);
	uint8_t h = HCounterValue(_hCounter, _lineH40Mode);
	return ((uint16_t)v << 8) | h;
}

void GenesisVdp::WriteDataPortByte(uint8_t value, bool highByte) {
	if (highByte) {
		_dataHighWrite = value;
		_pendingDataHighWrite = true;
		return;
	}

	uint16_t word = _pendingDataHighWrite
		? (uint16_t)(((uint16_t)_dataHighWrite << 8) | value)
		: (uint16_t)value;
	_pendingDataHighWrite = false;
	WriteDataPort(word);
}

void GenesisVdp::WriteControlPortByte(uint8_t value, bool highByte) {
	if (highByte) {
		_controlHighWrite = value;
		_pendingControlHighWrite = true;
		return;
	}

	uint16_t word = _pendingControlHighWrite
		? (uint16_t)(((uint16_t)_controlHighWrite << 8) | value)
		: (uint16_t)value;
	_pendingControlHighWrite = false;
	WriteControlPort(word);
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

		uint32_t addr = _addressReg & 0xFFFF;
		uint8_t hi = (uint8_t)(value >> 8);
		uint8_t lo = (uint8_t)(value & 0xFF);
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

		bool activeDisplay = _lineDisplayEnabled && _scanline < _screenHeight;
		if (activeDisplay) {
			EnqueueWriteFifo(accessMode, _addressReg, value);
		} else {
			ApplyPortWrite(accessMode, _addressReg, value);
		}
	} else if (accessMode == 3) { // CRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		uint16_t cramValue = value;

		if (!loggedFirstNonZeroCram && cramValue != 0) {
			loggedFirstNonZeroCram = true;
			MessageManager::Log(std::format("[Genesis][VDP] First non-zero CRAM write at idx {}: ${:04x} (raw=${:04x}, dataWrite#{}, frame={})",
				idx,
				cramValue,
				value,
				dataWriteCount,
				_state.FrameCount));
		}

		bool activeDisplay = _lineDisplayEnabled && _scanline < _screenHeight;
		if (activeDisplay) {
			EnqueueWriteFifo(accessMode, _addressReg, value);
		} else {
			ApplyPortWrite(accessMode, _addressReg, value);
		}
	} else if (accessMode == 5) { // VSRAM write
		uint8_t idx = (_addressReg / 2) & 0x3F;
		if (idx < 40) {
			uint16_t vsramValue = value & 0x07FF;

			if (!loggedFirstNonZeroVsram && vsramValue != 0) {
				loggedFirstNonZeroVsram = true;
				MessageManager::Log(std::format("[Genesis][VDP] First non-zero VSRAM write at idx {}: ${:04x} (raw=${:04x}, dataWrite#{}, frame={})",
					idx,
					vsramValue,
					value,
					dataWriteCount,
					_state.FrameCount));
			}

			bool activeDisplay = _lineDisplayEnabled && _scanline < _screenHeight;
			if (activeDisplay) {
				EnqueueWriteFifo(accessMode, _addressReg, value);
			} else {
				ApplyPortWrite(accessMode, _addressReg, value);
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

void GenesisVdp::EnqueueWriteFifo(uint8_t accessMode, uint16_t address, uint16_t value) {
	if (_writeFifoCount >= 4) {
		DrainWriteFifoOne();
	}

	_writeFifo[_writeFifoWrite].AccessMode = accessMode;
	_writeFifo[_writeFifoWrite].Address = address;
	_writeFifo[_writeFifoWrite].Value = value;
	_writeFifoWrite = (_writeFifoWrite + 1u) & 0x03u;
	if (_writeFifoCount < 4) {
		_writeFifoCount++;
	}
	UpdateFifoStatusBits();
}

void GenesisVdp::DrainWriteFifoOne() {
	if (_writeFifoCount == 0) {
		return;
	}

	const VdpWriteFifoEntry& entry = _writeFifo[_writeFifoRead];
	ApplyPortWrite(entry.AccessMode, entry.Address, entry.Value);
	_writeFifoRead = (_writeFifoRead + 1u) & 0x03u;
	_writeFifoCount--;
	UpdateFifoStatusBits();
}

void GenesisVdp::ApplyPortWrite(uint8_t accessMode, uint16_t address, uint16_t value) {
	switch (accessMode & 0x0Fu) {
		case 0x00:
		case 0x01: {
			uint32_t addr = address & 0xFFFFu;
			if (addr + 1u < VramSize) {
				_vram[addr] = (uint8_t)(value >> 8);
				_vram[addr + 1u] = (uint8_t)value;
			}
			break;
		}
		case 0x03: {
			uint8_t idx = (uint8_t)((address >> 1) & 0x3Fu);
			_cram[idx] = value;
			UpdatePaletteEntry(idx);
			break;
		}
		case 0x05: {
			uint8_t idx = (uint8_t)((address >> 1) & 0x3Fu);
			if (idx < 40) {
				_vsram[idx] = value & 0x07FFu;
			}
			break;
		}
		default:
			break;
	}
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
		_state.CodeRegister = _accessMode;
		_state.AddressRegister = _addressReg;

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
		} else {
			PrimeReadBuffer();
		}
		return;
	}

	if ((value & 0xE000) == 0x8000) {
		// Register write: 100R RRRR DDDD DDDD
		uint8_t reg = (value >> 8) & 0x1F;
		uint8_t data = value & 0xFF;
		uint8_t oldData = reg < 24 ? _state.Registers[reg] : 0;
		bool atLineBoundary = _hCounter == 0;
		bool displayEnabledBefore = IsDisplayEnabled();
		if (reg < 24) {
			_state.Registers[reg] = data;
		}

		if (reg == 0) {
			bool latchTransition = ((oldData & 0x02u) == 0) && ((data & 0x02u) != 0);
			if (latchTransition) {
				_state.Registers[0] = (uint8_t)(data & ~0x02u);
				_hvCounterLatch = ReadHVCounter();
				_state.Registers[0] = data;
			}
		}

		// Auto-increment from register 15
		if (reg == 15) _autoIncrement = data;
		if (reg == VdpReg::HIntCounter) _state.HIntCounter = data;

		// Update screen mode
		// Screen width is applied on scanline boundaries via line latch in Run().
		// If a write occurs exactly at the line boundary, apply it immediately to
		// the line latch so pre-line setup affects the upcoming rendered line.
		if (reg == 12 && atLineBoundary) {
			_lineH40Mode = IsH40Mode();
			_lineScreenWidth = _lineH40Mode ? 320 : 256;
			_screenWidth = _lineScreenWidth;
		}

		if (reg == 1) {
			bool displayEnabledAfter = IsDisplayEnabled();
			if (atLineBoundary) {
				_lineDisplayEnabled = displayEnabledAfter;
			}

			bool vintEnableRise = ((oldData & 0x20u) == 0u) && ((data & 0x20u) != 0u);
			if (vintEnableRise && _vintPending && !_vintNew) {
				_vintNew = true;
				if (_cpu) {
					_cpu->SetInterrupt(6);
				}
			}

			if (displayEnabledAfter != displayEnabledBefore) {
				MessageManager::Log(std::format("[Genesis][VDP] Display {} via R1 write (${:#04x})", displayEnabledAfter ? "enabled" : "disabled", data));
			}
		}

		if (reg == 0) {
			bool hintEnableRise = ((oldData & 0x10u) == 0u) && ((data & 0x10u) != 0u);
			if (hintEnableRise && _hintPending && !_hintNew) {
				_hintNew = true;
				if (_cpu) {
					_cpu->SetInterrupt(4);
				}
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
	_state.CodeRegister = _accessMode;
	_state.AddressRegister = _addressReg;

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
	bool blanking = !_lineDisplayEnabled || _scanline >= _screenHeight;
	if (blanking) {
		// Approximation informed by Mesen2-Expanded: blanking DMA is faster.
		return _lineH40Mode ? 5 : 6;
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

	return _lineH40Mode ? h40Lookup[cycle] : h32Lookup[cycle];
}

void GenesisVdp::ProcessDma() {
	if (!_state.DmaActive) return;

	if (!_dmaInitialized) {
		_dmaSourceReg23Latched = _state.Registers[23];
		_dmaLatchedMode = (uint8_t)((_state.Registers[23] >> 6) & 3);
		_dmaRemainingWords = ((uint16_t)_state.Registers[20] << 8) | _state.Registers[19];
		if (_dmaRemainingWords == 0) {
			_dmaRemainingWords = 0x10000;
		}

		// Mesen2-Expanded keeps the full low 7 bits of R23 in the initial bus-DMA source assembly.
		_dmaSourceAddress = ((uint32_t)(_state.Registers[23] & 0x7F) << 17)
		                | ((uint32_t)_state.Registers[22] << 9)
		                | ((uint32_t)_state.Registers[21] << 1);
		_dmaCopySourceAddress = ((uint16_t)_state.Registers[22] << 8) | _state.Registers[21];

		if (_dmaLatchedMode <= 1) {
			_state.DmaMode = 0;
			// Startup latency approximation for 68K-bus DMA in the core-cycle domain.
			_dmaStartupDelayCyclesRemaining = _lineH40Mode ? 7 : 9;
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

		bool activeDisplay = _lineDisplayEnabled && _scanline < _screenHeight;
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
		uint8_t dmaDestCode = _accessMode & 0x0Fu;
		uint32_t dmaDst = _addressReg;
		for (uint32_t i = 0; i < wordsThisStep; i++) {
			// Expanded-compatible source progression: wrap within current 128KB source window.
			uint32_t srcBase = _dmaSourceAddress & ~0x1FFFFu;
			uint32_t srcOffset = _dmaSourceAddress & 0x1FFFFu;

			uint8_t hi = _memoryManager->Read8(srcBase | srcOffset);
			uint8_t lo = _memoryManager->Read8(srcBase | ((srcOffset + 1u) & 0x1FFFFu));
			uint16_t word = ((uint16_t)hi << 8) | lo;

			switch (dmaDestCode) {
				case 0x01: {
					uint32_t addr = dmaDst & 0xFFFFu;
					if (addr + 1u < VramSize) {
						_vram[addr] = (uint8_t)(word >> 8);
						_vram[addr + 1u] = (uint8_t)word;
					}
					break;
				}
				case 0x03: {
					uint8_t idx = (uint8_t)((dmaDst >> 1) & 0x3Fu);
					_cram[idx] = word;
					UpdatePaletteEntry(idx);
					break;
				}
				case 0x05: {
					uint8_t idx = (uint8_t)((dmaDst >> 1) & 0x27u);
					_vsram[idx] = word & 0x07FFu;
					break;
				}
				default:
					break;
			}

			srcOffset = (srcOffset + 2u) & 0x1FFFFu;
			_dmaSourceAddress = srcBase | srcOffset;
			dmaDst += _autoIncrement;
		}
		_addressReg = (uint16_t)dmaDst;

		uint32_t srcWindowWordAddress = (_dmaSourceAddress & 0x1FFFFu) >> 1;
		_state.Registers[21] = (uint8_t)(srcWindowWordAddress & 0xFF);
		_state.Registers[22] = (uint8_t)((srcWindowWordAddress >> 8) & 0xFF);
		// R23 remains software-visible during active DMA writes while source progression
		// uses the latched DMA startup source for transfer determinism.
	} else if (_dmaLatchedMode == 2) {
		if (_dmaFillDataPending) {
			// In isolated/scaffold runs there may be no subsequent data-port write to seed fill data.
			// Fall back to the current fill byte (default 0x00) to avoid a permanent DMA busy stall.
			_dmaFillDataPending = false;
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
	SV(_lineDisplayEnabled);
	SV(_lineH40Mode);
	SV(_lineScreenWidth);
	SV(_vblankEnteredThisFrame);
	SV(_vintPending);
	SV(_vintNew);
	SV(_hintPending);
	SV(_hintNew);
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
	SV(_dmaSourceReg23Latched);
	SV(_writeFifoRead);
	SV(_writeFifoWrite);
	SV(_writeFifoCount);
	SV(_hvCounterLatch);
	for (uint8_t i = 0; i < 4; i++) {
		SV(_writeFifo[i].Address);
		SV(_writeFifo[i].Value);
		SV(_writeFifo[i].AccessMode);
	}
	SV(_prevLineDotOverflow);
	for (int i = 0; i < 24; i++) {
		SVI(_state.Registers[i]);
	}

	if (!s.IsSaving()) {
		RefreshPaletteCache();
		UpdateFifoStatusBits();
	}
}
