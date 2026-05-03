#pragma once
#include "pch.h"
#include <algorithm>
#include <cctype>
#include <ranges>

/// <summary>
/// Provides utility functions for string manipulation and parsing.
/// All functions are marked [[nodiscard]] to prevent accidental result discard.
/// Case conversion functions use std::ranges::transform (C++23).
/// </summary>
class StringUtilities {
public:
	/// <summary>
	/// Splits a string into substrings using a delimiter.
	/// </summary>
	/// <param name="input">String to split</param>
	/// <param name="delimiter">Character to split on</param>
	/// <returns>Vector of substrings (empty strings included if consecutive delimiters)</returns>
	/// <remarks>
	/// Example: Split("a,b,c", ',') returns {"a", "b", "c"}
	/// </remarks>
	[[nodiscard]] static vector<string> Split(string_view input, char delimiter) {
		vector<string> result;
		size_t index = 0;
		size_t lastIndex = 0;
		while ((index = input.find(delimiter, index)) != string_view::npos) {
			result.emplace_back(input.substr(lastIndex, index - lastIndex));
			index++;
			lastIndex = index;
		}
		result.emplace_back(input.substr(lastIndex));
		return result;
	}

	/// <summary>
	/// Extracts the Nth segment from a delimited string without constructing the full split vector.
	/// </summary>
	/// <param name="input">String to search</param>
	/// <param name="delimiter">Character to split on</param>
	/// <param name="n">Zero-based index of the segment to retrieve</param>
	/// <returns>The Nth segment, or empty string if n exceeds the number of segments</returns>
	/// <remarks>
	/// More efficient than Split()[n] when only one segment is needed.
	/// Example: GetNthSegment("a\nb\nc", '\n', 1) returns "b"
	/// </remarks>
	[[nodiscard]] static string GetNthSegment(string_view input, char delimiter, int n) {
		size_t start = 0;
		for (int i = 0; i < n; i++) {
			size_t pos = input.find(delimiter, start);
			if (pos == string_view::npos) {
				return "";
			}
			start = pos + 1;
		}
		size_t end = input.find(delimiter, start);
		if (end == string_view::npos) {
			return string(input.substr(start));
		}
		return string(input.substr(start, end - start));
	}

	/// <summary>
	/// Extracts the Nth segment from a delimited string as a string_view (zero-copy).
	/// </summary>
	/// <param name="input">String to search (must outlive the returned view)</param>
	/// <param name="delimiter">Character to split on</param>
	/// <param name="n">Zero-based index of the segment to retrieve</param>
	/// <returns>View of the Nth segment, or empty view if n exceeds the number of segments</returns>
	[[nodiscard]] static string_view GetNthSegmentView(string_view input, char delimiter, size_t n) {
		size_t start = 0;
		for (size_t i = 0; i < n; i++) {
			size_t pos = input.find(delimiter, start);
			if (pos == string_view::npos) {
				return {};
			}
			start = pos + 1;
		}
		size_t end = input.find(delimiter, start);
		if (end == string_view::npos) {
			return input.substr(start);
		}
		return input.substr(start, end - start);
	}

	/// <summary>
	/// Counts the number of segments in a delimited string without splitting.
	/// </summary>
	/// <param name="input">String to search</param>
	/// <param name="delimiter">Character to count</param>
	/// <returns>Number of segments (delimiter count + 1, or 0 for empty input)</returns>
	[[nodiscard]] static size_t CountSegments(string_view input, char delimiter) {
		if (input.empty()) {
			return 0;
		}
		return std::ranges::count(input, delimiter) + 1;
	}

	/// <summary>
	/// Removes leading whitespace (tabs and spaces) from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with leading whitespace removed</returns>
	[[nodiscard]] static string TrimLeft(string_view str) {
		size_t startIndex = str.find_first_not_of("\t ");
		if (startIndex == string_view::npos) {
			return "";
		} else if (startIndex > 0) {
			return string(str.substr(startIndex));
		}
		return string(str);
	}

