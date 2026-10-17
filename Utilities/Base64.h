#pragma once
#include "pch.h"
#include <array>
#include <string_view>

/// <summary>
/// Compile-time Base64 decode lookup table generator (256 entries, -1 for invalid chars).
/// Declared at namespace scope so the constexpr result is available for static member init.
/// </summary>
namespace Base64Detail {
	constexpr std::array<int8_t, 256> MakeDecodeLut() {
		std::array<int8_t, 256> lut{};
		for (int i = 0; i < 256; i++) lut[i] = -1;
		const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		for (int i = 0; i < 64; i++) lut[static_cast<uint8_t>(chars[i])] = static_cast<int8_t>(i);
		return lut;
	}
	inline constexpr auto DecodeLut = MakeDecodeLut();
}

/// <summary>
/// Base64 encoding and decoding utilities with [[nodiscard]] safety attributes.
/// Implements standard Base64 alphabet (RFC 4648) with '=' padding.
/// All methods are static and header-only for zero-cost inline expansion.
/// </summary>
class Base64 {
public:
	/// <summary>
	/// Encode binary data to Base64 string representation.
	/// </summary>
	/// <param name="data">Vector of bytes to encode (passed by const ref to avoid copy)</param>
	/// <returns>Base64-encoded string with '=' padding to 4-byte boundary</returns>
	/// <remarks>
	/// Uses standard Base64 alphabet: A-Z, a-z, 0-9, +, /
	/// Output length is always a multiple of 4 characters (padded with '=').
	/// Each 3 input bytes encode to 4 output characters (33% size increase).
	/// Common uses: Save state serialization, network transmission, embedding binary in text.
	/// </remarks>
	[[nodiscard]] static string Encode(const vector<uint8_t>& data) {
		std::string out;

		int val = 0, valb = -6;
		for (uint8_t c : data) {
			val = (val << 8) + c;
			valb += 8;
			while (valb >= 0) {
				out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
				valb -= 6;
			}
		}
		if (valb > -6)
			out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
		while (out.size() % 4)
			out.push_back('=');
		return out;
	}

	/// <summary>
	/// Decode Base64 string back to binary data.
	/// </summary>
	/// <param name="in">Base64-encoded string to decode (string_view avoids copy)</param>
	/// <returns>Vector of decoded bytes</returns>
	/// <remarks>
	/// Accepts both padded and unpadded Base64 strings.
	/// Stops decoding at first invalid character (including '=' padding).
	/// Each 4 input characters decode to 3 output bytes (25% size decrease).
	/// Invalid characters are silently ignored (no exception thrown).
	/// Inverse operation of Encode() - roundtrip is lossless.
	/// Uses compile-time constexpr lookup table instead of rebuilding per call.
	/// </remarks>
	[[nodiscard]] static vector<uint8_t> Decode(std::string_view in) {
		vector<uint8_t> out;

		int val = 0, valb = -8;
		for (uint8_t c : in) {
			if (Base64Detail::DecodeLut[c] == -1)
				break;
			val = (val << 6) + Base64Detail::DecodeLut[c];
			valb += 6;
			if (valb >= 0) {
				out.push_back(static_cast<uint8_t>(val >> valb));
				valb -= 8;
			}
		}
		return out;
	}
};
