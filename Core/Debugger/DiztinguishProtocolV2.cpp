#include "pch.h"
#include "DiztinguishProtocolV2.h"
#include "DiztinguishConfig.h"
#include <chrono>
#include <random>
#include <zlib.h>
#include <algorithm>

namespace DiztinguishProtocol {

// Static member initialization
std::atomic<uint32_t> DiztinguishMessage::_nextId(1);

// Message Implementation
DiztinguishMessage::DiztinguishMessage(MessageType type, const std::vector<uint8_t>& data)
    : _id(GenerateId()), _timestamp(GetCurrentTimestamp()), 
      _type(type), _data(data), _retryCount(0) {}

uint64_t DiztinguishMessage::GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool DiztinguishMessage::IsExpired(uint64_t timeoutMs) const {
    return (GetCurrentTimestamp() - _timestamp) > timeoutMs;
}

std::vector<uint8_t> DiztinguishMessage::Serialize() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(MessageHeader) + _data.size());
    
    MessageHeader header = {
        MAGIC_NUMBER,
        PROTOCOL_VERSION,
        static_cast<uint16_t>(_type),
        static_cast<uint32_t>(_data.size()),
        _id,
        _timestamp,
        0, // Reserved
        CalculateChecksum()
    };
    
    // Append header
    const uint8_t* headerBytes = reinterpret_cast<const uint8_t*>(&header);
    buffer.insert(buffer.end(), headerBytes, headerBytes + sizeof(MessageHeader));
    
    // Append data
    buffer.insert(buffer.end(), _data.begin(), _data.end());
    
    return buffer;
}

std::unique_ptr<DiztinguishMessage> DiztinguishMessage::Deserialize(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(MessageHeader)) {
        return nullptr;
    }
    
    const MessageHeader* header = reinterpret_cast<const MessageHeader*>(buffer.data());
    
    // Validate header
    if (header->magicNumber != MAGIC_NUMBER || 
        header->version != PROTOCOL_VERSION ||
        buffer.size() < sizeof(MessageHeader) + header->dataLength) {
        return nullptr;
    }
    
    // Extract data
    std::vector<uint8_t> data(buffer.begin() + sizeof(MessageHeader),
                             buffer.begin() + sizeof(MessageHeader) + header->dataLength);
    
    auto message = std::make_unique<DiztinguishMessage>(
        static_cast<MessageType>(header->messageType), data);
    message->_id = header->messageId;
    message->_timestamp = header->timestamp;
    
    // Validate checksum
    if (message->CalculateChecksum() != header->checksum) {
        return nullptr;
    }
    
    return message;
}

