# DiztinGUIsh Integration API Reference

## Overview

The DiztinGUIsh Integration provides a comprehensive C++ API for real-time communication between Mesen2 and DiztinGUIsh disassembly tool. This document covers the complete API surface including the enhanced Socket layer, configuration system, and Protocol V2.0 implementation.

## Table of Contents

1. [Socket Layer API](#socket-layer-api)
2. [Configuration System](#configuration-system)
3. [Protocol V2.0](#protocol-v2)
4. [Message Management](#message-management)
5. [Compression System](#compression-system)
6. [Error Handling](#error-handling)
7. [Performance Monitoring](#performance-monitoring)
8. [Usage Examples](#usage-examples)

## Socket Layer API

### Enhanced Socket Class

The Socket class provides advanced networking capabilities with comprehensive monitoring and statistics tracking.

```cpp
#include "Shared/Socket.h"

class Socket {
public:
    // Core functionality
    Socket();
    virtual ~Socket();
    
    // Connection management
    bool Connect(string host, uint16_t port);
    bool Listen(uint16_t port, bool ipv6 = false);
    shared_ptr<Socket> Accept();
    void Disconnect();
    
    // Data transmission
    int Send(void* buffer, size_t length);
    int Recv(void* buffer, size_t length);
    
    // Enhanced monitoring features (NEW)
    double GetBandwidthKBps() const;
    uint64_t GetConnectionDurationSeconds() const;
    bool IsHealthy() const;
    void ResetStatistics();
    SocketStatistics GetStatistics() const;
    
    // Configuration (NEW)
    void SetBufferSize(size_t bufferSize);
    void EnableCompression(bool enable);
    void SetCompressionLevel(int level);
    void SetBandwidthLimit(double limitKBps);
    
    // Health monitoring (NEW)
    double GetLatency() const;
    double GetPacketLoss() const;
    ConnectionHealth GetConnectionHealth() const;
    
    // Event callbacks (NEW)
    void SetOnDataReceived(std::function<void(const std::vector<uint8_t>&)> callback);
    void SetOnConnectionLost(std::function<void()> callback);
    void SetOnError(std::function<void(const std::string&)> callback);
};
```

### Socket Statistics

```cpp
struct SocketStatistics {
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<uint64_t> bytesReceived{0};
    std::atomic<uint64_t> messagesSent{0};
    std::atomic<uint64_t> messagesReceived{0};
    std::atomic<uint64_t> reconnections{0};
    std::atomic<uint64_t> errorCount{0};
    
    double averageLatency = 0.0;
    double packetLossRate = 0.0;
    std::chrono::steady_clock::time_point connectionStartTime;
    size_t currentBufferSize = DEFAULT_BUFFER_SIZE;
    bool compressionEnabled = false;
    int compressionLevel = 6;
    double bandwidthLimitKBps = 0.0; // 0 = unlimited
};
```

## Configuration System

### DiztinguishConfig Class

Comprehensive configuration management for the DiztinGUIsh integration.

```cpp
#include "DiztinguishConfig.h"

class DiztinguishConfig {
public:
    DiztinguishConfig();
    
    // Configuration management
    bool LoadFromFile(const std::string& filename);
    bool SaveToFile(const std::string& filename);
    void LoadDefaults();
    bool ValidateConfiguration();
    
    // Runtime configuration updates
    void SetPerformanceProfile(PerformanceProfile profile);
    void EnableDebugMode(bool enable);
    void SetBandwidthLimit(uint32_t limitKBps);
    void UpdateConnectionSettings(const ConnectionConfig& settings);
    
    // Configuration structures
    struct ConnectionConfig {
        std::string host = "localhost";
        uint16_t port = 27015;
        uint32_t timeoutMs = 5000;
        uint32_t retryDelayMs = 1000;
        uint32_t maxRetries = 3;
        bool enableCompression = true;
        bool enableHeartbeat = true;
    };
    
    struct PerformanceConfig {
        uint32_t maxBandwidthKBps = 1024;
        uint32_t bufferSizeKB = 64;
        uint32_t maxBatchSize = 100;
        uint32_t batchTimeoutMs = 50;
        bool adaptiveBandwidth = true;
        bool enableStatistics = true;
    };
    
    struct DebugConfig {
        bool enableLogging = false;
        bool verboseMode = false;
        bool enableProfiling = false;
        std::string logFilePath = "diztinguish_debug.log";
        uint32_t maxLogSizeMB = 10;
    };
    
    struct FilterConfig {
        bool enableROMFiltering = true;
        bool enableRAMFiltering = false;
        bool enableIOFiltering = true;
        std::vector<std::string> excludeAddressRanges;
        std::vector<std::string> includeAddressRanges;
    };
    
    struct UIConfig {
        bool enableRealTimeUpdates = true;
        uint32_t updateIntervalMs = 100;
        bool enableProgressIndicators = true;
        bool enableNotifications = true;
    };
    
    // Public configuration access
    ConnectionConfig connection;
    PerformanceConfig performance;
    DebugConfig debug;
    FilterConfig filter;
    UIConfig ui;
};
```

### Performance Profiles

```cpp
enum class PerformanceProfile {
    FAST,      // Optimized for speed, minimal features
    BALANCED,  // Default configuration
    QUALITY    // Maximum quality, all features enabled
};

// Profile-specific configurations
void DiztinguishConfig::SetPerformanceProfile(PerformanceProfile profile) {
    switch (profile) {
        case PerformanceProfile::FAST:
            performance.maxBandwidthKBps = 2048;
            performance.maxBatchSize = 200;
            performance.batchTimeoutMs = 25;
            connection.enableCompression = false;
            break;
            
        case PerformanceProfile::BALANCED:
            performance.maxBandwidthKBps = 1024;
            performance.maxBatchSize = 100;
            performance.batchTimeoutMs = 50;
            connection.enableCompression = true;
            break;
            
        case PerformanceProfile::QUALITY:
            performance.maxBandwidthKBps = 512;
            performance.maxBatchSize = 50;
            performance.batchTimeoutMs = 100;
            connection.enableCompression = true;
            debug.enableProfiling = true;
            break;
    }
}
```

## Protocol V2.0

### DiztinguishProtocolV2 Namespace

Enhanced protocol implementation with advanced features.

```cpp
#include "DiztinguishProtocolV2.h"

namespace DiztinguishProtocol {
    // Protocol version and magic number
    const uint16_t PROTOCOL_VERSION = 0x0200; // Version 2.0
    const uint32_t MAGIC_NUMBER = 0x44495354; // "DIST"
    
    // Message types
    enum class MessageType : uint16_t {
        HANDSHAKE = 0x0001,
        MEMORY_DATA = 0x0002,
        REGISTER_DATA = 0x0003,
        BREAKPOINT_DATA = 0x0004,
        SYMBOL_DATA = 0x0005,
        STATUS_UPDATE = 0x0006,
        ERROR_REPORT = 0x0007,
        BATCH_DATA = 0x0008,
        PRIORITY_DATA = 0x0009,
        COMPRESSION_TEST = 0x000A
    };
    
    // Protocol capabilities
    enum class ProtocolFeature {
        COMPRESSION = 0,
        MESSAGE_BATCHING = 1,
        PRIORITY_MESSAGES = 2,
        ERROR_RECOVERY = 3,
        BANDWIDTH_CONTROL = 4
    };
    
    // Message header structure
    struct MessageHeader {
        uint32_t magicNumber;      // Magic identifier
        uint16_t version;          // Protocol version
        uint16_t messageType;      // Message type
        uint32_t dataLength;       // Data payload length
        uint32_t messageId;        // Unique message identifier
        uint64_t timestamp;        // Message timestamp
        uint32_t reserved;         // Reserved for future use
        uint32_t checksum;         // Data integrity checksum
    };
}
```

### DiztinguishMessage Class

```cpp
class DiztinguishMessage {
public:
    DiztinguishMessage(MessageType type, const std::vector<uint8_t>& data);
    
    // Message properties
    uint32_t GetId() const { return _id; }
    uint64_t GetTimestamp() const { return _timestamp; }
    MessageType GetType() const { return _type; }
    const std::vector<uint8_t>& GetData() const { return _data; }
    uint32_t GetRetryCount() const { return _retryCount; }
    
    // Message lifecycle
    bool IsExpired(uint64_t timeoutMs) const;
    void IncrementRetryCount() { _retryCount++; }
    
    // Serialization
    std::vector<uint8_t> Serialize() const;
    static std::unique_ptr<DiztinguishMessage> Deserialize(const std::vector<uint8_t>& buffer);
    
    // Utility
    static uint64_t GetCurrentTimestamp();
    
private:
    uint32_t CalculateChecksum() const;
    static uint32_t GenerateId() { return _nextId++; }
    
    static std::atomic<uint32_t> _nextId;
    uint32_t _id;
    uint64_t _timestamp;
    MessageType _type;
    std::vector<uint8_t> _data;
    uint32_t _retryCount;
};
```

## Message Management

### MessageManager Class

Advanced message queuing and batch processing system.

```cpp
class MessageManager {
public:
    explicit MessageManager(const DiztinguishConfig& config);
    ~MessageManager();
    
    // Manager lifecycle
    bool Start();
    void Stop();
    
    // Message operations
    bool QueueMessage(std::unique_ptr<DiztinguishMessage> message);
    std::unique_ptr<DiztinguishMessage> DequeueMessage();
    void ProcessIncomingData(const std::vector<uint8_t>& data);
    
    // Batch processing
    void SetSendCallback(std::function<void(std::unique_ptr<MessageBatch>)> callback);
    
    // Statistics
    MessageStatistics GetStatistics() const;
    
private:
    // Internal processing
    void ProcessBatches();
    std::unique_ptr<MessageBatch> CreateBatch();
    void CompressBatch(MessageBatch& batch);
    void CleanupExpiredMessages();
    
    // Configuration and state
    const DiztinguishConfig& _config;
    std::atomic<bool> _running;
    
    // Threading
    std::thread _batchThread;
    std::thread _cleanupThread;
    std::condition_variable _batchCondition;
    
    // Message queues
    mutable std::mutex _queueMutex;
    std::deque<std::unique_ptr<DiztinguishMessage>> _outgoingQueue;
    std::deque<std::unique_ptr<DiztinguishMessage>> _incomingQueue;
    
    // Receive buffer
    std::vector<uint8_t> _receiveBuffer;
    
    // Batch processing
    std::atomic<uint32_t> _nextBatchId;
    std::function<void(std::unique_ptr<MessageBatch>)> _sendCallback;
    
    // Statistics
    std::atomic<uint64_t> _totalMessagesSent{0};
    std::atomic<uint64_t> _totalMessagesReceived{0};
    std::atomic<uint64_t> _totalBytesCompressed{0};
    std::atomic<double> _totalCompressionRatio{0.0};
};
```

### MessageBatch Structure

```cpp
struct MessageBatch {
    uint32_t batchId;
    uint64_t timestamp;
    std::vector<std::unique_ptr<DiztinguishMessage>> messages;
    
    // Compression data
    bool isCompressed = false;
    std::vector<uint8_t> compressedData;
    size_t originalSize = 0;
    
    // Batch statistics
    size_t GetTotalSize() const;
    double GetCompressionRatio() const;
};
```

## Compression System

### CompressionManager Class

Intelligent compression with algorithm selection and optimization.

```cpp
enum class CompressionType {
    NONE = 0,
    ZLIB = 1,
    LZ4 = 2
};

class CompressionManager {
public:
    explicit CompressionManager(const DiztinguishConfig& config);
    
    // Compression operations
    std::vector<uint8_t> Compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> Decompress(const std::vector<uint8_t>& data, CompressionType type);
    
    // Algorithm selection
    CompressionType SelectOptimalCompression(const std::vector<uint8_t>& data);
    
private:
    // Algorithm implementations
    std::vector<uint8_t> CompressZlib(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecompressZlib(const std::vector<uint8_t>& data);
    std::vector<uint8_t> CompressLZ4(const std::vector<uint8_t>& data);
    std::vector<uint8_t> DecompressLZ4(const std::vector<uint8_t>& data);
    
    // Analysis
    double CalculateEntropy(const std::vector<uint8_t>& data);
    
    const DiztinguishConfig& _config;
    static const size_t MIN_COMPRESSION_SIZE = 64;
};
```

## Error Handling

### ErrorRecoveryManager Class

Comprehensive error handling and recovery system.

```cpp
enum class ErrorType {
    CONNECTION_LOST,
    TIMEOUT,
    PROTOCOL_ERROR,
    COMPRESSION_ERROR,
    MEMORY_ERROR,
    UNKNOWN_ERROR
};

struct ErrorRecord {
    std::chrono::steady_clock::time_point timestamp;
    ErrorType type;
    std::string description;
};

class ErrorRecoveryManager {
public:
    explicit ErrorRecoveryManager(const DiztinguishConfig& config);
    
    // Error management
    bool ShouldRetry(const DiztinguishMessage& message) const;
    std::chrono::milliseconds CalculateRetryDelay(uint32_t retryCount) const;
    void RecordError(ErrorType type, const std::string& description);
    
    // Connection health
    bool IsConnectionHealthy() const;
    std::vector<ErrorRecord> GetRecentErrors(std::chrono::minutes timespan) const;
    
private:
    const DiztinguishConfig& _config;
    std::vector<ErrorRecord> _errorHistory;
    std::unordered_map<ErrorType, uint32_t> _errorCounts;
};
```

## Performance Monitoring

### BandwidthManager Class

Real-time bandwidth monitoring and adaptive control.

```cpp
class BandwidthManager {
public:
    explicit BandwidthManager(const DiztinguishConfig& config);
    
    // Bandwidth control
    bool CanSendMessage(size_t messageSize);
    void RecordSentMessage(size_t messageSize);
    
    // Monitoring
    double GetCurrentBandwidth() const;
    double GetTargetBandwidth() const;
    
private:
    void UpdateBandwidthUsage();
    void AdaptBandwidth();
    
    const DiztinguishConfig& _config;
    std::vector<std::pair<std::chrono::steady_clock::time_point, size_t>> _sentMessages;
    double _currentBandwidth;
    double _targetBandwidth;
    std::chrono::steady_clock::time_point _lastUpdateTime;
};
```

### Performance Statistics

```cpp
struct MessageStatistics {
    size_t outgoingQueueSize = 0;
    size_t incomingQueueSize = 0;
    uint64_t totalMessagesSent = 0;
    uint64_t totalMessagesReceived = 0;
    uint64_t totalBytesCompressed = 0;
    double totalCompressionRatio = 0.0;
};

struct PerformanceMetrics {
    double averageLatency = 0.0;
    double throughputKBps = 0.0;
    double cpuUsagePercent = 0.0;
    double memoryUsageMB = 0.0;
    size_t peakQueueSize = 0;
};
```

## Usage Examples

### Basic Socket Usage

```cpp
#include "Shared/Socket.h"
#include "DiztinguishConfig.h"

// Initialize configuration
DiztinguishConfig config;
config.LoadDefaults();
config.connection.host = "127.0.0.1";
config.connection.port = 27015;

// Create and configure socket
auto socket = std::make_shared<Socket>();
socket->SetBufferSize(config.performance.bufferSizeKB * 1024);
socket->EnableCompression(config.connection.enableCompression);

// Set up monitoring callbacks
socket->SetOnDataReceived([](const std::vector<uint8_t>& data) {
    printf("Received %zu bytes\n", data.size());
});

socket->SetOnConnectionLost([]() {
    printf("Connection lost - attempting reconnection\n");
});

// Connect with error handling
if (socket->Connect(config.connection.host, config.connection.port)) {
    printf("Connected successfully\n");
    
    // Monitor performance
    auto stats = socket->GetStatistics();
    printf("Bandwidth: %.2f KB/s\n", socket->GetBandwidthKBps());
    printf("Latency: %.2f ms\n", socket->GetLatency());
} else {
    printf("Connection failed\n");
}
```

### Advanced Message Management

```cpp
#include "DiztinguishProtocolV2.h"

using namespace DiztinguishProtocol;

// Initialize message manager
DiztinguishConfig config;
config.LoadFromFile("diztinguish_config.json");

MessageManager manager(config);
manager.Start();

// Set up message sending
manager.SetSendCallback([&socket](std::unique_ptr<MessageBatch> batch) {
    if (batch->isCompressed) {
        socket->Send(batch->compressedData.data(), batch->compressedData.size());
    } else {
        // Send individual messages
        for (const auto& message : batch->messages) {
            auto serialized = message->Serialize();
            socket->Send(serialized.data(), serialized.size());
        }
    }
});

// Create and queue messages
auto memoryData = std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04};
auto message = std::make_unique<DiztinguishMessage>(
    MessageType::MEMORY_DATA, memoryData);

if (manager.QueueMessage(std::move(message))) {
    printf("Message queued successfully\n");
} else {
    printf("Failed to queue message - queue may be full\n");
}

// Monitor statistics
auto stats = manager.GetStatistics();
printf("Outgoing queue: %zu messages\n", stats.outgoingQueueSize);
printf("Total sent: %llu messages\n", stats.totalMessagesSent);
printf("Compression ratio: %.2f%%\n", stats.totalCompressionRatio * 100);
```

### Configuration Management

```cpp
#include "DiztinguishConfig.h"

// Load configuration with validation
DiztinguishConfig config;
if (!config.LoadFromFile("custom_config.json")) {
    printf("Failed to load config, using defaults\n");
    config.LoadDefaults();
}

// Validate and adjust configuration
if (!config.ValidateConfiguration()) {
    printf("Invalid configuration detected, applying corrections\n");
    config.LoadDefaults(); // Reset to safe defaults
}

// Runtime configuration updates
config.SetPerformanceProfile(PerformanceProfile::FAST);
config.EnableDebugMode(true);
config.SetBandwidthLimit(2048); // 2 MB/s

// Save updated configuration
if (config.SaveToFile("updated_config.json")) {
    printf("Configuration saved successfully\n");
}

// Access specific configuration sections
printf("Max bandwidth: %u KB/s\n", config.performance.maxBandwidthKBps);
printf("Buffer size: %u KB\n", config.performance.bufferSizeKB);
printf("Compression enabled: %s\n", config.connection.enableCompression ? "Yes" : "No");
```

## Build Integration

### CMake Integration

```cmake
# Find required dependencies
find_package(ZLIB REQUIRED)

# Add DiztinGUIsh integration
add_library(DiztinguishIntegration STATIC
    Shared/Socket.cpp
    Debugger/DiztinguishConfig.cpp
    Debugger/DiztinguishProtocolV2.cpp
)

target_link_libraries(DiztinguishIntegration
    PRIVATE ZLIB::ZLIB
)

target_include_directories(DiztinguishIntegration
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with main project
target_link_libraries(Mesen2Core
    PRIVATE DiztinguishIntegration
)
```

### Visual Studio Integration

```xml
<!-- Add to Core.vcxproj -->
<ClInclude Include="Shared\Socket.h" />
<ClInclude Include="Debugger\DiztinguishConfig.h" />
<ClInclude Include="Debugger\DiztinguishProtocolV2.h" />

<ClCompile Include="Shared\Socket.cpp" />
<ClCompile Include="Debugger\DiztinguishConfig.cpp" />
<ClCompile Include="Debugger\DiztinguishProtocolV2.cpp" />

<!-- Link with zlib -->
<AdditionalDependencies>zlib.lib;%(AdditionalDependencies)</AdditionalDependencies>
```

---

## Support and Troubleshooting

### Common Issues

1. **Connection Timeouts**: Check firewall settings and increase timeout values in configuration
2. **High Memory Usage**: Reduce batch sizes and enable compression
3. **Performance Issues**: Use FAST performance profile for real-time applications
4. **Protocol Errors**: Ensure both sides use compatible protocol versions

### Debug Configuration

```cpp
// Enable comprehensive debugging
config.debug.enableLogging = true;
config.debug.verboseMode = true;
config.debug.enableProfiling = true;
config.debug.logFilePath = "diztinguish_debug.log";

// Performance monitoring
config.performance.enableStatistics = true;
socket->SetOnError([](const std::string& error) {
    printf("Socket error: %s\n", error.c_str());
});
```

### Performance Optimization

For optimal performance, consider these configuration settings:

- **Real-time applications**: Use `PerformanceProfile::FAST`
- **Network bandwidth limited**: Enable compression and reduce batch sizes
- **High throughput**: Increase buffer sizes and batch timeouts
- **Debug scenarios**: Enable comprehensive logging and profiling

This API provides a complete foundation for integrating DiztinGUIsh with Mesen2, offering advanced networking, compression, error handling, and performance monitoring capabilities.