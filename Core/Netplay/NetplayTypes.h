#pragma once
#include "pch.h"
#include "Shared/SettingTypes.h"

/// <summary>
/// Network play controller port identifier.
/// </summary>
/// <remarks>
/// Addressing scheme:
/// - Port 0-7: Virtual controller ports (8 ports × 5 subports = 40 total slots)
/// - SubPort 0-4: Subport within port (for multitaps/4-player adapters)
///
/// Examples:
/// - Port 0, SubPort 0: Player 1 controller (standard controller)
/// - Port 1, SubPort 0: Player 2 controller
/// - Port 0, SubPort 1: Player 2 on multitap (SNES Super Multitap port 1B)
/// - Port 0xFF, SubPort 0: Spectator (no controller access)
///
/// Platform mappings:
/// - NES: Ports 0-1 (2 standard controllers)
/// - SNES: Ports 0-1, with SubPorts 0-4 for multitap (up to 5 players per port)
/// - NES Four Score: Ports 0-1 with SubPorts 0-1 (4 controllers)
/// </remarks>
struct NetplayControllerInfo {
	uint8_t Port = 0;    ///< Controller port (0-7, or 0xFF for spectator)
	uint8_t SubPort = 0; ///< Subport within port (0-4 for multitap)
};

/// <summary>
/// Controller usage information for netplay lobby.
/// </summary>
/// <remarks>
/// Used for:
/// - Displaying available controller slots to clients
/// - Preventing controller conflicts (one player per slot)
/// - Controller type negotiation (standard/turbo/zapper/etc.)
/// </remarks>
struct NetplayControllerUsageInfo {
	NetplayControllerInfo Port = {};            ///< Port identifier
	ControllerType Type = ControllerType::None; ///< Controller type at this port
	bool InUse = false;                         ///< True if slot occupied by player
};

/// <summary>
/// Player information in netplay session.
/// </summary>
struct PlayerInfo {
	NetplayControllerInfo ControllerPort = {}; ///< Assigned controller port
	bool IsHost = false;                       ///< True if server host
};
