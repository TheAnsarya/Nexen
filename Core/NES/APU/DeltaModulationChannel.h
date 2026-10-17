#pragma once
#include "pch.h"
#include "NES/APU/ApuTimer.h"
#include "NES/INesMemoryHandler.h"
#include "Utilities/ISerializable.h"

class NesConsole;

/// <summary>
/// NES APU Delta Modulation Channel (DMC) - $4010-$4013.
/// </summary>
/// <remarks>
/// **Overview:**
/// The DMC is a 1-bit delta modulation sample playback channel.
/// It can play DPCM (Delta Pulse Code Modulation) samples stored
/// in ROM, creating sampled sounds like drums and voice.
///
/// **DPCM Format:**
/// Samples are stored as a bitstream where each bit indicates
/// whether to increment (+2) or decrement (-2) the output level.
/// Output is clamped to 0-127 ($00-$7F range, but only 0-126 used).
///
/// **Memory Access:**
/// The DMC has DMA capability to fetch samples from ROM:
/// - Reads from $C000-$FFFF (last 16KB of CPU address space)
/// - DMA halts CPU for 4 cycles per byte
/// - Can conflict with OAM DMA timing
///
/// **Period Table (NTSC):**
/// ```
/// Index: 0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15
/// Value: 428  380  340  320  286  254  226  214  190  160  142  128  106   84   72   54
/// ```
/// Frequencies: ~4.2 kHz to ~33.1 kHz
///
/// **Sample Address:**
/// Address = $C000 + (A * 64)  where A = value written to $4012
/// Length = (L * 16) + 1 bytes where L = value written to $4013
///
/// **Output Level:**
/// - Can be set directly via $4011 (bits 0-6)
/// - Modified by sample playback (±2 per bit)
/// - Contributes to non-linear mixing
///
/// **IRQ:**
/// Can generate IRQ when sample finishes (if not looping and IRQ enabled).
/// Reading $4015 acknowledges and clears the IRQ flag.
///
/// **Loop Mode:**
/// When loop flag is set, sample restarts from beginning when finished.
/// Otherwise, channel stops and optionally triggers IRQ.
///
/// **Registers:**
/// - $4010: Flags (IRQ enable, loop) and rate index
/// - $4011: Direct load of output level (7-bit)
/// - $4012: Sample address
/// - $4013: Sample length
/// </remarks>
class DeltaModulationChannel : public INesMemoryHandler, public ISerializable {
private:
	/// <summary>NTSC DMC period lookup table (16 entries).</summary>
	static constexpr uint16_t _dmcPeriodLookupTableNtsc[16] = {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54};

	/// <summary>PAL DMC period lookup table (16 entries).</summary>
	static constexpr uint16_t _dmcPeriodLookupTablePal[16] = {398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118, 98, 78, 66, 50};

	NesConsole* _console;
	ApuTimer _timer;              ///< Output timer

	uint16_t _sampleAddr = 0;     ///< Starting sample address ($C000 + A*64)
	uint16_t _sampleLength = 0;   ///< Sample length in bytes (L*16 + 1)
	uint8_t _outputLevel = 0;     ///< Current output level (0-127)
	bool _irqEnabled = false;     ///< IRQ enabled when sample ends
	bool _loopFlag = false;       ///< Restart sample when finished

	uint16_t _currentAddr = 0;    ///< Current sample read address
	uint16_t _bytesRemaining = 0; ///< Bytes left to read
	uint8_t _readBuffer = 0;      ///< Buffer for DMA-fetched byte
	bool _bufferEmpty = true;     ///< True if buffer needs refill

	uint8_t _shiftRegister = 0;   ///< Current sample byte being output
	uint8_t _bitsRemaining = 0;   ///< Bits left in shift register
	bool _silenceFlag = true;     ///< True if no sample loaded
	bool _needToRun = false;      ///< Flag for timing sync
	uint8_t _disableDelay = 0;    ///< Delay counter for disable
	uint8_t _transferStartDelay = 0; ///< Delay before DMA starts

	uint8_t _lastValue4011 = 0;   ///< Last value written to $4011

	/// <summary>Initializes sample playback from current address/length.</summary>
	void InitSample();

public:
	/// <summary>Constructs DMC for console.</summary>
	/// <param name="console">Parent NES console.</param>
	DeltaModulationChannel(NesConsole* console);

	/// <summary>Runs DMC to target CPU cycle.</summary>
	/// <param name="targetCycle">CPU cycle to run to.</param>
	void Run(uint32_t targetCycle);

	/// <summary>Resets DMC to initial state.</summary>
	/// <param name="softReset">True for soft reset, false for hard reset.</param>
	void Reset(bool softReset);

	/// <summary>Serializes DMC state.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;

	/// <summary>Checks if IRQ will fire within given cycles.</summary>
	/// <param name="cyclesToRun">Cycles to check ahead.</param>
	/// <returns>True if IRQ pending.</returns>
	bool IrqPending(uint32_t cyclesToRun);

	/// <summary>Checks if DMC needs processing.</summary>
	/// <returns>True if DMC needs to run.</returns>
	bool NeedToRun();

	/// <summary>Gets channel active status.</summary>
	/// <returns>True if bytes remaining > 0.</returns>
	bool GetStatus();

	/// <summary>Gets memory ranges handled by DMC.</summary>
	/// <param name="ranges">Range collection to populate.</param>
	void GetMemoryRanges(MemoryRanges& ranges) override;

	/// <summary>Handles register writes ($4010-$4013).</summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRam(uint16_t addr, uint8_t value) override;

	/// <summary>Reads from register (returns 0, write-only).</summary>
	/// <param name="addr">Register address.</param>
	/// <returns>0 (registers are write-only).</returns>
	uint8_t ReadRam(uint16_t addr) override { return 0; }

	/// <summary>Resets cycle counter at frame boundary.</summary>
	void EndFrame();

	/// <summary>Sets channel enable state from $4015 write.</summary>
	/// <param name="enabled">True to enable/restart, false to stop.</param>
	void SetEnabled(bool enabled);

	/// <summary>Processes one output clock (called by CPU).</summary>
	void ProcessClock();

	/// <summary>Initiates DMA transfer for next sample byte.</summary>
	void StartDmcTransfer();

	/// <summary>Gets address for next DMA read.</summary>
	/// <returns>Sample read address.</returns>
	uint16_t GetDmcReadAddress();

	/// <summary>Receives DMA-fetched byte.</summary>
	/// <param name="value">Byte read from memory.</param>
	void SetDmcReadBuffer(uint8_t value);

	/// <summary>Gets current output value.</summary>
	/// <returns>Output level (0-127).</returns>
	uint8_t GetOutput() { return _timer.GetLastOutput(); }

	/// <summary>Gets channel state for debugging.</summary>
	/// <returns>Snapshot of all DMC parameters.</returns>
	ApuDmcState GetState();
};