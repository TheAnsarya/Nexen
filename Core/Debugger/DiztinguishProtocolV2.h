#pragma once
#include "pch.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

// Enhanced DiztinGUIsh protocol with versioning, compression, and advanced features
namespace DiztinguishProtocolV2
{
	// Protocol version and capabilities
	constexpr uint16_t PROTOCOL_VERSION = 0x0200;  // Version 2.0
	constexpr uint32_t MAGIC_HEADER = 0x44495A54;  // "DIZT"
	
	// Enhanced message types
	enum class MessageType : uint16_t {
		// Core protocol (v1.x compatibility)
		Handshake         = 0x0001,
		HandshakeAck      = 0x0002,
		ConfigStream      = 0x0003,
		CpuState          = 0x0004,
		ExecTrace         = 0x0005,
		CdlUpdate         = 0x0006,
		LabelAdd          = 0x0007,
		LabelUpdate       = 0x0008,
		LabelDelete       = 0x0009,
		LabelSync         = 0x000A,
		BreakpointAdd     = 0x000B,
		BreakpointRemove  = 0x000C,
		MemoryDump        = 0x000D,
		Error             = 0x000E,
		Heartbeat         = 0x000F,
		Disconnect        = 0x0010,
		
		// Enhanced protocol v2.0
		ProtocolInfo      = 0x0100,  // Protocol capabilities and version
		CompressionConfig = 0x0101,  // Compression settings
		BandwidthControl  = 0x0102,  // Bandwidth limitation
		PerformanceStats  = 0x0103,  // Real-time performance data
		BatchedMessages   = 0x0104,  // Multiple messages in one packet
		
		// Advanced features
		MemoryWatch       = 0x0200,  // Watch memory address for changes
		CodeCoverage      = 0x0201,  // Code coverage information
		CallStack         = 0x0202,  // Function call stack
		ProfilerData      = 0x0203,  // Performance profiling data
		GameStateSnapshot = 0x0204,  // Complete game state
		
		// Debug and diagnostics
		DebugLog          = 0x0300,  // Debug log messages
		ConnectionTest    = 0x0301,  // Connection latency test
		BandwidthTest     = 0x0302,  // Bandwidth measurement
		ErrorRecovery     = 0x0303,  // Error recovery information
		
		// Custom extensions
		CustomCommand     = 0xF000,  // Custom user-defined commands
	};
	
	// Enhanced error codes
	enum class ErrorCode : uint16_t {
		None                    = 0x0000,
		UnknownMessage          = 0x0001,
		InvalidData             = 0x0002,
		ProtocolMismatch        = 0x0003,
		BufferOverflow          = 0x0004,
		CompressionError        = 0x0005,
		BandwidthExceeded       = 0x0006,
		AuthenticationFailed    = 0x0007,
		FeatureNotSupported     = 0x0008,
		ResourceExhausted       = 0x0009,
		ConnectionLost          = 0x000A,
	};
	
	// Protocol capabilities flags
	enum class CapabilityFlags : uint32_t {
		None              = 0x00000000,
		Compression       = 0x00000001,  // Supports data compression
		BatchedMessages   = 0x00000002,  // Supports message batching
		BandwidthControl  = 0x00000004,  // Supports bandwidth limiting
		AdvancedStats     = 0x00000008,  // Supports performance statistics
		MemoryWatch       = 0x00000010,  // Supports memory watching
		CodeCoverage      = 0x00000020,  // Supports code coverage
		ProfilerData      = 0x00000040,  // Supports profiler data
		CustomCommands    = 0x00000080,  // Supports custom commands
		ErrorRecovery     = 0x00000100,  // Supports error recovery
		Encryption        = 0x00000200,  // Supports data encryption (future)
	};
	
	// Compression algorithms
	enum class CompressionType : uint8_t {
		None        = 0x00,
		ZLib        = 0x01,  // Standard zlib compression
		LZ4         = 0x02,  // Fast LZ4 compression
		Custom      = 0xFF,  // Custom compression algorithm
	};
	
	// Enhanced message header
	#pragma pack(push, 1)
	struct MessageHeader {
		uint32_t magic;           // Magic header (MAGIC_HEADER)
		uint16_t version;         // Protocol version
		MessageType type;         // Message type
		uint16_t flags;           // Message flags (compressed, encrypted, etc.)
		uint32_t length;          // Payload length
		uint32_t sequence;        // Message sequence number
		uint64_t timestamp;       // Message timestamp (microseconds)
		uint32_t checksum;        // CRC32 checksum of payload
	};
	#pragma pack(pop)
	
	// Message flags
	constexpr uint16_t FLAG_COMPRESSED    = 0x0001;
	constexpr uint16_t FLAG_ENCRYPTED     = 0x0002;
	constexpr uint16_t FLAG_FRAGMENTED    = 0x0004;
	constexpr uint16_t FLAG_URGENT        = 0x0008;
	constexpr uint16_t FLAG_RESPONSE_REQ  = 0x0010;
	constexpr uint16_t FLAG_LAST_FRAGMENT = 0x0020;
	
