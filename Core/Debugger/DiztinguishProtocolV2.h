#pragma once

// Simple V2 protocol structures for memory dump functionality
// This is a simplified version focusing on core functionality

namespace DiztinguishProtocolV2
{
	// Memory regions for SNES
	enum class MemoryRegion : uint16_t {
		ROM           = 0x0001,  // ROM data
		WRAM          = 0x0002,  // Work RAM
		SRAM          = 0x0003,  // Save RAM
		VRAM          = 0x0004,  // Video RAM
		OAM           = 0x0005,  // Object Attribute Memory
		CGRAM         = 0x0006,  // Color Generator RAM
		APU_RAM       = 0x0007,  // APU RAM
		CPU_REGISTERS = 0x0008,  // CPU register state
		PPU_REGISTERS = 0x0009,  // PPU register state
		APU_REGISTERS = 0x000A,  // APU register state
		CUSTOM_RANGE  = 0x00FF   // Custom address range
	};

	// Memory dump status codes
	enum class DumpStatus : uint16_t {
		SUCCESS              = 0x0000,
		INVALID_REGION       = 0x0001,
		INVALID_ADDRESS      = 0x0002,
		INVALID_LENGTH       = 0x0003,
		MEMORY_NOT_AVAILABLE = 0x0004,
		REGION_NOT_MAPPED    = 0x0005,
		BUFFER_TOO_LARGE     = 0x0006
	};

	// Memory dump request message
	#pragma pack(push, 1)
	struct MemoryDumpRequest {
		MemoryRegion region;         // Memory region to dump
		uint32_t startAddress;       // Start address
		uint32_t length;             // Number of bytes to dump
		uint32_t requestId;          // Request identifier for tracking
	};

	// Memory dump response message
	struct MemoryDumpResponse {
		MemoryRegion region;         // Memory region dumped
		uint32_t startAddress;       // Actual start address
		uint32_t actualLength;       // Actual bytes dumped
		uint16_t status;             // Status code (0=success)
		uint32_t requestId;          // Original request ID
		uint32_t checksum;           // Data checksum for verification
		// Memory data follows this header
	};
	#pragma pack(pop)

}