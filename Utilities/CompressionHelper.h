#pragma once
#include "pch.h"
#include "miniz.h"

/// <summary>
/// Compression utilities using miniz library (zlib-compatible deflate algorithm).
/// Provides simple interface for compressing/decompressing data with size headers.
/// </summary>
/// <remarks>
/// All methods are static - header-only utility class.
/// Uses miniz.h for zlib-compatible compression (deflate algorithm).
/// Compressed format: [original_size:4][compressed_size:4][compressed_data]
/// 
/// Use cases:
/// - Save state compression (reduce file size)
/// - Network transfer compression
/// - Memory snapshot compression
/// 
/// Security: Decompress() limits output to 10MB to prevent decompression bombs.
/// </remarks>
class CompressionHelper {
public:
	/// <summary>
	/// Compress string data using zlib deflate algorithm.
	/// </summary>
	/// <param name="data">String data to compress</param>
	/// <param name="compressionLevel">Compression level (0=none, 1=fast, 9=best, -1=default)</param>
	/// <param name="output">Output vector to append compressed data</param>
	/// <remarks>
	/// Output format:
	/// - Bytes 0-3: Original size (uint32_t)
	/// - Bytes 4-7: Compressed size (uint32_t)
	/// - Bytes 8+: Compressed data
	/// 
	/// Compression levels:
	/// - 0: No compression (store only)
	/// - 1: Fast compression (low CPU, larger output)
	/// - 6: Default compression (balanced)
	/// - 9: Best compression (high CPU, smallest output)
	/// 
	/// Uses compressBound() to allocate worst-case buffer size.
	/// Allocates temporary buffer (deleted after compression).
	/// </remarks>
	static void Compress(string data, int compressionLevel, vector<uint8_t>& output) {
		unsigned long compressedSize = compressBound((unsigned long)data.size());
		uint8_t* compressedData = new uint8_t[compressedSize];
		compress2(compressedData, &compressedSize, (unsigned char*)data.c_str(), (unsigned long)data.size(), compressionLevel);

		uint32_t size = (uint32_t)compressedSize;
		uint32_t originalSize = (uint32_t)data.size();
		output.insert(output.end(), (char*)&originalSize, (char*)&originalSize + sizeof(uint32_t));
		output.insert(output.end(), (char*)&size, (char*)&size + sizeof(uint32_t));
		output.insert(output.end(), (char*)compressedData, (char*)compressedData + compressedSize);
		delete[] compressedData;
	}

	/// <summary>
	/// Decompress data compressed by Compress().
	/// </summary>
	/// <param name="input">Compressed data with size headers</param>
	/// <param name="output">Output vector for decompressed data (resized automatically)</param>
	/// <returns>True if decompression succeeded, false if error or size limit exceeded</returns>
	/// <remarks>
	/// Expects input format from Compress():
	/// - Bytes 0-3: Original size (uint32_t)
	/// - Bytes 4-7: Compressed size (uint32_t)
	/// - Bytes 8+: Compressed data
	/// 
	/// Security: Rejects decompression if either size > 10MB (prevents decompression bombs).
	/// Returns false if:
	/// - Size headers indicate > 10MB data
	/// - uncompress() fails (corrupted data, invalid format)
	/// 
	/// Output vector is resized to decompressed size before decompression.
	/// </remarks>
	static bool Decompress(vector<uint8_t>& input, vector<uint8_t>& output) {
		uint32_t decompressedSize;
		uint32_t compressedSize;

		memcpy(&decompressedSize, input.data(), sizeof(uint32_t));
		memcpy(&compressedSize, input.data() + sizeof(uint32_t), sizeof(uint32_t));

		if (decompressedSize >= 1024 * 1024 * 10 || compressedSize >= 1024 * 1024 * 10) {
			// Limit to 10mb the data's size
			return false;
		}

		output.resize(decompressedSize, 0);

		unsigned long decompSize = decompressedSize;
		if (uncompress(output.data(), &decompSize, input.data() + sizeof(uint32_t) * 2, (unsigned long)input.size() - sizeof(uint32_t) * 2) != MZ_OK) {
			return false;
		}

		return true;
	}
};