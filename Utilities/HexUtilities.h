#pragma once
#include "pch.h"

/// <summary>
/// Provides utilities for converting between hexadecimal strings and numeric values.
/// Optimized with cached hex strings for common byte values.
/// </summary>
class HexUtilities {
private:
	const static vector<string> _hexCache;

public:
	/// <summary>
	/// Converts an 8-bit value to a 2-character hex string.
	/// </summary>
	/// <param name="value">8-bit unsigned integer (0-255)</param>
	/// <returns>2-character hex string (e.g., "0A", "FF")</returns>
	/// <remarks>Uses cached strings for improved performance.</remarks>
	static string ToHex(uint8_t value);

	/// <summary>
	/// Converts an 8-bit value to a hex C-string pointer.
	/// </summary>
	/// <param name="value">8-bit unsigned integer (0-255)</param>
	/// <returns>Pointer to cached 2-character hex string (e.g., "0A", "FF")</returns>
	/// <remarks>
	/// Returns a pointer to a static cached string - do not free.
	/// Faster than ToHex(uint8_t) when a C-string is needed.
	/// </remarks>
	static const char* ToHexChar(uint8_t value);

	/// <summary>
	/// Converts a 16-bit value to a 4-character hex string.
	/// </summary>
	/// <param name="value">16-bit unsigned integer</param>
	/// <returns>4-character hex string (e.g., "00FF", "1234")</returns>
	static string ToHex(uint16_t value);

	/// <summary>
	/// Converts a 32-bit value to hex string (variable or fixed length).
	/// </summary>
	/// <param name="value">32-bit unsigned integer</param>
	/// <param name="fullSize">If true, always outputs 8 characters; if false, outputs minimum needed (default)</param>
	/// <returns>Hex string (e.g., "FF" or "000000FF" depending on fullSize)</returns>
	static string ToHex(uint32_t value, bool fullSize = false);

	/// <summary>
	/// Converts a signed 32-bit value to hex string (variable or fixed length).
	/// </summary>
	/// <param name="value">32-bit signed integer</param>
	/// <param name="fullSize">If true, always outputs 8 characters; if false, outputs minimum needed (default)</param>
	/// <returns>Hex string representation of the value's binary form</returns>
	static string ToHex(int32_t value, bool fullSize = false);

	/// <summary>
	/// Converts a 32-bit value to a 5-character hex string (20-bit address).
	/// </summary>
	/// <param name="value">32-bit unsigned integer (typically a 20-bit address)</param>
	/// <returns>5-character hex string (e.g., "0FFFF")</returns>
	/// <remarks>Used for SNES LoROM/HiROM 24-bit addressing.</remarks>
	static string ToHex20(uint32_t value);

	/// <summary>
	/// Converts a 32-bit value to a 6-character hex string (24-bit address).
	/// </summary>
	/// <param name="value">32-bit signed integer (typically a 24-bit address)</param>
	/// <returns>6-character hex string (e.g., "00FFFF")</returns>
	/// <remarks>Used for SNES 24-bit addressing.</remarks>
	static string ToHex24(int32_t value);

	/// <summary>
	/// Converts a 32-bit value to an 8-character hex string (full 32-bit).
	/// </summary>
	/// <param name="value">32-bit unsigned integer</param>
	/// <returns>8-character hex string (e.g., "0000FFFF")</returns>
	static string ToHex32(uint32_t value);

	/// <summary>
	/// Converts a 64-bit value to a 16-character hex string.
	/// </summary>
	/// <param name="value">64-bit unsigned integer</param>
	/// <returns>16-character hex string</returns>
	static string ToHex(uint64_t value);

	/// <summary>
	/// Converts a byte vector to a delimited hex string.
	/// </summary>
	/// <param name="data">Vector of bytes to convert</param>
	/// <param name="delimiter">Optional delimiter character between bytes (default: none)</param>
	/// <returns>Hex string of all bytes, optionally separated by delimiter (e.g., "0A1BFF" or "0A,1B,FF")</returns>
	static string ToHex(vector<uint8_t>& data, char delimiter = 0);

	/// <summary>
	/// Parses a hexadecimal string to an integer.
	/// </summary>
	/// <param name="hex">Hex string to parse (may include "0x" prefix)</param>
	/// <returns>Integer value of the hex string, or -1 on parse error</returns>
	/// <remarks>Handles both "FF" and "0xFF" formats.</remarks>
	static int FromHex(string hex);
};