uint32_t DiztinguishMessage::CalculateChecksum() const {
    // Simple CRC32-style checksum for data integrity
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : _data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

// MessageManager Implementation
MessageManager::MessageManager(const DiztinguishConfig& config)
    : _config(config), _running(false), _nextBatchId(1) {}

MessageManager::~MessageManager() {
    Stop();
}

bool MessageManager::Start() {
    if (_running.load()) {
        return false;
    }
    
    _running = true;
    _batchThread = std::thread(&MessageManager::ProcessBatches, this);
    _cleanupThread = std::thread(&MessageManager::CleanupExpiredMessages, this);
    
    return true;
}

void MessageManager::Stop() {
    if (!_running.load()) {
        return;
    }
    
    _running = false;
    _batchCondition.notify_all();
    
    if (_batchThread.joinable()) {
        _batchThread.join();
    }
    
    if (_cleanupThread.joinable()) {
        _cleanupThread.join();
    }
}

bool MessageManager::QueueMessage(std::unique_ptr<DiztinguishMessage> message) {
    if (!message || !_running.load()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(_queueMutex);
    
    if (_outgoingQueue.size() >= _config.performance.maxQueueSize) {
        // Queue full, drop oldest non-priority messages
        auto it = std::find_if(_outgoingQueue.begin(), _outgoingQueue.end(),
            [](const auto& msg) { return msg->GetType() != MessageType::PRIORITY_DATA; });
        
        if (it != _outgoingQueue.end()) {
            _outgoingQueue.erase(it);
        } else {
            return false; // Queue full with priority messages
        }
    }
    
    _outgoingQueue.push_back(std::move(message));
    _batchCondition.notify_one();
    
    return true;
}

std::unique_ptr<DiztinguishMessage> MessageManager::DequeueMessage() {
    std::lock_guard<std::mutex> lock(_queueMutex);
    
    if (_incomingQueue.empty()) {
        return nullptr;
    }
    
    auto message = std::move(_incomingQueue.front());
    _incomingQueue.pop_front();
    
    return message;
}

void MessageManager::ProcessIncomingData(const std::vector<uint8_t>& data) {
    _receiveBuffer.insert(_receiveBuffer.end(), data.begin(), data.end());
    
    // Process complete messages
    while (_receiveBuffer.size() >= sizeof(MessageHeader)) {
        const MessageHeader* header = reinterpret_cast<const MessageHeader*>(_receiveBuffer.data());
        size_t totalSize = sizeof(MessageHeader) + header->dataLength;
        
        if (_receiveBuffer.size() < totalSize) {
            break; // Incomplete message
        }
        
        std::vector<uint8_t> messageBuffer(_receiveBuffer.begin(), 
                                         _receiveBuffer.begin() + totalSize);
        _receiveBuffer.erase(_receiveBuffer.begin(), _receiveBuffer.begin() + totalSize);
        
        auto message = DiztinguishMessage::Deserialize(messageBuffer);
        if (message) {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _incomingQueue.push_back(std::move(message));
        }
    }
}

void MessageManager::ProcessBatches() {
    while (_running.load()) {
        std::unique_lock<std::mutex> lock(_queueMutex);
        
        _batchCondition.wait_for(lock, std::chrono::milliseconds(_config.performance.batchTimeoutMs),
            [this] { return !_outgoingQueue.empty() || !_running.load(); });
        
        if (!_running.load()) {
            break;
        }
        
        if (_outgoingQueue.empty()) {
            continue;
        }
        
        // Create batch from available messages
        auto batch = CreateBatch();
        lock.unlock();
        
        if (batch && _sendCallback) {
            _sendCallback(std::move(batch));
        }
    }
}

std::unique_ptr<MessageBatch> MessageManager::CreateBatch() {
    auto batch = std::make_unique<MessageBatch>();
    batch->batchId = _nextBatchId++;
    batch->timestamp = DiztinguishMessage::GetCurrentTimestamp();
    
    size_t maxMessages = std::min(_config.performance.maxBatchSize, _outgoingQueue.size());
    
    for (size_t i = 0; i < maxMessages; ++i) {
        if (_outgoingQueue.empty()) {
            break;
        }
        
        batch->messages.push_back(std::move(_outgoingQueue.front()));
        _outgoingQueue.pop_front();
    }
    
    // Compress batch if beneficial
    if (_config.connection.enableCompression && batch->messages.size() > 1) {
        CompressBatch(*batch);
    }
    
    return batch;
}

void MessageManager::CompressBatch(MessageBatch& batch) {
    if (batch.messages.empty()) {
        return;
    }
    
    // Serialize all messages
    std::vector<uint8_t> uncompressedData;
    for (const auto& message : batch.messages) {
        auto serialized = message->Serialize();
        uncompressedData.insert(uncompressedData.end(), serialized.begin(), serialized.end());
    }
    
    // Compress using zlib
    uLongf compressedSize = compressBound(uncompressedData.size());
    batch.compressedData.resize(compressedSize);
    
    int result = compress(batch.compressedData.data(), &compressedSize,
                         uncompressedData.data(), uncompressedData.size());
    
    if (result == Z_OK) {
        batch.compressedData.resize(compressedSize);
        batch.originalSize = uncompressedData.size();
        batch.isCompressed = true;
        
        // Only use compression if it saves significant space
        if (compressedSize >= uncompressedData.size() * 0.9) {
            batch.isCompressed = false;
            batch.compressedData.clear();
        }
    }
}

void MessageManager::CleanupExpiredMessages() {
    while (_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        std::lock_guard<std::mutex> lock(_queueMutex);
        
        // Remove expired messages from outgoing queue
        auto it = std::remove_if(_outgoingQueue.begin(), _outgoingQueue.end(),
            [](const auto& msg) { return msg->IsExpired(30000); }); // 30 second timeout
        _outgoingQueue.erase(it, _outgoingQueue.end());
    }
}

MessageStatistics MessageManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(_queueMutex);
    
    MessageStatistics stats;
    stats.outgoingQueueSize = _outgoingQueue.size();
    stats.incomingQueueSize = _incomingQueue.size();
    stats.totalMessagesSent = _totalMessagesSent.load();
    stats.totalMessagesReceived = _totalMessagesReceived.load();
    stats.totalBytesCompressed = _totalBytesCompressed.load();
    stats.totalCompressionRatio = _totalCompressionRatio.load();
    
    return stats;
}

// VersionManager Implementation
VersionManager::VersionManager() {
    // Initialize supported features for current version
    _supportedFeatures.insert(ProtocolFeature::COMPRESSION);
    _supportedFeatures.insert(ProtocolFeature::MESSAGE_BATCHING);
    _supportedFeatures.insert(ProtocolFeature::PRIORITY_MESSAGES);
    _supportedFeatures.insert(ProtocolFeature::ERROR_RECOVERY);
    _supportedFeatures.insert(ProtocolFeature::BANDWIDTH_CONTROL);
}

bool VersionManager::IsVersionCompatible(uint16_t version) const {
    // Major version must match, minor version can be different
    uint16_t majorVersion = version >> 8;
    uint16_t currentMajor = PROTOCOL_VERSION >> 8;
    
    return majorVersion == currentMajor;
}

bool VersionManager::SupportsFeature(ProtocolFeature feature) const {
    return _supportedFeatures.find(feature) != _supportedFeatures.end();
}

std::vector<ProtocolFeature> VersionManager::GetSupportedFeatures() const {
    return std::vector<ProtocolFeature>(_supportedFeatures.begin(), _supportedFeatures.end());
}

CapabilityFlags VersionManager::GetCapabilityFlags() const {
    CapabilityFlags flags = 0;
    
    for (auto feature : _supportedFeatures) {
        flags |= (1 << static_cast<int>(feature));
    }
    
    return flags;
}

void VersionManager::NegotiateCapabilities(CapabilityFlags remoteFlags) {
    _negotiatedFeatures.clear();
    
    for (auto feature : _supportedFeatures) {
        if (remoteFlags & (1 << static_cast<int>(feature))) {
            _negotiatedFeatures.insert(feature);
        }
    }
}

// CompressionManager Implementation
CompressionManager::CompressionManager(const DiztinguishConfig& config) 
    : _config(config) {}

std::vector<uint8_t> CompressionManager::Compress(const std::vector<uint8_t>& data) {
    if (!_config.connection.enableCompression || data.size() < MIN_COMPRESSION_SIZE) {
        return data; // Return uncompressed
    }
    
    // Choose compression algorithm based on data characteristics
    CompressionType type = SelectOptimalCompression(data);
    
    switch (type) {
        case CompressionType::ZLIB:
            return CompressZlib(data);
        case CompressionType::LZ4:
            return CompressLZ4(data);
        default:
            return data;
    }
}

std::vector<uint8_t> CompressionManager::Decompress(const std::vector<uint8_t>& data, 
                                                   CompressionType type) {
    switch (type) {
        case CompressionType::ZLIB:
            return DecompressZlib(data);
        case CompressionType::LZ4:
            return DecompressLZ4(data);
        default:
            return data;
    }
}

CompressionType CompressionManager::SelectOptimalCompression(const std::vector<uint8_t>& data) {
    // Analyze data characteristics for optimal compression
    if (data.size() > 1024) {
        // For larger data, analyze entropy
        double entropy = CalculateEntropy(data);
        if (entropy < 6.0) {
            return CompressionType::ZLIB; // High compression ratio
        }
    }
    
    return CompressionType::LZ4; // Fast compression for real-time data
}

std::vector<uint8_t> CompressionManager::CompressZlib(const std::vector<uint8_t>& data) {
    uLongf compressedSize = compressBound(data.size());
    std::vector<uint8_t> compressed(compressedSize + 4); // +4 for original size
    
    // Store original size in first 4 bytes
    *reinterpret_cast<uint32_t*>(compressed.data()) = static_cast<uint32_t>(data.size());
    
    int result = compress(compressed.data() + 4, &compressedSize,
                         data.data(), data.size());
    
    if (result != Z_OK) {
        return data; // Return original on compression failure
    }
    
    compressed.resize(compressedSize + 4);
    return compressed;
}

std::vector<uint8_t> CompressionManager::DecompressZlib(const std::vector<uint8_t>& data) {
    if (data.size() < 4) {
        return data;
    }
    
    uint32_t originalSize = *reinterpret_cast<const uint32_t*>(data.data());
    std::vector<uint8_t> decompressed(originalSize);
    
    uLongf decompressedSize = originalSize;
    int result = uncompress(decompressed.data(), &decompressedSize,
                           data.data() + 4, data.size() - 4);
    
    if (result != Z_OK) {
        return std::vector<uint8_t>(); // Return empty on failure
    }
    
    decompressed.resize(decompressedSize);
    return decompressed;
}

std::vector<uint8_t> CompressionManager::CompressLZ4(const std::vector<uint8_t>& data) {
    // Placeholder for LZ4 implementation
    // Would require LZ4 library integration
    return data;
}

std::vector<uint8_t> CompressionManager::DecompressLZ4(const std::vector<uint8_t>& data) {
    // Placeholder for LZ4 implementation
    // Would require LZ4 library integration
    return data;
}

double CompressionManager::CalculateEntropy(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return 0.0;
    }
    
    // Calculate frequency distribution
    std::array<int, 256> frequency = {};
    for (uint8_t byte : data) {
        frequency[byte]++;
    }
    
    // Calculate Shannon entropy
    double entropy = 0.0;
    double size = static_cast<double>(data.size());
    
    for (int freq : frequency) {
        if (freq > 0) {
            double probability = freq / size;
            entropy -= probability * std::log2(probability);
        }
    }
    
    return entropy;
}

