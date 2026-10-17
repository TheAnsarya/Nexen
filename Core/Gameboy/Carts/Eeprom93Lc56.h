#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>
/// 93LC56 Serial EEPROM emulation (256 bytes organized as 128 × 16-bit words).
/// Used in MBC7 cartridges for save data storage.
/// </summary>
/// <remarks>
/// **93LC56 Specifications:**
/// - Capacity: 256 bytes (2 Kbit) as 128 × 16-bit words
/// - Interface: 4-wire serial (CS, CLK, DI, DO)
/// - Protocol: Command + address + data, clocked on rising edge
///
/// **Pin Mapping (from MBC7):**
/// - Bit 7: CS (Chip Select) - must be high for operation
/// - Bit 6: CLK (Clock) - data sampled on rising edge
/// - Bit 1: DI (Data In) - serial data to EEPROM
/// - Bit 0: DO (Data Out) - serial data from EEPROM (read)
///
/// **Command Format (10 bits):**
/// Start bit (1) + Opcode (2 bits) + Address (7 bits)
///
/// **Commands:**
/// | Opcode | Command | Description |
/// |--------|---------|-------------|
/// | 10     | READ    | Read 16-bit word at address |
/// | 01     | WRITE   | Write 16-bit word at address |
/// | 11     | ERASE   | Erase word at address (set to $FFFF) |
/// | 00 11  | EWEN    | Enable write/erase operations |
/// | 00 00  | EWDS    | Disable write/erase operations |
/// | 00 10  | ERAL    | Erase all (set all to $FFFF) |
/// | 00 01  | WRAL    | Write all (set all to same value) |
///
/// **Read Operation:**
/// 1. Send READ command + address (10 bits)
/// 2. EEPROM outputs 16 data bits on DO line
/// 3. Can continue reading sequential addresses
///
/// **Write Operation:**
/// 1. Send EWEN command to enable writes
/// 2. Send WRITE command + address + 16-bit data
/// 3. EEPROM enters busy state during program cycle
/// 4. DO line goes high when complete
///
/// **Write Protection:**
/// Writes/erases are blocked until EWEN command is issued.
/// EWDS command re-enables protection.
/// </remarks>
class Eeprom93Lc56 final : public ISerializable {
private:
	/// <summary>EEPROM state machine modes.</summary>
	enum class Mode {
		Idle,           ///< Waiting for start bit
		Command,        ///< Receiving command + address
		ReadCommand,    ///< Outputting read data
		WriteCommand,   ///< Receiving write data
		WriteAllCommand ///< Receiving data for WRAL
	};

	/// <summary>Pointer to save RAM buffer (256 bytes).</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Current state machine mode.</summary>
	Mode _mode = Mode::Idle;

	/// <summary>Chip select line state.</summary>
	bool _chipSelect = false;

	/// <summary>Clock line state (for edge detection).</summary>
	bool _clk = false;

	/// <summary>Data input line state.</summary>
	uint8_t _dataIn = 0;

	/// <summary>Write/erase operations enabled (EWEN issued).</summary>
	bool _writeEnabled = false;

	/// <summary>Accumulated command bits (up to 10).</summary>
	uint16_t _commandId = 0;

	/// <summary>Number of command bits received.</summary>
	uint8_t _bitCount = 0;

	/// <summary>Current read address (7-bit word address).</summary>
	uint8_t _readAddress = 0;

	/// <summary>Current bit position in read operation (-1 = dummy bit).</summary>
	int8_t _readCounter = -1;

	/// <summary>Accumulated write data (16 bits).</summary>
	uint16_t _writeData = 0;

	/// <summary>Number of write data bits received.</summary>
	uint8_t _writeCounter = 0;

public:
	/// <summary>
	/// Sets the RAM buffer for EEPROM storage.
	/// </summary>
	/// <param name="saveRam">256-byte buffer for EEPROM contents.</param>
	void SetRam(uint8_t* saveRam) {
		_saveRam = saveRam;
	}

	/// <summary>
	/// Reads the current state of EEPROM pins including data output.
	/// </summary>
	/// <returns>
	/// Bit 7: CS state, Bit 6: CLK state, Bit 1: DI state,
	/// Bit 0: DO (data out or ready/busy status).
	/// </returns>
	uint8_t Read() {
		// Return current pin states
		uint8_t result = ((_chipSelect ? 0x80 : 0) |
		                  (_clk ? 0x40 : 0) |
		                  (_dataIn ? 0x02 : 0));

		if (_mode == Mode::ReadCommand) {
			if (_readCounter < 0) {
				// Dummy bit before data (always 1)
				return result | 0x01;
			}
			// Output data bits MSB first, alternating between high and low bytes
			return result | ((_saveRam[(_readAddress << 1) + (_readCounter >= 8 ? 0 : 1)] >> (7 - (_readCounter & 0x07))) & 0x01);
		} else {
			// Ready/busy status: 1 = ready (idle), 0 = busy
			return result | (_mode == Mode::Idle ? 1 : 0);
		}
	}