	// Enhanced protocol info message
	#pragma pack(push, 1)
	struct ProtocolInfoMessage {
		uint16_t serverVersion;       // Server protocol version
		uint32_t capabilities;        // Supported capability flags
		uint32_t maxMessageSize;      // Maximum message size
		uint16_t compressionTypes;    // Supported compression types (bitmask)
		uint32_t maxBandwidthKBps;    // Maximum bandwidth (KB/s)
		char serverName[64];          // Server identification
		char serverVersion_str[32];   // Human-readable version
	};
	#pragma pack(pop)
	
	// Compression configuration
	#pragma pack(push, 1)
	struct CompressionConfigMessage {
		CompressionType type;         // Compression algorithm
		uint8_t level;                // Compression level (0-9)
		uint32_t minSize;             // Minimum size to compress
		uint16_t reserved;            // Reserved for future use
	};
	#pragma pack(pop)
	
	// Bandwidth control
	#pragma pack(push, 1)
	struct BandwidthControlMessage {
		uint32_t maxBandwidthKBps;    // Maximum bandwidth limit
		uint32_t currentBandwidthKBps; // Current bandwidth usage
		uint16_t priority;            // Message priority (0=highest, 255=lowest)
		uint8_t flags;                // Control flags
		uint8_t reserved;             // Reserved
	};
	#pragma pack(pop)
	
	// Performance statistics
	#pragma pack(push, 1)
	struct PerformanceStatsMessage {
		uint64_t messagesProcessed;   // Total messages processed
		uint64_t bytesTransferred;    // Total bytes transferred
		uint32_t averageLatencyUs;    // Average message latency (microseconds)
		uint32_t peakLatencyUs;       // Peak message latency
		uint32_t currentBandwidthKBps; // Current bandwidth
		uint32_t cpuUsagePercent;     // CPU usage (0-10000, 100 = 1%)
		uint32_t memoryUsageMB;       // Memory usage in MB
		uint32_t queuedMessages;      // Messages in queue
		uint64_t droppedMessages;     // Messages dropped due to overflow
		uint32_t errorCount;          // Number of errors encountered
	};
	#pragma pack(pop)
	
	// Message management class
	class MessageManager
	{
	private:
		uint32_t _sequenceNumber = 0;
		std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> _messageTimestamps;
		std::vector<uint8_t> _sendBuffer;
		std::vector<uint8_t> _receiveBuffer;
		
		// Statistics
		uint64_t _totalMessages = 0;
		uint64_t _totalBytes = 0;
		uint64_t _compressedBytes = 0;
		uint32_t _averageLatency = 0;
		
		// Compression state
		CompressionType _compressionType = CompressionType::None;
		uint8_t _compressionLevel = 6;
		uint32_t _compressionMinSize = 1024;
		
	public:
		MessageManager();
		~MessageManager();
		
		// Message creation
		std::vector<uint8_t> CreateMessage(MessageType type, const void* payload, uint32_t length);
		std::vector<uint8_t> CreateBatchedMessage(const std::vector<std::pair<MessageType, std::vector<uint8_t>>>& messages);
		
		// Message parsing
		bool ParseMessage(const std::vector<uint8_t>& data, MessageHeader& header, std::vector<uint8_t>& payload);
		bool ValidateMessage(const MessageHeader& header, const std::vector<uint8_t>& payload);
		
		// Compression
		void SetCompression(CompressionType type, uint8_t level, uint32_t minSize);
		std::vector<uint8_t> CompressData(const std::vector<uint8_t>& data);
		std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& data);
		
		// Statistics
		uint64_t GetTotalMessages() const { return _totalMessages; }
		uint64_t GetTotalBytes() const { return _totalBytes; }
		uint64_t GetCompressedBytes() const { return _compressedBytes; }
		double GetCompressionRatio() const;
		uint32_t GetAverageLatency() const { return _averageLatency; }
		
		// Message tracking
		void TrackMessageSent(uint32_t sequence);
		void TrackMessageReceived(uint32_t sequence);
		uint32_t GetCurrentSequence() const { return _sequenceNumber; }
		
		// Utility functions
		static uint32_t CalculateChecksum(const void* data, uint32_t length);
		static uint64_t GetTimestamp();
		static std::string MessageTypeToString(MessageType type);
		static std::string ErrorCodeToString(ErrorCode code);
	};
	
	// Protocol version compatibility
	class VersionManager
	{
	public:
		static bool IsVersionSupported(uint16_t version);
		static uint16_t GetLatestVersion();
		static std::vector<uint16_t> GetSupportedVersions();
		static bool IsFeatureSupported(uint16_t version, CapabilityFlags feature);
	};
}

// Convenience aliases
using DZProtocol = DiztinguishProtocolV2;
using DZMessageType = DiztinguishProtocolV2::MessageType;
using DZErrorCode = DiztinguishProtocolV2::ErrorCode;