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

	_state = {};
	std::fill_n(_frameBuffer, std::size(_frameBuffer), 0u);

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

	// §13: Initialize UART to idle (power-on reset state)
	_state.SerialControl = 0;
	_state.UartTxCountdown = UartTxInactive;  // §6.3: TX idle sentinel
	_state.UartRxCountdown = UartRxInactive;  // §7.4: RX idle sentinel
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

	std::fill_n(_uartRxQueue, UartMaxRxQueue, uint16_t{0});
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
				// §5: Timer 4 = UART baud rate generator.
				// Does not set TimerDone, does not fire normal timer IRQ (§10).
				// Each underflow drives one UART clock tick (1 bit-time).
				// See §5 — Timer 4 update path.
				timer.Count = timer.BackupValue;
				TickUart();
				// Timer 4 keeps counting (not subject to HW Bug 13.6 stop)
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

	// HW Bug 13.6: Timer does not count while Timer Done flag is set (§5)
	// Exception: Timer 4 (UART baud generator) always counts
	if (timer.TimerDone && target != 4) {
		return;
	}

	timer.Count--;
	if (timer.Count == 0xff) { // Underflow
		// Timer 4 = UART baud generator — no TimerDone, calls TickUart()
		if (target == 4) {
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
	// Base address from DISPADR, offset by scanline * 80.
	// The & 0xFFFF mask below guarantees safe access — workRam is exactly
	// 64KB (WorkRamSize = 0x10000), so any DisplayAddress value is valid.
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
	_state.IrqPending |= static_cast<uint8_t>(source);
	UpdateIrqLine();
}

void LynxMikey::ClearIrqSource(LynxIrqSource::LynxIrqSource source) {
	_state.IrqPending &= ~static_cast<uint8_t>(source);
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

	// Audio registers: $FD20-$FD50 → forwarded to APU
	if (addr >= 0x20 && addr <= 0x50) {
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
			return static_cast<uint8_t>(_state.DisplayAddress & 0xff);

		case 0x95: // DISPADR high
			return static_cast<uint8_t>((_state.DisplayAddress >> 8) & 0xff);

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

		// Serial port (ComLynx) — see ~docs/plans/lynx-uart-comlynx-hardware-reference.md
		case 0x8c: { // SERCTL ($FD8C) — serial status register (read)
			// §3: Read returns status (different bit meanings from write §2):
			//   B7: TXRDY   — transmitter buffer empty (ready for data)
			//   B6: RXRDY   — receiver has data available
			//   B5: TXEMPTY — transmitter shift register idle
			//   B4: PARERR  — parity error (unimplemented — §9)
			//   B3: OVERRUN — receiver overrun (§7.3)
			//   B2: FRAMERR — framing error (never set — §9)
			//   B1: RXBRK   — break received (UART_BREAK_CODE bit 15)
			//   B0: PARBIT  — 9th bit / parity bit (data bit 8)
			uint8_t status = 0;
			// §3: TXRDY (B7) + TXEMPTY (B5) when not actively transmitting.
			// In our simplified model (no separate shift register), both are
			// always the same — set together as 0xA0 when TX is inactive.
			if (_state.UartTxCountdown & UartTxInactive) {
				status |= 0xa0;
			}
			// §3 B6: RXRDY — received byte available for reading
			if (_state.UartRxReady) {
				status |= 0x40;
			}
			// §3 B3: OVERRUN — new byte delivered before previous was read
			// NOTE: Handy/Mednafen have swapped comments on bits 3/2 (§3)
			if (_state.UartRxOverrunError) {
				status |= 0x08;
			}
			// §3 B2: FRAMERR — framing error (never generated in emulation)
			if (_state.UartRxFramingError) {
				status |= 0x04;
			}
			// §3 B1: RXBRK — break condition detected (bit 15 of RX data)
			if (_state.UartRxData & UartBreakCode) {
				status |= 0x02;
			}
			// §3 B0: PARBIT — 9th bit / parity bit (bit 8 of RX data)
			if (_state.UartRxData & 0x0100) {
				status |= 0x01;
			}
			return status;
		}
		case 0x8d: // SERDAT ($FD8D) — serial receive data register (§4)
			// §4: Reading clears RXRDY and returns the received byte (low 8 bits).
			// The 9th bit (parity/mark) is available via SERCTL read B0 (PARBIT).
			_state.UartRxReady = false;
			UpdateUartIrq(); // §9.1: RX IRQ condition may change
			return static_cast<uint8_t>(_state.UartRxData & 0xff);

		// I/O registers — EEPROM is wired through these
		case 0x88: // IODIR ($FD88) — I/O direction register
			return _ioDir;
		case 0x89: // IODAT ($FD89) — I/O data register
		{
			// Lynx I/O pin assignments (directly wired):
			//   Bit 0: EEPROM chip select (CS) — typically output
			//   Bit 1: EEPROM data (DI/DO) — input reads EEPROM DO pin
			//   Bit 2: EEPROM serial clock (CLK) — typically output
			//   Bit 3: AUDIN — audio comparator input (active low, active high on Lynx II)
			//   Bits 4-7: directly wired I/O pins (active high/low varies per cart)
			//
			// Output bits (ioDir=1) return the last written value.
			// Input bits (ioDir=0) read from external hardware.
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
			// Bit 3 (AUDIN): audio comparator — not connected in emulation,
			// reads as 0 when configured as input (no external audio source)
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

uint8_t LynxMikey::PeekRegister(uint8_t addr) const {
	// Side-effect-free register peek — no const_cast needed.
	// Duplicates ReadRegister logic but avoids mutating state.

	// Timer registers: $FD00-$FD1F
	if (addr < 0x20) {
		int timerIdx = GetTimerIndex(addr);
		int regOff = GetTimerRegOffset(addr);
		if (timerIdx >= 0 && timerIdx < 8) {
			const LynxTimerState& timer = _state.Timers[timerIdx];
			switch (regOff) {
				case 0: return timer.BackupValue;
				case 1: return timer.ControlA;
				case 2: return timer.Count;
				case 3: return timer.ControlB;
			}
		}
		return 0xff;
	}

	// Audio registers: $FD20-$FD50 — ReadRegister is side-effect-free
	if (addr >= 0x20 && addr <= 0x50) {
		return _apu ? _apu->ReadRegister(addr - 0x20) : 0xff;
	}

	switch (addr) {
		case 0x80: return _state.IrqPending;         // INTSET
		case 0x81: return 0xff;                        // INTRST (not readable)
		case 0x84: return _state.HardwareRevision;    // MIKEYHREV
		case 0x87: return 0;                           // SYSCTL1 (stub)
		case 0x88: return _ioDir;                      // IODIR
		case 0x89: {                                   // IODAT (side-effect-free)
			// See ReadRegister 0x89 for bit layout documentation
			uint8_t result = _ioData & _ioDir;
			if (_eeprom && !(_ioDir & 0x02)) {
				if (_eeprom->GetDataOut()) {
					result |= 0x02;
				} else {
					result &= ~0x02;
				}
			}
			return result;
		}
		case 0x8c: {                                   // SERCTL status
			uint8_t status = 0;
			if (_state.UartTxCountdown & UartTxInactive) status |= 0xa0;
			if (_state.UartRxReady) status |= 0x40;
			if (_state.UartRxOverrunError) status |= 0x08;
			if (_state.UartRxFramingError) status |= 0x04;
			if (_state.UartRxData & UartBreakCode) status |= 0x02;
			if (_state.UartRxData & 0x0100) status |= 0x01;
			return status;
		}
		case 0x8d:                                     // SERDAT — peek without clearing RXRDY
			return static_cast<uint8_t>(_state.UartRxData & 0xff);
		case 0x92: return _state.DisplayControl;       // DISPCTL
		case 0x94: return static_cast<uint8_t>(_state.DisplayAddress & 0xff);        // DISPADR low
		case 0x95: return static_cast<uint8_t>((_state.DisplayAddress >> 8) & 0xff); // DISPADR high

		// Palette GREEN registers
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			return _state.PaletteGreen[addr - 0xa0];

		// Palette BLUERED registers
		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			return _state.PaletteBR[addr - 0xb0];

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
					// Bit 6 is a self-clearing "reset timer" strobe — do not store it
					timer.ControlA = value & ~0x40;
					timer.Linked = (value & 0x07) == 7;
					// If bit 6 set, reset count to backup on next clock
					if (value & 0x40) {
						timer.Count = timer.BackupValue;
					}
					// Update IrqEnabled bitmask from CTLA bit 7 (interrupt enable)
					if (value & 0x80) {
						_state.IrqEnabled |= (1 << timerIdx);
					} else {
						_state.IrqEnabled &= ~(1 << timerIdx);
					}
					break;
				case 2: // COUNT
					timer.Count = value;
					break;
				case 3: // CTLB
					// Writing CTLB only clears the timer-done flag (bit 3).
					// Other CTLB bits (last-clock, borrow-in, borrow-out) are
					// read-only hardware status and must not be zeroed.
					timer.TimerDone = false;
					timer.ControlB &= ~0x08; // Clear timer-done bit
					break;
			}
		}
		return;
	}

	// Audio registers: $FD20-$FD50 → forwarded to APU
	if (addr >= 0x20 && addr <= 0x50) {
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

		// CPU sleep register
		case 0x91: // CPUSLEEP — write any value to halt CPU until next IRQ
			_cpu->GetState().StopState = LynxCpuStopState::WaitingForIrq;
			return;

		// Display registers
		case 0x92: // DISPCTL
			_state.DisplayControl = value;
			return;

		case 0x94: // DISPADR low
			_state.DisplayAddress = (_state.DisplayAddress & 0xff00) | value;
			return;

		case 0x95: // DISPADR high
			_state.DisplayAddress = (_state.DisplayAddress & 0x00ff) | (static_cast<uint16_t>(value) << 8);
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

		// Serial port (ComLynx) — see ~docs/plans/lynx-uart-comlynx-hardware-reference.md
		case 0x8c: // SERCTL ($FD8C) — serial control register (write)
			// §2: Write configures UART (different bit meanings from read §3):
			//   B7: TXINTEN  — transmit interrupt enable
			//   B6: RXINTEN  — receive interrupt enable
			//   B5: (reserved — write 0 for future compat)
			//   B4: PAREN    — parity enable
			//   B3: RESETERR — clear error flags (self-clearing)
			//   B2: TXOPEN   — open collector mode (not emulated, §9)
			//   B1: TXBRK    — send break signal (§6.2)
			//   B0: PAREVEN  — even parity (or 9th bit when parity disabled)
			// §9.1: Serial interrupt is level-sensitive. If TXINTEN is
			// set while TX is idle, IRQ fires continuously until cleared.
			_state.SerialControl = value;
			_state.UartTxIrqEnable = (value & 0x80) != 0;
			_state.UartRxIrqEnable = (value & 0x40) != 0;
			_state.UartParityEnable = (value & 0x10) != 0;
			_state.UartParityEven = (value & 0x01) != 0;

			// §2 B3: RESETERR — clear overrun and framing error flags
			if (value & 0x08) {
				_state.UartRxOverrunError = false;
				_state.UartRxFramingError = false;
			}

			// §2 B1: TXBRK — send break (§6.2: auto-repeats every 11 ticks)
			_state.UartSendBreak = (value & 0x02) != 0;
			if (_state.UartSendBreak) {
				_state.UartTxCountdown = UartTxTimePeriod;
				ComLynxTxLoopback(UartBreakCode); // §7.2: front-insert loopback
			}

			UpdateUartIrq();
			return;
		case 0x8d: // SERDAT ($FD8D) — serial transmit data register (§4)
		{
			// §4: Write starts transmission. Data loopbacks to RX (ComLynx bus §11).
			_state.UartTxData = value;

			// §4: Handle parity / 9th bit — when parity disabled, PAREVEN is the 9th bit
			// Note: Parity calculation is unimplemented (§9 — "Leave at zero!!")
			if (!_state.UartParityEnable && _state.UartParityEven) {
				_state.UartTxData |= 0x0100; // Set 9th bit (mark mode)
			}

			// §6.1: Start TX countdown (11 Timer 4 ticks = 11 bit-times)
			_state.UartTxCountdown = UartTxTimePeriod;

			// §6.1, §7.2: ComLynx self-loopback — TX output front-inserts to RX
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
	// See §7.3 — Timer 4 driven reception
	if (_state.UartRxCountdown == 0) [[unlikely]] {
		// RX period complete: pull byte from input queue
		if (_uartRxWaiting > 0) {
			// Overrun check: previous data not yet read (§3 — OVERRUN bit 3)
			if (_state.UartRxReady) {
				_state.UartRxOverrunError = true;
			}

			_state.UartRxData = _uartRxQueue[_uartRxOutputPtr];
			_uartRxOutputPtr = (_uartRxOutputPtr + 1) & (UartMaxRxQueue - 1);
			_uartRxWaiting--;
			_state.UartRxReady = true;

			// §7.3: If more data waiting, set inter-byte delay
			// Handy uses RX_TIME_PERIOD + RX_NEXT_DELAY = 11 + 44 = 55
			if (_uartRxWaiting > 0) {
				_state.UartRxCountdown = UartRxTimePeriod + UartRxNextDelay;
			} else {
				// §7.3: Queue empty after delivery — go inactive
				_state.UartRxCountdown = UartRxInactive;
			}
		}
	} else if (!(_state.UartRxCountdown & UartRxInactive)) [[unlikely]] {
		_state.UartRxCountdown--;
	}

	// --- Transmit --- (§6)
	if (_state.UartTxCountdown == 0) [[unlikely]] {
		// §6.1: TX period complete
		if (_state.UartSendBreak) {
			// §6.2: Break mode — continuously retransmit break signal
			_state.UartTxData = UartBreakCode;
			_state.UartTxCountdown = UartTxTimePeriod;
			ComLynxTxLoopback(_state.UartTxData); // §7.2: front-insert loopback
		} else {
			// §6.1: Normal completion — go idle (sentinel bit 31)
			_state.UartTxCountdown = UartTxInactive;
		}
	} else if (!(_state.UartTxCountdown & UartTxInactive)) [[unlikely]] {
		_state.UartTxCountdown--;
	}

	// §9.1, §10: Update serial IRQ (level-sensitive hardware bug)
	// Skip when neither IRQ is enabled (common case: no serial activity)
	if (_state.UartTxIrqEnable || _state.UartRxIrqEnable) [[unlikely]] {
		UpdateUartIrq();
	}
}

void LynxMikey::UpdateUartIrq() {
	// §9.1, §10: Serial IRQ uses Timer 4 IRQ line (bit 4 = $10).
	// Level-sensitive (hardware bug §9.1): asserts as long as condition persists.
	// Re-asserts on each check even if software cleared the pending bit.
	// This matches Handy's "Emulate the UART bug where UART IRQ is level sensitive"
	bool irq = false;

	// §9.1: TX IRQ — transmitter idle (countdown == 0 or inactive) and TX IRQ enabled
	bool txIdle = (_state.UartTxCountdown == 0) ||
	              ((_state.UartTxCountdown & UartTxInactive) != 0);
	if (txIdle && _state.UartTxIrqEnable) {
		irq = true;
	}

	// §9.1: RX IRQ — receive data ready and RX IRQ enabled
	if (_state.UartRxReady && _state.UartRxIrqEnable) {
		irq = true;
	}

	if (irq) {
		// §10: Serial interrupt is bit 4 ($10) in INTSET/INTRST
		_state.IrqPending |= static_cast<uint8_t>(LynxIrqSource::Timer4);
	}
	// Don't clear bit 4 here — let software clear via INTRST write ($FD81).
	// Level-sensitivity is achieved by re-asserting each tick (§9.1).

	UpdateIrqLine();
}

void LynxMikey::ComLynxTxLoopback(uint16_t data) {
	// ComLynx is a shared open-collector bus: transmitted data is received
	// by all connected units including the sender (mandatory self-loopback).
	// See §7.2 — Loopback inserts at FRONT of RX queue (not back) so the
	// sender always sees its own data before any externally-received bytes.
	// This is critical for collision detection on the ComLynx bus.
	if (_uartRxWaiting < static_cast<uint32_t>(UartMaxRxQueue)) {
		// If queue was empty, start the RX countdown (§7.1)
		if (_uartRxWaiting == 0) {
			_state.UartRxCountdown = UartRxTimePeriod;
		}

		// Front-insert: decrement output pointer and place data there (§7.2)
		_uartRxOutputPtr = (_uartRxOutputPtr - 1) & (UartMaxRxQueue - 1);
		_uartRxQueue[_uartRxOutputPtr] = data;
		_uartRxWaiting++;
	}
	// If queue is full, data is silently lost (same as ComLynxRxData).
}

void LynxMikey::ComLynxRxData(uint16_t data) {
	// §7.1: External data arrival — back-insert into RX queue.
	// Used by remote Lynx units to deliver ComLynx data (§11).
	if (_uartRxWaiting < static_cast<uint32_t>(UartMaxRxQueue)) {
		// §7.1: Trigger receive countdown only if queue was previously empty
		if (_uartRxWaiting == 0) {
			_state.UartRxCountdown = UartRxTimePeriod;
		}

		// §7.1: Append to back of queue
		_uartRxQueue[_uartRxInputPtr] = data;
		_uartRxInputPtr = (_uartRxInputPtr + 1) & (UartMaxRxQueue - 1);
		_uartRxWaiting++;
	}
	// If queue is full, data is silently lost (§7.1).
	// Overrun error is detected when the next byte is delivered from queue (§7.3).
}
