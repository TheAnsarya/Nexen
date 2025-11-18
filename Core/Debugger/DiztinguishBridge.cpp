#include "pch.h"
#include "DiztinguishBridge.h"
#include "SNES/SnesConsole.h"
#include "SNES/Debugger/SnesDebugger.h"
#include "SNES/Debugger/SnesCodeDataLogger.h"
#include "SNES/BaseCartridge.h"
#include "Utilities/CRC32.h"
#include <chrono>

using namespace DiztinguishProtocol;

DiztinguishBridge::DiztinguishBridge(SnesConsole* console, SnesDebugger* debugger)
	: _console(console)
	, _debugger(debugger)
	, _cart(nullptr)
	, _port(0)
	, _serverRunning(false)
	, _clientConnected(false)
	, _configReceived(false)
	, _currentFrame(0)
	, _lastTraceSentFrame(0)
	, _lastCdlSyncTime(0)
	, _messagesSent(0)
	, _messagesReceived(0)
	, _bytesSent(0)
	, _bytesReceived(0)
	, _connectionStartTime(0)
{
	// Initialize default configuration
	_config.enableExecTrace = 1;
	_config.enableMemoryAccess = 0;  // Disabled by default (high bandwidth)
	_config.enableCdlUpdates = 1;
	_config.traceFrameInterval = 4;  // Every 4 frames = 15 Hz
	_config.maxTracesPerFrame = 1000;

	_traceBuffer.reserve(1024);  // Pre-allocate
}

DiztinguishBridge::~DiztinguishBridge()
{
	StopServer();
}

bool DiztinguishBridge::StartServer(uint16_t port)
{
	if(_serverRunning) {
		return false;  // Already running
	}

	_port = port;
	_serverSocket = std::make_unique<Socket>();

	try {
		_serverSocket->Bind(_port);
		_serverSocket->Listen(1);  // Only accept 1 client
		_serverRunning = true;

		// Start server thread to accept connections
		_serverThread = std::make_unique<std::thread>(&DiztinguishBridge::ServerThreadMain, this);

		_debugger->Log("[DiztinGUIsh] Server started on port " + std::to_string(_port));
		return true;
	}
	catch(const std::exception& e) {
		_debugger->Log("[DiztinGUIsh] Failed to start server: " + std::string(e.what()));
		_serverSocket.reset();
		_serverRunning = false;
		return false;
	}
}

void DiztinguishBridge::StopServer()
{
	if(!_serverRunning) {
		return;
	}

	_serverRunning = false;
	_clientConnected = false;

	// Close sockets
	if(_clientSocket) {
		_clientSocket->Close();
		_clientSocket.reset();
	}
	if(_serverSocket) {
		_serverSocket->Close();
		_serverSocket.reset();
	}

	// Join threads
	if(_serverThread && _serverThread->joinable()) {
		_serverThread->join();
	}
	if(_receiveThread && _receiveThread->joinable()) {
		_receiveThread->join();
	}

	_debugger->Log("[DiztinGUIsh] Server stopped");
}

void DiztinguishBridge::ServerThreadMain()
{
	_debugger->Log("[DiztinGUIsh] Waiting for client connection...");

	while(_serverRunning) {
		try {
			// Accept incoming connection (blocking)
			_clientSocket = _serverSocket->Accept();

			if(_clientSocket && !_clientSocket->ConnectionError()) {
				_clientConnected = true;
				_connectionStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()
				).count();

				_debugger->Log("[DiztinGUIsh] Client connected!");

				// Send handshake
				SendHandshake();

				// Start receive thread
				_receiveThread = std::make_unique<std::thread>(&DiztinguishBridge::ReceiveThreadMain, this);

				// Wait for disconnect
				if(_receiveThread->joinable()) {
					_receiveThread->join();
				}

				_clientConnected = false;
				_configReceived = false;
				_traceBuffer.clear();
				_cdlDirty.clear();

				_debugger->Log("[DiztinGUIsh] Client disconnected");
			}
		}
		catch(const std::exception& e) {
		if(_serverRunning) {
			_debugger->Log("[DiztinGUIsh] Error in server thread: " + std::string(e.what()));
			}
		}
	}
}

