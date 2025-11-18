#pragma once
#include "pch.h"
#include "Debugger/DiztinguishProtocol.h"
#include "Utilities/Socket.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

class SnesConsole;
class SnesDebugger;
class Debugger;
class SnesCodeDataLogger;
class BaseCartridge;

// DiztinGUIsh Bridge - Real-time streaming integration
// Implements TCP server that streams execution traces, CDL updates, and debug info
// to DiztinGUIsh for live disassembly analysis
class DiztinguishBridge
{
private:
	SnesConsole* _console;
	SnesDebugger* _snesDebugger;
	Debugger* _debugger;
	BaseCartridge* _cart;

	// Server socket
	unique_ptr<Socket> _serverSocket;
	unique_ptr<Socket> _clientSocket;
	uint16_t _port;
	std::atomic<bool> _serverRunning;
	std::atomic<bool> _clientConnected;
	
	// Server thread
	unique_ptr<std::thread> _serverThread;
	unique_ptr<std::thread> _receiveThread;

	// Message queue (thread-safe)
	std::queue<std::vector<uint8_t>> _outgoingMessages;
	std::mutex _queueMutex;

	// Configuration (received from client)
	DiztinguishProtocol::ConfigStreamMessage _config;
	bool _configReceived;

	// Execution trace buffering
	std::vector<DiztinguishProtocol::ExecTraceEntry> _traceBuffer;
	uint32_t _currentFrame;
	uint32_t _lastTraceSentFrame;

	// CDL tracking
	std::unordered_map<uint32_t, uint8_t> _cdlDirty;  // Changed CDL entries
	uint64_t _lastCdlSyncTime;

	// Statistics
	uint64_t _messagesSent;
	uint64_t _messagesReceived;
	uint64_t _bytesSent;
	uint64_t _bytesReceived;
	uint64_t _connectionStartTime;

	// Internal methods
	void ServerThreadMain();
	void ReceiveThreadMain();
	void ProcessIncomingMessage(DiztinguishProtocol::MessageType type, const std::vector<uint8_t>& payload);
	void SendHandshake();
	void SendMessage(DiztinguishProtocol::MessageType type, const void* payload, uint32_t length);
	void FlushOutgoingMessages();

	// Message handlers (from client)
	void HandleHandshakeAck(const DiztinguishProtocol::HandshakeAckMessage& msg);
	void HandleConfigStream(const DiztinguishProtocol::ConfigStreamMessage& msg);
	void HandleCpuStateRequest();
	void HandleLabelAdd(const DiztinguishProtocol::LabelMessage& msg);
	void HandleLabelUpdate(const DiztinguishProtocol::LabelMessage& msg);
	void HandleLabelDelete(const DiztinguishProtocol::LabelMessage& msg);
	void HandleLabelSyncRequest();
	void HandleBreakpointAdd(const DiztinguishProtocol::BreakpointMessage& msg);
	void HandleBreakpointRemove(const DiztinguishProtocol::BreakpointMessage& msg);
	void HandleMemoryDumpRequest(const DiztinguishProtocol::MemoryDumpRequest& msg);
	void HandleHeartbeat();
	void HandleDisconnect();

public:
	DiztinguishBridge(SnesConsole* console, SnesDebugger* snesDebugger, Debugger* debugger);
	~DiztinguishBridge();

	// Server control
	bool StartServer(uint16_t port = 9998);
	void StopServer();
	bool IsServerRunning() const { return _serverRunning; }
	bool IsClientConnected() const { return _clientConnected; }
	uint16_t GetPort() const { return _port; }
	uint16_t GetServerPort() const { return _port; } // Alias for Lua API clarity

	// Called by emulator core
	void OnFrameStart(uint32_t frameNumber);
	void OnFrameEnd();
	void OnCpuExec(uint32_t pc, uint8_t opcode, bool mFlag, bool xFlag, uint8_t db, uint16_t dp, uint32_t effectiveAddr);
	void OnMemoryAccess(uint32_t address, uint8_t value, uint8_t type);
	void OnCdlChanged(uint32_t romOffset, uint8_t flags);
	void OnBreakpointHit(uint32_t address, uint8_t type);

	// Outgoing messages (called by debugger)
	void SendCpuState();
	void SendCdlSnapshot();
	void SendLabelAdd(uint32_t address, const char* name, uint8_t type);
	void SendLabelUpdate(uint32_t address, const char* name, uint8_t type);
	void SendLabelDelete(uint32_t address);
	void SendError(DiztinguishProtocol::ErrorCode code, const char* message);

	// Statistics
	uint64_t GetMessagesSent() const { return _messagesSent; }
	uint64_t GetMessagesReceived() const { return _messagesReceived; }
	uint64_t GetBytesSent() const { return _bytesSent; }
	uint64_t GetBytesReceived() const { return _bytesReceived; }
	uint64_t GetConnectionDuration() const;
	double GetBandwidthKBps() const;
};
