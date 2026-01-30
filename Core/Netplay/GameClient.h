#pragma once
#include "pch.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Netplay/NetplayTypes.h"

class Socket;
class GameClientConnection;
class ClientConnectionData;
class Emulator;

/// <summary>
/// Network play client - connects to GameServer for multiplayer sessions.
/// Sends local input to server and receives game state/other players' input.
/// </summary>
/// <remarks>
/// Architecture:
/// - Single GameClientConnection manages server communication
/// - Dedicated client thread for network I/O
/// - Local input sent to server every frame
/// - Server broadcasts all inputs back to client
///
/// Synchronization:
/// - Client runs in lockstep with server
/// - Server dictates frame advancement (client can't run ahead)
/// - Input lag compensation handled by server
/// - Save states synchronized for late joins
///
/// Controller management:
/// - Client selects one controller port on connection
/// - Server assigns available port (first-come-first-served)
/// - Client can change controller port (if available)
/// - Server validates controller selections
///
/// Connection lifecycle:
/// 1. Connect() starts client thread
/// 2. Thread connects to server socket
/// 3. Sends password authentication
/// 4. Requests controller port
/// 5. Receives player list and game state
/// 6. Sends input every frame
/// 7. Disconnect() closes connection and stops thread
///
/// Thread safety:
/// - Atomic _connected flag for connection state
/// - Atomic _stop flag for thread shutdown
/// - enable_shared_from_this for safe callback registration
/// - GameClientConnection uses socket synchronization
/// </remarks>
class GameClient : public INotificationListener, public std::enable_shared_from_this<GameClient> {
private:
	Emulator* _emu;
	unique_ptr<thread> _clientThread;
	unique_ptr<GameClientConnection> _connection;

	atomic<bool> _stop;
	atomic<bool> _connected;

	void Exec();

public:
	GameClient(Emulator* emu);
	virtual ~GameClient();

	bool Connected();
	void Connect(ClientConnectionData& connectionData);
	void Disconnect();

	void SelectController(NetplayControllerInfo controller);
	NetplayControllerInfo GetControllerPort();
	vector<NetplayControllerUsageInfo> GetControllerList();

	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;
};