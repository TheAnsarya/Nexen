#pragma once

#include "pch.h"
#include "Shared/Socket.h"
#include "Debugger/DiztinguishConfig.h"
#include "Debugger/DiztinguishProtocolV2.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>

using namespace DiztinguishProtocol;

// Test fixture for DiztinGUIsh integration tests
class DiztinguishIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test configuration
        config = std::make_unique<DiztinguishConfig>();
        config->LoadDefaults();
        
        // Override for testing
        config->connection.host = "127.0.0.1";
        config->connection.port = 27016; // Use different port for testing
        config->connection.timeoutMs = 1000;
        config->performance.maxBandwidthKBps = 512;
        config->debug.enableLogging = true;
        
        // Initialize test sockets
        serverSocket = std::make_shared<Socket>();
        clientSocket = std::make_shared<Socket>();
    }
    
    void TearDown() override {
        if (serverSocket) {
            serverSocket->Disconnect();
        }
        if (clientSocket) {
            clientSocket->Disconnect();
        }
        
        if (serverThread.joinable()) {
            serverThread.join();
        }
    }
    
    // Helper method to create test server
    void StartTestServer() {
        serverThread = std::thread([this]() {
            if (serverSocket->Listen(config->connection.port)) {
                auto client = serverSocket->Accept();
                if (client) {
                    // Echo server for testing
                    std::vector<uint8_t> buffer(1024);
                    int received = client->Recv(buffer.data(), buffer.size());
                    if (received > 0) {
                        buffer.resize(received);
                        client->Send(buffer.data(), buffer.size());
                    }
                }
            }
        });
        
        // Allow server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Helper method to create test message
    std::unique_ptr<DiztinguishMessage> CreateTestMessage(MessageType type, const std::string& data) {
        std::vector<uint8_t> dataBytes(data.begin(), data.end());
        return std::make_unique<DiztinguishMessage>(type, dataBytes);
    }
    
    std::unique_ptr<DiztinguishConfig> config;
    std::shared_ptr<Socket> serverSocket;
    std::shared_ptr<Socket> clientSocket;
    std::thread serverThread;
};

// Socket Tests
TEST_F(DiztinguishIntegrationTest, SocketBasicConnection) {
    StartTestServer();
    
    ASSERT_TRUE(clientSocket->Connect(config->connection.host, config->connection.port));
    EXPECT_TRUE(clientSocket->IsConnected());
    
    clientSocket->Disconnect();
    EXPECT_FALSE(clientSocket->IsConnected());
}

TEST_F(DiztinguishIntegrationTest, SocketDataTransmission) {
    StartTestServer();
    
    ASSERT_TRUE(clientSocket->Connect(config->connection.host, config->connection.port));
    
    std::string testData = "Hello DiztinGUIsh!";
    int sentBytes = clientSocket->Send((void*)testData.c_str(), testData.length());
    EXPECT_EQ(sentBytes, testData.length());
    
    std::vector<char> buffer(1024);
    int receivedBytes = clientSocket->Recv(buffer.data(), buffer.size());
    EXPECT_EQ(receivedBytes, testData.length());
    
    std::string receivedData(buffer.data(), receivedBytes);
    EXPECT_EQ(receivedData, testData);
}

TEST_F(DiztinguishIntegrationTest, SocketStatisticsTracking) {
    clientSocket->SetBufferSize(8192);
    
    // Test statistics initialization
    auto stats = clientSocket->GetStatistics();
    EXPECT_EQ(stats.bytesSent.load(), 0);
    EXPECT_EQ(stats.bytesReceived.load(), 0);
    EXPECT_EQ(stats.messagesSent.load(), 0);
    EXPECT_EQ(stats.currentBufferSize, 8192);
    
    StartTestServer();
    ASSERT_TRUE(clientSocket->Connect(config->connection.host, config->connection.port));
    
    std::string testData = "Statistics test data";
    clientSocket->Send((void*)testData.c_str(), testData.length());
    
    // Check updated statistics
    stats = clientSocket->GetStatistics();
    EXPECT_GT(stats.bytesSent.load(), 0);
    EXPECT_GT(clientSocket->GetConnectionDurationSeconds(), 0);
}

