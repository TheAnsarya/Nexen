#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "GBA/Cart/GbaEeprom.h"
#include "GBA/Cart/GbaGpio.h"

class Emulator;
class GbaConsole;
class GbaMemoryManager;
class GbaFlash;
class GbaGpio;
class GbaRtc;
class GbaTiltSensor;

/// <summary>
/// GBA cartridge handler - manages ROM, save storage, GPIO, and special hardware.
/// Supports various save types, real-time clocks, and motion sensors.
/// </summary>
/// <remarks>
/// **Cartridge Memory Map:**
/// - ROM: 0x08000000-0x09FFFFFF (up to 32MB)
/// - EEPROM: Mapped at various addresses depending on ROM size
/// - Flash/SRAM: 0x0E000000-0x0E00FFFF
///
/// **Save Types:**
/// - **SRAM**: 32KB static RAM (simple byte access)
/// - **EEPROM**: 512B or 8KB serial EEPROM (DMA-based)
/// - **Flash**: 64KB or 128KB (SST/Macronix/Panasonic)
///
/// **GPIO (General Purpose I/O):**
/// - Used by RTC, solar sensors, tilt sensors
/// - Accessible at $80000C4-$80000C9 when enabled
///
/// **Real-Time Clock (RTC):**
/// - Seiko S-3511 compatible
/// - Used by Pokemon games and others
/// - Date/time preserved between sessions
///
/// **Motion Sensors:**
/// - Tilt sensor: Yoshi's Universal Gravitation, WarioWare Twisted
/// - Gyro sensor: WarioWare Twisted
///
/// **Open Bus Behavior:**
/// - Reads beyond ROM return address bits on data bus
/// - Lower 16 bits of address become the read value
/// - Causes "mirror" pattern in memory viewers
///
/// **EEPROM Addressing:**
/// - Small ROMs (≤16MB): EEPROM at $D000000+
/// - Large ROMs (>16MB): EEPROM at $DFFFF00+
/// - Uses DMA3 for transfers
/// </remarks>
class GbaCart final : public ISerializable {
private:
	/// <summary>Emulator instance reference.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Memory manager for bus access.</summary>
	GbaMemoryManager* _memoryManager = nullptr;
	;

	/// <summary>Tilt sensor hardware (optional).</summary>
	shared_ptr<GbaTiltSensor> _tiltSensor;

	/// <summary>EEPROM save storage (optional).</summary>
	unique_ptr<GbaEeprom> _eeprom;

	/// <summary>Flash save storage (optional).</summary>
	unique_ptr<GbaFlash> _flash;

	/// <summary>GPIO port for RTC and sensors.</summary>
	unique_ptr<GbaGpio> _gpio;

	/// <summary>Real-time clock hardware (optional).</summary>
	unique_ptr<GbaRtc> _rtc;

	/// <summary>EEPROM base address (~1 disables).</summary>
	uint32_t _eepromAddr = ~1;

	/// <summary>EEPROM address detection mask.</summary>
	uint32_t _eepromMask = 0;

	/// <summary>Cartridge ROM data.</summary>
	uint8_t* _prgRom = nullptr;

	/// <summary>ROM size in bytes.</summary>
	uint32_t _prgRomSize = 0;

	/// <summary>Save RAM data (SRAM or Flash).</summary>
	uint8_t* _saveRam = nullptr;

	/// <summary>Save RAM size in bytes.</summary>
	uint32_t _saveRamSize = 0;

	/// <summary>True if save RAM has been modified since last save.</summary>
	bool _saveRamDirty = false;

	/// <summary>
	/// Reads from EEPROM via serial protocol.
	/// </summary>
	/// <param name="addr">Access address.</param>
	/// <returns>EEPROM data bit.</returns>
	__noinline uint8_t ReadEeprom(uint32_t addr);

	/// <summary>
	/// Writes to EEPROM via serial protocol.
	/// </summary>
	/// <param name="addr">Access address.</param>
	/// <param name="value">Data bit to write.</param>
	__noinline void WriteEeprom(uint32_t addr, uint8_t value);

public:
	/// <summary>Constructs a new GBA cartridge.</summary>
	GbaCart();

