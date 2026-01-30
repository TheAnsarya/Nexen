#pragma once
#include "pch.h"
#include <deque>
#include "Utilities/AutoResetEvent.h"
#include "Utilities/SimpleLock.h"
#include "Shared/BaseControlDevice.h"
#include "Shared/Interfaces/INotificationListener.h"
#include "Shared/Interfaces/IInputProvider.h"
#include "Shared/ControlDeviceState.h"
#include "Netplay/GameConnection.h"
#include "Netplay/ClientConnectionData.h"
#include "Netplay/NetplayTypes.h"

class Emulator;

/// <summary>
/// Client-side netplay server connection handler.
/// Manages server connection, input synchronization, and controller emulation.
/// </summary>
/// <remarks>
/// Architecture:
/// - GameClient owns single GameClientConnection instance
/// - Dedicated client thread for message processing
/// - Implements IInputProvider to override local controller input
///
/// Responsibilities:
/// - Server connection and authentication
/// - Controller port selection
/// - Local input capture and transmission
/// - Server input reception and buffering
/// - Input synchronization (buffer management)
/// - ROM loading/matching with server
/// - Late-join via save state
///
/// Input synchronization:
/// 1. Client reads local input (keyboard/gamepad)
/// 2. SendInput() transmits to server every frame
/// 3. Server broadcasts MovieDataMessage with all inputs
/// 4. PushControllerState() buffers inputs in deque
/// 5. SetInput() provides buffered input to emulation
/// 6. Buffer size maintained at _minimumQueueSize frames (lag compensation)
///
/// Connection lifecycle:
/// 1. Construction: Connect to server, send handshake
/// 2. Authentication: Receive server/game info, verify ROM
/// 3. Late-join: Receive save state if game in progress
/// 4. Controller selection: Choose available port
/// 5. Gameplay: Send/receive input every frame
/// 6. Shutdown: Disconnect and cleanup
///
/// Thread model:
/// - ProcessMessages() called from client thread (GameClient manages thread)
/// - SetInput() called from emulation thread (60 FPS)
/// - SendInput() called from emulation thread (60 FPS)
/// - Input deques protected by atomic counters and AutoResetEvents
/// </remarks>
class GameClientConnection final : public GameConnection, public INotificationListener, public IInputProvider {
private:
	std::deque<ControlDeviceState> _inputData[BaseControlDevice::PortCount]; ///< Input buffer queues (40 ports)
	atomic<uint32_t> _inputSize[BaseControlDevice::PortCount];               ///< Atomic queue sizes
	AutoResetEvent _waitForInput[BaseControlDevice::PortCount];              ///< Wait for input availability
	SimpleLock _writeLock;                                                   ///< Input queue write lock
	atomic<bool> _shutdown;                                                  ///< Shutdown flag
	atomic<bool> _enableControllers;                                         ///< Controllers enabled (not spectating)
	atomic<uint32_t> _minimumQueueSize;                                      ///< Minimum buffer size (lag compensation)

	vector<PlayerInfo> _playerList; ///< Connected player list from server

	shared_ptr<BaseControlDevice> _controlDevice;                               ///< Local controller device
	atomic<ControllerType> _controllerType;                                     ///< Current controller type
	ControlDeviceState _lastInputSent = {};                                     ///< Last input sent to server (delta compression)
	bool _gameLoaded = false;                                                   ///< True if ROM loaded and emulation running
	NetplayControllerInfo _controllerPort = {GameConnection::SpectatorPort, 0}; ///< Assigned port
	ClientConnectionData _connectionData = {};                                  ///< Connection parameters (host, port, password, name)
	string _serverSalt;                                                         ///< Authentication salt from server

private:
	/// <summary>
	/// Send handshake message to server.
	/// </summary>
	/// <remarks>
	/// Handshake contains:
	/// - Protocol version
	/// - Password hash (salted)
	/// - Player name
	/// - Client capabilities
	/// </remarks>
	void SendHandshake();

	/// <summary>
	/// Request controller port assignment from server.
	/// </summary>
	/// <param name="controller">Desired controller port</param>
	void SendControllerSelection(NetplayControllerInfo controller);

	/// <summary>
	/// Clear all input buffer queues.
	/// </summary>
	/// <remarks>
	/// Called on:
	/// - Game load/reset
	/// - Controller port change
	/// - Disconnect/reconnect
	/// </remarks>
	void ClearInputData();

	/// <summary>
	/// Add input state to buffer queue.
	/// </summary>
	/// <param name="port">Controller port number</param>
	/// <param name="state">Input state for frame</param>
	/// <remarks>
	/// Called when MovieDataMessage received from server.
	/// Adds to deque, signals _waitForInput event.
	/// </remarks>
	void PushControllerState(uint8_t port, ControlDeviceState state);

