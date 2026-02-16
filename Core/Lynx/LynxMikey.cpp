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
	if (timer.TimerDone) {
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
	if (timer.TimerDone) {
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

		// Serial port (ComLynx stub — no cable connected)
		case 0x8c: // SERDAT — receive data register
			// No cable connected = no data received, return last written value
			return _state.SerialData;
		case 0x8d: { // SERCTL — serial control / status register (read)
			// ComLynx stub: Always report TX complete, TX buffer empty, no RX data.
			// Bit layout on read:
			//   7: TxRdy      — transmitter buffer empty (1 = ready for next byte)
			//   6: RxRdy      — receiver has data (0 = no data available)
			//   5: TxEmpty    — transmitter completely empty (1 = idle)
			//   4: Parerr     — parity error (0 = no error)
			//   3: Overrun    — receiver overrun (0 = no overrun)
			//   2: Framerr    — framing error (0 = no error)
			//   1: Rxbrk      — break received (0 = no break)
			//   0: Parbit     — 9th bit / parity (0)
			// HW Bug 13.2: UART interrupt is level-sensitive (not edge-triggered).
			// This means if interrupts are enabled and the condition persists,
			// the IRQ will fire continuously until the cause is cleared.
			// HW Bug 13.3: TXD starts high at power-up, which looks like a break
			// signal to a connected Lynx. Games must handle this.
			uint8_t status = 0xa0; // TxRdy (bit 7) + TxEmpty (bit 5) = $A0
			return status;
		}

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

		// Serial port (ComLynx stub)
		case 0x8c: // SERDAT — transmit data register
			// Accept data but don't actually transmit (no cable connected).
			// The data is silently discarded. TX complete status is always
			// reported on the next SERCTL read so the game doesn't hang
			// waiting for transmission to finish.
			_state.SerialData = value;
			return;
		case 0x8d: // SERCTL — serial control register (write)
			// Bit layout on write:
			//   7: TxIntEn    — transmit interrupt enable
			//   6: RxIntEn    — receive interrupt enable
			//   5: Paren      — parity enable
			//   4: ResetErr   — reset error flags (self-clearing)
			//   3: TxOpen     — open collector mode for TX
			//   2: TxBreak    — send break signal
			//   1: ParEven    — even parity (0 = odd)
			//   0: Unused
			// HW Bug 13.2: Interrupt is level-sensitive. If TxIntEn is set and
			// TX is empty (which it always is in our stub), the IRQ will fire
			// continuously. Games must clear the interrupt enable to stop it.
			_state.SerialControl = value;
			return;

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

	// I/O registers (EEPROM wiring)
	SV(_ioDir);
	SV(_ioData);

	// Frame buffer
	SVArray(_frameBuffer, LynxConstants::ScreenWidth * LynxConstants::ScreenHeight);
}
