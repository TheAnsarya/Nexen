#pragma once
#include "pch.h"
#include <deque>
#include "Netplay/GameConnection.h"
#include "Netplay/NetplayTypes.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/ControlDeviceState.h"
#include "Utilities/SimpleLock.h"

class HandShakeMessage;
class GameServer;

/// <summary>
/// Server-side netplay client connection handler.
/// Manages single client connection, input collection, and state synchronization.
/// </summary>
/// <remarks>
/// Architecture:
/// - One GameServerConnection per connected client
/// - GameServer owns collection of connections
/// - Each connection runs on dedicated thread (managed by GameServer)
///
/// Responsibilities:
/// - Client authentication (password verification)
/// - Controller port assignment/management
/// - Input state collection from client
/// - Broadcasting movie data to client
/// - Configuration change notifications
/// - Disconnect handling (kick/ban)
///
/// Input flow:
/// 1. Client sends InputDataMessage every frame
/// 2. PushState() stores input in _inputData
/// 3. GameServer calls GetState() to collect input
/// 4. Server broadcasts collected inputs via SendMovieData()
///
/// Authentication:
/// 1. Client connects, sends HandShakeMessage
/// 2. ProcessHandshakeResponse() verifies password hash
/// 3. SendServerInformation() sends ROM/settings/player list
/// 4. SendGameInformation() sends current game state
/// 5. Client selects controller port via SelectControllerMessage
///
/// Thread model:
/// - ProcessMessages() called from client thread (GameServer manages threads)
/// - SendMovieData() called from emulation thread (GameServer::RecordInput)
/// - Input access protected by _inputLock
/// </remarks>
class GameServerConnection final : public GameConnection, public INotificationListener {
private:
	GameServer* _server = nullptr; ///< Parent server instance

	SimpleLock _inputLock;              ///< Input state synchronization
	ControlDeviceState _inputData = {}; ///< Current frame input from client

	string _previousConfig = ""; ///< Last configuration hash (detect changes)

	NetplayControllerInfo _controllerPort = {}; ///< Assigned controller port
	string _connectionHash;                     ///< Client authentication hash
	string _serverPassword;                     ///< Server password (hashed)
	bool _handshakeCompleted = false;           ///< True after successful authentication

	/// <summary>
	/// Store input state from client.
	/// </summary>
	/// <param name="state">Controller state for current frame</param>
	void PushState(ControlDeviceState state);

	/// <summary>
	/// Send server metadata to client.
	/// </summary>
	/// <remarks>
	/// Sends ServerInformationMessage containing:
	/// - Server name and version
	/// - Password requirement flag
	/// - Max player count
	/// </remarks>
	void SendServerInformation();

	/// <summary>
	/// Send game ROM information to client.
	/// </summary>
	/// <remarks>
	/// Sends GameInformationMessage containing:
	/// - ROM CRC32 hash
	/// - ROM filename
	/// - Region (NTSC/PAL)
	/// - Emulator settings (for sync)
	/// </remarks>
	void SendGameInformation();

	/// <summary>
	/// Assign controller port to client.
	/// </summary>
	/// <param name="port">Requested port (must be available)</param>
	/// <remarks>
	/// Port assignment:
	/// - Check if port available (not in use by other client)
	/// - Assign port to client
	/// - Notify all clients of player list change
	/// - Spectator mode if port is SpectatorPort (0xFF)
	/// </remarks>
	void SelectControllerPort(NetplayControllerInfo port);

	/// <summary>
	/// Force disconnect client with reason.
	/// </summary>
	/// <param name="disconnectMessage">Reason for disconnect (shown to client)</param>
	/// <remarks>
	/// Disconnect reasons:
	/// - Wrong password
	/// - Incompatible version
	/// - Kicked by host
	/// - Server shutdown
	/// </remarks>
	void SendForceDisconnectMessage(string disconnectMessage);

	/// <summary>
	/// Process client handshake response.
	/// </summary>
	/// <param name="message">Handshake message from client</param>
	/// <remarks>
	/// Handshake validation:
	/// - Verify protocol version
	/// - Check password hash
	/// - Check player name uniqueness
	/// - Send server/game info or disconnect
	/// </remarks>
	void ProcessHandshakeResponse(HandShakeMessage* message);

protected:
	/// <summary>
	/// Process received message from client.
	/// </summary>
	/// <param name="message">Parsed message object</param>
	/// <remarks>
	/// Message handling:
	/// - HandShake: Authenticate client
	/// - InputData: Store input for current frame
	/// - SelectController: Assign controller port
	/// </remarks>
	void ProcessMessage(NetMessage* message) override;

public:
	/// <summary>
	/// Constructor for server-side client connection.
	/// </summary>
	/// <param name="gameServer">Parent GameServer instance</param>
	/// <param name="emu">Emulator instance</param>
	/// <param name="socket">Connected client socket</param>
	/// <param name="serverPassword">Server password hash</param>
	GameServerConnection(GameServer* gameServer, Emulator* emu, unique_ptr<Socket> socket, string serverPassword);
	virtual ~GameServerConnection();

	/// <summary>
	/// Get current frame input state from client.
	/// </summary>
	/// <returns>Controller input state</returns>
	/// <remarks>
	/// Called by GameServer::RecordInput() every frame.
	/// Thread-safe (protected by _inputLock).
	/// </remarks>
	ControlDeviceState GetState();

	/// <summary>
	/// Send movie data frame to client.
	/// </summary>
	/// <param name="port">Controller port</param>
	/// <param name="state">Input state for this port</param>
	/// <remarks>
	/// Called by GameServer::RecordInput() to broadcast inputs.
	/// Sends MovieDataMessage containing all player inputs for frame.
	/// </remarks>
	void SendMovieData(uint8_t port, ControlDeviceState state);

	/// <summary>
	/// Get assigned controller port.
	/// </summary>
	/// <returns>Port info (or SpectatorPort if spectating)</returns>
	NetplayControllerInfo GetControllerPort();

	/// <summary>
	/// Process console notification events.
	/// </summary>
	/// <param name="type">Notification type</param>
	/// <param name="parameter">Type-specific parameter</param>
	/// <remarks>
	/// Handled events:
	/// - GameLoaded: Send game info to client
	/// - ConfigChanged: Send updated settings
	/// - GameReset: Notify client of reset
	/// </remarks>
	virtual void ProcessNotification(ConsoleNotificationType type, void* parameter) override;
};
