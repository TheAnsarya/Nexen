#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"

class Emulator;

/// <summary>
/// Epson RTC-4513 Real-Time Clock emulation.
/// Used with SPC7110 coprocessor in games like Far East of Eden Zero.
/// Provides battery-backed date/time functionality.
/// </summary>
/// <remarks>
/// The RTC-4513 is a serial real-time clock chip that maintains:
/// - Seconds, minutes, hours (12/24 hour mode)
/// - Day, month, year (with day-of-week)
/// - Control registers for hold, stop, and reset
/// 
/// Access protocol:
/// 1. Write command/index to select register
/// 2. Read/write data at selected register
/// 3. Auto-increment through registers
/// 
/// Time data is stored in BCD format with separate low and high nibble registers.
/// Battery backup maintains time when console is powered off.
/// </remarks>
class Rtc4513 : public ISerializable {
private:
	/// <summary>Reference to emulator for system time.</summary>
	Emulator* _emu;

	/// <summary>Unix timestamp of last time update.</summary>
	uint64_t _lastTime = 0;

	/// <summary>RTC enable state.</summary>
	uint8_t _enabled = 0;

	/// <summary>Current access mode (-1 = idle).</summary>
	int8_t _mode = -1;

	/// <summary>Current register index (-1 = idle).</summary>
	int8_t _index = -1;

	/// <summary>RTC register file (16 registers).</summary>
	uint8_t _regs[0x10] = {};

	/// <summary>Checks if RTC is in reset state.</summary>
	/// <returns>True if reset bit is set.</returns>
	[[nodiscard]] bool IsReset() { return (_regs[0xF] & 0x01) != 0; }

	/// <summary>Checks if RTC counter is stopped.</summary>
	/// <returns>True if stop bit is set.</returns>
	[[nodiscard]] bool IsStop() { return (_regs[0xF] & 0x02) != 0; }

	/// <summary>Checks if time output is held (frozen for reading).</summary>
	/// <returns>True if hold bit is set.</returns>
	[[nodiscard]] bool IsHold() { return (_regs[0xD] & 0x01) != 0; }

	/// <summary>Gets current seconds (0-59) from registers.</summary>
	/// <returns>Seconds value.</returns>
	uint8_t GetSeconds() { return _regs[0] + ((_regs[1] & 0x07) * 10); }

	/// <summary>Gets current minutes (0-59) from registers.</summary>
	/// <returns>Minutes value.</returns>
	uint8_t GetMinutes() { return _regs[2] + ((_regs[3] & 0x07) * 10); }

	/// <summary>Gets current hours (0-23) from registers.</summary>
	/// <returns>Hours value.</returns>
	uint8_t GetHours() { return _regs[4] + ((_regs[5] & 0x03) * 10); }

	/// <summary>Gets current day of month (1-31) from registers.</summary>
	/// <returns>Day value.</returns>
	uint8_t GetDay() { return _regs[6] + ((_regs[7] & 0x03) * 10); }

	/// <summary>Gets current month (1-12) from registers.</summary>
	/// <returns>Month value.</returns>
	uint8_t GetMonth() { return _regs[8] + ((_regs[9] & 0x01) * 10); }

	/// <summary>Gets current year (0-99) from registers.</summary>
	/// <returns>Year value (two digits).</returns>
	uint8_t GetYear() { return _regs[10] + (_regs[11] * 10); }

	/// <summary>Gets current day of week (0-6, Sunday=0) from registers.</summary>
	/// <returns>Day of week value.</returns>
	uint8_t GetDoW() { return _regs[12] & 0x07; }

	/// <summary>Updates internal time based on elapsed real time.</summary>
	void UpdateTime();

public:
	/// <summary>
	/// Creates a new RTC-4513 instance.
	/// </summary>
	/// <param name="emu">Reference to emulator for system time.</param>
	Rtc4513(Emulator* emu);

	/// <summary>Destructor.</summary>
	virtual ~Rtc4513();

	/// <summary>Loads RTC state from battery file.</summary>
	void LoadBattery();

	/// <summary>Saves RTC state to battery file.</summary>
	void SaveBattery();

	/// <summary>
	/// Reads from RTC register.
	/// </summary>
	/// <param name="addr">Address (only low bits used for mode).</param>
	/// <returns>Register data or status.</returns>
	uint8_t Read(uint16_t addr);

	/// <summary>
	/// Writes to RTC register.
	/// </summary>
	/// <param name="addr">Address (only low bits used for mode).</param>
	/// <param name="value">Data to write.</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Serializes RTC state for save states.</summary>
	void Serialize(Serializer& s) override;
};