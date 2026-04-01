using System.Net.Sockets;
using System.Net;
using Microsoft.Extensions.Logging;

namespace Diz.Import.Mesen;

/// <summary>
/// TCP client for connecting to Mesen2's DiztinGUIsh streaming server.
/// Handles binary protocol parsing and event-driven message processing.
/// </summary>
public class MesenLiveTraceClient : IDisposable
{
    private readonly ILogger<MesenLiveTraceClient>? _logger;
    private TcpClient? _tcpClient;
    private NetworkStream? _stream;
    private CancellationTokenSource? _cancellationTokenSource;
    private Task? _receiveTask;
    private bool _disposed;

    // Connection state
    public bool IsConnected => _tcpClient?.Connected == true;
    public string? ConnectedHost { get; private set; }
    public int ConnectedPort { get; private set; }
    
    // Statistics
    public long MessagesReceived { get; private set; }
    public long BytesReceived { get; private set; }
    public DateTime? ConnectionTime { get; private set; }
    public TimeSpan? ConnectionDuration => ConnectionTime.HasValue ? DateTime.Now - ConnectionTime.Value : null;

    // Events for different message types
    public event EventHandler<MesenHandshakeMessage>? HandshakeReceived;
    public event EventHandler<MesenExecTraceMessage>? ExecTraceReceived;
    public event EventHandler<MesenCdlUpdateMessage>? CdlUpdateReceived;
    public event EventHandler<MesenCpuStateMessage>? CpuStateReceived;
    public event EventHandler<MesenMemoryDumpMessage>? MemoryDumpReceived;
    public event EventHandler<MesenLabelMessage>? LabelReceived;
    public event EventHandler<MesenFrameMessage>? FrameReceived;
    public event EventHandler<MesenErrorMessage>? ErrorReceived;
    public event EventHandler? Disconnected;
    
    // Configuration
    public int ReceiveTimeoutMs { get; set; } = 5000;
    public int ConnectTimeoutMs { get; set; } = 3000;
    public bool LogRawMessages { get; set; } = false;

    public MesenLiveTraceClient(ILogger<MesenLiveTraceClient>? logger = null)
    {
        _logger = logger;
    }