// BandwidthManager Implementation
BandwidthManager::BandwidthManager(const DiztinguishConfig& config)
    : _config(config), _currentBandwidth(0), _targetBandwidth(config.performance.maxBandwidthKBps) {
    
    _lastUpdateTime = std::chrono::steady_clock::now();
}

bool BandwidthManager::CanSendMessage(size_t messageSize) {
    UpdateBandwidthUsage();
    
    if (_config.performance.adaptiveBandwidth) {
        AdaptBandwidth();
    }
    
    // Check if sending this message would exceed bandwidth limit
    double projectedUsage = _currentBandwidth + (messageSize * 1000.0 / 1024.0); // Convert to KB/s
    
    return projectedUsage <= _targetBandwidth;
}

void BandwidthManager::RecordSentMessage(size_t messageSize) {
    auto now = std::chrono::steady_clock::now();
    _sentMessages.emplace_back(now, messageSize);
    
    // Remove old entries (older than 1 second)
    auto cutoff = now - std::chrono::seconds(1);
    auto it = std::remove_if(_sentMessages.begin(), _sentMessages.end(),
        [cutoff](const auto& entry) { return entry.first < cutoff; });
    _sentMessages.erase(it, _sentMessages.end());
}

void BandwidthManager::UpdateBandwidthUsage() {
    auto now = std::chrono::steady_clock::now();
    
    // Calculate current bandwidth usage
    size_t totalBytes = 0;
    for (const auto& entry : _sentMessages) {
        totalBytes += entry.second;
    }
    
    _currentBandwidth = totalBytes / 1024.0; // Convert to KB/s
    _lastUpdateTime = now;
}