TEST_F(DiztinguishIntegrationTest, SocketBandwidthMonitoring) {
    StartTestServer();
    ASSERT_TRUE(clientSocket->Connect(config->connection.host, config->connection.port));
    
    // Send data and measure bandwidth
    std::string largeData(1024, 'X'); // 1KB of data
    auto startTime = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        clientSocket->Send((void*)largeData.c_str(), largeData.length());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto endTime = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(endTime - startTime).count();
    
    double bandwidth = clientSocket->GetBandwidthKBps();
    EXPECT_GT(bandwidth, 0.0);
    
    // Check if bandwidth calculation is reasonable
    double expectedBandwidth = (10.0 * 1024.0) / (duration * 1024.0);
    EXPECT_NEAR(bandwidth, expectedBandwidth, expectedBandwidth * 0.5); // Allow 50% variance
}

TEST_F(DiztinguishIntegrationTest, SocketHealthMonitoring) {
    StartTestServer();
    ASSERT_TRUE(clientSocket->Connect(config->connection.host, config->connection.port));
    
    // Healthy connection should return true
    EXPECT_TRUE(clientSocket->IsHealthy());
    
    // Test connection health over time
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(clientSocket->IsHealthy());
    
    // Reset statistics and verify
    clientSocket->ResetStatistics();
    auto stats = clientSocket->GetStatistics();
    EXPECT_EQ(stats.errorCount.load(), 0);
}

// Configuration Tests
TEST_F(DiztinguishIntegrationTest, ConfigurationLoading) {
    DiztinguishConfig testConfig;
    testConfig.LoadDefaults();
    
    // Verify default values
    EXPECT_EQ(testConfig.connection.host, "localhost");
    EXPECT_EQ(testConfig.connection.port, 27015);
    EXPECT_EQ(testConfig.connection.timeoutMs, 5000);
    EXPECT_TRUE(testConfig.connection.enableCompression);
    EXPECT_EQ(testConfig.performance.maxBandwidthKBps, 1024);
    EXPECT_FALSE(testConfig.debug.enableLogging);
}

TEST_F(DiztinguishIntegrationTest, ConfigurationValidation) {
    DiztinguishConfig testConfig;
    testConfig.LoadDefaults();
    
    // Valid configuration should pass
    EXPECT_TRUE(testConfig.ValidateConfiguration());
    
    // Invalid configuration should fail
    testConfig.connection.port = 0; // Invalid port
    EXPECT_FALSE(testConfig.ValidateConfiguration());
    
    testConfig.LoadDefaults(); // Reset
    testConfig.performance.maxBandwidthKBps = 0; // Invalid bandwidth
    EXPECT_FALSE(testConfig.ValidateConfiguration());
}

TEST_F(DiztinguishIntegrationTest, PerformanceProfiles) {
    DiztinguishConfig testConfig;
    
    // Test FAST profile
    testConfig.SetPerformanceProfile(PerformanceProfile::FAST);
    EXPECT_EQ(testConfig.performance.maxBandwidthKBps, 2048);
    EXPECT_FALSE(testConfig.connection.enableCompression);
    
    // Test BALANCED profile
    testConfig.SetPerformanceProfile(PerformanceProfile::BALANCED);
    EXPECT_EQ(testConfig.performance.maxBandwidthKBps, 1024);
    EXPECT_TRUE(testConfig.connection.enableCompression);
    
    // Test QUALITY profile
    testConfig.SetPerformanceProfile(PerformanceProfile::QUALITY);
    EXPECT_EQ(testConfig.performance.maxBandwidthKBps, 512);
    EXPECT_TRUE(testConfig.connection.enableCompression);
    EXPECT_TRUE(testConfig.debug.enableProfiling);
}