void DiztinguishBridge::ReceiveThreadMain()
{
	char headerBuf[sizeof(MessageHeader)];

	while(_clientConnected && _serverRunning) {
		try {
			// Read message header (5 bytes)
			int received = _clientSocket->Recv(headerBuf, sizeof(MessageHeader), 0);
			if(received != sizeof(MessageHeader)) {
				// Connection closed or error
				break;
			}

			MessageHeader header;
			header.type = (MessageType)headerBuf[0];
			header.length = *(uint32_t*)(headerBuf + 1);

			// Validate length
			if(header.length > 1024 * 1024) {  // Max 1MB per message
				_debugger->Log("[DiztinGUIsh] Invalid message length: " + std::to_string(header.length));
				SendError(ErrorCode::InvalidMessage, "Message too large");
				break;
			}

			// Read payload
			std::vector<uint8_t> payload(header.length);
			if(header.length > 0) {
				received = _clientSocket->Recv((char*)payload.data(), header.length, 0);
				if(received != (int)header.length) {
					break;
				}
			}

			_messagesReceived++;
			_bytesReceived += sizeof(MessageHeader) + header.length;

			// Process message
			ProcessIncomingMessage(header.type, payload);
		}
		catch(const std::exception& e) {
			_debugger->Log("[DiztinGUIsh] Error receiving message: " + std::string(e.what()));
			break;
		}
	}

	_clientConnected = false;
}

void DiztinguishBridge::ProcessIncomingMessage(MessageType type, const std::vector<uint8_t>& payload)
{
	switch(type) {
		case MessageType::HandshakeAck:
			if(payload.size() >= sizeof(HandshakeAckMessage)) {
				HandleHandshakeAck(*(const HandshakeAckMessage*)payload.data());
			}
			break;

		case MessageType::ConfigStream:
			if(payload.size() >= sizeof(ConfigStreamMessage)) {
				HandleConfigStream(*(const ConfigStreamMessage*)payload.data());
			}
			break;

		case MessageType::CpuStateRequest:
			HandleCpuStateRequest();
			break;

		case MessageType::LabelAdd:
			if(payload.size() >= sizeof(LabelMessage)) {
				HandleLabelAdd(*(const LabelMessage*)payload.data());
			}
			break;

		case MessageType::LabelUpdate:
			if(payload.size() >= sizeof(LabelMessage)) {
				HandleLabelUpdate(*(const LabelMessage*)payload.data());
			}
			break;

		case MessageType::LabelDelete:
			if(payload.size() >= sizeof(LabelMessage)) {
				HandleLabelDelete(*(const LabelMessage*)payload.data());
			}
			break;

		case MessageType::LabelSyncRequest:
			HandleLabelSyncRequest();
			break;

		case MessageType::BreakpointAdd:
			if(payload.size() >= sizeof(BreakpointMessage)) {
				HandleBreakpointAdd(*(const BreakpointMessage*)payload.data());
			}
			break;

		case MessageType::BreakpointRemove:
			if(payload.size() >= sizeof(BreakpointMessage)) {
				HandleBreakpointRemove(*(const BreakpointMessage*)payload.data());
			}
			break;

		case MessageType::MemoryDumpRequest:
			if(payload.size() >= sizeof(MemoryDumpRequest)) {
				HandleMemoryDumpRequest(*(const MemoryDumpRequest*)payload.data());
			}
			break;

		case MessageType::Heartbeat:
			HandleHeartbeat();
			break;

		case MessageType::Disconnect:
			HandleDisconnect();
			break;

		default:
			_debugger->Log("[DiztinGUIsh] Unknown message type: " + std::to_string((int)type));
			SendError(ErrorCode::InvalidMessage, "Unknown message type");
			break;
	}
}

void DiztinguishBridge::SendHandshake()
{
	if(!_clientConnected || !_cart) {
		return;
	}

	HandshakeMessage msg = {};
	msg.protocolVersionMajor = PROTOCOL_VERSION_MAJOR;
	msg.protocolVersionMinor = PROTOCOL_VERSION_MINOR;

	// Get ROM checksum
	// TODO: Get actual ROM data and calculate CRC32
	msg.romChecksum = 0;  // Placeholder
	msg.romSize = 0;      // Placeholder

	// Get ROM name
	// TODO: Get actual ROM name from cart
	strncpy_s(msg.romName, sizeof(msg.romName), "Unknown ROM", _TRUNCATE);

	SendMessage(MessageType::Handshake, &msg, sizeof(msg));
	_debugger->Log("[DiztinGUIsh] Sent handshake");
}

void DiztinguishBridge::SendMessage(MessageType type, const void* payload, uint32_t length)
{
	if(!_clientConnected) {
		return;
	}

	std::lock_guard<std::mutex> lock(_queueMutex);

	// Build message
	std::vector<uint8_t> message(sizeof(MessageHeader) + length);
	MessageHeader* header = (MessageHeader*)message.data();
	header->type = type;
	header->length = length;

	if(length > 0 && payload) {
		memcpy(message.data() + sizeof(MessageHeader), payload, length);
	}

	_outgoingMessages.push(std::move(message));
}