void BandwidthManager::AdaptBandwidth() {
    // Simple adaptive algorithm - can be enhanced with network condition detection
    if (_sentMessages.size() > _config.performance.maxBatchSize * 2) {
        // High message frequency, reduce target bandwidth slightly
        _targetBandwidth = std::max(_targetBandwidth * 0.95, 
                                   _config.performance.maxBandwidthKBps * 0.5);
    } else if (_sentMessages.size() < _config.performance.maxBatchSize / 2) {
        // Low message frequency, can increase bandwidth
        _targetBandwidth = std::min(_targetBandwidth * 1.05,
                                   static_cast<double>(_config.performance.maxBandwidthKBps));
    }
}

double BandwidthManager::GetCurrentBandwidth() const {
    return _currentBandwidth;
}

double BandwidthManager::GetTargetBandwidth() const {
    return _targetBandwidth;
}

// ErrorRecoveryManager Implementation
ErrorRecoveryManager::ErrorRecoveryManager(const DiztinguishConfig& config)
    : _config(config) {}

bool ErrorRecoveryManager::ShouldRetry(const DiztinguishMessage& message) const {
    return message.GetRetryCount() < _config.connection.maxRetries;
}

std::chrono::milliseconds ErrorRecoveryManager::CalculateRetryDelay(uint32_t retryCount) const {
    // Exponential backoff with jitter
    uint32_t baseDelay = _config.connection.retryDelayMs;
    uint32_t exponentialDelay = baseDelay * (1 << std::min(retryCount, 5u));
    
    // Add random jitter (±25%)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitterDist(-exponentialDelay / 4, exponentialDelay / 4);
    
    uint32_t finalDelay = exponentialDelay + jitterDist(gen);
    return std::chrono::milliseconds(finalDelay);
}

