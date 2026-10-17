#pragma once
#include "pch.h"
#include "Shared/Emulator.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/Serializer.h"

/// <summary>
/// GBA Flash ROM save storage - command-based interface for 64KB or 128KB saves.
/// Emulates SST, Macronix, and Panasonic flash chips.
/// </summary>
/// <remarks>
/// **Flash Types:**
/// - **64KB (512Kbit)**: Single bank, Macronix MX29L512 or similar
/// - **128KB (1Mbit)**: Dual bank, switchable via $5555 command
///
/// **Command Protocol:**
/// Flash uses a command sequence at magic addresses:
/// 1. Write $AA to $5555
/// 2. Write $55 to $2AAA
/// 3. Write command to $5555
///
/// **Commands:**
/// - $90: Enter Software ID mode
/// - $F0: Exit Software ID mode
/// - $80: Erase mode start
/// - $10: Full chip erase (after $80)
/// - $30: Sector erase (after $80)
/// - $A0: Write byte mode
/// - $B0: Bank switch (128KB only)
///
/// **Software ID:**
/// When in ID mode, reads return chip identification:
/// - Address 0: Manufacturer ID ($62 Sanyo, $32 Macronix)
/// - Address 1: Device ID ($13 for 128KB, $1B for 64KB)
///
/// **Sector Erase:**
/// - Erases 4KB sector to $FF
/// - Sector determined by address bits [15:12]
/// - Takes ~25ms on real hardware
///
/// **Byte Write:**
/// - Programs single byte (can only clear bits, not set)
/// - Takes ~20µs on real hardware
///
/// **Banking (128KB only):**
/// - Bank 0: $E000000-$E00FFFF
/// - Bank 1: $E010000-$E01FFFF
/// - Selected via bank switch command
/// </remarks>
class GbaFlash final : public ISerializable {
private:
	/// <summary>Flash chip state machine modes.</summary>
	enum class ChipMode {
		WaitingForCommand, ///< Waiting for command sequence
		Write,             ///< Write byte mode
		Erase,             ///< Erase mode (waiting for erase type)
		SetMemoryBank      ///< Bank switch mode (128KB only)
	};

	/// <summary>Emulator instance for logging.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Current state machine mode.</summary>
	ChipMode _mode = ChipMode::WaitingForCommand;

	/// <summary>Command sequence cycle counter.</summary>
	uint8_t _cycle = 0;

	/// <summary>True when in Software ID mode.</summary>
	uint8_t _softwareId = false;

	/// <summary>Flash memory buffer.</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Total flash size in bytes.</summary>
	uint32_t _saveRamSize = 0;

	/// <summary>Currently selected bank (0 or 0x10000).</summary>
	uint32_t _selectedBank = 0;

	/// <summary>True if 128KB chip with banking support.</summary>
	bool _allowBanking = false;

public:
	/// <summary>
	/// Constructs a Flash handler.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="saveRam">Pointer to save RAM buffer.</param>
	/// <param name="saveRamSize">Size of flash (64KB or 128KB).</param>
	GbaFlash(Emulator* emu, uint8_t* saveRam, uint32_t saveRamSize) {
		_emu = emu;
		_saveRam = saveRam;
		_saveRamSize = saveRamSize;
		_allowBanking = saveRamSize >= 0x20000;
	}

	/// <summary>Gets the currently selected bank offset.</summary>
	/// <returns>Bank offset (0 or 0x10000).</returns>
	uint32_t GetSelectedBank() {
		return _selectedBank;
	}

	/// <summary>
	/// Reads from flash memory.
	/// Returns chip ID when in Software ID mode.
	/// </summary>
	/// <param name="addr">Address to read.</param>
	/// <returns>Flash data or chip ID.</returns>
	uint8_t Read(uint32_t addr) {
		if (_softwareId && (addr & 0x03) < 2) {
			if (addr & 0x01) {
				return _saveRamSize == 0x10000 ? 0x1B : 0x13;
			} else {
				return _saveRamSize == 0x10000 ? 0x32 : 0x62;
			}
		}

		return _saveRam[_selectedBank | (addr & 0xFFFF)];
	}

	/// <summary>Resets flash state machine to idle.</summary>
	void ResetState() {
		_mode = ChipMode::WaitingForCommand;
		_cycle = 0;
	}

	/// <summary>
	/// Writes to flash memory or processes commands.
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write.</param>
	void Write(uint32_t addr, uint8_t value) {
		uint16_t cmd = addr & 0xFFFF;
		if (_mode == ChipMode::SetMemoryBank) {
			if (cmd == 0) {
				_selectedBank = (value & 0x01) << 16;
			}
			ResetState();
		}
		if (_mode == ChipMode::WaitingForCommand) {
			if (_cycle == 0) {
				if (cmd == 0x5555 && value == 0xAA) {
					// 1st write, $5555 = $AA
					_cycle++;
				} else if (value == 0xF0) {
					// Software ID exit
					ResetState();
					_softwareId = false;
				}
			} else if (_cycle == 1 && cmd == 0x2AAA && value == 0x55) {
				// 2nd write, $2AAA = $55
				_cycle++;
			} else if (_cycle == 2 && cmd == 0x5555) {
				// 3rd write, determines command type
				_cycle++;
				switch (value) {
					case 0x80:
						_emu->DebugLog("[Flash] 0x80 - Enter erase mode");
						_mode = ChipMode::Erase;
						break;

					case 0x90:
						_emu->DebugLog("[Flash] 0x90 - Enter software ID mode");
						ResetState();
						_softwareId = true;
						break;

					case 0xA0:
						_emu->DebugLog("[Flash] 0xA0 - Enter write byte mode");
						_mode = ChipMode::Write;
						break;

					case 0xB0:
						if (_allowBanking) {
							//_emu->DebugLog("[Flash] 0xB0 - Set memory bank");
							_mode = ChipMode::SetMemoryBank;
						}
						break;

					case 0xF0:
						_emu->DebugLog("[Flash] 0xF0 - Exit software ID mode");
						ResetState();
						_softwareId = false;
						break;

					default:
						_emu->DebugLog("[Flash] 0x" + HexUtilities::ToHex(value) + " - Unknown command");
						break;
				}
			} else {
				_cycle = 0;
			}
		} else if (_mode == ChipMode::Write) {
			// Write a single byte
			_saveRam[_selectedBank | (addr & 0xFFFF)] &= value;
			ResetState();
		} else if (_mode == ChipMode::Erase) {
			if (_cycle == 3) {
				// 4th write for erase command, $5555 = $AA
				if (cmd == 0x5555 && value == 0xAA) {
					_cycle++;
				} else {
					ResetState();
				}
			} else if (_cycle == 4) {
				// 5th write for erase command, $2AAA = $55
				if (cmd == 0x2AAA && value == 0x55) {
					_cycle++;
				} else {
					ResetState();
				}
			} else if (_cycle == 5) {
				if (cmd == 0x5555 && value == 0x10) {
					// Chip erase
					_emu->DebugLog("[Flash] Chip erase");
					memset(_saveRam, 0xFF, _saveRamSize);
				} else if (value == 0x30) {
					// Sector erase
					uint32_t offset = _selectedBank | (addr & 0xF000);
					_emu->DebugLog("[Flash] Sector erase: " + HexUtilities::ToHex(offset));
					if (offset + 0x1000 <= _saveRamSize) {
						memset(_saveRam + offset, 0xFF, 0x1000);
					}
				}
				ResetState();
			}
		}
	}

	void Serialize(Serializer& s) {
		SV(_mode);
		SV(_cycle);
		SV(_softwareId);
		SV(_selectedBank);
	}
};