void DiztinguishBridge::FlushOutgoingMessages()
{
	if(!_clientConnected || !_clientSocket) {
		return;
	}

	std::lock_guard<std::mutex> lock(_queueMutex);

	while(!_outgoingMessages.empty()) {
		const auto& message = _outgoingMessages.front();

		try {
			_clientSocket->BufferedSend((char*)message.data(), (int)message.size());
			_messagesSent++;
			_bytesSent += message.size();
		}
		catch(const std::exception& e) {
			_debugger->Log("[DiztinGUIsh] Error sending message: " + std::string(e.what()));
			_clientConnected = false;
			break;
		}

		_outgoingMessages.pop();
	}

	// Flush socket send buffer
	if(_clientSocket) {
		_clientSocket->SendBuffer();
	}
}

// Frame lifecycle
void DiztinguishBridge::OnFrameStart(uint32_t frameNumber)
{
	_currentFrame = frameNumber;
}

void DiztinguishBridge::OnFrameEnd()
{
	if(!_clientConnected || !_configReceived) {
		return;
	}

	// Send execution trace batch if enabled and interval reached
	if(_config.enableExecTrace && !_traceBuffer.empty()) {
		uint32_t framesSinceLastSend = _currentFrame - _lastTraceSentFrame;
		if(framesSinceLastSend >= _config.traceFrameInterval) {
			// Build batch message
			ExecTraceBatchMessage batchHeader;
			batchHeader.frameNumber = _currentFrame;
			batchHeader.entryCount = (uint16_t)std::min(_traceBuffer.size(), (size_t)_config.maxTracesPerFrame);

			// Calculate total size
			uint32_t totalSize = sizeof(batchHeader) + batchHeader.entryCount * sizeof(ExecTraceEntry);
			std::vector<uint8_t> payload(totalSize);

			// Copy header
			memcpy(payload.data(), &batchHeader, sizeof(batchHeader));

			// Copy trace entries
			memcpy(payload.data() + sizeof(batchHeader), _traceBuffer.data(), batchHeader.entryCount * sizeof(ExecTraceEntry));

			SendMessage(MessageType::ExecTraceBatch, payload.data(), totalSize);

			_traceBuffer.clear();
			_lastTraceSentFrame = _currentFrame;
		}
	}

	// Send CDL updates if enabled and dirty
	if(_config.enableCdlUpdates && !_cdlDirty.empty()) {
		uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();

		// Send CDL updates every 1 second
		if(now - _lastCdlSyncTime >= 1000) {
			// Build CDL update message
			std::vector<CdlUpdateEntry> updates;
			updates.reserve(_cdlDirty.size());

			for(const auto& [offset, flags] : _cdlDirty) {
				CdlUpdateEntry entry;
				entry.address = offset;
				entry.flags = flags;
				updates.push_back(entry);
			}

			SendMessage(MessageType::CdlUpdate, updates.data(), (uint32_t)(updates.size() * sizeof(CdlUpdateEntry)));

			_cdlDirty.clear();
			_lastCdlSyncTime = now;
		}
	}

	// Flush outgoing message queue
	FlushOutgoingMessages();
}

// CPU execution hook
void DiztinguishBridge::OnCpuExec(uint32_t pc, uint8_t opcode, bool mFlag, bool xFlag, uint8_t db, uint16_t dp, uint32_t effectiveAddr)
{
	if(!_clientConnected || !_configReceived || !_config.enableExecTrace) {
		return;
	}

	// Don't exceed max traces per frame
	if(_traceBuffer.size() >= _config.maxTracesPerFrame) {
		return;
	}

	ExecTraceEntry entry;
	entry.pc = pc;
	entry.opcode = opcode;
	entry.mFlag = mFlag ? 1 : 0;
	entry.xFlag = xFlag ? 1 : 0;
	entry.dbRegister = db;
	entry.dpRegister = dp;
	entry.effectiveAddr = effectiveAddr;

	_traceBuffer.push_back(entry);
}

// Message handlers
void DiztinguishBridge::HandleHandshakeAck(const HandshakeAckMessage& msg)
{
	if(!msg.accepted) {
		_debugger->Log("[DiztinGUIsh] Handshake rejected by client");
		_clientConnected = false;
		return;
	}

	_debugger->Log("[DiztinGUIsh] Handshake accepted by " + std::string(msg.clientName));
}

void DiztinguishBridge::HandleConfigStream(const ConfigStreamMessage& msg)
{
	_config = msg;
	_configReceived = true;

	_debugger->Log("[DiztinGUIsh] Configuration received:");
	_debugger->Log("  ExecTrace: " + std::string(_config.enableExecTrace ? "ON" : "OFF"));
	_debugger->Log("  MemoryAccess: " + std::string(_config.enableMemoryAccess ? "ON" : "OFF"));
	_debugger->Log("  CDL Updates: " + std::string(_config.enableCdlUpdates ? "ON" : "OFF"));
	_debugger->Log("  Trace Interval: " + std::to_string(_config.traceFrameInterval) + " frames");
	_debugger->Log("  Max Traces/Frame: " + std::to_string(_config.maxTracesPerFrame));
}

