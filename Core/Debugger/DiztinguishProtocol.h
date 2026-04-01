#pragma once
#include "pch.h"

// DiztinGUIsh streaming protocol definitions
// Version 1.0
// See docs/diztinguish/streaming-integration.md for full specification

namespace DiztinguishProtocol {

	// Protocol version
	constexpr uint16_t PROTOCOL_VERSION_MAJOR = 1;
	constexpr uint16_t PROTOCOL_VERSION_MINOR = 0;

#pragma pack(push, 1)  // Force 1-byte packing for protocol structures

	// Message types
	enum class MessageType : uint8_t {
		// Connection lifecycle
		Handshake = 0x01,
		HandshakeAck = 0x02,
		ConfigStream = 0x03,
		Heartbeat = 0x04,
		Disconnect = 0x05,

		// Execution trace streaming
		ExecTrace = 0x10,
		ExecTraceBatch = 0x11,

		// Memory and CDL
		MemoryAccess = 0x12,
		CdlUpdate = 0x13,
		CdlSnapshot = 0x14,

		// CPU state
		CpuState = 0x20,
		CpuStateRequest = 0x21,

		// Label synchronization
		LabelAdd = 0x30,
		LabelUpdate = 0x31,
		LabelDelete = 0x32,
		LabelSyncRequest = 0x33,
		LabelSyncResponse = 0x34,

		// Breakpoint control
		BreakpointAdd = 0x40,
		BreakpointRemove = 0x41,
		BreakpointHit = 0x42,
		BreakpointList = 0x43,

		// Memory dump
		MemoryDumpRequest = 0x50,
		MemoryDumpResponse = 0x51,

		// Error handling
		Error = 0xFF
	};

	// Message header (5 bytes)
	struct MessageHeader {
		MessageType type;
		uint32_t length;  // Payload length (little-endian)
	};

	// Handshake message (Mesen2 → DiztinGUIsh)
	struct HandshakeMessage {
		uint16_t protocolVersionMajor;
		uint16_t protocolVersionMinor;
		uint32_t romChecksum;  // CRC32 of ROM
		uint32_t romSize;
		char romName[256];     // Null-terminated
	};

	// Handshake acknowledgment (DiztinGUIsh → Mesen2)
	struct HandshakeAckMessage {
		uint16_t protocolVersionMajor;
		uint16_t protocolVersionMinor;
		uint8_t accepted;  // 0 = rejected, 1 = accepted
		char clientName[64];  // e.g., "DiztinGUIsh v2.0"
	};

	// Configuration message (DiztinGUIsh → Mesen2)
	struct ConfigStreamMessage {
		uint8_t enableExecTrace;    // 0/1
		uint8_t enableMemoryAccess; // 0/1
		uint8_t enableCdlUpdates;   // 0/1
		uint8_t traceFrameInterval; // Send every N frames (1-60)
		uint16_t maxTracesPerFrame; // Max traces per batch
	};

	// Execution trace entry (15 bytes)
	struct ExecTraceEntry {
		uint32_t pc;              // Program Counter (24-bit, padded)
		uint8_t opcode;           // Opcode byte
		uint8_t mFlag;            // M flag (0/1)
		uint8_t xFlag;            // X flag (0/1)
		uint8_t dbRegister;       // Data Bank register
		uint16_t dpRegister;      // Direct Page register
		uint32_t effectiveAddr;   // Calculated effective address (24-bit, padded)
	};

	// Execution trace batch message
	struct ExecTraceBatchMessage {
		uint32_t frameNumber;     // Frame number
		uint16_t entryCount;      // Number of trace entries following
		// Followed by entryCount × ExecTraceEntry
	};

	// Memory access entry
	struct MemoryAccessEntry {
		uint32_t address;         // SNES address (24-bit, padded)
		uint8_t value;            // Value read/written
		uint8_t type;             // 0=read, 1=write, 2=execute
	};

	// CDL update entry (differential)
	struct CdlUpdateEntry {
		uint32_t address;         // ROM offset
		uint8_t flags;            // CDL flags (bit 0=code, 1=data, 2=graphics, 3=pcm)
	};

	// CDL snapshot message
	struct CdlSnapshotMessage {
		uint32_t romSize;
		// Followed by romSize bytes of CDL data
	};

	// CPU state snapshot
	struct CpuStateSnapshot {
		uint16_t a;               // Accumulator
		uint16_t x;               // X register
		uint16_t y;               // Y register
		uint16_t s;               // Stack pointer
		uint16_t d;               // Direct page
		uint8_t db;               // Data bank
		uint32_t pc;              // Program counter (24-bit, padded)
		uint8_t p;                // Processor status
		uint8_t emulationMode;    // E flag
	};

	// Label message
	struct LabelMessage {
		uint32_t address;         // SNES address
		uint8_t type;             // 0=code, 1=data, 2=constant
		uint8_t deleted;          // For updates: 0=active, 1=deleted
		char name[256];           // Null-terminated label name
	};

