#pragma once
#include "pch.h"

class LynxMikey;

/// <summary>
/// Virtual ComLynx cable — connects multiple LynxMikey UART instances for
/// inter-unit serial communication.
///
/// ComLynx (§11) is a shared open-collector bus: all transmitted data is
/// received by every connected unit. The sender always gets self-loopback
/// via its own ComLynxTxLoopback() (front-insert into RX queue for collision
/// detection priority per §7.2). This cable handles the cross-unit broadcast:
/// when a unit transmits, Broadcast() calls ComLynxRxData() on every other
/// connected unit (back-insert).
///
/// Design notes:
/// - Single-threaded: all operations assumed on emulation thread
/// - Not serialized: cable is runtime wiring, not save state data
/// - Null-safe: LynxMikey works unchanged when _comLynxCable == nullptr
/// - Supports 2–18 units (real hardware limit is ~18 due to cable loading)
/// </summary>
class ComLynxCable final {
private:
	/// <summary>Connected Mikey units. Pointer stability guaranteed by
	/// LynxConsole owning LynxMikey with unique_ptr (no reallocation).</summary>
	std::vector<LynxMikey*> _connectedUnits;

public:
	ComLynxCable() = default;
	~ComLynxCable() = default;

	// Non-copyable, non-movable (owns runtime wiring)
	ComLynxCable(const ComLynxCable&) = delete;
	ComLynxCable& operator=(const ComLynxCable&) = delete;
	ComLynxCable(ComLynxCable&&) = delete;
	ComLynxCable& operator=(ComLynxCable&&) = delete;

	/// <summary>Connect a Mikey unit to the cable.
	/// Duplicate connections are silently ignored.</summary>
	void Connect(LynxMikey* unit);

	/// <summary>Disconnect a Mikey unit from the cable.
	/// No-op if unit was not connected.</summary>
	void Disconnect(LynxMikey* unit);

	/// <summary>Disconnect all units from the cable.</summary>
	void DisconnectAll();

	/// <summary>Broadcast transmitted data to all connected units except the sender.
	/// Called from LynxMikey::ComLynxTxLoopback() after self-loopback.
	/// Each recipient receives data via ComLynxRxData() (back-insert into RX queue).</summary>
	void Broadcast(LynxMikey* sender, uint16_t data);

	/// <summary>Number of currently connected units.</summary>
	[[nodiscard]] size_t GetConnectedCount() const { return _connectedUnits.size(); }

	/// <summary>Check if a specific unit is connected.</summary>
	[[nodiscard]] bool IsConnected(const LynxMikey* unit) const;
};
