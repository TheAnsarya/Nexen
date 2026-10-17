#pragma once
#include "pch.h"
#include "SNES/Coprocessors/BaseCoprocessor.h"
#include "Shared/Interfaces/IAudioProvider.h"
#include "Utilities/Audio/HermiteResampler.h"

class SnesConsole;
class Emulator;
class SnesMemoryManager;
class BaseCartridge;
class GbControlManager;
class Spc;
class Gameboy;
class GbPpu;

/// <summary>
/// Super Game Boy (SGB) coprocessor emulation.
/// The Super Game Boy is an SNES cartridge containing a Game Boy CPU/PPU that allows
/// playing Game Boy games on the SNES with enhanced features:
/// - Custom color palettes and borders
/// - Multi-player support (up to 4 players with SGB2)
/// - SNES audio mixing capability
/// - Custom tile/sprite graphics injection
/// Uses a proprietary command packet protocol for SGB ↔ SNES communication.
/// </summary>
/// <remarks>
/// The SGB runs an actual Game Boy CPU (modified for SGB) at a slightly different
/// clock rate than standard Game Boy. It communicates with the SNES through a
/// serial-like packet protocol, sending commands via the joypad port.
/// The LCD output is captured and transferred to SNES VRAM for display with
/// custom palettes and optional borders.
/// </remarks>
class SuperGameboy : public BaseCoprocessor, public IAudioProvider {
private:
	/// <summary>Pointer to SNES console.</summary>
	SnesConsole* _console = nullptr;

	/// <summary>Pointer to main emulator instance.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Pointer to SNES memory manager.</summary>
	SnesMemoryManager* _memoryManager = nullptr;

	/// <summary>Game Boy controller manager for input handling.</summary>
	GbControlManager* _controlManager = nullptr;

	/// <summary>Pointer to SNES cartridge.</summary>
	BaseCartridge* _cart = nullptr;

	/// <summary>Pointer to SNES SPC700 audio processor.</summary>
	Spc* _spc = nullptr;

	/// <summary>Embedded Game Boy instance.</summary>
	Gameboy* _gameboy = nullptr;

	/// <summary>Game Boy PPU for LCD output.</summary>
	GbPpu* _ppu = nullptr;

	/// <summary>SGB control register ($6000-$7FFF writes).</summary>
	uint8_t _control = 0;

	/// <summary>Master clock when reset was triggered.</summary>
	uint64_t _resetClock = 0;

	/// <summary>Clock ratio between SNES and Game Boy clocks.</summary>
	double _clockRatio = 0;

	/// <summary>Effective Game Boy clock rate after adjustment.</summary>
	double _effectiveClockRate = 0;

	/// <summary>Clock offset for synchronization.</summary>
	uint64_t _clockOffset = 0;

	/// <summary>Current tile row for VRAM transfers.</summary>
	uint8_t _row = 0;

	/// <summary>Current tile bank for VRAM transfers.</summary>
	uint8_t _bank = 0;

	/// <summary>Input state for each of the 4 possible players.</summary>
	uint8_t _input[4] = {};

	/// <summary>Currently selected input player index.</summary>
	uint8_t _inputIndex = 0;

	/// <summary>Flag indicating packet reception is in progress.</summary>
	bool _listeningForPacket = false;

	/// <summary>Waiting for high bit in serial protocol.</summary>
	bool _waitForHigh = true;

	/// <summary>Complete packet has been received.</summary>
	bool _packetReady = false;

	/// <summary>Clock cycle of last input write (for timing).</summary>
	uint64_t _inputWriteClock = 0;

	/// <summary>Current input value being transmitted.</summary>
	uint8_t _inputValue = 0;

	/// <summary>16-byte command packet buffer.</summary>
	uint8_t _packetData[16] = {};

	/// <summary>Current byte index within packet.</summary>
	uint8_t _packetByte = 0;

	/// <summary>Current bit index within byte.</summary>
	uint8_t _packetBit = 0;

	/// <summary>Accumulated bits for current byte.</summary>
	uint8_t _packetBuffer = 0;

	/// <summary>Selected LCD row for readout.</summary>
	uint8_t _lcdRowSelect = 0;

	/// <summary>Current read position in LCD buffer.</summary>
	uint16_t _readPosition = 0;

