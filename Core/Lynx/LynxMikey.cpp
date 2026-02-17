#include "pch.h"
#include "Lynx/LynxMikey.h"
#include "Lynx/LynxConsole.h"
#include "Lynx/LynxCpu.h"
#include "Lynx/LynxMemoryManager.h"
#include "Lynx/LynxApu.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

#include "Lynx/LynxEeprom.h"

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

	// Initialize UART to idle
	_state.SerialControl = 0;
	_state.UartTxCountdown = UartTxInactive;
	_state.UartRxCountdown = UartRxInactive;
	_state.UartTxData = 0;
	_state.UartRxData = 0;
	_state.UartRxReady = false;
	_state.UartTxIrqEnable = false;
	_state.UartRxIrqEnable = false;
	_state.UartParityEnable = false;
	_state.UartParityEven = false;
	_state.UartSendBreak = false;
	_state.UartRxOverrunError = false;
	_state.UartRxFramingError = false;

	memset(_uartRxQueue, 0, sizeof(_uartRxQueue));
	_uartRxInputPtr = 0;
	_uartRxOutputPtr = 0;
	_uartRxWaiting = 0;
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

	// HW Bug 13.6: Timer does not count while the Timer Done flag is set.
	// The Timer Done bit must be cleared (by writing to CTLB) before the
	// timer will resume counting. This is a hardware bug — the done flag
	// blocks the borrow output that drives the count enable.
	// Exception: Timer 4 (UART baud generator) always counts regardless
	// of TimerDone, as the UART needs continuous clocking.
	if (timer.TimerDone && index != 4) {
		// Still advance LastTick so we don't accumulate a huge delta
		timer.LastTick = currentCycle;
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
			if (index == 4) {
				// Timer 4 = UART baud rate generator.
				// Does not set TimerDone, does not fire normal IRQ.
				// Each underflow drives one UART clock tick (1 bit time).
				timer.Count = timer.BackupValue;
				TickUart();
				// Timer 4 keeps counting (no HW Bug 13.6 stop)
				continue;
			}

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

			// HW Bug 13.6: Stop counting now that Done is set
			break;
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

	// HW Bug 13.6: Timer does not count while Timer Done flag is set
	// Exception: Timer 4 (UART baud generator) always counts
	if (timer.TimerDone && target != 4) {
		return;
	}

	timer.Count--;
	if (timer.Count == 0xff) { // Underflow
		if (target == 4) {
			// Timer 4 = UART baud generator
			timer.Count = timer.BackupValue;
			TickUart();
			return;
		}

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
	// On real Lynx, there's no separate IRQ enable mask register.
	// IRQ enable/disable is controlled per-timer in each timer's CTLA bit 7.
	// The IrqPending bits are set only when a timer with IRQ enabled fires,
	// so we just check if any pending bits are set.
	bool irqActive = _state.IrqPending != 0;
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
	// No separate enable mask — IRQ enable is per-timer in CTLA bit 7
	return _state.IrqPending != 0;
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

	// Audio registers: $FD20-$FD4F → forwarded to APU
	if (addr >= 0x20 && addr < 0x50) {
		return _apu ? _apu->ReadRegister(addr - 0x20) : 0xff;
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

		// Serial port (ComLynx)
		case 0x8c: { // SERCTL ($FD8C) — serial status register (read)
			// Read returns status (different bit meanings from write):
			//   7: TxRdy   — transmitter buffer empty (ready for data)
			//   6: RxRdy   — receiver has data available
			//   5: TxEmpty — transmitter shift register idle
			//   4: Parerr  — parity error (not implemented)
			//   3: Overrun — receiver overrun
			//   2: Framerr — framing error
			//   1: Rxbrk   — break received
			//   0: Parbit  — 9th bit / parity
			uint8_t status = 0;
			// TxRdy (bit 7) + TxEmpty (bit 5) when not actively transmitting
			if (_state.UartTxCountdown & UartTxInactive) {
				status |= 0xa0;
			}
			if (_state.UartRxReady) {
				status |= 0x40;
			}
			if (_state.UartRxOverrunError) {
				status |= 0x08;
			}
			if (_state.UartRxFramingError) {
				status |= 0x04;
			}
			if (_state.UartRxData & UartBreakCode) {
				status |= 0x02;
			}
			if (_state.UartRxData & 0x0100) {
				status |= 0x01;
			}
			return status;
		}
		case 0x8d: // SERDAT ($FD8D) — serial receive data register
			// Reading clears RxReady and returns the received byte
			_state.UartRxReady = false;
			UpdateUartIrq();
			return (uint8_t)(_state.UartRxData & 0xff);

		// I/O registers — EEPROM is wired through these
		case 0x88: // IODIR ($FD88) — I/O direction register
			return _ioDir;
		case 0x89: // IODAT ($FD89) — I/O data register
		{
			// Bits configured as input (ioDir=0) read from external hardware
			// Bit 1: EEPROM data out (directly wired to EEPROM DO pin)
			// Other bits: directly wired pins (active high/low varies)
			uint8_t result = _ioData & _ioDir; // Output bits retain written value
			// Input bits — read from EEPROM
			if (_eeprom && !(_ioDir & 0x02)) {
				// Bit 1 is input: read EEPROM data out
				if (_eeprom->GetDataOut()) {
					result |= 0x02;
				} else {
					result &= ~0x02;
				}
			}
			return result;
		}

		// SYSCTL1 ($FD87)
		case 0x87:
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

	// Audio registers: $FD20-$FD4F → forwarded to APU
	if (addr >= 0x20 && addr < 0x50) {
		if (_apu) _apu->WriteRegister(addr - 0x20, value);
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

		// Serial port (ComLynx)
		case 0x8c: // SERCTL ($FD8C) — serial control register (write)
			// Write configures UART (different bit meanings from read):
			//   7: TxIntEn  — transmit interrupt enable
			//   6: RxIntEn  — receive interrupt enable
			//   5: (unused)
			//   4: Paren    — parity enable
			//   3: ResetErr — clear error flags (self-clearing)
			//   2: TxOpen   — open collector mode (not emulated)
			//   1: TxBreak  — send break signal
			//   0: ParEven  — even parity (or 9th bit when parity disabled)
			// HW Bug 13.2: Serial interrupt is level-sensitive. If TxIntEn is
			// set while TX is idle, IRQ fires continuously until cleared.
			_state.SerialControl = value;
			_state.UartTxIrqEnable = (value & 0x80) != 0;
			_state.UartRxIrqEnable = (value & 0x40) != 0;
			_state.UartParityEnable = (value & 0x10) != 0;
			_state.UartParityEven = (value & 0x01) != 0;

			// Bit 3: reset error flags
			if (value & 0x08) {
				_state.UartRxOverrunError = false;
				_state.UartRxFramingError = false;
			}

			// Bit 1: send break
			_state.UartSendBreak = (value & 0x02) != 0;
			if (_state.UartSendBreak) {
				_state.UartTxCountdown = UartTxTimePeriod;
				ComLynxTxLoopback(UartBreakCode);
			}

			UpdateUartIrq();
			return;
		case 0x8d: // SERDAT ($FD8D) — serial transmit data register
		{
			// Write starts transmission. Data loopbacks to RX (ComLynx bus).
			_state.UartTxData = value;

			// Handle parity / 9th bit
			if (!_state.UartParityEnable && _state.UartParityEven) {
				_state.UartTxData |= 0x0100; // Set 9th bit
			}

			// Start TX countdown (11 Timer 4 ticks = 11 bit times)
			_state.UartTxCountdown = UartTxTimePeriod;

			// ComLynx self-loopback: TX output feeds back to RX
			ComLynxTxLoopback(_state.UartTxData);
			return;
		}

		// I/O registers — EEPROM wiring
		case 0x88: // IODIR ($FD88) — I/O direction register
			_ioDir = value;
			return;

		case 0x89: // IODAT ($FD89) — I/O data register
		{
			uint8_t prev = _ioData;
			_ioData = value;
			if (_eeprom) {
				// Bit 0: EEPROM chip select (directly wired)
				_eeprom->SetChipSelect((value & 0x01) != 0);

				// Bit 1: EEPROM data in (directly wired)
				// Data is latched on clock edges, handled by ClockData below

				// Bit 2: EEPROM serial clock — trigger on rising edge
				if ((value & 0x04) && !(prev & 0x04)) {
					// Rising edge on clock line: latch data-in bit (bit 1)
					_eeprom->ClockData((value & 0x02) != 0);
				}
			}
			return;
		}

		// SYSCTL1 ($FD87)
		case 0x87:
			// TODO: system control (power off, cart power, etc.)
			return;

		default:
			return;
	}
}

uint32_t LynxMikey::GetFrameCount() const {
	return _console ? _console->GetFrameCount() : 0;
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

	// UART / ComLynx
	SV(_state.SerialControl);
	SV(_state.UartTxCountdown);
	SV(_state.UartRxCountdown);
	SV(_state.UartTxData);
	SV(_state.UartRxData);
	SV(_state.UartRxReady);
	SV(_state.UartTxIrqEnable);
	SV(_state.UartRxIrqEnable);
	SV(_state.UartParityEnable);
	SV(_state.UartParityEven);
	SV(_state.UartSendBreak);
	SV(_state.UartRxOverrunError);
	SV(_state.UartRxFramingError);

	// UART RX queue
	SVArray(_uartRxQueue, UartMaxRxQueue);
	SV(_uartRxInputPtr);
	SV(_uartRxOutputPtr);
	SV(_uartRxWaiting);

	SV(_state.HardwareRevision);

	// I/O registers (EEPROM wiring)
	SV(_ioDir);
	SV(_ioData);

	// Frame buffer
	SVArray(_frameBuffer, LynxConstants::ScreenWidth * LynxConstants::ScreenHeight);
}

// ============================================================================
// UART / ComLynx Implementation
// ============================================================================

void LynxMikey::TickUart() {
	// Called on each Timer 4 underflow — drives one UART clock tick.
	// 11 ticks = one serial frame (1 start + 8 data + 1 parity + 1 stop).

	// --- Receive ---
	if (_state.UartRxCountdown == 0) {
		// RX period complete: pull byte from input queue
		if (_uartRxWaiting > 0) {
			// Overrun check: previous data not yet read
			if (_state.UartRxReady) {
				_state.UartRxOverrunError = true;
			}

			_state.UartRxData = _uartRxQueue[_uartRxOutputPtr];
			_uartRxOutputPtr = (_uartRxOutputPtr + 1) % UartMaxRxQueue;
			_uartRxWaiting--;
			_state.UartRxReady = true;

			// If more data waiting, start inter-byte delay
			if (_uartRxWaiting > 0) {
				_state.UartRxCountdown = UartRxNextDelay;
			}
		}
	} else if (!(_state.UartRxCountdown & UartRxInactive)) {
		_state.UartRxCountdown--;
	}

	// --- Transmit ---
	if (_state.UartTxCountdown == 0) {
		// TX period complete
		if (_state.UartSendBreak) {
			// Break mode: continuously retransmit break signal
			_state.UartTxData = UartBreakCode;
			_state.UartTxCountdown = UartTxTimePeriod;
			ComLynxTxLoopback(_state.UartTxData);
		} else {
			// Normal completion: go idle
			_state.UartTxCountdown = UartTxInactive;
		}
	} else if (!(_state.UartTxCountdown & UartTxInactive)) {
		_state.UartTxCountdown--;
	}

	// Update serial IRQ (level-sensitive — HW Bug 13.2)
	UpdateUartIrq();
}

void LynxMikey::UpdateUartIrq() {
	// Serial IRQ uses Timer 4 IRQ line (bit 4).
	// Level-sensitive: asserts as long as condition persists.
	// Re-asserts on each check even if software cleared the pending bit.
	bool irq = false;

	// TX IRQ: transmitter idle (countdown == 0 or inactive) and TX IRQ enabled
	bool txIdle = (_state.UartTxCountdown == 0) ||
	              ((_state.UartTxCountdown & UartTxInactive) != 0);
	if (txIdle && _state.UartTxIrqEnable) {
		irq = true;
	}

	// RX IRQ: receive data ready and RX IRQ enabled
	if (_state.UartRxReady && _state.UartRxIrqEnable) {
		irq = true;
	}

	if (irq) {
		_state.IrqPending |= (uint8_t)LynxIrqSource::Timer4;
	}
	// Don't clear bit 4 here — let software clear via INTRST write.
	// Level-sensitivity is achieved by re-asserting each tick.

	UpdateIrqLine();
}

void LynxMikey::ComLynxTxLoopback(uint16_t data) {
	// ComLynx is a shared open-collector bus: transmitted data is received
	// by all connected units including the sender (mandatory self-loopback).
	ComLynxRxData(data);
}

void LynxMikey::ComLynxRxData(uint16_t data) {
	if (_uartRxWaiting < (uint32_t)UartMaxRxQueue) {
		// If queue was empty, start the RX countdown
		if (_uartRxWaiting == 0) {
			_state.UartRxCountdown = UartRxTimePeriod;
		}

		// Enqueue the byte
		_uartRxQueue[_uartRxInputPtr] = data;
		_uartRxInputPtr = (_uartRxInputPtr + 1) % UartMaxRxQueue;
		_uartRxWaiting++;
	}
	// If queue is full, data is silently lost.
	// Overrun error is detected when the next byte is delivered from queue.
}
