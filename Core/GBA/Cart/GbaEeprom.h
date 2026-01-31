#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

/// <summary>EEPROM state machine modes.</summary>
enum class GbaEepromMode {
	Idle,          ///< Waiting for command start
	Command,       ///< Receiving command bits
	ReadCommand,   ///< Received read command, processing address
	ReadDataReady, ///< Ready to output read data
	WriteCommand,  ///< Receiving write data
};

/// <summary>
/// GBA EEPROM save storage - serial interface for 512B or 8KB saves.
/// Uses a bit-serial protocol accessed via DMA or slow bus reads.
/// </summary>
/// <remarks>
/// **EEPROM Types:**
/// - **512 bytes (4Kbit)**: 6-bit addressing, 64 entries × 8 bytes
/// - **8KB (64Kbit)**: 14-bit addressing, 1024 entries × 8 bytes
///
/// **Protocol:**
/// EEPROMs are accessed bit-by-bit through a serial interface:
/// 1. Write command bits (2 bits: 11=read, 10=write)
/// 2. Write address bits (6 or 14 bits)
/// 3. Read: Receive 68 bits (4 dummy + 64 data)
/// 4. Write: Send 64 data bits + 1 stop bit
///
/// **Auto-Detection:**
/// - Address size detected from command length
/// - 9 bits total (2 cmd + 6 addr + 1 stop) = 512B
/// - 17 bits total (2 cmd + 14 addr + 1 stop) = 8KB
///
/// **DMA Access:**
/// - EEPROM must be accessed via DMA3
/// - DMA transfers 1 bit per cycle
/// - Games typically set up 9 or 17 word DMA transfers
///
/// **Timing:**
/// - Reads require ~108,368 cycles (~6.5ms)
/// - Writes require ~108,368 cycles + busy wait
/// </remarks>
class GbaEeprom final : public ISerializable {
private:
	/// <summary>Pointer to save RAM buffer.</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Address bit width (6 for 512B, 14 for 8KB).</summary>
	uint8_t _addressSize = 0;

	/// <summary>Maximum valid address.</summary>
	uint16_t _maxAddress = 0x3FF;

	/// <summary>Current state machine mode.</summary>
	GbaEepromMode _mode = GbaEepromMode::Idle;

	/// <summary>Current address being accessed.</summary>
	uint32_t _address = 0;

	/// <summary>Bit counter for address/data transfers.</summary>
	uint32_t _len = 0;

	/// <summary>Counter for read data output.</summary>
	uint32_t _counter = 0;

	/// <summary>Data buffer for write operations.</summary>
	uint64_t _writeData = 0;

public:
	/// <summary>
	/// Constructs an EEPROM handler.
	/// </summary>
	/// <param name="saveRam">Pointer to save RAM buffer.</param>
	/// <param name="saveType">EEPROM size type.</param>
	GbaEeprom(uint8_t* saveRam, GbaSaveType saveType) {
		if (saveType == GbaSaveType::Eeprom512) {
			_addressSize = 6;
			_maxAddress = 0x3F;
		} else if (saveType == GbaSaveType::Eeprom8192) {
			_addressSize = 14;
			_maxAddress = 0x3FF;
		}
		_saveRam = saveRam;
	}

	/// <summary>
	/// Reads one bit from EEPROM.
	/// Called by DMA during read operations.
	/// </summary>
	/// <returns>Bit 0 = data bit, other bits undefined.</returns>
	__noinline uint8_t Read() {
		switch (_mode) {
			case GbaEepromMode::ReadCommand:
				// Auto-detect EEPROM size and then start reading
				if (_addressSize != 0) {
					return 1;
				}

				_addressSize = _len == 7 ? 6 : 14;
				_maxAddress = _addressSize == 6 ? 0x3F : 0x3FF;
				_address >>= 1;
				_counter = 0;
				_mode = GbaEepromMode::ReadDataReady;
				[[fallthrough]];

			case GbaEepromMode::ReadDataReady:
				if (++_counter > 4) {
					uint8_t value;
					if (_address <= _maxAddress) {
						uint8_t offset = (68 - _counter);
						value = (_saveRam[(_address << 3) + (offset >> 3)] >> (offset & 0x07)) & 0x01;
					} else {
						value = 1;
					}
					if (_counter >= 68) {
						_mode = GbaEepromMode::Idle;
					}
					return value;
				}
				return 1;

			default:
				return 1;
		}
	}

	/// <summary>
	/// Writes one bit to EEPROM.
	/// Called by DMA during command/write operations.
	/// </summary>
	/// <param name="value">Bit 0 = data bit to write.</param>
	__noinline void Write(uint8_t value) {
		uint8_t bit = value & 0x01;

		switch (_mode) {
			case GbaEepromMode::Idle:
				if (bit) {
					_mode = GbaEepromMode::Command;
				}
				break;

			case GbaEepromMode::Command:
				_address = 0;
				_len = 0;
				_counter = 0;
				_mode = bit ? GbaEepromMode::ReadCommand : GbaEepromMode::WriteCommand;
				break;

			case GbaEepromMode::ReadCommand:
				if (_addressSize && _len == _addressSize) {
					_mode = GbaEepromMode::ReadDataReady;
					_counter = 0;
				} else {
					_len++;
					_address = (_address << 1) | bit;
				}
				break;

			case GbaEepromMode::WriteCommand:
				if (_addressSize == 0) {
					_mode = GbaEepromMode::Idle;
					return;
				}

				if (++_len > _addressSize) {
					if (_counter < 64) {
						_writeData = (_writeData << 1) | bit;
						_counter++;
					} else {
						if (_address <= _maxAddress) {
							for (int i = 0; i < 8; i++) {
								_saveRam[(_address << 3) + i] = _writeData & 0xFF;
								_writeData >>= 8;
							}
						}
						_mode = GbaEepromMode::Idle;
					}
				} else {
					_address = (_address << 1) | bit;
				}
				break;
		}
	}

	uint32_t GetSaveSize() {
		return (_maxAddress + 1) * 8;
	}

	void Serialize(Serializer& s) {
		SV(_addressSize);
		SV(_maxAddress);

		SV(_address);
		SV(_len);
		SV(_counter);
		SV(_writeData);
		SV(_mode);
	}
};