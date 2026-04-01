#include "pch.h"
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include "Utilities/Socket.h"
#include "Utilities/UPnPPortMapper.h"
using namespace std;

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib") // Winsock Library
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>

#define INVALID_SOCKET    (uintptr_t)-1
#define SOCKET_ERROR      -1
#define WSAGetLastError() errno
#define SOCKADDR_IN       sockaddr_in
#define SOCKADDR          sockaddr
#define TIMEVAL           timeval
#define SD_SEND           SHUT_WR
#define closesocket       close
#define WSAEWOULDBLOCK    EWOULDBLOCK
#define ioctlsocket       ioctl
#endif

Socket::Socket() {
#ifdef _WIN32
	WSADATA wsaDat;
	if (WSAStartup(MAKEWORD(2, 2), &wsaDat) != 0) {
		std::cout << "WSAStartup failed." << std::endl;
		SetConnectionErrorFlag();
		return;
	}
	_cleanupWSA = true;
#endif

	_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socket == INVALID_SOCKET) {
		std::cout << "Socket creation failed." << std::endl;
		SetConnectionErrorFlag();
	} else {
		SetSocketOptions();
	}
	
	// Initialize advanced features
	ResetStatistics();
}

Socket::Socket(uintptr_t socket) {
	_socket = socket;

	if (socket == INVALID_SOCKET) {
		SetConnectionErrorFlag();
	} else {
		SetSocketOptions();
	}
	
	// Initialize advanced features
	ResetStatistics();
}

Socket::~Socket() {
	if (_UPnPPort != -1) {
		UPnPPortMapper::RemoveNATPortMapping(static_cast<uint16_t>(_UPnPPort), IPProtocol::TCP);
	}

	if (_socket != INVALID_SOCKET) {
		Close();
	}

#ifdef _WIN32
	if (_cleanupWSA) {
		WSACleanup();
	}
#endif
}

void Socket::SetSocketOptions() {
	// Non-blocking mode
	u_long iMode = 1;
	ioctlsocket(_socket, FIONBIO, &iMode);

	// Set send/recv buffers to 256k
	int bufferSize = 0x40000;
	setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(int));
	setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(int));

	// Disable nagle's algorithm to improve latency
	u_long value = 1;
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));
}

void Socket::SetConnectionErrorFlag() {
	_connectionError = true;
}

void Socket::Close() {
	std::cout << "Socket closed." << std::endl;
	shutdown(_socket, SD_SEND);
	closesocket(_socket);
	SetConnectionErrorFlag();
}

bool Socket::ConnectionError() {
	return _connectionError;
}

void Socket::Bind(uint16_t port) {
	SOCKADDR_IN serverInf;
	serverInf.sin_family = AF_INET;
	serverInf.sin_addr.s_addr = INADDR_ANY;
	serverInf.sin_port = htons(port);

	if (UPnPPortMapper::AddNATPortMapping(port, port, IPProtocol::TCP)) {
		_UPnPPort = port;
	}

	if (::bind(_socket, (SOCKADDR*)(&serverInf), sizeof(serverInf)) == SOCKET_ERROR) {
		std::cout << "Unable to bind socket." << std::endl;
		SetConnectionErrorFlag();
	}
}