	/// <summary>Destructor - cleans up save storage.</summary>
	~GbaCart();

	/// <summary>Gets the current cartridge state for debugging.</summary>
	/// <returns>Copy of cartridge state.</returns>
	GbaCartState GetState();

	/// <summary>
	/// Initializes the cartridge with ROM and save type.
	/// </summary>
	/// <param name="emu">Emulator instance.</param>
	/// <param name="console">GBA console reference.</param>
	/// <param name="memoryManager">Memory manager reference.</param>
	/// <param name="saveType">Type of save storage.</param>
	/// <param name="rtcType">Type of RTC (if any).</param>
	/// <param name="cartType">Cartridge type for special hardware.</param>
	void Init(Emulator* emu, GbaConsole* console, GbaMemoryManager* memoryManager, GbaSaveType saveType, GbaRtcType rtcType, GbaCartridgeType cartType);

	/// <summary>
	/// Reads from cartridge ROM area.
	/// Handles EEPROM detection and GPIO reads.
	/// </summary>
	/// <typeparam name="checkEeprom">Whether to check for EEPROM access.</typeparam>
	/// <param name="addr">ROM address.</param>
	/// <returns>ROM data or open bus value.</returns>
	template <bool checkEeprom>
	__forceinline uint8_t ReadRom(uint32_t addr) {
		if constexpr (checkEeprom) {
			if ((addr & _eepromMask) == _eepromAddr) {
				return ReadEeprom(addr);
			}
		} else if (_gpio && addr >= 0x80000C4 && addr <= 0x80000C9 && _gpio->CanRead()) {
			return _gpio->Read(addr);
		}

		addr &= 0x1FFFFFF;
		if (addr < _prgRomSize) {
			return _prgRom[addr];
		}

		// Cartridge uses the same lines for the bottom 16-bits of the address and the data.
		// After a load outside of the rom's bounds, the value on the bus is the address, which becomes
		// the value returned by open bus.
		// Addresses are in half-words, so the data received is shifted 1 compared to "addr" here
		// which is in bytes, not half-words.
		return addr & 0x01 ? (addr >> 9) : (addr >> 1);
	}

	/// <summary>
	/// Writes to cartridge ROM area (EEPROM/GPIO only).
	/// </summary>
	/// <param name="addr">Address to write.</param>
	/// <param name="value">Value to write.</param>
	void WriteRom(uint32_t addr, uint8_t value);

	/// <summary>
	/// Reads from cartridge RAM (SRAM/Flash).
	/// </summary>
	/// <param name="addr">RAM address.</param>
	/// <param name="readAddr">Full read address for Flash commands.</param>
	/// <returns>RAM data byte.</returns>
	uint8_t ReadRam(uint32_t addr, uint32_t readAddr);

	/// <summary>
	/// Writes to cartridge RAM (SRAM/Flash).
	/// </summary>
	/// <param name="mode">Access mode (byte/half/word).</param>
	/// <param name="addr">RAM address.</param>
	/// <param name="value">Value to write.</param>
	/// <param name="writeAddr">Full write address for Flash commands.</param>
	/// <param name="fullValue">Complete value for word writes.</param>
	void WriteRam(GbaAccessModeVal mode, uint32_t addr, uint8_t value, uint32_t writeAddr, uint32_t fullValue);

	/// <summary>Debug-mode write to RAM bypassing Flash commands.</summary>
	void DebugWriteRam(uint32_t addr, uint8_t value);

	/// <summary>Gets absolute address info for RAM address.</summary>
	AddressInfo GetRamAbsoluteAddress(uint32_t addr);

	/// <summary>Gets relative RAM address from absolute address.</summary>
	int64_t GetRamRelativeAddress(AddressInfo& absAddress);

	/// <summary>Loads save data from file.</summary>
	void LoadBattery();

	/// <summary>Saves save data to file.</summary>
	void SaveBattery();

	/// <summary>Serializes cartridge state for save states.</summary>
	void Serialize(Serializer& s) override;
};