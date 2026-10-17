/// <summary>
/// MBC3 Real-Time Clock implementation with battery backup support.
/// Provides persistent timekeeping for save files.
/// </summary>
/// <remarks>
/// **RTC Specifications:**
/// - Seconds: 0-59 (6-bit, $00-$3B)
/// - Minutes: 0-59 (6-bit, $00-$3B)
/// - Hours: 0-23 (5-bit, $00-$17)
/// - Days: 0-511 (9-bit split: $00-$FF + high bit)
/// - Halt flag: Stops RTC when set
/// - Overflow flag: Set when day counter overflows 511
///
/// **RTC Registers ($08-$0C):**
/// - $08: Seconds (bits 0-5)
/// - $09: Minutes (bits 0-5)
/// - $0A: Hours (bits 0-4)
/// - $0B: Day counter low (bits 0-7)
/// - $0C: Day counter high + flags:
///   - Bit 0: Day counter bit 8
///   - Bit 6: Halt (0=running, 1=stopped)
///   - Bit 7: Day overflow (set when >511 days)
///
/// **Latch Mechanism:**
/// Writing to $6000-$7FFF latches current time into readable registers.
/// This prevents time from changing during a multi-register read.
/// Documentation says $00→$01 transition, but games expect any write.
///
/// **Battery Persistence:**
/// - RTC state saved with timestamp on power-off
/// - On load, elapsed real time is calculated
/// - Clock advances by elapsed seconds during restore
///
/// **Timing:**
/// - Clocked from 32.768 kHz crystal (same as real hardware)
/// - Synchronized to emulator master clock
/// </remarks>
#include "pch.h"
#include <time.h>
#include "Shared/Emulator.h"
#include "Shared/BatteryManager.h"
#include "Utilities/HexUtilities.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"

class GbMbc3Rtc : public ISerializable {
private:
	/// <summary>Emulator reference for timing.</summary>
	Emulator* _emu = nullptr;

	/// <summary>Live RTC registers [S, M, H, DL, DH].</summary>
	uint8_t _regs[5] = {};

	/// <summary>Latched RTC values for stable reads.</summary>
	uint8_t _latchedRegs[5] = {};

	/// <summary>Last written latch value (for $00→$01 detection).</summary>
	uint8_t _latchValue = 0;

	/// <summary>Master clock at last update.</summary>
	uint64_t _lastMasterClock = 0;

	/// <summary>Sub-second tick accumulator (32768 ticks = 1 second).</summary>
	uint64_t _tickCounter = 0;

public:
	/// <summary>Checks if RTC is running (not halted).</summary>
	/// <returns>True if RTC is advancing time.</returns>
	[[nodiscard]] bool IsRunning() { return (_regs[4] & 0x40) == 0; }

	/// <summary>Gets current seconds (0-59).</summary>
	[[nodiscard]] uint8_t GetSeconds() { return _regs[0]; }

	/// <summary>Gets current minutes (0-59).</summary>
	[[nodiscard]] uint8_t GetMinutes() { return _regs[1]; }

	/// <summary>Gets current hours (0-23).</summary>
	[[nodiscard]] uint8_t GetHours() { return _regs[2]; }

	/// <summary>Gets current day count (0-511).</summary>
	[[nodiscard]] uint16_t GetDayCount() { return _regs[3] | ((_regs[4] & 0x01) << 8); }

	/// <summary>Creates an RTC controller.</summary>
	/// <param name="emu">Emulator instance for timing.</param>
	GbMbc3Rtc(Emulator* emu) {
		_emu = emu;
		Init();
	}

	/// <summary>Initializes RTC and loads battery state.</summary>
	void Init() {
		_lastMasterClock = 0;
		LoadBattery();
	}

