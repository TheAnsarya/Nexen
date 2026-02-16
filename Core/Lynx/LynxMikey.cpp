#include "pch.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMemoryManager.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

void LynxMikey::Init(Emulator* emu, LynxConsole* console, LynxCpu* cpu, LynxMemoryManager* memoryManager) {
	_emu = emu;
	_console = console;
	_cpu = cpu;
	_memoryManager = memoryManager;

	memset(&_state, 0, sizeof(_state));
	memset(_frameBuffer, 0, sizeof(_frameBuffer));

	// Initialize hardware revision (Lynx II = 0x04)
	_state.HardwareRevision = 0x04;

	// Initialize all timers
	for (int i = 0; i < 8; i++) {
		_state.Timers[i].BackupValue = 0;
		_state.Timers[i].ControlA = 0;
		_state.Timers[i].Count = 0;
		_state.Timers[i].ControlB = 0;
		_state.Timers[i].LastTick = 0;
		_state.Timers[i].TimerDone = false;
		_state.Timers[i].Linked = false;
	}

	// Default palette (all black)
	for (int i = 0; i < 16; i++) {
		_state.PaletteGreen[i] = 0;
		_state.PaletteBR[i] = 0;
		_state.Palette[i] = 0xFF000000; // Opaque black
	}

	_state.IrqEnabled = 0;
	_state.IrqPending = 0;
	_state.CurrentScanline = 0;
	_state.DisplayAddress = 0;
	_state.DisplayControl = 0;
}

__forceinline int LynxMikey::GetTimerIndex(uint8_t addr) const {
	// Timers 0-3 at $FD00-$FD0F, Timers 4-7 at $FD10-$FD1F
	// But the actual layout is interleaved:
	// Timer 0: $FD00-$FD03, Timer 2: $FD04-$FD07, Timer 4: $FD08-$FD0B
	// Timer 1: $FD08-$FD0B... Actually:
	// Lynx timer registers: Timer N at $FD00 + N*4 for 0-3, $FD10 + (N-4)*4 for 4-7
	// Wait, the actual Lynx layout:
	// $FD00-$FD03: Timer 0 (HCount)
	// $FD04-$FD07: Timer 1
	// $FD08-$FD0B: Timer 2 (VCount)
	// $FD0C-$FD0F: Timer 3
	// $FD10-$FD13: Timer 4
	// $FD14-$FD17: Timer 5
	// $FD18-$FD1B: Timer 6
	// $FD1C-$FD1F: Timer 7
	if (addr < 0x20) {
		return (addr >> 2);
	}
	return -1;
}

__forceinline int LynxMikey::GetTimerRegOffset(uint8_t addr) const {
	return addr & 0x03;
}

void LynxMikey::TickTimer(int index, uint64_t currentCycle) {
	LynxTimerState& timer = _state.Timers[index];

	// Check if timer is enabled (bit 3 of CTLA)
	bool enabled = (timer.ControlA & 0x08) != 0;
	if (!enabled) {
		return;
	}

	// Check if timer is linked (clock source = 7)
	uint8_t clockSource = timer.ControlA & 0x07;
	if (clockSource == 7) {
		// Linked timer — only ticked by cascade, not by clock
		return;
	}

	uint32_t period = _prescalerPeriods[clockSource];
	if (period == 0) {
		return;
	}

	// Calculate how many ticks have elapsed since last update
	while (currentCycle - timer.LastTick >= period) {
		timer.LastTick += period;
		timer.Count--;

		if (timer.Count == 0xff) { // Underflow (wrapped from 0 to 0xFF)
			timer.TimerDone = true;
			timer.ControlB |= 0x08; // Set timer-done flag in CTLB

			// Fire IRQ if enabled (bit 7 of CTLA)
			if (timer.ControlA & 0x80) {
				SetIrqSource(static_cast<LynxIrqSource::LynxIrqSource>(1 << index));
			}

			// Reload from backup value
			timer.Count = timer.BackupValue;

			// Cascade to linked timer
			CascadeTimer(index);

			// Timer 0 = horizontal timer, triggers scanline processing
			if (index == 0) {
				RenderScanline();
				_state.CurrentScanline++;

				// Timer 2 = vertical timer
				if (_state.CurrentScanline >= LynxConstants::ScanlineCount) {
					_state.CurrentScanline = 0;
				}
			}
		}
	}
}