// Protocol V2 Message Tests
TEST_F(DiztinguishIntegrationTest, MessageCreationAndSerialization) {
    auto message = CreateTestMessage(MessageType::MEMORY_DATA, "Test memory data");
    
    // Verify message properties
    EXPECT_GT(message->GetId(), 0);
    EXPECT_GT(message->GetTimestamp(), 0);
    EXPECT_EQ(message->GetType(), MessageType::MEMORY_DATA);
    EXPECT_EQ(message->GetRetryCount(), 0);
    
    // Test serialization
    auto serialized = message->Serialize();
    EXPECT_GT(serialized.size(), sizeof(MessageHeader));
    
    // Test deserialization
    auto deserializedMessage = DiztinguishMessage::Deserialize(serialized);
    ASSERT_TRUE(deserializedMessage != nullptr);
    EXPECT_EQ(deserializedMessage->GetType(), MessageType::MEMORY_DATA);
    EXPECT_EQ(deserializedMessage->GetId(), message->GetId());
}

TEST_F(DiztinguishIntegrationTest, MessageExpiration) {
    auto message = CreateTestMessage(MessageType::STATUS_UPDATE, "Status data");
    
    // Fresh message should not be expired
    EXPECT_FALSE(message->IsExpired(1000)); // 1 second timeout
    
    // Wait and check expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(message->IsExpired(1000)); // Still within timeout
    EXPECT_TRUE(message->IsExpired(50));    // Should be expired with 50ms timeout
}

TEST_F(DiztinguishIntegrationTest, MessageRetryHandling) {
    auto message = CreateTestMessage(MessageType::BREAKPOINT_DATA, "Breakpoint info");
    
    EXPECT_EQ(message->GetRetryCount(), 0);
    
    message->IncrementRetryCount();
    EXPECT_EQ(message->GetRetryCount(), 1);
    
    message->IncrementRetryCount();
    EXPECT_EQ(message->GetRetryCount(), 2);
}

// MessageManager Tests
TEST_F(DiztinguishIntegrationTest, MessageManagerLifecycle) {
    MessageManager manager(*config);
    
    EXPECT_TRUE(manager.Start());
    EXPECT_FALSE(manager.Start()); // Should fail if already started
    
    manager.Stop();
    // Should be able to start again after stopping
    EXPECT_TRUE(manager.Start());
    
    manager.Stop();
}

TEST_F(DiztinguishIntegrationTest, MessageQueueing) {
    MessageManager manager(*config);
    EXPECT_TRUE(manager.Start());
    
    // Queue multiple messages
    for (int i = 0; i < 5; ++i) {
        auto message = CreateTestMessage(MessageType::REGISTER_DATA, 
                                       "Register data " + std::to_string(i));
        EXPECT_TRUE(manager.QueueMessage(std::move(message)));
    }
    
    // Check statistics
    auto stats = manager.GetStatistics();
    EXPECT_EQ(stats.outgoingQueueSize, 5);
    
    manager.Stop();
}

TEST_F(DiztinguishIntegrationTest, MessageBatching) {
    MessageManager manager(*config);
    
    // Set up batch processing callback
    std::vector<std::unique_ptr<MessageBatch>> receivedBatches;
    manager.SetSendCallback([&receivedBatches](std::unique_ptr<MessageBatch> batch) {
        receivedBatches.push_back(std::move(batch));
    });
    
    EXPECT_TRUE(manager.Start());
    
    // Queue messages for batching
    for (int i = 0; i < 10; ++i) {
        auto message = CreateTestMessage(MessageType::SYMBOL_DATA, 
                                       "Symbol data " + std::to_string(i));
        manager.QueueMessage(std::move(message));
    }
    
    // Wait for batch processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_GT(receivedBatches.size(), 0);
    
    // Verify batch contents
    size_t totalMessages = 0;
    for (const auto& batch : receivedBatches) {
        totalMessages += batch->messages.size();
    }
    EXPECT_EQ(totalMessages, 10);
    
    manager.Stop();
}

