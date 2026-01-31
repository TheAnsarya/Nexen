#pragma once
#include "pch.h"
#include "NES/INesMemoryHandler.h"
#include "NES/NesConsole.h"
#include "NES/NesCpu.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>Frame counter clock event types.</summary>
enum class FrameType {
	None = 0,         ///< No clock event
	QuarterFrame = 1, ///< Quarter-frame: envelope and linear counter
	HalfFrame = 2,    ///< Half-frame: length counter and sweep
};

/// <summary>
/// APU frame counter - generates timing signals for APU components.
/// </summary>
/// <remarks>
/// **Purpose:**
/// The frame counter provides timing signals to clock the envelope,
/// length counter, linear counter, and sweep units. It runs at a
/// fraction of the CPU clock to create musical timing.
///
/// **Modes:**
/// - 4-step mode ($4017 bit 7 = 0): Steps 0-3 + IRQ
///   - ~240 Hz envelope/linear, ~120 Hz length/sweep
///   - Generates IRQ at step 3 if not inhibited
/// - 5-step mode ($4017 bit 7 = 1): Steps 0-4, no IRQ
///   - Same timing but extra step, IRQ disabled
///
/// **4-Step Timing (NTSC):**
/// ```
/// Step    CPU Cycle   Action
/// 0       7457        Quarter-frame (envelope, linear)
/// 1       14913       Half-frame (length, sweep)
/// 2       22371       Quarter-frame
/// 3       29828       (nothing, prep for IRQ)
/// 4       29829       Half-frame + IRQ flag set
/// 5       29830       IRQ flag set again (loop)
/// ```
///
/// **5-Step Timing (NTSC):**
/// ```
/// Step    CPU Cycle   Action
/// 0       7457        Quarter-frame
/// 1       14913       Half-frame
/// 2       22371       Quarter-frame
/// 3       29829       (nothing)
/// 4       37281       Half-frame
/// 5       37282       (loop)
/// ```
///
/// **Write Delay:**
/// Writing to $4017 has a 3-4 cycle delay before taking effect,
/// depending on whether the write occurs on an odd or even cycle.
///
/// **IRQ Flag:**
/// IRQ flag is set on steps 3-5 of 4-step mode.
/// Reading $4015 clears the flag.
/// Flag can be inhibited by setting bit 6 of $4017.
/// </remarks>
class ApuFrameCounter : public INesMemoryHandler, public ISerializable {
private:
	/// <summary>Step cycle timings for NTSC (4-step and 5-step modes).</summary>
	const int32_t _stepCyclesNtsc[2][6] = {
		{7457, 14913, 22371, 29828, 29829, 29830},
		{7457, 14913, 22371, 29829, 37281, 37282}
	};

	/// <summary>Step cycle timings for PAL.</summary>
	const int32_t _stepCyclesPal[2][6] = {
		{8313, 16627, 24939, 33252, 33253, 33254},
		{8313, 16627, 24939, 33253, 41565, 41566}
	};

	/// <summary>Frame type for each step (quarter/half/none).</summary>
	const FrameType _frameType[2][6] = {
		{FrameType::QuarterFrame, FrameType::HalfFrame, FrameType::QuarterFrame, FrameType::None, FrameType::HalfFrame, FrameType::None},
		{FrameType::QuarterFrame, FrameType::HalfFrame, FrameType::QuarterFrame, FrameType::None, FrameType::HalfFrame, FrameType::None}
	};

	NesConsole* _console = nullptr;
	int32_t _stepCycles[2][6] = {};    ///< Active step cycle table (region-specific)
	int32_t _previousCycle = 0;        ///< CPU cycle at last update
	uint32_t _currentStep = 0;         ///< Current sequencer step (0-5)
	uint32_t _stepMode = 0;            ///< 0 = 4-step mode, 1 = 5-step mode
	bool _inhibitIRQ = false;          ///< IRQ inhibit flag ($4017 bit 6)
	uint8_t _blockFrameCounterTick = 0; ///< Prevents double-clocking on write
	int16_t _newValue = 0;             ///< Pending $4017 write value
	int8_t _writeDelayCounter = 0;     ///< Cycles until write takes effect

	bool _irqFlag = false;             ///< Frame counter IRQ flag
	uint64_t _irqFlagClearClock = 0;   ///< Clock when IRQ flag should clear

public:
	/// <summary>Constructs frame counter for console.</summary>
	/// <param name="console">Parent NES console.</param>
	ApuFrameCounter(NesConsole* console) {
		_console = console;
		Reset(false);
	}

	/// <summary>Resets frame counter to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	/// <remarks>
	/// On soft reset, step mode is preserved.
	/// "After reset or power-up, APU acts as if $4017 were written with $00
	/// from 9 to 12 clocks before first instruction begins."
	/// </remarks>
	void Reset(bool softReset) {
		_previousCycle = 0;
		_irqFlag = false;
		_irqFlagClearClock = 0;

		// "After reset: APU mode in $4017 was unchanged"
		if (!softReset) {
			_stepMode = 0;
		}

		_currentStep = 0;

		// Reset acts as if $00 was written to $4017
		_newValue = _stepMode ? 0x80 : 0x00;
		_writeDelayCounter = 3;
		_inhibitIRQ = false;

		_blockFrameCounterTick = 0;
	}

	/// <summary>Serializes frame counter state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_previousCycle);
		SV(_currentStep);
		SV(_stepMode);
		SV(_inhibitIRQ);
		SV(_blockFrameCounterTick);
		SV(_writeDelayCounter);
		SV(_newValue);
		SV(_irqFlag);
		SV(_irqFlagClearClock);

