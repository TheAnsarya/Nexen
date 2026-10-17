#pragma once
#include "pch.h"

/// <summary>
/// Network play message types for client-server protocol.
/// </summary>
/// <remarks>
/// Protocol flow:
/// 1. Client → Server: HandShake (password, version, player name)
/// 2. Server → Client: ServerInformation (ROM CRC, settings, player list)
/// 3. Server → Client: SaveState (if late-join, sync to current game state)
/// 4. Client → Server: SelectController (choose controller port)
/// 5. Gameplay:
///    - Client → Server: InputData (every frame)
///    - Server → All Clients: MovieData (broadcast all inputs every frame)
/// 6. Server → Client: GameInformation (on ROM change/reset)
/// 7. Server → Client: ForceDisconnect (kick/ban player)
///
/// Message format:
/// - 4 bytes: Message length (uint32_t)
/// - 1 byte: MessageType enum
/// - N bytes: Serialized message data (Serializer format)
/// </remarks>
enum class MessageType : uint8_t {
	HandShake = 0,        ///< Client authentication (password, version, name)
	SaveState = 1,        ///< Full save state for late-join sync
	InputData = 2,        ///< Client input state (controller buttons)
	MovieData = 3,        ///< Server broadcast of all inputs (movie frame)
	GameInformation = 4,  ///< ROM info (CRC, region, settings)
	PlayerList = 5,       ///< Connected player list update
	SelectController = 6, ///< Controller port selection request
	ForceDisconnect = 7,  ///< Server disconnect command (kick/ban)
	ServerInformation = 8 ///< Server info (name, version, password required)
};