	// Breakpoint message
	struct BreakpointMessage {
		uint32_t address;         // SNES address
		uint8_t type;             // 0=execute, 1=read, 2=write, 3=access
		uint8_t enabled;          // 0=disabled, 1=enabled
	};

	// Breakpoint hit notification (Mesen2 → DiztinGUIsh)
	struct BreakpointHitMessage {
		uint32_t address;
		uint8_t type;
		CpuStateSnapshot cpuState;
	};

	// Memory dump request (DiztinGUIsh → Mesen2)
	struct MemoryDumpRequest {
		uint8_t memoryType;       // 0=ROM, 1=WRAM, 2=SRAM, 3=VRAM
		uint32_t startAddress;
		uint32_t length;
	};

	// Memory dump response (Mesen2 → DiztinGUIsh)
	struct MemoryDumpResponse {
		uint8_t memoryType;
		uint32_t startAddress;
		uint32_t length;
		// Followed by length bytes of memory data
	};

	// Error message
	struct ErrorMessage {
		uint8_t errorCode;        // Error code
		char message[256];        // Null-terminated error description
	};

	// Error codes
	enum class ErrorCode : uint8_t {
		None = 0,
		ProtocolVersionMismatch = 1,
		RomChecksumMismatch = 2,
		InvalidMessage = 3,
		InvalidAddress = 4,
		MemoryAccessFailed = 5,
		BreakpointFailed = 6,
		LabelFailed = 7,
		InternalError = 0xFF
	};

	// Message direction
	enum class MessageDirection {
		ClientToServer,    // DiztinGUIsh → Mesen2
		ServerToClient,    // Mesen2 → DiztinGUIsh
		Bidirectional      // Both directions
	};

	// Helper: Get message direction
	inline MessageDirection GetMessageDirection(MessageType type) {
		switch(type) {
			case MessageType::Handshake:
			case MessageType::ExecTrace:
			case MessageType::ExecTraceBatch:
			case MessageType::MemoryAccess:
			case MessageType::CdlUpdate:
			case MessageType::CdlSnapshot:
			case MessageType::CpuState:
			case MessageType::BreakpointHit:
			case MessageType::LabelSyncResponse:
			case MessageType::MemoryDumpResponse:
				return MessageDirection::ServerToClient;

			case MessageType::HandshakeAck:
			case MessageType::ConfigStream:
			case MessageType::CpuStateRequest:
			case MessageType::LabelAdd:
			case MessageType::LabelUpdate:
			case MessageType::LabelDelete:
			case MessageType::LabelSyncRequest:
			case MessageType::BreakpointAdd:
			case MessageType::BreakpointRemove:
			case MessageType::MemoryDumpRequest:
				return MessageDirection::ClientToServer;

			case MessageType::Heartbeat:
			case MessageType::Disconnect:
			case MessageType::Error:
				return MessageDirection::Bidirectional;

			default:
				return MessageDirection::Bidirectional;
		}
	}

	// Helper: Get message type name
	inline const char* GetMessageTypeName(MessageType type) {
		switch(type) {
			case MessageType::Handshake: return "Handshake";
			case MessageType::HandshakeAck: return "HandshakeAck";
			case MessageType::ConfigStream: return "ConfigStream";
			case MessageType::Heartbeat: return "Heartbeat";
			case MessageType::Disconnect: return "Disconnect";
			case MessageType::ExecTrace: return "ExecTrace";
			case MessageType::ExecTraceBatch: return "ExecTraceBatch";
			case MessageType::MemoryAccess: return "MemoryAccess";
			case MessageType::CdlUpdate: return "CdlUpdate";
			case MessageType::CdlSnapshot: return "CdlSnapshot";
			case MessageType::CpuState: return "CpuState";
			case MessageType::CpuStateRequest: return "CpuStateRequest";
			case MessageType::LabelAdd: return "LabelAdd";
			case MessageType::LabelUpdate: return "LabelUpdate";
			case MessageType::LabelDelete: return "LabelDelete";
			case MessageType::LabelSyncRequest: return "LabelSyncRequest";
			case MessageType::LabelSyncResponse: return "LabelSyncResponse";
			case MessageType::BreakpointAdd: return "BreakpointAdd";
			case MessageType::BreakpointRemove: return "BreakpointRemove";
			case MessageType::BreakpointHit: return "BreakpointHit";
			case MessageType::BreakpointList: return "BreakpointList";
			case MessageType::MemoryDumpRequest: return "MemoryDumpRequest";
			case MessageType::MemoryDumpResponse: return "MemoryDumpResponse";
			case MessageType::Error: return "Error";
			default: return "Unknown";
		}
	}

#pragma pack(pop)  // Restore default packing

} // namespace DiztinguishProtocol
