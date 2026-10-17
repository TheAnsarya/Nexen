#pragma once
#include "pch.h"
#include "SNES/DSP/DspVoice.h"
#include "SNES/DSP/DspTypes.h"
#include "Utilities/ISerializable.h"

class Emulator;
class SnesConsole;
class Spc;

/// <summary>
/// SNES DSP (Digital Signal Processor) - 8-channel BRR sample playback and effects.
/// Generates all audio output from the SPC700 audio subsystem.
/// </summary>
/// <remarks>
/// **Architecture:**
/// The DSP is a dedicated audio processor that works in tandem with the SPC700 CPU.
/// It provides 8 independent voices for sample playback with hardware mixing and effects.
///
/// **BRR (Bit Rate Reduction) Format:**
/// - 4-bit ADPCM compression (9 bytes = 16 samples)
/// - Looping support with loop point markers
/// - Filter modes for improved quality
///
/// **Per-Voice Features:**
/// - Pitch: 14-bit frequency (0-128 kHz)
/// - ADSR/Gain envelope (attack/decay/sustain/release)
/// - Volume: Left/right panning
/// - Pitch modulation from previous voice
/// - Noise mode (white noise generator)
///
/// **Global Effects:**
/// - Echo: Configurable delay buffer with FIR filter
/// - Echo feedback with 8-tap FIR coefficients
/// - Master volume control
///
/// **Timing:**
/// - Runs at 32 kHz sample rate
/// - 32 cycles per sample (1 cycle per voice + mixing)
/// - Echo buffer in SPC RAM (max 240ms delay)
///
/// **Register Map ($00-$7F via $F2/$F3):**
/// - $x0-$x9: Voice registers (per-voice)
/// - $0C/$1C: Master volume L/R
/// - $2C/$3C: Echo volume L/R
/// - $4C: Key on
/// - $5C: Key off
/// - $6C: Flags (reset, mute, echo, noise clock)
/// - $7D: Source directory page
/// </remarks>
class Dsp final : public ISerializable {
private:
	/// <summary>Complete DSP state including all registers and echo buffer.</summary>
	DspState _state = {};

	/// <summary>The 8 voice channels.</summary>
	DspVoice _voices[8] = {};

	/// <summary>Parent emulator for callbacks.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Parent SPC700 for APU RAM access.</summary>
	Spc* _spc = nullptr;

	/// <summary>Number of output samples in buffer.</summary>
	uint16_t _outSampleCount = 0;

	/// <summary>Output sample buffer (stereo interleaved).</summary>
	int16_t _dspOutput[0x2000] = {};

	/// <summary>Updates the global sample counter for rate timing.</summary>
	void UpdateCounter();

	/// <summary>
	/// Calculates FIR filter output for echo processing.
	/// </summary>
	/// <param name="index">FIR tap index (0-7).</param>
	/// <param name="ch">Channel (0=left, 1=right).</param>
	/// <returns>Filtered sample value.</returns>
	int32_t CalculateFir(int index, int ch);

	/// <summary>Echo processing step 22.</summary>
	void EchoStep22();
	/// <summary>Echo processing step 23.</summary>
	void EchoStep23();
	/// <summary>Echo processing step 24.</summary>
	void EchoStep24();
	/// <summary>Echo processing step 25.</summary>
	void EchoStep25();
	/// <summary>Echo processing step 26.</summary>
	void EchoStep26();
	/// <summary>Echo processing step 27.</summary>
	void EchoStep27();
	/// <summary>Echo processing step 28.</summary>
	void EchoStep28();
	/// <summary>Echo processing step 29.</summary>
	void EchoStep29();
	/// <summary>Echo processing step 30.</summary>
	void EchoStep30();

public:
	/// <summary>
	/// Constructs a new DSP instance.
	/// </summary>
	/// <param name="emu">Parent emulator.</param>
	/// <param name="console">Parent SNES console.</param>
	/// <param name="spc">Parent SPC700 CPU.</param>
	Dsp(Emulator* emu, SnesConsole* console, Spc* spc);

	/// <summary>
	/// Loads DSP register state from an SPC file.
	/// </summary>
	/// <param name="regs">128-byte register array from SPC file.</param>
	void LoadSpcFileRegs(uint8_t* regs);

	/// <summary>Resets the DSP to power-on state.</summary>
	void Reset();

	/// <summary>Gets a reference to the DSP state for debugging.</summary>
	/// <returns>Reference to internal state.</returns>
	DspState& GetState() { return _state; }

	/// <summary>Checks if DSP output is muted.</summary>
	/// <returns>True if muted (always false currently).</returns>
	[[nodiscard]] bool IsMuted() { return false; }

	/// <summary>Gets the number of samples in the output buffer.</summary>
	/// <returns>Sample count (stereo pairs).</returns>
	[[nodiscard]] uint16_t GetSampleCount() { return _outSampleCount; }

	/// <summary>Gets the output sample buffer.</summary>
	/// <returns>Pointer to stereo interleaved samples.</returns>
	int16_t* GetSamples() { return _dspOutput; }

	/// <summary>Clears the output sample buffer.</summary>
	void ResetOutput() { _outSampleCount = 0; }

	/// <summary>
	/// Checks if the specified rate counter has ticked.
	/// </summary>
	/// <param name="rate">Rate to check against.</param>
	/// <returns>True if counter matches rate.</returns>
	bool CheckCounter(int32_t rate);

	/// <summary>
	/// Reads a DSP register value.
	/// </summary>
	/// <param name="reg">Register address (0-127).</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint8_t reg) { return _state.ExternalRegs[reg]; }

	/// <summary>
	/// Writes to a DSP register.
	/// </summary>
	/// <param name="reg">Register address (0-127).</param>
	/// <param name="value">Value to write.</param>
	void Write(uint8_t reg, uint8_t value);

	/// <summary>
	/// Reads a global DSP register.
	/// </summary>
	/// <param name="reg">Global register identifier.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadReg(DspGlobalRegs reg) { return _state.Regs[(int)reg]; }

	/// <summary>
	/// Writes to a global DSP register (updates both external and internal state).
	/// </summary>
	/// <param name="reg">Global register identifier.</param>
	/// <param name="value">Value to write.</param>
	void WriteGlobalReg(DspGlobalRegs reg, uint8_t value) {
		_state.ExternalRegs[(int)reg] = value;
		_state.Regs[(int)reg] = value;
	}

	/// <summary>
	/// Writes to a voice register.
	/// </summary>
	/// <param name="voiceIndex">Voice index (0-7).</param>
	/// <param name="reg">Voice register identifier.</param>
	/// <param name="value">Value to write.</param>
	void WriteVoiceReg(uint8_t voiceIndex, DspVoiceRegs reg, uint8_t value) {
		_state.ExternalRegs[voiceIndex * 0x10 + (int)reg] = value;
		_state.Regs[voiceIndex * 0x10 + (int)reg] = value;
	}

	/// <summary>
	/// Clamps a 32-bit value to 16-bit signed range.
	/// </summary>
	/// <param name="val">Value to clamp.</param>
	/// <returns>Clamped value in [-32768, 32767].</returns>
	[[nodiscard]] static constexpr int16_t Clamp16(int32_t val) {
		if (val < INT16_MIN) {
			return INT16_MIN;
		} else if (val > INT16_MAX) {
			return INT16_MAX;
		}
		return val;
	}

	/// <summary>Executes one DSP sample cycle (32 steps).</summary>
	void Exec();

	/// <summary>Serializes DSP state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;
};
