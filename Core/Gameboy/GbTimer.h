#pragma once
#include "pch.h"
#include "Utilities/ISerializable.h"
#include "Utilities/Serializer.h"
#include "Gameboy/GbTypes.h"

class GbMemoryManager;
class GbApu;

/// <summary>
/// Game Boy hardware timer implementation.
/// Handles DIV ($FF04), TIMA ($FF05), TMA ($FF06), and TAC ($FF07) registers.
/// The timer also drives the APU frame sequencer via the DIV register.
/// </summary>
class GbTimer : public ISerializable {
private:
	/// <summary>Memory manager reference for bus access.</summary>
	GbMemoryManager* _memoryManager = nullptr;

	/// <summary>APU reference for frame sequencer clocking.</summary>
	GbApu* _apu = nullptr;

	/// <summary>Timer state including DIV, TIMA, TMA, TAC values.</summary>
	GbTimerState _state = {};

	/// <summary>
	/// Updates the 16-bit internal divider and checks for timer/frame sequencer triggers.
	/// TIMA increments on the falling edge of the selected divider bit.
	/// </summary>
	/// <param name="value">New divider value.</param>
	void SetDivider(uint16_t value);

	/// <summary>
	/// Reloads TIMA from TMA after overflow.
	/// There is a 4-cycle delay before the reload takes effect.
	/// </summary>
	void ReloadCounter();

public:
	virtual ~GbTimer();

	/// <summary>
	/// Initializes the timer with memory manager and APU references.
	/// Sets up default divider value and timer divider mask.
	/// </summary>
	/// <param name="memoryManager">Memory manager for bus access.</param>
	/// <param name="apu">APU for frame sequencer clocking.</param>
	void Init(GbMemoryManager* memoryManager, GbApu* apu);

	/// <summary>Gets the current timer state for debugging/serialization.</summary>
	/// <returns>Copy of the current timer state.</returns>
	GbTimerState GetState();

	/// <summary>
	/// Executes one timer cycle.
	/// Increments DIV, checks for TIMA overflow, and handles reload delay.
	/// </summary>
	void Exec();

	/// <summary>
	/// Checks if the frame sequencer bit is set in the divider.
	/// Used for APU synchronization.
	/// </summary>
	/// <returns>True if frame sequencer bit is set.</returns>
	bool IsFrameSequencerBitSet();

	/// <summary>
	/// Reads a timer register.
	/// </summary>
	/// <param name="addr">Address ($FF04-$FF07).</param>
	/// <returns>Register value.</returns>
	uint8_t Read(uint16_t addr);

	/// <summary>
	/// Writes to a timer register.
	/// Writing to $FF04 (DIV) resets it to 0.
	/// </summary>
	/// <param name="addr">Address ($FF04-$FF07).</param>
	/// <param name="value">Value to write.</param>
	void Write(uint16_t addr, uint8_t value);

	/// <summary>Serializes timer state for save states.</summary>
	void Serialize(Serializer& s) override;
};