void DiztinguishBridge::HandleCpuStateRequest()
{
	SendCpuState();
}

void DiztinguishBridge::HandleLabelAdd(const LabelMessage& msg)
{
	// TODO: Implement label management in debugger
	_debugger->Log("[DiztinGUIsh] Label add: " + std::string(msg.name) + " @ $" + std::to_string(msg.address));
}

void DiztinguishBridge::HandleLabelUpdate(const LabelMessage& msg)
{
	// TODO: Implement label management
	_debugger->Log("[DiztinGUIsh] Label update: " + std::string(msg.name) + " @ $" + std::to_string(msg.address));
}

void DiztinguishBridge::HandleLabelDelete(const LabelMessage& msg)
{
	// TODO: Implement label management
	Log("[DiztinGUIsh] Label delete @ $" + std::to_string(msg.address));
}

void DiztinguishBridge::HandleLabelSyncRequest()
{
	// TODO: Send all labels to client
	Log("[DiztinGUIsh] Label sync requested");
}

void DiztinguishBridge::HandleBreakpointAdd(const BreakpointMessage& msg)
{
	// TODO: Implement breakpoint management
	Log("[DiztinGUIsh] Breakpoint add @ $" + std::to_string(msg.address));
}

void DiztinguishBridge::HandleBreakpointRemove(const BreakpointMessage& msg)
{
	// TODO: Implement breakpoint management
	Log("[DiztinGUIsh] Breakpoint remove @ $" + std::to_string(msg.address));
}

void DiztinguishBridge::HandleMemoryDumpRequest(const MemoryDumpRequest& msg)
{
	// TODO: Implement memory dump
	Log("[DiztinGUIsh] Memory dump requested: type=" + std::to_string(msg.memoryType) + 
		" addr=$" + std::to_string(msg.startAddress) + " len=" + std::to_string(msg.length));
}

void DiztinguishBridge::HandleHeartbeat()
{
	// Echo heartbeat
	SendMessage(MessageType::Heartbeat, nullptr, 0);
}

void DiztinguishBridge::HandleDisconnect()
{
	Log("[DiztinGUIsh] Client requested disconnect");
	_clientConnected = false;
}

void DiztinguishBridge::SendCpuState()
{
	// TODO: Get actual CPU state from SNES core
	CpuStateSnapshot state = {};
	SendMessage(MessageType::CpuState, &state, sizeof(state));
}

void DiztinguishBridge::SendError(ErrorCode code, const char* message)
{
	ErrorMessage msg;
	msg.errorCode = (uint8_t)code;
	strncpy_s(msg.message, sizeof(msg.message), message, _TRUNCATE);
	SendMessage(MessageType::Error, &msg, sizeof(msg));
}

void DiztinguishBridge::OnMemoryAccess(uint32_t address, uint8_t value, uint8_t type)
{
	// TODO: Implement if needed
}

void DiztinguishBridge::OnCdlChanged(uint32_t romOffset, uint8_t flags)
{
	if(!_clientConnected || !_configReceived || !_config.enableCdlUpdates) {
		return;
	}

	_cdlDirty[romOffset] = flags;
}

void DiztinguishBridge::OnBreakpointHit(uint32_t address, uint8_t type)
{
	if(!_clientConnected) {
		return;
	}

	BreakpointHitMessage msg;
	msg.address = address;
	msg.type = type;
	// TODO: Fill CPU state
	memset(&msg.cpuState, 0, sizeof(msg.cpuState));

	SendMessage(MessageType::BreakpointHit, &msg, sizeof(msg));
}

uint64_t DiztinguishBridge::GetConnectionDuration() const
{
	if(!_clientConnected || _connectionStartTime == 0) {
		return 0;
	}

	uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();

	return now - _connectionStartTime;
}

double DiztinguishBridge::GetBandwidthKBps() const
{
	uint64_t duration = GetConnectionDuration();
	if(duration == 0) {
		return 0.0;
	}

	return (_bytesSent / 1024.0) / (duration / 1000.0);
}

void DiztinguishBridge::SendCdlSnapshot()
{
	// TODO: Implement
}

void DiztinguishBridge::SendLabelAdd(uint32_t address, const char* name, uint8_t type)
{
	// TODO: Implement
}

void DiztinguishBridge::SendLabelUpdate(uint32_t address, const char* name, uint8_t type)
{
	// TODO: Implement
}

void DiztinguishBridge::SendLabelDelete(uint32_t address)
{
	// TODO: Implement
}