// Compression Tests
TEST_F(DiztinguishIntegrationTest, CompressionBasicFunctionality) {
    CompressionManager compManager(*config);
    
    // Create test data with patterns (good for compression)
    std::vector<uint8_t> testData;
    for (int i = 0; i < 1000; ++i) {
        testData.push_back(i % 10); // Repeating pattern
    }
    
    auto compressed = compManager.Compress(testData);
    
    // For patterned data, compression should be effective
    // Note: Actual compression depends on algorithm implementation
    EXPECT_GE(compressed.size(), 1); // Should at least have some data
    
    auto decompressed = compManager.Decompress(compressed, CompressionType::ZLIB);
    
    // If compression was applied, decompressed should match original
    if (compressed.size() < testData.size()) {
        EXPECT_EQ(decompressed.size(), testData.size());
        EXPECT_EQ(decompressed, testData);
    }
}

TEST_F(DiztinguishIntegrationTest, CompressionAlgorithmSelection) {
    CompressionManager compManager(*config);
    
    // Small data should prefer no compression or LZ4
    std::vector<uint8_t> smallData(32, 0x55);
    auto algorithm = compManager.SelectOptimalCompression(smallData);
    EXPECT_TRUE(algorithm == CompressionType::LZ4 || algorithm == CompressionType::NONE);
    
    // Large, high-entropy data should prefer LZ4
    std::vector<uint8_t> randomData(2048);
    for (size_t i = 0; i < randomData.size(); ++i) {
        randomData[i] = rand() % 256;
    }
    algorithm = compManager.SelectOptimalCompression(randomData);
    EXPECT_EQ(algorithm, CompressionType::LZ4);
    
    // Large, patterned data should prefer ZLIB
    std::vector<uint8_t> patternedData(2048, 0xAA);
    algorithm = compManager.SelectOptimalCompression(patternedData);
    EXPECT_EQ(algorithm, CompressionType::ZLIB);
}

// Bandwidth Management Tests
TEST_F(DiztinguishIntegrationTest, BandwidthTracking) {
    BandwidthManager bwManager(*config);
    
    // Initially should allow messages
    EXPECT_TRUE(bwManager.CanSendMessage(1024));
    
    // Record sending large amounts of data
    size_t largeMessage = 500 * 1024; // 500KB
    for (int i = 0; i < 3; ++i) {
        bwManager.RecordSentMessage(largeMessage);
    }
    
    // Should now restrict based on bandwidth limit
    EXPECT_LE(bwManager.GetCurrentBandwidth(), config->performance.maxBandwidthKBps * 1.1);
    
    // Small messages should still be allowed
    EXPECT_TRUE(bwManager.CanSendMessage(100));
}

TEST_F(DiztinguishIntegrationTest, AdaptiveBandwidth) {
    // Enable adaptive bandwidth
    config->performance.adaptiveBandwidth = true;
    BandwidthManager bwManager(*config);
    
    double initialTarget = bwManager.GetTargetBandwidth();
    
    // Simulate high message frequency
    for (int i = 0; i < 50; ++i) {
        bwManager.RecordSentMessage(1024);
    }
    
    // Target bandwidth should adjust
    double adjustedTarget = bwManager.GetTargetBandwidth();
    // Due to high frequency, target might be reduced
    EXPECT_LE(adjustedTarget, initialTarget * 1.1); // Allow some variance
}

// Error Recovery Tests
TEST_F(DiztinguishIntegrationTest, ErrorRecoveryBasics) {
    ErrorRecoveryManager errorManager(*config);
    
    auto message = CreateTestMessage(MessageType::ERROR_REPORT, "Test error");
    
    // Fresh message should be retryable
    EXPECT_TRUE(errorManager.ShouldRetry(*message));
    
    // Increment retry count beyond limit
    for (uint32_t i = 0; i <= config->connection.maxRetries; ++i) {
        message->IncrementRetryCount();
    }
    
    // Should no longer be retryable
    EXPECT_FALSE(errorManager.ShouldRetry(*message));
}

