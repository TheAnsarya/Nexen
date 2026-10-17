#pragma once
#include "pch.h"
#include "Gameboy/Carts/GbCart.h"
#include "Gameboy/Carts/GbMbc3Rtc.h"
#include "Gameboy/GbMemoryManager.h"
#include "Utilities/Serializer.h"

/// <summary>
/// MBC3 (Memory Bank Controller 3) with optional Real-Time Clock.
/// Supports up to 2MB ROM, 32KB RAM, and battery-backed RTC.
/// </summary>
/// <remarks>
/// **MBC3 Specifications:**
/// - ROM: Up to 2MB (128 banks × 16KB), MBC30: 4MB (256 banks)
/// - RAM: Up to 32KB (4 banks × 8KB), MBC30: 64KB (8 banks)
/// - RTC: Optional real-time clock with battery backup
/// - Games: Pokémon Gold/Silver/Crystal, Zelda: Oracle series
///
/// **Register Map:**
/// - $0000-$1FFF: RAM/RTC Enable (write $0A to enable)
/// - $2000-$3FFF: ROM Bank Number (7-bit, 1-127)
/// - $4000-$5FFF: RAM Bank / RTC Register Select
/// - $6000-$7FFF: Latch Clock Data
///
/// **RAM Bank / RTC Select ($4000-$5FFF):**
/// - $00-$03: Select RAM bank 0-3 ($00-$07 for MBC30)
/// - $08-$0C: Select RTC register (if RTC present)
///   - $08: RTC Seconds (0-59)
///   - $09: RTC Minutes (0-59)
///   - $0A: RTC Hours (0-23)
///   - $0B: RTC Day Low (bits 0-7)
///   - $0C: RTC Day High + Flags (bit 0: day bit 8, bit 6: halt, bit 7: day overflow)
///
/// **Clock Latch ($6000-$7FFF):**
/// Write $00 then $01 to latch current time into RTC registers.
/// This prevents time from changing during read sequence.
///
/// **MBC30 Variant:**
/// Extended version with 256 ROM banks and 8 RAM banks.
/// Used in Pokémon Crystal (Japanese version).
/// </remarks>
class GbMbc3 : public GbCart {
private:
	/// <summary>MBC30 extended variant (256 ROM banks, 8 RAM banks).</summary>
	bool _isMbc30 = false;

	/// <summary>Cartridge has RTC hardware.</summary>
	bool _hasRtcTimer = false;

	/// <summary>RAM/RTC access enabled ($0A written).</summary>
	bool _ramRtcEnabled = false;

	/// <summary>Current PRG bank (7-bit, banks 1-127/255).</summary>
	uint8_t _prgBank = 1;

	/// <summary>RAM bank or RTC register select (4-bit).</summary>
	uint8_t _ramBank = 0;

	/// <summary>Real-time clock controller.</summary>
	GbMbc3Rtc _rtc;

public:
	/// <summary>
	/// Creates an MBC3 mapper instance.
	/// </summary>
	/// <param name="emu">Emulator instance for timing.</param>
	/// <param name="hasRtcTimer">True if cartridge has RTC hardware.</param>
	/// <param name="isMbc30">True for MBC30 extended variant.</param>
	GbMbc3(Emulator* emu, bool hasRtcTimer, bool isMbc30) : _rtc(emu) {
		_isMbc30 = isMbc30;
		_hasRtcTimer = hasRtcTimer;
	}

	/// <summary>
	/// Initializes MBC3 registers and RTC.
	/// </summary>
	void InitCart() override {
		_memoryManager->MapRegisters(0x0000, 0x7FFF, RegisterAccess::Write);
		_rtc.Init();
	}

	/// <summary>
	/// Saves RTC state to battery file.
	/// </summary>
	void SaveBattery() override {
		_rtc.SaveBattery();
	}

	/// <summary>
	/// Updates memory mappings based on bank registers.
	/// Routes RAM or RTC access based on bank select value.
	/// </summary>
	void RefreshMappings() override {
		constexpr int prgBankSize = 0x4000;  // 16KB PRG banks
		constexpr int ramBankSize = 0x2000;  // 8KB RAM banks

		// Bank 0 fixed at $0000-$3FFF
		Map(0x0000, 0x3FFF, GbMemoryType::PrgRom, 0, true);
		// Switchable bank at $4000-$7FFF
		Map(0x4000, 0x7FFF, GbMemoryType::PrgRom, _prgBank * prgBankSize, true);

		// Max RAM bank: 3 for MBC3, 7 for MBC30
		if (_ramRtcEnabled && _ramBank <= (_isMbc30 ? 7 : 3)) {
			// RAM bank selected
			Map(0xA000, 0xBFFF, GbMemoryType::CartRam, _ramBank * ramBankSize, false);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::None);
		} else if (_hasRtcTimer && _ramRtcEnabled && _ramBank >= 0x08 && _ramBank <= 0x0C) {
			// RTC register selected ($08-$0C)
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::ReadWrite);
		} else {
			// Disabled: reads return $FF through handler
			Unmap(0xA000, 0xBFFF);
			_memoryManager->MapRegisters(0xA000, 0xBFFF, RegisterAccess::Read);
		}
	}

	/// <summary>
	/// Reads from RTC registers or disabled RAM.
	/// </summary>
	/// <param name="addr">Address being read ($A000-$BFFF).</param>
	/// <returns>RTC register value or $FF if disabled.</returns>
	uint8_t ReadRegister(uint16_t addr) override {
		if (!_ramRtcEnabled) {
			// Disabled RAM/RTC registers returns 0xFF on reads (?)
			return 0xFF;
		}

		if (_hasRtcTimer) {
			return _rtc.Read(_ramBank & 0x0F);
		} else {
			return 0xFF;
		}
	}

	/// <summary>
	/// Writes to MBC3 control registers or RTC.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint16_t addr, uint8_t value) override {
		if (addr <= 0x7FFF) {
			switch (addr & 0x6000) {
				case 0x0000:
					// RAM/RTC enable: $0A enables, else disables
					_ramRtcEnabled = ((value & 0x0F) == 0x0A);
					break;
				case 0x2000:
					// ROM bank: 0 maps to 1 (like MBC1)
					_prgBank = std::max<uint8_t>(1, value);
					break;
				case 0x4000:
					// RAM bank ($00-$03/$07) or RTC register ($08-$0C)
					_ramBank = value & 0x0F;
					break;
				case 0x6000:
					// Latch clock: write $00 then $01 to latch time
					_rtc.LatchData();
					break;
			}
			RefreshMappings();
		} else if (addr >= 0xA000 && addr <= 0xBFFF) {
			// Write to RTC register
			_rtc.Write(_ramBank & 0x0F, value);
		}
	}

	/// <summary>Serializes MBC3 state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) override {
		SV(_ramRtcEnabled);
		SV(_prgBank);
		SV(_ramBank);
		SV(_rtc);
	}
};