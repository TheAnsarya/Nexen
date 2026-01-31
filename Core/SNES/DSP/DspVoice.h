#pragma once
#include "pch.h"
#include "SNES/DSP/DspTypes.h"
#include "Utilities/ISerializable.h"

class Spc;
class Dsp;
struct SnesConfig;

/// <summary>
/// Represents a single voice channel in the SNES S-DSP.
/// </summary>
/// <remarks>
/// The S-DSP has 8 independent voice channels, each capable of:
/// - BRR (Bit Rate Reduction) compressed sample playback
/// - ADSR envelope generation with attack/decay/sustain/release
/// - Gain envelope mode (linear/exponential increase/decrease)
/// - Pitch with optional modulation from previous voice
/// - Stereo volume control
/// - Echo bus output
/// 
/// Each voice processes samples in a 32-step cycle synchronized
/// with the other voices and the global DSP state.
/// </remarks>
class DspVoice final : public ISerializable {
private:
	/// <summary>Pointer to parent SPC700 for memory access.</summary>
	Spc* _spc = nullptr;

	/// <summary>Pointer to parent DSP for shared state.</summary>
	Dsp* _dsp = nullptr;

	/// <summary>Pointer to this voice's DSP registers (10 bytes).</summary>
	uint8_t* _regs = nullptr;

	/// <summary>Pointer to SNES configuration for settings.</summary>
	SnesConfig* _cfg = nullptr;

	/// <summary>Pointer to shared DSP state.</summary>
	DspState* _shared = nullptr;

	/// <summary>Current envelope volume (11-bit, 0-2047).</summary>
	int32_t _envVolume = 0;

	/// <summary>Previous calculated envelope for ENVX output.</summary>
	int32_t _prevCalculatedEnv = 0;

	/// <summary>Sample interpolation position (12-bit fraction).</summary>
	int32_t _interpolationPos = 0;

	/// <summary>Current envelope state machine mode.</summary>
	EnvelopeMode _envMode = EnvelopeMode::Release;

	/// <summary>Current BRR block address in APU RAM.</summary>
	uint16_t _brrAddress = 0;

	/// <summary>Offset within current BRR block (0-8).</summary>
	uint16_t _brrOffset = 1;

	/// <summary>Voice index (0-7).</summary>
	uint8_t _voiceIndex = 0;

	/// <summary>Bit mask for this voice (1 << _voiceIndex).</summary>
	uint8_t _voiceBit = 0;

	/// <summary>Key-on delay counter (5 cycles for initialization).</summary>
	uint8_t _keyOnDelay = 0;

	/// <summary>Envelope output for ENVX register (7-bit).</summary>
	uint8_t _envOut = 0;

	/// <summary>Current position in sample ring buffer.</summary>
	uint8_t _bufferPos = 0;

	/// <summary>
	/// Decoded sample ring buffer (12 samples for Gaussian interpolation).
	/// The last 4 samples from previous BRR block plus 8 from current block.
	/// </summary>
	int16_t _sampleBuffer[12] = {};

	/// <summary>Reads a voice register value.</summary>
	/// <param name="reg">Register to read.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadReg(DspVoiceRegs reg) { return _regs[(int)reg]; }

	/// <summary>Writes a value to a voice register.</summary>
	/// <param name="reg">Register to write.</param>
	/// <param name="value">Value to write.</param>
	void WriteReg(DspVoiceRegs reg, uint8_t value) { _regs[(int)reg] = value; }

	/// <summary>Decodes one BRR sample block (9 bytes → 16 samples).</summary>
	void DecodeBrrSample();

	/// <summary>Processes ADSR/Gain envelope for this voice.</summary>
	void ProcessEnvelope();

	/// <summary>
	/// Updates the voice output sample.
	/// </summary>
	/// <param name="right">True for right channel, false for left.</param>
	void UpdateOutput(bool right);

public:
	/// <summary>
	/// Initializes the voice with its index and parent pointers.
	/// </summary>
	/// <param name="voiceIndex">Voice index (0-7).</param>
	/// <param name="spc">Parent SPC700 instance.</param>
	/// <param name="dsp">Parent DSP instance.</param>
	/// <param name="dspVoiceRegs">Pointer to voice register block.</param>
	/// <param name="cfg">SNES configuration.</param>
	void Init(uint8_t voiceIndex, Spc* spc, Dsp* dsp, uint8_t* dspVoiceRegs, SnesConfig* cfg);

	/// <summary>Step 1: Read BRR header, check key-on.</summary>
	void Step1();

	/// <summary>Step 2: Read BRR data byte 1.</summary>
	void Step2();

	/// <summary>Step 3: Decode BRR, update interpolation.</summary>
	void Step3();

	/// <summary>Step 3a: Sub-step for sample processing.</summary>
	void Step3a();

	/// <summary>Step 3b: Sub-step for envelope processing.</summary>
	void Step3b();

	/// <summary>Step 3c: Sub-step for output calculation.</summary>
	void Step3c();

	/// <summary>Step 4: Process left channel output.</summary>
	void Step4();

	/// <summary>Step 5: Process right channel output.</summary>
	void Step5();

	/// <summary>Step 6: Read next BRR block address.</summary>
	void Step6();

	/// <summary>Step 7: Handle sample end/loop.</summary>
	void Step7();

	/// <summary>Step 8: Process echo feedback.</summary>
	void Step8();

	/// <summary>Step 9: Finalize voice state for next cycle.</summary>
	void Step9();

	/// <summary>Serializes voice state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
