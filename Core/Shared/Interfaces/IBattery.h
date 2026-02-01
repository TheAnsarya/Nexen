#pragma once

/// <summary>
/// Interface for battery-backed save RAM persistence.
/// </summary>
/// <remarks>
/// Implementers:
/// - Console classes (NesConsole, SnesConsole, etc.)
/// - Cartridge classes (for mapper-specific save handling)
///
/// Battery types:
/// - SRAM: Static RAM with battery backup (NES/SNES cartridges)
/// - EEPROM: Electrically erasable memory (Game Boy, GBA)
/// - Flash: Flash memory (GBA games)
/// - Real-time clock: RTC with battery (Pokemon, Boktai)
///
/// Save timing:
/// - Auto-save: Every N seconds (configurable)
/// - Manual save: User-triggered
/// - On exit: Before emulator close
/// - On reset/power: Before console reset
///
/// Thread model:
/// - SaveBattery() may be called from any thread
/// - Implementation should use file locking
/// </remarks>
class IBattery {
public:
	/// <summary>
	/// Save battery-backed RAM to persistent storage.
	/// </summary>
	/// <remarks>
	/// Save locations:
	/// - Windows: %APPDATA%/Nexen/Saves/{RomName}.sav
	/// - Linux: ~/.config/Nexen/Saves/{RomName}.sav
	/// - macOS: ~/Library/Application Support/Nexen/Saves/{RomName}.sav
	///
	/// File formats:
	/// - Raw binary dump of SRAM/EEPROM/flash
	/// - No header (pure memory dump)
	/// - Size varies by game (8KB-128KB typical)
	/// </remarks>
	virtual void SaveBattery() = 0;
};