		if (!s.IsSaving()) {
			SetRegion(_console->GetRegion());
		}
	}

	/// <summary>Sets region-specific timing table.</summary>
	/// <param name="region">Console region (NTSC/PAL/Dendy).</param>
	void SetRegion(ConsoleRegion region) {
		switch (region) {
			case ConsoleRegion::Auto:
				// Auto should never be set here
				break;

			case ConsoleRegion::Ntsc:
			case ConsoleRegion::Dendy:
				memcpy(_stepCycles, _stepCyclesNtsc, sizeof(_stepCycles));
				break;

			case ConsoleRegion::Pal:
				memcpy(_stepCycles, _stepCyclesPal, sizeof(_stepCycles));
				break;
		}
	}

	uint32_t Run(int32_t& cyclesToRun) {
		uint32_t cyclesRan;

		if (_previousCycle + cyclesToRun >= _stepCycles[_stepMode][_currentStep]) {
			if (_stepMode == 0 && _currentStep >= 3) {
				// Set irq on the last 3 cycles for 4-step mode
				_irqFlag = true;
				_irqFlagClearClock = 0;

				if (!_inhibitIRQ) {
					_console->GetCpu()->SetIrqSource(IRQSource::FrameCounter);
				} else if (_currentStep == 5) {
					_irqFlag = false;
					_irqFlagClearClock = 0;
				}
			}

			FrameType type = _frameType[_stepMode][_currentStep];
			if (type != FrameType::None && !_blockFrameCounterTick) {
				_console->GetApu()->FrameCounterTick(type);

				// Do not allow writes to 4017 to clock the frame counter for the next cycle (i.e this odd cycle + the following even cycle)
				_blockFrameCounterTick = 2;
			}

			if (_stepCycles[_stepMode][_currentStep] < _previousCycle) {
				// This can happen when switching from PAL to NTSC, which can cause a freeze (endless loop in APU)
				cyclesRan = 0;
			} else {
				cyclesRan = _stepCycles[_stepMode][_currentStep] - _previousCycle;
			}

			cyclesToRun -= cyclesRan;

			_currentStep++;
			if (_currentStep == 6) {
				_currentStep = 0;
				_previousCycle = 0;
			} else {
				_previousCycle += cyclesRan;
			}
		} else {
			cyclesRan = cyclesToRun;
			cyclesToRun = 0;
			_previousCycle += cyclesRan;
		}

		if (_newValue >= 0) {
			_writeDelayCounter--;
			if (_writeDelayCounter == 0) {
				// Apply new value after the appropriate number of cycles has elapsed
				_stepMode = ((_newValue & 0x80) == 0x80) ? 1 : 0;

				_writeDelayCounter = -1;
				_currentStep = 0;
				_previousCycle = 0;
				_newValue = -1;

				if (_stepMode && !_blockFrameCounterTick) {
					//"Writing to $4017 with bit 7 set will immediately generate a clock for both the quarter frame and the half frame units, regardless of what the sequencer is doing."
					_console->GetApu()->FrameCounterTick(FrameType::HalfFrame);
					_blockFrameCounterTick = 2;
				}
			}
		}

		if (_blockFrameCounterTick > 0) {
			_blockFrameCounterTick--;
		}

		return cyclesRan;
	}

	bool NeedToRun(uint32_t cyclesToRun) {
		// Run APU when:
		//  -A new value is pending
		//  -The "blockFrameCounterTick" process is running
		//  -We're at the before-last or last tick of the current step
		return _newValue >= 0 || _blockFrameCounterTick > 0 || (_previousCycle + (int32_t)cyclesToRun >= _stepCycles[_stepMode][_currentStep] - 1);
	}

	void GetMemoryRanges(MemoryRanges& ranges) override {
		ranges.AddHandler(MemoryOperation::Write, 0x4017);
	}

	uint8_t ReadRam(uint16_t addr) override {
		return 0;
	}

	void WriteRam(uint16_t addr, uint8_t value) override {
		_console->GetApu()->Run();
		_newValue = value;

		// Reset sequence after $4017 is written to
		if (_console->GetCpu()->GetCycleCount() & 0x01) {
			//"If the write occurs between APU cycles, the effects occur 4 CPU cycles after the write cycle. "
			_writeDelayCounter = 4;
		} else {
			//"If the write occurs during an APU cycle, the effects occur 3 CPU cycles after the $4017 write cycle"
			_writeDelayCounter = 3;
		}

		_inhibitIRQ = (value & 0x40) == 0x40;
		if (_inhibitIRQ) {
			_console->GetCpu()->ClearIrqSource(IRQSource::FrameCounter);
			_irqFlag = false;
			_irqFlagClearClock = 0;
		}
	}

	bool GetIrqFlag() {
		if (_irqFlag) {
			uint64_t clock = _console->GetMasterClock();
			if (_irqFlagClearClock == 0) {
				// The flag will be cleared at the start of the next APU cycle (see AccuracyCoin test)
				_irqFlagClearClock = clock + ((clock & 0x01) ? 2 : 1);
			} else if (clock >= _irqFlagClearClock) {
				_irqFlagClearClock = 0;
				_irqFlag = false;
			}
		}
		return _irqFlag;
	}

	ApuFrameCounterState GetState() {
		ApuFrameCounterState state;
		state.IrqEnabled = !_inhibitIRQ;
		state.SequencePosition = std::min<uint8_t>(_currentStep, _stepMode ? 5 : 4);
		state.FiveStepMode = _stepMode == 1;
		return state;
	}
};