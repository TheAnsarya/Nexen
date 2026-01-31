#include "pch.h"
#include "Gameboy/GbTimer.h"
#include "Gameboy/GbTypes.h"
#include "Gameboy/GbMemoryManager.h"
#include "Gameboy/APU/GbApu.h"

// Initialize Game Boy hardware timer (DIV, TIMA, TMA, TAC registers)
void GbTimer::Init(GbMemoryManager* memoryManager, GbApu* apu) {
	_apu = apu;  // Timer drives APU frame sequencer
	_memoryManager = memoryManager;

	_state = {};
	_state.TimerDivider = 1024;  // Default timer divider (1024 = slowest)

	// Passes boot_div-dmgABCmgb test
	// But that test depends on LCD power on timings, so may be wrong.
	_state.Divider = 0x06;  // Initial DIV value after power-on
}

GbTimer::~GbTimer() {
}

GbTimerState GbTimer::GetState() {
	return _state;
}

void GbTimer::Exec() {
	if ((_state.Divider & 0x03) == 2) {
		_state.Reloaded = false;
		if (_state.NeedReload) {
			ReloadCounter();
		}
	}
	SetDivider(_state.Divider + 2);
}

void GbTimer::ReloadCounter() {
	_state.Counter = _state.Modulo;
	_memoryManager->RequestIrq(GbIrqSource::Timer);
	_state.NeedReload = false;
	_state.Reloaded = true;
}

// Update the 16-bit internal divider and check for timer/frame sequencer triggers
void GbTimer::SetDivider(uint16_t newValue) {
	// TIMA increments on falling edge of selected divider bit
	if (_state.TimerEnabled && !(newValue & _state.TimerDivider) && (_state.Divider & _state.TimerDivider)) {
		_state.Counter++;
		if (_state.Counter == 0) {
			_state.NeedReload = true;  // TIMA overflow, will reload from TMA
		}
	}

	// Frame sequencer bit: 0x2000 in CGB double-speed, 0x1000 in normal speed
	uint16_t frameSeqBit = _memoryManager->IsHighSpeed() ? 0x2000 : 0x1000;
	if (!(newValue & frameSeqBit) && (_state.Divider & frameSeqBit)) {
		_apu->ClockFrameSequencer();  // Clock APU frame sequencer on falling edge
	}

	_state.Divider = newValue;
}

bool GbTimer::IsFrameSequencerBitSet() {
	uint16_t frameSeqBit = _memoryManager->IsHighSpeed() ? 0x2000 : 0x1000;
	return _state.Divider & frameSeqBit;
}

uint8_t GbTimer::Read(uint16_t addr) {
	switch (addr) {
		case 0xFF04:
			return _state.Divider >> 8;
		case 0xFF05:
			return _state.Counter; // FF05 - TIMA - Timer counter (R/W)
		case 0xFF06:
			return _state.Modulo; // FF06 - TMA - Timer Modulo (R/W)
		case 0xFF07:
			return _state.Control | 0xF8; // FF07 - TAC - Timer Control (R/W)
	}
	return 0;
}

void GbTimer::Write(uint16_t addr, uint8_t value) {
	switch (addr) {
		case 0xFF04:
			SetDivider(0);
			break;

		case 0xFF05:
			// FF05 - TIMA - Timer counter (R/W)
			if (_state.NeedReload) {
				// Writing to TIMA when a reload is pending will cancel the reload and IRQ request
				_state.NeedReload = false;
			}

			if (!_state.Reloaded) {
				// Writes to TIMA on the cycle TIMA was reloaded with TMA are ignored
				_state.Counter = value;
			}
			break;

		case 0xFF06:
			// FF06 - TMA - Timer Modulo (R/W)
			_state.Modulo = value;
			if (_state.Reloaded) {
				// Writing to TMA on the same cycle it was reloaded into TIMA will also update TIMA
				_state.Counter = value;
			}
			break;

		case 0xFF07: {
			// FF07 - TAC - Timer Control (R/W)
			_state.Control = value;
			bool enabled = (value & 0x04) != 0;
			uint16_t newDivider = 0;
			switch (value & 0x03) {
				case 0:
					newDivider = 1 << 9;
					break;
				case 1:
					newDivider = 1 << 3;
					break;
				case 2:
					newDivider = 1 << 5;
					break;
				case 3:
					newDivider = 1 << 7;
					break;
			}

			if (_state.TimerEnabled) {
				// When changing the value of TAC, the TIMA register can get incremented due to a glitch
				bool incrementCounter;
				if (enabled) {
					incrementCounter = (_state.Divider & _state.TimerDivider) != 0 && (_state.Divider & newDivider) == 0;
				} else {
					incrementCounter = (_state.Divider & _state.TimerDivider) != 0;
				}

				if (incrementCounter) {
					_state.Counter++;
					if (_state.Counter == 0) {
						ReloadCounter();
					}
				}
			}

			_state.TimerEnabled = enabled;
			_state.TimerDivider = newDivider;
			break;
		}
	}
}

void GbTimer::Serialize(Serializer& s) {
	SV(_state.Divider);
	SV(_state.Counter);
	SV(_state.Modulo);
	SV(_state.Control);
	SV(_state.TimerEnabled);
	SV(_state.TimerDivider);
	SV(_state.NeedReload);
	SV(_state.Reloaded);
}
