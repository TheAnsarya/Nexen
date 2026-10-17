#pragma once
#include "pch.h"

/// <summary>
/// Complete state of the SNES S-DSP (Digital Signal Processor).
/// </summary>
/// <remarks>
/// The S-DSP is responsible for all SNES audio synthesis:
/// - 8 voices with BRR (Bit Rate Reduction) sample playback
/// - ADSR and Gain envelope generators
/// - Pitch modulation between voices
/// - Echo effect with FIR filter
/// - Noise generator
/// 
/// The DSP runs at ~32KHz sample rate and has 128 bytes of registers.
/// </remarks>
struct DspState {
	/// <summary>
	/// External register values as written by SPC700.
	/// Some registers have different read/write behavior.
	/// </summary>
	uint8_t ExternalRegs[128];

	/// <summary>Internal register values (actual operational state).</summary>
	uint8_t Regs[128];

	/// <summary>
	/// 15-bit LFSR for noise generation.
	/// Initial value is 0x4000 (bit 14 set).
	/// </summary>
	int32_t NoiseLfsr = 0x4000;

	/// <summary>Global sample counter for DSP timing.</summary>
	uint16_t Counter = 0;

	/// <summary>Current step within the DSP's 32-step cycle.</summary>
	uint8_t Step = 0;

	/// <summary>Buffered OUTX register value for reading.</summary>
	uint8_t OutRegBuffer = 0;

	/// <summary>Buffered ENVX register value for reading.</summary>
	uint8_t EnvRegBuffer = 0;

	/// <summary>Buffered ENDX (voice end) flags for reading.</summary>
	uint8_t VoiceEndBuffer = 0;

	/// <summary>Current voice output sample (after envelope).</summary>
	int32_t VoiceOutput = 0;

	/// <summary>Final mixed output samples [left, right].</summary>
	int32_t OutSamples[2] = {};

	// === Latched values (captured during specific DSP steps) ===

	/// <summary>Current voice pitch value (14-bit).</summary>
	int32_t Pitch = 0;

	/// <summary>Current BRR sample address in APU RAM.</summary>
	uint16_t SampleAddress = 0;

	/// <summary>Next BRR block address (after loop/end).</summary>
	uint16_t BrrNextAddress = 0;

	/// <summary>Sample directory table base address (DIR register).</summary>
	uint8_t DirSampleTableAddress = 0;

	/// <summary>Noise enable flags (NON register, 1 bit per voice).</summary>
	uint8_t NoiseOn = 0;

	/// <summary>Pitch modulation enable flags (PMON register).</summary>
	uint8_t PitchModulationOn = 0;

	/// <summary>Key-on flags being processed (KON register).</summary>
	uint8_t KeyOn = 0;

	/// <summary>New key-on requests (latched for next cycle).</summary>
	uint8_t NewKeyOn = 0;

	/// <summary>Key-off flags (KOFF register).</summary>
	uint8_t KeyOff = 0;

	/// <summary>Toggle for every-other-sample processing.</summary>
	uint8_t EveryOtherSample = 1;

	/// <summary>Current voice source number (SRCN register).</summary>
	uint8_t SourceNumber = 0;

	/// <summary>Current BRR block header byte.</summary>
	uint8_t BrrHeader = 0;

	/// <summary>Current BRR compressed data byte.</summary>
	uint8_t BrrData = 0;

	/// <summary>Loop flags (1 bit per voice, set when sample loops).</summary>
	uint8_t Looped = 0;

	/// <summary>ADSR1 register value for current voice.</summary>
	uint8_t Adsr1 = 0;

	// === Echo effect state ===

	/// <summary>Echo input samples [left, right].</summary>
	int32_t EchoIn[2] = {};

	/// <summary>Echo output samples after FIR filter [left, right].</summary>
	int32_t EchoOut[2] = {};

	/// <summary>Echo FIR filter history buffer [8 taps][left, right].</summary>
	int16_t EchoHistory[8][2] = {};

	/// <summary>Current position in echo ring buffer (APU RAM address).</summary>
	uint16_t EchoPointer = 0;

	/// <summary>Echo buffer length in bytes.</summary>
	uint16_t EchoLength = 0;

	/// <summary>Current offset within echo buffer.</summary>
	uint16_t EchoOffset = 0;

	/// <summary>Current position in FIR history buffer (0-7).</summary>
	uint8_t EchoHistoryPos = 0;

	/// <summary>Echo buffer start address (ESA register).</summary>
	uint8_t EchoRingBufferAddress = 0;

	/// <summary>Echo enable flags (EON register, 1 bit per voice).</summary>
	uint8_t EchoOn = 0;