void LynxMikey::CascadeTimer(int sourceIndex) {
	int target = _timerLinkTarget[sourceIndex];
	if (target < 0) {
		return;
	}

	LynxTimerState& timer = _state.Timers[target];

	// Only cascade if target is linked (clock source = 7) and enabled
	bool enabled = (timer.ControlA & 0x08) != 0;
	uint8_t clockSource = timer.ControlA & 0x07;
	if (!enabled || clockSource != 7) {
		return;
	}

	timer.Count--;
	if (timer.Count == 0xff) { // Underflow
		timer.TimerDone = true;
		timer.ControlB |= 0x08;

		if (timer.ControlA & 0x80) {
			SetIrqSource(static_cast<LynxIrqSource::LynxIrqSource>(1 << target));
		}

		timer.Count = timer.BackupValue;

		// Continue cascading
		CascadeTimer(target);
	}
}

void LynxMikey::UpdatePalette(int index) {
	uint8_t green = _state.PaletteGreen[index] & 0x0f;
	uint8_t blue = (_state.PaletteBR[index] >> 4) & 0x0f;
	uint8_t red = _state.PaletteBR[index] & 0x0f;

	// Expand 4-bit to 8-bit: replicate nibble
	uint32_t r = (red << 4) | red;
	uint32_t g = (green << 4) | green;
	uint32_t b = (blue << 4) | blue;

	_state.Palette[index] = 0xFF000000 | (r << 16) | (g << 8) | b;
}

void LynxMikey::UpdateIrqLine() {
	bool irqActive = (_state.IrqPending & _state.IrqEnabled) != 0;
	_cpu->SetIrqLine(irqActive);
}

void LynxMikey::RenderScanline() {
	uint16_t scanline = _state.CurrentScanline;
	if (scanline >= LynxConstants::ScreenHeight) {
		return; // VBlank period, nothing to render
	}

	// DMA not enabled check
	if (!(_state.DisplayControl & 0x01)) {
		return;
	}

	// Read 80 bytes from RAM (160 pixels at 4bpp = 80 bytes per line)
	// Base address from DISPADR, offset by scanline * 80
	uint16_t lineAddr = _state.DisplayAddress + (scanline * LynxConstants::BytesPerScanline);
	uint32_t* destLine = &_frameBuffer[scanline * LynxConstants::ScreenWidth];
	uint8_t* workRam = _console->GetWorkRam();

	for (int x = 0; x < LynxConstants::BytesPerScanline; x++) {
		uint8_t byte = workRam[(lineAddr + x) & 0xFFFF];

		// High nibble = first pixel, low nibble = second pixel
		uint8_t pix0 = (byte >> 4) & 0x0f;
		uint8_t pix1 = byte & 0x0f;

		destLine[x * 2] = _state.Palette[pix0];
		destLine[x * 2 + 1] = _state.Palette[pix1];
	}
}

void LynxMikey::SetIrqSource(LynxIrqSource::LynxIrqSource source) {
	_state.IrqPending |= (uint8_t)source;
	UpdateIrqLine();
}

void LynxMikey::ClearIrqSource(LynxIrqSource::LynxIrqSource source) {
	_state.IrqPending &= ~(uint8_t)source;
	UpdateIrqLine();
}

bool LynxMikey::HasPendingIrq() const {
	return (_state.IrqPending & _state.IrqEnabled) != 0;
}

void LynxMikey::Tick(uint64_t currentCycle) {
	for (int i = 0; i < 8; i++) {
		TickTimer(i, currentCycle);
	}
}

uint8_t LynxMikey::ReadRegister(uint8_t addr) {
	// Timer registers: $FD00-$FD1F
	if (addr < 0x20) {
		int timerIdx = GetTimerIndex(addr);
		int regOff = GetTimerRegOffset(addr);
		if (timerIdx >= 0 && timerIdx < 8) {
			LynxTimerState& timer = _state.Timers[timerIdx];
			switch (regOff) {
				case 0: return timer.BackupValue;
				case 1: return timer.ControlA;
				case 2: return timer.Count;
				case 3: return timer.ControlB;
			}
		}
		return 0xff;
	}

	// Interrupt registers
	switch (addr) {
		case 0x80: // INTSET — read pending IRQs
			return _state.IrqPending;

		case 0x81: // INTRST — not readable, returns open bus
			return 0xff;

		// Display registers
		case 0x92: // DISPCTL
			return _state.DisplayControl;

		case 0x94: // DISPADR low
			return (uint8_t)(_state.DisplayAddress & 0xff);

		case 0x95: // DISPADR high
			return (uint8_t)((_state.DisplayAddress >> 8) & 0xff);

		// Palette GREEN registers $FDA0-$FDAF
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			return _state.PaletteGreen[addr - 0xa0];

		// Palette BLUERED registers $FDB0-$FDBF
		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			return _state.PaletteBR[addr - 0xb0];

		// Serial port
		case 0x8c: // SERDAT
			return _state.SerialData;
		case 0x8d: // SERCTL
			return _state.SerialControl;

		// Hardware revision
		case 0x88: // IODIR (stub)
			return 0;
		case 0x89: // IODAT (stub)
			return 0;

		// SYSCTL1
		case 0x8b:
			return 0; // stub

		// MIKEYHREV
		case 0x84:
			return _state.HardwareRevision;

		default:
			return 0xff;
	}
}

