#include "pch.h"
#include "Genesis/GenesisPsg.h"
#include "Genesis/GenesisConsole.h"
#include "Shared/Emulator.h"
#include "Utilities/Serializer.h"

GenesisPsg::GenesisPsg(Emulator* emu, GenesisConsole* console) {
	_emu = emu;
	_console = console;
	Reset();
}

void GenesisPsg::Reset() {
	_state = {};
	// All channels silent by default
	for (int i = 0; i < 4; i++) {
		_state.Channels[i].Volume = 0x0F;
	}
	_state.NoiseShiftRegister = 0x8000;
}

void GenesisPsg::Write(uint8_t value) {
	_state.WriteCount++;

	if (value & 0x80) {
		// Latch/Data byte: bit 7 set
		// Bits 6-4: register select (3 bits → 0-7)
		// Bits 3-0: data nibble
		_state.LatchedRegister = (value >> 4) & 0x07;
		uint8_t channel = _state.LatchedRegister >> 1;

		if (_state.LatchedRegister & 0x01) {
			// Volume register (odd registers: 1, 3, 5, 7)
			_state.Channels[channel].Volume = value & 0x0F;
		} else if (channel < 3) {
			// Tone register (even registers: 0, 2, 4)
			_state.Channels[channel].ToneCounter = (_state.Channels[channel].ToneCounter & 0x3F0) | (value & 0x0F);
		} else {
			// Noise register (register 6)
			_state.NoiseMode = value & 0x07;
			_state.NoiseShiftRegister = 0x8000;
		}
	} else {
		// Data byte: bit 7 clear — updates previously latched register
		// Bits 5-0: data bits
		uint8_t channel = _state.LatchedRegister >> 1;

		if (_state.LatchedRegister & 0x01) {
			// Volume register
			_state.Channels[channel].Volume = value & 0x0F;
		} else if (channel < 3) {
			// Tone register — upper 6 bits
			_state.Channels[channel].ToneCounter = (_state.Channels[channel].ToneCounter & 0x0F) | ((value & 0x3F) << 4);
		} else {
			// Noise register
			_state.NoiseMode = value & 0x07;
			_state.NoiseShiftRegister = 0x8000;
		}
	}
}

void GenesisPsg::Serialize(Serializer& s) {
	SV(_state.LatchedRegister);
	SV(_state.NoiseMode);
	SV(_state.NoiseShiftRegister);
	SV(_state.WriteCount);
	for (int i = 0; i < 4; i++) {
		SVI(_state.Channels[i].ToneCounter);
		SVI(_state.Channels[i].Volume);
	}
}
