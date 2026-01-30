#pragma once

#include "pch.h"

/// <summary>
/// IPS (International Patching System) patch format implementation.
/// Classic ROM patching format used for NES/SNES/etc. game modifications.
/// </summary>
/// <remarks>
/// IPS format structure:
/// - Header: "PATCH" (5 bytes)
/// - Records: [offset:3][size:2][data:size] (repeating)
/// - RLE records: [offset:3][0000][size:2][byte:1] (for repeated bytes)
/// - Footer: "EOF" (3 bytes)
///
/// Limitations:
/// - Maximum file size: 16MB (24-bit addresses)
/// - No CRC validation
/// - No metadata support
/// - Simple run-length encoding only
///
/// Common use: ROM hacks, translations, bug fixes for retro games.
/// Superseded by UPS/BPS formats (better validation, larger files).
/// </remarks>
class IpsPatcher {
public:
	/// <summary>
	/// Apply IPS patch from file to input buffer.
	/// </summary>
	/// <param name="ipsFilepath">Path to IPS patch file</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied successfully</returns>
	static bool PatchBuffer(string ipsFilepath, vector<uint8_t>& input, vector<uint8_t>& output);

	/// <summary>
	/// Apply IPS patch from memory buffer.
	/// </summary>
	/// <param name="ipsData">IPS patch data</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied successfully</returns>
	static bool PatchBuffer(vector<uint8_t>& ipsData, vector<uint8_t>& input, vector<uint8_t>& output);

	/// <summary>
	/// Apply IPS patch from stream.
	/// </summary>
	/// <param name="ipsFile">Input stream containing IPS patch</param>
	/// <param name="input">Original ROM data</param>
	/// <param name="output">Patched ROM data (output)</param>
	/// <returns>True if patch applied successfully</returns>
	static bool PatchBuffer(std::istream& ipsFile, vector<uint8_t>& input, vector<uint8_t>& output);

	/// <summary>
	/// Create IPS patch from original and modified data.
	/// </summary>
	/// <param name="originalData">Original unmodified data</param>
	/// <param name="newData">Modified data</param>
	/// <returns>IPS patch data vector</returns>
	/// <remarks>
	/// Generates minimal IPS patch by finding differences between buffers.
	/// Uses RLE encoding for repeated bytes.
	/// Limited to 16MB files due to 24-bit addressing.
	/// </remarks>
	static vector<uint8_t> CreatePatch(vector<uint8_t> originalData, vector<uint8_t> newData);
};