#pragma once
#include "pch.h"
#include "GBA/GbaTypes.h"
#include "Utilities/ISerializable.h"

class GbaMemoryManager;
class GbaApu;

/// <summary>
/// GBA hardware timer implementation.
/// Provides 4 independent 16-bit timers with cascade support.
/// Timers 0 and 1 can drive Direct Sound audio DMA.
/// </summary>
class GbaTimer final : public ISerializable {
private:
	/// <summary>State for all 4 timers.</summary>
	GbaTimersState _state = {};

	/// <summary>Memory manager for IRQ signaling.</summary>
	GbaMemoryManager* _memoryManager = nullptr;

	/// <summary>APU for Direct Sound FIFO clocking.</summary>
	GbaApu* _apu = nullptr;

	/// <summary>Flag indicating pending timer start/stop operations.</summary>
	bool _hasPendingTimers = false;

	/// <summary>Flag indicating pending register writes.</summary>
	bool _hasPendingWrites = false;

	/// <summary>
	/// Clocks a timer overflow - reloads counter and cascades to next timer.
	/// Also triggers IRQ if enabled and clocks Direct Sound FIFO.
	/// </summary>
	/// <param name="i">Timer index (0-3).</param>
	void ClockTimer(int i);

	/// <summary>
	/// Template function to process a single timer each cycle.
	/// Checks prescaler mask and increments timer if enabled.
	/// </summary>
	/// <typeparam name="i">Timer index (0-3).</typeparam>
	/// <param name="masterClock">Current master clock value.</param>
	template <int i>
	__forceinline void ProcessTimer(uint64_t masterClock) {
		if (_state.Timer[i].ProcessTimer && (masterClock & _state.Timer[i].PrescaleMask) == 0) {
			if (++_state.Timer[i].Timer == 0) {
				ClockTimer(i);
			}
		}
	}

	/// <summary>
	/// Triggers a delayed update for timer enable/disable changes.
	/// </summary>
	/// <param name="timer">Timer state to update.</param>
	void TriggerUpdate(GbaTimerState& timer);

public:
	/// <summary>
	/// Initializes timers with memory manager and APU references.
	/// </summary>
	/// <param name="memoryManager">Memory manager for IRQ signaling.</param>
	/// <param name="apu">APU for Direct Sound FIFO clocking.</param>
	void Init(GbaMemoryManager* memoryManager, GbaApu* apu);

	/// <summary>Gets reference to timer state for direct access.</summary>
	/// <returns>Reference to timer state.</returns>
	GbaTimersState& GetState() { return _state; }

	/// <summary>
	/// Executes all 4 timers for one cycle.
	/// Called every master clock cycle.
	/// </summary>
	/// <param name="masterClock">Current master clock value.</param>
	__forceinline void Exec(uint64_t masterClock) {
		ProcessTimer<0>(masterClock);
		ProcessTimer<1>(masterClock);
		ProcessTimer<2>(masterClock);
		ProcessTimer<3>(masterClock);
	}

	/// <summary>
	/// Writes to a timer control register.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <param name="value">Value to write.</param>
	void WriteRegister(uint32_t addr, uint8_t value);

	/// <summary>
	/// Reads from a timer register.
	/// </summary>
	/// <param name="addr">Register address.</param>
	/// <returns>Register value.</returns>
	uint8_t ReadRegister(uint32_t addr);

	/// <summary>Checks if any timer has pending start/stop operations.</summary>
	/// <returns>True if pending timers exist.</returns>
	[[nodiscard]] bool HasPendingTimers() { return _hasPendingTimers; }

	/// <summary>Checks if any timer has pending register writes.</summary>
	/// <returns>True if pending writes exist.</returns>
	[[nodiscard]] bool HasPendingWrites() { return _hasPendingWrites; }

	/// <summary>Processes pending timer start/stop operations.</summary>
	void ProcessPendingTimers();

	/// <summary>Processes pending register writes.</summary>
	void ProcessPendingWrites();

	/// <summary>Serializes timer state for save states.</summary>
	void Serialize(Serializer& s) override;
};