    /// <summary>
    /// Connect to Mesen2 DiztinGUIsh server.
    /// </summary>
    /// <param name="host">Server hostname/IP (default: localhost)</param>
    /// <param name="port">Server port (default: 9998)</param>
    /// <returns>True if connected successfully</returns>
    public async Task<bool> ConnectAsync(string host = "localhost", int port = 9998)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(MesenLiveTraceClient));

        if (IsConnected)
        {
            _logger?.LogWarning("Already connected to {Host}:{Port}", ConnectedHost, ConnectedPort);
            return true;
        }

        try
        {
            _logger?.LogInformation("Connecting to Mesen2 at {Host}:{Port}...", host, port);
            
            _tcpClient = new TcpClient();
            _tcpClient.ReceiveTimeout = ReceiveTimeoutMs;
            
            // Connect with timeout
            using var timeoutCts = new CancellationTokenSource(ConnectTimeoutMs);
            await _tcpClient.ConnectAsync(host, port, timeoutCts.Token);
            
            _stream = _tcpClient.GetStream();
            ConnectedHost = host;
            ConnectedPort = port;
            ConnectionTime = DateTime.Now;
            
            // Start receive loop
            _cancellationTokenSource = new CancellationTokenSource();
            _receiveTask = ReceiveLoopAsync(_cancellationTokenSource.Token);
            
            _logger?.LogInformation("Connected successfully to {Host}:{Port}", host, port);
            return true;
        }
        catch (Exception ex)
        {
            _logger?.LogError(ex, "Failed to connect to {Host}:{Port}", host, port);
            CleanupConnection();
            return false;
        }
    }

    /// <summary>
    /// Disconnect from server gracefully.
    /// </summary>
    public void Disconnect()
    {
        if (!IsConnected)
            return;

        _logger?.LogInformation("Disconnecting from {Host}:{Port}...", ConnectedHost, ConnectedPort);
        
        _cancellationTokenSource?.Cancel();
        CleanupConnection();
        
        _logger?.LogInformation("Disconnected from server");
        Disconnected?.Invoke(this, EventArgs.Empty);
    }

    /// <summary>
    /// Get connection statistics as formatted string.
    /// </summary>
    public string GetConnectionStats()
    {
        if (!IsConnected)
            return "Not connected";

        var duration = ConnectionDuration?.ToString(@"mm\:ss") ?? "Unknown";
        var bytesPerSec = ConnectionDuration?.TotalSeconds > 0 ? BytesReceived / ConnectionDuration.Value.TotalSeconds : 0;
        var messagesPerSec = ConnectionDuration?.TotalSeconds > 0 ? MessagesReceived / ConnectionDuration.Value.TotalSeconds : 0;
        
        return $"Connected to {ConnectedHost}:{ConnectedPort} for {duration} | " +
               $"Received: {MessagesReceived:N0} messages, {BytesReceived:N0} bytes | " +
               $"Rate: {messagesPerSec:F1} msg/sec, {bytesPerSec:F1} bytes/sec";
    }

    /// <summary>
    /// Main message receive loop. Runs on background thread.
    /// </summary>
    private async Task ReceiveLoopAsync(CancellationToken cancellationToken)
    {
        try
        {
            var messageBuffer = new byte[4096]; // Adjust size as needed
            
            while (!cancellationToken.IsCancellationRequested && IsConnected)
            {
                // Read message header (5 bytes: type + length)
                var headerBytes = await ReadExactBytesAsync(5, cancellationToken);
                if (headerBytes == null)
                    break; // Connection closed
                
                var messageType = (MesenMessageType)headerBytes[0];
                var messageLength = BitConverter.ToUInt32(headerBytes, 1);
                
                if (LogRawMessages)
                    _logger?.LogDebug("Received message: Type={Type}, Length={Length}", messageType, messageLength);
                
                // Read message payload
                byte[]? payloadBytes = null;
                if (messageLength > 0)
                {
                    payloadBytes = await ReadExactBytesAsync((int)messageLength, cancellationToken);
                    if (payloadBytes == null)
                        break; // Connection closed
                }
                
                // Update statistics
                MessagesReceived++;
                BytesReceived += 5 + (payloadBytes?.Length ?? 0);
                
                // Parse and dispatch message
                ProcessMessage(messageType, payloadBytes);
            }
        }
        catch (OperationCanceledException)
        {
            _logger?.LogDebug("Receive loop cancelled");
        }
        catch (Exception ex)
        {
            _logger?.LogError(ex, "Error in receive loop");
        }
        finally
        {
            if (IsConnected)
            {
                _logger?.LogWarning("Receive loop ended unexpectedly, disconnecting");
                Disconnect();
            }
        }
    }

    /// <summary>
    /// Read exact number of bytes from stream with cancellation support.
    /// </summary>
    private async Task<byte[]?> ReadExactBytesAsync(int count, CancellationToken cancellationToken)
    {
        if (_stream == null || count <= 0)
            return null;

        var buffer = new byte[count];
        var totalRead = 0;

        while (totalRead < count && !cancellationToken.IsCancellationRequested)
        {
            var bytesRead = await _stream.ReadAsync(buffer.AsMemory(totalRead, count - totalRead), cancellationToken);
            if (bytesRead == 0)
            {
                _logger?.LogWarning("Connection closed while reading {Count} bytes (got {TotalRead})", count, totalRead);
                return null; // Connection closed
            }
            totalRead += bytesRead;
        }

        return totalRead == count ? buffer : null;
    }

    /// <summary>
    /// Parse binary message and raise appropriate event.
    /// </summary>
    private void ProcessMessage(MesenMessageType messageType, byte[]? payload)
    {
        try
        {
            switch (messageType)
            {
                case MesenMessageType.Handshake:
                    if (payload != null && payload.Length >= 21) // Expected handshake size
                    {
                        var handshake = ParseHandshakeMessage(payload);
                        _logger?.LogInformation("Handshake: {Message}", handshake);
                        HandshakeReceived?.Invoke(this, handshake);
                    }
                    break;

                case MesenMessageType.ExecTrace:
                    if (payload != null && payload.Length >= 14) // Expected trace size
                    {
                        var trace = ParseExecTraceMessage(payload);
                        if (LogRawMessages)
                            _logger?.LogDebug("ExecTrace: {Message}", trace);
                        ExecTraceReceived?.Invoke(this, trace);
                    }
                    break;

                case MesenMessageType.CdlUpdate:
                    if (payload != null && payload.Length >= 5) // Expected CDL size
                    {
                        var cdl = ParseCdlUpdateMessage(payload);
                        if (LogRawMessages)
                            _logger?.LogDebug("CdlUpdate: {Message}", cdl);
                        CdlUpdateReceived?.Invoke(this, cdl);
                    }
                    break;

                case MesenMessageType.CpuState:
                    if (payload != null && payload.Length >= 17) // Expected CPU state size
                    {
                        var cpuState = ParseCpuStateMessage(payload);
                        if (LogRawMessages)
                            _logger?.LogDebug("CpuState: {Message}", cpuState);
                        CpuStateReceived?.Invoke(this, cpuState);
                    }
                    break;

                case MesenMessageType.FrameStart:
                case MesenMessageType.FrameEnd:
                    if (payload != null && payload.Length >= 4)
                    {
                        var frame = ParseFrameMessage(payload, messageType == MesenMessageType.FrameStart);
                        if (LogRawMessages)
                            _logger?.LogDebug("Frame: {Message}", frame);
                        FrameReceived?.Invoke(this, frame);
                    }
                    break;

                case MesenMessageType.Error:
                    if (payload != null && payload.Length >= 2)
                    {
                        var error = ParseErrorMessage(payload);
                        _logger?.LogError("Server error: {Message}", error);
                        ErrorReceived?.Invoke(this, error);
                    }
                    break;

                default:
                    _logger?.LogWarning("Unknown message type: {Type}", messageType);
                    break;
            }
        }
        catch (Exception ex)
        {
            _logger?.LogError(ex, "Error processing message type {Type}", messageType);
        }
    }

    /// <summary>
    /// Parse handshake message from binary data.
    /// </summary>
    private static MesenHandshakeMessage ParseHandshakeMessage(byte[] data)
    {
        return new MesenHandshakeMessage
        {
            ProtocolVersion = data[0],
            EmulatorVersionMajor = BitConverter.ToUInt32(data, 1),
            EmulatorVersionMinor = BitConverter.ToUInt32(data, 5),
            EmulatorVersionPatch = BitConverter.ToUInt32(data, 9),
            ServerPort = BitConverter.ToUInt16(data, 13),
            RomSize = BitConverter.ToUInt32(data, 15),
            RomCrc32 = BitConverter.ToUInt32(data, 19)
        };
    }

    /// <summary>
    /// Parse execution trace message from binary data.
    /// </summary>
    private static MesenExecTraceMessage ParseExecTraceMessage(byte[] data)
    {
        return new MesenExecTraceMessage
        {
            PC = BitConverter.ToUInt32(data, 0) & 0xFFFFFF, // 24-bit address
            Opcode = data[4],
            MFlag = data[5] != 0,
            XFlag = data[6] != 0,
            DataBank = data[7],
            DirectPage = BitConverter.ToUInt16(data, 8),
            EffectiveAddr = BitConverter.ToUInt32(data, 10) & 0xFFFFFF // 24-bit address
        };
    }

    /// <summary>
    /// Parse CDL update message from binary data.
    /// </summary>
    private static MesenCdlUpdateMessage ParseCdlUpdateMessage(byte[] data)
    {
        return new MesenCdlUpdateMessage
        {
            Address = BitConverter.ToUInt32(data, 0) & 0xFFFFFF, // 24-bit address
            CdlFlags = data[4]
        };
    }

    /// <summary>
    /// Parse CPU state message from binary data.
    /// </summary>
    private static MesenCpuStateMessage ParseCpuStateMessage(byte[] data)
    {
        return new MesenCpuStateMessage
        {
            PC = BitConverter.ToUInt32(data, 0) & 0xFFFFFF, // 24-bit
            A = BitConverter.ToUInt16(data, 4),
            X = BitConverter.ToUInt16(data, 6),
            Y = BitConverter.ToUInt16(data, 8),
            SP = BitConverter.ToUInt16(data, 10),
            ProcessorFlags = data[12],
            DataBank = data[13],
            DirectPage = BitConverter.ToUInt16(data, 14),
            EmulationMode = data[16] != 0
        };
    }

    /// <summary>
    /// Parse frame message from binary data.
    /// </summary>
    private static MesenFrameMessage ParseFrameMessage(byte[] data, bool isStart)
    {
        return new MesenFrameMessage
        {
            FrameNumber = BitConverter.ToUInt32(data, 0),
            IsStart = isStart
        };
    }

    /// <summary>
    /// Parse error message from binary data.
    /// </summary>
    private static MesenErrorMessage ParseErrorMessage(byte[] data)
    {
        var errorCode = BitConverter.ToUInt16(data, 0);
        var errorText = System.Text.Encoding.UTF8.GetString(data, 2, data.Length - 2);
        
        return new MesenErrorMessage
        {
            ErrorCode = errorCode,
            ErrorText = errorText
        };
    }

    /// <summary>
    /// Clean up connection resources.
    /// </summary>
    private void CleanupConnection()
    {
        try
        {
            _cancellationTokenSource?.Cancel();
            _stream?.Close();
            _tcpClient?.Close();
        }
        catch (Exception ex)
        {
            _logger?.LogDebug(ex, "Error during connection cleanup");
        }
        finally
        {
            _stream?.Dispose();
            _tcpClient?.Dispose();
            _cancellationTokenSource?.Dispose();
            
            _stream = null;
            _tcpClient = null;
            _cancellationTokenSource = null;
            _receiveTask = null;
            ConnectedHost = null;
            ConnectedPort = 0;
            ConnectionTime = null;
        }
    }

    public void Dispose()
    {
        if (!_disposed)
        {
            Disconnect();
            _disposed = true;
        }
        GC.SuppressFinalize(this);
    }
}