	/// <summary>True when echo writes to RAM are enabled.</summary>
	bool EchoEnabled = false;
};

/// <summary>
/// S-DSP global register addresses.
/// </summary>
/// <remarks>
/// Global registers control master volume, echo parameters,
/// key on/off, and voice-wide settings. Addresses are in the
/// format 0xXY where X is voice-like position and Y is the register.
/// </remarks>
enum class DspGlobalRegs {
	/// <summary>Master volume left channel ($0C).</summary>
	MasterVolLeft = 0x0C,

	/// <summary>Master volume right channel ($1C).</summary>
	MasterVolRight = 0x1C,

	/// <summary>Echo volume left channel ($2C).</summary>
	EchoVolLeft = 0x2C,

	/// <summary>Echo volume right channel ($3C).</summary>
	EchoVolRight = 0x3C,

	/// <summary>Key-on flags - starts voice playback ($4C).</summary>
	KeyOn = 0x4C,

	/// <summary>Key-off flags - releases voice envelope ($5C).</summary>
	KeyOff = 0x5C,

	/// <summary>
	/// Flags register ($6C): noise frequency, echo write disable, mute, reset.
	/// </summary>
	Flags = 0x6C,

	/// <summary>Voice end flags - set when sample ends/loops ($7C, read-only).</summary>
	VoiceEnd = 0x7C,

	/// <summary>Echo feedback volume ($0D).</summary>
	EchoFeedbackVol = 0x0D,

	/// <summary>Pitch modulation enable per voice ($2D).</summary>
	PitchModulationOn = 0x2D,

	/// <summary>Noise enable per voice ($3D).</summary>
	NoiseOn = 0x3D,

	/// <summary>Echo enable per voice ($4D).</summary>
	EchoOn = 0x4D,

	/// <summary>Sample directory table address (page number) ($5D).</summary>
	DirSampleTableAddress = 0x5D,

	/// <summary>Echo ring buffer start address (page number) ($6D).</summary>
	EchoRingBufferAddress = 0x6D,

	/// <summary>Echo delay time in 16ms increments ($7D).</summary>
	EchoDelay = 0x7D,

	/// <summary>Echo FIR filter coefficient 0 ($0F).</summary>
	EchoFilterCoeff0 = 0x0F,

	/// <summary>Echo FIR filter coefficient 1 ($1F).</summary>
	EchoFilterCoeff1 = 0x1F,

	/// <summary>Echo FIR filter coefficient 2 ($2F).</summary>
	EchoFilterCoeff2 = 0x2F,

	/// <summary>Echo FIR filter coefficient 3 ($3F).</summary>
	EchoFilterCoeff3 = 0x3F,

	/// <summary>Echo FIR filter coefficient 4 ($4F).</summary>
	EchoFilterCoeff4 = 0x4F,

	/// <summary>Echo FIR filter coefficient 5 ($5F).</summary>
	EchoFilterCoeff5 = 0x5F,

	/// <summary>Echo FIR filter coefficient 6 ($6F).</summary>
	EchoFilterCoeff6 = 0x6F,

	/// <summary>Echo FIR filter coefficient 7 ($7F).</summary>
	EchoFilterCoeff7 = 0x7F
};

/// <summary>
/// S-DSP per-voice register offsets (add voice*0x10 for address).
/// </summary>
enum class DspVoiceRegs {
	/// <summary>Voice volume left (+$00).</summary>
	VolLeft,

	/// <summary>Voice volume right (+$01).</summary>
	VolRight,

	/// <summary>Pitch low byte (+$02).</summary>
	PitchLow,

	/// <summary>Pitch high byte (+$03, bits 0-5 only).</summary>
	PitchHigh,

	/// <summary>Source/sample number (+$04).</summary>
	SourceNumber,

	/// <summary>ADSR settings 1 - attack/decay rates (+$05).</summary>
	Adsr1,

	/// <summary>ADSR settings 2 - sustain level/rate (+$06).</summary>
	Adsr2,

	/// <summary>Gain settings (when ADSR disabled) (+$07).</summary>
	Gain,

	/// <summary>Current envelope value (+$08, read-only).</summary>
	Envelope,

	/// <summary>Current voice output (+$09, read-only).</summary>
	Out
};

/// <summary>
/// ADSR/Gain envelope state machine modes.
/// </summary>
enum class EnvelopeMode {
	/// <summary>Release phase - envelope decreases to 0.</summary>
	Release,

	/// <summary>Attack phase - envelope increases to max.</summary>
	Attack,

	/// <summary>Decay phase - envelope decreases to sustain level.</summary>
	Decay,

	/// <summary>Sustain phase - envelope held or slowly decreases.</summary>
	Sustain
};