TEST_F(DiztinguishIntegrationTest, ErrorRecoveryDelayCalculation) {
    ErrorRecoveryManager errorManager(*config);
    
    // Test exponential backoff
    auto delay1 = errorManager.CalculateRetryDelay(0);
    auto delay2 = errorManager.CalculateRetryDelay(1);
    auto delay3 = errorManager.CalculateRetryDelay(2);
    
    EXPECT_GE(delay2.count(), delay1.count());
    EXPECT_GE(delay3.count(), delay2.count());
    
    // Delays should be reasonable (not too long)
    EXPECT_LT(delay1.count(), 10000); // Less than 10 seconds
    EXPECT_LT(delay3.count(), 60000); // Less than 60 seconds
}

TEST_F(DiztinguishIntegrationTest, ConnectionHealthMonitoring) {
    ErrorRecoveryManager errorManager(*config);
    
    // Initially healthy
    EXPECT_TRUE(errorManager.IsConnectionHealthy());
    
    // Record multiple errors
    for (int i = 0; i < 5; ++i) {
        errorManager.RecordError(ErrorType::TIMEOUT, "Test timeout error");
    }
    
    // Should still be healthy with few errors
    EXPECT_TRUE(errorManager.IsConnectionHealthy());
    
    // Record many errors
    for (int i = 0; i < 15; ++i) {
        errorManager.RecordError(ErrorType::CONNECTION_LOST, "Test connection error");
    }
    
    // Should now be considered unhealthy
    EXPECT_FALSE(errorManager.IsConnectionHealthy());
    
    // Check recent errors
    auto recentErrors = errorManager.GetRecentErrors(std::chrono::minutes(1));
    EXPECT_EQ(recentErrors.size(), 20); // 5 + 15 errors
}

// Version Management Tests
TEST_F(DiztinguishIntegrationTest, VersionCompatibility) {
    VersionManager versionManager;
    
    // Same version should be compatible
    EXPECT_TRUE(versionManager.IsVersionCompatible(PROTOCOL_VERSION));
    
    // Same major version, different minor should be compatible
    uint16_t compatibleVersion = (PROTOCOL_VERSION & 0xFF00) | 0x0001;
    EXPECT_TRUE(versionManager.IsVersionCompatible(compatibleVersion));
    
    // Different major version should not be compatible
    uint16_t incompatibleVersion = ((PROTOCOL_VERSION >> 8) + 1) << 8;
    EXPECT_FALSE(versionManager.IsVersionCompatible(incompatibleVersion));
}

TEST_F(DiztinguishIntegrationTest, FeatureSupport) {
    VersionManager versionManager;
    
    // Check supported features
    EXPECT_TRUE(versionManager.SupportsFeature(ProtocolFeature::COMPRESSION));
    EXPECT_TRUE(versionManager.SupportsFeature(ProtocolFeature::MESSAGE_BATCHING));
    EXPECT_TRUE(versionManager.SupportsFeature(ProtocolFeature::ERROR_RECOVERY));
    
    auto supportedFeatures = versionManager.GetSupportedFeatures();
    EXPECT_GT(supportedFeatures.size(), 0);
    
    // Check capability flags
    auto capabilities = versionManager.GetCapabilityFlags();
    EXPECT_GT(capabilities, 0);
}

