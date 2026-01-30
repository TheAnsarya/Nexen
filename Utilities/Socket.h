#pragma once

#include "pch.h"

/// <summary>
/// Cross-platform socket wrapper for TCP/IP networking.
/// Provides unified interface for Windows (Winsock) and POSIX (Berkeley sockets).
/// </summary>
/// <remarks>
/// Platform handling:
/// - Windows: Initializes/cleans up WSA (Windows Sockets API)
/// - Linux/macOS: Uses standard POSIX sockets
///
/// Features:
/// - Client connections (Connect)
/// - Server listening (Bind, Listen, Accept)
/// - Data transfer (Send, Recv, buffered sending)
/// - Error tracking
/// - Optional UPnP port mapping integration
///
/// Common usage (server):
/// <code>
/// Socket listener;
/// listener.Bind(12345);
/// listener.Listen(5);
/// auto client = listener.Accept();
/// </code>
///
/// Common usage (client):
/// <code>
/// Socket socket;
/// if (socket.Connect("localhost", 12345)) {
///     socket.Send(data, size, 0);
/// }
/// </code>
///
/// Thread safety: Not thread-safe - use separate socket per thread or external synchronization.
/// </remarks>
class Socket {
private:
#ifdef _WIN32
	bool _cleanupWSA = false; ///< Track if WSA needs cleanup on Windows
#endif

	uintptr_t _socket = (uintptr_t)~0; ///< Socket handle (SOCKET on Windows, int on POSIX)
	bool _connectionError = false;     ///< Connection error flag
	int32_t _UPnPPort = -1;            ///< UPnP mapped port (-1 if none)

public:
	/// <summary>Construct new socket and initialize platform networking</summary>
	Socket();

	/// <summary>Construct socket from existing handle (for Accept)</summary>
	/// <param name="socket">Existing socket handle</param>
	Socket(uintptr_t socket);

	/// <summary>Destructor - closes socket and cleans up resources</summary>
	~Socket();

	/// <summary>Configure socket options (nodelay, keepalive, etc.)</summary>
	void SetSocketOptions();

	/// <summary>Mark socket as having connection error</summary>
	void SetConnectionErrorFlag();

	/// <summary>Close socket and release handle</summary>
	void Close();

	/// <summary>Check if connection error occurred</summary>
	/// <returns>True if connection error flagged</returns>
	bool ConnectionError();

	/// <summary>
	/// Bind socket to local port for listening.
	/// </summary>
	/// <param name="port">Local port number to bind</param>
	/// <remarks>Call before Listen() for server sockets</remarks>
	void Bind(uint16_t port);

	/// <summary>
	/// Connect to remote host.
	/// </summary>
	/// <param name="hostname">Hostname or IP address</param>
	/// <param name="port">Remote port number</param>
	/// <returns>True if connection succeeded</returns>
	bool Connect(const char* hostname, uint16_t port);

	/// <summary>
	/// Start listening for incoming connections.
	/// </summary>
	/// <param name="backlog">Maximum pending connection queue size</param>
	void Listen(int backlog);

	/// <summary>
	/// Accept incoming connection (blocking).
	/// </summary>
	/// <returns>New socket for accepted connection, or nullptr on error</returns>
	unique_ptr<Socket> Accept();

	/// <summary>
	/// Send data over socket.
	/// </summary>
	/// <param name="buf">Data buffer to send</param>
	/// <param name="len">Number of bytes to send</param>
	/// <param name="flags">Socket flags (MSG_NOSIGNAL, etc.)</param>
	/// <returns>Number of bytes sent, or -1 on error</returns>
	int Send(char* buf, int len, int flags);

	/// <summary>
	/// Buffer data for later sending (batching optimization).
	/// </summary>
	/// <param name="buf">Data to buffer</param>
	/// <param name="len">Number of bytes to buffer</param>
	/// <remarks>Call SendBuffer() to flush buffered data</remarks>
	void BufferedSend(char* buf, int len);

	/// <summary>Flush buffered send data</summary>
	void SendBuffer();

	/// <summary>
	/// Receive data from socket.
	/// </summary>
	/// <param name="buf">Buffer for received data</param>
	/// <param name="len">Maximum bytes to receive</param>
	/// <param name="flags">Socket flags</param>
	/// <returns>Number of bytes received, 0 on disconnect, -1 on error</returns>
	int Recv(char* buf, int len, int flags);
};
