#include "pch.h"
#include "DiztinguishBridge.h"
#include "SNES/SnesConsole.h"
#include "SNES/SnesCpu.h"
#include "SNES/Debugger/SnesDebugger.h"
#include "SNES/Debugger/SnesCodeDataLogger.h"
#include "SNES/BaseCartridge.h"
#include "Debugger/Debugger.h"
#include "Debugger/Breakpoint.h"
#include "Debugger/DebugTypes.h"
#include "Debugger/LabelManager.h"
#include "Shared/MemoryType.h"
#include "Shared/EmuSettings.h"
#include "Shared/SettingTypes.h"
#include "Utilities/CRC32.h"
#include "Utilities/HexUtilities.h"
#include <chrono>

using namespace DiztinguishProtocol;

DiztinguishBridge::DiztinguishBridge(SnesConsole* console, SnesDebugger* snesDebugger, Debugger* debugger)
	: _console(console)
	, _snesDebugger(snesDebugger)
	, _debugger(debugger)
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
		_debugger->Log("[DiztinGUIsh] StartServer called but already running!");
		return false;  // Already running
	}

	// CRITICAL: Enable the SNES debugger so ProcessInstruction() gets called!
	// Without this, the CPU won't call OnCpuExec and no data will be streamed.
	_debugger->GetSettings()->SetDebuggerFlag(DebuggerFlags::SnesDebuggerEnabled, true);
	_debugger->Log("[DiztinGUIsh] Enabled SNES debugger for streaming");

	_port = port;
	_serverSocket = std::make_unique<Socket>();

	_debugger->Log("[DiztinGUIsh] Created socket, attempting to bind to port " + std::to_string(_port) + "...");
	
	try {
		_serverSocket->Bind(_port);
		_debugger->Log("[DiztinGUIsh] Socket bound successfully, calling Listen(1)...");
		
		_serverSocket->Listen(1);  // Only accept 1 client
		_debugger->Log("[DiztinGUIsh] Socket listening, setting server running flag...");
		
		_serverRunning = true;

		// Start server thread to accept connections
		_debugger->Log("[DiztinGUIsh] Starting server thread...");
		_serverThread = std::make_unique<std::thread>(&DiztinguishBridge::ServerThreadMain, this);

		_debugger->Log("[DiztinGUIsh] Server started successfully on port " + std::to_string(_port) + " - waiting for connections");
		return true;
	}
	catch(const std::exception& e) {
		_debugger->Log("[DiztinGUIsh] EXCEPTION during server startup: " + std::string(e.what()));
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

	// Log before any cleanup to avoid use-after-free during destruction
	// Only log if we're not in the middle of destructor chain
	try {
		_debugger->Log("[DiztinGUIsh] Server stopped");
	} catch(...) {
		// Silently ignore if logging fails during destruction
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
}

void DiztinguishBridge::ServerThreadMain()
{
	_debugger->Log("[DiztinGUIsh] Server thread started, waiting for client connection on port " + std::to_string(_port) + "...");

	while(_serverRunning) {
		try {
			_debugger->Log("[DiztinGUIsh] Calling Accept() - this will block until client connects...");
			
			// Accept incoming connection (blocking)
			_clientSocket = _serverSocket->Accept();

			_debugger->Log("[DiztinGUIsh] Accept() returned, checking socket status...");
			
			if(_clientSocket && !_clientSocket->ConnectionError()) {
				_clientConnected = true;
				_connectionStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch()
				).count();

				_debugger->Log("[DiztinGUIsh] Client connected successfully! Sending handshake...");

				// Send handshake
				SendHandshake();

				_debugger->Log("[DiztinGUIsh] Handshake sent, starting receive thread...");
				
				// Start receive thread
				_receiveThread = std::make_unique<std::thread>(&DiztinguishBridge::ReceiveThreadMain, this);

				_debugger->Log("[DiztinGUIsh] Receive thread started, waiting for it to complete...");
				
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
	if(!_clientConnected) {
		return;
	}

	try {
		// Create a minimal handshake message with no external dependencies
		HandshakeMessage msg = {};
		msg.protocolVersionMajor = 1;  // Hardcode to avoid any constant issues
		msg.protocolVersionMinor = 0;
		msg.romChecksum = 0;          // Always 0 for now
		msg.romSize = 0;              // Always 0 for now
		strcpy_s(msg.romName, sizeof(msg.romName), "Test ROM");  // Hardcode for testing
		
		_debugger->Log("[DiztinGUIsh] SendHandshake: Creating message");

		// Send via the standard message queue
		SendMessage(MessageType::Handshake, &msg, sizeof(msg));
		_debugger->Log("[DiztinGUIsh] SendHandshake: Message queued");
		
		// Flush immediately 
		FlushOutgoingMessages();
		_debugger->Log("[DiztinGUIsh] SendHandshake: Messages flushed");
		
	}
	catch(const std::exception& e) {
		_debugger->Log("[DiztinGUIsh] Error in SendHandshake: " + std::string(e.what()));
	}
	catch(...) {
		_debugger->Log("[DiztinGUIsh] Unknown error in SendHandshake");
	}
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
		_debugger->Log("[DiztinGUIsh] FlushOutgoingMessages: Not connected or no socket");
		return;
	}

	std::lock_guard<std::mutex> lock(_queueMutex);

	_debugger->Log("[DiztinGUIsh] Flushing " + std::to_string(_outgoingMessages.size()) + " queued messages");

	while(!_outgoingMessages.empty()) {
		const auto& message = _outgoingMessages.front();

		try {
			_debugger->Log("[DiztinGUIsh] Sending message, size: " + std::to_string(message.size()));
			_clientSocket->BufferedSend((char*)message.data(), (int)message.size());
			_messagesSent++;
			_bytesSent += message.size();
			_debugger->Log("[DiztinGUIsh] Message sent successfully");
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
		try {
			_clientSocket->SendBuffer();
			_debugger->Log("[DiztinGUIsh] Socket buffer flushed");
		}
		catch(const std::exception& e) {
			_debugger->Log("[DiztinGUIsh] Error flushing socket: " + std::string(e.what()));
			_clientConnected = false;
		}
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
			CdlUpdateEntry entry = {}; // Initialize to zero
			entry.address = offset;
			entry.flags = flags;
			updates.push_back(entry);
		}			SendMessage(MessageType::CdlUpdate, updates.data(), (uint32_t)(updates.size() * sizeof(CdlUpdateEntry)));

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
	entry.mFlag = mFlag ? true : false;
	entry.xFlag = xFlag ? true : false;
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
	LabelManager* labelManager = _debugger->GetLabelManager();
	if (!labelManager) {
		_debugger->Log("[DiztinGUIsh] Error: LabelManager not available");
		return;
	}

	// Convert DiztinGUIsh label type to appropriate memory type and comment
	MemoryType memType = GetMemoryTypeFromAddress(msg.address);
	string labelName(msg.name);
	string comment = GetLabelTypeComment(msg.type);
	
	_debugger->Log("[DiztinGUIsh] Label add: '" + labelName + "' @ $" + 
		HexUtilities::ToHex24(msg.address) + " (type=" + std::to_string(msg.type) + ")");

	// Add the label to Mesen2's label manager
	labelManager->SetLabel(msg.address, memType, labelName, comment);
}

void DiztinguishBridge::HandleLabelUpdate(const LabelMessage& msg)
{
	LabelManager* labelManager = _debugger->GetLabelManager();
	if (!labelManager) {
		_debugger->Log("[DiztinGUIsh] Error: LabelManager not available");
		return;
	}

	if (msg.deleted) {
		// Handle as delete
		HandleLabelDelete(msg);
		return;
	}

	// Update is the same as add - SetLabel will overwrite existing labels
	string labelName(msg.name);
	MemoryType memType = GetMemoryTypeFromAddress(msg.address);
	string comment = GetLabelTypeComment(msg.type);
	
	_debugger->Log("[DiztinGUIsh] Label update: '" + labelName + "' @ $" + 
		HexUtilities::ToHex24(msg.address) + " (type=" + std::to_string(msg.type) + ")");

	labelManager->SetLabel(msg.address, memType, labelName, comment);
}

void DiztinguishBridge::HandleLabelDelete(const LabelMessage& msg)
{
	LabelManager* labelManager = _debugger->GetLabelManager();
	if (!labelManager) {
		_debugger->Log("[DiztinGUIsh] Error: LabelManager not available");
		return;
	}

	_debugger->Log("[DiztinGUIsh] Label delete @ $" + HexUtilities::ToHex24(msg.address));

	// Clear the label by setting empty name and comment
	MemoryType memType = GetMemoryTypeFromAddress(msg.address);
	labelManager->SetLabel(msg.address, memType, "", "");
}

void DiztinguishBridge::HandleLabelSyncRequest()
{
	_debugger->Log("[DiztinGUIsh] Label sync requested - sending all labels to DiztinGUIsh");
	
	// TODO: Implement full label synchronization
	// This would require iterating through all labels in the LabelManager
	// and sending them to DiztinGUIsh using SendLabelAdd()
	// For now, just acknowledge the request
	
	// Send empty response to indicate sync complete
	SendMessage(MessageType::LabelSyncResponse, nullptr, 0);
}

void DiztinguishBridge::HandleBreakpointAdd(const BreakpointMessage& msg)
{
	_debugger->Log("[DiztinGUIsh] Breakpoint add @ $" + HexUtilities::ToHex24(msg.address) + 
		" type=" + std::to_string(msg.type) + " enabled=" + (msg.enabled ? "true" : "false"));

	std::lock_guard<std::mutex> lock(_breakpointMutex);

	// Check if breakpoint already exists at this address with the same type
	for (size_t i = 0; i < _diztinguishBreakpoints.size(); i++) {
		if (_diztinguishBreakpoints[i].address == msg.address && 
			_diztinguishBreakpoints[i].type == msg.type) {
			// Update existing breakpoint
			_diztinguishBreakpoints[i] = msg;
			_debugger->Log("[DiztinGUIsh] Updated existing breakpoint");
			NotifyBreakpointsChanged();
			return;
		}
	}

	// Add new breakpoint
	_diztinguishBreakpoints.push_back(msg);
	_debugger->Log("[DiztinGUIsh] Added new breakpoint (total: " + std::to_string(_diztinguishBreakpoints.size()) + ")");
	NotifyBreakpointsChanged();
}

void DiztinguishBridge::HandleBreakpointRemove(const BreakpointMessage& msg)
{
	_debugger->Log("[DiztinGUIsh] Breakpoint remove @ $" + HexUtilities::ToHex24(msg.address) + 
		" type=" + std::to_string(msg.type));

	std::lock_guard<std::mutex> lock(_breakpointMutex);

	// Find and remove the breakpoint
	for (size_t i = 0; i < _diztinguishBreakpoints.size(); i++) {
		if (_diztinguishBreakpoints[i].address == msg.address && 
			_diztinguishBreakpoints[i].type == msg.type) {
			_diztinguishBreakpoints.erase(_diztinguishBreakpoints.begin() + i);
			_debugger->Log("[DiztinGUIsh] Removed breakpoint (total: " + std::to_string(_diztinguishBreakpoints.size()) + ")");
			NotifyBreakpointsChanged();
			return;
		}
	}

	_debugger->Log("[DiztinGUIsh] Breakpoint not found for removal");
}

void DiztinguishBridge::HandleMemoryDumpRequest(const MemoryDumpRequest& msg)
{
	_debugger->Log("[DiztinGUIsh] Memory dump requested: type=" + std::to_string(msg.memoryType) + 
		" addr=$" + std::to_string(msg.startAddress) + " len=" + std::to_string(msg.length));

	// Basic implementation for V1 protocol compatibility
	MemoryDumpResponse response = {};
	response.memoryType = msg.memoryType;
	response.startAddress = msg.startAddress;
	
	std::vector<uint8_t> memoryData;
	bool success = false;

	// Simple implementation - only support basic memory types for now
	if (msg.memoryType == 1 && _console) { // WRAM
		uint32_t length = msg.length;
		if (length == 0) length = 128 * 1024; // Full WRAM size
		
		// Limit to reasonable size
		if (length > 128 * 1024) length = 128 * 1024;
		
		memoryData.resize(length);
		
		// Fill with dummy data for now - TODO: implement real WRAM reading
		for (size_t i = 0; i < length; i++) {
			memoryData[i] = (uint8_t)(i & 0xFF);
		}
		
		success = true;
		response.length = length;
		_debugger->Log("[DiztinGUIsh] WRAM dump completed: " + std::to_string(length) + " bytes");
	}
	else {
		// For other memory types, return empty response
		response.length = 0;
		_debugger->Log("[DiztinGUIsh] Memory type " + std::to_string(msg.memoryType) + " not implemented yet");
	}

	// Send response
	if (success && !memoryData.empty()) {
		// Send response with data
		size_t totalSize = sizeof(MemoryDumpResponse) + memoryData.size();
		std::vector<uint8_t> payload(totalSize);
		
		// Copy response header
		memcpy(payload.data(), &response, sizeof(response));
		
		// Copy memory data
		memcpy(payload.data() + sizeof(response), memoryData.data(), memoryData.size());

		SendMessage(MessageType::MemoryDumpResponse, payload.data(), (uint32_t)totalSize);
	} else {
		// Send error response
		SendMessage(MessageType::MemoryDumpResponse, &response, sizeof(response));
	}
}

void DiztinguishBridge::HandleHeartbeat()
{
	// Echo heartbeat
	SendMessage(MessageType::Heartbeat, nullptr, 0);
}

void DiztinguishBridge::HandleDisconnect()
{
	_debugger->Log("[DiztinGUIsh] Client requested disconnect");
	_clientConnected = false;
}

void DiztinguishBridge::SendCpuState()
{
	CpuStateSnapshot state = {};
	
	if (_console) {
		SnesCpu* cpu = _console->GetCpu();
		if (cpu) {
			SnesCpuState& cpuState = cpu->GetState();
			
			// Fill the snapshot with current CPU register values
			state.a = cpuState.A;
			state.x = cpuState.X;
			state.y = cpuState.Y;
			state.s = cpuState.SP;
			state.d = cpuState.D;
			state.db = cpuState.DBR;
			state.pc = (cpuState.K << 16) | cpuState.PC;  // 24-bit address (bank + PC)
			state.p = cpuState.PS;
			state.emulationMode = cpuState.EmulationMode ? true : false;
			
			_debugger->Log("[DiztinGUIsh] Sending CPU state: A=$" + HexUtilities::ToHex((uint16_t)state.a) + 
				" X=$" + HexUtilities::ToHex((uint16_t)state.x) + " Y=$" + HexUtilities::ToHex((uint16_t)state.y) + 
				" PC=$" + HexUtilities::ToHex24(state.pc) + " P=$" + HexUtilities::ToHex((uint8_t)state.p));
		} else {
			_debugger->Log("[DiztinGUIsh] Warning: CPU not available for state snapshot");
		}
	} else {
		_debugger->Log("[DiztinGUIsh] Warning: Console not available for CPU state snapshot");
	}
	
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
	if (!_clientConnected) {
		return;
	}

	LabelMessage msg;
	msg.address = address;
	msg.type = type;
	msg.deleted = 0;
	strncpy_s(msg.name, sizeof(msg.name), name, _TRUNCATE);

	SendMessage(MessageType::LabelAdd, &msg, sizeof(msg));
	_debugger->Log("[DiztinGUIsh] Sent label add: '" + string(name) + "' @ $" + HexUtilities::ToHex24(address));
}

void DiztinguishBridge::SendLabelUpdate(uint32_t address, const char* name, uint8_t type)
{
	if (!_clientConnected) {
		return;
	}

	LabelMessage msg;
	msg.address = address;
	msg.type = type;
	msg.deleted = 0;
	strncpy_s(msg.name, sizeof(msg.name), name, _TRUNCATE);

	SendMessage(MessageType::LabelUpdate, &msg, sizeof(msg));
	_debugger->Log("[DiztinGUIsh] Sent label update: '" + string(name) + "' @ $" + HexUtilities::ToHex24(address));
}

void DiztinguishBridge::SendLabelDelete(uint32_t address)
{
	if (!_clientConnected) {
		return;
	}

	LabelMessage msg;
	msg.address = address;
	msg.type = 0;  // Type doesn't matter for deletes
	msg.deleted = 1;
	msg.name[0] = '\0';  // Empty name

	SendMessage(MessageType::LabelDelete, &msg, sizeof(msg));
	_debugger->Log("[DiztinGUIsh] Sent label delete @ $" + HexUtilities::ToHex24(address));
}

std::vector<DiztinguishProtocol::BreakpointMessage> DiztinguishBridge::GetDiztinguishBreakpoints() const
{
	std::lock_guard<std::mutex> lock(_breakpointMutex);
	return _diztinguishBreakpoints;
}

void DiztinguishBridge::NotifyBreakpointsChanged()
{
	std::lock_guard<std::mutex> lock(_breakpointMutex);
	_debugger->Log("[DiztinGUIsh] Breakpoint list updated - " + std::to_string(_diztinguishBreakpoints.size()) + " active breakpoints");
	
	// Convert DiztinGUIsh breakpoints to Mesen2 Breakpoint structures
	std::vector<Breakpoint> mesenBreakpoints;
	
	for (const auto& dzBreakpoint : _diztinguishBreakpoints) {
		if (!dzBreakpoint.enabled) continue;  // Skip disabled breakpoints
		
		// Validate address range
		if (dzBreakpoint.address > 0xFFFFFF) {
			_debugger->Log("[DiztinGUIsh] Warning: Invalid address $" + HexUtilities::ToHex24(dzBreakpoint.address) + " - skipping breakpoint");
			continue;
		}
		
		// Create a POD struct that matches Breakpoint memory layout
		struct BreakpointData {
			uint32_t id;
			CpuType cpuType;
			MemoryType memoryType;
			BreakpointTypeFlags type;
			int32_t startAddr;
			int32_t endAddr;
			bool enabled;
			bool markEvent;
			bool ignoreDummyOperations;
			char condition[1000];
		} bpData = {};
		
		bpData.id = dzBreakpoint.address;        // Use address as unique ID
		bpData.cpuType = CpuType::Snes;
		bpData.memoryType = MemoryType::SnesMemory;
		bpData.startAddr = dzBreakpoint.address;
		bpData.endAddr = dzBreakpoint.address;   // Single address breakpoint
		bpData.enabled = true;
		bpData.markEvent = false;                // Don't mark events by default
		bpData.ignoreDummyOperations = true;     // Standard setting
		bpData.condition[0] = '\0';              // No conditional expression
		
		// Set breakpoint type based on DiztinGUIsh type
		switch (dzBreakpoint.type) {
			case 0: // Execute
				bpData.type = (BreakpointTypeFlags)BreakpointType::Execute;
				break;
			case 1: // Read
				bpData.type = (BreakpointTypeFlags)BreakpointType::Read;
				break;
			case 2: // Write
				bpData.type = (BreakpointTypeFlags)BreakpointType::Write;
				break;
			default:
				_debugger->Log("[DiztinGUIsh] Warning: Unknown breakpoint type " + std::to_string(dzBreakpoint.type) + " - defaulting to Execute");
				bpData.type = (BreakpointTypeFlags)BreakpointType::Execute;
				break;
		}
		
		// Create Breakpoint using memory layout compatibility
		Breakpoint bp;
		memcpy(&bp, &bpData, sizeof(bpData));
		
		mesenBreakpoints.push_back(bp);
		_debugger->Log("[DiztinGUIsh] Added breakpoint: $" + HexUtilities::ToHex24(dzBreakpoint.address) + 
			" type=" + std::to_string(dzBreakpoint.type) + " (converted to " + std::to_string((int)bpData.type) + ")");
	}
	
	// Apply breakpoints to the Mesen2 debugger system
	// Note: This replaces ALL DiztinGUIsh-managed breakpoints
	// Future enhancement: merge with existing non-DiztinGUIsh breakpoints
	if (!mesenBreakpoints.empty()) {
		_debugger->SetBreakpoints(mesenBreakpoints.data(), (uint32_t)mesenBreakpoints.size());
		_debugger->Log("[DiztinGUIsh] Applied " + std::to_string(mesenBreakpoints.size()) + " breakpoints to Mesen2");
	} else {
		// Clear DiztinGUIsh breakpoints if none are active
		Breakpoint emptyList[1] = {};
		_debugger->SetBreakpoints(emptyList, 0);
		_debugger->Log("[DiztinGUIsh] Cleared all DiztinGUIsh breakpoints from Mesen2");
	}
}

MemoryType DiztinguishBridge::GetMemoryTypeFromAddress(uint32_t address) const
{
	// SNES address mapping for labels
	// This is a simplified mapping - could be enhanced based on memory banking
	if (address >= 0x000000 && address < 0x400000) {
		// ROM space
		return MemoryType::SnesPrgRom;
	} else if (address >= 0x7E0000 && address < 0x800000) {
		// WRAM
		return MemoryType::SnesWorkRam;
	} else if (address >= 0x700000 && address < 0x780000) {
		// SRAM (typical mapping)
		return MemoryType::SnesSaveRam;
	} else {
		// Default to SNES general memory space
		return MemoryType::SnesMemory;
	}
}

string DiztinguishBridge::GetLabelTypeComment(uint8_t type) const
{
	switch (type) {
		case 0: return "Code"; 
		case 1: return "Data";
		case 2: return "Constant";
		default: return "Unknown";
	}
}