bool Socket::Connect(const char* hostname, uint16_t port) {
	// Resolve IP address for hostname
	bool result = false;
	addrinfo hint;
	memset((void*)&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_protocol = IPPROTO_TCP;
	hint.ai_socktype = SOCK_STREAM;
	addrinfo* addrInfo;

	if (getaddrinfo(hostname, std::to_string(port).c_str(), &hint, &addrInfo) != 0) {
		std::cout << "Failed to resolve hostname." << std::endl;
		SetConnectionErrorFlag();
	} else {
		// Set socket in non-blocking mode
		u_long iMode = 1;
		ioctlsocket(_socket, FIONBIO, &iMode);

		// Attempt to connect to server
		connect(_socket, addrInfo->ai_addr, (int)addrInfo->ai_addrlen);

		fd_set writeSockets;
#ifdef _WIN32
		writeSockets.fd_count = 1;
		writeSockets.fd_array[0] = _socket;
#else
		FD_ZERO(&writeSockets);
		FD_SET(_socket, &writeSockets);
#endif

		// Timeout after 3 seconds
		TIMEVAL timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;

		// check if the socket is ready
		int returnVal = select((int)_socket + 1, nullptr, &writeSockets, nullptr, &timeout);
		if (returnVal > 0) {
			result = true;
		} else {
			// Could not connect
			if (returnVal == SOCKET_ERROR) {
				// int nError = WSAGetLastError();
				// std::cout << "select failed: nError " << std::to_string(nError) << " returnVal" << std::to_string(returnVal) << std::endl;
			}
			SetConnectionErrorFlag();
		}

		freeaddrinfo(addrInfo);
	}

	return result;
}

void Socket::Listen(int backlog) {
	if (listen(_socket, backlog) == SOCKET_ERROR) {
		std::cout << "listen failed." << std::endl;
		SetConnectionErrorFlag();
	}
}

unique_ptr<Socket> Socket::Accept() {
	uintptr_t socket = accept(_socket, nullptr, nullptr);
	return unique_ptr<Socket>(new Socket(socket));
}

bool WouldBlock(int nError) {
	return nError == WSAEWOULDBLOCK || nError == EAGAIN;
}

int Socket::Send(char* buf, int len, int flags) {
	int retryCount = 100;
	int nError = 0;
	int returnVal;
	do {
		// Loop until everything has been sent (shouldn't loop at all in the vast majority of cases)
		returnVal = send(_socket, buf, len, flags);

		if (returnVal > 0) {
			// Sent partial data, adjust pointer & length
			buf += returnVal;
			len -= returnVal;
		} else if (returnVal == SOCKET_ERROR) {
			nError = WSAGetLastError();
			if (nError != 0) {
				if (!WouldBlock(nError)) {
					SetConnectionErrorFlag();
				} else {
					retryCount--;
					if (retryCount == 0) {
						// Connection seems dead, close it.
						std::cout << "Unable to send data, closing socket." << std::endl;
						Close();
						return 0;
					}

					std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(20));
				}
			}
		}
	} while (WouldBlock(nError) && len > 0);

	return returnVal;
}

int Socket::Recv(char* buf, int len, int flags) {
	int returnVal = recv(_socket, buf, len, flags);

	if (returnVal == SOCKET_ERROR) {
		int nError = WSAGetLastError();
		if (nError && !WouldBlock(nError)) {
			std::cout << "recv failed: nError " << std::to_string(nError) << " returnVal" << std::to_string(returnVal) << std::endl;
			SetConnectionErrorFlag();
		}
	} else if (returnVal == 0) {
		// Socket closed
		std::cout << "Socket closed by peer." << std::endl;
		Close();
	} else if(returnVal > 0) {
		// Update statistics for successful receive
		_bytesReceived += returnVal;
		_messagesReceived++;
		_lastActivity = std::chrono::steady_clock::now();
	}

	return returnVal;
}

int Socket::BlockingRecv(char *buf, int len, int timeoutSeconds)
{
	// For non-blocking sockets, use select() to wait for data
	int totalReceived = 0;
	
	while(totalReceived < len) {
		fd_set readSockets;
		#ifdef _WIN32
			readSockets.fd_count = 1;
			readSockets.fd_array[0] = _socket;
		#else
			FD_ZERO(&readSockets);
			FD_SET(_socket, &readSockets);
		#endif
		
		TIMEVAL timeout;
		timeout.tv_sec = timeoutSeconds;
		timeout.tv_usec = 0;
		
		int ready = select((int)_socket + 1, &readSockets, nullptr, nullptr, &timeout);
		
		if(ready <= 0) {
			// Timeout or error
			if(ready == 0) {
				std::cout << "BlockingRecv: Timeout waiting for data" << std::endl;
			} else {
				std::cout << "BlockingRecv: Select error" << std::endl;
			}
			return totalReceived > 0 ? totalReceived : -1;
		}
		
		// Socket has data available
		int received = recv(_socket, buf + totalReceived, len - totalReceived, 0);
		
		if(received > 0) {
			totalReceived += received;
			_bytesReceived += received;
			_lastActivity = std::chrono::steady_clock::now();
		} else if(received == 0) {
			// Connection closed
			std::cout << "BlockingRecv: Connection closed by peer" << std::endl;
			Close();
			return totalReceived > 0 ? totalReceived : 0;
		} else {
			// Error
			int nError = WSAGetLastError();
			if(!WouldBlock(nError)) {
				std::cout << "BlockingRecv: Recv error " << nError << std::endl;
				SetConnectionErrorFlag();
				return totalReceived > 0 ? totalReceived : -1;
			}
			// WouldBlock shouldn't happen after select() says data is ready, but retry
		}
	}
	
	return totalReceived;
}