void LynxMikey::WriteRegister(uint8_t addr, uint8_t value) {
	// Timer registers: $FD00-$FD1F
	if (addr < 0x20) {
		int timerIdx = GetTimerIndex(addr);
		int regOff = GetTimerRegOffset(addr);
		if (timerIdx >= 0 && timerIdx < 8) {
			LynxTimerState& timer = _state.Timers[timerIdx];
			switch (regOff) {
				case 0: // BACKUP
					timer.BackupValue = value;
					break;
				case 1: // CTLA
					timer.ControlA = value;
					timer.Linked = (value & 0x07) == 7;
					// If bit 6 set, reset count to backup on next clock
					if (value & 0x40) {
						timer.Count = timer.BackupValue;
					}
					break;
				case 2: // COUNT
					timer.Count = value;
					break;
				case 3: // CTLB
					// Writing clears the timer-done flag
					timer.ControlB = 0;
					timer.TimerDone = false;
					break;
			}
		}
		return;
	}

	// Interrupt registers
	switch (addr) {
		case 0x80: // INTSET — write sets IRQ bits (software IRQ)
			_state.IrqPending |= value;
			UpdateIrqLine();
			return;

		case 0x81: // INTRST — write clears IRQ bits
			_state.IrqPending &= ~value;
			UpdateIrqLine();
			return;

		// Display registers
		case 0x92: // DISPCTL
			_state.DisplayControl = value;
			return;

		case 0x94: // DISPADR low
			_state.DisplayAddress = (_state.DisplayAddress & 0xff00) | value;
			return;

		case 0x95: // DISPADR high
			_state.DisplayAddress = (_state.DisplayAddress & 0x00ff) | ((uint16_t)value << 8);
			return;

		// Palette GREEN registers $FDA0-$FDAF
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			_state.PaletteGreen[addr - 0xa0] = value;
			UpdatePalette(addr - 0xa0);
			return;

		// Palette BLUERED registers $FDB0-$FDBF
		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			_state.PaletteBR[addr - 0xb0] = value;
			UpdatePalette(addr - 0xb0);
			return;

		// Serial port
		case 0x8c: // SERDAT
			_state.SerialData = value;
			return;
		case 0x8d: // SERCTL
			_state.SerialControl = value;
			return;

		// SYSCTL1
		case 0x8b:
			// TODO: system control (power off, cart power, etc.)
			return;

		default:
			return;
	}
}

void LynxMikey::Serialize(Serializer& s) {
	// Timer state
	for (int i = 0; i < 8; i++) {
		SV(_state.Timers[i].BackupValue);
		SV(_state.Timers[i].ControlA);
		SV(_state.Timers[i].Count);
		SV(_state.Timers[i].ControlB);
		SV(_state.Timers[i].LastTick);
		SV(_state.Timers[i].TimerDone);
		SV(_state.Timers[i].Linked);
	}

	// IRQ state
	SV(_state.IrqEnabled);
	SV(_state.IrqPending);

	// Display state
	SV(_state.DisplayAddress);
	SV(_state.DisplayControl);
	SV(_state.CurrentScanline);

	// Palette
	SVArray(_state.Palette, 16);
	SVArray(_state.PaletteGreen, 16);
	SVArray(_state.PaletteBR, 16);

	// Serial
	SV(_state.SerialData);
	SV(_state.SerialControl);
	SV(_state.HardwareRevision);

	// Frame buffer
	SVArray(_frameBuffer, LynxConstants::ScreenWidth * LynxConstants::ScreenHeight);
}