void ErrorRecoveryManager::RecordError(ErrorType type, const std::string& description) {
    auto now = std::chrono::steady_clock::now();
    
    ErrorRecord record = { now, type, description };
    _errorHistory.push_back(record);
    
    // Keep only recent errors (last hour)
    auto cutoff = now - std::chrono::hours(1);
    auto it = std::remove_if(_errorHistory.begin(), _errorHistory.end(),
        [cutoff](const auto& record) { return record.timestamp < cutoff; });
    _errorHistory.erase(it, _errorHistory.end());
    
    // Update error statistics
    _errorCounts[type]++;
}

bool ErrorRecoveryManager::IsConnectionHealthy() const {
    // Consider connection unhealthy if there have been too many recent errors
    auto now = std::chrono::steady_clock::now();
    auto recentCutoff = now - std::chrono::minutes(5);
    
    size_t recentErrors = std::count_if(_errorHistory.begin(), _errorHistory.end(),
        [recentCutoff](const auto& record) { return record.timestamp >= recentCutoff; });
    
    return recentErrors < 10; // Threshold for unhealthy connection
}

std::vector<ErrorRecord> ErrorRecoveryManager::GetRecentErrors(std::chrono::minutes timespan) const {
    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - timespan;
    
    std::vector<ErrorRecord> recent;
    std::copy_if(_errorHistory.begin(), _errorHistory.end(), std::back_inserter(recent),
        [cutoff](const auto& record) { return record.timestamp >= cutoff; });
    
    return recent;
}

} // namespace DiztinguishProtocol