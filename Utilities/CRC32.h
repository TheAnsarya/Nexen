#pragma once
#include "pch.h"

/// <summary>
/// CRC32 checksum calculation utilities using slice-by-16 algorithm for optimal performance.
/// All public methods are marked [[nodiscard]] to prevent accidentally discarding checksum results.
/// Implementation adapted from https://github.com/stbrumme/crc32 (zlib license).
/// </summary>
class CRC32 {
private:
	/// <summary>
	/// Internal optimized CRC32 calculation using slice-by-16 algorithm.
	/// Processes 16 bytes at a time for improved throughput.
	/// </summary>
	/// <param name="data">Pointer to data to checksum</param>
	/// <param name="length">Number of bytes to process</param>
	/// <param name="previousCrc32">Previous CRC32 value for incremental calculations (0xFFFFFFFF for new calculation)</param>
	/// <returns>Updated CRC32 value (NOT finalized - requires XOR with 0xFFFFFFFF)</returns>
	static uint32_t crc32_16bytes(const void* data, size_t length, uint32_t previousCrc32);

public:
	/// <summary>
	/// Calculate CRC32 checksum for a memory buffer.
	/// </summary>
	/// <param name="buffer">Pointer to buffer data</param>
	/// <param name="length">Number of bytes to checksum</param>
	/// <returns>32-bit CRC32 checksum value</returns>
	/// <remarks>
	/// Common use cases: ROM validation, save state integrity, patch verification.
	/// Uses slice-by-16 algorithm for high performance (processes 16 bytes per iteration).
	/// </remarks>
	[[nodiscard]] static uint32_t GetCRC(uint8_t* buffer, std::streamoff length);

	/// <summary>
	/// Calculate CRC32 checksum for a byte vector.
	/// </summary>
	/// <param name="data">Vector of bytes to checksum</param>
	/// <returns>32-bit CRC32 checksum value</returns>
	/// <remarks>
	/// Convenience wrapper for vector data structures.
	/// Uses same slice-by-16 algorithm as buffer variant.
	/// </remarks>
	[[nodiscard]] static uint32_t GetCRC(vector<uint8_t>& data);

	/// <summary>
	/// Calculate CRC32 checksum for an entire file.
	/// </summary>
	/// <param name="filename">Path to file to checksum</param>
	/// <returns>32-bit CRC32 checksum value, or 0 if file cannot be read</returns>
	/// <remarks>
	/// Reads entire file into memory before calculating checksum.
	/// Returns 0 on file read errors (no exception thrown).
	/// Common use: ROM file verification against known-good checksums.
	/// </remarks>
	[[nodiscard]] static uint32_t GetCRC(string filename);
};
