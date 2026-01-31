#pragma once
#include "pch.h"
#include "PCE/PceTypes.h"
#include "Utilities/ISerializable.h"

class PceConsole;

/// <summary>
/// PC Engine hardware timer - HuC6280 integrated.
/// 7-bit countdown timer with IRQ generation.
/// </summary>
/// <remarks>
/// The HuC6280 includes a simple programmable timer:
/// - 7-bit counter (0-127)
/// - Countdown at ~7.16 kHz (1024 CPU clock divider)
/// - Generates Timer IRQ when counter reaches 0
/// - Reloads from latch value on underflow
///
/// **Registers ($0C00-$0C01 in bank $FF):**
/// - $0C00: Timer latch (write) / counter (read)
/// - $0C01: Timer enable (bit 0)
///
/// **Timing:**
/// - Clocked every 1024 master clock cycles
/// - One tick = ~143 microseconds at 7.16 MHz
/// - Maximum period: 127 × 143μs ≈ 18.2ms
///
/// **Common Uses:**
/// - CD-ROM timing
/// - Music tempo
/// - Game timing events
/// </remarks>
class PceTimer final : public ISerializable {
private:
	/// <summary>Timer state (counter, latch, enable).</summary>
	PceTimerState _state = {};

	/// <summary>Console instance for IRQ signaling.</summary>
	PceConsole* _console = nullptr;

public:
	/// <summary>Constructs timer with console reference.</summary>
	PceTimer(PceConsole* console);

	/// <summary>Gets timer state reference.</summary>
	PceTimerState& GetState() { return _state; }

	/// <summary>Executes one timer tick.</summary>
	void Exec();

	/// <summary>Writes to timer register.</summary>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Reads from timer register.</summary>
	uint8_t Read(uint16_t addr);

	/// <summary>Serializes timer state for save states.</summary>
	void Serialize(Serializer& s) override;
};
