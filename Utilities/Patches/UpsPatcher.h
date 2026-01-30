#pragma once

#include "pch.h"

/// <summary>
/// UPS (Universal Patching System) patch format implementation.
/// Alternative modern ROM patching format with CRC32 validation and XOR encoding.
/// </summary>
/// <remarks>
/// UPS format features:
/// - CRC32 validation (source, target, patch)
/// - Variable-length integer encoding
/// - XOR-based encoding (reversible patches)
/// - No file size limit
/// - Bidirectional patching support
///
/// Format structure:
/// - Header: "UPS1" + source size + target size
/// - Changes: [offset delta][XOR data] (variable-length encoded)
/// - Footer: source CRC32 + target CRC32 + patch CRC32
///
/// Advantages over IPS: CRC validation, unlimited file size, reversible patches.
/// Comparison to BPS: Similar features, different encoding (XOR vs delta).
///
/// Used by: ROM hackers, emulator savestates, game modders.
/// Reference: https://www.romhacking.net/documents/392/
/// </remarks>
class UpsPatcher {
private:
	/// <summary>
	/// Read variable-length base-128 encoded integer from stream.
	/// </summary>
	/// <param name="file">Input stream</param>
	/// <returns>Decoded 64-bit integer</returns>
	/// <remarks>
	/// UPS variable-length encoding (same as BPS):
	/// - Uses 7 bits per byte for data
	/// - Bit 7 indicates continuation (1 = more bytes, 0 = last byte)
	/// - Little-endian byte order
	/// </remarks>
	static int64_t ReadBase128Number(std::istream& file);

public:
	/// <summary>
	/// Apply UPS patch from stream to input buffer.
	/// </summary>
	/// <param name="upsFile">Input stream containing UPS patch</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied and validated successfully</returns>
	/// <remarks>
	/// Validates source CRC32 before patching.
	/// Validates target CRC32 after patching.
	/// Returns false if validation fails.
	/// XOR encoding allows bidirectional patching (can undo patch).
	/// </remarks>
	static bool PatchBuffer(std::istream& upsFile, vector<uint8_t>& input, vector<uint8_t>& output);

	/// <summary>
	/// Apply UPS patch from file to input buffer.
	/// </summary>
	/// <param name="upsFilepath">Path to UPS patch file</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied and validated successfully</returns>
	static bool PatchBuffer(string upsFilepath, vector<uint8_t>& input, vector<uint8_t>& output);
};