	/// <summary>
	/// Writes to EEPROM control pins. Operations occur on clock rising edge.
	/// </summary>
	/// <param name="value">Pin states (CS, CLK, DI).</param>
	void Write(uint8_t value) {
		uint8_t prevClk = _clk;
		bool clk = (value & 0x40);
		_dataIn = (value & 0x02) >> 1;
		_chipSelect = value & 0x80;
		_clk = clk;

		if (!_chipSelect) {
			// Chip select disabled - return to idle
			_mode = Mode::Idle;
			return;
		}

		if (!clk || prevClk) {
			// Not a rising clock edge - no operation
			return;
		}

		// Process on rising clock edge
		switch (_mode) {
			case Mode::Idle:
				// Start bit detection (DI = 1)
				if (_dataIn) {
					_mode = Mode::Command;
					_commandId = 0;
					_bitCount = 0;
					_writeCounter = 0;
					_writeData = 0;
					_readCounter = -1;
					_readAddress = 0;
				}
				break;

			case Mode::Command:
				// Shift in command bits
				_commandId <<= 1;
				_commandId |= _dataIn;
				_bitCount++;

				if (_bitCount >= 10) {
					// Full command received - decode opcode
					uint8_t id = _commandId >> 6;
					if ((id & 0b1100) == 0b1000) {
						// READ command: 10xxxxxxx
						_mode = Mode::ReadCommand;
						_readAddress = _commandId & 0x7F;
					} else if ((id & 0b1100) == 0b0100) {
						// WRITE command: 01xxxxxxx
						_mode = Mode::WriteCommand;
					} else if ((id & 0b1100) == 0b1100) {
						// ERASE command: 11xxxxxxx - erase single word
						if (_writeEnabled) {
							uint8_t addr = _commandId & 0x7F;
							_saveRam[addr << 1] = 0xFF;
							_saveRam[(addr << 1) + 1] = 0xFF;
						}
						_mode = Mode::Idle;
					} else if ((id & 0b1111) == 0b0000) {
						// EWDS: 00 00xxxxx - Disable writes
						_writeEnabled = false;
						_mode = Mode::Idle;
					} else if ((id & 0b1111) == 0b0011) {
						// EWEN: 00 11xxxxx - Enable writes
						_writeEnabled = true;
						_mode = Mode::Idle;
					} else if ((id & 0b1111) == 0b0010) {
						// ERAL: 00 10xxxxx - Erase all
						if (_writeEnabled) {
							memset(_saveRam, 0xFF, 256);
						}
						_mode = Mode::Idle;
					} else if ((id & 0b1111) == 0b0001) {
						// WRAL: 00 01xxxxx - Write all
						_mode = Mode::WriteAllCommand;
					}
				}
				break;

			case Mode::ReadCommand:
				// Output read data, advance bit counter
				_readCounter++;
				if (_readCounter == 16) {
					// Word complete - advance to next address or idle
					_readAddress = (_readAddress + 1) & 0x7F;
					_readCounter = 0;
					_mode = Mode::Idle;
				}
				break;

			case Mode::WriteAllCommand:
			case Mode::WriteCommand: {
				// Shift in write data
				_writeData <<= 1;
				_writeData |= _dataIn;
				_writeCounter++;
				if (_writeCounter >= 16) {
					// 16-bit word complete - program to EEPROM
					if (_writeEnabled) {
						if (_mode == Mode::WriteAllCommand) {
							// Write same value to all 128 words
							for (int i = 0; i < 0x80; i++) {
								_saveRam[i << 1] = _writeData;
								_saveRam[(i << 1) + 1] = _writeData >> 8;
							}
						} else {
							// Write to single word
							uint8_t addr = _commandId & 0x7F;
							_saveRam[addr << 1] = _writeData;
							_saveRam[(addr << 1) + 1] = _writeData >> 8;
						}
					}
					_mode = Mode::Idle;
				}
				break;
			}
		}
	}

	/// <summary>Serializes EEPROM state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) {
		SV(_mode);
		SV(_chipSelect);
		SV(_clk);
		SV(_dataIn);
		SV(_writeEnabled);
		SV(_commandId);
		SV(_bitCount);
		SV(_readAddress);
		SV(_readCounter);
		SV(_writeData);
		SV(_writeCounter);
	}
};