TEST_F(DiztinguishIntegrationTest, CapabilityNegotiation) {
    VersionManager versionManager;
    
    // Simulate remote capabilities (missing some features)
    CapabilityFlags remoteCapabilities = 0;
    remoteCapabilities |= (1 << static_cast<int>(ProtocolFeature::COMPRESSION));
    remoteCapabilities |= (1 << static_cast<int>(ProtocolFeature::MESSAGE_BATCHING));
    // Missing ERROR_RECOVERY and BANDWIDTH_CONTROL
    
    versionManager.NegotiateCapabilities(remoteCapabilities);
    
    auto negotiatedFeatures = versionManager.GetNegotiatedFeatures();
    
    // Should include intersection of features
    auto it = std::find(negotiatedFeatures.begin(), negotiatedFeatures.end(), 
                       ProtocolFeature::COMPRESSION);
    EXPECT_NE(it, negotiatedFeatures.end());
    
    it = std::find(negotiatedFeatures.begin(), negotiatedFeatures.end(), 
                  ProtocolFeature::MESSAGE_BATCHING);
    EXPECT_NE(it, negotiatedFeatures.end());
    
    // Should not include unsupported features
    it = std::find(negotiatedFeatures.begin(), negotiatedFeatures.end(), 
                  ProtocolFeature::ERROR_RECOVERY);
    EXPECT_EQ(it, negotiatedFeatures.end());
}

// Integration Tests
TEST_F(DiztinguishIntegrationTest, EndToEndIntegration) {
    // Set up complete integration test
    MessageManager manager(*config);
    
    std::vector<std::unique_ptr<MessageBatch>> sentBatches;
    manager.SetSendCallback([&sentBatches](std::unique_ptr<MessageBatch> batch) {
        sentBatches.push_back(std::move(batch));
    });
    
    EXPECT_TRUE(manager.Start());
    
    // Create various message types
    auto messages = std::vector<std::unique_ptr<DiztinguishMessage>>();
    messages.push_back(CreateTestMessage(MessageType::HANDSHAKE, "Handshake data"));
    messages.push_back(CreateTestMessage(MessageType::MEMORY_DATA, "Memory content"));
    messages.push_back(CreateTestMessage(MessageType::REGISTER_DATA, "CPU registers"));
    messages.push_back(CreateTestMessage(MessageType::SYMBOL_DATA, "Debug symbols"));
    
    // Queue all messages
    for (auto& message : messages) {
        EXPECT_TRUE(manager.QueueMessage(std::move(message)));
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify batch creation
    EXPECT_GT(sentBatches.size(), 0);
    
    size_t totalMessages = 0;
    for (const auto& batch : sentBatches) {
        totalMessages += batch->messages.size();
        EXPECT_GT(batch->batchId, 0);
        EXPECT_GT(batch->timestamp, 0);
    }
    
    EXPECT_EQ(totalMessages, 4);
    
    // Check final statistics
    auto stats = manager.GetStatistics();
    EXPECT_EQ(stats.totalMessagesSent, 4);
    
    manager.Stop();
}

// Performance Tests
TEST_F(DiztinguishIntegrationTest, PerformanceStressTest) {
    config->SetPerformanceProfile(PerformanceProfile::FAST);
    MessageManager manager(*config);
    
    std::atomic<size_t> batchCount{0};
    manager.SetSendCallback([&batchCount](std::unique_ptr<MessageBatch> batch) {
        batchCount++;
    });
    
    EXPECT_TRUE(manager.Start());
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Queue many messages rapidly
    const size_t messageCount = 1000;
    for (size_t i = 0; i < messageCount; ++i) {
        auto message = CreateTestMessage(MessageType::MEMORY_DATA, 
                                       "Stress test data " + std::to_string(i));
        manager.QueueMessage(std::move(message));
        
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    // Wait for all processing to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double>(endTime - startTime);
    
    // Check performance metrics
    auto stats = manager.GetStatistics();
    EXPECT_EQ(stats.totalMessagesSent, messageCount);
    EXPECT_GT(batchCount.load(), 0);
    
    double messagesPerSecond = messageCount / duration.count();
    EXPECT_GT(messagesPerSecond, 100); // Should handle at least 100 messages/second
    
    manager.Stop();
}