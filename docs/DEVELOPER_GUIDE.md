# DiztinGUIsh-Mesen2 Integration: Developer Guide

## 📋 Table of Contents

1. [Development Environment Setup](#development-environment-setup)
2. [Architecture Overview](#architecture-overview)
3. [Implementation Patterns](#implementation-patterns)
4. [Testing Strategies](#testing-strategies)
5. [Debugging Techniques](#debugging-techniques)
6. [Performance Optimization](#performance-optimization)
7. [Deployment Procedures](#deployment-procedures)
8. [Contributing Guidelines](#contributing-guidelines)

---

## 💻 Development Environment Setup

### Prerequisites

#### Required Software
```powershell
# Visual Studio 2022 with C++ and C# workloads
# .NET 9.0 SDK or later
# Git for version control
# PowerShell 5.1 or PowerShell Core 7.0+

# Verify installations
dotnet --version                    # Should show 9.0 or later
git --version                      # Any recent version
$PSVersionTable.PSVersion          # Should show 5.1+ or 7.0+
```

#### Optional but Recommended
- **JetBrains Rider** or **Visual Studio Code** for cross-platform development
- **Windows Terminal** for improved PowerShell experience
- **Postman** or **Insomnia** for API testing (if implementing REST endpoints)
- **WireShark** for network protocol debugging

### Repository Setup

#### Clone and Initialize
```powershell
# Clone both repositories
git clone https://github.com/TheAnsarya/Mesen2.git
git clone https://github.com/TheAnsarya/DiztinGUIsh.git

# Set up development branches
cd Mesen2
git checkout -b feature/diztinguish-integration
cd ..\DiztinGUIsh
git checkout -b feature/mesen2-integration
```

#### Build Verification
```powershell
# Build Mesen2 (C++)
cd Mesen2
msbuild Mesen.sln /p:Configuration=Debug /p:Platform=x64

# Build DiztinGUIsh (C#)
cd DiztinGUIsh
dotnet build --configuration Debug

# Verify no compilation errors
echo "Build verification complete"
```

### Development Tools Configuration

#### Visual Studio Setup
1. **Load Solutions**: Open both `Mesen.sln` and `DiztinGUIsh.sln`
2. **Configure Debugging**: Set multiple startup projects
3. **Enable Native/Managed Debugging**: For cross-platform debugging
4. **Install Extensions**: 
   - ReSharper (optional, for C# development)
   - Visual Assist (optional, for C++ development)

#### Git Configuration
```powershell
# Configure git for the project
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Set up useful aliases
git config --global alias.co checkout
git config --global alias.br branch
git config --global alias.ci commit
git config --global alias.st status
```

---

## 🏗️ Architecture Overview

### High-Level Architecture

```
┌─────────────────┐    TCP/IP Socket    ┌─────────────────┐
│     Mesen2      │◄─────────────────────►│   DiztinGUIsh   │
│   (C++ Server)  │    Protocol V2.0     │   (C# Client)   │
│                 │                      │                 │
│ ┌─────────────┐ │                      │ ┌─────────────┐ │
│ │DiztinBridge │ │                      │ │Mesen2Client │ │
│ │   Core      │ │                      │ │  Services   │ │
│ └─────────────┘ │                      │ └─────────────┘ │
│ ┌─────────────┐ │                      │ ┌─────────────┐ │
│ │   Socket    │ │                      │ │    UI       │ │
│ │   Layer     │ │                      │ │  Components │ │
│ └─────────────┘ │                      │ └─────────────┘ │
└─────────────────┘                      └─────────────────┘
```

### Component Breakdown

#### Mesen2 Side Architecture (C++)

The Mesen2 integration follows a layered architecture where each component has distinct responsibilities:

**DiztinguishBridge Layer**
Serves as the primary integration orchestrator in `DiztinguishBridge.cpp`. This component manages the server lifecycle, from initialization through shutdown. When Mesen2 executes SNES instructions, the bridge captures CPU state changes and formats them according to Protocol V2.0 specifications. Message routing occurs through a centralized dispatcher that validates incoming requests and delegates to appropriate handlers.

**Socket Communication Layer**
Implemented in `Socket.cpp`, this layer provides enterprise-grade networking capabilities. Beyond basic TCP/IP connectivity, it implements connection monitoring with real-time bandwidth calculation and health checking. The socket layer automatically handles connection persistence, detecting and recovering from network interruptions without data loss.

**Protocol Definition Layer**
Defined in `DiztinguishProtocolV2.h`, this establishes the communication contract between Mesen2 and DiztinGUIsh. The protocol supports structured message types for commands and responses, ensuring reliable bidirectional communication. Built-in message sequencing prevents out-of-order processing, while checksums guarantee data integrity during transmission.

#### DiztinGUIsh Side Architecture (C#)

The DiztinGUIsh integration leverages modern .NET patterns with dependency injection and service-oriented architecture:

**Client Implementation Layer**
The `Mesen2StreamingClient` in namespace `Diz.Core.Mesen2` manages the client-side connection lifecycle. This component handles protocol negotiation, maintains connection state, and provides an event-driven interface for real-time data reception. The implementation uses async/await patterns throughout to ensure UI responsiveness during high-throughput debugging sessions.

**Configuration Management Layer**
The `Mesen2Configuration` service manages persistent settings using .NET's configuration system. It provides validation logic to ensure network parameters are within acceptable ranges and supports change notifications for dynamic reconfiguration during active sessions. Settings are automatically persisted across application restarts.

**Factory and Lifecycle Management**
The `Mesen2StreamingClientFactory` integrates with the dependency injection container to provide proper service instantiation. It applies configuration settings during client creation and ensures appropriate resource cleanup when connections are disposed.

**Integration Controller Layer**
Implemented in `Diz.Controllers.Services`, the `Mesen2IntegrationController` provides high-level orchestration of the integration workflow. This component coordinates project-level operations like label synchronization, breakpoint management, and trace data integration with DiztinGUIsh's analysis engine.

### Data Flow Patterns

#### Real-Time Trace Streaming
```
1. Mesen2 executes instruction
2. DiztinguishBridge captures CPU state
3. Protocol V2.0 serializes data
4. Socket layer transmits to DiztinGUIsh
5. Mesen2StreamingClient receives and deserializes
6. Events fire to update UI components
7. TraceViewer displays real-time execution
```

#### Breakpoint Management
```
1. User sets breakpoint in DiztinGUIsh UI
2. Mesen2IntegrationController processes request
3. Client sends ADD_BREAKPOINT command
4. Mesen2 receives and configures breakpoint
5. Acknowledgment sent back to DiztinGUIsh
6. UI updates to reflect active breakpoint
```

---

## 🔧 Implementation Patterns

### Error Handling Patterns

#### Mesen2 Error Recovery (C++)

Mesen2's error handling follows a defensive programming approach where each layer provides specific error recovery mechanisms:

**Message Processing Resilience**
The `DiztinguishBridge::ProcessMessage()` method implements comprehensive exception handling around all message processing operations. When invalid messages are received, the system logs detailed error information including message type, payload size, and checksum data. Rather than crashing, the bridge sends structured error responses back to DiztinGUIsh, allowing the client to understand and potentially recover from the error condition.

**Network Layer Protection**
The Socket layer implements connection monitoring that detects various failure modes. When network interruptions occur, the system maintains connection metadata and attempts automatic reconnection using exponential backoff timing. This prevents rapid reconnection attempts that could overwhelm system resources while ensuring reliable service restoration.

#### DiztinGUIsh Error Management (C#)

**Polly Integration for Resilience**
DiztinGUIsh uses the Polly library to implement sophisticated retry patterns in the `RobustMesen2Client` wrapper. The retry policy configuration includes specific exception types that warrant retry attempts versus those that should fail immediately. This distinction ensures that transient network issues don't interrupt debugging sessions while permanent configuration errors are reported immediately.

**Circuit Breaker Protection**
The circuit breaker pattern prevents cascading failures when Mesen2 becomes unavailable. After a configurable number of consecutive failures, the circuit opens and prevents further connection attempts for a specified duration. This allows Mesen2 time to recover while providing clear feedback to users about system availability.

**Structured Logging and Diagnostics**
Error handling integrates with .NET's logging infrastructure to provide comprehensive diagnostic information. Log entries include correlation IDs for tracking related operations, performance timing data, and structured data that enables automated log analysis for identifying recurring issues.

### Configuration Management Pattern

#### Centralized Configuration
```csharp
public class Mesen2IntegrationSettings
{
    public NetworkSettings Network { get; set; } = new();
    public PerformanceSettings Performance { get; set; } = new();
    public DebugSettings Debug { get; set; } = new();
    public UISettings UI { get; set; } = new();
}

public class NetworkSettings
{
    public string DefaultHost { get; set; } = "localhost";
    public int DefaultPort { get; set; } = 1234;
    public int ConnectionTimeoutMs { get; set; } = 5000;
    public bool AutoReconnectEnabled { get; set; } = true;
    public int MaxReconnectAttempts { get; set; } = 5;
    public int ReconnectDelayMs { get; set; } = 3000;
}

public class PerformanceSettings
{
    public int TraceBufferSize { get; set; } = 10000;
    public bool EnableTraceCompression { get; set; } = true;
    public int MaxTracesPerSecond { get; set; } = 1000;
    public int MemoryDumpChunkSize { get; set; } = 8192;
}

// Configuration validation
public class Mesen2SettingsValidator : AbstractValidator<Mesen2IntegrationSettings>
{
    public Mesen2SettingsValidator()
    {
        RuleFor(x => x.Network.DefaultPort)
            .InclusiveBetween(1, 65535)
            .WithMessage("Port must be between 1 and 65535");
            
        RuleFor(x => x.Network.ConnectionTimeoutMs)
            .GreaterThanOrEqualTo(1000)
            .WithMessage("Connection timeout must be at least 1000ms");
            
        RuleFor(x => x.Performance.TraceBufferSize)
            .GreaterThan(0)
            .LessThanOrEqualTo(1000000)
            .WithMessage("Trace buffer size must be between 1 and 1,000,000");
    }
}
```

### Event-Driven Communication Pattern

#### Publisher-Subscriber Implementation
```csharp
public interface IMesen2EventBus
{
    Task PublishAsync<T>(T @event) where T : class;
    void Subscribe<T>(Func<T, Task> handler) where T : class;
    void Unsubscribe<T>(Func<T, Task> handler) where T : class;
}

// Event definitions
public record CpuStateChangedEvent(CpuStateData CpuState, DateTime Timestamp);
public record TraceDataReceivedEvent(TraceEntry[] Traces, DateTime Timestamp);
public record ConnectionStatusChangedEvent(ConnectionStatus Status, string Message);

// Event handlers
public class TraceViewEventHandler
{
    private readonly ILogger<TraceViewEventHandler> _logger;
    
    public TraceViewEventHandler(ILogger<TraceViewEventHandler> logger)
    {
        _logger = logger;
    }
    
    public async Task HandleTraceDataReceived(TraceDataReceivedEvent @event)
    {
        try
        {
            // Update trace view with new data
            await UpdateTraceView(@event.Traces);
            
            // Update performance metrics
            await UpdatePerformanceMetrics(@event.Traces.Length, @event.Timestamp);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Failed to handle trace data received event");
        }
    }
    
    private async Task UpdateTraceView(TraceEntry[] traces)
    {
        // Implementation for updating the UI
        await Task.CompletedTask;
    }
    
    private async Task UpdatePerformanceMetrics(int traceCount, DateTime timestamp)
    {
        // Update performance statistics
        await Task.CompletedTask;
    }
}
```

---

## 🧪 Testing Strategies

### Unit Testing Approach

#### Testing Mesen2 Components (C++)
```cpp
#include <gtest/gtest.h>
#include "Core/Debugger/DiztinguishBridge.h"

class DiztinguishBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        bridge = std::make_unique<DiztinguishBridge>();
        mockSocket = std::make_shared<MockSocket>();
    }
    
    void TearDown() override {
        bridge.reset();
    }
    
    std::unique_ptr<DiztinguishBridge> bridge;
    std::shared_ptr<MockSocket> mockSocket;
};

TEST_F(DiztinguishBridgeTest, HandlesCpuStateRequestCorrectly) {
    // Arrange
    ProtocolMessage request;
    request.type = MessageType::RequestCpuState;
    request.sequenceNumber = 12345;
    
    // Act
    bool result = bridge->ProcessMessage(request);
    
    // Assert
    EXPECT_TRUE(result);
    
    // Verify CPU state was sent
    auto sentMessages = mockSocket->GetSentMessages();
    ASSERT_EQ(1, sentMessages.size());
    EXPECT_EQ(MessageType::CpuStateResponse, sentMessages[0].type);
    EXPECT_EQ(12345, sentMessages[0].sequenceNumber);
}

TEST_F(DiztinguishBridgeTest, HandlesInvalidMessageGracefully) {
    // Arrange
    ProtocolMessage invalidMessage;
    invalidMessage.type = static_cast<MessageType>(255); // Invalid type
    
    // Act
    bool result = bridge->ProcessMessage(invalidMessage);
    
    // Assert
    EXPECT_FALSE(result);
    
    // Verify error response was sent
    auto sentMessages = mockSocket->GetSentMessages();
    ASSERT_EQ(1, sentMessages.size());
    EXPECT_EQ(MessageType::ErrorResponse, sentMessages[0].type);
}
```

#### Testing DiztinGUIsh Components (C#)
```csharp
using Xunit;
using Moq;
using Microsoft.Extensions.Logging;

public class Mesen2StreamingClientTests
{
    private readonly Mock<ILogger<Mesen2StreamingClient>> _mockLogger;
    private readonly Mock<IMesen2Configuration> _mockConfig;
    private readonly Mesen2StreamingClient _client;
    
    public Mesen2StreamingClientTests()
    {
        _mockLogger = new Mock<ILogger<Mesen2StreamingClient>>();
        _mockConfig = new Mock<IMesen2Configuration>();
        
        _client = new Mesen2StreamingClient(_mockLogger.Object, _mockConfig.Object);
    }
    
    [Fact]
    public async Task ConnectAsync_WithValidHostAndPort_ReturnsTrue()
    {
        // Arrange
        _mockConfig.Setup(x => x.ConnectionTimeoutMs).Returns(5000);
        
        // Act
        var result = await _client.ConnectAsync("localhost", 1234);
        
        // Assert
        Assert.True(result);
        Assert.True(_client.IsConnected);
    }
    
    [Fact]
    public async Task ConnectAsync_WithInvalidHost_ReturnsFalse()
    {
        // Arrange
        _mockConfig.Setup(x => x.ConnectionTimeoutMs).Returns(5000);
        
        // Act
        var result = await _client.ConnectAsync("invalid-host", 1234);
        
        // Assert
        Assert.False(result);
        Assert.False(_client.IsConnected);
    }
    
    [Fact]
    public async Task RequestCpuStateAsync_WhenConnected_SendsCorrectMessage()
    {
        // Arrange
        await _client.ConnectAsync("localhost", 1234);
        
        // Act
        await _client.RequestCpuStateAsync();
        
        // Assert
        // Verify the correct message was sent (would require access to internal state or mock)
        // This might require dependency injection of a socket abstraction for testing
    }
}
```

### Integration Testing

#### End-to-End Integration Test
```csharp
public class Mesen2IntegrationE2ETests : IClassFixture<IntegrationTestFixture>
{
    private readonly IntegrationTestFixture _fixture;
    
    public Mesen2IntegrationE2ETests(IntegrationTestFixture fixture)
    {
        _fixture = fixture;
    }
    
    [Fact]
    public async Task FullIntegrationWorkflow_WithMockMesen2_WorksCorrectly()
    {
        // Arrange
        var mockMesen2Server = new MockMesen2Server();
        await mockMesen2Server.StartAsync(1234);
        
        var client = _fixture.ServiceProvider.GetRequiredService<IMesen2StreamingClient>();
        
        var cpuStateReceived = false;
        var receivedCpuState = default(CpuStateData);
        
        client.CpuStateReceived += (sender, e) => {
            cpuStateReceived = true;
            receivedCpuState = e.CpuState;
        };
        
        try
        {
            // Act
            var connected = await client.ConnectAsync("localhost", 1234);
            Assert.True(connected);
            
            await client.RequestCpuStateAsync();
            
            // Wait for response
            await Task.Delay(1000);
            
            // Assert
            Assert.True(cpuStateReceived);
            Assert.NotEqual(0u, receivedCpuState.PC);
        }
        finally
        {
            await client.DisconnectAsync();
            await mockMesen2Server.StopAsync();
        }
    }
}

public class IntegrationTestFixture
{
    public IServiceProvider ServiceProvider { get; }
    
    public IntegrationTestFixture()
    {
        var services = new ServiceCollection();
        
        // Register real implementations for integration testing
        services.RegisterMesen2Integration();
        
        // Override specific services for testing
        services.AddScoped<IMesen2Configuration, TestMesen2Configuration>();
        
        ServiceProvider = services.BuildServiceProvider();
    }
}
```

### Performance Testing

#### Load Testing Example
```csharp
public class PerformanceTests
{
    [Fact]
    public async Task TraceProcessing_WithHighVolume_MaintainsPerformance()
    {
        // Arrange
        var client = new Mesen2StreamingClient(Mock.Of<ILogger<Mesen2StreamingClient>>(), new TestConfiguration());
        var mockServer = new MockMesen2Server();
        
        var tracesProcessed = 0;
        var stopwatch = Stopwatch.StartNew();
        
        client.TraceReceived += (sender, e) => {
            Interlocked.Add(ref tracesProcessed, e.Traces.Length);
        };
        
        await mockServer.StartAsync(1234);
        await client.ConnectAsync("localhost", 1234);
        
        // Act - Generate high volume of traces
        const int targetTraces = 100000;
        await mockServer.GenerateTraces(targetTraces, TimeSpan.FromSeconds(10));
        
        // Wait for processing to complete
        while (tracesProcessed < targetTraces && stopwatch.Elapsed < TimeSpan.FromSeconds(30))
        {
            await Task.Delay(100);
        }
        
        stopwatch.Stop();
        
        // Assert
        Assert.True(tracesProcessed >= targetTraces * 0.95, "Should process at least 95% of traces");
        Assert.True(stopwatch.Elapsed < TimeSpan.FromSeconds(15), "Should complete within 15 seconds");
        
        var tracesPerSecond = tracesProcessed / stopwatch.Elapsed.TotalSeconds;
        Assert.True(tracesPerSecond > 5000, $"Should process >5000 traces/sec, actual: {tracesPerSecond:F0}");
    }
}
```

---

## 🐛 Debugging Techniques

### Diagnostic Logging

#### Structured Logging Setup
```csharp
public static class LoggingExtensions
{
    public static IServiceCollection AddMesen2Logging(this IServiceCollection services)
    {
        return services.AddLogging(builder =>
        {
            builder.ClearProviders();
            
            // Console logging for development
            builder.AddConsole(options =>
            {
                options.IncludeScopes = true;
                options.TimestampFormat = "yyyy-MM-dd HH:mm:ss.fff ";
            });
            
            // File logging for debugging
            builder.AddFile("Logs/mesen2-integration-{Date}.log", options =>
            {
                options.MinLevel = LogLevel.Debug;
                options.MaxFileSize = 10 * 1024 * 1024; // 10MB
                options.MaxRollingFiles = 5;
            });
            
            // Structured logging with Serilog
            builder.AddSerilog(configuration =>
            {
                configuration
                    .MinimumLevel.Debug()
                    .WriteTo.Console(outputTemplate: 
                        "[{Timestamp:HH:mm:ss} {Level:u3}] [{SourceContext}] {Message:lj}{NewLine}{Exception}")
                    .WriteTo.File("Logs/mesen2-integration-.log",
                        rollingInterval: RollingInterval.Day,
                        retainedFileCountLimit: 7);
            });
        });
    }
}

// Usage in components
public class Mesen2StreamingClient
{
    private readonly ILogger<Mesen2StreamingClient> _logger;
    
    public async Task<bool> ConnectAsync(string host, int port, CancellationToken cancellationToken = default)
    {
        using var scope = _logger.BeginScope("Connection attempt to {Host}:{Port}", host, port);
        
        _logger.LogInformation("Starting connection attempt");
        
        try
        {
            // Connection logic
            var result = await ConnectInternal(host, port, cancellationToken);
            
            _logger.LogInformation("Connection attempt completed with result: {Result}", result);
            return result;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Connection attempt failed");
            throw;
        }
    }
}
```

### Network Traffic Analysis

#### Packet Capture Integration
```csharp
public class NetworkDiagnostics
{
    public static void EnablePacketCapture(string outputFile)
    {
        // Integration with packet capture libraries or external tools
        var captureProcess = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "netsh",
                Arguments = $"trace start capture=yes tracefile={outputFile} provider=Microsoft-Windows-TCPIP",
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        
        captureProcess.Start();
    }
    
    public static void DisablePacketCapture()
    {
        var stopProcess = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = "netsh",
                Arguments = "trace stop",
                UseShellExecute = false,
                CreateNoWindow = true
            }
        };
        
        stopProcess.Start();
        stopProcess.WaitForExit();
    }
}

// Usage
public async Task DiagnoseConnectionIssue()
{
    NetworkDiagnostics.EnablePacketCapture("connection-debug.etl");
    
    try
    {
        await client.ConnectAsync("localhost", 1234);
        // Perform operations that exhibit the issue
    }
    finally
    {
        NetworkDiagnostics.DisablePacketCapture();
        Console.WriteLine("Packet capture saved to connection-debug.etl");
    }
}
```

### Memory Profiling

#### Memory Usage Monitoring
```csharp
public class MemoryProfiler
{
    private readonly Timer _monitoringTimer;
    private readonly ILogger<MemoryProfiler> _logger;
    
    public MemoryProfiler(ILogger<MemoryProfiler> logger)
    {
        _logger = logger;
        _monitoringTimer = new Timer(LogMemoryUsage, null, TimeSpan.FromSeconds(10), TimeSpan.FromSeconds(10));
    }
    
    private void LogMemoryUsage(object state)
    {
        var currentProcess = Process.GetCurrentProcess();
        var workingSet = currentProcess.WorkingSet64;
        var privateMemory = currentProcess.PrivateMemorySize64;
        
        _logger.LogDebug(
            "Memory Usage - Working Set: {WorkingSetMB} MB, Private: {PrivateMB} MB", 
            workingSet / 1024 / 1024,
            privateMemory / 1024 / 1024);
        
        // Check for potential memory leaks
        if (workingSet > 500 * 1024 * 1024) // 500 MB threshold
        {
            _logger.LogWarning("High memory usage detected: {WorkingSetMB} MB", workingSet / 1024 / 1024);
            
            // Trigger garbage collection
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }
    }
}
```

---

## ⚡ Performance Optimization

### Asynchronous Processing Patterns

#### High-Throughput Message Processing
```csharp
public class HighPerformanceMessageProcessor
{
    private readonly Channel<Mesen2Message> _messageChannel;
    private readonly ChannelWriter<Mesen2Message> _messageWriter;
    private readonly ChannelReader<Mesen2Message> _messageReader;
    
    public HighPerformanceMessageProcessor()
    {
        var options = new BoundedChannelOptions(10000)
        {
            FullMode = BoundedChannelFullMode.Wait,
            SingleReader = true,
            SingleWriter = false
        };
        
        _messageChannel = Channel.CreateBounded<Mesen2Message>(options);
        _messageWriter = _messageChannel.Writer;
        _messageReader = _messageChannel.Reader;
    }
    
    public async Task StartProcessing(CancellationToken cancellationToken)
    {
        await foreach (var message in _messageReader.ReadAllAsync(cancellationToken))
        {
            await ProcessMessageOptimized(message);
        }
    }
    
    public async Task<bool> QueueMessage(Mesen2Message message, CancellationToken cancellationToken = default)
    {
        return await _messageWriter.WriteAsync(message, cancellationToken);
    }
    
    private async Task ProcessMessageOptimized(Mesen2Message message)
    {
        // Use object pooling for frequently allocated objects
        var processor = _processorPool.Get();
        
        try
        {
            await processor.ProcessAsync(message);
        }
        finally
        {
            _processorPool.Return(processor);
        }
    }
}
```

### Memory Optimization

#### Object Pooling Implementation
```csharp
public class MessageProcessorPool : ObjectPool<MessageProcessor>
{
    private readonly ConcurrentQueue<MessageProcessor> _pool = new();
    private readonly Func<MessageProcessor> _factory;
    private int _currentCount = 0;
    private const int MaxPoolSize = 100;
    
    public MessageProcessorPool(Func<MessageProcessor> factory)
    {
        _factory = factory;
    }
    
    public override MessageProcessor Get()
    {
        if (_pool.TryDequeue(out var processor))
        {
            Interlocked.Decrement(ref _currentCount);
            return processor;
        }
        
        return _factory();
    }
    
    public override void Return(MessageProcessor processor)
    {
        if (_currentCount < MaxPoolSize)
        {
            processor.Reset(); // Clear state for reuse
            _pool.Enqueue(processor);
            Interlocked.Increment(ref _currentCount);
        }
        else
        {
            // Pool is full, dispose the object
            processor.Dispose();
        }
    }
}

// Usage with DI
public static void RegisterObjectPools(this IServiceCollection services)
{
    services.AddSingleton<ObjectPool<MessageProcessor>>(provider =>
        new MessageProcessorPool(() => new MessageProcessor()));
    
    services.AddSingleton<ObjectPool<ByteArrayBuffer>>(provider =>
        new DefaultObjectPool<ByteArrayBuffer>(new ByteArrayBufferPooledObjectPolicy()));
}
```

### Network Optimization

#### Connection Pooling
```csharp
public class Mesen2ConnectionPool : IDisposable
{
    private readonly SemaphoreSlim _connectionSemaphore;
    private readonly ConcurrentQueue<IMesen2StreamingClient> _availableConnections;
    private readonly List<IMesen2StreamingClient> _allConnections;
    private readonly IMesen2StreamingClientFactory _clientFactory;
    private readonly int _maxConnections;
    
    public Mesen2ConnectionPool(IMesen2StreamingClientFactory clientFactory, int maxConnections = 5)
    {
        _clientFactory = clientFactory;
        _maxConnections = maxConnections;
        _connectionSemaphore = new SemaphoreSlim(maxConnections, maxConnections);
        _availableConnections = new ConcurrentQueue<IMesen2StreamingClient>();
        _allConnections = new List<IMesen2StreamingClient>();
    }
    
    public async Task<IMesen2StreamingClient> GetConnectionAsync(
        string host, 
        int port, 
        CancellationToken cancellationToken = default)
    {
        await _connectionSemaphore.WaitAsync(cancellationToken);
        
        if (_availableConnections.TryDequeue(out var connection) && connection.IsConnected)
        {
            return connection;
        }
        
        // Create new connection
        connection = _clientFactory.Create();
        await connection.ConnectAsync(host, port, cancellationToken);
        
        lock (_allConnections)
        {
            _allConnections.Add(connection);
        }
        
        return connection;
    }
    
    public void ReturnConnection(IMesen2StreamingClient connection)
    {
        if (connection.IsConnected)
        {
            _availableConnections.Enqueue(connection);
        }
        
        _connectionSemaphore.Release();
    }
    
    public void Dispose()
    {
        lock (_allConnections)
        {
            foreach (var connection in _allConnections)
            {
                connection.Dispose();
            }
            _allConnections.Clear();
        }
        
        _connectionSemaphore.Dispose();
    }
}
```

---

This developer guide provides comprehensive implementation details, testing strategies, debugging techniques, and performance optimization patterns for the DiztinGUIsh-Mesen2 integration. Use these patterns and examples to build robust, high-performance integrations.
