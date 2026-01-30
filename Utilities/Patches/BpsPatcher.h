#pragma once

#include "pch.h"

/// <summary>
/// BPS (Binary Patching System) patch format implementation.
/// Modern ROM patching format with CRC32 validation and delta encoding.
/// </summary>
/// <remarks>
/// BPS format advantages over IPS:
/// - CRC32 validation (source, target, patch)
/// - Variable-length integer encoding (compact)
/// - Delta encoding (stores differences efficiently)
/// - No file size limit
/// - Metadata support
///
/// Format structure:
/// - Header: "BPS1" + source size + target size + metadata size + metadata
/// - Actions: SourceRead, TargetRead, SourceCopy, TargetCopy (variable-length encoded)
/// - Footer: source CRC32 + target CRC32 + patch CRC32
///
/// Used by: ROM hackers, emulator developers, game modders.
/// Reference: https://www.romhacking.net/documents/746/
/// </remarks>
class BpsPatcher {
private:
	/// <summary>
	/// Read variable-length base-128 encoded integer from stream.
	/// </summary>
	/// <param name="file">Input stream</param>
	/// <returns>Decoded 64-bit integer</returns>
	/// <remarks>
	/// BPS variable-length encoding:
	/// - Uses 7 bits per byte for data
	/// - Bit 7 indicates continuation (1 = more bytes, 0 = last byte)
	/// - Little-endian byte order
	/// </remarks>
	static int64_t ReadBase128Number(std::istream& file);

public:
	/// <summary>
	/// Apply BPS patch from stream to input buffer.
	/// </summary>
	/// <param name="bpsFile">Input stream containing BPS patch</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied and validated successfully</returns>
	/// <remarks>
	/// Validates source CRC32 before patching.
	/// Validates target CRC32 after patching.
	/// Returns false if validation fails.
	/// </remarks>
	static bool PatchBuffer(std::istream& bpsFile, vector<uint8_t>& input, vector<uint8_t>& output);

	/// <summary>
	/// Apply BPS patch from file to input buffer.
	/// </summary>
	/// <param name="bpsFilepath">Path to BPS patch file</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied and validated successfully</returns>
	static bool PatchBuffer(string bpsFilepath, vector<uint8_t>& input, vector<uint8_t>& output);
};