#pragma once
#include "pch.h"
#include "Netplay/MessageType.h"
#include "Shared/SaveStateManager.h"
#include "Utilities/Socket.h"
#include "Utilities/Serializer.h"

/// <summary>
/// Base class for all network play messages.
/// Implements serialization and socket transmission.
/// </summary>
/// <remarks>
/// Message lifecycle:
/// 1. Construction: Create message object (e.g., new InputDataMessage())
/// 2. Populate fields (e.g., msg->SetInput(inputState))
/// 3. Send(socket): Serialize and transmit over TCP
/// 4. Receiver: Reconstruct from buffer (NetMessage(buffer, length))
/// 5. Initialize(): Deserialize fields from buffer
/// 6. ProcessMessage(): Handle message in subclass
///
/// Serialization format:
/// - Uses Serializer class (same as save states)
/// - Includes version number for compatibility
/// - Binary format with variable-length encoding
///
/// Derived message types:
/// - HandShakeMessage: Client authentication
/// - InputDataMessage: Controller input state
/// - MovieDataMessage: Broadcast input frame
/// - SaveStateMessage: Full emulator state
/// - GameInformationMessage: ROM metadata
/// - PlayerListMessage: Connected players
/// - SelectControllerMessage: Port selection
/// - ForceDisconnectMessage: Disconnect reason
/// - ServerInformationMessage: Server metadata
/// </remarks>
class NetMessage {
protected:
	MessageType _type;          ///< Message type identifier
	stringstream _receivedData; ///< Deserialization buffer

	/// <summary>
	/// Constructor for sending messages.
	/// </summary>
	/// <param name="type">Message type</param>
	NetMessage(MessageType type) {
		_type = type;
	}

	/// <summary>
	/// Constructor for receiving messages.
	/// </summary>
	/// <param name="buffer">Received message buffer</param>
	/// <param name="length">Buffer length in bytes</param>
	/// <remarks>
	/// Buffer format:
	/// - Byte 0: MessageType enum
	/// - Bytes 1-N: Serialized message data
	/// </remarks>
	NetMessage(void* buffer, uint32_t length) {
		_type = (MessageType)((uint8_t*)buffer)[0];
		_receivedData.write((char*)buffer + 1, length - 1);
	}

public:
	virtual ~NetMessage() {
	}

	/// <summary>
	/// Initialize message from received data.
	/// </summary>
	/// <remarks>
	/// Called after construction from buffer to deserialize fields.
	/// Uses Serializer to load data into message-specific fields.
	/// </remarks>
	void Initialize() {
		Serializer s(SaveStateManager::FileFormatVersion, false);
		if (s.LoadFrom(_receivedData)) {
			Serialize(s);
		}
	}

	/// <summary>
	/// Get message type.
	/// </summary>
	/// <returns>MessageType enum value</returns>
	MessageType GetType() {
		return _type;
	}

	/// <summary>
	/// Serialize and send message over socket.
	/// </summary>
	/// <param name="socket">TCP socket for transmission</param>
	/// <remarks>
	/// Wire format:
	/// - 4 bytes: uint32_t message length (including type byte)
	/// - 1 byte: MessageType
	/// - N bytes: Serialized message data
	///
	/// Blocking send:
	/// - May block if socket send buffer is full
	/// - Typically < 1ms for small messages (input data)
	/// - May take 100ms+ for large messages (save states)
	/// </remarks>
	void Send(Socket& socket) {
		Serializer s(SaveStateManager::FileFormatVersion, true);
		Serialize(s);

		stringstream out;
		s.SaveTo(out);

		string data = out.str();
		uint32_t messageLength = (uint32_t)data.size() + 1;
		data = string((char*)&messageLength, 4) + (char)_type + data;
		socket.Send((char*)data.c_str(), (int)data.size(), 0);
	}

protected:
	/// <summary>
	/// Serialize/deserialize message fields.
	/// </summary>
	/// <param name="s">Serializer instance (save or load mode)</param>
	/// <remarks>
	/// Subclasses must implement to handle message-specific fields.
	/// Example:
	/// void InputDataMessage::Serialize(Serializer& s) {
	///     SV(_inputState);
	/// }
	/// </remarks>
	virtual void Serialize(Serializer& s) = 0;
};
