#pragma once
#include "pch.h"
#include "Atari2600/Atari2600Types.h"

class Atari2600Riot {
private:
	Atari2600RiotState _state = {};
	uint8_t _ram[128] = {};

	void ConfigureTimer(uint8_t value, uint16_t divider) {
		_state.Timer = value;
		_state.TimerDivider = std::max<uint16_t>(1, divider);
		_state.TimerDividerCounter = _state.TimerDivider;
		_state.TimerUnderflow = false;
		_state.InterruptFlag = false;
	}

	[[nodiscard]] uint8_t ReadPortValue(uint8_t outputLatch, uint8_t inputLatch, uint8_t directionMask) const {
		uint8_t outputBits = (uint8_t)(outputLatch & directionMask);
		uint8_t inputBits = (uint8_t)(inputLatch & (uint8_t)~directionMask);
		return (uint8_t)(outputBits | inputBits);
	}

public:
	void Reset() {
		_state = {};
		_state.TimerDivider = 1;
		_state.TimerDividerCounter = 1;
		memset(_ram, 0, sizeof(_ram));
	}

	void StepCpuCycles(uint32_t cycles) {
		for (uint32_t i = 0; i < cycles; i++) {
			_state.CpuCycles++;

			if (_state.TimerDividerCounter > 0) {
				_state.TimerDividerCounter--;
			}

			if (_state.TimerDividerCounter == 0) {
				_state.TimerDividerCounter = _state.TimerDivider;

				if (_state.Timer == 0) {
					_state.Timer = 0x00FF;
					_state.TimerUnderflow = true;
					_state.InterruptFlag = true;
					_state.InterruptEdgeCount++;
				} else {
					_state.Timer--;
				}
			}
		}
	}

	uint8_t ReadRegister(uint16_t addr) {
		if (!(addr & 0x0200)) {
			return _ram[addr & 0x7F];
		}
		switch (addr & 0x07) {
			case 0x00: return ReadPortValue(_state.PortA, _state.PortAInput, _state.PortADirection);
			case 0x01: return ReadPortValue(_state.PortB, _state.PortBInput, _state.PortBDirection);
			case 0x02: return _state.PortADirection;
			case 0x03: return _state.PortBDirection;
			case 0x04: {
				uint8_t timerValue = (uint8_t)(_state.Timer & 0xFF);
				_state.TimerUnderflow = false;
				return timerValue;
			}
			case 0x05: return (uint8_t)((_state.Timer >> 8) & 0xFF);
			case 0x06: {
				uint8_t interruptValue = _state.InterruptFlag ? 1 : 0;
				_state.InterruptFlag = false;
				return interruptValue;
			}
			case 0x07: {
				uint8_t underflowValue = _state.TimerUnderflow ? 1 : 0;
				_state.TimerUnderflow = false;
				return underflowValue;
			}
			default: return 0;
		}
	}

	void WriteRegister(uint16_t addr, uint8_t value) {
		if (!(addr & 0x0200)) {
			_ram[addr & 0x7F] = value;
			return;
		}
		switch (addr & 0x07) {
			case 0x00:
				_state.PortA = value;
				break;
			case 0x01:
				_state.PortB = value;
				break;
			case 0x02:
				_state.PortADirection = value;
				break;
			case 0x03:
				_state.PortBDirection = value;
				break;
			case 0x04:
				ConfigureTimer(value, 1);
				break;
			case 0x05:
				ConfigureTimer(value, 8);
				break;
			case 0x06:
				ConfigureTimer(value, 64);
				break;
			case 0x07:
				ConfigureTimer(value, 1024);
				break;
		}
	}

	Atari2600RiotState GetState() const {
		return _state;
	}

	void SetState(const Atari2600RiotState& state) {
		_state = state;
		if (_state.TimerDivider == 0) {
			_state.TimerDivider = 1;
		}
		if (_state.TimerDividerCounter == 0) {
			_state.TimerDividerCounter = _state.TimerDivider;
		}
	}

	uint8_t* GetRamData() {
		return _ram;
	}

	static constexpr uint32_t RamSize = 128;
};