void Socket::BufferedSend(char *buf, int len)
{
	if(_connectionError || _socket == INVALID_SOCKET) {
		return;
	}

	std::lock_guard<std::mutex> lock(_sendBufferMutex);
	_sendBuffer.insert(_sendBuffer.end(), buf, buf + len);
	// Note: Statistics are updated in SendBuffer() when actually sent
}

void Socket::SendBuffer()
{
	if(_connectionError || _socket == INVALID_SOCKET) {
		return;
	}

	std::lock_guard<std::mutex> lock(_sendBufferMutex);
	
	if(_sendBuffer.empty()) {
		return;
	}

	int totalSent = 0;
	int bufferSize = (int)_sendBuffer.size();
	
	// Update activity timestamp
	_lastActivity = std::chrono::steady_clock::now();
	
	// Wait for socket to be ready to send (non-blocking mode)
	fd_set writeSockets;
	#ifdef _WIN32
		writeSockets.fd_count = 1;
		writeSockets.fd_array[0] = _socket;
	#else
		FD_ZERO(&writeSockets);
		FD_SET(_socket, &writeSockets);
	#endif

	timeval timeout;
	timeout.tv_sec = 5;  // 5 second timeout
	timeout.tv_usec = 0;

	// Check if socket is ready to send
	int ready = select((int)_socket + 1, nullptr, &writeSockets, nullptr, &timeout);
	if(ready <= 0) {
		// Timeout or error - don't set connection error, just skip this send
		std::cout << "[Socket] Not ready to send (select returned " << ready << "), skipping flush" << std::endl;
		return;
	}
	
	while(totalSent < bufferSize) {
		int sent = Send(_sendBuffer.data() + totalSent, bufferSize - totalSent, 0);
		if(sent <= 0) {
			// Error or connection closed
			SetConnectionErrorFlag();
			break;
		}
		totalSent += sent;
	}

	// Update statistics
	_bytesSent += totalSent;
	_messagesSent++;

	// Clear the buffer after sending (or on error)
	_sendBuffer.clear();
}

// Advanced Socket Features Implementation

double Socket::GetConnectionDurationSeconds() const
{
	auto now = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _connectionStart);
	return duration.count() / 1000.0;
}

double Socket::GetBandwidthKBps() const
{
	double duration = GetConnectionDurationSeconds();
	if(duration <= 0) return 0.0;
	
	uint64_t totalBytes = _bytesSent + _bytesReceived;
	return (totalBytes / 1024.0) / duration; // KB per second
}

bool Socket::IsHealthy() const
{
	if(_connectionError || _socket == INVALID_SOCKET) {
		return false;
	}
	
	// Check if connection has been inactive for too long (30 seconds)
	auto now = std::chrono::steady_clock::now();
	auto timeSinceActivity = std::chrono::duration_cast<std::chrono::seconds>(now - _lastActivity);
	return timeSinceActivity.count() < 30;
}

void Socket::ResetStatistics()
{
	_bytesSent = 0;
	_bytesReceived = 0;
	_messagesSent = 0;
	_messagesReceived = 0;
	_connectionStart = std::chrono::steady_clock::now();
	_lastActivity = _connectionStart;
}

size_t Socket::GetBufferedBytes() const
{
	std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(_sendBufferMutex));
	return _sendBuffer.size();
}

bool Socket::ShouldFlush() const
{
	return _sendBuffer.size() >= _maxBufferSize;
}

void Socket::FlushIfNeeded()
{
	if(ShouldFlush()) {
		SendBuffer();
	}
}
