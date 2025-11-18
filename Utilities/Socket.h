#pragma once

#include "pch.h"
#include <mutex>
#include <chrono>
#include <atomic>

class Socket
{
private:
	#ifdef _WIN32
	bool _cleanupWSA = false;
	#endif
	
	uintptr_t _socket = (uintptr_t)~0;
	bool _connectionError = false;
	int32_t _UPnPPort = -1;
	
	// Send buffer for BufferedSend/SendBuffer
	std::vector<char> _sendBuffer;
	std::mutex _sendBufferMutex;
	
	// Advanced Features - Statistics & Health
	std::atomic<uint64_t> _bytesSent{0};
	std::atomic<uint64_t> _bytesReceived{0};
	std::atomic<uint64_t> _messagesSent{0};
	std::atomic<uint64_t> _messagesReceived{0};
	std::chrono::steady_clock::time_point _connectionStart;
	std::chrono::steady_clock::time_point _lastActivity;
	
	// Buffer management
	size_t _maxBufferSize = 64 * 1024; // 64KB default
	bool _autoFlush = true;
	std::atomic<bool> _compressionEnabled{false};

public:
	Socket();
	Socket(uintptr_t socket);
	~Socket();

	void SetSocketOptions();
	void SetConnectionErrorFlag();

	void Close();
	bool ConnectionError();

	void Bind(uint16_t port);
	bool Connect(const char* hostname, uint16_t port);
	void Listen(int backlog);
	unique_ptr<Socket> Accept();

	int Send(char *buf, int len, int flags);
	void BufferedSend(char *buf, int len);
	void SendBuffer();
	int Recv(char *buf, int len, int flags);
	
	// Advanced Features
	void SetBufferSize(size_t size) { _maxBufferSize = size; }
	void SetAutoFlush(bool autoFlush) { _autoFlush = autoFlush; }
	void EnableCompression(bool enable) { _compressionEnabled = enable; }
	
	// Statistics & Health Monitoring
	uint64_t GetBytesSent() const { return _bytesSent; }
	uint64_t GetBytesReceived() const { return _bytesReceived; }
	uint64_t GetMessagesSent() const { return _messagesSent; }
	uint64_t GetMessagesReceived() const { return _messagesReceived; }
	double GetConnectionDurationSeconds() const;
	double GetBandwidthKBps() const;
	bool IsHealthy() const;
	void ResetStatistics();
	
	// Buffer Management
	size_t GetBufferedBytes() const;
	bool ShouldFlush() const;
	void FlushIfNeeded();
};