	/// <summary>
	/// Disable controller input (switch to spectator mode).
	/// </summary>
	void DisableControllers();

	/// <summary>
	/// Attempt to load ROM matching server.
	/// </summary>
	/// <param name="filename">ROM filename from server</param>
	/// <param name="crc32">Expected CRC32 hash</param>
	/// <returns>True if ROM loaded and hash matches</returns>
	/// <remarks>
	/// ROM loading strategy:
	/// 1. Try exact filename in ROM paths
	/// 2. Search for matching CRC32 in ROM directories
	/// 3. Prompt user for ROM location
	/// 4. Fail if no match found
	/// </remarks>
	bool AttemptLoadGame(string filename, uint32_t crc32);

protected:
	/// <summary>
	/// Process received message from server.
	/// </summary>
	/// <param name="message">Parsed message object</param>
	/// <remarks>
	/// Message handling:
	/// - ServerInformation: Store server metadata
	/// - GameInformation: Load matching ROM
	/// - SaveState: Apply save state for late-join
	/// - MovieData: Buffer input for emulation
	/// - PlayerList: Update connected players
	/// - ForceDisconnect: Display reason and disconnect
	/// </remarks>
	void ProcessMessage(NetMessage* message) override;

public:
	/// <summary>
	/// Constructor for client connection.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="socket">Connected server socket</param>
	/// <param name="connectionData">Connection parameters (host, password, name)</param>
	GameClientConnection(Emulator* emu, unique_ptr<Socket> socket, ClientConnectionData& connectionData);
	virtual ~GameClientConnection();

	/// <summary>
	/// Shutdown connection gracefully.
	/// </summary>
	/// <remarks>
	/// Shutdown sequence:
	/// - Set _shutdown flag
	/// - Signal all _waitForInput events (unblock emulation)
	/// - Disconnect socket
	/// - Clean up resources
	/// </remarks>
	void Shutdown();

	/// <summary>
	/// Process console notification events.
	/// </summary>
	/// <param name="type">Notification type</param>
	/// <param name="parameter">Type-specific parameter</param>
	/// <remarks>
	/// Handled events:
	/// - GameLoaded: Clear input buffers, initialize controllers
	/// - GameReset: Clear input buffers
	/// </remarks>
	void ProcessNotification(ConsoleNotificationType type, void* parameter) override;

	/// <summary>
	/// Provide input to emulation (IInputProvider implementation).
	/// </summary>
	/// <param name="device">Target controller device</param>
	/// <returns>True if input provided, false if buffer empty</returns>
	/// <remarks>
	/// Input priority:
	/// - Netplay input (this method) overrides local input
	/// - Called every frame from emulation thread
	/// - Blocks if buffer empty (waits for MovieData from server)
	/// - Maintains buffer at _minimumQueueSize (lag compensation)
	///
	/// Buffer management:
	/// - Pop input from deque front
	/// - Decrement _inputSize counter
	/// - Return false if queue empty (block emulation)
	/// </remarks>
	bool SetInput(BaseControlDevice* device) override;

	/// <summary>
	/// Initialize local controller device.
	/// </summary>
	/// <remarks>
	/// Called after controller port selection.
	/// Creates BaseControlDevice for assigned port/type.
	/// </remarks>
	void InitControlDevice();

	/// <summary>
	/// Send local input to server.
	/// </summary>
	/// <remarks>
	/// Called every frame from emulation thread.
	/// Reads local controller device state.
	/// Sends InputDataMessage to server.
	/// </remarks>
	void SendInput();

	/// <summary>
	/// Select controller port.
	/// </summary>
	/// <param name="controller">Desired port (or SpectatorPort)</param>
	/// <remarks>
	/// Port selection:
	/// - Send SelectControllerMessage to server
	/// - Server validates availability
	/// - Server sends updated PlayerListMessage
	/// - Initialize local controller device
	/// </remarks>
	void SelectController(NetplayControllerInfo controller);

	/// <summary>
	/// Get list of available controller ports.
	/// </summary>
	/// <returns>Controller usage info for all ports</returns>
	/// <remarks>
	/// Used for controller selection UI (lobby).
	/// Shows which ports are occupied and available.
	/// </remarks>
	vector<NetplayControllerUsageInfo> GetControllerList();

	/// <summary>
	/// Get currently assigned controller port.
	/// </summary>
	/// <returns>Port info (or SpectatorPort if spectating)</returns>
	NetplayControllerInfo GetControllerPort();
};