	/// <summary>LCD frame buffer (4 tiles × 1280 bytes per row).</summary>
	uint8_t _lcdBuffer[4][1280] = {};

	/// <summary>Audio resampler for mixing GB audio with SNES.</summary>
	HermiteResampler _resampler;

	/// <summary>Gets the number of active players (1-4).</summary>
	/// <returns>Player count.</returns>
	uint8_t GetPlayerCount();

	/// <summary>
	/// Sets the active input index for reading.
	/// </summary>
	/// <param name="index">Player index (0-3).</param>
	void SetInputIndex(uint8_t index);

	/// <summary>
	/// Sets input state for a player.
	/// </summary>
	/// <param name="index">Player index.</param>
	/// <param name="value">Input button state.</param>
	void SetInputValue(uint8_t index, uint8_t value);

public:
	/// <summary>
	/// Creates a new Super Game Boy instance.
	/// </summary>
	/// <param name="console">SNES console instance.</param>
	/// <param name="gameboy">Game Boy instance to embed.</param>
	SuperGameboy(SnesConsole* console, Gameboy* gameboy);

	/// <summary>Destructor - cleans up SGB resources.</summary>
	~SuperGameboy();

	/// <summary>Resets SGB to initial power-on state.</summary>
	void Reset() override;

	/// <summary>
	/// Reads from SGB register space.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Register value or LCD data.</returns>
	uint8_t Read(uint32_t addr) override;

	/// <summary>
	/// Writes to SGB register space.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint32_t addr, uint8_t value) override;

	/// <summary>Main execution loop - runs Game Boy CPU.</summary>
	void Run() override;

	/// <summary>
	/// Processes joypad port writes for packet communication.
	/// </summary>
	/// <param name="value">Value written to joypad port.</param>
	void ProcessInputPortWrite(uint8_t value);

	/// <summary>Logs the current command packet for debugging.</summary>
	void LogPacket();

	/// <summary>Called at HBlank for LCD capture timing.</summary>
	void ProcessHBlank();

	/// <summary>Called at VBlank for frame completion.</summary>
	void ProcessVBlank();

	/// <summary>
	/// Writes a pixel color to the LCD buffer.
	/// </summary>
	/// <param name="scanline">Current scanline.</param>
	/// <param name="pixel">Pixel X position.</param>
	/// <param name="color">2-bit color value.</param>
	void WriteLcdColor(uint8_t scanline, uint8_t pixel, uint8_t color);

	/// <summary>
	/// Mixes Game Boy audio with SNES audio output.
	/// </summary>
	/// <param name="out">Output buffer.</param>
	/// <param name="sampleCount">Number of samples to generate.</param>
	/// <param name="sampleRate">Target sample rate.</param>
	void MixAudio(int16_t* out, uint32_t sampleCount, uint32_t sampleRate) override;

	/// <summary>Updates the clock ratio after settings change.</summary>
	void UpdateClockRatio();

	/// <summary>Gets the effective Game Boy clock rate.</summary>
	/// <returns>Clock rate in Hz.</returns>
	uint32_t GetClockRate();

	/// <summary>Gets current input player index.</summary>
	/// <returns>Player index (0-3).</returns>
	uint8_t GetInputIndex();

	/// <summary>Gets input state for current player.</summary>
	/// <returns>Button state bitmask.</returns>
	uint8_t GetInput();

	/// <summary>
	/// Peeks SGB memory without side effects.
	/// </summary>
	/// <param name="addr">Address to peek.</param>
	/// <returns>Data value.</returns>
	uint8_t Peek(uint32_t addr) override;

	/// <summary>
	/// Peeks a block of SGB memory.
	/// </summary>
	/// <param name="addr">Starting address.</param>
	/// <param name="output">Buffer for data.</param>
	void PeekBlock(uint32_t addr, uint8_t* output) override;

	/// <summary>
	/// Gets absolute address for debugging.
	/// </summary>
	/// <param name="address">Relative address.</param>
	/// <returns>Absolute address info.</returns>
	AddressInfo GetAbsoluteAddress(uint32_t address) override;

	/// <summary>
	/// Serializes SGB state for save states.
	/// </summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override;
};