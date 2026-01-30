#pragma once
#include "pch.h"
#include <thread>
#include "Netplay/GameServerConnection.h"
#include "Netplay/NetplayTypes.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/Interfaces/IInputProvider.h"
#include "Shared/Interfaces/IInputRecorder.h"
#include "Shared/IControllerHub.h"

class Emulator;

/// <summary>
/// Network play server - hosts multiplayer game sessions.
/// Coordinates input from multiple clients and broadcasts game state.
/// </summary>
/// <remarks>
/// Architecture:
/// - Dedicated accept thread for incoming connections
/// - Per-client GameServerConnection (one thread per client)
/// - Host player uses local input (IInputProvider)
/// - Remote clients send input over TCP (IInputRecorder broadcasts)
///
/// Synchronization:
/// - All clients must send input for current frame before advancing
/// - Server waits for all inputs before running frame
/// - Input lag compensation for network latency
/// - Deterministic replay ensures perfect sync
///
/// Controller management:
/// - Up to 8 virtual ports (4 standard + 4 expansion)
/// - Each port can have up to 5 subports (multitap support)
/// - Clients can claim any unclaimed controller port
/// - Host has priority for port selection
///
/// Connection lifecycle:
/// 1. StartServer() opens listening socket
/// 2. AcceptConnections() waits for clients
/// 3. Client connects, sends password, selects controller
/// 4. Server adds client to _openConnections
/// 5. Client sends input every frame
/// 6. StopServer() closes all connections
///
/// Thread safety:
/// - _openConnections protected by implicit synchronization (single thread access)
/// - Atomic _stop flag for thread shutdown
/// - enable_shared_from_this for safe callback registration
/// </remarks>
class GameServer : public IInputRecorder, public IInputProvider, public INotificationListener, public std::enable_shared_from_this<GameServer> {
private:
	Emulator* _emu;
	unique_ptr<thread> _serverThread;
	unique_ptr<Socket> _listener;
	atomic<bool> _stop;
	uint16_t _port = 0;
	string _password;
	vector<unique_ptr<GameServerConnection>> _openConnections;
	bool _initialized = false;

	GameServerConnection* _netPlayDevices[BaseControlDevice::PortCount][IControllerHub::MaxSubPorts] = {};

	NetplayControllerInfo _hostControllerPort = {};

	void AcceptConnections();
	void UpdateConnections();

	void Exec();

public:
	GameServer(Emulator* emu);
	virtual ~GameServer();

	void RegisterServerInput();

	void StartServer(uint16_t port, string password);
	void StopServer();
	bool Started();

	NetplayControllerInfo GetHostControllerPort();
	void SetHostControllerPort(NetplayControllerInfo controller);
	vector<NetplayControllerUsageInfo> GetControllerList();
	vector<PlayerInfo> GetPlayerList();
	void SendPlayerList();

	static vector<NetplayControllerUsageInfo> GetControllerList(Emulator* emu, vector<PlayerInfo>& players);

	bool SetInput(BaseControlDevice* device) override;
	void RecordInput(vector<shared_ptr<BaseControlDevice>> devices) override;

	// Inherited via INotificationListener
	virtual void ProcessNotification(ConsoleNotificationType type, void* parameter) override;

	void RegisterNetPlayDevice(GameServerConnection* connection, NetplayControllerInfo controller);
	void UnregisterNetPlayDevice(GameServerConnection* device);
	NetplayControllerInfo GetFirstFreeControllerPort();
	GameServerConnection* GetNetPlayDevice(NetplayControllerInfo controller);
};