	/// <summary>
	/// Loads RTC state from battery file and advances time.
	/// </summary>
	/// <remarks>
	/// Battery file format: 5 bytes registers + 8 bytes timestamp (ms since epoch).
	/// On load, calculates elapsed time since save and advances RTC accordingly.
	/// </remarks>
	void LoadBattery() {
		vector<uint8_t> rtcData = _emu->GetBatteryManager()->LoadBattery(".rtc");

		if (rtcData.size() == sizeof(_regs) + sizeof(uint64_t)) {
			memcpy(_regs, rtcData.data(), sizeof(_regs));

			// Parse saved timestamp (big-endian)
			uint64_t time = 0;
			for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
				time <<= 8;
				time |= rtcData[sizeof(_regs) + i];
			}

			// Calculate elapsed time since save
			int64_t elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - time;
			if (elapsedMs > 0) {
				// Run clock forward based on how much time has passed since the game was turned off
				RunForDuration(elapsedMs / 1000);
			}
		}
	}

	/// <summary>
	/// Saves RTC state and current timestamp to battery file.
	/// </summary>
	void SaveBattery() {
		vector<uint8_t> rtcData;
		rtcData.resize(sizeof(_regs) + sizeof(uint64_t), 0);

		// Save register state
		memcpy(rtcData.data(), _regs, sizeof(_regs));

		// Save current timestamp (big-endian) for elapsed time calculation on load
		uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		for (uint32_t i = 0; i < sizeof(uint64_t); i++) {
			rtcData[sizeof(_regs) + i] = (time >> 56) & 0xFF;
			time <<= 8;
		}

		_emu->GetBatteryManager()->SaveBattery(".rtc", std::span<const uint8_t>(rtcData));
	}

	/// <summary>
	/// Updates RTC based on elapsed emulator time.
	/// Called before reads/writes to sync with real time.
	/// </summary>
	void UpdateTime() {
		uint64_t elaspedClocks = _emu->GetMasterClock() - _lastMasterClock;
		uint64_t clocksPerTick = _emu->GetMasterClockRate() / 32768;

		if (IsRunning()) {
			// Accumulate ticks (32768 per second like real 32.768 kHz crystal)
			_tickCounter += elaspedClocks / clocksPerTick;
			while (_tickCounter >= 32768) {
				RunForDuration(1);
				_tickCounter -= 32768;
			}
		}

		// Track remainder for accurate timing
		uint64_t remainder = elaspedClocks % clocksPerTick;
		_lastMasterClock = _emu->GetMasterClock() - remainder;
	}

	/// <summary>
	/// Advances RTC by specified number of seconds.
	/// Handles overflow through minutes, hours, and days.
	/// </summary>
	/// <param name="seconds">Number of seconds to advance.</param>
	void RunForDuration(int64_t seconds) {
		while (seconds > 0) {
			// Advance in batches up to next minute boundary
			int64_t maxAmount = _regs[0] >= 60 ? 1 : std::min<int64_t>(60 - _regs[0], seconds);

			seconds -= maxAmount;
			_regs[0] = (_regs[0] + maxAmount) & 0x3F;

			// Handle overflow cascades
			if (GetSeconds() == 60) {
				_regs[0] = 0;
				_regs[1] = (_regs[1] + 1) & 0x3F;

				if (GetMinutes() == 60) {
					_regs[1] = 0;
					_regs[2] = (_regs[2] + 1) & 0x1F;

					if (GetHours() == 24) {
						_regs[2] = 0;

						// Day counter overflow handling
						if (GetDayCount() == 0xFF) {
							// Roll over to day 256+ (set high bit)
							_regs[3] = 0;
							_regs[4] |= 0x01;
						} else if (GetDayCount() == 0x1FF) {
							// Day 511 overflow: wrap to 0, set overflow flag
							_regs[3] = 0;
							_regs[4] &= ~0x01;
							_regs[4] |= 0x80; // set overflow
						} else {
							_regs[3]++;
						}
					}
				}
			}
		}
	}

	/// <summary>
	/// Latches current time values for stable reading.
	/// </summary>
	/// <remarks>
	/// Despite documentation saying $00→$01 transition is required,
	/// latch-rtc-test only passes if latching occurs on every write.
	/// </remarks>
	void LatchData() {
		// Despite documentation saying otherwise, the latch-rtc-test only works if latching
		// is done on every write (rather than latching on a 0->1 transition on bit 0)
		UpdateTime();
		memcpy(_latchedRegs, _regs, sizeof(_regs));
	}

	/// <summary>
	/// Reads from latched RTC registers.
	/// </summary>
	/// <param name="addr">Register selector ($08-$0C).</param>
	/// <returns>Latched register value.</returns>
	uint8_t Read(uint16_t addr) {
		switch (addr) {
			case 0x08:
				return _latchedRegs[0]; // Seconds
			case 0x09:
				return _latchedRegs[1]; // Minutes
			case 0x0A:
				return _latchedRegs[2]; // Hours
			case 0x0B:
				return _latchedRegs[3]; // Day counter low
			case 0x0C:
				return _latchedRegs[4]; // Day counter high + carry/halt flags
		}

		return 0xFF;
	}

	/// <summary>
	/// Writes to RTC registers.
	/// </summary>
	/// <param name="addr">Register selector ($08-$0C).</param>
	/// <param name="value">Value to write.</param>
	/// <remarks>
	/// Writing to seconds register resets the sub-second counter.
	/// </remarks>

	void Write(uint16_t addr, uint8_t value) {
		UpdateTime();

		switch (addr) {
			case 0x08:
				// Seconds - also resets sub-second counter
				_regs[0] = value & 0x3F;
				_lastMasterClock = _emu->GetMasterClock();
				_tickCounter = 0;
				break;

			case 0x09:
				// Minutes
				_regs[1] = value & 0x3F;
				break;
			case 0x0A:
				// Hours
				_regs[2] = value & 0x1F;
				break;
			case 0x0B:
				// Day counter low byte
				_regs[3] = value;
				break;
			case 0x0C:
				// Day counter high bit + halt + overflow flags
				_regs[4] = value & 0xC1;
				break;
		}
	}

	/// <summary>Serializes RTC state for save states.</summary>
	/// <param name="s">Serializer instance.</param>
	void Serialize(Serializer& s) {
		SVArray(_regs, 5);
		SVArray(_latchedRegs, 5);
		SV(_latchValue);
		SV(_lastMasterClock);
		SV(_tickCounter);
	}
};