	/// <summary>
	/// Removes trailing whitespace (tabs, carriage returns, newlines, spaces) from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with trailing whitespace removed</returns>
	[[nodiscard]] static string TrimRight(string_view str) {
		size_t endIndex = str.find_last_not_of("\t\r\n ");
		if (endIndex == string_view::npos) {
			return "";
		} else if (endIndex > 0) {
			return string(str.substr(0, endIndex + 1));
		}
		return string(str);
	}

	/// <summary>
	/// Removes leading and trailing whitespace from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with leading and trailing whitespace removed</returns>
	[[nodiscard]] static string Trim(string_view str) {
		return TrimLeft(TrimRight(str));
	}

	/// <summary>
	/// Converts a string to uppercase using std::ranges::transform (C++23).
	/// </summary>
	/// <param name="str">String to convert</param>
	/// <returns>Uppercase version of the string</returns>
	/// <remarks>Uses std::toupper with proper unsigned char casting to avoid UB.</remarks>
	[[nodiscard]] static string ToUpper(string str) {
		std::ranges::transform(str, str.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
		return str;
	}

	/// <summary>
	/// Converts a string to lowercase using std::ranges::transform (C++23).
	/// </summary>
	/// <param name="str">String to convert</param>
	/// <returns>Lowercase version of the string</returns>
	/// <remarks>Uses std::tolower with proper unsigned char casting to avoid UB.</remarks>
	[[nodiscard]] static string ToLower(string str) {
		std::ranges::transform(str, str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return str;
	}

	/// <summary>
	/// Copies a string to a fixed-size buffer.
	/// </summary>
	/// <param name="str">Source string</param>
	/// <param name="outBuffer">Destination buffer</param>
	/// <param name="maxSize">Maximum bytes to copy</param>
	/// <remarks>
	/// Always null-terminates when maxSize > 0 to keep interop string reads safe.
	/// </remarks>
	static void CopyToBuffer(string_view str, char* outBuffer, uint32_t maxSize) {
		if (!outBuffer || maxSize == 0) {
			return;
		}

		uint32_t copySize = std::min<uint32_t>((uint32_t)str.size(), maxSize - 1);
		if (copySize > 0) {
			memcpy(outBuffer, str.data(), copySize);
		}
		outBuffer[copySize] = '\0';
	}

	/// <summary>
	/// Checks if a string starts with specified content (C++23 std::string::starts_with).
	/// </summary>
	/// <param name="str">String to check</param>
	/// <param name="content">Prefix to look for</param>
	/// <returns>True if str starts with content</returns>
	[[nodiscard]] static bool StartsWith(string_view str, string_view content) {
		return str.starts_with(content);
	}

	/// <summary>
	/// Checks if a string ends with specified content (C++23 std::string::ends_with).
	/// </summary>
	/// <param name="str">String to check</param>
	/// <param name="content">Suffix to look for</param>
	/// <returns>True if str ends with content</returns>
	[[nodiscard]] static bool EndsWith(string_view str, string_view content) {
		return str.ends_with(content);
	}

	/// <summary>
	/// Checks if a string contains specified content (C++23 std::string::contains).
	/// </summary>
	/// <param name="str">String to search in</param>
	/// <param name="content">Substring to look for</param>
	/// <returns>True if str contains content</returns>
	[[nodiscard]] static bool Contains(const string& str, string_view content) {
		return str.contains(content);
	}

	/// <summary>
	/// Creates a string from a C-string with maximum length.
	/// </summary>
	/// <param name="src">Source C-string</param>
	/// <param name="maxLen">Maximum length to read</param>
	/// <returns>String up to first null terminator or maxLen, whichever is shorter</returns>
	[[nodiscard]] static string GetString(char* src, uint32_t maxLen) {
		return GetString((uint8_t*)src, maxLen);
	}

	/// <summary>
	/// Creates a string from a byte array with maximum length.
	/// </summary>
	/// <param name="src">Source byte array</param>
	/// <param name="maxLen">Maximum length to read</param>
	/// <returns>String up to first null byte or maxLen, whichever is shorter</returns>
	/// <remarks>Useful for reading null-terminated strings from binary data.</remarks>
	[[nodiscard]] static string GetString(uint8_t* src, int maxLen) {
		for (int i = 0; i < maxLen; i++) {
			if (src[i] == 0) {
				return string(src, src + i);
			}
		}
		return string(src, src + maxLen);
	}
};
