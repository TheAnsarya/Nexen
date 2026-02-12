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
			result.push_back(string(input.substr(lastIndex, index - lastIndex)));
			index++;
			lastIndex = index;
		}
		result.push_back(string(input.substr(lastIndex)));
		return result;
	}

	/// <summary>
	/// Removes leading whitespace (tabs and spaces) from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with leading whitespace removed</returns>
	[[nodiscard]] static string TrimLeft(const string& str) {
		size_t startIndex = str.find_first_not_of("\t ");
		if (startIndex == string::npos) {
			return "";
		} else if (startIndex > 0) {
			return str.substr(startIndex);
		}
		return str;
	}

	/// <summary>
	/// Removes trailing whitespace (tabs, carriage returns, newlines, spaces) from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with trailing whitespace removed</returns>
	[[nodiscard]] static string TrimRight(const string& str) {
		size_t endIndex = str.find_last_not_of("\t\r\n ");
		if (endIndex == string::npos) {
			return "";
		} else if (endIndex > 0) {
			return str.substr(0, endIndex + 1);
		}
		return str;
	}

	/// <summary>
	/// Removes leading and trailing whitespace from a string.
	/// </summary>
	/// <param name="str">String to trim</param>
	/// <returns>String with leading and trailing whitespace removed</returns>
	[[nodiscard]] static string Trim(const string& str) {
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
	/// Does not null-terminate. Copies min(str.size(), maxSize) bytes.
	/// Caller is responsible for ensuring buffer is large enough.
	/// </remarks>
	static void CopyToBuffer(const string& str, char* outBuffer, uint32_t maxSize) {
		memcpy(outBuffer, str.c_str(), std::min<uint32_t>((uint32_t)str.size(), maxSize));
	}

	/// <summary>
	/// Checks if a string starts with specified content (C++23 std::string::starts_with).
	/// </summary>
	/// <param name="str">String to check</param>
	/// <param name="content">Prefix to look for</param>
	/// <returns>True if str starts with content</returns>
	[[nodiscard]] static bool StartsWith(string& str, const char* content) {
		return str.starts_with(content);
	}

	/// <summary>
	/// Checks if a string ends with specified content (C++23 std::string::ends_with).
	/// </summary>
	/// <param name="str">String to check</param>
	/// <param name="content">Suffix to look for</param>
	/// <returns>True if str ends with content</returns>
	[[nodiscard]] static bool EndsWith(string& str, const char* content) {
		return str.ends_with(content);
	}

	/// <summary>
	/// Checks if a string contains specified content (C++23 std::string::contains).
	/// </summary>
	/// <param name="str">String to search in</param>
	/// <param name="content">Substring to look for</param>
	/// <returns>True if str contains content</returns>
	[[nodiscard]] static bool Contains(string& str, const char* content) {
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
