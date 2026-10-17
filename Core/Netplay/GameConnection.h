#pragma once
#include "pch.h"
#include "Utilities/SimpleLock.h"

class Socket;
class NetMessage;
class Emulator;

/// <summary>
/// Base class for netplay TCP connections (client and server sides).
/// Handles socket I/O, message framing, and error detection.
/// </summary>
/// <remarks>
/// Architecture:
/// - GameServerConnection: Server-side per-client connection
/// - GameClientConnection: Client-side server connection
///
/// Message framing protocol:
/// 1. Read 4-byte message length prefix
/// 2. Read N bytes of message data
/// 3. Parse MessageType from first byte
/// 4. Deserialize message body
/// 5. Dispatch to ProcessMessage()
///
/// Thread model:
/// - ProcessMessages() called from connection thread (server/client threads)
/// - SendNetMessage() may be called from emulation thread
/// - Socket operations protected by _socketLock
///
/// Error handling:
/// - ConnectionError() detects socket failures
/// - Disconnect() closes socket and cleans up
/// - Derived classes handle reconnection logic
/// </remarks>
class GameConnection {
protected:
	static constexpr int MaxMsgLength = 1500000; ///< Max message size (1.5MB for save states)

	unique_ptr<Socket> _socket; ///< TCP socket for communication
	Emulator* _emu;             ///< Emulator instance reference

	uint8_t _readBuffer[GameConnection::MaxMsgLength] = {};    ///< Socket read buffer
	uint8_t _messageBuffer[GameConnection::MaxMsgLength] = {}; ///< Message assembly buffer
	int _readPosition = 0;                                     ///< Current read position in buffer
	SimpleLock _socketLock;                                    ///< Socket operation synchronization

private:
	/// <summary>
	/// Read available data from socket into buffer.
	/// </summary>
	/// <remarks>
	/// Non-blocking read:
	/// - Reads up to MaxMsgLength bytes
	/// - Returns immediately if no data available
	/// - Accumulates partial messages across calls
	/// </remarks>
	void ReadSocket();

	/// <summary>
	/// Extract complete message from read buffer.
	/// </summary>
	/// <param name="buffer">Output message buffer</param>
	/// <param name="messageLength">Output message length</param>
	/// <returns>True if complete message extracted</returns>
	/// <remarks>
	/// Message framing:
	/// - First 4 bytes: uint32_t message length
	/// - Remaining bytes: Message type + data
	/// - Handles partial reads (incomplete messages)
	/// </remarks>
	bool ExtractMessage(void* buffer, uint32_t& messageLength);

	/// <summary>
	/// Read and parse next message from buffer.
	/// </summary>
	/// <returns>Parsed message object or nullptr if no complete message</returns>
	/// <remarks>
	/// Message construction:
	/// - Reads MessageType from first byte
	/// - Allocates appropriate message subclass
	/// - Calls Initialize() to deserialize fields
	/// </remarks>
	NetMessage* ReadMessage();

	/// <summary>
	/// Process received message (implemented by derived classes).
	/// </summary>
	/// <param name="message">Parsed message object</param>
	virtual void ProcessMessage(NetMessage* message) = 0;

protected:
	/// <summary>
	/// Disconnect socket and mark connection as closed.
	/// </summary>
	void Disconnect();

public:
	static constexpr uint8_t SpectatorPort = 0xFF; ///< Special port number for spectators

	/// <summary>
	/// Constructor for connection.
	/// </summary>
	/// <param name="emu">Emulator instance</param>
	/// <param name="socket">Connected TCP socket (ownership transferred)</param>
	GameConnection(Emulator* emu, unique_ptr<Socket> socket);
	virtual ~GameConnection();

	/// <summary>
	/// Check if connection has errors.
	/// </summary>
	/// <returns>True if socket error detected</returns>
	bool ConnectionError();

	/// <summary>
	/// Process pending messages from socket.
	/// </summary>
	/// <remarks>
	/// Called periodically from connection thread:
	/// - Reads socket data
	/// - Extracts complete messages
	/// - Dispatches to ProcessMessage()
	/// - Repeats until no more complete messages
	/// </remarks>
	void ProcessMessages();

	/// <summary>
	/// Send message over connection.
	/// </summary>
	/// <param name="message">Message to send</param>
	/// <remarks>
	/// Thread-safe send:
	/// - Locks socket during transmission
	/// - Serializes message to wire format
	/// - Blocks until sent or error
	/// </remarks>
	void SendNetMessage(NetMessage& message);
};